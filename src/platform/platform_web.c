#include "platform.h"

#ifdef PLATFORM_WEB

#include <emscripten.h>
#include <emscripten/html5.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// 웹용 가상 콘솔 시뮬레이션
#define SCREEN_WIDTH 120
#define SCREEN_HEIGHT 50
static char g_screen_buffer[SCREEN_HEIGHT][SCREEN_WIDTH + 1];
static color_t g_color_buffer[SCREEN_HEIGHT][SCREEN_WIDTH];
static int g_cursor_x = 0, g_cursor_y = 0;
static color_t g_current_color = COLOR_WHITE;
static bool g_screen_dirty = false;

// 키보드 입력 상태
static game_key_t g_last_key = KEY_NONE;
static bool g_keys_pressed[32] = {false};

// 키보드 콜백 함수들
EM_BOOL keydown_callback(int eventType, const EmscriptenKeyboardEvent *e, void *userData);
EM_BOOL keyup_callback(int eventType, const EmscriptenKeyboardEvent *e, void *userData);

bool platform_init(void) {
    printf("웹 플랫폼 초기화 시작...\n");
    
    // 화면 버퍼 초기화 - UTF-8 호환
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            g_screen_buffer[y][x] = ' ';
            g_color_buffer[y][x] = COLOR_WHITE;
        }
        g_screen_buffer[y][SCREEN_WIDTH] = '\0'; // null terminator
    }
    
    printf("화면 버퍼 초기화 완료\n");
    
    // 키보드 이벤트 리스너 등록
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, 1, keydown_callback);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, 1, keyup_callback);
    
    printf("키보드 이벤트 리스너 등록 완료\n");
    
    // 문서와 출력 영역에 포커스 설정
    EM_ASM({
        try {
            // 문서에 포커스 설정
            document.body.focus();
            document.body.setAttribute('tabindex', '0');
            
            // 출력 영역 설정
            var output = document.getElementById('output');
            if (output) {
                output.setAttribute('tabindex', '0');
                output.style.outline = 'none';
                output.focus();
            }
            
            // UTF-8 인코딩 확인
            document.characterSet = 'UTF-8';
            
            console.log('웹 플랫폼 설정 완료');
            console.log('인코딩:', document.characterSet);
        } catch (e) {
            console.error('웹 플랫폼 설정 오류:', e);
        }
    });
    
    srand((unsigned int)time(NULL));
    
    printf("웹 플랫폼 초기화 완료\n");
    return true;
}

void platform_cleanup(void) {
    // 웹에서는 정리할 내용이 없음
}

void platform_clear_screen(void) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            g_screen_buffer[y][x] = ' ';
            g_color_buffer[y][x] = COLOR_BLACK;
        }
    }
    g_cursor_x = g_cursor_y = 0;
    g_screen_dirty = true;
    
    // HTML 출력 영역도 지우기
    EM_ASM({
        var output = document.getElementById('output');
        if (output) {
            output.textContent = '';
        }
    });
}

void platform_goto_xy(int x, int y) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        g_cursor_x = x;
        g_cursor_y = y;
    }
}

void platform_set_color(color_t color) {
    g_current_color = color;
}

void platform_reset_color(void) {
    g_current_color = COLOR_WHITE;
}

void platform_print(const char* text) {
    if (!text) return;
    
    int len = strlen(text);
    for (int i = 0; i < len && g_cursor_x < SCREEN_WIDTH && g_cursor_y < SCREEN_HEIGHT; i++) {
        char ch = text[i];
        if (ch == '\n') {
            g_cursor_x = 0;
            g_cursor_y++;
            if (g_cursor_y >= SCREEN_HEIGHT) break;
        } else if (ch == '\r') {
            g_cursor_x = 0;
        } else {
            // 모든 문자를 허용 (한글, 특수문자, 이모지 포함)
            g_screen_buffer[g_cursor_y][g_cursor_x] = ch;
            g_color_buffer[g_cursor_y][g_cursor_x] = g_current_color;
            g_cursor_x++;
            if (g_cursor_x >= SCREEN_WIDTH) {
                g_cursor_x = 0;
                g_cursor_y++;
                if (g_cursor_y >= SCREEN_HEIGHT) break;
            }
        }
    }
    g_screen_dirty = true;
    
    // 브라우저 콘솔에도 출력 (디버깅용)
    printf("%s", text);
}

void platform_print_at(int x, int y, const char* text) {
    platform_goto_xy(x, y);
    platform_print(text);
}

void platform_hide_cursor(void) {
    // 웹에서는 적용 불가
}

void platform_show_cursor(void) {
    // 웹에서는 적용 불가
}

void platform_set_console_size(int width, int height) {
    // CSS를 통해 캔버스 크기 조정
    EM_ASM({
        var canvas = document.getElementById('gameCanvas');
        if (canvas) {
            canvas.width = $0 * 8;
            canvas.height = $1 * 16;
        }
    }, width, height);
}

// 키 매핑 함수
static game_key_t map_key_code(int keyCode) {
    switch (keyCode) {
        case 38: case 87: return KEY_UP;     // Arrow Up, W
        case 40: case 83: return KEY_DOWN;   // Arrow Down, S
        case 37: case 65: return KEY_LEFT;   // Arrow Left, A
        case 39: case 68: return KEY_RIGHT;  // Arrow Right, D
        case 13: return KEY_ENTER;           // Enter
        case 27: return KEY_ESC;             // Escape
        case 32: return KEY_SPACE;           // Space
        case 49: return KEY_1;               // 1
        case 50: return KEY_2;               // 2
        case 51: return KEY_3;               // 3
        case 52: return KEY_4;               // 4
        default: return KEY_NONE;
    }
}

EM_BOOL keydown_callback(int eventType, const EmscriptenKeyboardEvent *e, void *userData) {
    if (!e) return EM_FALSE;
    
    game_key_t key = map_key_code(e->keyCode);
    if (key != KEY_NONE) {
        g_last_key = key;
        if (key >= 0 && key < 32) { // 안전한 범위 확인
            g_keys_pressed[key] = true;
        }
        
        // 화살표 키와 스페이스 키의 기본 동작 방지
        if (e->keyCode >= 37 && e->keyCode <= 40 || e->keyCode == 32) {
            return EM_TRUE;
        }
    }
    return EM_FALSE;
}

EM_BOOL keyup_callback(int eventType, const EmscriptenKeyboardEvent *e, void *userData) {
    if (!e) return EM_FALSE;
    
    game_key_t key = map_key_code(e->keyCode);
    if (key != KEY_NONE && key >= 0 && key < 32) { // 안전한 범위 확인
        g_keys_pressed[key] = false;
    }
    return EM_FALSE;
}

game_key_t platform_get_key_pressed(void) {
    game_key_t key = g_last_key;
    g_last_key = KEY_NONE;
    return key;
}

bool platform_is_key_down(game_key_t key) {
    if (key >= 0 && key < 32) {
        return g_keys_pressed[key];
    }
    return false;
}

void platform_sleep(uint32_t ms) {
    // 비동기 슬립 (브라우저에서 차단 방지)
    emscripten_sleep(ms);
}

uint64_t platform_get_time_ms(void) {
    return (uint64_t)emscripten_get_now();
}

// 스레딩 작업 (웹용 더미 구현)
thread_handle_t platform_create_thread(thread_func_t func, void* arg) {
    thread_handle_t handle = {0};
    // 웹에서는 실제 스레드 대신 메인 루프에서 처리
    return handle;
}

void platform_join_thread(thread_handle_t thread) {
    (void)thread;
}

void platform_detach_thread(thread_handle_t thread) {
    (void)thread;
}

bool platform_thread_running(thread_handle_t thread) {
    (void)thread;
    return false;
}

// 뮤텍스 작업 (단일 스레드 웹에서는 무동작)
mutex_handle_t platform_create_mutex(void) {
    mutex_handle_t handle = {0};
    return handle;
}

void platform_destroy_mutex(mutex_handle_t mutex) {
    (void)mutex;
}

void platform_lock_mutex(mutex_handle_t mutex) {
    (void)mutex;
}

void platform_unlock_mutex(mutex_handle_t mutex) {
    (void)mutex;
}

// 유틸리티 함수
int platform_random(int min, int max) {
    return min + rand() % (max - min + 1);
}

void platform_seed_random(uint32_t seed) {
    srand(seed);
}

// 웹 플랫폼용 화면 출력 함수 - UTF-8 완전 지원
void platform_present_buffer(void) {
    if (!g_screen_dirty) return;
    
    // 전체 화면 텍스트를 하나의 문자열로 구성
    static char full_screen_text[SCREEN_HEIGHT * (SCREEN_WIDTH + 10) + 1000];
    full_screen_text[0] = '\0';
    
    // 색상 정보도 함께 전송
    strcat(full_screen_text, "<div style='font-family: monospace; line-height: 1.2; white-space: pre; color: #00ff00;'>");
    
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        // 각 라인을 null-terminated 문자열로 만들기
        char line[SCREEN_WIDTH + 1];
        memcpy(line, g_screen_buffer[y], SCREEN_WIDTH);
        line[SCREEN_WIDTH] = '\0';
        
        // 라인 끝의 공백 제거
        int end = SCREEN_WIDTH - 1;
        while (end >= 0 && line[end] == ' ') {
            line[end] = '\0';
            end--;
        }
        
        // HTML 특수 문자 처리하면서 라인 추가
        for (int i = 0; line[i] != '\0'; i++) {
            char ch = line[i];
            if (ch == '<') {
                strcat(full_screen_text, "&lt;");
            } else if (ch == '>') {
                strcat(full_screen_text, "&gt;");
            } else if (ch == '&') {
                strcat(full_screen_text, "&amp;");
            } else if (ch == ' ') {
                strcat(full_screen_text, "&nbsp;");
            } else {
                // 문자를 그대로 추가 (UTF-8 지원)
                char temp[2] = {ch, '\0'};
                strcat(full_screen_text, temp);
            }
        }
        
        if (y < SCREEN_HEIGHT - 1) {
            strcat(full_screen_text, "\n");
        }
    }
    
    strcat(full_screen_text, "</div>");
    
    // JavaScript로 UTF-8 텍스트 전송
    EM_ASM({
        try {
            var output = document.getElementById('output');
            if (output) {
                // UTF-8 문자열을 그대로 사용
                var text = UTF8ToString($0);
                output.innerHTML = text;
            }
        } catch (e) {
            console.error('화면 업데이트 오류:', e);
        }
    }, full_screen_text);
    
    g_screen_dirty = false;
}

#endif // PLATFORM_WEB
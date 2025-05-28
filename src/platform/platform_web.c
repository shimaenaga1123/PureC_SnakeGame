#include "platform.h"

#ifdef PLATFORM_WEB

#include <emscripten.h>
#include <emscripten/html5.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// 웹용 간단한 콘솔 시뮬레이션
static char g_screen_buffer[50][100];
static int g_cursor_x = 0, g_cursor_y = 0;
static color_t g_current_color = COLOR_WHITE;

// 키보드 입력 콜백 전방 선언
EM_BOOL keydown_callback(int eventType, const EmscriptenKeyboardEvent *e, void *userData);

bool platform_init(void) {
    memset(g_screen_buffer, ' ', sizeof(g_screen_buffer));
    for (int i = 0; i < 50; i++) {
        g_screen_buffer[i][99] = '\0';
    }
    
    // 캔버스 및 입력 설정
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 1, keydown_callback);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 1, NULL);
    
    srand((unsigned int)time(NULL));
    return true;
}

void platform_cleanup(void) {
    // 웹에서는 정리할 내용이 없음
}

void platform_clear_screen(void) {
    memset(g_screen_buffer, ' ', sizeof(g_screen_buffer));
    g_cursor_x = g_cursor_y = 0;
    
    // 웹 콘솔/캔버스 지우기
    EM_ASM({
        if (typeof clearConsole !== 'undefined') {
            clearConsole();
        }
    });
}

void platform_goto_xy(int x, int y) {
    g_cursor_x = x;
    g_cursor_y = y;
}

void platform_set_color(color_t color) {
    g_current_color = color;
}

void platform_reset_color(void) {
    g_current_color = COLOR_WHITE;
}

void platform_print(const char* text) {
    if (g_cursor_y < 50 && g_cursor_x < 99) {
        int len = strlen(text);
        int available = 99 - g_cursor_x;
        int copy_len = len < available ? len : available;
        
        memcpy(&g_screen_buffer[g_cursor_y][g_cursor_x], text, copy_len);
        g_cursor_x += copy_len;
    }
    
    // 브라우저 콘솔에 출력
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
    // CSS/HTML을 통해 처리
    EM_ASM({
        var canvas = document.getElementById('gameCanvas');
        if (canvas) {
            canvas.width = $0 * 8;
            canvas.height = $1 * 16;
        }
    }, width, height);
}

static game_key_t g_last_key = KEY_NONE;

EM_BOOL keydown_callback(int eventType, const EmscriptenKeyboardEvent *e, void *userData) {
    switch (e->keyCode) {
        case 38: g_last_key = KEY_UP; break;
        case 40: g_last_key = KEY_DOWN; break;
        case 37: g_last_key = KEY_LEFT; break;
        case 39: g_last_key = KEY_RIGHT; break;
        case 13: g_last_key = KEY_ENTER; break;
        case 27: g_last_key = KEY_ESC; break;
        case 32: g_last_key = KEY_SPACE; break;
        case 87: g_last_key = KEY_W; break;
        case 65: g_last_key = KEY_A; break;
        case 83: g_last_key = KEY_S; break;
        case 68: g_last_key = KEY_D; break;
        case 49: g_last_key = KEY_1; break;
        case 50: g_last_key = KEY_2; break;
        case 51: g_last_key = KEY_3; break;
        case 52: g_last_key = KEY_4; break;
    }
    return EM_TRUE;
}

game_key_t platform_get_key_pressed(void) {
    game_key_t key = g_last_key;
    g_last_key = KEY_NONE;
    return key;
}

bool platform_is_key_down(game_key_t key) {
    // 웹용 간소화 - 실제로는 키 상태 추적이 필요함
    return false;
}

void platform_sleep(uint32_t ms) {
    // 적절한 비동기 동작을 위해 emscripten_sleep 사용
    emscripten_sleep(ms);
}

uint64_t platform_get_time_ms(void) {
    return (uint64_t)emscripten_get_now();
}

// 스레딩 작업 (웹용 간소화)
thread_handle_t platform_create_thread(thread_func_t func, void* arg) {
    // 실제 스레딩을 위해서는 웹 워커나 비동기 콜백이 필요함
    // 현재는 더미 핸들 반환
    thread_handle_t handle = {0};
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

// Web 플랫폼용 더블 버퍼링 함수 (빈 구현)
void platform_present_buffer(void) {
    // Web에서는 DOM 업데이트가 자동으로 처리됨
}

#endif // PLATFORM_WEB
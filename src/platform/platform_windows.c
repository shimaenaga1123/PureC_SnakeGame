#include "platform.h"

#ifdef PLATFORM_WINDOWS

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <io.h>
#include <fcntl.h>

static HANDLE g_console_handle = NULL;
static HANDLE g_input_handle = NULL;
static HANDLE g_back_buffer = NULL;  // 더블 버퍼링용 백 버퍼
static COORD g_buffer_size = {0, 0};
static SMALL_RECT g_write_region = {0, 0, 0, 0};
static bool g_double_buffering_enabled = false;

// 화면 변경 추적을 위한 버퍼
static char** g_screen_text_buffer = NULL;
static WORD* g_screen_attr_buffer = NULL;
static bool g_screen_initialized = false;

bool platform_init(void) {
    g_console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    g_input_handle = GetStdHandle(STD_INPUT_HANDLE);
    
    if (g_console_handle == INVALID_HANDLE_VALUE || g_input_handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    // UTF-8 출력을 위한 설정
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // 콘솔 모드 설정 (가상 터미널 처리 활성화)
    DWORD console_mode;
    if (GetConsoleMode(g_console_handle, &console_mode)) {
        console_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(g_console_handle, console_mode);
    }

    // 화면 버퍼 크기 가져오기
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(g_console_handle, &csbi)) {
        g_buffer_size.X = csbi.dwSize.X;
        g_buffer_size.Y = csbi.dwSize.Y;

        // 텍스트 추적 버퍼 할당
        g_screen_text_buffer = (char**)malloc(g_buffer_size.Y * sizeof(char*));
        g_screen_attr_buffer = (WORD*)malloc(g_buffer_size.X * g_buffer_size.Y * sizeof(WORD));

        if (g_screen_text_buffer && g_screen_attr_buffer) {
            for (int y = 0; y < g_buffer_size.Y; y++) {
                g_screen_text_buffer[y] = (char*)malloc(g_buffer_size.X * 4 + 1); // UTF-8용 충분한 공간
                if (g_screen_text_buffer[y]) {
                    memset(g_screen_text_buffer[y], 0, g_buffer_size.X * 4 + 1);
                }
            }

            // 속성 버퍼 초기화
            WORD default_attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
            for (int i = 0; i < g_buffer_size.X * g_buffer_size.Y; i++) {
                g_screen_attr_buffer[i] = default_attr;
            }

            g_screen_initialized = true;
        }
    }
    
    srand((unsigned int)time(NULL));
    return true;
}

void platform_cleanup(void) {
    platform_reset_color();
    platform_show_cursor();

    // 화면 추적 버퍼 해제
    if (g_screen_text_buffer) {
        for (int y = 0; y < g_buffer_size.Y; y++) {
            if (g_screen_text_buffer[y]) {
                free(g_screen_text_buffer[y]);
            }
        }
        free(g_screen_text_buffer);
        g_screen_text_buffer = NULL;
    }

    if (g_screen_attr_buffer) {
        free(g_screen_attr_buffer);
        g_screen_attr_buffer = NULL;
    }

    g_screen_initialized = false;
}

void platform_clear_screen(void) {
    if (g_screen_initialized) {
        // 화면 추적 버퍼도 클리어
        for (int y = 0; y < g_buffer_size.Y; y++) {
            if (g_screen_text_buffer[y]) {
                memset(g_screen_text_buffer[y], 0, g_buffer_size.X * 4 + 1);
            }
        }

        WORD default_attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        for (int i = 0; i < g_buffer_size.X * g_buffer_size.Y; i++) {
            g_screen_attr_buffer[i] = default_attr;
        }
    }

    // 한 번만 화면 클리어
    printf("\033[2J\033[H");
    fflush(stdout);
}

static COORD g_current_pos = {0, 0};
static WORD g_current_attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

void platform_goto_xy(int x, int y) {
    g_current_pos.X = (SHORT)x;
    g_current_pos.Y = (SHORT)y;
}

void platform_set_color(color_t color) {
    g_current_attr = (WORD)color;
}

void platform_reset_color(void) {
    g_current_attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    printf("\033[0m");
    fflush(stdout);
}

void platform_print(const char* text) {
    if (!text) return;

    // 현재 위치가 유효한 범위 내인지 확인
    if (g_current_pos.X < 0 || g_current_pos.X >= g_buffer_size.X ||
        g_current_pos.Y < 0 || g_current_pos.Y >= g_buffer_size.Y) {
        return;
    }

    if (g_screen_initialized && g_screen_text_buffer && g_screen_attr_buffer) {
        // 현재 위치의 기존 내용과 비교
        int buffer_index = g_current_pos.Y * g_buffer_size.X + g_current_pos.X;
        bool content_changed = false;
        bool attr_changed = false;

        // 텍스트 변경 확인
        char* current_line = g_screen_text_buffer[g_current_pos.Y];
        int start_pos = g_current_pos.X * 4; // UTF-8을 위한 충분한 공간

        if (strncmp(&current_line[start_pos], text, strlen(text)) != 0) {
            content_changed = true;
            // 새 텍스트 저장
            strncpy(&current_line[start_pos], text, strlen(text));
        }

        // 속성 변경 확인
        if (g_screen_attr_buffer[buffer_index] != g_current_attr) {
            attr_changed = true;
            g_screen_attr_buffer[buffer_index] = g_current_attr;
        }

        // 변경된 내용만 출력 (깜빡거림 최소화)
        if (content_changed || attr_changed) {
            printf("\033[%d;%dH", g_current_pos.Y + 1, g_current_pos.X + 1);

            // 색상 설정
            const char* color_codes[] = {
                "\033[30m", "\033[34m", "\033[32m", "\033[36m",
                "\033[31m", "\033[35m", "\033[33m", "\033[37m",
                "\033[90m", "\033[94m", "\033[92m", "\033[96m",
                "\033[91m", "\033[95m", "\033[93m", "\033[97m"
            };

            if (g_current_attr < 16) {
                printf("%s", color_codes[g_current_attr]);
            }
            printf("%s", text);
        }
    } else {
        // fallback: 직접 출력
        printf("\033[%d;%dH", g_current_pos.Y + 1, g_current_pos.X + 1);

        const char* color_codes[] = {
            "\033[30m", "\033[34m", "\033[32m", "\033[36m",
            "\033[31m", "\033[35m", "\033[33m", "\033[37m",
            "\033[90m", "\033[94m", "\033[92m", "\033[96m",
            "\033[91m", "\033[95m", "\033[93m", "\033[97m"
        };

        if (g_current_attr < 16) {
            printf("%s", color_codes[g_current_attr]);
        }

        printf("%s", text);
    }

    // 커서 위치 업데이트
    g_current_pos.X += strlen(text);
}

void platform_print_at(int x, int y, const char* text) {
    platform_goto_xy(x, y);
    platform_print(text);
}

void platform_hide_cursor(void) {
    printf("\033[?25l");
    fflush(stdout);
}

void platform_show_cursor(void) {
    printf("\033[?25h");
    fflush(stdout);
}

void platform_set_console_size(int width, int height) {
    COORD newBuf = {(SHORT)width, (SHORT)height};
    SetConsoleScreenBufferSize(g_console_handle, newBuf);

    SMALL_RECT win = {0, 0, (SHORT)(width-1), (SHORT)(height-1)};
    SetConsoleWindowInfo(g_console_handle, TRUE, &win);
}

// 모든 변경사항을 한 번에 출력 (깜빡거림 방지)
void platform_present_buffer(void) {
    static bool first_present = true;

    if (first_present) {
        // 첫 번째 출력시에만 전체 화면 갱신
        fflush(stdout);
        first_present = false;
    } else {
        // 이후에는 버퍼링된 출력만 플러시
        fflush(stdout);
    }
}

game_key_t platform_get_key_pressed(void) {
    if (!_kbhit()) return KEY_NONE;
    
    int ch = _getch();
    if (ch == 224) { // 확장 키
        ch = _getch();
        switch (ch) {
            case 72: return KEY_UP;
            case 80: return KEY_DOWN;
            case 75: return KEY_LEFT;
            case 77: return KEY_RIGHT;
        }
    } else {
        switch (ch) {
            case 13: return KEY_ENTER;
            case 27: return KEY_ESC;
            case 32: return KEY_SPACE;
            case 'w': case 'W': return KEY_W;
            case 'a': case 'A': return KEY_A;
            case 's': case 'S': return KEY_S;
            case 'd': case 'D': return KEY_D;
            case '1': return KEY_1;
            case '2': return KEY_2;
            case '3': return KEY_3;
            case '4': return KEY_4;
        }
    }
    return KEY_NONE;
}

bool platform_is_key_down(game_key_t key) {
    int vk_key = 0;
    switch (key) {
        case KEY_UP: vk_key = VK_UP; break;
        case KEY_DOWN: vk_key = VK_DOWN; break;
        case KEY_LEFT: vk_key = VK_LEFT; break;
        case KEY_RIGHT: vk_key = VK_RIGHT; break;
        case KEY_ENTER: vk_key = VK_RETURN; break;
        case KEY_ESC: vk_key = VK_ESCAPE; break;
        case KEY_SPACE: vk_key = VK_SPACE; break;
        case KEY_W: vk_key = 'W'; break;
        case KEY_A: vk_key = 'A'; break;
        case KEY_S: vk_key = 'S'; break;
        case KEY_D: vk_key = 'D'; break;
        default: return false;
    }
    return (GetAsyncKeyState(vk_key) & 0x8000) != 0;
}

void platform_sleep(uint32_t ms) {
    Sleep(ms);
}

uint64_t platform_get_time_ms(void) {
    return GetTickCount64();
}

// 스레딩 작업
thread_handle_t platform_create_thread(thread_func_t func, void* arg) {
    thread_handle_t handle = {0};
    handle.handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, arg, 0, &handle.id);
    return handle;
}

void platform_join_thread(thread_handle_t thread) {
    if (thread.handle) {
        WaitForSingleObject(thread.handle, INFINITE);
        CloseHandle(thread.handle);
    }
}

void platform_detach_thread(thread_handle_t thread) {
    if (thread.handle) {
        CloseHandle(thread.handle);
    }
}

bool platform_thread_running(thread_handle_t thread) {
    if (!thread.handle) return false;
    DWORD exit_code;
    GetExitCodeThread(thread.handle, &exit_code);
    return exit_code == STILL_ACTIVE;
}

// 뮤텍스 작업
mutex_handle_t platform_create_mutex(void) {
    mutex_handle_t handle = {0};
    handle.handle = CreateMutex(NULL, FALSE, NULL);
    return handle;
}

void platform_destroy_mutex(mutex_handle_t mutex) {
    if (mutex.handle) {
        CloseHandle(mutex.handle);
    }
}

void platform_lock_mutex(mutex_handle_t mutex) {
    if (mutex.handle) {
        WaitForSingleObject(mutex.handle, INFINITE);
    }
}

void platform_unlock_mutex(mutex_handle_t mutex) {
    if (mutex.handle) {
        ReleaseMutex(mutex.handle);
    }
}

// 유틸리티 함수
int platform_random(int min, int max) {
    return min + rand() % (max - min + 1);
}

void platform_seed_random(uint32_t seed) {
    srand(seed);
}

#endif // PLATFORM_WINDOWS
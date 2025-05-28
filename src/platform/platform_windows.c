#include "platform.h"

#ifdef PLATFORM_WINDOWS

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>

static HANDLE g_console_handle = NULL;
static HANDLE g_input_handle = NULL;
static HANDLE g_back_buffer = NULL;  // 더블 버퍼링용 백 버퍼
static CHAR_INFO* g_screen_buffer = NULL;  // 화면 버퍼
static COORD g_buffer_size = {0, 0};
static SMALL_RECT g_write_region = {0, 0, 0, 0};
static bool g_double_buffering_enabled = false;

bool platform_init(void) {
    g_console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    g_input_handle = GetStdHandle(STD_INPUT_HANDLE);
    
    if (g_console_handle == INVALID_HANDLE_VALUE || g_input_handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    // UTF-8 코드 페이지 설정
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    // 콘솔 모드 설정 (깜빡임 최소화)
    DWORD console_mode;
    GetConsoleMode(g_console_handle, &console_mode);
    console_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(g_console_handle, console_mode);
    
    // 더블 버퍼링 초기화
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(g_console_handle, &csbi)) {
        g_buffer_size.X = csbi.dwSize.X;
        g_buffer_size.Y = csbi.dwSize.Y;
        g_write_region.Left = 0;
        g_write_region.Top = 0;
        g_write_region.Right = g_buffer_size.X - 1;
        g_write_region.Bottom = g_buffer_size.Y - 1;
        
        // 백 버퍼 생성
        g_back_buffer = CreateConsoleScreenBuffer(
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            CONSOLE_TEXTMODE_BUFFER,
            NULL
        );
        
        if (g_back_buffer != INVALID_HANDLE_VALUE) {
            // 화면 버퍼 메모리 할당
            size_t buffer_size = g_buffer_size.X * g_buffer_size.Y * sizeof(CHAR_INFO);
            g_screen_buffer = (CHAR_INFO*)malloc(buffer_size);
            
            if (g_screen_buffer) {
                // 버퍼 초기화
                for (int i = 0; i < g_buffer_size.X * g_buffer_size.Y; i++) {
                    g_screen_buffer[i].Char.UnicodeChar = L' ';
                    g_screen_buffer[i].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
                }
                g_double_buffering_enabled = true;
            }
        }
    }
    
    srand((unsigned int)time(NULL));
    return true;
}

void platform_cleanup(void) {
    platform_reset_color();
    platform_show_cursor();
    
    // 더블 버퍼링 리소스 정리
    if (g_screen_buffer) {
        free(g_screen_buffer);
        g_screen_buffer = NULL;
    }
    
    if (g_back_buffer != INVALID_HANDLE_VALUE) {
        CloseHandle(g_back_buffer);
        g_back_buffer = INVALID_HANDLE_VALUE;
    }
    
    g_double_buffering_enabled = false;
}

void platform_clear_screen(void) {
    if (g_double_buffering_enabled && g_screen_buffer) {
        // 더블 버퍼링 사용 시 백 버퍼만 클리어
        for (int i = 0; i < g_buffer_size.X * g_buffer_size.Y; i++) {
            g_screen_buffer[i].Char.UnicodeChar = L' ';
            g_screen_buffer[i].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        }
    } else {
        // 기존 방식 (fallback)
        COORD coord = {0, 0};
        DWORD written;
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        
        GetConsoleScreenBufferInfo(g_console_handle, &csbi);
        DWORD size = csbi.dwSize.X * csbi.dwSize.Y;
        
        FillConsoleOutputCharacter(g_console_handle, ' ', size, coord, &written);
        FillConsoleOutputAttribute(g_console_handle, csbi.wAttributes, size, coord, &written);
        SetConsoleCursorPosition(g_console_handle, coord);
    }
}

static COORD g_current_pos = {0, 0};
static WORD g_current_attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

void platform_goto_xy(int x, int y) {
    g_current_pos.X = (SHORT)x;
    g_current_pos.Y = (SHORT)y;
    
    if (!g_double_buffering_enabled) {
        SetConsoleCursorPosition(g_console_handle, g_current_pos);
    }
}

void platform_set_color(color_t color) {
    g_current_attributes = (WORD)color;
    
    if (!g_double_buffering_enabled) {
        SetConsoleTextAttribute(g_console_handle, (WORD)color);
    }
}

void platform_reset_color(void) {
    g_current_attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    
    if (!g_double_buffering_enabled) {
        SetConsoleTextAttribute(g_console_handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }
}

void platform_print(const char* text) {
    if (g_double_buffering_enabled && g_screen_buffer) {
        // 더블 버퍼링 사용 시 백 버퍼에 직접 쓰기
        int len = strlen(text);
        for (int i = 0; i < len; i++) {
            if (g_current_pos.X >= 0 && g_current_pos.X < g_buffer_size.X &&
                g_current_pos.Y >= 0 && g_current_pos.Y < g_buffer_size.Y) {
                
                int index = g_current_pos.Y * g_buffer_size.X + g_current_pos.X;
                
                // UTF-8 문자 처리 개선
                if ((unsigned char)text[i] >= 128) {
                    // 유니코드 문자 처리 - 간단한 이모지 대체
                    g_screen_buffer[index].Char.UnicodeChar = L'*';
                } else {
                    g_screen_buffer[index].Char.AsciiChar = text[i];
                }
                g_screen_buffer[index].Attributes = g_current_attributes;
                
                g_current_pos.X++;
                if (g_current_pos.X >= g_buffer_size.X) {
                    g_current_pos.X = 0;
                    g_current_pos.Y++;
                }
            }
        }
    } else {
        // 기존 방식 사용
        printf("%s", text);
    }
}

void platform_print_at(int x, int y, const char* text) {
    platform_goto_xy(x, y);
    platform_print(text);
}

// 더블 버퍼링된 화면을 실제 콘솔에 출력하는 함수
void platform_present_buffer(void) {
    if (g_double_buffering_enabled && g_screen_buffer) {
        WriteConsoleOutput(
            g_console_handle,
            g_screen_buffer,
            g_buffer_size,
            (COORD){0, 0},
            &g_write_region
        );
    }
}

void platform_hide_cursor(void) {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(g_console_handle, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(g_console_handle, &cursorInfo);
}

void platform_show_cursor(void) {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(g_console_handle, &cursorInfo);
    cursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(g_console_handle, &cursorInfo);
}

void platform_set_console_size(int width, int height) {
    COORD newBuf = {(SHORT)width, (SHORT)height};
    SetConsoleScreenBufferSize(g_console_handle, newBuf);
    
    SMALL_RECT win = {0, 0, (SHORT)(width-1), (SHORT)(height-1)};
    SetConsoleWindowInfo(g_console_handle, TRUE, &win);
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
#include "platform.h"

#ifdef PLATFORM_WINDOWS

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>

static HANDLE g_console_handle = NULL;
static HANDLE g_input_handle = NULL;

bool platform_init(void) {
    g_console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    g_input_handle = GetStdHandle(STD_INPUT_HANDLE);
    
    if (g_console_handle == INVALID_HANDLE_VALUE || g_input_handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    // UTF-8 코드 페이지 설정
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    srand((unsigned int)time(NULL));
    return true;
}

void platform_cleanup(void) {
    platform_reset_color();
    platform_show_cursor();
}

void platform_clear_screen(void) {
    COORD coord = {0, 0};
    DWORD written;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    
    GetConsoleScreenBufferInfo(g_console_handle, &csbi);
    DWORD size = csbi.dwSize.X * csbi.dwSize.Y;
    
    FillConsoleOutputCharacter(g_console_handle, ' ', size, coord, &written);
    FillConsoleOutputAttribute(g_console_handle, csbi.wAttributes, size, coord, &written);
    SetConsoleCursorPosition(g_console_handle, coord);
}

void platform_goto_xy(int x, int y) {
    COORD coord = {(SHORT)x, (SHORT)y};
    SetConsoleCursorPosition(g_console_handle, coord);
}

void platform_set_color(color_t color) {
    SetConsoleTextAttribute(g_console_handle, (WORD)color);
}

void platform_reset_color(void) {
    SetConsoleTextAttribute(g_console_handle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

void platform_print(const char* text) {
    printf("%s", text);
}

void platform_print_at(int x, int y, const char* text) {
    platform_goto_xy(x, y);
    platform_print(text);
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
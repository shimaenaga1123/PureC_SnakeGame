#include "platform.h"

#if defined(PLATFORM_MACOS) || defined(PLATFORM_UNIX)

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>

static struct termios g_original_termios;
static bool g_termios_saved = false;

bool platform_init(void) {
    // 원래 터미널 설정 저장
    if (tcgetattr(STDIN_FILENO, &g_original_termios) == 0) {
        g_termios_saved = true;

        // 터미널을 raw 모드로 설정
        struct termios raw = g_original_termios;
        raw.c_lflag &= ~(ECHO | ICANON);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

        // stdin을 논블로킹 모드로 설정
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    }
    
    srand((unsigned int)time(NULL));
    return true;
}

void platform_cleanup(void) {
    platform_reset_color();
    platform_show_cursor();
    
    // 원래 터미널 설정 복원
    if (g_termios_saved) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_original_termios);
    }
}

void platform_clear_screen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
}

void platform_goto_xy(int x, int y) {
    printf("\033[%d;%dH", y + 1, x + 1);
    fflush(stdout);
}

void platform_set_color(color_t color) {
    const char* color_codes[] = {
        "\033[30m", "\033[34m", "\033[32m", "\033[36m",
        "\033[31m", "\033[35m", "\033[33m", "\033[37m",
        "\033[90m", "\033[94m", "\033[92m", "\033[96m",
        "\033[91m", "\033[95m", "\033[93m", "\033[97m"
    };
    
    if (color < 16) {
        printf("%s", color_codes[color]);
        fflush(stdout);
    }
}

void platform_reset_color(void) {
    printf("\033[0m");
    fflush(stdout);
}

void platform_print(const char* text) {
    printf("%s", text);
    fflush(stdout);
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
    // Unix에서는 터미널 창의 크기를 안정적으로 조정할 수 없음
    // 이는 일반적으로 터미널 에뮬레이터에서 처리됨
    (void)width;
    (void)height;
}

game_key_t platform_get_key_pressed(void) {
    char ch;
    if (read(STDIN_FILENO, &ch, 1) == 1) {
        if (ch == 27) { // ESC 시퀀스
            char seq[3];
            if (read(STDIN_FILENO, &seq[0], 1) == 1 && seq[0] == '[') {
                if (read(STDIN_FILENO, &seq[1], 1) == 1) {
                    switch (seq[1]) {
                        case 'A': return KEY_UP;
                        case 'B': return KEY_DOWN;
                        case 'C': return KEY_RIGHT;
                        case 'D': return KEY_LEFT;
                    }
                }
            }
            return KEY_ESC;
        } else {
            switch (ch) {
                case '\n': case '\r': return KEY_ENTER;
                case ' ': return KEY_SPACE;
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
    }
    return KEY_NONE;
}

bool platform_is_key_down(game_key_t key) {
    // Unix에서는 키가 현재 눌려있는지 쉽게 확인할 수 없음
    // 이를 위해서는 더 복잡한 입력 처리가 필요함
    (void)key;
    return false;
}

void platform_sleep(uint32_t ms) {
    usleep(ms * 1000);
}

uint64_t platform_get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// 스레딩 작업
thread_handle_t platform_create_thread(thread_func_t func, void* arg) {
    thread_handle_t handle = {0};
    pthread_t* thread = malloc(sizeof(pthread_t));
    if (thread && pthread_create(thread, NULL, func, arg) == 0) {
        handle.handle = thread;
        handle.id = (uint32_t)(uintptr_t)thread;
    } else {
        free(thread);
    }
    return handle;
}

void platform_join_thread(thread_handle_t thread) {
    if (thread.handle) {
        pthread_join(*(pthread_t*)thread.handle, NULL);
        free(thread.handle);
    }
}

void platform_detach_thread(thread_handle_t thread) {
    if (thread.handle) {
        pthread_detach(*(pthread_t*)thread.handle);
        free(thread.handle);
    }
}

bool platform_thread_running(thread_handle_t thread) {
    // 이것은 단순화된 검사 - 실제로는 더 정교한 추적이 필요함
    return thread.handle != NULL;
}

// 뮤텍스 작업
mutex_handle_t platform_create_mutex(void) {
    mutex_handle_t handle = {0};
    pthread_mutex_t* mutex = malloc(sizeof(pthread_mutex_t));
    if (mutex && pthread_mutex_init(mutex, NULL) == 0) {
        handle.handle = mutex;
    } else {
        free(mutex);
    }
    return handle;
}

void platform_destroy_mutex(mutex_handle_t mutex) {
    if (mutex.handle) {
        pthread_mutex_destroy((pthread_mutex_t*)mutex.handle);
        free(mutex.handle);
    }
}

void platform_lock_mutex(mutex_handle_t mutex) {
    if (mutex.handle) {
        pthread_mutex_lock((pthread_mutex_t*)mutex.handle);
    }
}

void platform_unlock_mutex(mutex_handle_t mutex) {
    if (mutex.handle) {
        pthread_mutex_unlock((pthread_mutex_t*)mutex.handle);
    }
}

// 유틸리티 함수
int platform_random(int min, int max) {
    return min + rand() % (max - min + 1);
}

void platform_seed_random(uint32_t seed) {
    srand(seed);
}

// Windows 전용 더블 버퍼링 함수 (Unix에서는 빈 구현)
void platform_present_buffer(void) {
    // Unix/Linux에서는 터미널이 자체적으로 버퍼링을 처리함
    fflush(stdout);
}

#endif // PLATFORM_MACOS || PLATFORM_UNIX

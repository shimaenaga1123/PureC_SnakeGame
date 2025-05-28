/**
 * @file platform.h
 * @brief 크로스 플랫폼 추상화 레이어 헤더 파일
 * 
 * 다양한 운영체제에서 동일한 인터페이스로 콘솔 조작, 입력 처리,
 * 스레드 관리, 타이밍 등의 기능을 제공합니다.
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stdbool.h>

// 플랫폼 식별
#if defined(PLATFORM_MACOS) || defined(PLATFORM_WINDOWS) || defined(PLATFORM_WEB) || defined(PLATFORM_UNIX)
    #define PLATFORM_SUPPORTED
#elif defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
#elif defined(__APPLE__)
    #define PLATFORM_MACOS
#elif defined(__EMSCRIPTEN__)
    #define PLATFORM_WEB
#else
    #define PLATFORM_UNIX
#endif

/**
 * @brief 콘솔 색상 상수
 */
typedef enum {
    COLOR_BLACK = 0,               // 검정
    COLOR_BLUE = 1,                // 파랑
    COLOR_GREEN = 2,               // 초록
    COLOR_CYAN = 3,                // 청록
    COLOR_RED = 4,                 // 빨강
    COLOR_MAGENTA = 5,             // 자홍
    COLOR_YELLOW = 6,              // 노랑
    COLOR_WHITE = 7,               // 흰색
    COLOR_BRIGHT_BLACK = 8,        // 밝은 검정 (회색)
    COLOR_BRIGHT_BLUE = 9,         // 밝은 파랑
    COLOR_BRIGHT_GREEN = 10,       // 밝은 초록
    COLOR_BRIGHT_CYAN = 11,        // 밝은 청록
    COLOR_BRIGHT_RED = 12,         // 밝은 빨강
    COLOR_BRIGHT_MAGENTA = 13,     // 밝은 자홍
    COLOR_BRIGHT_YELLOW = 14,      // 밝은 노랑
    COLOR_BRIGHT_WHITE = 15        // 밝은 흰색
} color_t;

/**
 * @brief 게임에서 사용하는 키 코드
 */
typedef enum {
    KEY_NONE = 0,                  // 키 입력 없음
    KEY_UP,                        // 위쪽 화살표
    KEY_DOWN,                      // 아래쪽 화살표
    KEY_LEFT,                      // 왼쪽 화살표
    KEY_RIGHT,                     // 오른쪽 화살표
    KEY_ENTER,                     // 엔터 키
    KEY_ESC,                       // ESC 키
    KEY_SPACE,                     // 스페이스 바
    KEY_W, KEY_A, KEY_S, KEY_D,    // WASD 키
    KEY_1, KEY_2, KEY_3, KEY_4     // 숫자 키 1-4
} game_key_t;

/**
 * @brief 스레드 핸들 구조체
 */
typedef struct {
    void* handle;                  // 플랫폼별 스레드 핸들
    uint32_t id;                   // 스레드 ID
} thread_handle_t;

/**
 * @brief 뮤텍스 핸들 구조체
 */
typedef struct {
    void* handle;                  // 플랫폼별 뮤텍스 핸들
} mutex_handle_t;

/**
 * @brief 스레드 함수 포인터 타입
 */
typedef void* (*thread_func_t)(void* arg);

// 플랫폼 초기화 및 정리
bool platform_init(void);
void platform_cleanup(void);

// 콘솔 조작 함수들
void platform_clear_screen(void);
void platform_goto_xy(int x, int y);
void platform_set_color(color_t color);
void platform_reset_color(void);
void platform_print(const char* text);
void platform_print_at(int x, int y, const char* text);
void platform_hide_cursor(void);
void platform_show_cursor(void);
void platform_set_console_size(int width, int height);
void platform_present_buffer(void);  // 더블 버퍼링 출력 함수 (Windows 전용)

// 입력 처리 함수들
game_key_t platform_get_key_pressed(void);
bool platform_is_key_down(game_key_t key);

// 타이밍 관련 함수들
void platform_sleep(uint32_t ms);
uint64_t platform_get_time_ms(void);

// 스레딩 관련 함수들
thread_handle_t platform_create_thread(thread_func_t func, void* arg);
void platform_join_thread(thread_handle_t thread);
void platform_detach_thread(thread_handle_t thread);
bool platform_thread_running(thread_handle_t thread);

// 뮤텍스 관련 함수들
mutex_handle_t platform_create_mutex(void);
void platform_destroy_mutex(mutex_handle_t mutex);
void platform_lock_mutex(mutex_handle_t mutex);
void platform_unlock_mutex(mutex_handle_t mutex);

// 유틸리티 함수들
int platform_random(int min, int max);
void platform_seed_random(uint32_t seed);

#endif // PLATFORM_H

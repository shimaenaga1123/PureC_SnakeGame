#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "platform/platform.h"
#include "game/game.h"
#include "ui/ui.h"

#ifdef PLATFORM_WEB
#include <emscripten.h>
#endif

// 외부 AI 함수 선언
void ai_update_players(game_state_t* game, int ai_personality);

// 함수 전방 선언
void start_game(game_mode_t mode);

/**
 * @brief 애플리케이션의 전체 상태를 나타내는 구조체
 * 
 * 게임 상태, UI 컨텍스트, 오디오 시스템, 실행 상태, 게임 진행 상태, 타이밍 정보를 포함합니다.
 */
typedef struct {
    game_state_t game;                  // 게임 상태
    ui_context_t ui;                    // UI 컨텍스트
    bool running;                       // 애플리케이션 실행 중 여부
    bool in_game;                       // 게임 플레이 중 여부
    uint64_t last_update_time;          // 마지막 게임 업데이트 시간
    uint64_t last_ai_update_time;       // 마지막 AI 업데이트 시간
} app_state_t;

static app_state_t g_app;

#ifdef PLATFORM_WEB
/**
 * @brief 웹용 메인 루프 함수 (Emscripten 콜백)
 */
void web_main_loop(void) {
    static int loop_count = 0;
    loop_count++;
    
    if (!g_app.running) {
        printf("애플리케이션 종료 중...\n");
        emscripten_cancel_main_loop();
        return;
    }
    
    // 5초마다 상태 로그 출력 (디버깅용)
    if (loop_count % 300 == 0) {  // 60 FPS * 5초
        printf("메인 루프 실행 중... (루프 %d회, 게임 중: %s)\n", 
               loop_count, g_app.in_game ? "예" : "아니오");
    }
    
    if (!g_app.in_game) {
        // UI 처리
        ui_render(&g_app.ui);
        
        game_key_t key = platform_get_key_pressed();
        if (key != KEY_NONE) {
            // ESC 키로 메인 메뉴로 돌아가기
            if (key == KEY_ESC && g_app.ui.current_state != UI_STATE_MAIN_MENU) {
                ui_set_state(&g_app.ui, UI_STATE_MAIN_MENU);
            } else {
                ui_handle_input(&g_app.ui, key);
                
                // 게임 시작 확인
                if (g_app.ui.current_state == UI_STATE_PLAYING) {
                    printf("게임 시작...\n");
                    start_game(g_app.ui.selected_mode);
                }
                
                // 종료 확인
                if (g_app.ui.current_state == UI_STATE_MAIN_MENU && 
                    g_app.ui.selected_option == 3 && key == KEY_ENTER) {
                    printf("사용자가 종료를 선택했습니다.\n");
                    g_app.running = false;
                }
            }
        }
    } else {
        // 게임 루프 처리
        uint64_t current_time = platform_get_time_ms();
        
        game_key_t key = platform_get_key_pressed();
        
        // ESC 키로 게임 종료
        if (key == KEY_ESC) {
            printf("게임에서 메뉴로 복귀\n");
            g_app.in_game = false;
            ui_set_state(&g_app.ui, UI_STATE_MAIN_MENU);
            platform_clear_screen();
            return;
        }
        
        // 사용자 입력 처리
        if (key != KEY_NONE) {
            game_handle_input(&g_app.game, 0, key);
        }
        
        // AI 플레이어 업데이트 - 100ms마다
        if (current_time - g_app.last_ai_update_time >= 100) {
            ai_update_players(&g_app.game, g_app.ui.ai_personality);
            g_app.last_ai_update_time = current_time;
        }
        
        // 게임 상태 업데이트 - 게임 속도에 따라
        if (current_time - g_app.last_update_time >= (uint64_t)g_app.game.game_speed) {
            if (!game_update(&g_app.game)) {
                printf("게임 오버\n");
                g_app.in_game = false;
                platform_clear_screen();
                ui_set_state(&g_app.ui, UI_STATE_GAME_OVER);
                ui_show_game_over(&g_app.ui, &g_app.game);
                return;
            }
            g_app.last_update_time = current_time;
        }
        
        // 게임 렌더링
        game_render(&g_app.game);
    }
}
#endif

/**
 * @brief 게임 루프를 실행하는 스레드 함수
 *
 * AI 업데이트, 게임 상태 업데이트, 렌더링을 처리합니다.
 * 
 * @param arg 애플리케이션 상태에 대한 포인터
 * @return NULL 반환
 */
void* game_thread(void* arg) {
    app_state_t* app = (app_state_t*)arg;
    
    while (app->running && app->in_game) {
        uint64_t current_time = platform_get_time_ms();
        
        // AI 플레이어 업데이트 - 100ms마다 실행
        if (current_time - app->last_ai_update_time >= 100) {
            ai_update_players(&app->game, app->ui.ai_personality);
            app->last_ai_update_time = current_time;
        }
        
        // 게임 상태 업데이트 - 게임 속도에 따라 실행
        if (current_time - app->last_update_time >= (uint64_t)app->game.game_speed) {
            if (!game_update(&app->game)) {
                app->in_game = false;
                // 게임 오버 시 화면 완전 정리
                platform_clear_screen();
                ui_set_state(&app->ui, UI_STATE_GAME_OVER);
                ui_show_game_over(&app->ui, &app->game);
            }
            app->last_update_time = current_time;
        }
        
        // 게임 렌더링
        if (app->in_game) {
            game_render(&app->game);
        }
        
        platform_sleep(16); // 약 60 FPS 유지
    }
    
    return NULL;
}

/**
 * @brief 새로운 게임을 시작합니다
 * 
 * 게임 모드에 따라 게임을 초기화하고 스레드를 생성합니다.
 * 
 * @param mode 게임 모드
 */
void start_game(game_mode_t mode) {
    if (!game_init(&g_app.game, mode)) {
        return;
    }

    // UI 설정을 게임에 적용
    switch (g_app.ui.game_speed_setting) {
        case 0: // 느림
            g_app.game.game_speed = 200;
            break;
        case 1: // 보통
            g_app.game.game_speed = 150;
            break;
        case 2: // 빠름
            g_app.game.game_speed = 100;
            break;
    }
    
    g_app.in_game = true;
    g_app.last_update_time = platform_get_time_ms();
    g_app.last_ai_update_time = g_app.last_update_time;
    
#ifdef PLATFORM_WEB
    // 웹에서는 메인 루프에서 모든 것을 처리
    platform_clear_screen();
#else
    // 네이티브 플랫폼: 게임 스레드 생성
    thread_handle_t game_thread_handle = platform_create_thread(game_thread, &g_app);

    // 입력 처리 루프
    while (g_app.running && g_app.in_game) {
        game_key_t key = platform_get_key_pressed();

        // ESC 키로 게임 종료
        if (key == KEY_ESC) {
            g_app.in_game = false;
            ui_set_state(&g_app.ui, UI_STATE_MAIN_MENU);
            platform_clear_screen();
            break;
        }

        // 사용자 입력 처리 (플레이어 0만)
        game_handle_input(&g_app.game, 0, key);
        platform_sleep(16);
    }

    // 게임 스레드가 끝날 때까지 대기
    platform_join_thread(game_thread_handle);

    // 게임 종료 시 화면 완전 정리
    if (!g_app.in_game) {
        platform_clear_screen();
    }

    game_cleanup(&g_app.game);
#endif
}

/**
 * @brief 메인 함수 - 애플리케이션 진입점
 * 
 * 플랫폼 초기화, 오디오 시스템 설정, UI 설정, 메인 루프 실행을 담당합니다.
 * 
 * @return 프로그램 종료 코드 (0: 정상 종료, 1: 오류)
 */
int main(void) {
    // 플랫폼 초기화
    if (!platform_init()) {
        printf("플랫폼 초기화에 실패했습니다\n");
        return 1;
    }
    
    // 애플리케이션 초기화
    g_app.running = true;
    g_app.in_game = false;
    ui_init(&g_app.ui);

    // 콘솔 설정
    platform_hide_cursor();
    platform_set_console_size(120, 50);
    
#ifdef PLATFORM_WEB
    // 웹 플랫폼: Emscripten 메인 루프 사용
    printf("크로스 플랫폼 뱀 게임을 시작합니다...\n");
    emscripten_set_main_loop(web_main_loop, 60, 1);
    
    // 웹에서는 여기까지 도달하지 않음 (무한 루프)
    return 0;
#else
    // 네이티브 플랫폼: 기존 메인 루프
    while (g_app.running) {
        if (!g_app.in_game) {
            // UI 처리
            ui_render(&g_app.ui);
            
            game_key_t key = platform_get_key_pressed();
            if (key != KEY_NONE) {
                // ESC 키로 메인 메뉴로 돌아가기
                if (key == KEY_ESC && g_app.ui.current_state != UI_STATE_MAIN_MENU) {
                    ui_set_state(&g_app.ui, UI_STATE_MAIN_MENU);
                } else {
                    ui_handle_input(&g_app.ui, key);
                    
                    // 게임 시작 확인
                    if (g_app.ui.current_state == UI_STATE_PLAYING) {
                        start_game(g_app.ui.selected_mode);
                    }
                    
                    // 종료 확인
                    if (g_app.ui.current_state == UI_STATE_MAIN_MENU && 
                        g_app.ui.selected_option == 3 && key == KEY_ENTER) {
                        g_app.running = false;
                    }
                }
            }
        } else {
            // 게임이 별도 스레드에서 실행 중
            platform_sleep(16);
        }
        
        platform_sleep(16); // 약 60 FPS 유지
    }

    ui_cleanup(&g_app.ui);
    platform_cleanup();
    
    printf("게임을 종료합니다. 플레이해주셔서 감사합니다!\n");
    return 0;
#endif
}

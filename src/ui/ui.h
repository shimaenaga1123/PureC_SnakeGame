/**
 * @file ui.h
 * @brief 사용자 인터페이스 시스템 헤더 파일
 * 
 * 게임의 메뉴 시스템, 설정 화면, 게임 오버 화면 등
 * 모든 UI 관련 기능을 정의합니다.
 */

#ifndef UI_H
#define UI_H

#include "../platform/platform.h"
#include "../game/game.h"

/**
 * @brief UI 상태를 나타내는 열거형
 */
typedef enum {
    UI_STATE_MAIN_MENU,            // 메인 메뉴
    UI_STATE_GAME_MODE_SELECT,     // 게임 모드 선택
    UI_STATE_AI_DIFFICULTY_SELECT, // AI 난이도 선택
    UI_STATE_PLAYING,              // 게임 중
    UI_STATE_GAME_OVER,            // 게임 종료
    UI_STATE_SETTINGS              // 설정 메뉴
} ui_state_t;

/**
 * @brief 메뉴 옵션을 나타내는 구조체
 */
typedef struct {
    char text[64];                 // 표시될 텍스트
    int value;                     // 옵션 값
} menu_option_t;

/**
 * @brief UI 컨텍스트 - UI 시스템의 전체 상태를 관리
 */
typedef struct {
    ui_state_t current_state;      // 현재 UI 상태
    ui_state_t previous_state;     // 이전 UI 상태
    int selected_option;           // 선택된 옵션 인덱스
    int num_options;               // 옵션 개수
    menu_option_t options[10];     // 메뉴 옵션 배열
    char title[128];               // 화면 제목
    char message[512];             // 화면 메시지 (더 긴 메시지 지원)
    game_mode_t selected_mode;     // 선택된 게임 모드
    
    // 게임 설정
    int game_speed_setting;        // 게임 속도 설정 (0=느림, 1=보통, 2=빠름)
    int game_difficulty;           // 게임 난이도 (0=쉬움, 1=보통, 2=어려움)
    bool show_grid;                // 격자 표시 여부
    bool show_fps;                 // FPS 표시 여부
    
    // 게임플레이 설정
    
    // 게임플레이 설정
    bool auto_pause;               // 자동 일시정지 여부
    bool screen_shake;             // 화면 흔들림 효과
    int controls_scheme;           // 조작 방식 (0=화살표+WASD, 1=화살표만, 2=WASD만)
} ui_context_t;

// 함수 선언
void ui_init(ui_context_t* ui);
void ui_cleanup(ui_context_t* ui);
void ui_update(ui_context_t* ui);
void ui_render(ui_context_t* ui);
void ui_handle_input(ui_context_t* ui, game_key_t key);
void ui_set_state(ui_context_t* ui, ui_state_t state);
void ui_show_main_menu(ui_context_t* ui);
void ui_show_game_mode_select(ui_context_t* ui);
void ui_show_ai_difficulty_select(ui_context_t* ui);
void ui_show_settings(ui_context_t* ui);
void ui_show_game_over(ui_context_t* ui, game_state_t* game);

#endif // UI_H

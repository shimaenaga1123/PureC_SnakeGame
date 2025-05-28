#include "ui.h"

#include <string.h>
#include <stdio.h>

// 정적 함수 선언
static void ui_handle_main_menu_selection(ui_context_t* ui);
static void ui_handle_ai_difficulty_selection(ui_context_t* ui);
static void ui_handle_game_over_selection(ui_context_t* ui);
static void ui_handle_settings_selection(ui_context_t* ui);

/**
 * @brief UI 시스템을 초기화합니다
 * 
 * @param ui UI 컨텍스트 포인터
 */
void ui_init(ui_context_t* ui) {
    if (!ui) return;
    
    memset(ui, 0, sizeof(ui_context_t));
    ui->current_state = UI_STATE_MAIN_MENU;
    ui->selected_option = 0;
    
    // 기본 게임 설정값으로 초기화
    ui->game_speed_setting = 1;    // 보통
    ui->game_difficulty = 1;       // 보통
    ui->show_grid = false;
    ui->show_fps = false;
    
    // 기본 게임플레이 설정값
    ui->auto_pause = false;
    ui->screen_shake = true;
    ui->controls_scheme = 0;       // 화살표+WASD
    
    ui_show_main_menu(ui);
}

/**
 * @brief UI 시스템을 정리합니다
 * 
 * @param ui UI 컨텍스트 포인터
 */
void ui_cleanup(ui_context_t* ui) {
    (void)ui; // 정리할 리소스가 없음
}

/**
 * @brief UI를 업데이트합니다
 * 
 * @param ui UI 컨텍스트 포인터
 */
void ui_update(ui_context_t* ui) {
    // 이 간단한 구현에서는 렌더링에서 UI 업데이트를 처리
    (void)ui;
}

/**
 * @brief UI를 화면에 렌더링합니다
 * 
 * @param ui UI 컨텍스트 포인터
 */
void ui_render(ui_context_t* ui) {
    if (!ui) return;
    
    platform_clear_screen();
    platform_hide_cursor();
    
    // 제목 그리기
    platform_set_color(COLOR_BRIGHT_GREEN);
    platform_print_at(30, 2, ui->title);
    
    // 메뉴 옵션들 그리기
    for (int i = 0; i < ui->num_options; i++) {
        int y = 8 + i * 2;
        
        // 선택된 옵션에 화살표 표시
        if (i == ui->selected_option) {
            platform_set_color(COLOR_BRIGHT_YELLOW);
            platform_print_at(25, y, "> ");
        } else {
            platform_print_at(25, y, "  ");
        }
        
        // 옵션 텍스트 색상 설정
        platform_set_color(i == ui->selected_option ? COLOR_BRIGHT_WHITE : COLOR_WHITE);
        platform_print_at(27, y, ui->options[i].text);
    }
    
    // 메시지 그리기 (여러 줄 지원)
    if (strlen(ui->message) > 0) {
        platform_set_color(COLOR_BRIGHT_CYAN);
        
        char message_copy[1024];
        strncpy(message_copy, ui->message, sizeof(message_copy) - 1);
        message_copy[sizeof(message_copy) - 1] = '\0';
        
        char* line = strtok(message_copy, "\n");
        int line_y = 20;
        
        while (line != NULL && line_y < 28) {
            platform_print_at(10, line_y, line);
            line = strtok(NULL, "\n");
            line_y++;
        }
    }
    
    // 조작 방법 안내
    platform_set_color(COLOR_BRIGHT_BLACK);
    platform_print_at(20, 29, "화살표 키로 이동, Enter로 선택, ESC로 뒤로가기");
    
    platform_reset_color();
    
    // Windows에서 더블 버퍼링된 화면을 실제 콘솔에 출력
    platform_present_buffer();
}

/**
 * @brief 사용자 입력을 처리합니다
 * 
 * @param ui UI 컨텍스트 포인터
 * @param key 입력된 키
 */
void ui_handle_input(ui_context_t* ui, game_key_t key) {
    if (!ui) return;
    
    switch (key) {
        case KEY_UP:
            // 위로 이동 (순환)
            if (ui->selected_option > 0) {
                ui->selected_option--;
            } else {
                ui->selected_option = ui->num_options - 1;
            }
            break;
            
        case KEY_DOWN:
            // 아래로 이동 (순환)
            if (ui->selected_option < ui->num_options - 1) {
                ui->selected_option++;
            } else {
                ui->selected_option = 0;
            }
            break;
            
        case KEY_LEFT:
            // 왼쪽 키 처리
            break;

        case KEY_RIGHT:
            // 오른쪽 키 처리
            break;
            
        case KEY_ENTER:
            // 현재 상태에 따른 선택 처리
            switch (ui->current_state) {
                case UI_STATE_MAIN_MENU:
                    ui_handle_main_menu_selection(ui);
                    break;
                    
                case UI_STATE_AI_DIFFICULTY_SELECT:
                    ui_handle_ai_difficulty_selection(ui);
                    break;
                    
                case UI_STATE_GAME_OVER:
                    ui_handle_game_over_selection(ui);
                    break;
                    
                case UI_STATE_SETTINGS:
                    ui_handle_settings_selection(ui);
                    break;
                    
                default:
                    break;
            }
            break;
            
        default:
            // 다른 키는 무시
            break;
    }
}

/**
 * @brief 메인 메뉴 선택을 처리합니다
 * 
 * @param ui UI 컨텍스트 포인터
 */
static void ui_handle_main_menu_selection(ui_context_t* ui) {
    switch (ui->selected_option) {
        case 0: // 혼자서 도전
            ui->selected_mode = GAME_MODE_SINGLE;
            ui_set_state(ui, UI_STATE_PLAYING);
            break;
        case 1: // AI와 대전
            ui_set_state(ui, UI_STATE_AI_DIFFICULTY_SELECT);
            break;
        case 2: // 설정
            ui_set_state(ui, UI_STATE_SETTINGS);
            break;
        case 3: // 종료
            // 게임 종료 신호
            break;
    }
}

/**
 * @brief AI 난이도 선택을 처리합니다
 * 
 * @param ui UI 컨텍스트 포인터
 */
static void ui_handle_ai_difficulty_selection(ui_context_t* ui) {
    switch (ui->selected_option) {
        case 0: // 쉬움
            ui->selected_mode = GAME_MODE_VS_AI_EASY;
            ui_set_state(ui, UI_STATE_PLAYING);
            break;
        case 1: // 보통
            ui->selected_mode = GAME_MODE_VS_AI_MEDIUM;
            ui_set_state(ui, UI_STATE_PLAYING);
            break;
        case 2: // 어려움
            ui->selected_mode = GAME_MODE_VS_AI_HARD;
            ui_set_state(ui, UI_STATE_PLAYING);
            break;
        case 3: // 뒤로가기
            ui_set_state(ui, UI_STATE_MAIN_MENU);
            break;
    }
}

/**
 * @brief 게임 오버 선택을 처리합니다
 * 
 * @param ui UI 컨텍스트 포인터
 */
static void ui_handle_game_over_selection(ui_context_t* ui) {
    switch (ui->selected_option) {
        case 0: // 다시 플레이
            ui_set_state(ui, UI_STATE_PLAYING);
            break;
        case 1: // 메인 메뉴
            ui_set_state(ui, UI_STATE_MAIN_MENU);
            break;
    }
}

/**
 * @brief 설정 메뉴 선택을 처리합니다
 * 
 * @param ui UI 컨텍스트 포인터
 */
static void ui_handle_settings_selection(ui_context_t* ui) {
    switch (ui->selected_option) {
        case 0: // 게임 속도
            ui->game_speed_setting = (ui->game_speed_setting + 1) % 3;
            ui_show_settings(ui);
            break;
        case 1: // 격자 표시
            ui->show_grid = !ui->show_grid;
            ui_show_settings(ui);
            break;
        case 2: // FPS 표시
            ui->show_fps = !ui->show_fps;
            ui_show_settings(ui);
            break;
        case 3: // 자동 일시정지
            ui->auto_pause = !ui->auto_pause;
            ui_show_settings(ui);
            break;
        case 4: // 화면 흔들림
            ui->screen_shake = !ui->screen_shake;
            ui_show_settings(ui);
            break;
        case 5: // 조작 방식
            ui->controls_scheme = (ui->controls_scheme + 1) % 3;
            ui_show_settings(ui);
            break;
        case 6: // 메인 메뉴로 돌아가기
            ui_set_state(ui, UI_STATE_MAIN_MENU);
            break;
    }
}



/**
 * @brief UI 상태를 변경합니다
 * 
 * @param ui UI 컨텍스트 포인터
 * @param state 새로운 UI 상태
 */
void ui_set_state(ui_context_t* ui, ui_state_t state) {
    if (!ui) return;
    
    ui->previous_state = ui->current_state;
    ui->current_state = state;
    ui->selected_option = 0;
    
    // 상태에 따른 화면 설정
    switch (state) {
        case UI_STATE_MAIN_MENU:
            ui_show_main_menu(ui);
            break;
        case UI_STATE_AI_DIFFICULTY_SELECT:
            ui_show_ai_difficulty_select(ui);
            break;
        case UI_STATE_SETTINGS:
            ui_show_settings(ui);
            break;
        default:
            break;
    }
}

/**
 * @brief 메인 메뉴를 표시합니다
 * 
 * @param ui UI 컨텍스트 포인터
 */
void ui_show_main_menu(ui_context_t* ui) {
    if (!ui) return;
    
    strcpy(ui->title, "🐍 크로스 플랫폼 뱀 게임 🐍");
    strcpy(ui->message, "");
    
    ui->num_options = 4;
    strcpy(ui->options[0].text, "🎯 혼자서 도전 (점수 도전 모드)");
    strcpy(ui->options[1].text, "🤖 AI와 대전 (생존 배틀 모드)");
    strcpy(ui->options[2].text, "⚙️  설정");
    strcpy(ui->options[3].text, "🚪 종료");
    
    ui->options[0].value = 0;
    ui->options[1].value = 1;
    ui->options[2].value = 2;
    ui->options[3].value = 3;
}

/**
 * @brief AI 난이도 선택 화면을 표시합니다
 * 
 * @param ui UI 컨텍스트 포인터
 */
void ui_show_ai_difficulty_select(ui_context_t* ui) {
    if (!ui) return;
    
    strcpy(ui->title, "🤖 AI 난이도 선택");
    strcpy(ui->message, "AI와의 대전에서 승리하세요!\n\n"
                       "• 쉬움: AI가 실수를 자주 합니다\n"
                       "• 보통: 균형잡힌 AI와 대전합니다\n"
                       "• 어려움: 매우 똑똑한 AI와 대전합니다");
    
    ui->num_options = 4;
    strcpy(ui->options[0].text, "😊 쉬움 - AI 초보자");
    strcpy(ui->options[1].text, "😐 보통 - AI 중급자");
    strcpy(ui->options[2].text, "😰 어려움 - AI 고수");
    strcpy(ui->options[3].text, "⬅️  뒤로가기");
    
    ui->options[0].value = GAME_MODE_VS_AI_EASY;
    ui->options[1].value = GAME_MODE_VS_AI_MEDIUM;
    ui->options[2].value = GAME_MODE_VS_AI_HARD;
    ui->options[3].value = -1;
}

/**
 * @brief 게임 오버 화면을 표시합니다
 * 
 * @param ui UI 컨텍스트 포인터
 * @param game 게임 상태 포인터
 */
void ui_show_game_over(ui_context_t* ui, game_state_t* game) {
    if (!ui || !game) return;
    
    strcpy(ui->title, "🎮 게임 종료");
    
    if (game->mode == GAME_MODE_SINGLE && game->num_players > 0) {
        snake_t* player_snake = &game->players[0];
        
        // 등급 계산
        const char* rank;
        const char* rank_emoji;
        if (player_snake->score < 200) {
            rank = "초보자";
            rank_emoji = "🌱";
        } else if (player_snake->score < 500) {
            rank = "입문자";
            rank_emoji = "🥉";
        } else if (player_snake->score < 1000) {
            rank = "아마추어";
            rank_emoji = "🥈";
        } else if (player_snake->score < 2000) {
            rank = "숙련자";
            rank_emoji = "🥇";
        } else if (player_snake->score < 5000) {
            rank = "전문가";
            rank_emoji = "⭐";
        } else if (player_snake->score < 10000) {
            rank = "마스터";
            rank_emoji = "💎";
        } else {
            rank = "전설";
            rank_emoji = "👑";
        }
        
        // 게임 시간 계산
        uint64_t game_time = (platform_get_time_ms() - game->game_start_time) / 1000;
        int minutes = (int)(game_time / 60);
        int seconds = (int)(game_time % 60);
        
        snprintf(ui->message, sizeof(ui->message), 
                "🏆 최종 점수: %d점\n"
                "%s 최종 등급: %s\n"
                "🐍 뱀 길이: %d칸\n"
                "⏱️  게임 시간: %d:%02d\n"
                "🍎 먹은 사과: %d개\n"
                "🧱 생성된 장애물: %d개\n\n"
                "훌륭한 플레이였습니다!", 
                player_snake->score,
                rank_emoji, rank,
                player_snake->length,
                minutes, seconds,
                player_snake->score / 100,
                game->obstacles_count);
    } else if (game->winner_id >= 0) {
        const char* winner_name = (game->winner_id == 0) ? "사용자" : "AI";
        const char* winner_emoji = (game->winner_id == 0) ? "🎉" : "🤖";
        
        snprintf(ui->message, sizeof(ui->message), 
                "%s %s 승리!\n\n"
                "🏆 승자 점수: %d점\n"
                "🐍 승자 뱀 길이: %d칸\n\n"
                "%s", 
                winner_emoji, winner_name,
                game->players[game->winner_id].score,
                game->players[game->winner_id].length,
                (game->winner_id == 0) ? "축하합니다!" : "다음에는 더 잘해보세요!");
    } else {
        strcpy(ui->message, "🤝 무승부!\n\n모든 플레이어가 동시에 탈락했습니다.\n다시 도전해보세요!");
    }
    
    ui->num_options = 2;
    strcpy(ui->options[0].text, "🔄 다시 플레이");
    strcpy(ui->options[1].text, "🏠 메인 메뉴");
    
    ui->options[0].value = 0;
    ui->options[1].value = 1;
}

/**
 * @brief 설정 화면을 표시합니다
 * 
 * @param ui UI 컨텍스트 포인터
 */
void ui_show_settings(ui_context_t* ui) {
    if (!ui) return;
    
    strcpy(ui->title, "⚙️ 게임 설정");
    strcpy(ui->message, "");
    
    ui->num_options = 7;

    // 게임 속도 옵션
    const char* speed_names[] = {"느림", "보통", "빠름"};
    snprintf(ui->options[0].text, sizeof(ui->options[0].text), 
             "🏃 게임 속도: %s", speed_names[ui->game_speed_setting]);

    // 격자 표시 옵션
    snprintf(ui->options[1].text, sizeof(ui->options[1].text), 
             "📋 격자 표시: %s", ui->show_grid ? "켜짐" : "꺼짐");

    // FPS 표시 옵션
    snprintf(ui->options[2].text, sizeof(ui->options[2].text), 
             "📊 FPS 표시: %s", ui->show_fps ? "켜짐" : "꺼짐");

    // 자동 일시정지 옵션
    snprintf(ui->options[3].text, sizeof(ui->options[3].text), 
             "⏸️  자동 일시정지: %s", ui->auto_pause ? "켜짐" : "꺼짐");

    // 화면 흔들림 옵션
    snprintf(ui->options[4].text, sizeof(ui->options[4].text), 
             "📳 화면 흔들림: %s", ui->screen_shake ? "켜짐" : "꺼짐");

    // 조작 방식 옵션
    const char* control_names[] = {"화살표+WASD", "화살표만", "WASD만"};
    snprintf(ui->options[5].text, sizeof(ui->options[5].text), 
             "🎮 조작 방식: %s", control_names[ui->controls_scheme]);

    strcpy(ui->options[6].text, "⬅️  메인 메뉴로 돌아가기");
    
    for (int i = 0; i < ui->num_options; i++) {
        ui->options[i].value = i;
    }
}


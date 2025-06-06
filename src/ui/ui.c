#include "ui.h"

#include <string.h>
#include <stdio.h>

// UI 상태 추적을 위한 정적 변수들
static ui_state_t g_previous_ui_state = UI_STATE_MAIN_MENU;
static int g_previous_selected_option = -1;
static char g_previous_title[128] = {0};
static char g_previous_message[512] = {0};
static menu_option_t g_previous_options[10];
static int g_previous_num_options = 0;
static bool g_ui_initialized = false;

// 정적 함수 선언
static void ui_handle_main_menu_selection(ui_context_t* ui);
static void ui_handle_ai_difficulty_selection(ui_context_t* ui);
static void ui_handle_game_over_selection(ui_context_t* ui);

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
    ui->ai_personality = 0;        // 균형잡힌
    
    // UI 추적 변수 초기화
    g_ui_initialized = false;
    g_previous_ui_state = UI_STATE_MAIN_MENU;
    g_previous_selected_option = -1;
    memset(g_previous_title, 0, sizeof(g_previous_title));
    memset(g_previous_message, 0, sizeof(g_previous_message));
    memset(g_previous_options, 0, sizeof(g_previous_options));
    g_previous_num_options = 0;
    
    ui_show_main_menu(ui);
}

/**
 * @brief UI 시스템을 정리합니다
 * 
 * @param ui UI 컨텍스트 포인터
 */
void ui_cleanup(ui_context_t* ui) {
    (void)ui; // 정리할 리소스가 없음
    g_ui_initialized = false;
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
 * @brief 문자열이 변경되었는지 확인하는 헬퍼 함수
 */
static bool string_changed(const char* current, const char* previous) {
    return strcmp(current, previous) != 0;
}

/**
 * @brief 옵션 배열이 변경되었는지 확인하는 헬퍼 함수
 */
static bool options_changed(const menu_option_t* current, const menu_option_t* previous, int current_num, int previous_num) {
    if (current_num != previous_num) return true;
    
    for (int i = 0; i < current_num; i++) {
        if (strcmp(current[i].text, previous[i].text) != 0 || current[i].value != previous[i].value) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 화면의 특정 영역을 지우는 헬퍼 함수
 */
static void clear_area(int start_x, int start_y, int width, int height) {
    platform_set_color(COLOR_BLACK);
    for (int y = start_y; y < start_y + height; y++) {
        for (int x = start_x; x < start_x + width; x++) {
            platform_print_at(x, y, " ");
        }
    }
}

/**
 * @brief 전체 화면을 완전히 정리하는 함수
 */
static void clear_full_screen(void) {
    platform_clear_screen();

    // 추가로 전체 화면을 공백으로 채워서 확실하게 정리
    platform_set_color(COLOR_BLACK);
    for (int y = 0; y < 50; y++) {
        for (int x = 0; x < 120; x++) {
            platform_print_at(x, y, " ");
        }
    }
}

/**
 * @brief UI를 화면에 렌더링합니다 (차분 렌더링 적용)
 *
 * @param ui UI 컨텍스트 포인터
 */
void ui_render(ui_context_t* ui) {
    if (!ui) return;

    bool force_full_redraw = false;

    // 첫 렌더링이거나 UI 상태가 변경된 경우 전체 다시 그리기
    if (!g_ui_initialized || ui->current_state != g_previous_ui_state) {
        clear_full_screen();  // 완전한 화면 정리
        platform_hide_cursor();
        force_full_redraw = true;
        g_ui_initialized = true;
        g_previous_ui_state = ui->current_state;

        // 상태 변경시 이전 추적 데이터 모두 리셋
        memset(g_previous_title, 0, sizeof(g_previous_title));
        memset(g_previous_message, 0, sizeof(g_previous_message));
        memset(g_previous_options, 0, sizeof(g_previous_options));
        g_previous_num_options = 0;
        g_previous_selected_option = -1;
    }

    // 제목 업데이트 (변경된 경우만)
    if (force_full_redraw || string_changed(ui->title, g_previous_title)) {
        // 제목을 더 잘 보이는 색상으로 설정
        platform_set_color(COLOR_BRIGHT_CYAN);

        // 이전 제목 영역 지우기 (필요한 경우)
        if (!force_full_redraw) {
            clear_area(15, 2, 70, 1);
        }

        platform_print_at(20, 2, ui->title);
        strcpy(g_previous_title, ui->title);
    }

    // 메뉴 옵션들 업데이트 (변경된 경우만)
    bool options_need_update = force_full_redraw ||
                              options_changed(ui->options, g_previous_options, ui->num_options, g_previous_num_options) ||
                              ui->selected_option != g_previous_selected_option;

    if (options_need_update) {
        // 이전 옵션 영역 지우기 (필요한 경우)
        if (!force_full_redraw) {
            int max_options = (g_previous_num_options > ui->num_options) ? g_previous_num_options : ui->num_options;
            clear_area(25, 8, 60, max_options * 2);
        }

        // 새 옵션들 그리기
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

        // 현재 옵션 상태를 이전 상태로 저장
        memcpy(g_previous_options, ui->options, sizeof(menu_option_t) * ui->num_options);
        g_previous_num_options = ui->num_options;
        g_previous_selected_option = ui->selected_option;
    }

    // 메시지 업데이트 (변경된 경우만)
    if (force_full_redraw || string_changed(ui->message, g_previous_message)) {
        // 이전 메시지 영역 지우기 (필요한 경우)
        if (!force_full_redraw) {
            clear_area(10, 20, 80, 8);
        }

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

        strcpy(g_previous_message, ui->message);
    }

    // 조작 방법 안내 (한 번만 출력)
    if (force_full_redraw) {
        platform_set_color(COLOR_BRIGHT_BLACK);
        platform_print_at(20, 29, "화살표 키로 이동, Enter로 선택, ESC로 뒤로가기");
        if (ui->current_state == UI_STATE_MAIN_MENU || ui->current_state == UI_STATE_AI_DIFFICULTY_SELECT) {
            platform_print_at(20, 30, "좌우 화살표로 설정 변경");
        }
    }

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
            // 왼쪽 키 처리 (설정 변경)
            if (ui->current_state == UI_STATE_MAIN_MENU && ui->selected_option == 1) {
                // 게임 속도 변경
                ui->game_speed_setting = (ui->game_speed_setting == 0) ? 2 : ui->game_speed_setting - 1;
                ui_show_main_menu(ui);
            } else if (ui->current_state == UI_STATE_AI_DIFFICULTY_SELECT && ui->selected_option == 3) {
                // AI 특성 변경
                ui->ai_personality = (ui->ai_personality == 0) ? 4 : ui->ai_personality - 1;
                ui_show_ai_difficulty_select(ui);
            }
            break;

        case KEY_RIGHT:
            // 오른쪽 키 처리 (설정 변경)
            if (ui->current_state == UI_STATE_MAIN_MENU && ui->selected_option == 1) {
                // 게임 속도 변경
                ui->game_speed_setting = (ui->game_speed_setting + 1) % 3;
                ui_show_main_menu(ui);
            } else if (ui->current_state == UI_STATE_AI_DIFFICULTY_SELECT && ui->selected_option == 3) {
                // AI 특성 변경
                ui->ai_personality = (ui->ai_personality + 1) % 5;
                ui_show_ai_difficulty_select(ui);
            }
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
        case 1: // 게임 속도 설정 (좌우 화살표로 변경)
            // Enter는 무시, 좌우 화살표로만 변경
            break;
        case 2: // AI와 대전
            ui_set_state(ui, UI_STATE_AI_DIFFICULTY_SELECT);
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
        case 3: // AI 특성 설정 (좌우 화살표로 변경)
            // Enter는 무시, 좌우 화살표로만 변경
            break;
        case 4: // 뒤로가기
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

    // 상태 변경시 화면을 완전히 정리하기 위해 초기화 플래그 리셋
    g_ui_initialized = false;
    g_previous_ui_state = (ui_state_t)-1; // 강제 전체 다시 그리기
    memset(g_previous_title, 0, sizeof(g_previous_title));
    memset(g_previous_message, 0, sizeof(g_previous_message));
    memset(g_previous_options, 0, sizeof(g_previous_options));
    g_previous_num_options = 0;
    g_previous_selected_option = -1;

    // 특히 게임 오버 상태로 전환시 화면 완전 정리
    if (state == UI_STATE_GAME_OVER) {
        clear_full_screen();
    }
    
    // 상태에 따른 화면 설정
    switch (state) {
        case UI_STATE_MAIN_MENU:
            ui_show_main_menu(ui);
            break;
        case UI_STATE_AI_DIFFICULTY_SELECT:
            ui_show_ai_difficulty_select(ui);
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
    
    // 게임 속도 옵션 (좌우 화살표로 변경 가능)
    const char* speed_names[] = {"느림", "보통", "빠름"};
    snprintf(ui->options[1].text, sizeof(ui->options[1].text), 
             "⚡ 게임 속도: %s ← →", speed_names[ui->game_speed_setting]);
    
    strcpy(ui->options[2].text, "🤖 AI와 대전 (생존 배틀 모드)");
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
    
    strcpy(ui->title, "🤖 AI 난이도 및 특성 선택");
    strcpy(ui->message, "AI와의 대전에서 승리하세요!\n\n"
                       "• 쉬움: AI가 실수를 자주 합니다\n"
                       "• 보통: 균형잡힌 AI와 대전합니다\n"
                       "• 어려움: 매우 똑똑한 AI와 대전합니다\n\n"
                       "AI 특성을 선택하여 플레이 스타일을 변경할 수 있습니다.");
    
    ui->num_options = 5;
    strcpy(ui->options[0].text, "😊 쉬움 - AI 초보자");
    strcpy(ui->options[1].text, "😐 보통 - AI 중급자");
    strcpy(ui->options[2].text, "😰 어려움 - AI 고수");
    
    // AI 특성 옵션 (좌우 화살표로 변경 가능)
    const char* personality_names[] = {"균형잡힌", "공격적", "방어적", "신중한", "무모한"};
    snprintf(ui->options[3].text, sizeof(ui->options[3].text),
             "🎭 AI 특성: %s ← →", personality_names[ui->ai_personality]);
    
    strcpy(ui->options[4].text, "⬅️ 뒤로가기");
    
    ui->options[0].value = GAME_MODE_VS_AI_EASY;
    ui->options[1].value = GAME_MODE_VS_AI_MEDIUM;
    ui->options[2].value = GAME_MODE_VS_AI_HARD;
    ui->options[3].value = -1;
    ui->options[4].value = -1;
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
                "⏱️ 게임 시간: %d:%02d\n"
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

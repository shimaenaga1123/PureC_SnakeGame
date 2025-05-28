#include "game.h"
#include "ai.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// 방향 벡터 배열 (우, 좌, 위, 아래)
static const position_t dir_vectors[4] = {
    {1, 0},   // 우측 (RIGHT)
    {-1, 0},  // 좌측 (LEFT)
    {0, -1},  // 위쪽 (UP)
    {0, 1}    // 아래쪽 (DOWN)
};

// 플레이어별 뱀 몸통 색상
static const color_t player_colors[MAX_PLAYERS] = {
    COLOR_BRIGHT_GREEN,    // 플레이어 1: 밝은 초록 (사용자)
    COLOR_BRIGHT_RED       // 플레이어 2: 밝은 빨강 (AI)
};

// 플레이어별 뱀 머리 색상
static const color_t player_head_colors[MAX_PLAYERS] = {
    COLOR_GREEN,           // 플레이어 1: 초록 (사용자)
    COLOR_RED              // 플레이어 2: 빨강 (AI)
};

/**
 * @brief 부드러운 보간 함수 (선형 보간)
 * 
 * @param a 시작값
 * @param b 끝값
 * @param t 보간 계수 (0.0 ~ 1.0)
 * @return 보간된 값
 */
float lerp(float a, float b, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return a + (b - a) * t;
}

/**
 * @brief 위치에 대한 부드러운 보간
 * 
 * @param a 시작 위치
 * @param b 끝 위치
 * @param t 보간 계수
 * @return 보간된 부드러운 위치
 */
smooth_position_t lerp_position(position_t a, position_t b, float t) {
    smooth_position_t result;
    result.x = lerp((float)a.x, (float)b.x, t);
    result.y = lerp((float)a.y, (float)b.y, t);
    return result;
}

/**
 * @brief 주어진 위치가 게임 영역 내에 유효한지 확인합니다
 * 
 * @param pos 확인할 위치
 * @return 유효하면 true, 아니면 false
 */
bool is_valid_position(position_t pos) {
    return pos.x >= 0 && pos.x < GAME_WIDTH && 
           pos.y >= 0 && pos.y < GAME_HEIGHT;
}

/**
 * @brief 현재 위치에서 특정 방향으로 이동했을 때의 다음 위치를 계산합니다
 * 
 * @param pos 현재 위치
 * @param dir 이동 방향
 * @return 다음 위치
 */
position_t get_next_position(position_t pos, direction_t dir) {
    position_t next = {
        pos.x + dir_vectors[dir].x,
        pos.y + dir_vectors[dir].y
    };
    return next;
}

/**
 * @brief 두 방향이 서로 반대 방향인지 확인합니다
 * 
 * @param dir1 첫 번째 방향
 * @param dir2 두 번째 방향
 * @return 반대 방향이면 true, 아니면 false
 */
bool is_opposite_direction(direction_t dir1, direction_t dir2) {
    return (dir1 == DIR_LEFT && dir2 == DIR_RIGHT) ||
           (dir1 == DIR_RIGHT && dir2 == DIR_LEFT) ||
           (dir1 == DIR_UP && dir2 == DIR_DOWN) ||
           (dir1 == DIR_DOWN && dir2 == DIR_UP);
}

/**
 * @brief 새로운 뱀 노드를 생성합니다
 * 
 * @param pos 노드가 위치할 좌표
 * @return 생성된 노드의 포인터 (실패시 NULL)
 */
static snake_node_t* create_snake_node(position_t pos) {
    snake_node_t* node = malloc(sizeof(snake_node_t));
    if (node) {
        node->pos = pos;
        node->smooth_pos.x = (float)pos.x;
        node->smooth_pos.y = (float)pos.y;
        node->next = NULL;
    }
    return node;
}

/**
 * @brief 뱀의 모든 노드를 해제합니다
 * 
 * @param snake 해제할 뱀의 포인터
 */
static void free_snake(snake_t* snake) {
    snake_node_t* current = snake->head;
    while (current) {
        snake_node_t* next = current->next;
        free(current);
        current = next;
    }
    snake->head = snake->tail = NULL;
    snake->length = 0;
}

/**
 * @brief 게임을 초기화합니다
 * 
 * @param game 게임 상태 포인터
 * @param mode 게임 모드
 * @return 초기화 성공시 true, 실패시 false
 */
bool game_init(game_state_t* game, game_mode_t mode) {
    if (!game) {
        return false;
    }
    
    memset(game, 0, sizeof(game_state_t));
    
    // 맵을 빈 공간으로 초기화
    for (int y = 0; y < GAME_HEIGHT; y++) {
        for (int x = 0; x < GAME_WIDTH; x++) {
            game->map[y][x] = CELL_EMPTY;
        }
    }
    
    game->mode = mode;
    game->state = GAME_STATE_PLAYING;
    game->num_players = 0;
    game->game_over = false;
    game->winner_id = -1;
    game->game_start_time = platform_get_time_ms();
    game->pause_start_time = 0;
    game->total_pause_time = 0;
    game->game_speed = 150; // 움직임당 밀리초
    game->smooth_motion_enabled = true;
    game->motion_interpolation = 0.0f;
    game->game_mutex = platform_create_mutex();
    game->apples_eaten = 0;
    game->actual_play_time = 0;
    
    // 모드에 따른 플레이어 초기화
    switch (mode) {
        case GAME_MODE_SINGLE:
            // 싱글 플레이어 모드 (혼자서 점수 도전)
            game_add_player(game, PLAYER_HUMAN);
            break;
            
        case GAME_MODE_VS_AI_EASY:
            // AI 대전 - 쉬움
            game_add_player(game, PLAYER_HUMAN);
            game_add_player(game, PLAYER_AI_EASY);
            break;
            
        case GAME_MODE_VS_AI_MEDIUM:
            // AI 대전 - 보통
            game_add_player(game, PLAYER_HUMAN);
            game_add_player(game, PLAYER_AI_MEDIUM);
            break;
            
        case GAME_MODE_VS_AI_HARD:
            // AI 대전 - 어려움
            game_add_player(game, PLAYER_HUMAN);
            game_add_player(game, PLAYER_AI_HARD);
            break;
    }
    
    // 초기 사과 생성
    game_generate_apple(game);
    
    return true;
}

/**
 * @brief 게임을 일시정지/재개합니다
 * 
 * @param game 게임 상태 포인터
 */
void game_toggle_pause(game_state_t* game) {
    if (!game || game->game_over) return;
    
    platform_lock_mutex(game->game_mutex);
    
    if (game->state == GAME_STATE_PLAYING) {
        // 게임 일시정지
        game->state = GAME_STATE_PAUSED;
        game->pause_start_time = platform_get_time_ms();
    } else if (game->state == GAME_STATE_PAUSED) {
        // 게임 재개
        game->state = GAME_STATE_PLAYING;
        
        // 일시정지 시간 누적
        if (game->pause_start_time > 0) {
            game->total_pause_time += platform_get_time_ms() - game->pause_start_time;
            game->pause_start_time = 0;
        }
    }
    
    platform_unlock_mutex(game->game_mutex);
}

/**
 * @brief 게임이 일시정지 상태인지 확인합니다
 * 
 * @param game 게임 상태 포인터
 * @return 일시정지 상태면 true
 */
bool game_is_paused(const game_state_t* game) {
    return game && game->state == GAME_STATE_PAUSED;
}

/**
 * @brief 실제 플레이 시간을 가져옵니다 (일시정지 시간 제외)
 * 
 * @param game 게임 상태 포인터
 * @return 실제 플레이 시간 (밀리초)
 */
uint64_t game_get_play_time(game_state_t* game) {
    if (!game) return 0;
    
    uint64_t current_time = platform_get_time_ms();
    uint64_t total_time = current_time - game->game_start_time;
    uint64_t pause_time = game->total_pause_time;
    
    // 현재 일시정지 중이면 현재 일시정지 시간도 빼기
    if (game->state == GAME_STATE_PAUSED && game->pause_start_time > 0) {
        pause_time += current_time - game->pause_start_time;
    }
    
    return total_time - pause_time;
}

/**
 * @brief 부드러운 모션을 업데이트합니다
 * 
 * @param game 게임 상태 포인터
 * @param delta_time 프레임 간격 (초)
 */
void game_update_smooth_motion(game_state_t* game, float delta_time) {
    if (!game || !game->smooth_motion_enabled) return;
    
    // 각 플레이어의 부드러운 모션 업데이트
    for (int i = 0; i < game->num_players; i++) {
        snake_t* snake = &game->players[i];
        if (!snake->alive) continue;
        
        // 모션 진행도 업데이트
        float motion_speed = 60.0f / (float)game->game_speed; // 속도에 따른 보간 속도
        snake->move_progress += delta_time * motion_speed;
        if (snake->move_progress > 1.0f) {
            snake->move_progress = 1.0f;
        }
        
        // 각 노드의 부드러운 위치 업데이트
        snake_node_t* current = snake->head;
        snake_node_t* prev = NULL;
        
        while (current) {
            if (prev == NULL) {
                // 머리는 이동 진행도에 따라 보간
                position_t target_pos = current->pos;
                current->smooth_pos.x = lerp(current->smooth_pos.x, (float)target_pos.x, 0.3f);
                current->smooth_pos.y = lerp(current->smooth_pos.y, (float)target_pos.y, 0.3f);
            } else {
                // 몸통은 앞 노드를 따라감
                float follow_speed = 0.2f;
                current->smooth_pos.x = lerp(current->smooth_pos.x, prev->smooth_pos.x, follow_speed);
                current->smooth_pos.y = lerp(current->smooth_pos.y, prev->smooth_pos.y, follow_speed);
            }
            
            prev = current;
            current = current->next;
        }
    }
}

/**
 * @brief 게임 통계를 업데이트합니다
 * 
 * @param game 게임 상태 포인터
 */
void game_update_statistics(game_state_t* game) {
    if (!game) return;
    
    game->actual_play_time = game_get_play_time(game);
    
    // 메모리에서만 통계 유지 (파일 저장 안함)
}

/**
 * @brief 게임 리소스를 정리합니다
 * 
 * @param game 게임 상태 포인터
 */
void game_cleanup(game_state_t* game) {
    if (!game) return;
    
    // 통계 업데이트
    game_update_statistics(game);
    
    // 모든 플레이어의 뱀 메모리 해제
    for (int i = 0; i < game->num_players; i++) {
        free_snake(&game->players[i]);
    }
    
    platform_destroy_mutex(game->game_mutex);
}

/**
 * @brief 게임에 새로운 플레이어를 추가합니다
 * 
 * @param game 게임 상태 포인터
 * @param type 플레이어 유형
 */
void game_add_player(game_state_t* game, player_type_t type) {
    if (game->num_players >= MAX_PLAYERS) return;
    
    int id = game->num_players;
    snake_t* snake = &game->players[id];
    
    snake->id = id;
    snake->type = type;
    snake->direction = (id == 0) ? DIR_RIGHT : DIR_LEFT; // 서로 반대 방향으로 시작
    snake->next_direction = snake->direction;
    snake->length = 4;
    snake->score = 0;
    snake->alive = true;
    snake->color = player_colors[id];
    snake->head_color = player_head_colors[id];
    snake->move_progress = 0.0f;
    snake->last_pos = (position_t){0, 0};
    
    // 플레이어별 시작 위치 설정 (대전 모드 고려)
    position_t start_positions[MAX_PLAYERS] = {
        {10, 20},  // 사용자: 왼쪽 중앙
        {30, 20}   // AI: 오른쪽 중앙
    };
    
    position_t start_pos = start_positions[id];
    
    // 초기 뱀 몸통 생성
    snake_node_t* prev_node = NULL;
    for (int i = 0; i < snake->length; i++) {
        position_t pos;
        if (id == 0) {
            // 사용자는 왼쪽에서 오른쪽으로
            pos = (position_t){start_pos.x - i, start_pos.y};
        } else {
            // AI는 오른쪽에서 왼쪽으로
            pos = (position_t){start_pos.x + i, start_pos.y};
        }
        
        snake_node_t* node = create_snake_node(pos);
        
        if (i == 0) {
            // 첫 번째 노드는 머리
            snake->head = node;
            snake->tail = node;
        } else {
            // 이전 노드와 현재 노드 연결
            prev_node->next = node;
            snake->tail = node;
        }
        
        prev_node = node;
        
        // 맵에 표시
        if (i == 0) {
            game->map[pos.y][pos.x] = CELL_SNAKE_HEAD;
        } else {
            game->map[pos.y][pos.x] = CELL_SNAKE_BODY;
        }
    }
    
    // 플레이어 수 증가
    game->num_players++;
}

/**
 * @brief 게임 상태를 업데이트합니다
 * 
 * @param game 게임 상태 포인터
 * @return 게임이 계속되면 true, 종료되면 false
 */
bool game_update(game_state_t* game) {
    if (!game || game->game_over) return false;
    
    // 일시정지 상태면 업데이트하지 않음
    if (game->state == GAME_STATE_PAUSED) {
        return true;
    }
    
    platform_lock_mutex(game->game_mutex);
    
    int alive_count = 0;
    int last_alive = -1;
    
    // 각 뱀 업데이트
    for (int i = 0; i < game->num_players; i++) {
        snake_t* snake = &game->players[i];
        if (!snake->alive) continue;
        
        alive_count++;
        last_alive = i;
        
        // 방향 업데이트 (역방향으로는 이동 불가)
        if (!is_opposite_direction(snake->direction, snake->next_direction)) {
            snake->direction = snake->next_direction;
        }
        
        // 마지막 위치 저장 (부드러운 모션용)
        snake->last_pos = snake->head->pos;
        snake->move_progress = 0.0f; // 새로운 움직임 시작
        
        // 다음 머리 위치 계산
        position_t next_pos = get_next_position(snake->head->pos, snake->direction);
        
        // 벽 충돌 검사
        if (!is_valid_position(next_pos)) {
            snake->alive = false;
            continue;
        }
        
        // 다음 위치의 내용물 확인
        char cell = game->map[next_pos.y][next_pos.x];
        bool grow = false;
        
        if (cell == CELL_APPLE) {
            grow = true;
            snake->score += 100;  // 사과 점수
            game->apples_eaten++;
            game_generate_apple(game);

            // 사과를 먹었을 때 80% 확률로 장애물 생성
            if (platform_random(1, 100) <= 80) {
                game_generate_obstacle(game);
            }

            // 게임 속도 점진적 증가
            if (game->game_speed > 80) {
                game->game_speed -= 1;
            }
        } else if (cell == CELL_SNAKE_HEAD || cell == CELL_SNAKE_BODY) {
            snake->alive = false;
            continue;
        } else if (cell == CELL_OBSTACLE) {
            snake->alive = false;
            continue;
        }
        
        // 뱀 이동
        snake_node_t* new_head = create_snake_node(next_pos);
        if (!new_head) {
            snake->alive = false;
            continue;
        }
        
        // 맵 업데이트 - 기존 머리를 몸통으로 변경
        game->map[snake->head->pos.y][snake->head->pos.x] = CELL_SNAKE_BODY;
        
        // 새 머리 추가
        new_head->next = snake->head;
        snake->head = new_head;
        snake->length++;
        
        // 새 머리를 맵에 표시
        game->map[next_pos.y][next_pos.x] = CELL_SNAKE_HEAD;
        
        // 성장하지 않을 때 꼬리 제거
        if (!grow && snake->tail) {
            snake_node_t* old_tail = snake->tail;
            snake_node_t* current = snake->head;
            
            // 새로운 꼬리 찾기
            while (current->next != snake->tail) {
                current = current->next;
            }
            
            snake->tail = current;
            snake->tail->next = NULL;
            
            // 기존 꼬리를 맵에서 제거
            game->map[old_tail->pos.y][old_tail->pos.x] = CELL_EMPTY;
            free(old_tail);
            snake->length--;
        }
        
        snake->score++; // 이동 점수
    }
    
    // 게임 종료 조건 확인
    if (game->mode == GAME_MODE_SINGLE) {
        // 싱글 플레이어: 플레이어가 죽으면 게임 종료
        if (alive_count == 0) {
            game->game_over = true;
            game->state = GAME_STATE_GAME_OVER;
        }
    } else {
        // AI 대전 모드: 1명 이하 생존시 게임 종료
        if (alive_count <= 1) {
            game->game_over = true;
            game->state = GAME_STATE_GAME_OVER;
            if (alive_count == 1) {
                game->winner_id = last_alive;
            }
        }
    }
    
    platform_unlock_mutex(game->game_mutex);
    return !game->game_over;
}

/**
 * @brief 게임 화면을 렌더링합니다
 * 
 * @param game 게임 상태 포인터
 */
void game_render(game_state_t* game) {
    if (!game) return;
    
    static bool first_render = true;
    static uint64_t last_game_start = 0;
    
    // 새 게임용 first_render 플래그 리셋
    if (game->game_start_time != last_game_start) {
        first_render = true;
        last_game_start = game->game_start_time;
    }
    
    // 깜박임 방지를 위해 첫 렌더링에만 화면 클리어
    if (first_render) {
        platform_clear_screen();
        first_render = false;
        
        // 테두리 그리기 (한 번만)
        platform_set_color(COLOR_WHITE);
        for (int x = 0; x < GAME_WIDTH + 2; x++) {
            platform_print_at(x * 2, 0, "██");
            platform_print_at(x * 2, GAME_HEIGHT + 1, "██");
        }
        for (int y = 1; y <= GAME_HEIGHT; y++) {
            platform_print_at(0, y, "██");
            platform_print_at((GAME_WIDTH + 1) * 2, y, "██");
        }
    }
    
    // 게임 필드 그리기
    for (int y = 0; y < GAME_HEIGHT; y++) {
        for (int x = 0; x < GAME_WIDTH; x++) {
            char cell = game->map[y][x];
            position_t screen_pos = {(x + 1) * 2, y + 1};
            
            switch (cell) {
                case CELL_EMPTY:
                    platform_set_color(COLOR_BLACK);
                    platform_print_at(screen_pos.x, screen_pos.y, "  ");
                    break;
                    
                case CELL_APPLE:
                    platform_set_color(COLOR_BRIGHT_YELLOW);
                    platform_print_at(screen_pos.x, screen_pos.y, "🍎");
                    break;
                    
                case CELL_OBSTACLE:
                    platform_set_color(COLOR_BRIGHT_RED);
                    platform_print_at(screen_pos.x, screen_pos.y, "💣");
                    break;
                    
                case CELL_SNAKE_HEAD:
                case CELL_SNAKE_BODY:
                    // 어느 뱀에 속하는지 찾기
                    for (int i = 0; i < game->num_players; i++) {
                        snake_t* snake = &game->players[i];
                        if (!snake->alive) continue;
                        
                        snake_node_t* current = snake->head;
                        bool found = false;
                        
                        while (current && !found) {
                            if (current->pos.x == x && current->pos.y == y) {
                                if (current == snake->head) {
                                    platform_set_color(snake->head_color);
                                    platform_print_at(screen_pos.x, screen_pos.y, "[]");
                                } else {
                                    platform_set_color(snake->color);
                                    platform_print_at(screen_pos.x, screen_pos.y, "##");
                                }
                                found = true;
                            }
                            current = current->next;
                        }
                        
                        if (found) break;
                    }
                    break;
            }
        }
    }
    
    // 우측에 UI 정보 표시
    platform_set_color(COLOR_WHITE);
    int ui_x = (GAME_WIDTH + 3) * 2;
    
    platform_print_at(ui_x, 2, "*** 뱀 게임 ***");
    
    // 일시정지 상태 표시
    if (game->state == GAME_STATE_PAUSED) {
        platform_set_color(COLOR_BRIGHT_YELLOW);
        platform_print_at(ui_x, 4, "⏸️  일시정지 중");
        platform_set_color(COLOR_WHITE);
        platform_print_at(ui_x, 5, "SPACE로 재개");
    }
    
    // 게임 모드 표시
    const char* mode_names[] = {
        "혼자서 도전", "AI 대전 (쉬움)", "AI 대전 (보통)", "AI 대전 (어려움)"
    };
    platform_set_color(COLOR_BRIGHT_CYAN);
    platform_print_at(ui_x, game->state == GAME_STATE_PAUSED ? 7 : 4, mode_names[game->mode]);
    
    int info_start_y = game->state == GAME_STATE_PAUSED ? 9 : 6;
    
    if (game->mode == GAME_MODE_SINGLE) {
        // 싱글 플레이어 모드 정보 표시
        snake_t* player_snake = &game->players[0];
        
        // 점수 표시
        platform_set_color(COLOR_BRIGHT_GREEN);
        char score_text[32];
        snprintf(score_text, sizeof(score_text), "점수: %d", player_snake->score);
        platform_print_at(ui_x, info_start_y, score_text);
        
        // 길이 표시
        platform_set_color(COLOR_BRIGHT_CYAN);
        char length_text[32];
        snprintf(length_text, sizeof(length_text), "길이: %d", player_snake->length);
        platform_print_at(ui_x, info_start_y + 1, length_text);
        
        // 상태 표시
        platform_set_color(player_snake->alive ? COLOR_BRIGHT_GREEN : COLOR_BRIGHT_RED);
        platform_print_at(ui_x, info_start_y + 3, player_snake->alive ? "상태: 생존" : "상태: 사망");
    } else {
        // AI 대전 모드 정보 표시
        platform_print_at(ui_x, info_start_y, "플레이어 정보:");
        
        for (int i = 0; i < game->num_players; i++) {
            snake_t* snake = &game->players[i];
            char status[64];
            
            platform_set_color(snake->color);
            const char* player_name = (i == 0) ? "사용자" : "AI";
            snprintf(status, sizeof(status), "%s: %d점 %s", 
                    player_name, snake->score, snake->alive ? "생존" : "사망");
            platform_print_at(ui_x, info_start_y + 2 + i, status);
        }
    }
    
    // 속도와 장애물 정보
    platform_set_color(COLOR_BRIGHT_YELLOW);
    char speed_text[32];
    snprintf(speed_text, sizeof(speed_text), "속도: %d ms", game->game_speed);
    platform_print_at(ui_x, info_start_y + 6, speed_text);
    
    platform_set_color(COLOR_RED);
    char obstacles_text[32];
    snprintf(obstacles_text, sizeof(obstacles_text), "장애물: %d개", game->obstacles_count);
    platform_print_at(ui_x, info_start_y + 7, obstacles_text);
    
    // 플레이 시간 표시
    platform_set_color(COLOR_BRIGHT_MAGENTA);
    uint64_t play_time = game_get_play_time(game) / 1000;
    int minutes = (int)(play_time / 60);
    int seconds = (int)(play_time % 60);
    char time_text[32];
    snprintf(time_text, sizeof(time_text), "시간: %d:%02d", minutes, seconds);
    platform_print_at(ui_x, info_start_y + 8, time_text);
    
    // 조작 방법 안내
    platform_set_color(COLOR_WHITE);
    platform_print_at(ui_x, 15, "조작 방법:");
    platform_print_at(ui_x, 16, "↑↓←→ 또는 WASD");
    platform_print_at(ui_x, 17, "SPACE: 일시정지");
    platform_print_at(ui_x, 18, "ESC: 메뉴");
    
    if (game->game_over) {
        platform_set_color(COLOR_BRIGHT_RED);
        platform_print_at(ui_x, 20, "게임 종료!");
        
        if (game->mode != GAME_MODE_SINGLE) {
            if (game->winner_id >= 0) {
                char winner[32];
                const char* winner_name = (game->winner_id == 0) ? "사용자" : "AI";
                snprintf(winner, sizeof(winner), "승자: %s", winner_name);
                platform_print_at(ui_x, 21, winner);
            } else {
                platform_print_at(ui_x, 21, "무승부!");
            }
        }
        
        platform_set_color(COLOR_BRIGHT_YELLOW);
        platform_print_at(ui_x, 23, "ESC로 메뉴 이동");
    }
    
    platform_reset_color();
    
    // Windows에서 더블 버퍼링된 화면을 실제 콘솔에 출력
    platform_present_buffer();
}

/**
 * @brief 플레이어 입력을 처리합니다
 * 
 * @param game 게임 상태 포인터
 * @param player_id 플레이어 ID (0만 사용 - 사용자만 입력)
 * @param key 입력된 키
 */
void game_handle_input(game_state_t* game, int player_id, game_key_t key) {
    if (!game) return;
    
    // 일시정지 키 처리
    if (key == KEY_SPACE) {
        game_toggle_pause(game);
        return;
    }
    
    // 일시정지 중이면 다른 입력 무시
    if (game->state == GAME_STATE_PAUSED) {
        return;
    }
    
    // 사용자(플레이어 0)만 입력 처리
    if (player_id != 0 || !game->players[0].alive) {
        return;
    }
    
    snake_t* snake = &game->players[0];
    direction_t new_dir = snake->next_direction;
    
    // 사용자 입력 처리: 화살표 키 또는 WASD
    switch (key) {
        case KEY_UP:
        case KEY_W:
            new_dir = DIR_UP;
            break;
        case KEY_DOWN:
        case KEY_S:
            new_dir = DIR_DOWN;
            break;
        case KEY_LEFT:
        case KEY_A:
            new_dir = DIR_LEFT;
            break;
        case KEY_RIGHT:
        case KEY_D:
            new_dir = DIR_RIGHT;
            break;
        default:
            return; // 다른 키는 무시
    }
    
    // 현재 방향과 반대가 아닐 때만 업데이트
    if (!is_opposite_direction(snake->direction, new_dir)) {
        platform_lock_mutex(game->game_mutex);
        snake->next_direction = new_dir;
        platform_unlock_mutex(game->game_mutex);
    }
}

/**
 * @brief 새로운 사과를 생성합니다
 * 
 * @param game 게임 상태 포인터
 */
void game_generate_apple(game_state_t* game) {
    if (!game) return;
    
    // 빈 공간 찾기
    position_t empty_positions[GAME_WIDTH * GAME_HEIGHT];
    int empty_count = 0;
    
    for (int y = 0; y < GAME_HEIGHT; y++) {
        for (int x = 0; x < GAME_WIDTH; x++) {
            if (game->map[y][x] == CELL_EMPTY) {
                empty_positions[empty_count++] = (position_t){x, y};
            }
        }
    }
    
    if (empty_count > 0) {
        int index = platform_random(0, empty_count - 1);
        position_t pos = empty_positions[index];
        game->map[pos.y][pos.x] = CELL_APPLE;
        game->apples_count++;
    }
}

/**
 * @brief 새로운 장애물을 생성합니다
 * 
 * @param game 게임 상태 포인터
 */
void game_generate_obstacle(game_state_t* game) {
    if (!game) return;
    
    // 빈 공간 찾기
    position_t empty_positions[GAME_WIDTH * GAME_HEIGHT];
    int empty_count = 0;
    
    for (int y = 0; y < GAME_HEIGHT; y++) {
        for (int x = 0; x < GAME_WIDTH; x++) {
            if (game->map[y][x] == CELL_EMPTY) {
                empty_positions[empty_count++] = (position_t){x, y};
            }
        }
    }
    
    if (empty_count > 0) {
        int index = platform_random(0, empty_count - 1);
        position_t pos = empty_positions[index];
        game->map[pos.y][pos.x] = CELL_OBSTACLE;
        game->obstacles_count++;
    }
}

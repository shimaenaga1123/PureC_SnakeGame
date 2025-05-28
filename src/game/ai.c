#include "ai.h"
#include "game.h"
#include <stdlib.h>
#include <limits.h>

/**
 * @brief AI 난이도별 매개변수를 정의하는 구조체
 */
typedef struct {
    int look_ahead_depth;          // 미리보기 깊이
    float aggression;              // 공격성 (0.0 = 방어적, 1.0 = 공격적)
    float randomness;              // 무작위성 (0.0 = 예측 가능, 1.0 = 무작위)
    float risk_tolerance;          // 위험 감수 정도
    float food_priority;           // 음식 우선순위
} ai_params_t;

/**
 * @brief AI 성향별 보정값을 정의하는 구조체
 */
typedef struct {
    float aggression_modifier;     // 공격성 보정
    float risk_modifier;           // 위험 감수 보정
    float patience_modifier;       // 인내심 보정
    float territorial_modifier;    // 영역 의식 보정
} ai_personality_modifiers_t;

// AI 난이도별 기본 설정값
static const ai_params_t ai_difficulties[] = {
    {2, 0.3f, 0.3f, 0.4f, 0.6f},  // 쉬움: 짧은 미리보기, 방어적, 무작위, 낮은 위험 감수
    {4, 0.5f, 0.2f, 0.6f, 0.8f},  // 보통: 중간 미리보기, 균형잡힌, 약간 예측 가능
    {6, 0.7f, 0.1f, 0.8f, 0.9f}   // 어려움: 긴 미리보기, 공격적, 매우 예측 가능
};

// AI 성향별 보정값
static const ai_personality_modifiers_t personality_modifiers[] = {
    {0.0f, 0.0f, 0.0f, 0.0f},     // 균형잡힌: 보정 없음
    {0.4f, 0.3f, -0.2f, 0.2f},    // 공격적: 높은 공격성, 낮은 인내심
    {-0.3f, -0.4f, 0.3f, -0.1f},  // 방어적: 낮은 공격성, 높은 인내심
    {-0.2f, -0.5f, 0.4f, 0.1f},   // 신중한: 매우 낮은 위험 감수
    {0.5f, 0.6f, -0.4f, -0.2f}    // 무모한: 매우 높은 위험 감수
};

/**
 * @brief 두 점 사이의 맨하탄 거리를 계산합니다
 * 
 * @param a 첫 번째 점
 * @param b 두 번째 점
 * @return 맨하탄 거리
 */
static int manhattan_distance(position_t a, position_t b) {
    return abs(a.x - b.x) + abs(a.y - b.y);
}

/**
 * @brief 뱀 머리에서 가장 가까운 사과를 찾습니다
 * 
 * @param game 게임 상태 포인터
 * @param snake_head 뱀 머리의 위치
 * @return 가장 가까운 사과의 위치 (없으면 {-1, -1})
 */
static position_t find_nearest_apple(game_state_t* game, position_t snake_head) {
    position_t nearest = {-1, -1};
    int min_distance = INT_MAX;
    
    for (int y = 0; y < GAME_HEIGHT; y++) {
        for (int x = 0; x < GAME_WIDTH; x++) {
            if (game->map[y][x] == CELL_APPLE) {
                position_t apple_pos = {x, y};
                int distance = manhattan_distance(snake_head, apple_pos);
                if (distance < min_distance) {
                    min_distance = distance;
                    nearest = apple_pos;
                }
            }
        }
    }
    
    return nearest;
}

/**
 * @brief 상대방 뱀의 머리 위치를 찾습니다
 * 
 * @param game 게임 상태 포인터
 * @param my_snake_id 자신의 뱀 ID
 * @return 상대방 뱀 머리 위치 (없으면 {-1, -1})
 */
static position_t find_opponent_head(game_state_t* game, int my_snake_id) {
    for (int i = 0; i < game->num_players; i++) {
        if (i != my_snake_id && game->players[i].alive) {
            return game->players[i].head->pos;
        }
    }
    return (position_t){-1, -1};
}

/**
 * @brief 특정 위치가 안전한지 확인합니다 (충돌 없음)
 * 
 * @param game 게임 상태 포인터
 * @param pos 확인할 위치
 * @param snake_id 확인하는 뱀의 ID
 * @return 안전하면 true, 위험하면 false
 */
static bool is_safe_position(game_state_t* game, position_t pos, int snake_id) {
    if (!is_valid_position(pos)) return false;
    
    char cell = game->map[pos.y][pos.x];
    if (cell == CELL_OBSTACLE) return false;
    
    // 모든 뱀과의 충돌 검사 (자신 포함)
    for (int i = 0; i < game->num_players; i++) {
        snake_t* snake = &game->players[i];
        if (!snake->alive) continue;
        
        snake_node_t* current = snake->head;
        while (current) {
            // 자신의 꼬리는 제외 (이동할 것이므로)
            if (i == snake_id && current == snake->tail) {
                current = current->next;
                continue;
            }
            
            if (current->pos.x == pos.x && current->pos.y == pos.y) {
                return false;
            }
            current = current->next;
        }
    }
    
    return true;
}

/**
 * @brief 미리보기를 통한 위치 안전성 평가
 * 
 * @param game 게임 상태 포인터
 * @param pos 평가할 위치
 * @param depth 미리보기 깊이
 * @param snake_id 뱀 ID
 * @return 안전한 이동 경로 수
 */
static int evaluate_position_safety(game_state_t* game, position_t pos, int depth, int snake_id) {
    if (depth <= 0) return 1;
    if (!is_safe_position(game, pos, snake_id)) return 0;
    
    int safe_moves = 0;
    
    // 이 위치에서 가능한 모든 움직임 검사
    for (int dir = 0; dir < 4; dir++) {
        position_t next_pos = get_next_position(pos, (direction_t)dir);
        if (evaluate_position_safety(game, next_pos, depth - 1, snake_id)) {
            safe_moves++;
        }
    }
    
    return safe_moves;
}

/**
 * @brief 영역 제어 점수를 계산합니다 (공격적 AI용)
 * 
 * @param pos 평가할 위치
 * @param opponent_pos 상대방 위치
 * @return 영역 제어 점수
 */
static int evaluate_territorial_control(position_t pos, position_t opponent_pos) {
    if (opponent_pos.x == -1 || opponent_pos.y == -1) return 0;
    
    int control_score = 0;
    
    // 상대방과의 거리
    int distance_to_opponent = manhattan_distance(pos, opponent_pos);
    
    // 중앙 영역 선호
    int center_x = GAME_WIDTH / 2;
    int center_y = GAME_HEIGHT / 2;
    int distance_to_center = manhattan_distance(pos, (position_t){center_x, center_y});
    
    // 공격적일 때는 상대방에게 가까워지고, 방어적일 때는 멀어짐
    control_score += (20 - distance_to_opponent);
    control_score += (15 - distance_to_center);
    
    return control_score;
}

/**
 * @brief AI 의사 결정 - 최적의 이동 방향을 계산합니다
 * 
 * @param game 게임 상태 포인터
 * @param snake_id 뱀 ID
 * @param ai_personality AI 특성 (0=균형, 1=공격적, 2=방어적, 3=신중, 4=무모)
 * @return 최적의 이동 방향
 */
direction_t ai_get_best_move(game_state_t* game, int snake_id, int ai_personality) {
    snake_t* snake = &game->players[snake_id];
    if (!snake->alive) return snake->direction;
    
    // AI 난이도에 따른 기본 매개변수 가져오기
    ai_params_t params;
    switch (snake->type) {
        case PLAYER_AI_EASY:
            params = ai_difficulties[0];
            break;
        case PLAYER_AI_MEDIUM:
            params = ai_difficulties[1];
            break;
        case PLAYER_AI_HARD:
            params = ai_difficulties[2];
            break;
        default:
            params = ai_difficulties[1];
            break;
    }
    
    // 전달받은 AI 특성 사용 (0=균형, 1=공격적, 2=방어적, 3=신중, 4=무모)
    int personality = ai_personality;
    ai_personality_modifiers_t personality_mod = personality_modifiers[personality];
    
    // 성향에 따른 매개변수 보정
    params.aggression += personality_mod.aggression_modifier;
    params.risk_tolerance += personality_mod.risk_modifier;
    
    // 범위 제한
    if (params.aggression < 0.0f) params.aggression = 0.0f;
    if (params.aggression > 1.0f) params.aggression = 1.0f;
    if (params.risk_tolerance < 0.0f) params.risk_tolerance = 0.0f;
    if (params.risk_tolerance > 1.0f) params.risk_tolerance = 1.0f;
    
    // 무작위성 추가 - 설정된 확률로 무작위 움직임
    if ((float)rand() / (float)RAND_MAX < params.randomness) {
        direction_t random_dirs[4];
        int valid_count = 0;
        
        for (int dir = 0; dir < 4; dir++) {
            if (!is_opposite_direction(snake->direction, (direction_t)dir)) {
                position_t next_pos = get_next_position(snake->head->pos, (direction_t)dir);
                if (is_safe_position(game, next_pos, snake_id)) {
                    random_dirs[valid_count++] = (direction_t)dir;
                }
            }
        }
        
        if (valid_count > 0) {
            return random_dirs[rand() % valid_count];
        }
    }
    
    // 가장 가까운 사과와 상대방 위치 찾기
    position_t apple_pos = find_nearest_apple(game, snake->head->pos);
    position_t opponent_pos = find_opponent_head(game, snake_id);
    
    direction_t best_move = snake->direction;
    int best_score = -1;
    
    // 모든 가능한 움직임 평가
    for (int dir = 0; dir < 4; dir++) {
        direction_t test_dir = (direction_t)dir;
        
        // 역방향 이동 금지
        if (is_opposite_direction(snake->direction, test_dir)) {
            continue;
        }
        
        position_t next_pos = get_next_position(snake->head->pos, test_dir);
        
        // 즉시 안전성 검사
        if (!is_safe_position(game, next_pos, snake_id)) {
            continue;
        }
        
        int score = 0;
        
        // 1. 안전성 평가 (기본 가중치)
        int safety_score = evaluate_position_safety(game, next_pos, params.look_ahead_depth, snake_id);
        score += safety_score * 100;
        
        // 2. 사과까지의 거리 (음식 우선순위에 따라)
        if (apple_pos.x >= 0 && apple_pos.y >= 0) {
            int apple_distance = manhattan_distance(next_pos, apple_pos);
            score += (50 - apple_distance) * (int)(params.food_priority * params.aggression * 10);
        }
        
        // 3. 영역 제어 점수 (성향에 따라)
        if (opponent_pos.x >= 0 && opponent_pos.y >= 0) {
            int territorial_score = evaluate_territorial_control(next_pos, opponent_pos);
            
            if (personality == 1 || personality == 4) { // 공격적 || 무모한
                // 공격적/무모한 AI는 상대방에게 접근
                score += territorial_score * (int)(params.aggression * 5);
            } else if (personality == 2 || personality == 3) { // 방어적 || 신중한
                // 방어적/신중한 AI는 상대방을 회피
                score -= territorial_score * (int)((1.0f - params.risk_tolerance) * 3);
            }
        }
        
        // 4. 중앙 지향 (기본 전략)
        int center_x = GAME_WIDTH / 2;
        int center_y = GAME_HEIGHT / 2;
        int distance_to_center = manhattan_distance(next_pos, (position_t){center_x, center_y});
        score += (30 - distance_to_center) * 2;
        
        // 5. 현재 방향 유지 보너스 (부드러운 움직임)
        if (test_dir == snake->direction) {
            score += 5;
        }
        
        // 6. 벽 회피 (성향에 따른 보정)
        int wall_distance = 0;
        wall_distance += next_pos.x; // 왼쪽 벽까지의 거리
        wall_distance += next_pos.y; // 위쪽 벽까지의 거리
        wall_distance += (GAME_WIDTH - 1 - next_pos.x); // 오른쪽 벽까지의 거리
        wall_distance += (GAME_HEIGHT - 1 - next_pos.y); // 아래쪽 벽까지의 거리
        
        if (personality == 3) { // 신중한
            // 신중한 AI는 벽을 더 많이 회피
            score += wall_distance * 3;
        } else if (personality == 4) { // 무모한
            // 무모한 AI는 벽을 덜 회피
            score += wall_distance;
        } else {
            score += wall_distance * 2;
        }
        
        if (score > best_score) {
            best_score = score;
            best_move = test_dir;
        }
    }
    
    return best_move;
}

/**
 * @brief 모든 AI 플레이어들을 업데이트합니다
 * 
 * @param game 게임 상태 포인터
 * @param ai_personality AI 특성 (0=균형, 1=공격적, 2=방어적, 3=신중, 4=무모)
 */
void ai_update_players(game_state_t* game, int ai_personality) {
    for (int i = 0; i < game->num_players; i++) {
        snake_t* snake = &game->players[i];
        
        // AI 플레이어만 업데이트 (인간 플레이어 제외)
        if (snake->alive && snake->type != PLAYER_HUMAN) {
            direction_t best_move = ai_get_best_move(game, i, ai_personality);
            snake->next_direction = best_move;
        }
    }
}

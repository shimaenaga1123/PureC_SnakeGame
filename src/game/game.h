#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdbool.h>
#include "../platform/platform.h"

// 게임 영역 크기 상수
#define GAME_WIDTH 40              // 게임 가로 크기
#define GAME_HEIGHT 40             // 게임 세로 크기
#define MAX_PLAYERS 2              // 최대 플레이어 수 (사용자 + AI)

/**
 * @brief 뱀의 이동 방향을 나타내는 열거형
 */
typedef enum {
    DIR_RIGHT = 0,                 // 우측
    DIR_LEFT = 1,                  // 좌측  
    DIR_UP = 2,                    // 위쪽
    DIR_DOWN = 3                   // 아래쪽
} direction_t;

/**
 * @brief 게임 모드를 나타내는 열거형
 */
typedef enum {
    GAME_MODE_SINGLE,              // 싱글 플레이어 (혼자서 점수 도전)
    GAME_MODE_VS_AI_EASY,          // AI 대전 - 쉬움
    GAME_MODE_VS_AI_MEDIUM,        // AI 대전 - 보통
    GAME_MODE_VS_AI_HARD           // AI 대전 - 어려움
} game_mode_t;

/**
 * @brief 플레이어 유형을 나타내는 열거형
 */
typedef enum {
    PLAYER_HUMAN,                  // 인간 플레이어
    PLAYER_AI_EASY,                // 쉬운 AI
    PLAYER_AI_MEDIUM,              // 중간 AI
    PLAYER_AI_HARD                 // 어려운 AI
} player_type_t;

/**
 * @brief 게임 상태를 나타내는 열거형
 */
typedef enum {
    GAME_STATE_PLAYING,            // 플레이 중
    GAME_STATE_PAUSED,             // 일시정지
    GAME_STATE_GAME_OVER           // 게임 오버
} game_state_enum_t;

/**
 * @brief 2D 좌표를 나타내는 구조체
 */
typedef struct {
    int x, y;                      // X, Y 좌표
} position_t;

/**
 * @brief 부드러운 모션을 위한 보간 위치 구조체
 */
typedef struct {
    float x, y;                    // 실제 렌더링 위치 (부동소수점)
} smooth_position_t;

/**
 * @brief 뱀의 각 몸통 부분을 나타내는 연결 리스트 노드
 */
typedef struct snake_node {
    position_t pos;                // 논리적 위치 정보
    smooth_position_t smooth_pos;  // 부드러운 렌더링 위치
    struct snake_node* next;       // 다음 노드 포인터
} snake_node_t;

/**
 * @brief 뱀 플레이어 정보를 담는 구조체
 */
typedef struct {
    int id;                        // 플레이어 ID
    player_type_t type;            // 플레이어 유형 (인간/AI)
    snake_node_t* head;            // 뱀의 머리 노드
    snake_node_t* tail;            // 뱀의 꼬리 노드
    direction_t direction;         // 현재 이동 방향
    direction_t next_direction;    // 다음 이동 방향
    int length;                    // 뱀의 길이
    int score;                     // 점수
    bool alive;                    // 생존 여부
    color_t color;                 // 뱀 몸통 색상
    color_t head_color;            // 뱀 머리 색상
    
    // 부드러운 모션 관련
    float move_progress;           // 이동 진행도 (0.0 ~ 1.0)
    position_t last_pos;           // 마지막 위치 (보간용)
} snake_t;

/**
 * @brief 게임 맵의 셀 유형을 나타내는 열거형
 */
typedef enum {
    CELL_EMPTY = '.',              // 빈 공간
    CELL_APPLE = 'A',              // 사과
    CELL_OBSTACLE = '#',           // 장애물
    CELL_SNAKE_HEAD = 'H',         // 뱀 머리
    CELL_SNAKE_BODY = 'B'          // 뱀 몸통
} cell_type_t;

// 오디오 시스템 전방 선언
struct audio_system;

/**
 * @brief 게임의 전체 상태를 나타내는 구조체
 */
typedef struct {
    char map[GAME_HEIGHT][GAME_WIDTH];      // 게임 맵
    snake_t players[MAX_PLAYERS];           // 플레이어 배열
    int num_players;                        // 현재 플레이어 수
    game_mode_t mode;                       // 게임 모드
    game_state_enum_t state;                // 게임 상태 (플레이/일시정지/종료)
    int apples_count;                       // 사과 개수
    int obstacles_count;                    // 장애물 개수
    bool game_over;                         // 게임 종료 여부
    int winner_id;                          // 승자 ID (-1이면 승자 없음)
    uint64_t game_start_time;               // 게임 시작 시간
    uint64_t pause_start_time;              // 일시정지 시작 시간
    uint64_t total_pause_time;              // 총 일시정지 시간
    int game_speed;                         // 게임 속도 (밀리초)
    bool smooth_motion_enabled;             // 부드러운 모션 활성화 여부
    float motion_interpolation;             // 모션 보간 계수 (0.0 ~ 1.0)
    mutex_handle_t game_mutex;              // 게임 상태 동기화용 뮤텍스

    // 통계 정보
    int apples_eaten;                       // 먹은 사과 수
    uint64_t actual_play_time;              // 실제 플레이 시간 (일시정지 제외)
} game_state_t;

// 함수 선언
bool game_init(game_state_t* game, game_mode_t mode);
void game_cleanup(game_state_t* game);
bool game_update(game_state_t* game);
void game_update_smooth_motion(game_state_t* game, float delta_time);
void game_render(game_state_t* game);
void game_handle_input(game_state_t* game, int player_id, game_key_t key);
void game_toggle_pause(game_state_t* game);
bool game_is_paused(const game_state_t* game);
void game_add_player(game_state_t* game, player_type_t type);
void game_generate_apple(game_state_t* game);
void game_generate_obstacle(game_state_t* game);

// 통계 관련 함수들
uint64_t game_get_play_time(game_state_t* game);
void game_update_statistics(game_state_t* game);

// AI를 위한 헬퍼 함수들
bool is_valid_position(position_t pos);
position_t get_next_position(position_t pos, direction_t dir);
bool is_opposite_direction(direction_t dir1, direction_t dir2);

// 부드러운 모션 헬퍼 함수들
float lerp(float a, float b, float t);
smooth_position_t lerp_position(position_t a, position_t b, float t);

#endif // GAME_H

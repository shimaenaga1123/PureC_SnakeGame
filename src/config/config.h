/**
 * @file config.h
 * @brief 설정 파일 관리 시스템 헤더 파일
 * 
 * 게임 설정을 파일로 저장하고 로드하는 기능을 제공합니다.
 * 사용자 설정, 키 바인딩, 오디오 설정 등을 관리합니다.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "../platform/platform.h"

#define CONFIG_FILE_PATH "config/settings.cfg"
#define SCORES_FILE_PATH "config/highscores.dat"
#define MAX_HIGHSCORES 10
#define MAX_KEY_BINDINGS 16

/**
 * @brief 키 바인딩 구조체
 */
typedef struct {
    game_key_t up;              // 위쪽 이동 키
    game_key_t down;            // 아래쪽 이동 키
    game_key_t left;            // 왼쪽 이동 키
    game_key_t right;           // 오른쪽 이동 키
    game_key_t pause;           // 일시정지 키
    game_key_t menu;            // 메뉴 키
} key_bindings_t;

/**
 * @brief AI 성향을 나타내는 열거형
 */
typedef enum {
    AI_PERSONALITY_BALANCED,    // 균형잡힌 (기본)
    AI_PERSONALITY_AGGRESSIVE,  // 공격적
    AI_PERSONALITY_DEFENSIVE,   // 방어적
    AI_PERSONALITY_CAUTIOUS,    // 신중한
    AI_PERSONALITY_RECKLESS     // 무모한
} ai_personality_t;

/**
 * @brief 최고 점수 기록 구조체
 */
typedef struct {
    char player_name[32];       // 플레이어 이름
    int score;                  // 점수
    int length;                 // 뱀 길이
    uint64_t timestamp;         // 기록 시간
    int game_mode;              // 게임 모드
    char rank_name[16];         // 등급명
} highscore_entry_t;

/**
 * @brief 게임 설정 구조체
 */
typedef struct {
    // 게임 설정
    int game_speed_setting;     // 게임 속도 설정 (0-2)
    bool show_grid;             // 격자 표시
    bool show_fps;              // FPS 표시
    bool auto_pause;            // 자동 일시정지
    bool screen_shake;          // 화면 흔들림
    int controls_scheme;        // 조작 방식
    bool smooth_motion;         // 부드러운 모션
    
    // AI 설정
    ai_personality_t ai_personality; // AI 성향
    
    // 키 바인딩
    key_bindings_t key_bindings;
    
    // 게임플레이 통계
    int total_games_played;     // 총 게임 수
    int total_apples_eaten;     // 총 먹은 사과 수
    uint64_t total_play_time;   // 총 플레이 시간 (초)
    int best_score;             // 최고 점수
    
    // 설정 파일 버전 (호환성 체크용)
    int config_version;
} game_config_t;

// 설정 관리 함수들
bool config_init(void);
void config_cleanup(void);
bool config_load(game_config_t* config);
bool config_save(const game_config_t* config);
void config_set_defaults(game_config_t* config);

// 최고 점수 관리 함수들
bool highscores_load(highscore_entry_t scores[MAX_HIGHSCORES], int* count);
bool highscores_save(const highscore_entry_t scores[MAX_HIGHSCORES], int count);
bool highscores_add_entry(const highscore_entry_t* entry);
bool highscores_is_high_score(int score);
void highscores_get_rank_name(int score, char* rank_name, size_t size);

// 유틸리티 함수들
const char* config_get_key_name(game_key_t key);
game_key_t config_parse_key_name(const char* name);
const char* config_get_ai_personality_name(ai_personality_t personality);

#endif // CONFIG_H

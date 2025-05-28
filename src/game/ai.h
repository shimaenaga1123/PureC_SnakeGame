/**
 * @file ai.h
 * @brief AI 플레이어 관련 함수들의 헤더 파일
 * 
 * 뱀 게임의 AI 플레이어 동작을 제어하는 함수들을 선언합니다.
 * 다양한 난이도의 AI와 전략적 움직임을 구현합니다.
 */

#ifndef AI_H
#define AI_H

#include "game.h"

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
 * @brief 특정 뱀에 대한 최적의 이동 방향을 계산합니다
 * 
 * AI 난이도에 따라 다른 전략을 사용하여 최적의 움직임을 결정합니다.
 * 안전성, 사과까지의 거리, 다른 뱀들과의 위치 관계를 고려합니다.
 * 
 * @param game 게임 상태 포인터
 * @param snake_id 대상 뱀의 ID
 * @param ai_personality AI 특성 (0=균형, 1=공격적, 2=방어적, 3=신중, 4=무모)
 * @return 최적의 이동 방향
 */
direction_t ai_get_best_move(game_state_t* game, int snake_id, int ai_personality);

/**
 * @brief 모든 AI 플레이어들의 다음 움직임을 업데이트합니다
 * 
 * 게임의 모든 AI 플레이어들에 대해 최적의 이동 방향을 계산하고
 * 각 뱀의 next_direction을 설정합니다.
 * 
 * @param game 게임 상태 포인터
 * @param ai_personality AI 특성 (0=균형, 1=공격적, 2=방어적, 3=신중, 4=무모)
 */
void ai_update_players(game_state_t* game, int ai_personality);

#endif // AI_H

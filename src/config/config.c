/**
 * @file config.c
 * @brief 설정 파일 관리 시스템 구현
 * 
 * 게임 설정과 최고 점수를 파일로 저장하고 로드하는 기능을 구현합니다.
 */

#include "config.h"
#include "../platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CONFIG_VERSION 1
#define MAX_LINE_LENGTH 256
#define MAX_KEY_LENGTH 64
#define MAX_VALUE_LENGTH 128

// 전역 설정 변수
static game_config_t g_config;
static bool g_config_initialized = false;

// 키 이름 매핑 테이블
static const struct {
    game_key_t key;
    const char* name;
} key_name_table[] = {
    {KEY_NONE, "NONE"},
    {KEY_UP, "UP"},
    {KEY_DOWN, "DOWN"},
    {KEY_LEFT, "LEFT"},
    {KEY_RIGHT, "RIGHT"},
    {KEY_ENTER, "ENTER"},
    {KEY_ESC, "ESC"},
    {KEY_SPACE, "SPACE"},
    {KEY_W, "W"},
    {KEY_A, "A"},
    {KEY_S, "S"},
    {KEY_D, "D"},
    {KEY_1, "1"},
    {KEY_2, "2"},
    {KEY_3, "3"},
    {KEY_4, "4"}
};

// AI 성향 이름 테이블
static const char* ai_personality_names[] = {
    "균형잡힌",
    "공격적",
    "방어적", 
    "신중한",
    "무모한"
};

/**
 * @brief 문자열을 트림합니다 (앞뒤 공백 제거)
 */
static char* trim_string(char* str) {
    if (!str) return NULL;
    
    // 앞쪽 공백 제거
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') {
        str++;
    }
    
    // 뒤쪽 공백 제거
    char* end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
    
    return str;
}

/**
 * @brief 설정 시스템을 초기화합니다
 */
bool config_init(void) {
    if (g_config_initialized) {
        return true;
    }
    
    // 기본값 설정
    config_set_defaults(&g_config);
    
    // 설정 파일 로드 시도
    if (!config_load(&g_config)) {
        // 로드 실패시 기본값으로 저장
        config_save(&g_config);
    }
    
    g_config_initialized = true;
    return true;
}

/**
 * @brief 설정 시스템을 정리합니다
 */
void config_cleanup(void) {
    if (g_config_initialized) {
        config_save(&g_config);
        g_config_initialized = false;
    }
}

/**
 * @brief 기본 설정값을 설정합니다
 */
void config_set_defaults(game_config_t* config) {
    if (!config) return;
    
    memset(config, 0, sizeof(game_config_t));
    
    // 게임 설정 기본값
    config->game_speed_setting = 1;        // 보통
    config->show_grid = false;
    config->show_fps = false;
    config->auto_pause = true;              // 기본적으로 활성화
    config->screen_shake = true;
    config->controls_scheme = 0;            // 화살표+WASD
    config->smooth_motion = true;           // 부드러운 모션 활성화
    
    // AI 설정
    config->ai_personality = AI_PERSONALITY_BALANCED;
    
    // 키 바인딩 기본값
    config->key_bindings.up = KEY_UP;
    config->key_bindings.down = KEY_DOWN;
    config->key_bindings.left = KEY_LEFT;
    config->key_bindings.right = KEY_RIGHT;
    config->key_bindings.pause = KEY_SPACE;
    config->key_bindings.menu = KEY_ESC;
    
    // 버전 정보
    config->config_version = CONFIG_VERSION;
}

/**
 * @brief 설정을 파일에서 로드합니다
 */
bool config_load(game_config_t* config) {
    if (!config) return false;
    
    FILE* file = fopen(CONFIG_FILE_PATH, "r");
    if (!file) {
        return false;
    }
    
    char line[MAX_LINE_LENGTH];
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
    
    while (fgets(line, sizeof(line), file)) {
        // 주석과 빈 줄 스킵
        char* trimmed = trim_string(line);
        if (strlen(trimmed) == 0 || trimmed[0] == '#') {
            continue;
        }
        
        // 키=값 파싱
        if (sscanf(trimmed, "%63[^=]=%127s", key, value) == 2) {
            char* trimmed_key = trim_string(key);
            char* trimmed_value = trim_string(value);
            
            // 설정값 적용
            if (strcmp(trimmed_key, "game_speed_setting") == 0) {
                config->game_speed_setting = atoi(trimmed_value);
            } else if (strcmp(trimmed_key, "show_grid") == 0) {
                config->show_grid = (strcmp(trimmed_value, "true") == 0);
            } else if (strcmp(trimmed_key, "show_fps") == 0) {
                config->show_fps = (strcmp(trimmed_value, "true") == 0);
            } else if (strcmp(trimmed_key, "auto_pause") == 0) {
                config->auto_pause = (strcmp(trimmed_value, "true") == 0);
            } else if (strcmp(trimmed_key, "screen_shake") == 0) {
                config->screen_shake = (strcmp(trimmed_value, "true") == 0);
            } else if (strcmp(trimmed_key, "controls_scheme") == 0) {
                config->controls_scheme = atoi(trimmed_value);
            } else if (strcmp(trimmed_key, "smooth_motion") == 0) {
                config->smooth_motion = (strcmp(trimmed_value, "true") == 0);
            } else if (strcmp(trimmed_key, "ai_personality") == 0) {
                config->ai_personality = atoi(trimmed_value);
            } else if (strcmp(trimmed_key, "key_up") == 0) {
                config->key_bindings.up = config_parse_key_name(trimmed_value);
            } else if (strcmp(trimmed_key, "key_down") == 0) {
                config->key_bindings.down = config_parse_key_name(trimmed_value);
            } else if (strcmp(trimmed_key, "key_left") == 0) {
                config->key_bindings.left = config_parse_key_name(trimmed_value);
            } else if (strcmp(trimmed_key, "key_right") == 0) {
                config->key_bindings.right = config_parse_key_name(trimmed_value);
            } else if (strcmp(trimmed_key, "key_pause") == 0) {
                config->key_bindings.pause = config_parse_key_name(trimmed_value);
            } else if (strcmp(trimmed_key, "key_menu") == 0) {
                config->key_bindings.menu = config_parse_key_name(trimmed_value);
            } else if (strcmp(trimmed_key, "total_games_played") == 0) {
                config->total_games_played = atoi(trimmed_value);
            } else if (strcmp(trimmed_key, "total_apples_eaten") == 0) {
                config->total_apples_eaten = atoi(trimmed_value);
            } else if (strcmp(trimmed_key, "total_play_time") == 0) {
                config->total_play_time = (uint64_t)atoll(trimmed_value);
            } else if (strcmp(trimmed_key, "best_score") == 0) {
                config->best_score = atoi(trimmed_value);
            }
        }
    }
    
    fclose(file);
    return true;
}

/**
 * @brief 설정을 파일에 저장합니다
 */
bool config_save(const game_config_t* config) {
    if (!config) return false;
    
    FILE* file = fopen(CONFIG_FILE_PATH, "w");
    if (!file) {
        return false;
    }
    
    fprintf(file, "# 크로스 플랫폼 뱀 게임 설정 파일\n");
    fprintf(file, "# 자동 생성됨 - 수동 편집 가능\n\n");
    
    fprintf(file, "# 게임 설정\n");
    fprintf(file, "game_speed_setting=%d\n", config->game_speed_setting);
    fprintf(file, "show_grid=%s\n", config->show_grid ? "true" : "false");
    fprintf(file, "show_fps=%s\n", config->show_fps ? "true" : "false");
    fprintf(file, "auto_pause=%s\n", config->auto_pause ? "true" : "false");
    fprintf(file, "screen_shake=%s\n", config->screen_shake ? "true" : "false");
    fprintf(file, "controls_scheme=%d\n", config->controls_scheme);
    fprintf(file, "smooth_motion=%s\n", config->smooth_motion ? "true" : "false");
    
    fprintf(file, "\n# AI 설정\n");
    fprintf(file, "ai_personality=%d\n", config->ai_personality);
    
    fprintf(file, "\n# 키 바인딩\n");
    fprintf(file, "key_up=%s\n", config_get_key_name(config->key_bindings.up));
    fprintf(file, "key_down=%s\n", config_get_key_name(config->key_bindings.down));
    fprintf(file, "key_left=%s\n", config_get_key_name(config->key_bindings.left));
    fprintf(file, "key_right=%s\n", config_get_key_name(config->key_bindings.right));
    fprintf(file, "key_pause=%s\n", config_get_key_name(config->key_bindings.pause));
    fprintf(file, "key_menu=%s\n", config_get_key_name(config->key_bindings.menu));
    
    fprintf(file, "\n# 게임 통계\n");
    fprintf(file, "total_games_played=%d\n", config->total_games_played);
    fprintf(file, "total_apples_eaten=%d\n", config->total_apples_eaten);
    fprintf(file, "total_play_time=%llu\n", (unsigned long long)config->total_play_time);
    fprintf(file, "best_score=%d\n", config->best_score);
    
    fprintf(file, "\n# 버전 정보\n");
    fprintf(file, "config_version=%d\n", config->config_version);
    
    fclose(file);
    return true;
}

/**
 * @brief 최고 점수를 로드합니다
 */
bool highscores_load(highscore_entry_t scores[MAX_HIGHSCORES], int* count) {
    if (!scores || !count) return false;
    
    FILE* file = fopen(SCORES_FILE_PATH, "rb");
    if (!file) {
        *count = 0;
        return false;
    }
    
    size_t read_count = fread(scores, sizeof(highscore_entry_t), MAX_HIGHSCORES, file);
    *count = (int)read_count;
    
    fclose(file);
    return true;
}

/**
 * @brief 최고 점수를 저장합니다
 */
bool highscores_save(const highscore_entry_t scores[MAX_HIGHSCORES], int count) {
    if (!scores || count < 0 || count > MAX_HIGHSCORES) return false;
    
    FILE* file = fopen(SCORES_FILE_PATH, "wb");
    if (!file) {
        return false;
    }
    
    size_t written = fwrite(scores, sizeof(highscore_entry_t), count, file);
    fclose(file);
    
    return written == (size_t)count;
}

/**
 * @brief 새로운 최고 점수 기록을 추가합니다
 */
bool highscores_add_entry(const highscore_entry_t* entry) {
    if (!entry) return false;
    
    highscore_entry_t scores[MAX_HIGHSCORES];
    int count = 0;
    
    // 기존 점수 로드
    highscores_load(scores, &count);
    
    // 삽입 위치 찾기
    int insert_pos = count;
    for (int i = 0; i < count; i++) {
        if (entry->score > scores[i].score) {
            insert_pos = i;
            break;
        }
    }
    
    // 기록이 상위 10위 안에 들지 않으면 추가하지 않음
    if (insert_pos >= MAX_HIGHSCORES) {
        return false;
    }
    
    // 기존 기록들을 뒤로 밀기
    for (int i = (count < MAX_HIGHSCORES ? count : MAX_HIGHSCORES - 1); i > insert_pos; i--) {
        scores[i] = scores[i - 1];
    }
    
    // 새 기록 삽입
    scores[insert_pos] = *entry;
    
    // 개수 업데이트
    if (count < MAX_HIGHSCORES) {
        count++;
    }
    
    return highscores_save(scores, count);
}

/**
 * @brief 주어진 점수가 최고 점수에 해당하는지 확인합니다
 */
bool highscores_is_high_score(int score) {
    highscore_entry_t scores[MAX_HIGHSCORES];
    int count = 0;
    
    if (!highscores_load(scores, &count)) {
        return true; // 파일이 없으면 첫 기록이므로 최고 점수
    }
    
    if (count < MAX_HIGHSCORES) {
        return true; // 아직 10개 미만이면 기록 가능
    }
    
    // 가장 낮은 점수보다 높으면 기록 가능
    return score > scores[count - 1].score;
}

/**
 * @brief 점수에 따른 등급명을 가져옵니다
 */
void highscores_get_rank_name(int score, char* rank_name, size_t size) {
    if (!rank_name || size == 0) return;
    
    const char* rank;
    if (score < 200) rank = "초보자";
    else if (score < 500) rank = "입문자";
    else if (score < 1000) rank = "아마추어";
    else if (score < 2000) rank = "숙련자";
    else if (score < 5000) rank = "전문가";
    else if (score < 10000) rank = "마스터";
    else rank = "전설";
    
    strncpy(rank_name, rank, size - 1);
    rank_name[size - 1] = '\0';
}

/**
 * @brief 키 코드에 해당하는 이름을 가져옵니다
 */
const char* config_get_key_name(game_key_t key) {
    for (size_t i = 0; i < sizeof(key_name_table) / sizeof(key_name_table[0]); i++) {
        if (key_name_table[i].key == key) {
            return key_name_table[i].name;
        }
    }
    return "UNKNOWN";
}

/**
 * @brief 키 이름에 해당하는 키 코드를 가져옵니다
 */
game_key_t config_parse_key_name(const char* name) {
    if (!name) return KEY_NONE;
    
    for (size_t i = 0; i < sizeof(key_name_table) / sizeof(key_name_table[0]); i++) {
        if (strcmp(key_name_table[i].name, name) == 0) {
            return key_name_table[i].key;
        }
    }
    return KEY_NONE;
}

/**
 * @brief AI 성향 이름을 가져옵니다
 */
const char* config_get_ai_personality_name(ai_personality_t personality) {
    if (personality >= 0 && personality < sizeof(ai_personality_names) / sizeof(ai_personality_names[0])) {
        return ai_personality_names[personality];
    }
    return "알 수 없음";
}

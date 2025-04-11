#pragma once

typedef struct tetris_difficulty {
    int gravity_interval;
    int trigger_type; // determines to interpret <trigger_at> as time/score/...
    int trigger_at; // at what time/score/anything
} tetris_difficulty;

typedef struct tetris_difficulty_manager {
    tetris_difficulty **levels;
    int level_count;
    int current_level;
} tetris_difficulty_manager;

tetris_difficulty **make_default_difficulty();
tetris_difficulty_manager *make_difficulty_manager();
void free_tetris_difficulty_manager(tetris_difficulty_manager *manager);
void update_tetris_difficulty(void *board_); // gotta do what you gotta do
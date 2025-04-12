#include "board.h"
#include "difficulty_manager.h"
#include <stdlib.h>

enum {
    TRIGGER_TIME = 1,
    TRIGGER_SCORE = 2
};

tetris_difficulty **make_default_difficulty() {
    tetris_difficulty **ret = malloc(13*sizeof(tetris_difficulty*));
    for (int i = 0; i < 13; i++) {
        ret[i] = malloc(sizeof(tetris_difficulty));
        ret[i]->gravity_interval = (10-i)*100*1000; // 1s -> 0.1s in microseconds
        ret[i]->trigger_at = (60*i)*1000*1000; // every 60s difficulty increase
        ret[i]->trigger_type = TRIGGER_TIME;
        if (i == 10) {
            ret[i]->gravity_interval = 75*1000;
        }
        if (i == 11) {
            ret[i]->gravity_interval = 50*1000;
        }
        if (i == 12) {
            ret[i]->gravity_interval = 25*1000;
        }
    }
    return ret;
}

void free_tetris_difficulty_manager(tetris_difficulty_manager *manager) {
    for (int i = 0; i < manager->level_count; i++) {
        free(manager->levels[i]);
    }
    free(manager->levels);
    free(manager);
}

void update_tetris_difficulty(void *board_) {
    tetris_board *board = (tetris_board*)board_;
    tetris_difficulty_manager *manager = board->difficulty_manager;
    int level = 0;
    for (int i = 0; i < manager->level_count; i++) {
        if (manager->levels[i]->trigger_at <= board->counters->total_time_elapsed) level = i; 
    }
    board->limits->time_since_gravity = manager->levels[level]->gravity_interval;
    manager->current_level = level;
}

tetris_difficulty_manager *make_difficulty_manager() {
    tetris_difficulty_manager *manager = malloc(sizeof(tetris_difficulty_manager));
    manager->level_count = 13;
    manager->current_level = 0;
    manager->levels = make_default_difficulty();
    return manager;
}
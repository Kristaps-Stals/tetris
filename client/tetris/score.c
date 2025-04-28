#include "score.h"
#include "board.h"
#include <stdlib.h>
#include "../shared/kstring.h"

typedef struct scoring_info {
    long long base_score;
    bool is_difficult;
    int base_garbage; // garbage sent times 4
    char* name;
} scoring_info;

enum {
    CLEAR_NORMAL = 0,
    CLEAR_TSPIN = 1,
    CLEAR_MINI_TSPIN = 2
};

scoring_info clear_types[3][5] = {
    // normal clears
    {
        {0, 0, 0, ""}, // 0 lines
        {100, 0, 0, "Single"}, // 1
        {300, 0, 1, "Double"}, // 2
        {500, 0, 2, "Triple"}, // 3
        {800, 1, 4, "Tetris"} // 4
    },
    // t-spin clears
    {
        {400, 0, 0, "T-spin"}, // 0 lines
        {800, 1, 2, "T-spin Single"}, // 1
        {1200, 1, 4, "T-spin Double"}, // 2
        {1600, 1, 6, "T-spin Triple"}, // 3
        {0, 0, 0, ""}, // -
    },
    // mini t-spin clears
    {
        {100, 0, 0, "Mini T-spin"}, // 0 lines
        {200, 1, 0, "Mini T-spin Single"}, // 1
        {400, 1, 1, "Mini T-spin Double"}, // 2
        {0, 0, 0, ""}, // -
        {0, 0, 0, ""}, // -
    }
};

score_report *make_blank_score_report() {
    score_report *score_rep = malloc(sizeof(score_report));
    score_rep->garbage = 0;
    score_rep->message = NULL;
    score_rep->score = 0;
    return score_rep;
}

void free_score_report(score_report *score_report) {
    free(score_report->message);
    free(score_report);
}

// checks for full lines in <board> and deletes them.
// returns amount of lines cleared
int check_line_clear(tetris_board *board) {
    int lines_cleared = 0;
    for (int i = 0; i < board->height; i++) {
        int bad = 0;
        for (int j = 0; j < board->width; j++) {
            if (board->state[i][j] == -1) bad = 1;
        }
        if (bad) continue;
        for (int ii = i-1; ii >= 0; ii--) {
            for (int j = 0; j < board->width; j++) {
                board->state[ii+1][j] = board->state[ii][j];
            }
        }
        lines_cleared++;
    }
    return lines_cleared;
}

// dir[0]
// X#
// ###
//

// dir[1]
//  #X
// ###
//

// dir[2]
//  #
// ###
//   X

// dir[3]
//  #
// ###
// X

const int dirx[4] = {0, 2, 2, 0};
const int diry[4] = {0, 0, 2, 2};

void get_t_spin_heuristic(tetris_board *board, int *front, int *back) {
    int vals[4];
    tetromino *t = board->active_tetromino;
    int **state = board->state;
    for (int i = 0; i < 4; i++) {
        int ny = t->y + diry[i];
        int nx = t->x + dirx[i];
        if (nx < 0 || nx >= board->width || ny < 0 || ny >= board->height || state[ny][nx] != -1) {
            vals[i] = 1;
        } else {
            vals[i] = 0;
        }
    }
    for (int i = 0; i < 2; i++) {
        *front += vals[(i+t->rotation)%4];
    }
    for (int i = 2; i < 4; i++) {
        *back += vals[(i+t->rotation)%4];
    }
}

int get_line_clear_type(tetris_board *board) {
    if (
        board->active_tetromino->type != 5 || // if not t-piece, return normal
        board->counters->last_rotation == -1 // last move wasnt a rotation
    ) return CLEAR_NORMAL;
    
    int type = CLEAR_NORMAL;

    int front_blocks = 0;
    int back_blocks = 0;
    get_t_spin_heuristic(board, &front_blocks, &back_blocks);
    int total_blocks = front_blocks + back_blocks;

    // check for mini t-spin
    if (total_blocks >= 3) type = CLEAR_MINI_TSPIN; // atleast, could still be proper tspin

    // check for the more restrictive proper t-spin
    if (
        (front_blocks == 2 && back_blocks >= 1) ||
        (total_blocks >= 3 && board->counters->last_rotation == 4) // last rotation was done with the last SRS test
    ) type = CLEAR_TSPIN;

    return type;
}

bool check_all_clear(tetris_board *board) {
    for (int i = 0; i < board->height; i++) {
        for (int j = 0; j < board->width; j++) {
            if (board->state[i][j] != -1) return false;
        }
    }
    return true;
}

int b2b_bonus_to_b2b_level(int x) {
    if (x <= 0) return 0;
    if (x <= 2) return 1; // 1x-2x = level 1
    if (x <= 7) return 2; // 3x-7x = level 2
    if (x <= 23) return 3; // 8x-23x = level 3
    if (x <= 66) return 4; // 24x-66x = level 4
    return 5; // 67x+ = level 5
}

int hardcoded_0_garbage_combo_scaling(int combo) {
    if (combo <= 1) return 0;
    if (combo <= 5) return 1;
    if (combo <= 15) return 2;
    if (combo <= 20) return 3;
    // else as if base is 0.5
    int combo_mult_4x = 4+combo;
    int base_2x = 1;
    return (base_2x*combo_mult_4x)/8;
}

char buf[100];
score_report *update_clear_lines(void* board_) {
    tetris_board *board = board_;
    score_report *score_rep = make_blank_score_report();

    int clear_type = get_line_clear_type(board); // do before actually clearing

    int lines_cleared = check_line_clear(board); // actually clear
    int current_level = board->difficulty_manager->current_level+1; // 0-indexed    

    scoring_info s_info = clear_types[clear_type][lines_cleared];

    // combo
    if (lines_cleared == 0) board->counters->combo = -1;
    else board->counters->combo++;

    // b2b bonus
    if (lines_cleared > 0 && s_info.is_difficult == 0) board->counters->b2b_bonus = -1;
    if (s_info.is_difficult == 1) board->counters->b2b_bonus++;
    int b2b_level = b2b_bonus_to_b2b_level(board->counters->b2b_bonus);

    bool all_clear = check_all_clear(board);

    score_rep->score = s_info.base_score * current_level;
    if (b2b_level >= 1) { // 1.5x score for having any b2b bonus
        score_rep->score *= 3;
        score_rep->score /= 2;
    }

    // + 0.25 to combo mult per combo, 0 = 1x, 1 = 1.25x, 2 = 1.5x, 3 = 1.75x... saved as 4x the amount to be a whole number
    int combo_mult_4x = 4 + board->counters->combo;
    int base_garbage = s_info.base_garbage + b2b_level;
    score_rep->garbage = (base_garbage * combo_mult_4x) / 4;
    if (base_garbage == 0 && lines_cleared != 0) {
        score_rep->garbage = hardcoded_0_garbage_combo_scaling(board->counters->combo);
    }
    if (lines_cleared == 0) score_rep->garbage = 0; // if no lines cleared, no garbage is sent

    char b2b_text[20];
    b2b_text[0] = 0;
    if (board->counters->b2b_bonus >= 1 && lines_cleared > 0) {
        sprintf(b2b_text, " %dx B2B", board->counters->b2b_bonus);
    }

    char combo_text[20];
    combo_text[0] = 0;
    if (board->counters->combo >= 1 && lines_cleared > 0) {
        sprintf(combo_text, " %dx Combo", board->counters->combo);
    }

    char allclear_text[20];
    allclear_text[0] = 0;
    if (all_clear) {
        sprintf(allclear_text, " ALL CLEAR!");
        score_rep->garbage += 10; // full clear bonus garbage
    }

    sprintf(buf, "%s%s%s%s", s_info.name, b2b_text, combo_text, allclear_text);
    score_rep->message = copy_text(buf);

    return score_rep;
}
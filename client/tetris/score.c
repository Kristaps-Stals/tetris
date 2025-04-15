#include "score.h"
#include "board.h"
#include <stdlib.h>
#include "../shared/kstring.h"

typedef struct scoring_info {
    long long base_score;
    bool is_difficult;
    char* name;
} scoring_info;

scoring_info clear_types[3][5] = {
    // normal clears
    {
        {0, 0, ""}, // 0 lines
        {100, 0, "Single"}, // 1
        {300, 0, "Double"}, // 2
        {500, 0, "Triple"}, // 3
        {800, 1, "Tetris"} // 4
    },
    // t-spin clears
    {
        {400, 0, "T-spin"}, // 0 lines
        {800, 1, "T-spin Single"}, // 1
        {1200, 1, "T-spin Double"}, // 2
        {1600, 1, "T-spin Triple"}, // 3
        {0, 0, ""}, // -
    },
    // mini t-spin clears
    {
        {100, 0, "Mini T-spin"}, // 0 lines
        {200, 1, "Mini T-spin Single"}, // 1
        {400, 1, "Mini T-spin Double"}, // 2
        {0, 0, ""}, // -
        {0, 0, ""}, // -
    }
};

score_report *make_blank_score_report() {
    score_report *score_rep = malloc(sizeof(score_report));
    score_rep->garbage = 0;
    score_rep->message = "";
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

score_report *update_clear_lines(void* board_) {
    tetris_board *board = board_;
    score_report *score_rep = make_blank_score_report();

    int lines_cleared = check_line_clear(board);
    int current_level = board->difficulty_manager->current_level+1; // 0-indexed    

    score_rep->score = clear_types[0][lines_cleared].base_score * current_level;

    board->counters->score += score_rep->score;

    return score_rep;
}
#include "SRS_rotation.h"
#include "board.h"
#include <stdlib.h>

// right rotation tests for normal blocks,
// left rotations are the same but both coordinates are negated
int normal_rot_right[4][5][2] = {
    // 0>>1
    {
        {0, 0},
        {0, -1},
        {-1, -1},
        {2, 0},
        {2, -1}
    },
    // 1>>2
    {
        {0, 0},
        {0, 1},
        {1, 1},
        {-2, 0},
        {-2, 1}
    },
    // 2>>3
    {
        {0, 0},
        {0, 1},
        {-1, 1},
        {2, 0},
        {2, 1}
    },
    // 3>>0
    {
        {0, 0},
        {0, -1},
        {1, -1},
        {-2, 0},
        {-2, -1}
    }
};

int I_rot_right[4][5][2] = {
    // 0>>1
    {
        {0, 0},
        {0, -2},
        {0, 1},
        {1, -2},
        {-2, 1}
    },
    // 1>>2
    {
        {0, 0},
        {0, -1},
        {0, 2},
        {-2, -1},
        {1, 2}
    },
    // 2>>3
    {
        {0, 0},
        {0, 2},
        {0, -1},
        {-1, 2},
        {2, -1}
    },
    // 3>>0
    {
        {0, 0},
        {0, 1},
        {0, -2},
        {2, 1},
        {-1, -2}
    }
};

// right = 1, left = 3
bool rotate_tetromino(tetris_board *board, int dir) {

    tetromino *test = deepcpy_tetromino(board->active_tetromino);

    int rot_idx;
    if (dir == 1) {
        rot_idx = test->rotation;
        test->rotation++;
        test->rotation %= 4;
    }

    if (dir == 3) {
        test->rotation += 3; // -1 +4
        test->rotation %= 4;
        rot_idx = test->rotation;
    }

    for (int i = 0; i < 5; i++) {
        int y_dif = normal_rot_right[rot_idx][i][0];
        int x_dif = normal_rot_right[rot_idx][i][1];
        if (test->type == 0) { // if I piece
            y_dif = I_rot_right[rot_idx][i][0];
            x_dif = I_rot_right[rot_idx][i][1];
        }
        if (dir == 3) { // flip to the left rotation values
            y_dif *= -1;
            x_dif *= -1;
        }
        test->y += y_dif;
        test->x += x_dif;
        if (valid_pos(test, board)) {
            free(board->active_tetromino);
            board->active_tetromino = test;
            return true;
        }
        test->y -= y_dif;
        test->x -= x_dif;
    }

    free(test);
    return false;
}
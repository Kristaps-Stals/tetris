#include "tetromino_shapes.h"

// {y_offset, x_offset}
const int shapes[7][4][4][2] = 
{
    // I piece
    {
        // rot 0
        {
            {1, 0},
            {1, 1},
            {1, 2},
            {1, 3}
        },
        // rot 1
        {
            {0, 2},
            {1, 2},
            {2, 2},
            {3, 2}
        },
        // rot 2
        {
            {2, 0},
            {2, 1},
            {2, 2},
            {2, 3}
        },
        // rot 3
        {
            {0, 1},
            {1, 1},
            {2, 1},
            {3, 1}
        }
    },
    // J piece
    {
        // rot 0
        {
            {0, 0},
            {1, 0},
            {1, 1},
            {1, 2}
        },
        // rot 1
        {
            {0, 1},
            {0, 2},
            {1, 1},
            {2, 1}
        },
        // rot 2
        {
            {1, 0},
            {1, 1},
            {1, 2},
            {2, 2}
        },
        // rot 3
        {
            {0, 1},
            {1, 1},
            {2, 0},
            {2, 1}
        }
    },
    // L piece
    {
        // rot 0
        {
            {0, 2},
            {1, 0},
            {1, 1},
            {1, 2}
        },
        // rot 1
        {
            {0, 1},
            {1, 1},
            {2, 1},
            {2, 2}
        },
        // rot 2
        {
            {1, 0},
            {1, 1},
            {1, 2},
            {2, 0}
        },
        // rot 3
        {
            {0, 0},
            {0, 1},
            {1, 1},
            {2, 1}
        }
    },
    // O piece
    {
        // rot 0
        {
            {0, 0},
            {0, 1},
            {1, 0},
            {1, 1}
        },
        // rot 1
        {
            {0, 0},
            {0, 1},
            {1, 0},
            {1, 1}
        },
        // rot 2
        {
            {0, 0},
            {0, 1},
            {1, 0},
            {1, 1}
        },
        // rot 3
        {
            {0, 0},
            {0, 1},
            {1, 0},
            {1, 1}
        }
    },
    // S piece
    {
        // rot 0
        {
            {0, 1},
            {0, 2},
            {1, 0},
            {1, 1}
        },
        // rot 1
        {
            {0, 1},
            {1, 1},
            {1, 2},
            {2, 2}
        },
        // rot 2
        {
            {1, 1},
            {1, 2},
            {2, 0},
            {2, 1}
        },
        // rot 3
        {
            {0, 0},
            {1, 0},
            {1, 1},
            {2, 1}
        }
    },
    // T piece
    {
        // rot 0
        {
            {0, 1},
            {1, 0},
            {1, 1},
            {1, 2}
        },
        // rot 1
        {
            {0, 1},
            {1, 1},
            {1, 2},
            {2, 1}
        },
        // rot 2
        {
            {1, 0},
            {1, 1},
            {1, 2},
            {2, 1}
        },
        // rot 3
        {
            {0, 1},
            {1, 0},
            {1, 1},
            {2, 1}
        }
    },
    // Z piece
    {
        // rot 0
        {
            {0, 0},
            {0, 1},
            {1, 1},
            {1, 2}
        },
        // rot 1
        {
            {0, 2},
            {1, 1},
            {1, 2},
            {2, 1}
        },
        // rot 2
        {
            {1, 0},
            {1, 1},
            {2, 1},
            {2, 2}
        },
        // rot 3
        {
            {0, 1},
            {1, 0},
            {1, 1},
            {2, 0}
        }
    }
};

int shape_spawn_width[7] = {4, 3, 3, 2, 3, 3, 3};

// get tetromino pieces of <type> in <rotation>,
// i-th blocks j-th coordinate j = 0 means y coordinate, j = 1 means x coordinate.
// a tetromino consists of 4 blocks.
int get_shapes(int type, int rotation, int i, int j) {
    return shapes[type][rotation][i][j];
}

// get tetromino pieces width of type <type> in blocks.
int get_shape_spawn_width(int type) {
    return shape_spawn_width[type];
}
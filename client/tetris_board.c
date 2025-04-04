#include <ncurses.h>
#include <stdlib.h>
#include "tetris_board.h"
#include "tetromino_shapes.h"
typedef long long ll;

const int DIR_UP = 0;
const int DIR_RIGHT = 1;
const int DIR_DOWN = 2;
const int DIR_LEFT = 3;

// normal directions {y, x}
const int normal_dir[4][2] = {
    {-1, 0}, // up
    {0, 1}, // right
    {1, 0}, // down
    {0, -1} // left
};

tetromino *deepcpy_tetromino(tetromino *a) {
    tetromino *b = malloc(sizeof(tetromino));
    b->rotation = a->rotation;
    b->type = a->type;
    b->x = a->x;
    b->y = a->y;
    return b;
}

int **get_tetromino_positions(tetromino *t) {
    int **pos = malloc(4*sizeof(int*));
    for (int i = 0; i < 4; i++) {
        pos[i] = malloc(2*sizeof(int));
        pos[i][0] = t->y+get_shapes(t->type, t->rotation, i, 0);
        pos[i][1] = t->x+get_shapes(t->type, t->rotation, i, 1);
    }
    return pos;
}
void free_pos(int **pos) {
    for (int i = 0; i < 4; i++) free(pos[i]);
    free(pos);
}

tetris_bag *contstruct_bag(int *seed) {
    srand(*seed);
    tetris_bag *bag = malloc(sizeof(tetris_bag));
    int avail[7] = {0, 1, 2, 3, 4, 5, 6};
    for (int i = 6; i >= 0; i--) {
        int am = i+1;
        int idx = rand()%am;
        int at = 0;
        while (1) {
            while (avail[at] == -1) at++;
            if (idx <= 0) break;
            idx--;
            at++;
        }
        bag->stack[i] = avail[at];
        avail[at] = -1;
    }
    bag->top = 6;
    *seed = rand();
    return bag;
}

tetris_bag_manager *construct_bag_manager(tetris_board* board, int seed) {
    tetris_bag_manager *manager = malloc(sizeof(tetris_bag_manager));
    manager->bag_seed = seed;
    manager->now = contstruct_bag(&manager->bag_seed);
    manager->next = contstruct_bag(&manager->bag_seed);
    manager->held_tetromino = -1;

    // make hold window
    manager->hold_w = 12;
    manager->hold_h = 6;
    manager->hold_x = board->win_x-manager->hold_w;
    manager->hold_y = board->win_y;
    manager->hold = newwin(manager->hold_h, manager->hold_w, manager->hold_y, manager->hold_x);

    // make upcoming window
    manager->upcoming_w = 12;
    manager->upcoming_h = 18;
    manager->upcoming_x = board->win_x+board->win_w;
    manager->upcoming_y = board->win_y;
    manager->upcoming = newwin(manager->upcoming_h, manager->upcoming_w, manager->upcoming_y, manager->upcoming_x);

    return manager;
}

tetromino *construct_tetromino(tetromino_construct_info *info) {
    tetromino *ret = malloc(sizeof(tetromino));
    ret->type = info->id;
    int play_width = info->board->width;
    int piece_width = get_shape_spawn_width(ret->type);
    int center_2x = play_width; // center * 2
    ret->x = (center_2x-piece_width+1)/2;
    ret->y = 0;
    ret->rotation = 0;
    return ret;
}

tetromino *take_from_bag(tetris_board *board, tetris_bag_manager *manager) {
    if (manager->now->top < 0) {
        // now is empty
        free(manager->now);
        manager->now = manager->next;
        manager->next = contstruct_bag(&manager->bag_seed);
    }
    tetromino_construct_info *tinfo = malloc(sizeof(tetromino_construct_info));
    tinfo->board = board;
    tinfo->id = manager->now->stack[manager->now->top];
    manager->now->top--;
    tetromino *ret = construct_tetromino(tinfo);
    free(tinfo);
    return ret;
}

tetris_board *construct_tetris_board(const tetris_board_settings *settings) {
    tetris_board *board = malloc(sizeof(tetris_board));
    int h = settings->play_height;
    int w = settings->play_width;

    board->height = h;
    board->width = w;
    
    board->win_h = h+2;
    board->win_w = 2*w+2;
    board->win_y = (LINES-board->win_h)/2;
    board->win_x = (COLS-board->win_w)/2;
    board->win = newwin(board->win_h, board->win_w, board->win_y, board->win_x);

    board->counters = malloc(sizeof(board_counters));
    board->counters->time_since_gravity = 0;
    board->counters->hold_count = 0;

    board->limits = malloc(sizeof(board_counters));
    board->limits->time_since_gravity = 1000*250;
    board->limits->hold_count = 1;

    board->state = (int**)malloc(h*sizeof(int*));
    for (int i = 0; i < h; i++) {
        board->state[i] = malloc(w*sizeof(int));
        for (int j = 0; j < w; j++) {
            board->state[i][j] = -1;
        }
    }
    
    board->bag_manager = construct_bag_manager(board, 0); // seed set to 0 currently, change later

    board->active_tetromino = take_from_bag(board, board->bag_manager);

    return board;
}

void check_line_clear(tetris_board *board) {
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
    }
}

bool can_move(tetris_board *board, tetromino *t, int dir) {
    int **pos = get_tetromino_positions(t);
    for (int i = 0; i < 4; i++) {
        int y = pos[i][0] + normal_dir[dir][0];
        int x = pos[i][1] + normal_dir[dir][1];
        if (x < 0 || x >= board->width || y >= board->height || board->state[y][x] != -1) {
            free_pos(pos);
            return false;
        }
    }
    free_pos(pos);
    return true;
}

bool move_tetromino(tetris_board *board, tetromino *t, int dir) {
    if (!can_move(board, t, dir)) return false;
    t->y += normal_dir[dir][0];
    t->x += normal_dir[dir][1];
    return true;
}

void hard_drop(tetris_board *board) {
    while (can_move(board, board->active_tetromino, DIR_DOWN)) move_tetromino(board, board->active_tetromino, DIR_DOWN);
    int **pos = get_tetromino_positions(board->active_tetromino);
    for (int i = 0; i < 4; i++) {
        int y = pos[i][0];
        int x = pos[i][1];
        board->state[y][x] = board->active_tetromino->type;
    }
    free_pos(pos);
    free(board->active_tetromino);
    board->active_tetromino = take_from_bag(board, board->bag_manager);

    board->counters->hold_count = 0;

    check_line_clear(board);
}

bool valid_pos(tetromino *test, tetris_board *board) {
    int **pos = get_tetromino_positions(test);
    for (int i = 0; i < 4; i++) {
        int y = pos[i][0];
        int x = pos[i][1];
        if (x < 0 || x >= board->width || y >= board->height || board->state[y][x] != -1) {
            free_pos(pos);
            return false;
        }
    }
    free_pos(pos);
    return true;
}

// right = 1, left = 3
void rotate_tetromino(tetris_board *board, int dir) {

    tetromino *test = deepcpy_tetromino(board->active_tetromino);

    if (dir == 1) {
        test->rotation++;
        test->rotation %= 4;
        if (valid_pos(test, board)) {
            free(board->active_tetromino);
            board->active_tetromino = test;
            return;
        }
    }

    if (dir == 3) {
        test->rotation += 3; // -1 +4
        test->rotation %= 4;
        if (valid_pos(test, board)) {
            free(board->active_tetromino);
            board->active_tetromino = test;
            return;
        }
    }
    free(test);
}

void apply_gravity(tetris_board *board) {
    if (move_tetromino(board, board->active_tetromino, DIR_DOWN)) return;
    hard_drop(board);
}

// swaps active tetromino with hold tetromino (if exists)
// returns true if swapped
// returns false if nothing is done
bool hold_tetromino(tetris_board *board) {
    if (board->counters->hold_count >= board->limits->hold_count) return false;
    board->counters->hold_count++;

    int our_tetromino = board->active_tetromino->type; // will be in hold after

    if (board->bag_manager->held_tetromino == -1) {
        // no piece in hold, take from bag
        board->active_tetromino = take_from_bag(board, board->bag_manager);
    } else {
        // piece already in hold, take from hold
        tetromino_construct_info *tinfo = malloc(sizeof(tetromino_construct_info));
        tinfo->board = board;
        tinfo->id = board->bag_manager->held_tetromino;
        board->active_tetromino = construct_tetromino(tinfo);
        free(tinfo);
    }

    board->bag_manager->held_tetromino = our_tetromino;
    return true;
}

void update_board(tetris_board_update *update) {
    int user_input = update->user_input;
    ll delta_time = update->delta_time;
    tetris_board *board = update->board;
    board_counters *counters = board->counters;
    board_counters *limits = board->limits;

    // user input
    switch(user_input) {
        case 'x':
            rotate_tetromino(board, DIR_RIGHT);
            break;
        case 'z':
            rotate_tetromino(board, DIR_LEFT);
            break;
        case 'c':
            hold_tetromino(board);
            break;
        case KEY_RIGHT:
            move_tetromino(board, board->active_tetromino, DIR_RIGHT);
            break;
        case KEY_LEFT:
            move_tetromino(board, board->active_tetromino, DIR_LEFT);
            break;
        case KEY_DOWN:
            if (move_tetromino(board, board->active_tetromino, DIR_DOWN)) counters->time_since_gravity = 0;
            break;
        case ' ':
            hard_drop(board);
            break;
    }

    // check if falling
    counters->time_since_gravity += delta_time;
    while (counters->time_since_gravity > limits->time_since_gravity) {
        counters->time_since_gravity -= limits->time_since_gravity;
        apply_gravity(board);
    }
}

// returns array with x tetromino types 
int *get_next_x_in_bag(tetris_bag_manager *bag, int x) {
    bool now_is_empty = false;
    int *ans = malloc(x*sizeof(int));
    int top_idx = bag->now->top;
    for (int i = 0; i < x; i++) {
        if (top_idx < 0) {
            now_is_empty = true;
            top_idx = 6;
        }
        if (now_is_empty) {
            ans[i] = bag->next->stack[top_idx];
        } else {
            ans[i] = bag->now->stack[top_idx];
        }
        top_idx--;
    }
    return ans;
}

void draw_upcoming(tetris_board *board) {
    tetris_bag_manager *bag = board->bag_manager;

    // clear window
    for (int i = 0; i < bag->upcoming_h; i++) {
        for (int j = 0; j < bag->upcoming_w; j++) {
            mvwaddch(bag->upcoming, i, j, ' ');
        }
    }

    // draw window title
    int mid = (bag->hold_w-4)/2;
    mvwprintw(bag->upcoming, 1, mid, "NEXT");

    // draw upcoming pieces
    int *upcoming_types = get_next_x_in_bag(bag, 5);
    tetromino t;
    t.rotation = 0;
    t.x = 0;
    t.y = 0;
    for (int i = 0; i < 5; i++) {
        t.type = upcoming_types[i];
        int base_y = 3+3*i;
        int w = get_shape_spawn_width(t.type);
        int base_x = (bag->upcoming_w-w*2)/2;
        int **pos = get_tetromino_positions(&t);
        for (int j = 0; j < 4; j++) {
            mvwaddch(bag->upcoming, base_y+pos[j][0], base_x+pos[j][1]*2, ' ' | COLOR_PAIR(t.type+1));
            mvwaddch(bag->upcoming, base_y+pos[j][0], base_x+pos[j][1]*2+1, ' ' | COLOR_PAIR(t.type+1));
        }
    }
    free(upcoming_types);

    wrefresh(bag->upcoming);
}

void draw_hold(tetris_board *board) {
    tetris_bag_manager *bag = board->bag_manager;
    for (int i = 0; i < bag->hold_h; i++) {
        for (int j = 0; j < bag->hold_w; j++) {
            mvwaddch(bag->hold, i, j, ' ');
        }
    }
    int mid = (bag->hold_w-4)/2;
    mvwprintw(bag->hold, 1, mid, "HOLD");
    int tetromino_id = board->bag_manager->held_tetromino;
    if (tetromino_id != -1) {
        int w = get_shape_spawn_width(tetromino_id);
        int base_y = 3;
        if (tetromino_id == 0) base_y--; // the I piece has one blank space on top, so we move it up
        int base_x = (bag->hold_w-w*2)/2;
        tetromino t;
        t.rotation = 0;
        t.type = tetromino_id;
        t.x = 0;
        t.y = 0;
        int **pos = get_tetromino_positions(&t);
        for (int i = 0; i < 4; i++) {
            mvwaddch(bag->hold, base_y+pos[i][0], base_x+pos[i][1]*2, ' ' | COLOR_PAIR(tetromino_id+1));
            mvwaddch(bag->hold, base_y+pos[i][0], base_x+pos[i][1]*2+1, ' ' | COLOR_PAIR(tetromino_id+1));
        }
        free_pos(pos);
    }
    wrefresh(bag->hold);
}

void draw_tetris_board(tetris_board *board) {
    // erase();
    // mvprintw(0, 0, "%d", board->active_tetromino->y);
    // werase(board->win);
    
    int h = board->height;
    int w = board->width;
    int win_h = board->win_h;
    int win_w = board->win_w;
    for (int i = 0; i < win_h; i++) {
        for (int j = 0; j < win_w; j++) {
            mvwaddch(board->win, i, j, '.' | A_DIM);
        }
    }
    wborder(board->win, 0, 0, ' ', 0, ' ', ' ', 0, 0);

    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            if (board->state[i][j] == -1) continue;
            if (board->state[i][j] == 3) wattron(board->win, A_BOLD);
            mvwaddch(board->win, i+1, 2*j+1, ' ' | COLOR_PAIR(board->state[i][j]+1));
            mvwaddch(board->win, i+1, 2*j+2, ' ' | COLOR_PAIR(board->state[i][j]+1));
            if (board->state[i][j] == 3) wattroff(board->win, A_BOLD);
        }
    }

    if (board->active_tetromino != NULL) {
        // draw prediction
        tetromino *prediction = deepcpy_tetromino(board->active_tetromino);
        while (move_tetromino(board, prediction, DIR_DOWN));
        int **pos = get_tetromino_positions(prediction);
        for (int i = 0; i < 4; i++) {
            mvwaddch(board->win, pos[i][0]+1, 2*pos[i][1]+1, '@');
            mvwaddch(board->win, pos[i][0]+1, 2*pos[i][1]+2, '@');
        }
        free_pos(pos);
        free(prediction);
        
        // draw active tetromino
        pos = get_tetromino_positions(board->active_tetromino);
        for (int i = 0; i < 4; i++) {
            mvwaddch(board->win, pos[i][0]+1, 2*pos[i][1]+1, ' ' | COLOR_PAIR(board->active_tetromino->type+1));
            mvwaddch(board->win, pos[i][0]+1, 2*pos[i][1]+2, ' ' | COLOR_PAIR(board->active_tetromino->type+1));
        }
        free_pos(pos);
    }
    wrefresh(board->win);

    draw_hold(board);
    draw_upcoming(board);
}

void deconstruct_tetris_board(tetris_board *board) {
    delwin(board->win);
    for(int i = 0; i < board->height; i++) {
        free(board->state[i]);
    }
    free(board->state);
    free(board->active_tetromino);
    free(board->counters);
    free(board->limits);
    tetris_bag_manager *bag = board->bag_manager;
    delwin(bag->upcoming);
    delwin(bag->hold);
    free(bag->now);
    free(bag->next);
    free(board);
}
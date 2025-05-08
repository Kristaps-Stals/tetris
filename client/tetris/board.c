#include <ncurses.h>
#include <stdlib.h>
#include "board.h"
#include "tetromino_shapes.h"
#include "SRS_rotation.h"
#include <time.h>
#include <string.h>
#include "score.h"
#include "../menus/keyboard_manager.h"
#include "../net/net.h"
#include "../../shared/kstring.h"
typedef long long ll;

const int DIR_UP = 0;
const int DIR_RIGHT = 1;
const int DIR_DOWN = 2;
const int DIR_LEFT = 3;

// normal directions {y, x}
const int normal_dir[4][2] = {
    {1, 0}, // up
    {0, 1}, // right
    {-1, 0}, // down
    {0, -1} // left
};

void delete_window(WINDOW *win) {
    werase(win);
    wrefresh(win);
    delwin(win);
}

tetromino *deepcpy_tetromino(tetromino *a) {
    tetromino *b = malloc(sizeof(tetromino));
    b->rotation = a->rotation;
    b->type = a->type;
    b->x = a->x;
    b->y = a->y;
    return b;
}

// returns highest placed piece
int calculate_highest_piece(tetris_board *board){
    for (int i = board->height-1; i >= 0; i--) {
        for (int j = 0; j < board->width; j++) {
            if (board->state[i][j] != -1) return i;
        }
    }
    return 0; // returns the floor as the highest piece
}

// returns array of block positions for tetromino <t>
int **get_tetromino_positions(tetromino *t) {
    int **pos = malloc(4*sizeof(int*));
    for (int i = 0; i < 4; i++) {
        pos[i] = malloc(2*sizeof(int));
        pos[i][0] = t->y-get_shapes(t->type, t->rotation, i, 0);
        pos[i][1] = t->x+get_shapes(t->type, t->rotation, i, 1);
    }
    return pos;
}
void free_pos(int **pos) { // frees positions given by get_tetromino_positions()
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

// <board> is used for knowing where to place the hold and upcoming windows.
// <seed> is used to seed the bag manager.
tetris_bag_manager *construct_bag_manager(tetris_board* board, int seed) {
    tetris_bag_manager *manager = malloc(sizeof(tetris_bag_manager));
    manager->bag_seed = seed;
    manager->now = contstruct_bag(&manager->bag_seed);
    manager->next = contstruct_bag(&manager->bag_seed);
    manager->held_tetromino = -1;

    // make hold window
    manager->hold_w = 8;
    manager->hold_h = 6;
    manager->hold_x = board->win_x-manager->hold_w-2;
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
    ret->x = (center_2x-piece_width)/2;
    ret->y = 21;
    ret->rotation = 0;
    return ret;
}

// takes next piece from manager and sets parameters as if its going
// to be spawned on board. 
// if only_look is true, the piece will not be actually removed.
tetromino *take_from_bag(tetris_board *board, tetris_bag_manager *manager, bool only_look) {
    if (manager->now->top < 0) {
        // now is empty, get next bag
        free(manager->now);
        manager->now = manager->next;
        manager->next = contstruct_bag(&manager->bag_seed);
    }
    tetromino_construct_info *tinfo = malloc(sizeof(tetromino_construct_info));
    tinfo->board = board;
    tinfo->id = manager->now->stack[manager->now->top];
    if (only_look == false) manager->now->top--;
    tetromino *ret = construct_tetromino(tinfo);
    free(tinfo);
    return ret;
}

board_counters *make_default_counters() {
    board_counters *counters = malloc(sizeof(board_counters));
    counters->time_since_gravity = 0;
    counters->gravity_count = 0;
    counters->hold_count = 0;
    counters->total_time_elapsed = 0;
    counters->score = 0;
    counters->lock_delay = 0;
    counters->lock_times = 0;
    counters->b2b_bonus = -1;
    counters->combo = -1;
    counters->last_rotation = -1;
    return counters;
}

board_counters *make_default_limits() {
    board_counters *limits = malloc(sizeof(board_counters));
    // limits->time_since_gravity = 1000*250; // controlled by difficulty manager
    limits->gravity_count = 60; // max times gravity can be applied before forced hard drop
    limits->hold_count = 1; // max times you can press hold without hard dropping
    limits->total_time_elapsed = 0; // does nothing
    limits->score = 0; // does nothing
    limits->lock_delay = 500*1000; // 0.5s lock delay, time without doing anything on the ground before locking
    limits->lock_times = 12; // max times can move before disabling resetting of lock delay
    limits->b2b_bonus = 0; // does nothing
    limits->combo = 0; // does nothing
    limits->last_rotation = 0; // does nothing
    return limits;
}

tetris_info_manager *make_default_info_manager(tetris_board *board) {
    tetris_info_manager *info_manager = malloc(sizeof(tetris_info_manager));
    info_manager->info_h = 4;
    info_manager->info_w = board->win_w;
    info_manager->info_x = board->win_x;
    info_manager->info_y = board->win_y - info_manager->info_h; 
    info_manager->info_win = newwin(
        info_manager->info_h,
        info_manager->info_w,
        info_manager->info_y,
        info_manager->info_x
    );

    info_manager->clear_h = 3;
    info_manager->clear_w = board->win_w+18;
    info_manager->clear_x = board->win_x-9;
    info_manager->clear_y = board->win_y + board->win_h + 1;
    info_manager->clear_win = newwin(
        info_manager->clear_h,
        info_manager->clear_w,
        info_manager->clear_y,
        info_manager->clear_x
    );

    return info_manager;
}

void free_info_manager(tetris_info_manager *info_manager) {
    wclear(info_manager->info_win);
    wrefresh(info_manager->info_win);
    delwin(info_manager->info_win);
    
    wclear(info_manager->clear_win);
    wrefresh(info_manager->clear_win);
    delwin(info_manager->clear_win);
    
    free(info_manager);
}

tetris_garbage_manager *construct_tetris_garbage_manager(tetris_board *board) {
    tetris_garbage_manager *manager = malloc(sizeof(tetris_garbage_manager));

    manager->h = board->win_y+board->win_h-1; // extends to the top of the screen, may be a problem if something is above board
    manager->w = 1;
    manager->y = 0;
    manager->x = board->win_x-1;

    manager->win = newwin(
        manager->h,
        manager->w,
        manager->y,
        manager->x
    );

    manager->armed_garbage = 0;
    manager->max_garbage_in_queue = 20;
    manager->time_to_arm = 500*1e3; // 500 ms to arm
    manager->queue_amount = malloc(manager->max_garbage_in_queue*sizeof(int));
    manager->queue_timer = malloc(manager->max_garbage_in_queue*sizeof(int));
    for (int i = 0; i < manager->max_garbage_in_queue; i++) {
        manager->queue_amount[i] = 0;
        manager->queue_timer[i] = 0;
    }
    return manager;
}
void free_garbage_manager(tetris_garbage_manager *manager){
    delete_window(manager->win);
    free(manager->queue_amount);
    free(manager->queue_timer);
    free(manager);
}
// called upon dropping a piece and not clearing a line
int trigger_garbage(tetris_board *board) {
    tetris_garbage_manager *manager = board->garbage_manager;
    int amount_to_add = 8;
    if (manager->armed_garbage < amount_to_add) amount_to_add = manager->armed_garbage;
    if (amount_to_add == 0) return 0;

    manager->armed_garbage -= amount_to_add;
    for (int i = board->height-1; i >= 0; i--) {
        for (int j = 0; j < board->width; j++) {
            int target_y = i+amount_to_add;
            if (board->state[i][j] != -1) {
                if (target_y >= board->height) return 1; // garbage out of bounds, lose state    
                board->state[target_y][j] = board->state[i][j];
            } else {
                if (target_y < board->height) {
                    board->state[target_y][j] = board->state[i][j];
                }
            }
        }
    }

    int gap_x = rand()%board->width;
    int at_y = 0;
    while (amount_to_add > 0 && at_y < board->height) {
        for (int i = 0; i < board->width; i++) {
            if (i == gap_x) board->state[at_y][i] = -1;
            else board->state[at_y][i] = 7;
        }
        at_y++;
        amount_to_add--;
    }
    return 0;
}
// called every frame
void update_garbage(tetris_garbage_manager *manager, int delta_time) {
    for (int i = 0; i < manager->max_garbage_in_queue; i++) {
        if (manager->queue_amount[i] == 0) continue;
        manager->queue_timer[i] -= delta_time;
        if (manager->queue_timer[i] <= 0) {
            manager->armed_garbage += manager->queue_amount[i];
            manager->queue_amount[i] = 0;
        }
    }
}

// called when need to add new garbage
void add_garbage(tetris_board *board, int amount) {
    tetris_garbage_manager *manager = board->garbage_manager;
    int small_id = 0; // id of smallest time left
    for (int i = 0; i < manager->max_garbage_in_queue; i++) {
        if (manager->queue_amount[i] == 0) {
            manager->queue_amount[i] = amount;
            manager->queue_timer[i] = manager->time_to_arm;
            return;
        }
        if (manager->queue_timer[i] < manager->queue_timer[small_id]) small_id = i;
    }
    // if no empty spots in queue, force smallest time out
    manager->armed_garbage += manager->queue_amount[small_id];
    manager->queue_amount[small_id] = amount;
    manager->queue_timer[small_id] = manager->time_to_arm;
}
void send_garbage(tetris_board *board, int garbage_amount) {
    tetris_garbage_manager *manager = board->garbage_manager;

    // 1. remove from armed garbage
    int remove_armed_garbage_amount = garbage_amount;
    if (manager->armed_garbage < garbage_amount) remove_armed_garbage_amount = manager->armed_garbage;
    manager->armed_garbage -= remove_armed_garbage_amount;
    garbage_amount -= remove_armed_garbage_amount;

    // 2. remove from garbage in queue
    while (garbage_amount > 0) {
        int low_id = 0;
        bool have_queue = false;
        for (int i = 0; i < manager->max_garbage_in_queue; i++) {
            if (manager->queue_amount[i] != 0) {
                have_queue = true;
                if (
                    manager->queue_amount[low_id] == 0 ||
                    (manager->queue_timer[i] < manager->queue_timer[low_id] &&
                    manager->queue_amount[i] != 0)
                ) {
                    low_id = i;
                }
            }
        }
        if (have_queue == false) break; // no more in queue
        int rem_amount = garbage_amount;
        if (manager->queue_amount[low_id] < garbage_amount) rem_amount = manager->queue_amount[low_id];
        manager->queue_amount[low_id] -= rem_amount;
        garbage_amount -= rem_amount;
    }

    if (garbage_amount == 0) return;

    // 3. send remainng to opponent
    send_message(board->sockfd, MSG_SEND_GARBAGE, garbage_amount, NULL, 0);

    refresh();
}
void draw_garbage(tetris_garbage_manager *manager) {
    
    for (int i = 0; i < manager->h; i++) {
        mvwaddch(manager->win, i, 0, ' ');
    }

    int armed = manager->armed_garbage;
    int not_armed = 0;
    for (int i = 0; i < manager->max_garbage_in_queue; i++) {
        not_armed += manager->queue_amount[i];
    }

    int at_y = manager->h-1;
    while (armed > 0) {
        if (at_y < 0) break;
        armed--;
        mvwaddch(manager->win, at_y, 0, ' ' | COLOR_PAIR(11));
        at_y--;
    }
    while (not_armed > 0) {
        if (at_y < 0) break;
        not_armed--;
        mvwaddch(manager->win, at_y, 0, ' ' | COLOR_PAIR(12));
        at_y--;
    }
    
    wrefresh(manager->win);
}

tetris_board *construct_tetris_board(const tetris_board_settings *settings) {
    tetris_board *board = malloc(sizeof(tetris_board));

    // play space
    board->height = settings->play_height;
    board->width = settings->play_width;
    board->state = (int**)malloc(board->height*sizeof(int*));
    for (int i = 0; i < board->height; i++) {
        board->state[i] = malloc(board->width*sizeof(int));
        for (int j = 0; j < board->width; j++) {
            board->state[i][j] = -1;
        }
    }

    int h = settings->window_height;
    int w = settings->window_width;

    // main window
    board->win_h = h+2;
    board->win_w = 2*w+2;
    board->win_y = settings->win_y;
    board->win_x = settings->win_x;

    board->win = newwin(board->win_h, board->win_w, board->win_y, board->win_x);

    // counters and limits
    board->counters = make_default_counters();
    board->limits = make_default_limits();

    // difficulty
    board->difficulty_manager = make_difficulty_manager();
    update_tetris_difficulty(board);
    
    // tetrominos
    board->bag_manager = construct_bag_manager(board, settings->bag_seed);
    board->active_tetromino = take_from_bag(board, board->bag_manager, false);

    // garbage
    board->garbage_manager = construct_tetris_garbage_manager(board);

    // others
    board->highest_tetromino = calculate_highest_piece(board);
    board->info_manager = make_default_info_manager(board);
    board->is_controlled = settings->controlled;
    board->player_id = settings->player_id;
    board->sockfd = settings->sockfd;
    board->player_name = copy_text(settings->player_name);

    return board;
}

// returns true if tetromino <t> can move in direction <dir> on <board>, else false.
bool can_move(tetris_board *board, tetromino *t, int dir) {
    int **pos = get_tetromino_positions(t);
    for (int i = 0; i < 4; i++) {
        int y = pos[i][0] + normal_dir[dir][0];
        int x = pos[i][1] + normal_dir[dir][1];
        if (x < 0 || y < 0 || x >= board->width || y >= board->height || board->state[y][x] != -1) {
            free_pos(pos);
            return false;
        }
    }
    free_pos(pos);
    return true;
}

// attempts to move tetrmonio <t> in direction <dir> on <board>.
// if succeeds returns true, else false
bool move_tetromino(tetris_board *board, tetromino *t, int dir) {
    if (!can_move(board, t, dir)) return false;
    t->y += normal_dir[dir][0];
    t->x += normal_dir[dir][1];
    return true;
}

// things that need to be reset when a new tetromino is added
void new_tetromino_reset(tetris_board *board) {
    board->counters->lock_delay = 0;
    board->counters->lock_times = 0;
    board->counters->gravity_count = 0;
    board->counters->last_rotation = -1;
}

void draw_score_messages(tetris_board *board, score_report *score_rep) {
    WINDOW *win = board->info_manager->clear_win;
    werase(win);
    int w = board->info_manager->clear_w;
    char buf[50];
    wattron(win, A_BOLD);
    int xpos = (w-strlen(score_rep->message))/2;
    mvwprintw(win, 0, xpos, "%s", score_rep->message);

    if (score_rep->score != 0) {
        sprintf(buf, "+%lli score", score_rep->score);
        xpos = (w-strlen(buf))/2;
        mvwprintw(win, 1, xpos, "%s", buf);
    }

    if (score_rep->garbage != 0) {
        sprintf(buf, "%lli garbage", score_rep->garbage);
        xpos = (w-strlen(buf))/2;
        mvwprintw(win, 2, xpos, "%s", buf);
    }
    wattroff(win, A_BOLD);
    wrefresh(win);
}

// will free the score report
// returns 1 when game lost due to out of bounds garbage
int handle_score_report(tetris_board *board, score_report *score_rep) {
    // add score
    board->counters->score += score_rep->score;

    send_garbage(board, score_rep->garbage);

    // trigger own garbage if no lines cleared
    int ret = 0;
    if (score_rep->lines_cleared == 0) {
        ret = trigger_garbage(board);
    }

    // draw messages at bottom of board
    draw_score_messages(board, score_rep);

    free_score_report(score_rep);
    return ret;
}

// hard drops the active tetromino in <board>
// returns 1 when lost due to out of bounds garbage
int hard_drop(tetris_board *board) {
    while (can_move(board, board->active_tetromino, DIR_DOWN)) {
        move_tetromino(board, board->active_tetromino, DIR_DOWN);
        board->counters->score += 2; // score per line for hard drop
    }
    int **pos = get_tetromino_positions(board->active_tetromino);
    for (int i = 0; i < 4; i++) {
        int y = pos[i][0];
        int x = pos[i][1];
        board->state[y][x] = board->active_tetromino->type;
    }
    free_pos(pos);
    int ret = handle_score_report(board, update_clear_lines(board)); // update this before we take new piece, so it properly checks t-spins
    free(board->active_tetromino);
    board->active_tetromino = take_from_bag(board, board->bag_manager, false);
    board->counters->hold_count = 0;
    board->highest_tetromino = calculate_highest_piece(board);
    new_tetromino_reset(board);
    return ret;
}

// returns true if tetromino <test> (assumed to be an actively falling tetromino)
// is in a valid position <board>
bool valid_pos(tetromino *test, tetris_board *board) {
    int **pos = get_tetromino_positions(test);
    for (int i = 0; i < 4; i++) {
        int y = pos[i][0];
        int x = pos[i][1];
        if (x < 0 || x >= board->width || y < 0 || y >= board->height || board->state[y][x] != -1) {
            free_pos(pos);
            return false;
        }
    }
    free_pos(pos);
    return true;
}

// tries to move the active tetromino in <board> down
// returns 1 when game lost due to out of bounds garbage
int apply_gravity(tetris_board *board, bool *is_changed) {
    board->counters->gravity_count++;
    if (move_tetromino(board, board->active_tetromino, DIR_DOWN)) {
        board->counters->last_rotation = -1;
        *is_changed = true;
        return 0;
    }
    if (
        board->counters->lock_delay >= board->limits->lock_delay ||
        board->counters->lock_times >= board->limits->lock_times ||
        board->counters->gravity_count >= board->limits->gravity_count
    ) {
        int ret = hard_drop(board);
        *is_changed = true;
        if (ret == 1) return 1;
    }
    return 0;
}

// swaps active tetromino with hold tetromino (if exists)
// returns true if swapped
// returns false if nothing is done
bool hold_tetromino(tetris_board *board) {
    if (board->counters->hold_count >= board->limits->hold_count) return false;
    board->counters->hold_count++;

    int our_tetromino = board->active_tetromino->type; // will be in hold after
    free(board->active_tetromino);
    
    if (board->bag_manager->held_tetromino == -1) {
        // no piece in hold, take from bag
        board->active_tetromino = take_from_bag(board, board->bag_manager, false);
    } else {
        // piece already in hold, take from hold
        tetromino_construct_info *tinfo = malloc(sizeof(tetromino_construct_info));
        tinfo->board = board;
        tinfo->id = board->bag_manager->held_tetromino;
        board->active_tetromino = construct_tetromino(tinfo);
        free(tinfo);
    }

    board->bag_manager->held_tetromino = our_tetromino;
    new_tetromino_reset(board);
    return true;
}

void handle_movement(tetris_board *board) {
    // lock_delay
    if (!can_move(board, board->active_tetromino, DIR_DOWN)){
        board->counters->lock_delay = 0;
        board->counters->lock_times++;
    }
}

void update_info_manager(tetris_board *board) {
    tetris_info_manager *info_manager = board->info_manager;
    
    werase(info_manager->info_win);
    char buf[50];
    
    if (board->player_name) {
        sprintf(buf, "%s", board->player_name);
        int mid = (info_manager->info_w-strlen(buf))/2;
        mvwprintw(info_manager->info_win, 0, mid, "%s", buf); 
    }

    sprintf(buf, "Time: %llds", board->counters->total_time_elapsed/1000000);
    int mid = (info_manager->info_w-strlen(buf))/2;
    mvwprintw(info_manager->info_win, 1, mid, "%s", buf); 

    sprintf(buf, "Level: %d", board->difficulty_manager->current_level);
    mid = (info_manager->info_w-strlen(buf))/2;
    mvwprintw(info_manager->info_win, 2, mid, "%s", buf);

    sprintf(buf, "Score: %d", board->counters->score);
    mid = (info_manager->info_w-strlen(buf))/2;
    mvwprintw(info_manager->info_win, 3, mid, "%s", buf);

    wrefresh(info_manager->info_win);
}

int update_controlled(tetris_board_update *update, bool *is_changed) {
    int user_input = update->user_input;
    ll delta_time = update->delta_time;
    tetris_board *board = update->board;
    board_counters *counters = board->counters;
    board_counters *limits = board->limits;

    counters->total_time_elapsed += delta_time;
    update_tetris_difficulty(board);

    // if (user_input == 'p') add_garbage(board, 1);

    if (user_input == get_keyboard_button(GAME_ROTATE_RIGHT)){
        if (rotate_tetromino(board, DIR_RIGHT)) {
            handle_movement(board);
            *is_changed = true;
        }
    }
    if (user_input == get_keyboard_button(GAME_ROTATE_LEFT)) {
        if (rotate_tetromino(board, DIR_LEFT)) {
            handle_movement(board);
            *is_changed = true;
        }
    }
    if (user_input == get_keyboard_button(GAME_HOLD)) {
        hold_tetromino(board);
        *is_changed = true;
    }
    if (user_input == get_keyboard_button(GAME_RIGHT)) {
        if (move_tetromino(board, board->active_tetromino, DIR_RIGHT)) {
            handle_movement(board);
            board->counters->last_rotation = -1;
            *is_changed = true;
        }
    }
    if (user_input == get_keyboard_button(GAME_LEFT)) {
        if (move_tetromino(board, board->active_tetromino, DIR_LEFT)) {
            handle_movement(board);
            board->counters->last_rotation = -1;
            *is_changed = true;
        }
    }
    if (user_input == get_keyboard_button(GAME_SOFTDROP)) {
        board->counters->last_rotation = -1;
        if (move_tetromino(board, board->active_tetromino, DIR_DOWN)) {
            counters->time_since_gravity = 0;
            board->counters->last_rotation = -1;
            counters->score += 1; // score for soft drop
            *is_changed = true;
        }
    }
    if (user_input == get_keyboard_button(GAME_HARDDROP)) {
        board->counters->last_rotation = -1;
        int ret = hard_drop(board);
        *is_changed = true;
        if (ret == 1) return 1;
    }

    // advance lock_delay if on ground
    if (!can_move(board, board->active_tetromino, DIR_DOWN)) {
        board->counters->lock_delay += delta_time;
    }

    // check if falling
    counters->time_since_gravity += delta_time;
    while (counters->time_since_gravity > limits->time_since_gravity) {
        counters->time_since_gravity -= limits->time_since_gravity;
        int ret = apply_gravity(board, is_changed);
        if (ret == 1) return ret;
    }

    update_garbage(board->garbage_manager, update->delta_time);

    // lose condition
    if (!valid_pos(board->active_tetromino, board)) {
        *is_changed = true;
        return 1;
    }
    return 0;
}

// returns 1 if game ended (0 otherwise)
int update_board(tetris_board_update *update, bool *is_changed) {
    tetris_board *board = update->board;
    
    // user input
    int ret = 0;
    if (board->is_controlled) {
        ret = update_controlled(update, is_changed);
    }

    draw_tetris_board(board);
    return ret;
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

// draws upcoming pieces window for <board>
void draw_upcoming(tetris_board *board) {
    tetris_bag_manager *bag = board->bag_manager;

    // clear window
    for (int i = 0; i < bag->upcoming_h; i++) {
        for (int j = 0; j < bag->upcoming_w; j++) {
            mvwaddch(bag->upcoming, i, j, ' ');
        }
    }

    // draw window title
    int mid = (bag->upcoming_w-4)/2;
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
            mvwaddch(bag->upcoming, base_y-pos[j][0], base_x+pos[j][1]*2, ' ' | COLOR_PAIR(t.type+1));
            mvwaddch(bag->upcoming, base_y-pos[j][0], base_x+pos[j][1]*2+1, ' ' | COLOR_PAIR(t.type+1));
        }
        free_pos(pos);
    }
    free(upcoming_types);

    wrefresh(bag->upcoming);
}

// draws hold window for <board>
void draw_hold(tetris_board *board) {
    tetris_bag_manager *bag = board->bag_manager;

    // clear window
    for (int i = 0; i < bag->hold_h; i++) {
        for (int j = 0; j < bag->hold_w; j++) {
            mvwaddch(bag->hold, i, j, ' ');
        }
    }

    // draw window title
    int mid = (bag->hold_w-4)/2;
    mvwprintw(bag->hold, 1, mid, "HOLD");

    // draw hold piece
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
            mvwaddch(bag->hold, base_y-pos[i][0], base_x+pos[i][1]*2, ' ' | COLOR_PAIR(tetromino_id+1));
            mvwaddch(bag->hold, base_y-pos[i][0], base_x+pos[i][1]*2+1, ' ' | COLOR_PAIR(tetromino_id+1));
        }
        free_pos(pos);
    }
    wrefresh(bag->hold);
}

// draws everything related to the <board>
void draw_tetris_board(tetris_board *board) {
    // erase();
    // mvprintw(0, 0, "%d", board->active_tetromino->y);
    // werase(board->win);
    
    // fill background (also erases everything)
    int h = board->height;
    int w = board->width;
    int win_h = board->win_h;
    int win_w = board->win_w;
    for (int i = 0; i < win_h; i++) {
        for (int j = 0; j < win_w; j++) {
            mvwaddch(board->win, i, j, '.' | A_DIM);
        }
    }

    // draw border
    wborder(board->win, 0, 0, ' ', 0, ' ', ' ', 0, 0);

    // draw already placed blocks
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            if (board->state[i][j] == -1) continue;
            int draw_y = board->win_h-2-i;
            int draw_x1 = 2*j+1;
            int draw_x2 = 2*j+2;
            if (draw_y >= 0){
                mvwaddch(board->win, draw_y, draw_x1, ' ' | COLOR_PAIR(board->state[i][j]+1));
                mvwaddch(board->win, draw_y, draw_x2, ' ' | COLOR_PAIR(board->state[i][j]+1));
            }
        }
    }

    if (board->active_tetromino != NULL) {
        // draw warning
        if (board->highest_tetromino >= 16) {
            tetromino *warning = take_from_bag(board, board->bag_manager, true);
            int **pos = get_tetromino_positions(warning);
            int phase = (board->counters->total_time_elapsed/(1000*500))%2; // phase flips 0/1 every 500ms
            for (int i = 0; i < 4; i++) {
                if (phase == 1) {
                    mvwaddch(board->win, board->win_h-2-pos[i][0], 2*pos[i][1]+1, 'X' | COLOR_PAIR(10));
                    mvwaddch(board->win, board->win_h-2-pos[i][0], 2*pos[i][1]+2, 'X' | COLOR_PAIR(10));
                }
            }
            free(warning);
            free_pos(pos);
        }

        // draw prediction
        tetromino *prediction = deepcpy_tetromino(board->active_tetromino);
        while (move_tetromino(board, prediction, DIR_DOWN));
        int **pos = get_tetromino_positions(prediction);
        for (int i = 0; i < 4; i++) {
            mvwaddch(board->win, board->win_h-2-pos[i][0], 2*pos[i][1]+1, '@');
            mvwaddch(board->win, board->win_h-2-pos[i][0], 2*pos[i][1]+2, '@');
        }
        free_pos(pos);
        free(prediction);
        
        // draw active tetromino
        pos = get_tetromino_positions(board->active_tetromino);
        for (int i = 0; i < 4; i++) {
            mvwaddch(board->win, board->win_h-2-pos[i][0], 2*pos[i][1]+1, ' ' | COLOR_PAIR(board->active_tetromino->type+1));
            mvwaddch(board->win, board->win_h-2-pos[i][0], 2*pos[i][1]+2, ' ' | COLOR_PAIR(board->active_tetromino->type+1));
        }
        free_pos(pos);
    }
    wrefresh(board->win);

    // draw other related windows
    draw_hold(board);
    draw_upcoming(board);
    draw_garbage(board->garbage_manager);
    update_info_manager(board);
}

void free_bag_manager(tetris_bag_manager *bag) {
    delete_window(bag->upcoming);
    delete_window(bag->hold);
    free(bag->now);
    free(bag->next);
    free(bag);
}

void deconstruct_tetris_board(tetris_board *board) {
    delete_window(board->win);
    for(int i = 0; i < board->height; i++) {
        free(board->state[i]);
    }
    free(board->state);
    free(board->active_tetromino);
    free(board->counters);
    free(board->limits);
    free_bag_manager(board->bag_manager);
    free_tetris_difficulty_manager(board->difficulty_manager);
    free_info_manager(board->info_manager);
    free_garbage_manager(board->garbage_manager);
    if (board->player_name) free(board->player_name);
    free(board);
}
#pragma once

enum {
    GAME_RIGHT = 0,
    GAME_LEFT = 1,
    GAME_SOFTDROP = 2,
    GAME_HARDDROP = 3,
    GAME_ROTATE_LEFT = 4,
    GAME_ROTATE_RIGHT = 5,
    GAME_HOLD = 6,
    MENU_UP = 7,
    MENU_RIGHT = 8,
    MENU_DOWN = 9,
    MENU_LEFT = 10,
    MENU_SELECT = 11,
    MENU_BACK = 12,
    MENU_SELECT2 = 13,
};

typedef struct {
    int game_left;
    int game_right;
    int game_softdrop;
    int game_harddrop;
    int game_rotate_left;
    int game_rotate_right;
    int game_hold;
    int menu_up;
    int menu_right;
    int menu_down;
    int menu_left;
    int menu_select;
    int menu_back;
    int menu_select2;
} keyboard_binds;

void init_binds();
void load_binds();
void save_binds();

int get_keyboard_button(int action);
void set_keyboard_button(int action, int new_button);
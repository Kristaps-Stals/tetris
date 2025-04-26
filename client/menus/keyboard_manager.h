#pragma once

enum {
    TOTAL_BINDS = 14, // update pls
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
    int button; // which button triggers it
    char *setting_name; // shown in settings menu
    char *config_name; // name in config file
} keyboard_bind;

void init_binds();
void load_binds();
void save_binds();

int get_keyboard_button(int action);
void set_keyboard_button(int action, int new_button);
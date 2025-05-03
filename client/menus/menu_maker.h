#pragma once
#include "textbox.h"

enum {
    CLOSE_MENU = -1,
    START_GAME = 1,
    OPEN_SETTINGS = 2,
    OPEN_KEYBINDINGS = 3,
    SAVE_KEYBINDINGS = 4,
    DEFAULT_KEYBINDINGS = 5,
    CLOSE_KEYBINDINGS = 6,
    OPEN_JOIN = 7,
    ATTEMPT_JOIN = 8,
    TOGGLE_READY = 9
};

enum {
    MAIN_MENU_ID = 0,
    SETTINGS_MENU_ID = 1,
    ENDSCREEN_MENU_ID = 2,
    KEYBINDINGS_MENU_ID = 3,
    JOIN_LOBBY_MENU_ID = 4,
    LOBBY_MENU_ID = 5,
};

enum {
    WRITE_ID_JOIN_IP = 0,
    WRITE_ID_JOIN_PORT = 1
};

typedef struct menu_manager {
    textbox **stack;
    int top, max_stack;
    bool is_editing;
    int       server_socket;
    char      slot_names[8][32];
    bool      slot_ready[8]; // new field to track readiness

} menu_manager;

bool open_menu(menu_manager *manager, textbox *new_menu);
menu_manager *make_menu_manager();
void free_menu_manager(menu_manager *manager);
textbox *make_main_menu();
textbox *make_settings_menu();
textbox *make_endscreen(tetris_board *board);
int update_menus(menu_manager *manager, int user_input);
int manage_menus(menu_manager *manager, int user_input);
textbox *make_lobby_menu(menu_manager *manager);

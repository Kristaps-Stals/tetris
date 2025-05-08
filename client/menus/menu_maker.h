#pragma once
#include "../tetris/board.h"
#include "textbox.h"
#include "../../shared/protocol.h"

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
    TOGGLE_READY = 9,
    TOGGLE_PLAYER_STATE = 10,
    SAVE_SETTINGS = 11,
    CLOSE_VERSUS_ENDSCREEN = 12,
};

enum {
    MAIN_MENU_ID = 0,
    SETTINGS_MENU_ID = 1,
    ENDSCREEN_MENU_ID = 2,
    KEYBINDINGS_MENU_ID = 3,
    JOIN_LOBBY_MENU_ID = 4,
    LOBBY_MENU_ID = 5,
    ENDSCREEN_VERSUS_MENU_ID = 6,
};

enum {
    WRITE_ID_JOIN_IP = 0,
    WRITE_ID_JOIN_PORT = 1,
    WRITE_ID_NICKNAME = 2
};

enum {
    UPDSTATE_SOLO = 1,
    UPDSTATE_VERSUS = 2,
};

typedef struct menu_manager {
    void* parent;
    textbox **stack;
    int top, max_stack;
    bool is_editing;
    int server_socket;
    int player_id;
    char slot_names[8][32];
    int player_1, player_2;
    int player_1_ready, player_2_ready;
    char start_counter;
    int bag_seed;
} menu_manager;

bool open_menu(menu_manager *manager, textbox *new_menu);
menu_manager *make_menu_manager(void* parent);
void free_menu_manager(menu_manager *manager);
textbox *make_main_menu();
textbox *make_settings_menu();
textbox *make_endscreen(tetris_board *board);
textbox *make_endscreen_versus(msg_winner_t *msg);
textbox *make_lobby_menu();
void pop_menu_stack(menu_manager *manager);
void update_lobby_menu(menu_manager *manager);
int update_menus(menu_manager *manager, int user_input);
int manage_menus(menu_manager *manager, int user_input);

int change_elem_text(menu_manager *menu_manager_, int elem_id, char *new_text);
int change_elem_visibility(menu_manager *menu_manager_, int elem_id, bool visible);
int add_elem_attributes(menu_manager *menu_manager_, int elem_id, attr_t attributes);
int remove_elem_attributes(menu_manager *menu_manager_, int elem_id, attr_t attributes);
int set_elem_attributes(menu_manager *menu_manager_, int elem_id, attr_t attributes);

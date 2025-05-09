#include "../tetris/board.h"
#include "textbox.h"
#include "menu_maker.h"
#include <stdlib.h>
#include "string.h"
#include "../../shared/kstring.h"
#include "keyboard_manager.h"
#include "../net/net.h" 
#include <unistd.h>    
#include <fcntl.h>
#include "settings.h"

char last_saved_nick[NICKNAME_MAX_LEN] = "";

menu_manager *make_menu_manager(void* parent) {
    menu_manager *manager = malloc(sizeof(menu_manager));
    manager->parent = parent;
    manager->max_stack = 10;
    manager->stack = malloc((manager->max_stack+1)*sizeof(textbox*));
    manager->top = 0;
    manager->is_editing = false;
    manager->server_socket = -1; 
    for (int i = 0; i < 8; i++) {
        // manager->slot_ready[i] = false;
        strcpy(manager->slot_names[i], "(empty)");
    }
    manager->player_1 = -1;
    manager->player_1_ready = 0;
    manager->player_2 = -1;
    manager->player_2_ready = 0;
    manager->player_id = -1;
    manager->stack[0] = make_main_menu();
    manager->start_counter = ' ';
    return manager;
}
void free_menu_manager(menu_manager *manager) {
    for (int i = manager->top; i >= 0; i--) {
        free_textbox(manager->stack[i]);
    }
    free(manager->stack);
    free(manager);
}

textbox *make_main_menu() {
    int w = 20;
    int h = 6;
    int x = (COLS-w)/2;
    int y = (LINES-h)/2;

    size_info *pos = make_size_info(h, w, y, x);
    int ELEM_CNT = 5;
    textbox_element **elems = malloc(ELEM_CNT*sizeof(textbox_element*));

    size_info *pos_text1 = make_size_info(1, 9, 0, 2);
    char* text1 = "Main menu";
    textbox_text *info_text1 = make_text(text1);
    elems[0] = make_element(TEXT_ID, pos_text1, info_text1, NULL, A_NORMAL);

    size_info *pos_button1 = make_size_info(1, 18, 1, 1);
    textbox_neighbours *next_button1 = make_neighbours(4, -1, 2, -1);
    textbox_button *info_button1 = make_button("       solo       ", START_GAME);
    elems[1] = make_element(BUTTON_ID, pos_button1, info_button1, next_button1, A_NORMAL);

    size_info *pos_button4 = make_size_info(1, 18, 2, 1);
    textbox_neighbours *next_button4 = make_neighbours(1, -1, 3, -1);
    textbox_button *info_button4 = make_button("      versus      ", OPEN_JOIN);
    elems[2] = make_element(BUTTON_ID, pos_button4, info_button4, next_button4, A_NORMAL);

    size_info *pos_button2 = make_size_info(1, 18, 3, 1);
    textbox_neighbours *next_button2 = make_neighbours(2, -1, 4, -1);
    textbox_button *info_button2 = make_button("     settings     ", OPEN_SETTINGS);
    elems[3] = make_element(BUTTON_ID, pos_button2, info_button2, next_button2, A_NORMAL);

    size_info *pos_button3 = make_size_info(1, 18, 4, 1);
    textbox_neighbours *next_button3 = make_neighbours(3, -1, 1, -1);
    textbox_button *info_button3 = make_button("       quit       ", CLOSE_MENU);
    elems[4] = make_element(BUTTON_ID, pos_button3, info_button3, next_button3, A_NORMAL);

    return make_textbox(pos, elems, ELEM_CNT, 1, MAIN_MENU_ID);
}

textbox *make_settings_menu() {
    // snapshot the currently saved nick
    strncpy(last_saved_nick, get_nickname(), NICKNAME_MAX_LEN);

    int w = 40;
    int h = 20;
    int x = (COLS - w) / 2;
    int y = (LINES - h) / 2;
    size_info *pos = make_size_info(h, w, y, x);

    const int ELEM_CNT = 7;
    textbox_element **elems = malloc(ELEM_CNT * sizeof(textbox_element*));

    // title
    elems[0] = make_element(
        TEXT_ID,
        make_size_info(1, 9, 0, 2),
        make_text("Settings"),
        NULL,
        A_NORMAL
    );

    // nickname label
    elems[1] = make_element(
        TEXT_ID,
        make_size_info(1, 8, 2, 3),
        make_text("Nickname"),
        NULL,
        A_NORMAL
    );

    // nickname input field
    elems[2] = make_element(
        WRITE_ELEMENT_ID,
        make_size_info(1, 16, 2, w-19),
        make_write_elem((char*)get_nickname(), 16, WRITE_ID_NICKNAME),
        make_neighbours(6, -1, 4, -1), 
        A_NORMAL
    );

    // save button
    elems[3] = make_element(
        BUTTON_ID,
        make_size_info(1, 4, h - 2, w - 1 - 4 - 6),
        make_button("save", SAVE_SETTINGS),
        make_neighbours(2, 4, 6, 4), // up→nick(2), right→back(4), down→edit(6), left→back(4)
        A_NORMAL
    );
    elems[3]->visible = false;

    // back button
    elems[4] = make_element(
        BUTTON_ID,
        make_size_info(1, 4, h - 2, w - 1 - 4 - 1),
        make_button("back", CLOSE_MENU),
        make_neighbours(2, 3, 6, 3),
        A_NORMAL
    );

    // keybindings label
    elems[5] = make_element(
        TEXT_ID,
        make_size_info(1, 11, 1, 3),
        make_text("Keybindings"),
        NULL,
        A_NORMAL
    );

    // edit‐keybindings button
    elems[6] = make_element(
        BUTTON_ID,
        make_size_info(1, 4, 1, w - 1 - 4 - 2),
        make_button("edit", OPEN_KEYBINDINGS),
        make_neighbours(4, -1, 2, -1),
        A_NORMAL
    );

    return make_textbox(pos, elems, ELEM_CNT, 6, SETTINGS_MENU_ID);
}

textbox *make_endscreen(tetris_board *board) {
    int w = 40;
    int h = 20;
    int x = (COLS-w)/2;
    int y = (LINES-h)/2;

    int midh = h/2;
    char text[100];

    size_info *pos = make_size_info(h, w, y, x);
    int ELEM_CNT = 5;
    textbox_element **elems = malloc(ELEM_CNT*sizeof(textbox_element*));

    sprintf(text, "Endscreen");
    size_info *pos_text1 = make_size_info(1, (int)strlen(text), 0, 2);
    textbox_text *info_text1 = make_text(text);
    elems[0] = make_element(TEXT_ID, pos_text1, info_text1, NULL, A_NORMAL);

    sprintf(text, "GAME OVER");
    int text2_x = (w-(int)strlen(text))/2;
    size_info *pos_text2 = make_size_info(1, (int)strlen(text), midh-3, text2_x);
    textbox_text *info_text2 = make_text(text);
    elems[1] = make_element(TEXT_ID, pos_text2, info_text2, NULL, A_NORMAL);

    sprintf(text, "%.2f s", board->counters->total_time_elapsed / 1000000.0);
    int text3_x = (w-(int)strlen(text))/2;
    size_info *pos_text3 = make_size_info(1, (int)strlen(text), midh-1, text3_x);
    textbox_text *info_text3 = make_text(text);
    elems[2] = make_element(TEXT_ID, pos_text3, info_text3, NULL, A_NORMAL);

    sprintf(text, "Score: %d", board->counters->score);
    int text4_x = (w-(int)strlen(text))/2;
    size_info *pos_text4 = make_size_info(1, (int)strlen(text), midh, text4_x);
    textbox_text *info_text4 = make_text(text);
    elems[3] = make_element(TEXT_ID, pos_text4, info_text4, NULL, A_NORMAL);
    
    size_info *pos_button1 = make_size_info(1, 4, h-2, w-1-4);
    textbox_neighbours *next_button1 = make_neighbours(-1, -1, -1, -1);
    textbox_button *info_button1 = make_button("back", CLOSE_MENU);
    elems[4] = make_element(BUTTON_ID, pos_button1, info_button1, next_button1, A_NORMAL);

    return make_textbox(pos, elems, ELEM_CNT, 4, ENDSCREEN_MENU_ID);
}

textbox *make_keybind_menu() {
    int w = 40;
    int h = 20;
    int x = (COLS-w)/2;
    int y = (LINES-h)/2;

    size_info *pos = make_size_info(h, w, y, x);

    int bind_elems = TOTAL_BINDS*2;
    int ELEM_CNT = bind_elems+4;
    textbox_element **elems = malloc(ELEM_CNT*sizeof(textbox_element*));

    for (int i = 0; i < TOTAL_BINDS; i++) {
        keyboard_bind bind = get_bind(i);

        size_info *pos_bind = make_size_info(1, 20, 1+i, 3);
        textbox_text *info_text_bind = make_text(bind.setting_name);
        elems[i*2] = make_element(TEXT_ID, pos_bind, info_text_bind, NULL, A_NORMAL);

        size_info *pos_button_bind = make_size_info(1, 8, 1+i, w-1-9);
        int up = 2*i+1 -2;
        int down = 2*i+1 +2;
        if (i == 0) up = bind_elems+1;
        if (i == TOTAL_BINDS-1) down = bind_elems+1;
        textbox_neighbours *next_button_bind = make_neighbours(up, -1, down, -1);
        textbox_keybind_select *info_keybind = make_keybind_select("...", i); // TODO: link trigger
        elems[2*i+1] = make_element(KEYBIND_SELECT_ID, pos_button_bind, info_keybind, next_button_bind, A_NORMAL);
    }

    size_info *pos_text1 = make_size_info(1, 11, 0, 2);
    textbox_text *info_text1 = make_text("Keybindings");
    elems[bind_elems+0] = make_element(TEXT_ID, pos_text1, info_text1, NULL, A_NORMAL);

    size_info *pos_button1 = make_size_info(1, 6, h-2, w-1-6-1);
    textbox_neighbours *next_button1 = make_neighbours(bind_elems-1, bind_elems+3, 1, bind_elems+2);
    textbox_button *info_button1 = make_button("cancel", CLOSE_KEYBINDINGS);
    elems[bind_elems+1] = make_element(BUTTON_ID, pos_button1, info_button1, next_button1, A_NORMAL);

    size_info *pos_button2 = make_size_info(1, 4, h-2, w-1-6-1-6);
    textbox_neighbours *next_button2 = make_neighbours(bind_elems-1, bind_elems+1, 1, bind_elems+3);
    textbox_button *info_button2 = make_button("save", SAVE_KEYBINDINGS);
    elems[bind_elems+2] = make_element(BUTTON_ID, pos_button2, info_button2, next_button2, A_NORMAL);

    size_info *pos_button3 = make_size_info(1, 9, h-2, w-1-4-1-6-12);
    textbox_neighbours *next_button3 = make_neighbours(bind_elems-1, bind_elems+2, 1, bind_elems+1);
    textbox_button *info_button3 = make_button("defaults", DEFAULT_KEYBINDINGS);
    elems[bind_elems+3] = make_element(BUTTON_ID, pos_button3, info_button3, next_button3, A_NORMAL);

    return make_textbox(pos, elems, ELEM_CNT, 1, KEYBINDINGS_MENU_ID);
}

textbox *make_join_menu() {
    int w = 23;
    int h = 4;
    int x = (COLS-w)/2;
    int y = (LINES-h)/2;

    size_info *pos = make_size_info(h, w, y, x);

    int ELEM_CNT = 6;
    textbox_element **elems = malloc(ELEM_CNT*sizeof(textbox_element*));
    
    size_info *pos_write = make_size_info(1, 15, 1, 1);
    textbox_neighbours *next_write = make_neighbours(4, 1, 4, 1);
    textbox_write *info_write = make_write_elem("127.0.0.1", 15, WRITE_ID_JOIN_IP);
    elems[0] = make_element(WRITE_ELEMENT_ID, pos_write, info_write, next_write, A_NORMAL);

    pos_write = make_size_info(1, 5, 1, 17);
    next_write = make_neighbours(3, 0, 3, 0);
    info_write = make_write_elem("5000", 5, WRITE_ID_JOIN_PORT);
    elems[1] = make_element(WRITE_ELEMENT_ID, pos_write, info_write, next_write, A_NORMAL);

    size_info *pos_text1 = make_size_info(1, 4, 0, 2);
    textbox_text *info_text1 = make_text("Join");
    elems[2] = make_element(TEXT_ID, pos_text1, info_text1, NULL, A_NORMAL);

    size_info *pos_button1 = make_size_info(1, 4, h-2, w-1-4);
    textbox_neighbours *next_button1 = make_neighbours(1, 4, 1, 4);
    textbox_button *info_button1 = make_button("back", CLOSE_MENU);
    elems[3] = make_element(BUTTON_ID, pos_button1, info_button1, next_button1, A_NORMAL);

    size_info *pos_button2 = make_size_info(1, 4, h-2, w-1-4-6);
    textbox_neighbours *next_button2 = make_neighbours(0, 3, 0, 3);
    textbox_button *info_button2 = make_button("join", ATTEMPT_JOIN);
    elems[4] = make_element(BUTTON_ID, pos_button2, info_button2, next_button2, A_NORMAL);

    size_info *pos_text2 = make_size_info(1, 1, 1, 16);
    textbox_text *info_text2 = make_text(":");
    elems[5] = make_element(TEXT_ID, pos_text2, info_text2, NULL, A_NORMAL);

    return make_textbox(pos, elems, ELEM_CNT, 0, JOIN_LOBBY_MENU_ID);
}

textbox *make_endscreen_versus(msg_winner_t *msg) {
    int w = 40;
    int h = 20;
    int x = (COLS-w)/2;
    int y = (LINES-h)/2;

    int midh = h/2;
    char text[100];

    size_info *pos = make_size_info(h, w, y, x);
    int ELEM_CNT = 8;
    textbox_element **elems = malloc(ELEM_CNT*sizeof(textbox_element*));

    sprintf(text, "Endscreen");
    size_info *pos_text1 = make_size_info(1, (int)strlen(text), 0, 2);
    textbox_text *info_text1 = make_text(text);
    elems[0] = make_element(TEXT_ID, pos_text1, info_text1, NULL, A_NORMAL);

    sprintf(text, "GAME OVER");
    int text2_x = (w-(int)strlen(text))/2;
    size_info *pos_text2 = make_size_info(1, (int)strlen(text), midh-4, text2_x);
    textbox_text *info_text2 = make_text(text);
    elems[1] = make_element(TEXT_ID, pos_text2, info_text2, NULL, A_NORMAL);

    sprintf(text, "%.2f s", msg->total_time / 1000000.0);
    int text3_x = (w-(int)strlen(text))/2;
    size_info *pos_text3 = make_size_info(1, (int)strlen(text), midh-3, text3_x);
    textbox_text *info_text3 = make_text(text);
    elems[2] = make_element(TEXT_ID, pos_text3, info_text3, NULL, A_NORMAL);

    sprintf(text, "Player_1: %s", msg->player_names[0]);
    if (msg->winner == 0) {
        sprintf(text, "Player_1: %s (winner)", msg->player_names[0]);
    }
    int posx = (w-(int)strlen(text))/2;
    size_info *pos_text = make_size_info(1, (int)strlen(text), midh-1, posx);
    textbox_text *info_text = make_text(text);
    elems[3] = make_element(TEXT_ID, pos_text, info_text, NULL, A_NORMAL);
    if (msg->winner == 0) {
        elems[3]->attributes = COLOR_PAIR(13);
    }

    sprintf(text, "Score: %d", msg->score_player_1);
    int text4_x = (w-(int)strlen(text))/2;
    size_info *pos_text4 = make_size_info(1, (int)strlen(text), midh, text4_x);
    textbox_text *info_text4 = make_text(text);
    elems[4] = make_element(TEXT_ID, pos_text4, info_text4, NULL, A_NORMAL);
    
    sprintf(text, "Player_2: %s", msg->player_names[1]);
    if (msg->winner == 1) {
        sprintf(text, "Player_2: %s (winner)", msg->player_names[1]);
    }
    posx = (w-(int)strlen(text))/2;
    pos_text = make_size_info(1, (int)strlen(text), midh+2, posx);
    info_text = make_text(text);
    elems[5] = make_element(TEXT_ID, pos_text, info_text, NULL, A_NORMAL);
    if (msg->winner == 1) {
        elems[5]->attributes = COLOR_PAIR(13);
    }

    sprintf(text, "Score: %d", msg->score_player_2);
    posx = (w-(int)strlen(text))/2;
    pos_text = make_size_info(1, (int)strlen(text), midh+3, posx);
    info_text = make_text(text);
    elems[6] = make_element(TEXT_ID, pos_text, info_text, NULL, A_NORMAL);

    size_info *pos_button1 = make_size_info(1, 4, h-2, w-1-4);
    textbox_neighbours *next_button1 = make_neighbours(-1, -1, -1, -1);
    textbox_button *info_button1 = make_button("back", CLOSE_VERSUS_ENDSCREEN);
    elems[7] = make_element(BUTTON_ID, pos_button1, info_button1, next_button1, A_NORMAL);

    return make_textbox(pos, elems, ELEM_CNT, 7, ENDSCREEN_VERSUS_MENU_ID);
}

textbox *make_lobby_menu() {
    int w = 40;
    int h = 20;
    int x = (COLS-w)/2;
    int y = (LINES-h)/2;

    size_info *pos = make_size_info(h, w, y, x);

    int name_textbox_cnt = 10;
    int ELEM_CNT = name_textbox_cnt+8;
    textbox_element **elems = malloc(ELEM_CNT*sizeof(textbox_element*));

    int max_name_len = 30;
    char* p = "default_text_max_len_name_abc";
    size_info *pos_elem;
    textbox_neighbours *next_elem;
    textbox_text *info_text;
    textbox_button *info_button;

    for (int i = 0; i < 8; i++) {
        pos_elem = make_size_info(1, max_name_len, 8+i, 3);
        info_text = make_text(p);
        elems[i] = make_element(TEXT_ID, pos_elem, info_text, NULL, A_NORMAL);
    }

    pos_elem = make_size_info(1, max_name_len, 3, 3);
    info_text = make_text(p);
    elems[name_textbox_cnt-2] = make_element(TEXT_ID, pos_elem, info_text, NULL, A_NORMAL);
    
    pos_elem = make_size_info(1, max_name_len, 5, 3);
    info_text = make_text(p);
    elems[name_textbox_cnt-1] = make_element(TEXT_ID, pos_elem, info_text, NULL, A_NORMAL);
    
    pos_elem = make_size_info(1, 5, h-2, w-1-5-1);
    next_elem = make_neighbours(-1, name_textbox_cnt+6, -1, name_textbox_cnt+5);
    info_button = make_button("leave", CLOSE_MENU);
    elems[name_textbox_cnt+0] = make_element(BUTTON_ID, pos_elem, info_button, next_elem, A_NORMAL);

    pos_elem = make_size_info(1, 2, 4, 3);
    info_text = make_text("VS");
    elems[name_textbox_cnt+1] = make_element(TEXT_ID, pos_elem, info_text, NULL, A_NORMAL);

    pos_elem = make_size_info(1, 5, 0, 2);
    info_text = make_text("Lobby");
    elems[name_textbox_cnt+2] = make_element(TEXT_ID, pos_elem, info_text, NULL, A_NORMAL);

    pos_elem = make_size_info(1, 7, 7, 2);
    info_text = make_text("Online:");
    elems[name_textbox_cnt+3] = make_element(TEXT_ID, pos_elem, info_text, NULL, A_NORMAL);

    pos_elem = make_size_info(1, 9, 2, 2);
    info_text = make_text("Players:");
    elems[name_textbox_cnt+4] = make_element(TEXT_ID, pos_elem, info_text, NULL, A_NORMAL);

    pos_elem = make_size_info(1, 14, h-2, w-1-5-1-14-1);
    next_elem = make_neighbours(-1, name_textbox_cnt+0, -1, name_textbox_cnt+6);
    info_button = make_button("toggle status", TOGGLE_PLAYER_STATE);
    elems[name_textbox_cnt+5] = make_element(BUTTON_ID, pos_elem, info_button, next_elem, A_NORMAL);

    pos_elem = make_size_info(1, 14, h-2, w-1-5-1-14-1-5-1);
    next_elem = make_neighbours(-1, name_textbox_cnt+5, -1, name_textbox_cnt+0);
    info_button = make_button("ready", TOGGLE_READY);
    elems[name_textbox_cnt+6] = make_element(BUTTON_ID, pos_elem, info_button, next_elem, A_NORMAL);
    
    pos_elem = make_size_info(1, 9, h-2, 2);
    info_text = make_text(" ");
    elems[name_textbox_cnt+7] = make_element(TEXT_ID, pos_elem, info_text, NULL, A_NORMAL);

    return make_textbox(pos, elems, ELEM_CNT, name_textbox_cnt+0, LOBBY_MENU_ID);
}
void update_lobby_menu(menu_manager *manager) {
    for (int i = 0; i < 8; i++) { 
        change_elem_text(manager, i, manager->slot_names[i]);
        if (manager->player_id == i) {
            add_elem_attributes(manager, i, A_BOLD);
        } else {
            remove_elem_attributes(manager, i, A_BOLD);
        }
    }

    if (manager->player_1 != -1) {
        change_elem_text(manager, 8, manager->slot_names[manager->player_1]);
    } else {
        change_elem_text(manager, 8, "(empty)");
    }

    if (manager->player_id == manager->player_1) {
        add_elem_attributes(manager, 8, A_BOLD);
    } else {
        remove_elem_attributes(manager, 8, A_BOLD);
    }

    if (manager->player_2 != -1) {
        change_elem_text(manager, 9, manager->slot_names[manager->player_2]);
    } else {
        change_elem_text(manager, 9, "(empty)");
    }

    if (manager->player_id == manager->player_2) {
        add_elem_attributes(manager, 9, A_BOLD);
    } else {
        remove_elem_attributes(manager, 9, A_BOLD);
    }

    if (manager->player_1_ready) {
        add_elem_attributes(manager, 8, COLOR_PAIR(13));
    } else {
        remove_elem_attributes(manager, 8, COLOR_PAIR(13));
    }

    if (manager->player_2_ready) {
        add_elem_attributes(manager, 9, COLOR_PAIR(13));
    } else {
        remove_elem_attributes(manager, 9, COLOR_PAIR(13));
    }

    if (manager->player_id == manager->player_1 || manager->player_id == manager->player_2) {
        change_elem_visibility(manager, 16, true);
    } else {
        change_elem_visibility(manager, 16, false);
    }

    char txt[5];
    sprintf(txt, "%c", manager->start_counter);
    change_elem_text(manager, 17, txt);
}

void update_save_button_visibility(menu_manager *manager) {
    if (manager->stack[manager->top]->id != SETTINGS_MENU_ID) return;
    int len = 0;
    char* new_nick = fetch_text_from_element(manager, WRITE_ID_NICKNAME, &len);
    bool changed = false;
    if (new_nick && len > 0) {
        if (strncmp(last_saved_nick, new_nick, NICKNAME_MAX_LEN) != 0) {
            changed = true;
        }
    }
    change_elem_visibility(manager, 3, changed);
    if (new_nick) free(new_nick);
}

void save_nickname_if_changed(menu_manager *manager) {
    int len = 0;
    char* new_nick = fetch_text_from_element(manager, WRITE_ID_NICKNAME, &len);
    if (new_nick && len > 0) {
        if (strncmp(last_saved_nick, new_nick, NICKNAME_MAX_LEN) != 0) {
            set_nickname(new_nick);
            strncpy(last_saved_nick, new_nick, NICKNAME_MAX_LEN);
        }
        free(new_nick);
    }
}

// tries to open <new_menu>, returns true on success, false on failure
bool open_menu(menu_manager *manager, textbox *new_menu) {
    if (manager->top == manager->max_stack) {
        free_textbox(new_menu);
        return false;
    }

    // erase previous window
    textbox **stack = manager->stack;
    werase(stack[manager->top]->win);
    wrefresh(stack[manager->top]->win);

    // add new window
    manager->top++;
    manager->stack[manager->top] = new_menu;
    return true;
}

// pops the top textbox in menu manager
void pop_menu_stack(menu_manager *manager) {
    if (manager->server_socket >= 0 && manager->stack[manager->top]->id == LOBBY_MENU_ID) {
        // send MSG_LEAVE before disconnecting
        char reason[] = "Left the lobby";
        send_message(manager->server_socket, MSG_LEAVE, manager->player_id, reason, sizeof(reason));
        close(manager->server_socket);
        manager->server_socket = -1;
    }

    // continue popping menu
    textbox **stack = manager->stack;
    werase(stack[manager->top]->win);
    wrefresh(stack[manager->top]->win);
    free_textbox(stack[manager->top]);
    manager->top--;
}

// returns any positive trigger vals
int update_menus(menu_manager *manager, int user_input) {
    textbox **stack = manager->stack;
    int top = manager->top;

    textbox *active_menu = stack[top];
    int ret = update_textbox(active_menu, user_input);
    if (ret == CLOSE_MENU || user_input == get_keyboard_button(MENU_BACK)) {
        if (
            stack[manager->top]->id != KEYBINDINGS_MENU_ID && // disable default back in keybindings
            manager->is_editing == false // make sure nothing is being edited
        ) { 
            // close menu and go back one layer
            pop_menu_stack(manager);
        }
    }

    if (ret == STOP_EDITING) manager->is_editing = false;
    if (ret == START_EDITING) manager->is_editing = true;

    if (ret > 0) return ret;
    return 0;
}

void close_keybindings(menu_manager *manager) {
    load_binds();
    pop_menu_stack(manager);
}

void close_versus_endscreen(menu_manager* manager) {
    req_lobby_sync(manager);
    pop_menu_stack(manager);
    if (manager->stack[manager->top]->id == LOBBY_MENU_ID) {
        manager->stack[manager->top]->element_selected = 15;
    }
}

void toggle_ready_state(menu_manager *manager) {
    send_message(manager->server_socket, MSG_TOGGLE_READY, manager->player_id, NULL, 0);
}

void toggle_player_state(menu_manager *manager) {
    send_message(manager->server_socket, MSG_TOGGLE_PLAYER, manager->player_id, NULL, 0);
}

// returns signals for main gameloop
// returns 1 to start game
int manage_menus(menu_manager *manager, int user_input) {
    // before handling input, update save button visibility 
    if (manager->stack[manager->top]->id == SETTINGS_MENU_ID) {
        update_save_button_visibility(manager);
    }

    int update_result = update_menus(manager, user_input);

    int ret = 0;
    switch(update_result) {
        case START_GAME:
            ret = UPDSTATE_SOLO;
            break;
        case OPEN_SETTINGS:
            open_menu(manager, make_settings_menu());
            break;
        case OPEN_KEYBINDINGS:  
            open_menu(manager, make_keybind_menu());
            break;
        case OPEN_JOIN:
            open_menu(manager, make_join_menu());
            break;
        case SAVE_KEYBINDINGS:
            save_binds();
            close_keybindings(manager);
            break;
        case DEFAULT_KEYBINDINGS:
            set_default_binds();
            break;
        case CLOSE_KEYBINDINGS:
            close_keybindings(manager);
            break;
        case ATTEMPT_JOIN:
            attempt_join_lobby(manager);
            break;
        case TOGGLE_READY:
            toggle_ready_state(manager);
            break;
        case TOGGLE_PLAYER_STATE:
            toggle_player_state(manager);
            break;
        case SAVE_SETTINGS:
            save_nickname_if_changed(manager);
            save_settings();
            pop_menu_stack(manager);
            break;
        case CLOSE_VERSUS_ENDSCREEN:
            close_versus_endscreen(manager);
            break;
        case CLOSE_MENU:
            break;
    }

    return ret;
}

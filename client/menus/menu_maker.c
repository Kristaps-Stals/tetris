#include "../tetris/board.h"
#include "textbox.h"
#include "menu_maker.h"
#include <stdlib.h>
#include "string.h"
#include "keyboard_manager.h"

enum {
    CLOSE_MENU = -1,
    START_GAME = 1,
    OPEN_SETTINGS = 2,
    OPEN_KEYBINDINGS = 3,
    SAVE_KEYBINDINGS = 4,
    DEFAULT_KEYBINDINGS = 5,
    CLOSE_KEYBINDINGS = 6
};

enum {
    MAIN_MENU_ID = 0,
    SETTINGS_MENU_ID = 1,
    ENDSCREEN_MENU_ID = 2,
    KEYBINDINGS_MENU_ID = 3
};

menu_manager *make_menu_manager() {
    menu_manager *manager = malloc(sizeof(menu_manager));
    manager->max_stack = 10;
    manager->stack = malloc((manager->max_stack+1)*sizeof(textbox*));
    manager->top = 0;
    manager->stack[0] = make_main_menu();
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
    int h = 5;
    int x = (COLS-w)/2;
    int y = (LINES-h)/2;

    size_info *pos = make_size_info(h, w, y, x);
    int ELEM_CNT = 4;
    textbox_element **elems = malloc(ELEM_CNT*sizeof(textbox_element*));

    size_info *pos_text1 = make_size_info(1, 9, 0, 2);
    char* text1 = "Main menu";
    textbox_text *info_text1 = make_text(text1);
    elems[0] = make_element(TEXT_ID, pos_text1, info_text1);

    size_info *pos_button1 = make_size_info(1, 18, 1, 1);
    textbox_neighbours *next_button1 = make_neighbours(3, -1, 2, -1);
    textbox_button *info_button1 = make_button("       play       ", START_GAME, next_button1);
    elems[1] = make_element(BUTTON_ID, pos_button1, info_button1);

    size_info *pos_button2 = make_size_info(1, 18, 2, 1);
    textbox_neighbours *next_button2 = make_neighbours(1, -1, 3, -1);
    textbox_button *info_button2 = make_button("     settings     ", OPEN_SETTINGS, next_button2);
    elems[2] = make_element(BUTTON_ID, pos_button2, info_button2);

    size_info *pos_button3 = make_size_info(1, 18, 3, 1);
    textbox_neighbours *next_button3 = make_neighbours(2, -1, 1, -1);
    textbox_button *info_button3 = make_button("       quit       ", CLOSE_MENU, next_button3);
    elems[3] = make_element(BUTTON_ID, pos_button3, info_button3);

    return make_textbox(pos, elems, ELEM_CNT, 1, MAIN_MENU_ID);
}

textbox *make_settings_menu() {
    int w = 40;
    int h = 20;
    int x = (COLS-w)/2;
    int y = (LINES-h)/2;

    size_info *pos = make_size_info(h, w, y, x);
    int ELEM_CNT = 4;
    textbox_element **elems = malloc(ELEM_CNT*sizeof(textbox_element*));

    size_info *pos_text1 = make_size_info(1, 9, 0, 2);
    textbox_text *info_text1 = make_text("Settings");
    elems[0] = make_element(TEXT_ID, pos_text1, info_text1);

    size_info *pos_button1 = make_size_info(1, 4, h-2, w-1-4-1);
    textbox_neighbours *next_button1 = make_neighbours(3, -1, 3, -1);
    textbox_button *info_button1 = make_button("back", CLOSE_MENU, next_button1);
    elems[1] = make_element(BUTTON_ID, pos_button1, info_button1);

    size_info *pos_text2 = make_size_info(1, 11, 1, 3);
    textbox_text *info_text2 = make_text("Keybindings");
    elems[2] = make_element(TEXT_ID, pos_text2, info_text2);;

    size_info *pos_button2 = make_size_info(1, 4, 1, w-1-4-2);
    textbox_neighbours *next_button2 = make_neighbours(1, -1, 1, -1);
    textbox_button *info_button2 = make_button("edit", OPEN_KEYBINDINGS, next_button2);
    elems[3] = make_element(BUTTON_ID, pos_button2, info_button2);

    return make_textbox(pos, elems, ELEM_CNT, 3, SETTINGS_MENU_ID);
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
    elems[0] = make_element(TEXT_ID, pos_text1, info_text1);

    sprintf(text, "GAME OVER");
    int text2_x = (w-(int)strlen(text))/2;
    size_info *pos_text2 = make_size_info(1, (int)strlen(text), midh-3, text2_x);
    textbox_text *info_text2 = make_text(text);
    elems[1] = make_element(TEXT_ID, pos_text2, info_text2);

    sprintf(text, "%.2f s", board->counters->total_time_elapsed / 1000000.0);
    int text3_x = (w-(int)strlen(text))/2;
    size_info *pos_text3 = make_size_info(1, (int)strlen(text), midh-1, text3_x);
    textbox_text *info_text3 = make_text(text);
    elems[2] = make_element(TEXT_ID, pos_text3, info_text3);

    sprintf(text, "Score: %d", board->counters->score);
    int text4_x = (w-(int)strlen(text))/2;
    size_info *pos_text4 = make_size_info(1, (int)strlen(text), midh, text4_x);
    textbox_text *info_text4 = make_text(text);
    elems[3] = make_element(TEXT_ID, pos_text4, info_text4);
    
    size_info *pos_button1 = make_size_info(1, 4, h-2, w-1-4);
    textbox_neighbours *next_button1 = make_neighbours(-1, -1, -1, -1);
    textbox_button *info_button1 = make_button("back", CLOSE_MENU, next_button1);
    elems[4] = make_element(BUTTON_ID, pos_button1, info_button1);

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
        elems[i*2] = make_element(TEXT_ID, pos_bind, info_text_bind);

        size_info *pos_button_bind = make_size_info(1, 8, 1+i, w-1-9);
        int up = 2*i+1 -2;
        int down = 2*i+1 +2;
        if (i == 0) up = bind_elems+1;
        if (i == TOTAL_BINDS-1) down = bind_elems+1;
        textbox_neighbours *next_button_bind = make_neighbours(up, -1, down, -1);
        textbox_keybind_select *info_keybind = make_keybind_select("...", i, next_button_bind); // TODO: link trigger
        elems[2*i+1] = make_element(KEYBIND_SELECT_ID, pos_button_bind, info_keybind);
    }

    size_info *pos_text1 = make_size_info(1, 11, 0, 2);
    textbox_text *info_text1 = make_text("Keybindings");
    elems[bind_elems+0] = make_element(TEXT_ID, pos_text1, info_text1);

    size_info *pos_button1 = make_size_info(1, 6, h-2, w-1-6-1);
    textbox_neighbours *next_button1 = make_neighbours(bind_elems-1, bind_elems+3, 1, bind_elems+2);
    textbox_button *info_button1 = make_button("cancel", CLOSE_KEYBINDINGS, next_button1);
    elems[bind_elems+1] = make_element(BUTTON_ID, pos_button1, info_button1);

    size_info *pos_button2 = make_size_info(1, 4, h-2, w-1-6-1-6);
    textbox_neighbours *next_button2 = make_neighbours(bind_elems-1, bind_elems+1, 1, bind_elems+3);
    textbox_button *info_button2 = make_button("save", SAVE_KEYBINDINGS, next_button2);
    elems[bind_elems+2] = make_element(BUTTON_ID, pos_button2, info_button2);

    size_info *pos_button3 = make_size_info(1, 9, h-2, w-1-4-1-6-12);
    textbox_neighbours *next_button3 = make_neighbours(bind_elems-1, bind_elems+2, 1, bind_elems+1);
    textbox_button *info_button3 = make_button("defaults", DEFAULT_KEYBINDINGS, next_button3);
    elems[bind_elems+3] = make_element(BUTTON_ID, pos_button3, info_button3);

    return make_textbox(pos, elems, ELEM_CNT, 1, KEYBINDINGS_MENU_ID);
}

// tries to open <new_menu>, returns true on success, false on failure
bool open_menu(menu_manager *manager, textbox *new_menu) {
    if (manager->top == manager->max_stack) return false;
    manager->top++;
    manager->stack[manager->top] = new_menu;
    return true;
}

// pops the top textbox in menu manager
void pop_menu_stack(menu_manager *manager) {
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
        if (stack[manager->top]->id != KEYBINDINGS_MENU_ID) { // disable default back in keybindings
            // close menu and go back one layer
            pop_menu_stack(manager);
        }
    }

    if (ret > 0) return ret;
    return 0;
}

void close_keybindings(menu_manager *manager) {
    load_binds();
    pop_menu_stack(manager);
}

// returns signals for main gameloop
// returns 1 to start game
int manage_menus(menu_manager *manager, int user_input) {
    int update_result = update_menus(manager, user_input);

    int ret = 0;
    // what each button trigger val does
    switch(update_result) {
        case START_GAME:
            ret = 1;
            break;
        case OPEN_SETTINGS:
            open_menu(manager, make_settings_menu());
            break;
        case OPEN_KEYBINDINGS:
            open_menu(manager, make_keybind_menu());
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
    }
    return ret;
}
#include "textbox.h"
#include "menu_maker.h"
#include <stdlib.h>

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
    int h = 4;
    int x = (COLS-w)/2;
    int y = (LINES-h)/2;

    size_info *pos = make_size_info(h, w, y, x);
    int ELEM_CNT = 3;
    textbox_element **elems = malloc(ELEM_CNT*sizeof(textbox_element*));

    size_info *pos_text1 = make_size_info(1, 9, 0, 2);
    char* text1 = "Main menu";
    textbox_text *info_text1 = make_text(text1);
    elems[0] = make_element(TEXT_ID, pos_text1, info_text1);

    size_info *pos_button1 = make_size_info(1, 18, 1, 1);
    textbox_neighbours *next_button1 = make_neighbours(-1, -1, 2, -1);
    textbox_button *info_button1 = make_button("       play       ", 1, next_button1);
    elems[1] = make_element(BUTTON_ID, pos_button1, info_button1);

    size_info *pos_button2 = make_size_info(1, 18, 2, 1);
    textbox_neighbours *next_button2 = make_neighbours(1, -1, -1, -1);
    textbox_button *info_button2 = make_button("     settings     ", 2, next_button2);
    elems[2] = make_element(BUTTON_ID, pos_button2, info_button2);

    return make_textbox(pos, elems, ELEM_CNT, 1);
}

textbox *make_settings_menu() {
    int w = 40;
    int h = 20;
    int x = (COLS-w)/2;
    int y = (LINES-h)/2;

    size_info *pos = make_size_info(h, w, y, x);
    int ELEM_CNT = 2;
    textbox_element **elems = malloc(ELEM_CNT*sizeof(textbox_element*));

    size_info *pos_text1 = make_size_info(1, 9, 0, 2);
    textbox_text *info_text1 = make_text("Settings");
    elems[0] = make_element(TEXT_ID, pos_text1, info_text1);

    size_info *pos_button1 = make_size_info(1, 4, h-2, w-1-4);
    textbox_neighbours *next_button1 = make_neighbours(-1, -1, -1, -1);
    textbox_button *info_button1 = make_button("back", -1, next_button1);
    elems[1] = make_element(BUTTON_ID, pos_button1, info_button1);

    return make_textbox(pos, elems, ELEM_CNT, 1);
}

// tries to open <new_menu>, returns true on success, false of failure
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
    if (ret == -1 && top > 0) {
        // go back one layer
        pop_menu_stack(manager);
    }

    if (ret > 0) return ret;
    return 0;
}

// returns signals for main gameloop
// returns 1 to start game
int manage_menus(menu_manager *manager, int user_input) {
    int update_result = update_menus(manager, user_input);

    int ret = 0;
    // what each button trigger val does
    switch(update_result) {
        case 1:
            ret = 1;
            break;
        case 2:
            open_menu(manager, make_settings_menu());
            break;
    }
    return ret;
}
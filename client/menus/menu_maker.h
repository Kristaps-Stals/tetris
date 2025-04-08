#pragma once
#include "textbox.h"

typedef struct menu_manager {
    textbox **stack;
    int top, max_stack;
} menu_manager;

menu_manager *make_menu_manager();
void free_menu_manager(menu_manager *manager);
textbox *make_main_menu();
textbox *make_settings_menu();
int update_menus(menu_manager *manager, int user_input);
int manage_menus(menu_manager *manager, int user_input);
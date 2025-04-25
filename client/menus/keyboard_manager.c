#include <ncurses.h>
#include <stdlib.h>
#include "keyboard_manager.h"
#include <fcntl.h>
#include <unistd.h>

keyboard_binds *binds;

void set_default_binds() {
    binds->game_left = KEY_LEFT;
    binds->game_right = KEY_RIGHT;
    binds->game_softdrop = KEY_DOWN;
    binds->game_harddrop = ' ';
    binds->game_rotate_left = 'z';
    binds->game_rotate_right = 'x';
    binds->game_hold = 'c';
    binds->menu_up = KEY_UP;
    binds->menu_right = KEY_RIGHT;
    binds->menu_down = KEY_DOWN;
    binds->menu_left = KEY_LEFT;
    binds->menu_select = KEY_ENTER;
    binds->menu_back = 'x';
}

void load_binds_from_file(char *file_path){
    int fd = open(file_path, O_WRONLY | O_CREAT);
    if (!fd) return;
    write(fd, "test", 4);
    close(fd);
}

void load_binds() {
    if (binds != NULL) free(binds);
    binds = malloc(sizeof(keyboard_binds));
    set_default_binds(binds);
    load_binds_from_file("config/keyboard_binds.config");
}

void save_binds() {

}

int get_keyboard_button(int action) {
    switch (action) {
        case GAME_LEFT:
            return binds->game_left;
        case GAME_RIGHT:
            return binds->game_right;
        case GAME_SOFTDROP:
            return binds->game_softdrop;
        case GAME_HARDDROP:
            return binds->game_harddrop;
        case GAME_ROTATE_LEFT:
            return binds->game_rotate_left;
        case GAME_ROTATE_RIGHT:
            return binds->game_rotate_right;
        case GAME_HOLD:
            return binds->game_hold;
        
        case MENU_UP:
            return binds->menu_up;
        case MENU_RIGHT:
            return binds->menu_right;
        case MENU_DOWN:
            return binds->menu_down;
        case MENU_LEFT:
            return binds->menu_left;
        case MENU_SELECT:
            return binds->menu_select;
        case MENU_BACK:
            return binds->menu_back;
    }
    return 0;
}

void set_keyboard_button(int action, int new_button) {
    switch (action) {
        case GAME_LEFT:
            binds->game_left = new_button;
            break;
        case GAME_RIGHT:
            binds->game_right = new_button;
            break;
        case GAME_SOFTDROP:
            binds->game_softdrop = new_button;
            break;
        case GAME_HARDDROP:
            binds->game_harddrop = new_button;
            break;
        case GAME_ROTATE_LEFT:
            binds->game_rotate_left = new_button;
            break;
        case GAME_ROTATE_RIGHT:
            binds->game_rotate_right = new_button;
            break;
        case GAME_HOLD:
            binds->game_hold = new_button;
            break;
        
        case MENU_UP:
            binds->menu_up = new_button;
            break;
        case MENU_RIGHT:
            binds->menu_right = new_button;
            break;
        case MENU_DOWN:
            binds->menu_down = new_button;
            break;
        case MENU_LEFT:
            binds->menu_left = new_button;
            break;
        case MENU_SELECT:
            binds->menu_select = new_button;
            break;
        case MENU_BACK:
            binds->menu_back = new_button;
            break;
    }   
}
#include <ncurses.h>
#include <stdlib.h>
#include "keyboard_manager.h"
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include "../../shared/kstring.h"

keyboard_bind *binds;
const char* keyboard_config_file = "config/keyboard.config";

void set_default_binds() {
    binds[GAME_LEFT] = (keyboard_bind){KEY_LEFT, "Tetris left", "game_left"};
    binds[GAME_RIGHT] = (keyboard_bind){KEY_RIGHT, "Tetris right", "game_right"};
    binds[GAME_SOFTDROP] = (keyboard_bind){KEY_DOWN, "Soft drop", "game_softdrop"};
    binds[GAME_HARDDROP] = (keyboard_bind){' ', "Hard drop", "game_harddrop"};
    binds[GAME_ROTATE_LEFT] = (keyboard_bind){'z', "Rotate left", "game_rotate_left"};
    binds[GAME_ROTATE_RIGHT] = (keyboard_bind){'x', "Rotate right", "game_rotate_right"};
    binds[GAME_HOLD] = (keyboard_bind){'c', "Hold piece", "game_hold"};
    binds[MENU_UP] = (keyboard_bind){KEY_UP, "Menu up", "menu_up"};
    binds[MENU_RIGHT] = (keyboard_bind){KEY_RIGHT, "Menu right", "menu_right"};
    binds[MENU_DOWN] = (keyboard_bind){KEY_DOWN, "Menu down", "menu_down"};
    binds[MENU_LEFT] = (keyboard_bind){KEY_LEFT, "Menu left", "menu_left"};
    binds[MENU_SELECT] = (keyboard_bind){10, "Menu select", "menu_select"};
    binds[MENU_SELECT2] = (keyboard_bind){'z', "Menu select 2", "menu_select2"};
    binds[MENU_BACK] = (keyboard_bind){'x', "Menu back", "menu_back"};
}

char* parse_bind_line(char* s, int *num) {
    char* tmp = s;
    int len = 1; // zero byte at the end
    while (
        (*tmp <= 'Z' && *tmp >= 'A') || 
        (*tmp <= 'z' && *tmp >= 'a') || 
        (*tmp <= '9' && *tmp >= '0') ||
        *tmp == '_'
    ) {
        tmp++;
        len++;
    }
    char* ret = malloc(len*sizeof(char));
    for (int i = 0; i < len-1; i++) {
        ret[i] = s[i];
    }
    ret[len-1] = 0;
    (*num) = 0;
    if (*tmp == 0) return ret;
    tmp++;
    *num = char_to_num(tmp);
    return ret;
}

void process_keyboard_config_line(char* s) {
    int num;
    char* first_word = parse_bind_line(s, &num);
    for (int i = 0; i < TOTAL_BINDS; i++) {
        if (char_cmp(first_word, binds[i].config_name) == 0) {
            binds[i].button = num;
        }
    }
    free(first_word);
}

void load_binds_from_file(){
    FILE* f = fopen(keyboard_config_file, "r");
    if (!f) return;
    char buf[105];
    while (fgets(buf, 100, f)) {
        process_keyboard_config_line(buf);
    }
    fclose(f);
}

// loads binds from file
void load_binds() {
    set_default_binds(binds);
    load_binds_from_file();
}

// saves current binds into file
void save_binds() {
    FILE *f = fopen(keyboard_config_file, "w");
    if (!f) return;
    for (int i = 0; i < TOTAL_BINDS; i++) {
        fprintf(f, "%s:%d\n", binds[i].config_name, binds[i].button);
    }
    fclose(f);
}

void init_binds() {
    binds = malloc(TOTAL_BINDS*sizeof(keyboard_bind));
    load_binds();
    save_binds();
}

int get_keyboard_button(int action) {
    assert(action >= 0 && action < TOTAL_BINDS);
    return binds[action].button;
}

void set_keyboard_button(int action, int new_button) {
    assert(action >= 0 && action < TOTAL_BINDS);
    binds[action].button = new_button;
}

keyboard_bind get_bind(int action) {
    assert(action >= 0 && action < TOTAL_BINDS);
    return binds[action];
}
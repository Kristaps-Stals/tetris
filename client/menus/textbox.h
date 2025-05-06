#pragma once
#include<ncurses.h>

enum {
    TEXT_ID = 0,
    BUTTON_ID = 1,
    KEYBIND_SELECT_ID = 2,
    WRITE_ELEMENT_ID = 3
};

enum {
    START_EDITING = 5000,
    STOP_EDITING = 5001
};

typedef struct size_info {
    int h, w, y, x;
} size_info;

typedef struct textbox_text {
    char *text;
    int text_len;
} textbox_text;

typedef struct textbox_neighbours {
    int up, right, down, left;
} textbox_neighbours;

typedef struct textbox_button {
    char *text;
    int text_len, trigger_val;
} textbox_button;

typedef struct textbox_keybind_select {
    char *text_normal, *text_editing;
    int text_normal_len, text_editing_len, keybind_id;
    bool is_editing;
} textbox_keybind_select;

typedef struct textbox_write {
    char* text;
    int max_len, curr_len, write_id;
    bool is_editing;
} textbox_write;

typedef struct textbox_element {
    int type;
    bool visible;
    void *info;
    size_info *pos;
    textbox_neighbours *neighbour; // which element to go to if active element
    attr_t attributes;
} textbox_element;

typedef struct textbox {
    int id;
    WINDOW *win;
    size_info *pos; // window position
    textbox_element **elements;
    int element_count;
    int element_selected; // currently active element
} textbox;

size_info *make_size_info(int h, int w, int y, int x);

textbox_neighbours *make_neighbours(int up, int right, int down, int left);

textbox_text *make_text(char *text);

textbox_button *make_button(char *text, int trigger_val);

textbox_keybind_select *make_keybind_select(char *text_editing, int keybind_id);

textbox_write *make_write_elem(char* default_text, int max_len, int write_id);

textbox_element *make_element(int type, size_info *pos, void *element_info, textbox_neighbours *neighbours, attr_t attributes);

textbox *make_textbox(size_info *pos, textbox_element **element_list, int element_count, int default_element, int id);
void free_textbox(textbox *box);
void draw_textbox(textbox *tbox);
int update_textbox(textbox *tbox, int user_input);
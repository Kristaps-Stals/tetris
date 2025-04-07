#pragma once
#include<ncurses.h>

enum {
    TEXT_ID = 0,
    BUTTON_ID = 1
};

typedef struct size_info {
    int h, w, y, x;
} size_info;

typedef struct textbox_text {
    char* text;
    int text_len;
} textbox_text;

typedef struct textbox_neighbours {
    int up, right, down, left;
} textbox_neighbours;

typedef struct textbox_button {
    char* text;
    int text_len, trigger_val;
    textbox_neighbours *neighbour; // which element to go to if active element
} textbox_button;

typedef struct textbox_element {
    int type;
    void* info;
    size_info *pos;
} textbox_element;

typedef struct textbox {
    WINDOW *win;
    size_info *pos; // window position
    textbox_element **elements;
    int element_count;
    int element_selected; // currently active element
} textbox;

size_info *make_size_info(int h, int w, int y, int x);

textbox_neighbours *make_neighbours(int up, int right, int down, int left);

textbox_text *make_text(char* text);
void free_text(textbox_text *text);

textbox_button *make_button(char* text, int trigger_val, textbox_neighbours *neighbours);
void free_button(textbox_button *button);

textbox_element *make_element(int type, size_info *pos, void* element_info);
void free_element(textbox_element *elem);

textbox *make_textbox(size_info *pos, textbox_element **element_list, int element_count, int default_element);
void free_textbox(textbox *box);

void draw_textbox(textbox *tbox);
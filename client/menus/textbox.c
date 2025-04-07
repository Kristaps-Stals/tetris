#include <ncurses.h>
#include <stdlib.h>
#include "textbox.h"

int char_len(char* s) {
    int x = 0;
    while (*s != 0) {
        x++;
        s++;
    }
    return x;
}

size_info *make_size_info(int h, int w, int y, int x) {
    size_info *info = malloc(sizeof(size_info));
    info->h = h;
    info->w = w;
    info->y = y;
    info->x = x;
    return info;
}
// default free

textbox_neighbours *make_neighbours(int up, int right, int down, int left) {
    textbox_neighbours *neighbours = malloc(sizeof(textbox_neighbours));
    neighbours->up = up;
    neighbours->right = right;
    neighbours->down = down;
    neighbours->left = left;
    return neighbours;
}
// default free

textbox_text *make_text(char* text) {
    textbox_text *ret = malloc(sizeof(textbox_text));
    ret->text = text;
    ret->text_len = char_len(text);
    return ret;
}
void free_text(textbox_text *text) {
    free(text->text);
    free(text);
}

textbox_button *make_button(char* text, int trigger_val, textbox_neighbours *neighbours) {
    textbox_button *button = malloc(sizeof(textbox_button));
    button->text = text;
    button->text_len = char_len(text);
    button->neighbour = neighbours;
    button->trigger_val = trigger_val;
    return button;
}
void free_button(textbox_button *button) {
    free(button->text);
    free(button->neighbour);
    free(button);
}

textbox_element *make_element(int type, size_info *pos, void* element_info) {
    textbox_element *elem = malloc(sizeof(textbox_element));
    elem->type = type;
    elem->pos = pos;
    elem->info = element_info;
    return elem;
}
void free_element(textbox_element *elem) {
    switch(elem->type) {
        case TEXT_ID:
            free_text(elem->info);
            break;
        case BUTTON_ID:
            free_button(elem->info);
            break;
    }
    free(elem->pos);
    free(elem);
}

textbox *make_textbox(size_info *pos, textbox_element **element_list, int element_count, int default_element) {
    textbox *box = malloc(sizeof(textbox));
    box->pos = pos;
    box->elements = element_list;
    box->element_count = element_count;
    box->win = newwin(pos->h, pos->w, pos->y, pos->x);
    box->element_selected = default_element;
    return box;
}
void free_textbox(textbox *box) {
    free(box->pos);
    for (int i = 0; i < box->element_count; i++) {
        free_element(box->elements[i]);
    }
    free(box);
}

void draw_text(WINDOW *win, textbox_element *element, int is_selected) {
    size_info *pos = element->pos;
    textbox_text *info = element->info;
    char *text = info->text;
    for (int i = 0; i < pos->h; i++) {
        for (int j = 0; j < pos->w; j++) {
            if (*text == 0) break;
            mvwaddch(win, pos->y+i, pos->x+j, *text);
            text++;
        }
        if (*text == 0) break;
    }
}

void draw_button(WINDOW *win, textbox_element *element, int is_selected) {
    size_info *pos = element->pos;
    textbox_button *info = element->info;
    char *text = info->text;
    for (int i = 0; i < pos->h; i++) {
        for (int j = 0; j < pos->w; j++) {
            if (*text == 0) break;
            if (is_selected) {
                mvwaddch(win, pos->y+i, pos->x+j, *text | A_REVERSE);
            } else {
                mvwaddch(win, pos->y+i, pos->x+j, *text);
            }
            text++;
        }
        if (*text == 0) break;
    }
}

void draw_element(WINDOW *win, textbox_element *element, int is_selected) {
    switch(element->type) {
        case TEXT_ID:
            draw_text(win, element, is_selected);
            break;
        case BUTTON_ID:
            draw_button(win, element, is_selected);
            break;
    }
}

void draw_textbox(textbox *tbox) {
    werase(tbox->win);
    // draw border
    wborder(tbox->win, 0, 0, 0, 0, 0, 0, 0, 0);
    
    // draw elements
    for (int i = 0; i < tbox->element_count; i++) {
        bool is_selected = false;
        if (tbox->element_selected == i) is_selected = true;
        draw_element(tbox->win, tbox->elements[i], is_selected);
    }

    wrefresh(tbox->win);
}
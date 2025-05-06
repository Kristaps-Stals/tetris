#define _XOPEN_SOURCE 700 // dont know what it does, but without it sigaction highlighting doesnt work
#include <ncurses.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "textbox.h"
#include "menu_maker.h"
#include "../shared/kstring.h"
#include "keyboard_manager.h"

// EXTRAS
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

// returns 1 if selected is updated
int update_selected(textbox *tbox, int user_input) {
    int at = tbox->element_selected;
    int cnt = 0;
    while (1) {
        textbox_neighbours *next = tbox->elements[at]->neighbour;
        if (next == NULL) break;
        if (user_input == get_keyboard_button(MENU_UP)) {
            if (next->up >= 0) at = next->up;
        }
        if (user_input == get_keyboard_button(MENU_RIGHT)) {
            if (next->right >= 0) at = next->right;
        }
        if (user_input == get_keyboard_button(MENU_DOWN)) {
            if (next->down >= 0) at = next->down;
        }
        if (user_input == get_keyboard_button(MENU_LEFT)) {
            if (next->left >= 0) at = next->left;
        }
        if (tbox->elements[at]->visible) break; // if element is visible stop
        // keep going until at a visible element
        cnt++;
        if (cnt > 1000) {
            // mvprintw(0, 0, "INFINITE LOOP IN TEXTBOX ELEMENTS (INVISIBILITY SKIPS ELEMENTS)");
            // refresh();
            at = tbox->element_selected;
            break;
        }
    }

    int ret = 0;
    if (at != tbox->element_selected) ret = 1;
    tbox->element_selected = at;
    return ret;
}

// returns true if user has pressed menu select
bool is_menu_select_pressed(int user_input) {
    if (
        user_input == get_keyboard_button(MENU_SELECT) || 
        user_input == get_keyboard_button(MENU_SELECT2)
    ) {
        return true;
    } else {
        return false;
    }
}

// TEXT
textbox_text *make_text(char *text) {
    textbox_text *ret = malloc(sizeof(textbox_text));
    ret->text = copy_text(text);
    ret->text_len = char_len(text);
    return ret;
}
void free_text(textbox_text *text) {
    free(text->text);
    free(text);
}
void draw_text(WINDOW *win, textbox_element *element) {
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

// BUTTON
textbox_button *make_button(char *text, int trigger_val) {
    textbox_button *button = malloc(sizeof(textbox_button));
    button->text = copy_text(text);
    button->text_len = char_len(text);
    button->trigger_val = trigger_val;
    return button;
}
void free_button(textbox_button *button) {
    free(button->text);
    free(button);
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
int update_handle_button(textbox *tbox, int user_input) {
    textbox_element *selected_elem = tbox->elements[tbox->element_selected];
    textbox_button *info = selected_elem->info;

    update_selected(tbox, user_input);

    if (is_menu_select_pressed(user_input)) {
        return info->trigger_val;
    } else {
        return 0;
    }
}

// KEYBIND_SELECT
void update_keybind_text(textbox_keybind_select *keybind_select) {
    int keybind_id = keybind_select->keybind_id;
    keyboard_bind bind = get_bind(keybind_id);
    char buf[16];
    char c = ' ';
    if (bind.button >= 21 && bind.button <= 255) c = bind.button;
    sprintf(buf, "%c (%d)", c, bind.button);
    if (keybind_select->text_normal != NULL) free(keybind_select->text_normal);
    keybind_select->text_normal = copy_text(buf);
    keybind_select->text_normal_len = char_len(buf);
}
textbox_keybind_select *make_keybind_select(char *text_editing, int keybind_id) {
    textbox_keybind_select *keybind_select = malloc(sizeof(textbox_keybind_select));
    keybind_select->text_editing = copy_text(text_editing);
    keybind_select->text_editing_len = char_len(text_editing);
    keybind_select->is_editing = false;
    keybind_select->keybind_id = keybind_id;

    keybind_select->text_normal = NULL;
    update_keybind_text(keybind_select);

    return keybind_select;
}
void free_keybind_select(textbox_keybind_select *keybind_select) {
    free(keybind_select->text_normal);
    free(keybind_select->text_editing);
    free(keybind_select);
}
void draw_keybind_select(WINDOW *win, textbox_element *element, int is_selected) {
    size_info *pos = element->pos;
    textbox_keybind_select *info = element->info;
    update_keybind_text(info); // not optimal to update every frame, but it is convienent :3
    char *text = info->text_normal;
    if (info->is_editing) text = info->text_editing;

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
int update_handle_keybind_select(textbox *tbox, int user_input) {
    textbox_element *selected_elem = tbox->elements[tbox->element_selected];
    textbox_keybind_select *info = selected_elem->info;

    if (info->is_editing) {
        if (user_input == -1) return 0;
        set_keyboard_button(info->keybind_id, user_input);
        info->is_editing = false;
        return 0;
    }

    update_selected(tbox, user_input);

    if (is_menu_select_pressed(user_input)) {
        info->is_editing = true;
    }

    return 0;
}

// WRITING ELEMENT
textbox_write *make_write_elem(char* default_text, int max_len, int write_id) {
    textbox_write *write_elem = malloc(sizeof(textbox_write));

    write_elem->text = calloc(max_len+1, sizeof(char));
    write_elem->curr_len = char_len(default_text);
    assert(write_elem->curr_len <= max_len);
    char_copy_char(default_text, write_elem->text);

    write_elem->max_len = max_len;
    write_elem->write_id = write_id;
    write_elem->is_editing = false;
    return write_elem;
}
void free_write_elem(textbox_write *write_elem) {
    free(write_elem->text);
    free(write_elem);
}
void draw_write_elem(WINDOW *win, textbox_element *element, int is_selected) {
    size_info *pos = element->pos;
    textbox_write *info = element->info;
    char *text = info->text;

    for (int i = 0; i < pos->h; i++) for (int j = 0; j < pos->w; j++) {
        if (is_selected) mvwaddch(win, pos->y+i, pos->x+j, ' ' | A_REVERSE);
    }

    int len = info->curr_len;

    int at_char = len-1;
    int cursor_pos = len;
    if (cursor_pos > pos->w-1) cursor_pos = pos->w-1;

    int start_idx = len-1;
    int right_space = 1;
    if (info->is_editing && info->curr_len != info->max_len) right_space = 2;
    if (start_idx > pos->w-right_space) start_idx = pos->w-right_space;

    for (int i = start_idx; i >= 0; i--) {
        if (at_char < 0) break;
        if (is_selected) {
            mvwaddch(win, pos->y, pos->x+i, text[at_char] | A_REVERSE);
        } else {
            mvwaddch(win, pos->y, pos->x+i, text[at_char]);
        }
        at_char--;
    }

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    int ms = now.tv_nsec/1e6;
    int phase = 0;
    if (ms >= 500) phase = 1;
    if (info->is_editing && info->curr_len != info->max_len) {
        if (phase == 0) {
            mvwaddch(win, pos->y, pos->x+cursor_pos, ' ' | A_REVERSE);
        }
        if (phase == 1) {
            mvwaddch(win, pos->y, pos->x+cursor_pos, ' ');
        }
    }
}
int update_write_elem(textbox *tbox, int user_input) {
    textbox_element *selected_elem = tbox->elements[tbox->element_selected];
    textbox_write *info = selected_elem->info;

    if (info->is_editing) {
        if (user_input == 10 || user_input == 27) { // enter or escape
            info->is_editing = false;
            return STOP_EDITING;
        }
        if (user_input == 263 || user_input == 260) { // backspace or left arrow
            if (info->curr_len <= 0) return 0;
            info->curr_len--;
            info->text[info->curr_len] = 0;
            return 0;
        }
        if (user_input >= 32 && user_input <= 255) {
            if (info->curr_len >= info->max_len) return 0;
            info->text[info->curr_len] = user_input;
            info->curr_len++;
            return 0;
        }
        return 0;
    }

    if (update_selected(tbox, user_input)) {
        return 0;
    }

    if (is_menu_select_pressed(user_input)) {
        info->is_editing = true;
        return START_EDITING;
    }

    return 0;
}

// ELEMENT
// pass NULL to neighbours if dont want any
textbox_element *make_element(int type, size_info *pos, void* element_info, textbox_neighbours *neighbours) {
    textbox_element *elem = malloc(sizeof(textbox_element));
    elem->type = type;
    elem->pos = pos;
    elem->info = element_info;
    elem->visible = true;
    if (neighbours == NULL) {
        elem->neighbour = make_neighbours(-1, -1, -1, -1);
    } else {
        elem->neighbour = neighbours;
    }
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
        case KEYBIND_SELECT_ID:
            free_keybind_select(elem->info);
            break;
        case WRITE_ELEMENT_ID:
            free_write_elem(elem->info);
            break;
    }
    free(elem->pos);
    free(elem->neighbour);
    free(elem);
}
void draw_element(WINDOW *win, textbox_element *element, int is_selected) {
    if (element->visible == false) return;
    switch(element->type) {
        case TEXT_ID:
            draw_text(win, element);
            break;
        case BUTTON_ID:
            draw_button(win, element, is_selected);
            break;
        case KEYBIND_SELECT_ID:
            draw_keybind_select(win, element, is_selected);
            break;
        case WRITE_ELEMENT_ID:
            draw_write_elem(win, element, is_selected);
            break;
    }
}

// returns 0 on success,
// -1 if element out of range,
// -2 if element does not have text field (or is not supported),
// deep copies text, does not delete it
// currently only supports TEXT and BUTTON
int change_elem_text(menu_manager *menu_manager_, int elem_id, char *new_text) {
    textbox *tbox = menu_manager_->stack[menu_manager_->top];
    if (elem_id < 0 || elem_id >= tbox->element_count) return -1;
    textbox_element *elem = tbox->elements[elem_id];
    switch(elem->type) {
        case TEXT_ID:
            textbox_text *info_txt = elem->info;
            free(info_txt->text);
            info_txt->text = copy_text(new_text);       
            break;
        case BUTTON_ID:
            textbox_button *info_but = elem->info;
            free(info_but->text);
            info_but->text = copy_text(new_text);
            break;
    }
    return 0;
}

// returns 0 on success,
// -1 if element out of range
int change_elem_visibility(menu_manager *menu_manager_, int elem_id, bool visible) {
    textbox *tbox = menu_manager_->stack[menu_manager_->top];
    if (elem_id < 0 || elem_id >= tbox->element_count) return -1;
    tbox->elements[elem_id]->visible = visible;
    return 0;
}

// TEXTBOX
textbox *make_textbox(size_info *pos, textbox_element **element_list, int element_count, int default_element, int id) {
    textbox *box = malloc(sizeof(textbox));
    box->id = id;
    box->pos = pos;
    box->elements = element_list;
    box->element_count = element_count;
    box->win = newwin(pos->h, pos->w, pos->y, pos->x);
    box->element_selected = default_element;
    return box;
}
void free_textbox(textbox *box) {
    delwin(box->win);
    free(box->pos);
    for (int i = 0; i < box->element_count; i++) {
        free_element(box->elements[i]);
    }
    free(box->elements);
    free(box);
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
// returns any signals that are created from elements
int update_textbox(textbox *tbox, int user_input) {
    int ret_val = 0;

    // update elements (if needed)
    if (tbox->element_selected >= 0 && tbox->element_selected < tbox->element_count) {
        textbox_element *selected_elem = tbox->elements[tbox->element_selected];
        switch (selected_elem->type) {
            case BUTTON_ID:
                ret_val = update_handle_button(tbox, user_input);
                break;
            case KEYBIND_SELECT_ID:
                ret_val = update_handle_keybind_select(tbox, user_input);
                break;
            case WRITE_ELEMENT_ID:
                ret_val = update_write_elem(tbox, user_input);
                break;
        }
    }
    
    draw_textbox(tbox);
    return ret_val;
}
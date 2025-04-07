#include "textbox.h"
#include <stdlib.h>

textbox *make_main_menu() {
    int w = 20;
    int h = 4;
    int x = (COLS-w)/2;
    int y = (LINES-h)/2;

    size_info *pos = make_size_info(h, w, y, x);
    int ELEM_CNT = 3;
    textbox_element **elems = malloc(ELEM_CNT*sizeof(textbox_element*));

    size_info *pos_text1 = make_size_info(1, 9, 0, 1);
    textbox_text *info_text1 = make_text("Main menu");
    elems[0] = make_element(TEXT_ID, pos_text1, info_text1);

    size_info *pos_button1 = make_size_info(1, 18, 1, 1);
    textbox_neighbours *next_button1 = make_neighbours(-1, -1, 2, -1);
    textbox_button *info_button1 = make_button("       play       ", 1, next_button1);
    elems[1] = make_element(BUTTON_ID, pos_button1, info_button1);

    size_info *pos_button2 = make_size_info(1, 18, 2, 1);
    textbox_neighbours *next_button2 = make_neighbours(1, -1, -1, -1);
    textbox_button *info_button2 = make_button("     settings     ", 2, next_button2);
    elems[2] = make_element(BUTTON_ID, pos_button2, info_button2);

    textbox *test = make_textbox(pos, elems, ELEM_CNT, 1);
    free(pos);

    return test;
}
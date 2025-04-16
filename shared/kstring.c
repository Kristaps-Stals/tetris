#include<stdlib.h>

int char_len(char* s) {
    int x = 0;
    while (*s != 0) {
        x++;
        s++;
    }
    return x;
}

char* copy_text(char* s) {
    int siz = char_len(s);
    char* ret = malloc(siz+1);
    for (int i = 0; i < siz; i++) ret[i] = s[i];
    ret[siz] = 0;
    return ret;
}
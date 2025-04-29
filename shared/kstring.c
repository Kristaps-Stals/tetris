#include<stdlib.h>

int char_len(char *s) {
    int x = 0;
    while (*s != 0) {
        x++;
        s++;
    }
    return x;
}

char* copy_text(char *s) {
    int siz = char_len(s);
    char *ret = malloc(siz+1);
    for (int i = 0; i < siz; i++) ret[i] = s[i];
    ret[siz] = 0;
    return ret;
}

// make sure that <from> is not bigger than <to>
void char_copy_char(char *from, char *to) {
    while (1) {
        *to = *from;
        if (*from == 0) break;
        to++; from++;
    }
}

// numbers up to 10 digits
// can do negative
int char_to_num(char *s) {
    int num = 0;
    int mlt = 1;
    if (*s == '-') {
        mlt = -1;
        s++;
    }
    int len = 0;
    while (*s >= '0' && *s <= '9') {
        len++;
        s++;
    }
    while (len) {
        s--;
        len--;
        num += mlt*(*s-'0');
        mlt *= 10;
    }
    return num;
}

// if a is lexicographically smaller than b, return 1
// if a is bigger than b return -1
// if a is equal to b return 0
int char_cmp(char *a, char *b) {
    while (1) {
        if (*a == 0 && *b == 0) return 0;
        if (*a < *b) return 1;
        if (*a > *b) return -1;
        a++; b++;
    }
}
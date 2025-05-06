#include "settings.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

static char nickname[NICKNAME_MAX_LEN] = "Player";

static const char* settings_file = "config/settings.config";

void set_nickname(const char* new_nick) {
    strncpy(nickname, new_nick, NICKNAME_MAX_LEN-1);
    nickname[NICKNAME_MAX_LEN-1] = 0;
}

const char* get_nickname() {
    return nickname;
}

void load_settings() {
    FILE* f = fopen(settings_file, "r");
    if (!f) return;
    char buf[NICKNAME_MAX_LEN];
    if (fgets(buf, sizeof(buf), f)) {
        buf[strcspn(buf, "\r\n")] = 0;
        set_nickname(buf);
    }
    fclose(f);
}

void save_settings() {
    mkdir("config", 0777); // ensure config directory exists
    FILE* f = fopen(settings_file, "w");
    if (!f) return;
    fprintf(f, "%s\n", nickname);
    fclose(f);
}

void init_settings() {
    load_settings();
}

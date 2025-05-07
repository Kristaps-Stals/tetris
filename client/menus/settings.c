#include "settings.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

static char nickname[NICKNAME_MAX_LEN] = "Player";
static const char* settings_file = "config/settings.config";

void set_nickname(const char* new_nick) {
    strncpy(nickname, new_nick, NICKNAME_MAX_LEN - 1);
    nickname[NICKNAME_MAX_LEN - 1] = '\0';
}

const char* get_nickname() {
    return nickname;
}

void set_default_settings() {
    set_nickname("Player");
}

static void process_setting_line(char *line) {
    char *sep = strchr(line, ':');
    if (!sep) return;
    *sep = '\0';
    char *key = line;
    char *val = sep + 1;
    val[strcspn(val, "\r\n")] = '\0';
    if (strcmp(key, "nickname") == 0) {
        set_nickname(val);
    }
    // more settings here
}

void load_settings() {
    set_default_settings();
    FILE* f = fopen(settings_file, "r");
    if (!f) return;
    char buf[128];
    while (fgets(buf, sizeof buf, f)) {
        process_setting_line(buf);
    }
    fclose(f);
}

void save_settings() {
    mkdir("config", 0777);
    FILE* f = fopen(settings_file, "w");
    if (!f) return;
    fprintf(f, "nickname:%s\n", nickname);
    fclose(f);
}

void init_settings() {
    load_settings();
}

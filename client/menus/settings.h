#pragma once

#define NICKNAME_MAX_LEN 32

void init_settings();
void load_settings();
void save_settings();

void set_default_settings();

const char* get_nickname();
void set_nickname(const char* new_nick);

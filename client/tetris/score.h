#pragma once

typedef struct score_report {
    long long score;
    long long garbage;
    int lines_cleared;
    char* message;
} score_report;

void free_score_report(score_report *score_report);
score_report *update_clear_lines(void* board_);
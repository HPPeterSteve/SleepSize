#include "sleepsize.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
static void get_state_path(char *path, size_t size) {
    const char *homedir = getenv("HOME");
    if (!homedir) {
        struct passwd *pw = getpwuid(getuid());
        if (pw) homedir = pw->pw_dir;
    }
    if (homedir) {
        snprintf(path, size, "%s/.sleepsize.list", homedir);
    } else {
        snprintf(path, size, ".sleepsize.list");
    }
}
int sleep_state_load(char paths[SS_MAX_FILES][PATH_MAX]) {
    char state_file[PATH_MAX];
    get_state_path(state_file, sizeof(state_file));
    FILE *f = fopen(state_file, "r");
    if (!f) return 0;
    int count = 0;
    char line[PATH_MAX];
    while (fgets(line, sizeof(line), f) && count < SS_MAX_FILES) {
        line[strcspn(line, "\r\n")] = 0;
        if (line[0]) {
            strncpy(paths[count], line, PATH_MAX);
            count++;
        }
    }
    fclose(f);
    return count;
}
void sleep_state_save(char paths[SS_MAX_FILES][PATH_MAX], int count) {
    char state_file[PATH_MAX];
    get_state_path(state_file, sizeof(state_file));
    FILE *f = fopen(state_file, "w");
    if (!f) return;
    for (int i = 0; i < count; i++) {
        if (paths[i][0]) {
            fprintf(f, "%s\n", paths[i]);
        }
    }
    fclose(f);
}

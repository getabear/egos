/*
 * (C) 2025, Cornell University
 * All rights reserved.
 *
 * Description: a simple cd
 */

#include "app.h"
#include <string.h>

int main(int argc, char** argv) {
    if (argc == 1) {
        int home_ino = dir_lookup(0, "home/");
        // INFO("workdir = 0x%x", workdir);                 // [INFO] workdir = 0x80602004
        // INFO("workdir_ino = 0x%x", workdir_ino);         // [INFO] workdir_ino = 0x0
        workdir_ino  = dir_lookup(home_ino, "yunhao/");
        
        strcpy(workdir, "/home/yunhao");
        return 0;
    }

    /* Set the inode number to the new working directory */
    strcat(argv[1], "/");
    int dir_ino = dir_lookup(workdir_ino, argv[1]);
    if (dir_ino == -1) {
        INFO("cd: directory %s not found", argv[1]);
        return -1;
    }

    workdir_ino = dir_ino;

    /* Set the path name to the new working directory */
    if (!strcmp("./", argv[1])) return 0;

    uint len = strlen(workdir);
    if (strcmp("../", argv[1])) {
        if (len > 1) strcat(workdir, "/");
        strncat(workdir, argv[1], strlen(argv[1]) - 1);
    } else {
        while (workdir[len] != '/') workdir[len--] = 0;
        if (len) workdir[len] = 0;
    }

    return 0;
}

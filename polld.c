/*
 * Polling daemon
 *
 * Copyright (c) 2004 by Michal Cihar <michal@cihar.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * In addition to GNU GPL this code may be used also in non GPL programs but
 * if and only if programmer/distributor of that code receives written
 * permission from author of this code.
 *
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

char        defaultconfig[] = "/etc/polld";
char        *config;
char        **filelist = NULL;
char        *err;
int         listlen = 0;
int         alloclen = 0;
FILE        *file;
char        *line = NULL;
size_t      len = 0;
int         i;

int main(int argc, char **argv) {

    /* First parameter is optionally config file name */
    if (argc == 1) {
        config = defaultconfig;
    } else {
        config = argv[1];
    }

    /* Open configuration */
    file = fopen(config, "r");
    if (file == NULL) {
        err = strerror(errno);
        fprintf(stderr, "polld: failed to open configuration %s (%s)\n", config, err);
        exit(1);
    }

    /* Read config file lines */
    while (getline(&line, &len, file) != -1) {
        /* Ignore comments */
        if (line[0] == '#') continue;

        len = strlen(line);

        /* Remove trailing \n */
        if (len > 0 && line[len - 1] == '\n') {
            len[line - 1] = 0;
            len--;
        }

        /* Ignore empty lines */
        if (len == 0) {
            free(line);
            line = NULL;
            continue;
        }

        /* Okay, we seem to have valid line */
        listlen++;

        /* Do we need to reallocate list? */
        if (listlen > alloclen) {
            filelist = (char **)realloc(filelist, (listlen + 4 ) * sizeof(char *));
            if (filelist == NULL) {
                fprintf(stderr, "polld: not enough memory\n");
                exit(2);
            }
            alloclen = listlen + 4;
        }

        /* Add to list */
        filelist[listlen - 1] = line;

        /* Zero to force getline to malloc this for us */
        line = NULL;
        len = 0;
    }

    /* Close config file */
    fclose(file);

    /* Disconnect terminal.. */
    daemon(0, 0);

    /* Write pid file (be quiet on error) */
    file = fopen("/var/run/polld.pid", "w");
    if (file != NULL) {
        fprintf(file, "%d\n", getpid());
        fclose(file);
    }

    /* Main loop */
    while (1) {
        sleep(10);
        for (i = 0; i < listlen; i++) {
            file = fopen(filelist[i], "r");
            if (file != NULL) fclose(file);
        }
    }

    exit(0);
}

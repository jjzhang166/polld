/*
 * Polling daemon
 *
 * Copyright (c) 2004-2006 by Michal Čihař <michal@cihar.com>
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
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>

/* These are usually defined by make: */
#ifndef CONFIGFILE
#   define CONFIGFILE "/etc/polld"
#endif
#ifndef PIDFILE
#   define PIDFILE "/var/run/polld.pid"
#endif
#ifndef SLEEPTIME
#   define SLEEPTIME 10
#endif

/* Commandline params */
char        *config = NULL;
char        *pid = NULL;
int         sleeptime = SLEEPTIME;

/* Parameters */
char        **filelist = NULL;
int         listlen = 0;
int         alloclen = 0;

#define ERRORPREFIX "polld: "
#define VERSION "0.1"

void show_version(void) {
    printf("polld version " VERSION "\n"
            "Copyright (C) 2004-2006 Michal Cihar <michal@cihar.com>\n"
            "This is free software.  You may redistribute copies of it under the terms of\n"
            "the GNU General Public License <http://www.gnu.org/licenses/gpl.html>.\n"
            "There is NO WARRANTY, to the extent permitted by law.\n");
    exit(0);
}

void show_help(void) {
    printf( "Usage: polld [params]\n"
            "Parameters\n"
            " -h --help             show this help\n"
            " -v --version          show version information\n"
            " -c --config file      configuration file to use\n"
            " -p --pid file         lock file for storing pid\n"
            " -s --sleep seconds    number of seconds to sleep in sleep cycle\n"
            "\n");
    exit(0);
}

void parse_params(int argc, char **argv) {
    struct option long_options[] = {
       {"help"      , 0, 0, 'h'},
       {"version"   , 0, 0, 'v'},
       {"config"    , 1, 0, 'c'},
       {"sleep"     , 1, 0, 's'},
       {"pid"       , 1, 0, 'p'},
       {0           , 0, 0, 0}
    };
    int c;
    int option_index;

    /* No error messages from getopt */
    opterr = 0;

    while ((c = getopt_long (argc, argv, "hvc:s:p:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'h':
                show_help();
                break;
            case 'v':
                show_version();
                break;
            case 'c':
                if (config != NULL) free(config);
                config = strdup(optarg);
                break;
            case 'p':
                if (pid != NULL) free(pid);
                pid = strdup(optarg);
                break;
            case 's':
                sleeptime = atoi(optarg);
                if (sleeptime <= 0) {
                    fprintf(stderr, ERRORPREFIX "Invalid sleep time!\n");
                    exit(1);
                }
                break;
            default:
                fprintf(stderr, ERRORPREFIX "Invalid parameters, use --help for help.\n");
                exit(1);
                break;
        }
    }

    if (optind < argc) {
        fprintf(stderr, ERRORPREFIX "Invalid parameters, use --help for help.\n");
        exit(1);
    }
    if (config == NULL) {
        config = strdup(CONFIGFILE);
    }
    if (pid == NULL) {
        pid = strdup(PIDFILE);
    }
}

void load_config() {
    FILE        *file;
    char        *line = NULL;
    size_t      len = 0;

    /* Open configuration */
    file = fopen(config, "r");
    if (file == NULL) {
        fprintf(stderr, ERRORPREFIX "Failed to open configuration %s (%s)\n", config, strerror(errno));
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
}


int main(int argc, char **argv) {
    int         i;
    FILE        *file;

    /* Parse parameters is optionally config file name */
    parse_params(argc, argv);

    /* Load configuration */
    load_config();

    /* Disconnect terminal.. */
    daemon(0, 0);

    /* Write pid file (be quiet on error) */
    file = fopen(pid, "w");
    if (file != NULL) {
        fprintf(file, "%d\n", getpid());
        fclose(file);
    }

    /* Main loop */
    while (1) {
        sleep(sleeptime);
        for (i = 0; i < listlen; i++) {
            file = fopen(filelist[i], "r");
            if (file != NULL) fclose(file);
        }
    }

    exit(0);
}

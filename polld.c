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
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if LINUX
#include <sys/ioctl.h>
#include <linux/fs.h>
#endif

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
#ifndef VERSION
#   define VERSION "unknown"
#endif

/* From linux/fs.h */
#define BLKRRPART  _IO(0x12,95)    /* re-read partition table */

/* Commandline params */
char        *config = NULL;
char        *pid = NULL;
int         sleeptime = SLEEPTIME;

/* Parameters */
char        **filelist = NULL;
int         listlen = 0;
int         alloclen = 0;

/* Global flags */
volatile int    shutdown = 0;
volatile int    reload = 0;

#define ERRORPREFIX "polld: "

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
            " -p --pid file         lock file for storing pid, empty for no locking\n"
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

void check_lock(void) {
    FILE        *file;
    int         other;

    if (strlen(pid) == 0) return;

    /* Read existing pid */
    file = fopen(pid, "r");
    if (file != NULL) {
        if (fscanf(file, "%d", &other) == 1) {
            if (kill(other, 0) == 0) {
                fprintf(stderr, ERRORPREFIX "Another instance is running, please stop it first!\n");
                exit(1);
            }
        } else {
            fprintf(stderr, ERRORPREFIX "Can not parse pidfile, ignoring!\n");
        }
        fclose(file);
    }
}


void do_lock(void) {
    FILE        *file;

    if (strlen(pid) == 0) return;

    /* Write pid file */
    file = fopen(pid, "w");
    if (file != NULL) {
        fprintf(file, "%d\n", getpid());
        fclose(file);
    } else {
        fprintf(stderr, ERRORPREFIX "Can not create pidfile!\n");
        exit(1);
    }
}

void bye(void) {
    /* Remove lock file */
    if (strlen(pid) != 0) {
        unlink(pid);
    }

    exit(0);
}

/* Signal handlers */
void interrupt(int sign)
{
    signal(sign, SIG_IGN);
    shutdown = 1;
    bye();
}

void hup(int sign)
{
    reload = 1;
}


int main(int argc, char **argv) {
    int i;
	int file;

    /* Parse parameters is optionally config file name */
    parse_params(argc, argv);

    /* Load configuration */
    load_config();

    /* Check for locking while we have terminal */
    check_lock();

    /* Fake lock while we have terminal */
    do_lock();

    /* Disconnect terminal.. */
    daemon(0, 0);

    /* Real lock */
    do_lock();

    /* Trap signals */
    signal(SIGINT, interrupt);
    signal(SIGQUIT, interrupt);
    signal(SIGTERM, interrupt);
    signal(SIGHUP, hup);

    /* Main loop */
    while (!shutdown) {
        sleep(sleeptime);

        /* Should we reload? */
        if (reload) {
            for (i = 0; i < listlen; i++) {
                free(filelist[i]);
            }

            free(filelist);
            filelist = NULL;
            listlen = 0;
            alloclen = 0;

            load_config();
        }

        /* Do the main work */
        for (i = 0; i < listlen; i++) {
            file = open(filelist[i], O_RDONLY);
            if (file != -1) {
#if LINUX
				ioctl(file, BLKRRPART);
#endif
				close(file);
			}
        }
    }

    /* Cleanup */
    bye();

    /* Just to make gcc happy */
    return 0;
}

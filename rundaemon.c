/*
 * rundaemon.c - a simple program to daemonize anything
 *
 * Redirects stdin/stdout/stderr to /dev/null but ignores other open
 * file descriptors. Doesn't modify environment variables.
 *
 * SPDX-License-Identifier: Unlicense
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#define VERSION_STR "1.0"

#define DIE(fmt, args...) do { \
        fprintf(stderr, "Error: " fmt "\n", ##args); \
        exit(EXIT_FAILURE); \
    } while(0)

static const char usage_text[] =
    "rundaemon version " VERSION_STR "\n"
    "Usage: rundaemon [-hci] [-p PIDFILE] COMMAND [ARGUMENTS...]\n"
    "   Options:\n"
    "       -h          Show this help text\n"
    "       -c          Change directory to / before launching COMMAND\n"
    "       -i          Don't redirect stdin/stdout/stderr to /dev/null\n"
    "       -p PIDFILE  Write a PID file\n"
    "";

int main(int argc, char *argv[])
{
    bool    change_dir = false;
    bool    redirect_stdio = true;
    FILE    *old_stderr = stderr;
    char    *pidfile = NULL;
    char    **cmd_argv;
    int     opt;
    pid_t   cpid;

    while ((opt = getopt(argc, argv, "+hcip:")) != -1)
    {
        switch (opt)
        {
            case 'h':
                puts(usage_text);
                exit(EXIT_SUCCESS);
                break;
            case 'c':
                change_dir = true;
                break;
            case 'i':
                redirect_stdio = false;
                break;
            case 'p':
                if (strlen(optarg) == 0)
                    DIE("PID file length is zero");
                pidfile = optarg;
                break;
            default:
                DIE("Invalid options\n%s", usage_text);
        }
    }

    if (argc <= optind)
        DIE("No command specified\n%s", usage_text);
    cmd_argv = &argv[optind];

    cpid = fork();
    if (cpid < 0)
        DIE("first fork failed: %s", strerror(errno));
    else if (cpid > 0)
        exit(EXIT_SUCCESS);

    if (setsid() < 0)
        DIE("setsid failed: %s", strerror(errno));

    cpid = fork();
    if (cpid < 0)
        DIE("second fork failed: %s", strerror(errno));
    else if (cpid > 0)
        exit(EXIT_SUCCESS);

    if (pidfile != NULL)
    {
        FILE *pid_fp = fopen(pidfile, "w");
        if (pid_fp == NULL)
            DIE("Failed to open PID file '%s': %s", pidfile, strerror(errno));
        fprintf(pid_fp, "%d\n", getpid());
        fclose(pid_fp);
    }

    if (change_dir && (chdir("/") < 0))
        DIE("chdir failed: %s", strerror(errno));

    if (redirect_stdio)
    {
        int fd_err, fd_null;
        fd_err = dup(STDERR_FILENO);
        if ((fd_err < 0) || ((old_stderr = fdopen(fd_err, "w")) == NULL))
            DIE("failed to dup/fdopen stderr: %s", strerror(errno));
        if ((fd_null = open("/dev/null", O_RDWR)) < 0)
            DIE("failed to open /dev/null: %s", strerror(errno));

        if ((dup2(fd_null, STDIN_FILENO) < 0) ||
            (dup2(fd_null, STDOUT_FILENO) < 0) ||
            (dup2(fd_null, STDERR_FILENO) < 0))
        {
            fprintf(old_stderr, "Error: failed to redirect stdio: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    execvp(cmd_argv[0], cmd_argv);
    fprintf(old_stderr, "Error: failed to exec '%s': %s\n", cmd_argv[0], strerror(errno));
    exit(EXIT_FAILURE);
}

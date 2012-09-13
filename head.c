/*
 * head.c - copy the first part of files
 *
 * Version: 2008-1.01
 * Build:   c89 -o head head.c
 * Source:  <http://pdcore.sourceforge.net/>
 * Spec:    <http://www.opengroup.org/onlinepubs/9699919799/utilities/head.html>
 *
 * This is free and unencumbered software released into the public domain,
 * provided "as is", without warranty of any kind, express or implied. See the
 * file UNLICENSE and the website <http://unlicense.org> for further details.
 */


#define _POSIX_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define USAGE   "usage: head [-n number] [file ...]\n"
#define BUFSIZE 4096

static long ctol(char *s);
static void headfile(int fd, char *fn, int lines);
static void error(char *s);

static int exitstatus;


int main(int argc, char **argv)
{
    extern int opterr, optind;
    extern char *optarg;
    int c, fd, many, first;
    char *fn;
    long lines = 10;

    setlocale(LC_ALL, "");
    opterr = 0;

    while ((c = getopt(argc, argv, "n:")) != -1)
        switch (c)
        {
        case 'n':
            if (*optarg && (lines = ctol(optarg)) > 0)
                break;
            /* else fall through */
        default:
            fprintf(stderr, USAGE);
            exit(1);
        }

    if (optind >= argc)
        headfile(STDIN_FILENO, "stdin", lines);
    else
    {
        many  = (optind + 1 < argc) ? 1 : 0;
        first = 1;

        while (optind < argc)
        {
            fn = argv[optind++];

            if (many)
            {
                if (first)
                    first = 0;
                else
                    printf("\n");
                printf("==> %s <==\n", fn);
            }

            if (strcmp(fn, "-") == 0)
                headfile(STDIN_FILENO, "stdin", lines);
            else
                if ((fd = open(fn, O_RDONLY)) == -1)
                    error(fn);
                else
                {
                    headfile(fd, fn, lines);
                    if (close(fd) == -1)
                        error(fn);
                }
        }
    }

    return(exitstatus);
}


long ctol(char *s)
{
    int badch = 0;
    char *c = s;

    /* only copes with non-zero, optionally signed, */
    /* decimal integers; that's all we need         */
    if (! (isdigit(*c) || *c == '+' || *c == '-'))
        badch = 1;
    else
        for (c++; *c; c++)
            if (! isdigit(*c))
                badch = 1;

    return (badch ? 0 : atol(s));
}


void headfile(int fd, char *fn, int lines)
{
    unsigned char buf[BUFSIZE], *c;
    ssize_t n, o;

    while (lines)
    {
        if ((n = read(fd, buf, BUFSIZE)) <= 0)
            break;

        for (c = buf; lines && c < (buf + n); c++)
            if (*c == '\n')
                lines--;

        o = lines ? n : c - buf;
        if (write(STDOUT_FILENO, buf, (size_t)o) != o)
        {
            error("stdout");
            break;
        }
    }

    if (n < 0)
        error(fn);
}


void error(char *s)
{
    fprintf(stderr, "head: %s: %s\n", s, strerror(errno));
    exitstatus = 1;
}

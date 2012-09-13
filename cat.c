/*
 * cat.c - concatenate file(s) to standard output
 *
 * Version: 2008-1.01
 * Build:   c89 -o cat cat.c
 * Source:  <http://pdcore.sourceforge.net/>
 * Spec:    <http://www.opengroup.org/onlinepubs/9699919799/utilities/cat.html>
 *
 * This is free and unencumbered software released into the public domain,
 * provided "as is", without warranty of any kind, express or implied. See the
 * file UNLICENSE and the website <http://unlicense.org> for further details.
 */


#define _POSIX_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define USAGE   "usage: cat [-u] [file ...]\n"
#define BUFSIZE 4096

static void catfile(int fd, char *fn);
static void error(char *s);

static int exitstatus;
static int optu;


int main(int argc, char **argv)
{
    extern int opterr, optind;
    int c, fd;
    char *fn;

    setlocale(LC_ALL, "");
    opterr = 0;

    while ((c = getopt(argc, argv, "u")) != -1)
        switch (c)
        {
        case 'u':
            optu = 1;
            break;
        default:
            fprintf(stderr, USAGE);
            return(1);
        }

    if (optind >= argc)
        catfile(STDIN_FILENO, "stdin");
    else
        while (optind < argc)
        {
            fn = argv[optind++];

            if (strcmp(fn, "-") == 0)
                catfile(STDIN_FILENO, "stdin");
            else
                if ((fd = open(fn, O_RDONLY)) == -1)
                    error(fn);
                else
                {
                    catfile(fd, fn);
                    if (close(fd) == -1)
                        error(fn);
                }
        }

    return(exitstatus);
}


void catfile(int fd, char *fn)
{
    unsigned char buf[BUFSIZE];
    ssize_t n;

    while ((n = read(fd, buf, (optu ? 1 : BUFSIZE))) > 0)
        if (write(STDOUT_FILENO, buf, (size_t)n) != n)
        {
            error("stdout");
            break;
        }

    if (n < 0)
        error(fn);
}


void error(char *s)
{
    fprintf(stderr, "cat: %s: %s\n", s, strerror(errno));
    exitstatus = 1;
}

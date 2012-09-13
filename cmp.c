/*
 * cmp.c - compare two files
 *
 * Version: 2008-1.01
 * Build:   c89 -o cmp cmp.c
 * Source:  <http://pdcore.sourceforge.net/>
 * Spec:    <http://www.opengroup.org/onlinepubs/9699919799/utilities/cmp.html>
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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define USAGE      "usage: cmp [-l|-s] file1 file2\n"
#define ELSMUTEXCL "Options -l and -s are mutually exclusive"
#define EBOTHSTDIN "Input files must be different"
#define BUFSIZE    4096
#define SYSERR     1
#define APPERR     0

static int cmpfiles(int fd1, char *fn1, int fd2, char *fn2);
static void fatal(int errtype, char *s);

static int optl, opts;


int main(int argc, char **argv)
{
    extern int opterr, optind;
    int c, exitstatus = 0;
    int fd1, fd2;
    char *fn1, *fn2;

    setlocale(LC_ALL, "");
    opterr = 0;

    while ((c = getopt(argc, argv, "ls")) != -1)
        switch (c)
        {
        case 'l':
            optl = 1;
            break;
        case 's':
            opts = 1;
            break;
        default:
            fprintf(stderr, USAGE);
            exit(2);
        }

    argc -= optind;
    argv += optind;

    if (optl && opts)
        fatal(APPERR, ELSMUTEXCL);

    if (argc != 2)
        fatal(APPERR, USAGE);

    fn1 = argv[0];
    if (strcmp(fn1, "-") == 0)
        fd1 = STDIN_FILENO;
    else
        if ((fd1 = open(fn1, O_RDONLY)) == -1)
            fatal(SYSERR, fn1);

    fn2 = argv[1];
    if (strcmp(fn2, "-") == 0)
        fd2 = STDIN_FILENO;
    else
        if ((fd2 = open(fn2, O_RDONLY)) == -1)
            fatal(SYSERR, fn2);

    if (fd1 == STDIN_FILENO && fd2 == STDIN_FILENO)
        fatal(APPERR, EBOTHSTDIN);

    exitstatus = cmpfiles(fd1, fn1, fd2, fn2);

    if (fd1 != STDIN_FILENO && (close(fd1) == -1))
        fatal(SYSERR, fn1);

    if (fd2 != STDIN_FILENO && (close(fd2) == -1))
        fatal(SYSERR, fn2);

    return(exitstatus);
}


int cmpfiles(int fd1, char *fn1, int fd2, char *fn2)
{
    unsigned char buf1[BUFSIZE], buf2[BUFSIZE];
    ssize_t n1, n2, byte = 1, line = 1;
    int cnt, i, eof = 0, differ = 0, shorter = 0;

    while (! eof)
    {
        if ((n1 = read(fd1, buf1, BUFSIZE)) == -1)
            fatal(SYSERR, fn1);

        if ((n2 = read(fd2, buf2, BUFSIZE)) == -1)
            fatal(SYSERR, fn2);

        if (n1 != BUFSIZE || n2 != BUFSIZE)
        {
            eof = 1;
            if (n1 != n2)
                shorter = (n1 < n2) ? 1 : 2;
        }

        cnt = (n1 < n2) ? n1 : n2;
        for (i = 0; i < cnt; i++)
        {
            if (buf1[i] != buf2[i])
            {
                if (! differ && ! optl && ! opts)
                {
                    printf("%s %s differ: char %d, line %d\n", fn1, fn2, byte, line);
                    return(1);  /* only need to report first mismatch */
                }

                differ = 1;

                if (optl)
                    printf("%d %o %o\n", byte, buf1[i], buf2[i]);
            }

            byte++;
            if (buf1[i] == '\n')
                line++;
        }
    }

    if (shorter && (optl || (! opts && ! differ)))
    {
        differ = 1;
        fprintf(stderr, "cmp: EOF on %s\n", (shorter == 1 ? fn1 : fn2));
    }

    return(differ || shorter);
}


void fatal(int errtype, char *s)
{
    if (errtype == SYSERR)
        fprintf(stderr, "cmp: %s: %s\n", s, strerror(errno));
    else
        fprintf(stderr, "cmp: %s\n", s);

    exit(2);
}

/*
 * tee.c - duplicate standard input
 *
 * Version: 2008-1.01
 * Build:   c89 -o tee tee.c
 * Source:  <http://pdcore.sourceforge.net/>
 * Spec:    <http://www.opengroup.org/onlinepubs/9699919799/utilities/tee.html>
 *
 * This is free and unencumbered software released into the public domain,
 * provided "as is", without warranty of any kind, express or implied. See the
 * file UNLICENSE and the website <http://unlicense.org> for further details.
 */


#define _POSIX_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define USAGE    "usage: tee [-ai] [file ...]\n"
#define AMBIGOPT "tee: Ambiguous [-]; operand or bad option?\n"
#define FLIMIT   "tee: Maximum of %d output files exceeded\n"
#define BUFSIZE  4096
#define MAXFILES 13

static void error(char *s);

static int exitstatus;


int main(int argc, char **argv)
{
    extern int opterr, optind;
    int c, i;
    unsigned char buf[BUFSIZE];
    ssize_t n;
    int slot, fdfn[MAXFILES + 1][2], mode = O_WRONLY | O_CREAT | O_TRUNC;

    setlocale(LC_ALL, "");
    opterr = 0;

    while ((c = getopt(argc, argv, "ai")) != -1)
        switch (c)
        {
        case 'a':   /* append to rather than overwrite file(s) */
            mode = O_WRONLY | O_CREAT | O_APPEND;
            break;

        case 'i':   /* ignore the SIGINT signal */
            signal(SIGINT, SIG_IGN);
            break;

        default:
            fprintf(stderr, USAGE);
            return(1);
        }

    i = optind;
    fdfn[0][0] = STDOUT_FILENO;

    for (slot = 1; i < argc ; i++)
    {
        if (slot > MAXFILES)
            fprintf(stderr, FLIMIT, MAXFILES);
        else
        {
            if ((fdfn[slot][0] = open(argv[i], mode, 0666)) == -1)
                error(argv[i]);
            else
                fdfn[slot++][1] = i;
        }
    }

    while ((n = read(STDIN_FILENO, buf, BUFSIZE)) > 0)
    {
        for (i = 0; i < slot; i++)
            if (write(fdfn[i][0], buf, (size_t)n) != n)
                error(argv[fdfn[i][1]]);
    }

    if (n < 0)
        error("stdin");

    for (i = 1; i < slot; i++)
        if (close(fdfn[i][0]) == -1)
            error(argv[fdfn[i][1]]);

    return(exitstatus);
}


void error(char *s)
{
    fprintf(stderr, "tee: %s: %s\n", s, strerror(errno));
    exitstatus = 1;
}

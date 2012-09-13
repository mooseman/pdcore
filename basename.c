/*
 * basename.c - return the directory portion of a pathname
 *
 * Version: 2008-1.01
 * Build:   c89 -o basename basename.c
 * Source:  <http://pdcore.sourceforge.net/>
 * Spec:    <http://www.opengroup.org/onlinepubs/9699919799/utilities/basename.html>
 *
 * This is free and unencumbered software released into the public domain,
 * provided "as is", without warranty of any kind, express or implied. See the
 * file UNLICENSE and the website <http://unlicense.org> for further details.
 */


#define _POSIX_SOURCE

#include <locale.h>
#include <stdio.h>

#define USAGE "usage: basename string [suffix]\n"


int main(int argc, char **argv)
{
    char *str, *head, *tail, *newtail, *sfx, *sfxtail;

    setlocale (LC_ALL, "");

    if (argc < 2 || argc > 3)
    {
        fprintf(stderr, USAGE);
        return(1);
    }

    tail = str = argv[1];
    while (*tail)
        tail++;
    while (tail > str && tail[-1] == '/')
        tail--;

    head = tail;
    while (head > str && head[-1] != '/')
        head--;

    if (argc == 3)
    {
        newtail = tail;
        sfxtail = sfx = argv[2];
        while (*sfxtail)
            sfxtail++;

        while (newtail > (head + 1) && sfxtail > sfx && newtail[-1] == sfxtail[-1])
        {
            newtail--;
            sfxtail--;
        }

        if (sfxtail == sfx)
            tail = newtail;
    }

    printf("%.*s\n", (tail - head), head);
    return(0);
}

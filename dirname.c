/*
 * dirname.c - return the directory portion of a pathname
 *
 * Version: 2008-1.01
 * Build:   c89 -o dirname dirname.c
 * Source:  <http://pdcore.sourceforge.net/>
 * Spec:    <http://www.opengroup.org/onlinepubs/9699919799/utilities/dirname.html>
 *
 * This is free and unencumbered software released into the public domain,
 * provided "as is", without warranty of any kind, express or implied. See the
 * file UNLICENSE and the website <http://unlicense.org> for further details.
 */


#define _POSIX_SOURCE

#include <locale.h>
#include <stdio.h>

#define USAGE "usage: dirname string\n"


int main(int argc, char **argv)
{
    char *head, *tail;

    setlocale (LC_ALL, "");

    if (argc != 2)
    {
        fprintf(stderr, USAGE);
        return(1);
    }

    head = tail = argv[1];
    while (*tail)
        tail++;

    while (tail > head && tail[-1] == '/')
        tail--;
    while (tail > head && tail[-1] != '/')
        tail--;
    while (tail > head && tail[-1] == '/')
        tail--;

    if (head == tail)
        printf(*head == '/' ? "/\n" : ".\n");
    else
        printf("%.*s\n", (tail - head), head);

    return(0);
}

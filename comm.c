/*
 * comm.c - select or reject lines common to two files
 *
 * Version: 2008-1.01
 * Build:   c89 -o comm comm.c
 * Source:  <http://pdcore.sourceforge.net/>
 * Spec:    <http://www.opengroup.org/onlinepubs/9699919799/utilities/comm.html>
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
#include <unistd.h>

#define USAGE     "usage: comm [-123] file1 file2\n"
#define ENOMALLOC "Unable to allocate memory for read buffer"
#define BUFSIZE   4096
#define LINESIZE  64
#define SYSERR    1
#define APPERR    0

typedef struct { int     fd;
                 char   *fn;
                 int     eof;
                 char    buf[BUFSIZE];
                 size_t  bufpos;
                 size_t  eobuf;
                 char   *line;
                 size_t  linesz;
               } FINFO;

static void initfile(FINFO *file, char *fn);
static void cleanup(FINFO *file);
static void commfiles(FINFO *file1, FINFO *file2);
static int getline(FINFO *file);
static void fillbuf(FINFO *file);
static void fatal(int errtype, char *s);

static int opt1, opt2, opt3;


int main(int argc, char **argv)
{
    extern int optind, opterr;
    int c;
    FINFO file1, file2;

    setlocale(LC_ALL, "");
    opterr = 0;

    while ((c = getopt(argc, argv, "123")) != -1)
        switch (c)
        {
        case '1':
            opt1 = 1;
            break;
        case '2':
            opt2 = 1;
            break;
        case '3':
            opt3 = 1;
            break;
        default:
            fprintf(stderr, USAGE);
            exit(1);
        }

    argc -= optind;
    argv += optind;

    if (argc != 2)
        fatal(APPERR, USAGE);

    initfile(&file1, argv[0]);
    initfile(&file2, argv[1]);

    commfiles(&file1, &file2);

    cleanup(&file1);
    cleanup(&file2);

    return(0);
}


void initfile(FINFO *file, char *fn)
{
    int fd;

    if (strcmp(fn, "-") == 0)
        fd = STDIN_FILENO;
    else
        if ((fd = open(fn, O_RDONLY)) == -1)
            fatal(SYSERR, fn);

    file->fd     = fd;
    file->fn     = fn;
    file->eof    = 0;
    file->bufpos = 0;
    file->eobuf  = 0;
    file->linesz = LINESIZE;

    if ((file->line = malloc(file->linesz)) == NULL)
        fatal(APPERR, ENOMALLOC);
}


void cleanup(FINFO *file)
{
    free(file->line);

    if (strcmp(file->fn, "-") != 0 && close(file->fd) == -1)
        fatal(SYSERR, file->fn);
}


void commfiles(FINFO *file1, FINFO *file2)
{
    int cmp;

    if (getline(file1) && getline(file2))
        while(1)
        {
            cmp = strcoll(file1->line, file2->line);

            if (cmp == 0)
            {
                if (! opt3)
                    printf("%s%s%s\n", (opt1 ? "" : "\t"), (opt2 ? "" : "\t"), file1->line);

                if ((getline(file1) + getline(file2)) < 2)
                    break;
            }

            else if (cmp > 0)
            {
                if (! opt2)
                    printf("%s%s\n", (opt1 ? "" : "\t"), file2->line);

                if (! getline(file2))
                    break;
            }

            else   /* cmp < 0 */
            {
                if (! opt1)
                    printf("%s\n", file1->line);

                if (! getline(file1))
                    break;
            }
        }

    while(! file1->eof && ! opt1)
    {
        printf("%s\n", file1->line);
        getline(file1);
    }

    while(! file2->eof && ! opt2)
    {
        printf("%s%s\n", (opt1 ? "" : "\t"), file2->line);
        getline(file2);
    }
}


int getline(FINFO *file)
{
    int success = 0;
    size_t i = 0;

    if (! file->eof)
    {
        while(1)
        {
            if (i == file->linesz)
            {
                file->linesz = file->linesz + LINESIZE;
                if ((file->line = realloc(file->line, file->linesz)) == NULL)
                    fatal(APPERR, ENOMALLOC);
            }

            if (file->bufpos >= file->eobuf)
            {
                fillbuf(file);
                if (file->eof)
                {
                    if (i > 0)
                    {
                        file->line[i] = '\0';
                        success = 1;
                    }
                    break;
                }
            }

            if (file->buf[file->bufpos] == '\n')
            {
                file->bufpos++;
                file->line[i] = '\0';
                success = 1;
                break;
            }
            else
                file->line[i++] = file->buf[file->bufpos++];
        }
    }

    return(success);
}


void fillbuf(FINFO *file)
{
    ssize_t n;

    if (! file->eof)
    {
        if ((n = read(file->fd, file->buf, BUFSIZE)) < 0)
            fatal(SYSERR, file->fn);

        if (n == 0)
            file->eof = 1;
        else
        {
            file->bufpos = 0;
            file->eobuf  = n;
        }
    }
}


void fatal(int errtype, char *s)
{
    if (errtype == SYSERR)
        fprintf(stderr, "comm: %s: %s\n", s, strerror(errno));
    else
        fprintf(stderr, "comm: %s\n", s);

    exit(1);
}

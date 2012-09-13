/*
 * tail.c - copy the last part of a file
 *
 * Version: 2008-1.01
 * Build:   c89 -o tail tail.c
 * Source:  <http://pdcore.sourceforge.net/>
 * Spec:    <http://www.opengroup.org/onlinepubs/9699919799/utilities/tail.html>
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
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define USAGE     "usage: tail [-f] [-c number] [-n number] [file]\n"
#define NOMALLOC  "Unable to allocate memory for read buffer"
#define NOFOLLOW  "tail: Option [-f] ignored; can only follow regular files and FIFOs\n"
#define BADBYTES  "Option [-c] requires non-zero byte offset"
#define BADLINES  "Option [-n] requires integer line offset"
#define CNMUTEXCL "Options [-c] and [-n] are mutually exclusive"
#define FVANISH   "Followed file has vanished"
#define FSHRUNK   "Followed file has shrunk"
#define SYSERR    1
#define APPERR    0

static int isdecint(char *s);
static void tailfile(int fd, char *fn);
static void tailbyteoffset(int fd, char *fn, int isregfile, ssize_t len);
static void taillineoffset(int fd, char *fn, int isregfile, ssize_t len);
static void copytoeof(int fd, char *fn);
static void fatal(int errtype, char *s);

static unsigned char* buf;
static ssize_t bufsize, offset;
static int optf, optc, optn;


int main(int argc, char **argv)
{
    extern int opterr, optind;
    extern char *optarg;
    int c, fd;
    char *fn;

    setlocale(LC_ALL, "");
    opterr = 0;

    /*
     * try to get a buffer _twice_ the size mandated by the standard as in
     * later processing we may be paging through the file in _half_ buffer
     * increments; this way we guarentee POSIX buffer size conformance
     */
    bufsize = sysconf(_SC_LINE_MAX) * 20;   /* POSIX says [{LINE_MAX)*10] */
    bufsize = bufsize < 4096 ? 4096 : bufsize;

    if ((buf = malloc(bufsize)) == NULL)
        fatal(APPERR, NOMALLOC);

    while ((c = getopt(argc, argv, "fc:n:")) != -1)
        switch (c)
        {
        case 'f':   /* follow file */
            optf = 1;
            break;

        case 'c':   /* offset in bytes */
            optc = 1;
            offset = 0;
            if (*optarg && isdecint(optarg))
            {
                offset = atol(optarg);
                offset = isdigit(*optarg) ? -offset : offset;
            }

            if (offset == 0)
                fatal(APPERR, BADBYTES);
            else
                break;

        case 'n':   /* offset in lines */
            optn = 1;
            if (*optarg && isdecint(optarg))
            {
                offset = atol(optarg);
                offset = isdigit(*optarg) ? -offset : offset;

                /* the standard says [-n 0] is OK, but [-n +0] isn't &_& */
                if (offset != 0 || (offset == 0 && *optarg != '+'))
                    break;
            }
            fatal(APPERR, BADLINES);

        default:
            fprintf(stderr, USAGE);
            exit(1);
        }

    if (! optc && ! optn)   /* if none, set the default options */
    {
        optn = 1;
        offset = -10;
    }

    if (optc && optn)
        fatal(APPERR, CNMUTEXCL);

    if (optind >= argc)
        tailfile(STDIN_FILENO, "stdin");
    else
    {
        fn = argv[optind++];

        if (strcmp(fn, "-") == 0)
            tailfile(STDIN_FILENO, "stdin");
        else
            if ((fd = open(fn, O_RDONLY)) == -1)
                fatal(SYSERR, fn);
            else
            {
                tailfile(fd, fn);
                if (close(fd) == -1)
                    fatal(SYSERR, fn);
            }
    }

    return(0);
}


int isdecint(char *s)
{
    int badch = 0;
    char *c = s;

    if (! (isdigit(*c) || *c == '+' || *c == '-'))
        badch = 1;
    else
        for (c++; *c; c++)
            if (! isdigit(*c))
                badch = 1;

    return (! badch);
}


void tailfile(int fd, char *fn)
{
    struct stat sbuf;
    int isregfile;
    ssize_t n, len;

    if (fstat(fd, &sbuf) == -1)
        fatal(SYSERR, fn);

    if (S_ISREG(sbuf.st_mode))
    {
        isregfile = 1;
        len = sbuf.st_size;
    }
    else
        isregfile = len = 0;

    if (offset != 0)   /* [-n 0] is permitted; but no point processing that */
    {
        if (optc)
            tailbyteoffset(fd, fn, isregfile, len);
        else
            taillineoffset(fd, fn, isregfile, len);
    }

    if (optf)
    {
        if (fd == STDIN_FILENO)   /* can't follow; warn then exit no error */
        {
            fprintf(stderr, NOFOLLOW);
            exit(0);
        }

        while(1)   /* follow forever */
        {
            sleep(1);

            if (isregfile)   /* if we can, be polite to the user */
            {
                if (stat(fn, &sbuf) == -1)
                    fatal(APPERR, FVANISH);
                else if (sbuf.st_size < len)
                    fatal(APPERR, FSHRUNK);

                len = sbuf.st_size;
            }

            while ((n = read(fd, buf, bufsize)) > 0)
                if (write(STDOUT_FILENO, buf, n) != n)
                    fatal(SYSERR, "stdout");

            if (n < 0)
                fatal(SYSERR, fn);
        }
    }
}


void tailbyteoffset(int fd, char *fn, int isregfile, ssize_t len)
{
    ssize_t n, halfbuf;

    if (isregfile)   /* should be seekable, so we'll do so; it's fastest */
    {
        if (offset > 0)
            offset = (offset - 1) > len ? len : offset - 1;
        else
            offset = (len + offset) > 0 ? len + offset : 0;

        if (lseek(fd, (off_t)offset, SEEK_SET) == (off_t)-1)
            fatal(SYSERR, fn);

        copytoeof(fd, fn);
    }

    else   /* possibly non-seekable */
    {
        if (offset > 0)   /* forwards through file */
        {
            offset--;

            while(1)
            {
                if ((n = read(fd, buf, bufsize)) < 0)
                    fatal(SYSERR, fn);

                if (n == 0)
                    offset = 0;

                if (offset <= n)
                    break;
                else
                    offset -= n;
            }

            if (write(STDOUT_FILENO, buf + offset, n - offset) != n - offset)
                fatal(SYSERR, "stdout");

            copytoeof(fd, fn);
        }

        else   /* backwards through file; remember that offset is negative */
        {
            halfbuf = bufsize / 2;

            if ((n = read(fd, buf, bufsize)) < 0)
                fatal(SYSERR, fn);

            if (n < bufsize)   /* we've got the whole file */
            {
                offset = (n + offset) < 0 ? 0 : n + offset;
                len = n - offset;
            }

            else   /* we haven't got the whole file */
            {
                while(1)   /* page through the file, half a buffer at a time */
                {
                    memcpy(buf, buf + halfbuf, halfbuf);

                    if ((n = read(fd, buf + halfbuf, halfbuf)) < 0)
                        fatal(SYSERR, fn);
                    else if (n < halfbuf)
                        break;
                }

                offset = (halfbuf + n + offset) < 0 ? 0 : halfbuf + n + offset;
                len = halfbuf + n - offset;
            }

            if (write(STDOUT_FILENO, buf + offset, len) != len)
                fatal(SYSERR, "stdout");
        }
    }
}


void taillineoffset(int fd, char *fn, int isregfile, ssize_t len)
{
    ssize_t n, i, halfbuf;

    if (offset > 0)   /* forwards through file */
    {
        offset--;

        while(1)
        {
            if ((n = read(fd, buf, bufsize)) < 0)
                fatal(SYSERR, fn);

            if (n == 0)
            {
                offset = 0;
                break;
            }

            for (i = 0; i < n && offset > 0; i++)
                if (buf[i] == '\n')
                    offset--;

            if (offset == 0)
            {
                offset = i;
                break;
            }
        }

        if (write(STDOUT_FILENO, buf + offset, n - offset) != n - offset)
            fatal(SYSERR, "stdout");

        copytoeof(fd, fn);
    }

    else   /* backwards through file; remember that offset is negative */
    {
        if (isregfile && len > 0)   /* should be seekable, so we'll do so */
        {
            n = (len - bufsize) < 0 ? 0 : len - bufsize;

            if (lseek(fd, (off_t)n, SEEK_SET) == (off_t)-1)
                fatal(SYSERR, fn);

            if ((n = read(fd, buf, bufsize)) < 0)
                fatal(SYSERR, fn);

            if (buf[n-1] == '\n')
                offset--;

            for (i = n - 1; i >= 0 && offset < 0; i--)
                if (buf[i] == '\n')
                    offset++;

            if (offset == 0)
                offset = i + 2;
            else
                offset = 0;

            len = n - offset;

            if (write(STDOUT_FILENO, buf + offset, len) != len)
                fatal(SYSERR, "stdout");
        }

        else
        {
            halfbuf = bufsize / 2;

            if ((n = read(fd, buf, bufsize)) < 0)
                fatal(SYSERR, fn);

            if (n < bufsize)   /* we've got the whole file */
            {
                if (n == 0)
                    offset = 0;
                else
                {
                    if (buf[n-1] == '\n')
                        offset--;

                    for (i = n - 1; i >= 0 && offset < 0; i--)
                        if (buf[i] == '\n')
                            offset++;

                    if (offset == 0)
                        offset = i + 2;
                    else
                        offset = 0;
                }

                len = n - offset;
            }

            else   /* we haven't got the whole file */
            {
                while(1)   /* page through the file, half a buffer at a time */
                {
                    memcpy(buf, buf + halfbuf, halfbuf);

                    if ((n = read(fd, buf + halfbuf, halfbuf)) < 0)
                        fatal(SYSERR, fn);
                    else if (n < halfbuf)
                        break;
                }

                if (buf[halfbuf+n-1] == '\n')
                    offset--;

                for (i = halfbuf + n - 1; i >= 0 && offset < 0; i--)
                    if (buf[i] == '\n')
                        offset++;

                if (offset == 0)
                    offset = i + 2;
                else
                    offset = 0;

                len = halfbuf + n - offset;
            }

            if (write(STDOUT_FILENO, buf + offset, len) != len)
                fatal(SYSERR, "stdout");
        }
    }
}


void copytoeof(int fd, char *fn)
{
    ssize_t n;

    while ((n = read(fd, buf, bufsize)) > 0)
        if (write(STDOUT_FILENO, buf, n) != n)
            fatal(SYSERR, "stdout");

    if (n < 0)
        fatal(SYSERR, fn);
}


void fatal(int errtype, char *s)
{
    if (errtype == SYSERR)
        fprintf(stderr, "tail: %s: %s\n", s, strerror(errno));
    else
        fprintf(stderr, "tail: %s\n", s);

    exit(1);
}

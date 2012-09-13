/*
 * cksum - report checksum and octet count of file(s)
 *
 * Version: 2008-1.01
 * Build:   c89 -o cksum cksum.c
 * Source:  <http://pdcore.sourceforge.net/>
 * Spec:    <http://www.opengroup.org/onlinepubs/9699919799/utilities/cksum.html>
 *
 * This is free and unencumbered software released into the public domain,
 * provided "as is", without warranty of any kind, express or implied. See the
 * file UNLICENSE and the website <http://unlicense.org> for further details.
 */


#define _POSIX_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFSIZE 4096

static void cksumfile(int fd, char *fn);
static void error(char *s);

static int exitstatus;


int main(int argc, char **argv)
{
    int i, fd, hasrun = 0;
    char *fn;

    setlocale(LC_ALL, "");

    for (i = 1; i < argc; i++)
    {
        fn = argv[i];

        if (strcmp(fn, "--") == 0)
            continue;
        else
        {
            hasrun = 1;

            if (strcmp(fn, "-") == 0)
                cksumfile(STDIN_FILENO, "stdin");
            else
                if ((fd = open(fn, O_RDONLY)) == -1)
                    error(fn);
                else
                {
                    cksumfile(fd, fn);
                    if (close(fd) == -1)
                        error(fn);
                }
        }
    }

    if (! hasrun)
        cksumfile(STDIN_FILENO, "stdin");

    return(exitstatus);
}


void cksumfile(int fd, char *fn)
{
    unsigned char buf[BUFSIZE];
    ssize_t cnt, n;
    uint32_t crctab[256], crc, i;
    int j;

    for (i = 0; i < 256; ++i)
    {
        for (crc = i << 24, j = 0; j < 8; j++)
            crc = (crc << 1) ^ (crc & 0x80000000 ? 0x04c11db7 : 0);
        crctab[i] = crc;
    }

    cnt = crc = 0;

    while ((n = read(fd, buf, BUFSIZE)) > 0)
    {
        cnt += n;

        for (i = 0; i < (size_t)n; i++)
            crc = crctab[((crc >> 24) ^ buf[i]) & 0xFF] ^ (crc << 8);
    }

    if (n < 0)
        error(fn);
    else
    {
        for (i = cnt; i != 0; i >>= 8)
            crc = crctab[((crc >> 24) ^ i) & 0xFF] ^ (crc << 8);

        printf("%lu %lu", (unsigned long)~crc, (unsigned long)cnt);
        if (fd != STDIN_FILENO)
            printf(" %s", fn);
        printf("\n");
    }
}


void error(char *s)
{
    fprintf(stderr, "cksum: %s: %s\n", s, strerror(errno));
    exitstatus = 1;
}

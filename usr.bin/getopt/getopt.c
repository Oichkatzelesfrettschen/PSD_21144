/*	$NetBSD: getopt.c,v 1.6 2000/07/03 02:51:18 matt Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: getopt.c,v 1.6 2000/07/03 02:51:18 matt Exp $");
#endif /* not lint */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * Demonstration program for getopt(3) converted to standard C17 syntax.
 */

/* Program entry point. */
int main(int argc, char **argv)
{
        int c;              /* option character */
        int status = 0;     /* program exit status */

        /* Past the program name and the option letters. */
        optind = 2;
        while ((c = getopt(argc, argv, argv[1])) != -1) {
                switch (c) {
                case '?':
                        /* getopt routine gave message */
                        status = 1;
                        break;
                default:
                        if (optarg != NULL)
                                printf(" -%c %s", c, optarg);
                        else
                                printf(" -%c", c);
                        break;
                }
        }
        printf(" --");
        for (; optind < argc; optind++)
                printf(" %s", argv[optind]);
        printf("\n");

        exit(status);
}

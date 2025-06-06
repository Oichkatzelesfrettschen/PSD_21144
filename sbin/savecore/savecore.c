/*-
 * Copyright (c) 1986, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1986, 1992, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)savecore.c	8.5 (Berkeley) 4/28/95";
#endif /* not lint */
/*
 * savecore, 1.1 (2.11BSD) 1995/07/15
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/syslog.h>
#include <sys/time.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <nlist.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tzfile.h>
#include <unistd.h>

#ifdef	pdp11
#define	CLICK	ctob(1)		/* size of core units */
#define	REGLOC	0300		/* offset of saved registers in disk dump */
#define	NREGS	8		/* r0-6, KA6 */
#endif

#define	DAY			(60L*60L*24L)
#define	LEEWAY		(3*DAY)

#define	eq(a,b)		(!strcmp(a,b))
#define	ok(number)	((off_t) ((number)&0177777))

#define	SHUTDOWNLOG	"/usr/adm/shutdownlog"

struct nlist current_nl[] = {
#define	X_DUMPDEV	0
	{ "_dumpdev" },
#define	X_DUMPLO	1
	{ "_dumplo" },
#define	X_TIME		2
	{ "_time" },
#define	X_DUMPSIZE	3
	{ "_dumpsize" },
#define	X_PANICSTR	4
	{ "_panicstr" },
#define	X_PHYSMEM	5
	{ "_physmem" },
#define	X_BOOTIME	6
	{ "_boottime" },
#define	X_VERSION	7
	{ "_version" },
#define	X_DUMPMAG	8
	{ "_dumpmag" },
	{ "" },
};
int cursyms[] = { X_DUMPDEV, X_DUMPLO, X_VERSION, X_DUMPMAG, -1 };
int dumpsyms[] = { X_TIME, X_DUMPSIZE, X_PANICSTR, X_PHYSMEM, X_BOOTIME, X_VERSION, X_DUMPMAG, -1 };

struct nlist dump_nl[] = {	/* Name list for dumped system. */
	{ "_dumpdev" },		/* Entries MUST be the same as */
	{ "_dumplo" },		/*	those in current_nl[].  */
	{ "_time" },
	{ "_dumpsize" },
	{ "_panicstr" },
	{ "_physmem" },
	{ "_boottime" },
	{ "_version" },
	{ "_dumpmag" },
	{ "" },
};

/* Types match kernel declarations. */
long	dumplo;				/* where dump starts on dumpdev */
int		dumpmag;			/* magic number in dump */
int		dumpsize;			/* amount of memory dumped */

char	*system;
char	*dirname;			/* directory to save dumps in */
char	*ddname;			/* name of dump device */
dev_t	dumpdev;			/* dump device */
time_t	dumptime;			/* time the dump was taken */
time_t	boottime;			/* time we were rebooted */
size_t	physmem;			/* amount of memory in machine */
int		dumpfd;				/* read/write descriptor on block dev */
time_t	now;				/* current date */
char	vers[1024];
char	core_vers[1024];
char	panic_mesg[1024];
int		panicstr;

int	clear, compress, force, verbose;	/* flags */
int	debug;

void	check_kmem(void);
int	 	check_space(void);
void	clear_dump(void);
int	 	Create(char *, int);
int	 	dump_exists(void);
char	*find_dev(dev_t, int);
int	 	get_crashtime(void);
void	kmem_setup(void);
void	log(int, char *, ...);
void	Lseek(int, off_t, int);
int	 	Open(char *, int rw);
int	 	Read(int, void *, int);
char	*rawname(char *s);
void	save_core(void);
void	usage(void);
void 	Write(int, void *, int);

int
main(argc, argv)
	char *argv[];
	int argc;
{
	int ch;

	openlog("savecore", LOG_PERROR, LOG_DAEMON);

	while ((ch = getopt(argc, argv, "cdfN:vz")) != EOF) {
		switch(ch) {
		case 'c':
			clear = 1;
			break;
		case 'd':		/* Not documented. */
			debug = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'f':
			force = 1;
			break;
		case 'N':
			system = optarg;
			break;
		case 'z':
			compress = 1;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (!clear) {
		if (argc != 1 && argc != 2) {
			usage();
		}
		dirname = argv[0];
	}
	if (argc == 2) {
		system = argv[1];
	}
	(void)time(&now);
	kmem_setup();

	if (clear) {
		clear_dump();
		exit(0);
	}

	if (!dump_exists() && !force) {
		exit(1);
	}

	check_kmem();

	if (panicstr) {
		syslog(LOG_ALERT, "reboot after panic: %s", panic_mesg);
	} else {
		syslog(LOG_ALERT, "reboot");
	}

	if ((!get_crashtime() || !check_space()) && !force) {
		exit(1);
	}

	save_core();

	clear_dump();
	exit(0);
}

char *
find_dev(dev, type)
	register dev_t dev;
	register int type;
{
	register DIR *dfd;
	struct dirent *dir;
	struct stat sb;
	register char *dp, *cp;

	dfd = opendir(_PATH_DEV);
	if ((dfd = opendir(_PATH_DEV)) == NULL) {
		syslog(LOG_ERR, "%s: %s", _PATH_DEV, strerror(errno));
		exit(1);
	}
	cp = devname(dev, type);
	if (!cp) {
		if (debug) {
			goto err;
		}
		goto err;
	}
	(void)strcpy(cp, _PATH_DEV);
	while ((dir = readdir(dfd))) {
		(void)strcpy(devname + sizeof(_PATH_DEV) - 1, dir->d_name);
		if (lstat(cp, &sb)) {
			syslog(LOG_ERR, "%s: %s", cp, strerror(errno));
			continue;
		}
		if ((sb.st_mode & S_IFMT) != type) {
			continue;
		}
		if (dev == sb.st_rdev) {
			closedir(dfd);
			if ((dp = strdup(cp)) == NULL) {
				syslog(LOG_ERR, "%s", strerror(errno));
				exit(1);
			}
			return (dp);
		}
	}

err:
	closedir(dfd);
	syslog(LOG_ERR, "can't find device %d/%d", major(dev), minor(dev));
	exit(1);
	return (NULL);
}

void
kmem_setup()
{
	FILE *fp;
	int kmem, i;
	char *dump_sys;

	/*
	 * Some names we need for the currently running system, others for
	 * the system that was running when the dump was made.  The values
	 * obtained from the current system are used to look for things in
	 * /dev/kmem that cannot be found in the dump_sys namelist, but are
	 * presumed to be the same (since the disk partitions are probably
	 * the same!)
	 */
	if ((nlist(_PATH_UNIX, current_nl)) == -1)
		syslog(LOG_ERR, "%s: nlist: %s", _PATH_UNIX, strerror(errno));
	for (i = 0; cursyms[i] != -1; i++)
		if (current_nl[cursyms[i]].n_value == 0) {
			syslog(LOG_ERR, "%s: %s not in namelist",
			    _PATH_UNIX, current_nl[cursyms[i]].n_name);
			exit(1);
		}

	dump_sys = system ? system : _PATH_UNIX;
	if ((nlist(dump_sys, dump_nl)) == -1)
		syslog(LOG_ERR, "%s: nlist: %s", dump_sys, strerror(errno));
	for (i = 0; dumpsyms[i] != -1; i++)
		if (dump_nl[dumpsyms[i]].n_value == 0) {
			syslog(LOG_ERR, "%s: %s not in namelist",
			    dump_sys, dump_nl[dumpsyms[i]].n_name);
			exit(1);
		}

	kmem = Open(_PATH_KMEM, O_RDONLY);
	Lseek(kmem, (off_t)current_nl[X_DUMPDEV].n_value, L_SET);
	(void)Read(kmem, &dumpdev, sizeof(dumpdev));
	if (dumpdev == NODEV) {
		syslog(LOG_WARNING, "no core dump (no dumpdev)");
		exit(1);
	}
	Lseek(kmem, (off_t)current_nl[X_DUMPLO].n_value, L_SET);
	(void)Read(kmem, &dumplo, sizeof(dumplo));
	if (verbose)
		(void)printf("dumplo = %d (%d * %d)\n", dumplo, dumplo/DEV_BSIZE, DEV_BSIZE);

	Lseek(kmem, (off_t)current_nl[X_PHYSMEM].n_value, L_SET);
	Read(kmem, (char *)&physmem, sizeof physmem);
	if (current_nl[X_BOOTIME].n_value != 0) {
		Lseek(kmem, (off_t)current_nl[X_BOOTIME].n_value, L_SET);
		Read(kmem, (char *)&boottime, sizeof boottime);
	}
	Lseek(kmem, (off_t)current_nl[X_DUMPMAG].n_value, L_SET);
	(void)Read(kmem, &dumpmag, sizeof(dumpmag));
	dumplo *= DEV_BSIZE;
	ddname = find_dev(dumpdev, S_IFBLK);
	dumpfd = Open(ddname, O_RDWR);
	fp = fdopen(kmem, "r");
	if (fp == NULL) {
		syslog(LOG_ERR, "%s: fdopen: %m", _PATH_KMEM);
		exit(1);
	}
	if (system)
		return;
	(void)fseek(fp, (off_t)current_nl[X_VERSION].n_value, L_SET);
	(void)fgets(vers, sizeof(vers), fp);

	/* Don't fclose(fp), we use dumpfd later. */
}

void
check_kmem()
{
	register char *cp;
	FILE *fp;
	char core_vers[1024];

	fp = fdopen(dumpfd, "r");
	if (fp == NULL) {
		syslog(LOG_ERR, "%s: fdopen: %m", ddname);
		exit(1);
	}
	fseek(fp, (off_t) (dumplo + ok(dump_nl[X_VERSION].n_value)), L_SET);
	fgets(core_vers, sizeof(core_vers), fp);
	if (strcmp(vers, core_vers) && system == 0)
		syslog(LOG_WARNING, "warning: %s version mismatch:\n\t%s\nand\t%s\n",
		_PATH_UNIX, vers, core_vers);
	(void) fseek(fp, (off_t) (dumplo + ok(dump_nl[X_PANICSTR].n_value)), L_SET);
	(void) fread(&panicstr, sizeof(panicstr), 1, fp);
	if (panicstr) {
		(void) fseek(fp, dumplo + ok(panicstr), L_SET);
		cp = panic_mesg;
		do
			*cp = getc(fp);
		while (*cp++ && cp < &panic_mesg[sizeof(panic_mesg)]);
	}
	/* Don't fclose(fp), we use dumpfd later. */
}

void
clear_dump()
{
	long newdumplo;

	newdumplo = 0;
	Lseek(dumpfd, (off_t)(dumplo + ok(dump_nl[X_DUMPMAG].n_value)), L_SET);
	Write(dumpfd, &newdumplo, sizeof(newdumplo));
}

int
dump_exists()
{
	int newdumpmag;

	Lseek(dumpfd, (off_t)(dumplo + ok(dump_nl[X_DUMPMAG].n_value)), L_SET);
	(void)Read(dumpfd, &newdumpmag, sizeof(newdumpmag));
	if (newdumpmag != dumpmag) {
		if (verbose)
			syslog(LOG_WARNING, "magic number mismatch (%x != %x)",
			    newdumpmag, dumpmag);
		syslog(LOG_WARNING, "no core dump");
		return (0);
	}
	return (1);
}


char *
rawname(s)
	char *s;
{
	char *sl, name[MAXPATHLEN];

	if ((sl = strrchr(s, '/')) == NULL || sl[1] == '0') {
		syslog(LOG_ERR,
		    "can't make raw dump device name from %s", s);
		return (s);
	}
	(void)snprintf(name, sizeof(name), "%.*s/r%s", sl - s, s, sl + 1);
	if ((sl = strdup(name)) == NULL) {
		syslog(LOG_ERR, "%s", strerror(errno));
		exit(1);
	}
	return (sl);
}

int
get_crashtime()
{
	time_t clobber = (time_t)0;

	if (dumpdev == NODEV) {
		return (0);
	}
	if (system) {
		return (1);
	}
	dumpfd = Open(ddname, 2);
	Lseek(dumpfd, (off_t)(dumplo + ok(dump_nl[X_TIME].n_value)), L_SET);
	(void)Read(dumpfd, &dumptime, sizeof(dumptime));
	if (!debug) {
		Lseek(dumpfd, (off_t)(dumplo + ok(dump_nl[X_TIME].n_value)), L_SET);
		Write(dumpfd, (char *)&clobber, sizeof clobber);
	}
	close(dumpfd);
	if (dumptime == 0) {
		if (verbose || debug)
			syslog(LOG_ERR, "dump time is zero");
		return (0);
	}
	(void)printf("savecore: system went down at %s", ctime(&dumptime));
#define	LEEWAY	(7 * SECSPERDAY)
	if (dumptime < now - LEEWAY || dumptime > now + LEEWAY) {
		(void)printf("dump time is unreasonable\n");
		return (0);
	}
	if (dumptime > now) {
		printf("Time was lost: was %s\n", ctime(&now));
		if (boottime != 0) {
			struct timeval tp;
			now = now - boottime + dumptime;
			tp.tv_sec = now;
			tp.tv_usec = 0;
			if (settimeofday(&tp, (struct timezone*) NULL)) {
				printf("\t-- resetting clock to %s\n", ctime(&now));
			}
		}
	}
	return (1);
}

char *
path(file)
	char *file;
{
	register char *cp = malloc(strlen(file) + strlen(dirname) + 2);

	(void) strcpy(cp, dirname);
	(void) strcat(cp, "/");
	(void) strcat(cp, file);
	return (cp);
}

char buf[1024 * 1024];

int
read_number(fn)
	char *fn;
{
	register FILE *fp;
	register int bounds;

	fn = path(fn);
	if ((fp = fopen(fn, "r")) == NULL) {
		syslog(LOG_WARNING, "%s: %s", fn, strerror(errno));
		bounds = 0;
	}
	if (fgets(buf, sizeof(buf), fp) == NULL) {
		if (ferror(fp)){
			syslog(LOG_WARNING, "%s: %s", fn, strerror(errno));
		}
		bounds = 0;
	} else {
		bounds = atoi(buf);
	}
	if (fp != NULL) {
		fclose(fp);
	}
	if ((fp = fopen(fn, "w")) == NULL) {
		syslog(LOG_ERR, "%s: %m", fn);
	} else {
		fprintf(fp, "%d\n", bounds + 1);
		fclose(fp);
	}
	fclose(fp);
	return (bounds);
}

void
save_core()
{
	register FILE *fp;
	register int bounds, ifd, nr, nw, ofd;
	char *rawp, cpath[MAXPATHLEN];

	/*
	 * Get the current number and update the bounds file.  Do the update
	 * now, because may fail later and don't want to overwrite anything.
	 */
	cpath = path(cpath);
	bounds = read_number("bounds");

	/* Create the core file. */
	(void)snprintf(cpath, sizeof(cpath), "%s/vmcore.%d%s", dirname, bounds, compress ? ".Z" : "");
	if (compress) {
		if ((fp = zopen(cpath, "w", 0)) == NULL) {
			syslog(LOG_ERR, "%s: %s", cpath, strerror(errno));
			exit(1);
		}
	} else {
		ofd = Create(cpath, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	}

	/* Open the raw device. */
	rawp = rawname(ddname);
	if ((ifd = open(rawp, O_RDONLY)) == -1) {
		syslog(LOG_WARNING, "%s: %m; using block device", rawp);
		ifd = dumpfd;
	}

	/* Read the dump size. */
	Lseek(dumpfd, (off_t)(dumplo + ok(dump_nl[X_DUMPSIZE].n_value)), L_SET);
	(void)Read(dumpfd, &dumpsize, sizeof(dumpsize));

	/* Seek to the start of the core. */
	Lseek(ifd, (off_t)dumplo, L_SET);

	/* Copy the core file. */
	dumpsize *= NBPG;
	syslog(LOG_NOTICE, "writing %score to %s", compress ? "compressed " : "", path);
	for (; dumpsize > 0; dumpsize -= nr) {
		(void)printf("%6dK\r", dumpsize / 1024);
		(void)fflush(stdout);
		nr = read(ifd, buf, MIN(dumpsize, sizeof(buf)));
		if (nr <= 0) {
			if (nr == 0)
				syslog(LOG_WARNING, "WARNING: EOF on dump device");
			else
				syslog(LOG_ERR, "%s: %m", rawp);
			goto err2;
		}
		if (compress)
			nw = fwrite(buf, 1, nr, fp);
		else
			nw = write(ofd, buf, nr);
		if (nw != nr) {
			syslog(LOG_ERR, "%s: %s",
			    path, strerror(nw == 0 ? EIO : errno));
err2:			syslog(LOG_WARNING, "WARNING: vmcore may be incomplete");
			(void) printf("\n");
			exit(1);
		}
	}
	(void) printf("\n");
	(void) close(ifd);
	if (compress)
		(void)fclose(fp);
	else
		(void)close(ofd);

	/* Copy the kernel. */
	ifd = Open(system ? system : _PATH_UNIX, O_RDONLY);
	(void) snprintf(path, sizeof(path), "%s/vmunix.%d%s", dirname, bounds, compress ? ".Z" : "");
	if (compress) {
		if ((fp = zopen(path, "w", 0)) == NULL) {
			syslog(LOG_ERR, "%s: %s", path, strerror(errno));
			exit(1);
		}
	} else {
		ofd = Create(path, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	}
	syslog(LOG_NOTICE, "writing %skernel to %s", compress ? "compressed " : "", path);
	while ((nr = read(ifd, buf, sizeof(buf))) > 0) {
		if (compress)
			nw = fwrite(buf, 1, nr, fp);
		else
			nw = write(ofd, buf, nr);
		if (nw != nr) {
			syslog(LOG_ERR, "%s: %s", path, strerror(nw == 0 ? EIO : errno));
			syslog(LOG_WARNING, "WARNING: system may be incomplete");
			exit(1);
		}
	}
	if (nr < 0) {
		syslog(LOG_ERR, "%s: %s", system ? system : _PATH_UNIX, strerror(errno));
		syslog(LOG_WARNING, "WARNING: system may be incomplete");
		exit(1);
	}
	if (compress)
		(void) fclose(fp);
	else
		(void) close(ofd);
}

char *days[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

char *months[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep",
	"Oct", "Nov", "Dec"
};

void
log_entry()
{
	FILE *fp;
	struct tm *tm, *localtime();

	tm = localtime(&now);
	fp = fopen(SHUTDOWNLOG, "a");
	if (fp == 0)
		return;
	fseek(fp, 0L, 2);
	fprintf(fp, "%02d:%02d  %s %s %2d, %4d.  Reboot", tm->tm_hour,
		tm->tm_min, days[tm->tm_wday], months[tm->tm_mon],
		tm->tm_mday, tm->tm_year + 1900);
	if (panicstr)
		fprintf(fp, " after panic:  %s\n", panic_mesg);
	else
		putc('\n', fp);
	fclose(fp);
}

int
check_space()
{
	register FILE *fp;
	char *tvmunix;
	off_t minfree, spacefree, vmunixsize, needed;
	struct stat st;
	struct statfs fsbuf;
	char buf[100], path[MAXPATHLEN];

	tvmunix = system ? system : _PATH_UNIX;
	if (stat(tvmunix, &st) < 0) {
		syslog(LOG_ERR, "%s: %m", tvmunix);
		exit(1);
	}
	vmunixsize = st.st_blocks * S_BLKSIZE;
	if (statfs(dirname, &fsbuf) < 0) {
		syslog(LOG_ERR, "%s: %m", dirname);
		exit(1);
	}
 	spacefree = (fsbuf.f_bavail * fsbuf.f_bsize) / 1024;

	(void)snprintf(path, sizeof(path), "%s/minfree", dirname);
	if ((fp = fopen(path, "r")) == NULL)
		minfree = 0;
	else {
		if (fgets(buf, sizeof(buf), fp) == NULL)
			minfree = 0;
		else
			minfree = atoi(buf);
		(void)fclose(fp);
	}

	needed = (dumpsize + vmunixsize) / 1024;
 	if (minfree > 0 && spacefree - needed < minfree) {
		syslog(LOG_WARNING, "no dump, not enough free space on device");
		return (0);
	}
	if (spacefree - needed < minfree)
		syslog(LOG_WARNING, "dump performed, but free space threshold crossed");
	return (1);
}

/*
 * Versions of std routines that exit on error.
 */

int
Open(name, rw)
	char *name;
	int rw;
{
	int fd;

	if ((fd = open(name, rw)) < 0) {
		syslog(LOG_ERR, "%s: %m", name);
		exit(1);
	}
	return fd;
}

int
Read(fd, buff, size)
	int fd, size;
	char *buff;
{
	int ret;

	if ((ret = read(fd, buff, size)) < 0) {
		syslog(LOG_ERR, "read: %m");
		exit(1);
	}
	return ret;
}

off_t
Lseek(fd, off, flag)
	int fd, flag;
	off_t off;
{
	off_t ret;

	if ((ret = lseek(fd, off, flag)) == -1L) {
		syslog(LOG_ERR, "lseek: %m");
		exit(1);
	}
	return ret;
}

int
Create(file, mode)
	char *file;
	int mode;
{
	register int fd;

	if ((fd = creat(file, mode)) < 0) {
		syslog(LOG_ERR, "%s: %m", file);
		exit(1);
	}
	return fd;
}

void
Write(fd, buf, size)
	int fd, size;
	char *buf;

{
	if (write(fd, buf, size) < size) {
		syslog(LOG_ERR, "write: %s", strerror(n == -1 ? errno : EIO));
		exit(1);
	}
}

void
usage()
{
	(void)syslog(LOG_ERR, "usage: savecore [-cfvz] [-N system] directory");
	exit(1);
}

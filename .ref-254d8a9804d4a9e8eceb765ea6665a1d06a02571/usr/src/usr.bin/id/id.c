/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1991, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)id.c	8.3 (Berkeley) %G%";
#endif /* not lint */

#include <sys/param.h>

#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void	current __P((void));
void	err __P((const char *, ...));
void	pretty __P((struct passwd *));
void	group __P((struct passwd *, int));
void	usage __P((void));
void	user __P((struct passwd *));
struct passwd *
	who __P((char *));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct group *gr;
	struct passwd *pw;
	int Gflag, ch, gflag, id, nflag, pflag, rflag, uflag;

	Gflag = gflag = nflag = pflag = rflag = uflag = 0;
	while ((ch = getopt(argc, argv, "Ggnpru")) != EOF)
		switch(ch) {
		case 'G':
			Gflag = 1;
			break;
		case 'g':
			gflag = 1;
			break;
		case 'n':
			nflag = 1;
			break;
		case 'p':
			pflag = 1;
			break;
		case 'r':
			rflag = 1;
			break;
		case 'u':
			uflag = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	switch(Gflag + gflag + pflag + uflag) {
	case 1:
		break;
	case 0:
		if (!nflag && !rflag)
			break;
		/* FALLTHROUGH */
	default:
		usage();
	}

	pw = *argv ? who(*argv) : NULL;

	if (gflag) {
		id = pw ? pw->pw_gid : rflag ? getgid() : getegid();
		if (nflag && (gr = getgrgid(id)))
			(void)printf("%s\n", gr->gr_name);
		else
			(void)printf("%u\n", id);
		exit(0);
	}

	if (uflag) {
		id = pw ? pw->pw_uid : rflag ? getuid() : geteuid();
		if (nflag && (pw = getpwuid(id)))
			(void)printf("%s\n", pw->pw_name);
		else
			(void)printf("%u\n", id);
		exit(0);
	}

	if (Gflag) {
		group(pw, nflag);
		exit(0);
	}

	if (pflag) {
		pretty(pw);
		exit(0);
	}

	if (pw)
		user(pw);
	else
		current();
	exit(0);
}

void
pretty(pw)
	struct passwd *pw;
{
	struct group *gr;
	u_int eid, rid;
	char *login;

	if (pw) {
		(void)printf("uid\t%s\n", pw->pw_name);
		(void)printf("groups\t");
		group(pw, 1);
	} else {
		if ((login = getlogin()) == NULL)
			err("getlogin: %s", strerror(errno));

		pw = getpwuid(rid = getuid());
		if (pw == NULL || strcmp(login, pw->pw_name))
			(void)printf("login\t%s\n", login);
		if (pw)
			(void)printf("uid\t%s\n", pw->pw_name);
		else
			(void)printf("uid\t%u\n", rid);
		
		if ((eid = geteuid()) != rid)
			if (pw = getpwuid(eid))
				(void)printf("euid\t%s", pw->pw_name);
			else
				(void)printf("euid\t%u", eid);
		if ((rid = getgid()) != (eid = getegid()))
			if (gr = getgrgid(rid))
				(void)printf("rgid\t%s\n", gr->gr_name);
			else
				(void)printf("rgid\t%u\n", rid);
		(void)printf("groups\t");
		group(NULL, 1);
	}
}

void
current()
{
	struct group *gr;
	struct passwd *pw;
	int cnt, id, eid, lastid, ngroups;
	gid_t groups[NGROUPS];
	char *fmt;

	id = getuid();
	(void)printf("uid=%u", id);
	if (pw = getpwuid(id))
		(void)printf("(%s)", pw->pw_name);
	if ((eid = geteuid()) != id) {
		(void)printf(" euid=%u", eid);
		if (pw = getpwuid(eid))
			(void)printf("(%s)", pw->pw_name);
	}
	id = getgid();
	(void)printf(" gid=%u", id);
	if (gr = getgrgid(id))
		(void)printf("(%s)", gr->gr_name);
	if ((eid = getegid()) != id) {
		(void)printf(" egid=%u", eid);
		if (gr = getgrgid(eid))
			(void)printf("(%s)", gr->gr_name);
	}
	if (ngroups = getgroups(NGROUPS, groups)) {
		for (fmt = " groups=%u", lastid = -1, cnt = 0; cnt < ngroups;
		    fmt = ", %u", lastid = id) {
			id = groups[cnt++];
			if (lastid == id)
				continue;
			(void)printf(fmt, id);
			if (gr = getgrgid(id))
				(void)printf("(%s)", gr->gr_name);
		}
	}
	(void)printf("\n");
}

void
user(pw)
	register struct passwd *pw;
{
	register struct group *gr;
	register char *fmt, **p;
	int cnt, id, lastid, ngroups, groups[NGROUPS + 1];

	id = pw->pw_uid;
	(void)printf("uid=%u(%s)", id, pw->pw_name);
	(void)printf(" gid=%u", pw->pw_gid);
	if (gr = getgrgid(pw->pw_gid))
		(void)printf("(%s)", gr->gr_name);
	ngroups = NGROUPS + 1;
	(void) getgrouplist(pw->pw_name, pw->pw_gid, groups, &ngroups);
	fmt = " groups=%u";
	for (lastid = -1, cnt = 0; cnt < ngroups; ++cnt) {
		if (lastid == (id = groups[cnt]))
			continue;
		(void)printf(fmt, id);
		fmt = " %u";
		if (gr = getgrgid(id))
			(void)printf("(%s)", gr->gr_name);
		lastid = id;
	}
	(void)printf("\n");
}

void
group(pw, nflag)
	struct passwd *pw;
	int nflag;
{
	struct group *gr;
	int cnt, id, lastid, ngroups;
	gid_t groups[NGROUPS + 1];
	char *fmt;

	if (pw) {
		ngroups = NGROUPS + 1;
		(void) getgrouplist(pw->pw_name, pw->pw_gid, groups, &ngroups);
	} else {
		groups[0] = getgid();
		ngroups = getgroups(NGROUPS, groups + 1) + 1;
	}
	fmt = nflag ? "%s" : "%u";
	for (lastid = -1, cnt = 0; cnt < ngroups; ++cnt) {
		if (lastid == (id = groups[cnt]))
			continue;
		if (nflag) {
			if (gr = getgrgid(id))
				(void)printf(fmt, gr->gr_name);
			else
				(void)printf(*fmt == ' ' ? " %u" : "%u",
				    id);
			fmt = " %s";
		} else {
			(void)printf(fmt, id);
			fmt = " %u";
		}
		lastid = id;
	}
	(void)printf("\n");
}

struct passwd *
who(u)
	char *u;
{
	struct passwd *pw;
	long id;
	char *ep;

	/*
	 * Translate user argument into a pw pointer.  First, try to
	 * get it as specified.  If that fails, try it as a number.
	 */
	if (pw = getpwnam(u))
		return(pw);
	id = strtol(u, &ep, 10);
	if (*u && !*ep && (pw = getpwuid(id)))
		return(pw);
	err("%s: No such user", u);
	/* NOTREACHED */
}

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

void
#if __STDC__
err(const char *fmt, ...)
#else
err(fmt, va_alist)
	char *fmt;
        va_dcl
#endif
{
	va_list ap;
#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	(void)fprintf(stderr, "id: ");
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)fprintf(stderr, "\n");
	exit(1);
	/* NOTREACHED */
}

void
usage()
{
	(void)fprintf(stderr, "usage: id [user]\n");
	(void)fprintf(stderr, "       id -G [-n] [user]\n");
	(void)fprintf(stderr, "       id -g [-nr] [user]\n");
	(void)fprintf(stderr, "       id -u [-nr] [user]\n");
	exit(1);
}

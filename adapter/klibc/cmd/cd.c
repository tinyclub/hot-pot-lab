/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1997-2005
 *	Herbert Xu <herbert@gondor.apana.org.au>.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

/*
 * The cd and pwd commands.
 */
#include "shell.h"
#include "errno.h"
#include "mystring.h"
//#include "var.h"

#if 0
#include "nodes.h"	/* for jobs.h */
#include "jobs.h"
#include "options.h"
#include "output.h"
#include "memalloc.h"
#include "error.h"
#include "exec.h"
#include "redir.h"
#include "main.h"

#include "show.h"
#include "cd.h"
#endif


/*
 * Actually do the chdir.  We also call hashcd to let the routines in exec.c
 * know that the current directory has changed.
 */

STATIC int
docd(const char *dest, int flags)
{
	int err;

	TRACE(("docd(\"%s\", %d) called\n", dest, flags));

	err = chdir(dest);
	if (err)
		goto out;

out:
	return err;
}

extern void sh_error(const char *msg, ...);
int
cd_main(int argc, char *argv[])
{
	const char *dest;
	char c;
	int flags = 0;
	
	dest = NULL;
	if (argc > 1)
		dest = argv[1];

	if (*dest == '/')
		goto step6;
	if (*dest == '.') {
		c = dest[1];
dotdot:
		switch (c) {
		case '\0':
		case '/':
			goto step6;
		case '.':
			c = dest[2];
			if (c != '.')
				goto dotdot;
		}
	}
	if (!*dest)
		dest = ".";
step6:
	if (!docd(dest, flags))
		goto out;

	sh_error("can't cd to %s", dest);
	/* NOTREACHED */
out:
	return 0;
}

extern void
sh_warnx(const char *fmt, ...);
/*
 * Find out what the current directory is. If we already know the current
 * directory, this routine returns immediately.
 */
inline
STATIC char *
getpwd(void)
{
	char *buf;
	buf = malloc(PATH_MAX);
	if (buf != NULL)
	{
		if(getcwd(buf, sizeof(buf)))
		{
			return buf;
		}
		else
		{
			free(buf);
		}
	}

	sh_warnx("getcwd() failed: %s", strerror(errno));
	return NULL;
}

extern int printk(const char *fmt, ...);

#define out1fmt printk
int
pwd_main(int argc, char **argv)
{
	char *dir = getpwd();
	
	out1fmt(snlfmt, dir ? dir : "");
	if (dir != NULL)
	{
		free(dir);
	}

	return 0;
}



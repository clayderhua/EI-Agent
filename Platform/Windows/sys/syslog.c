/*
 * Copyright (c) 1983, 1988, 1993
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

/*
 * SYSLOG -- print message on log file
 *
 * This routine looks a lot like printf, except that it outputs to the
 * log file instead of the standard output.  Also:
 *	adds a timestamp,
 *	prints the module name in front of the message,
 *	has some other formatting types (or will sometime),
 *	adds a newline on the end of the message.
 *
 * The output of this routine is intended to be read by syslogd(8).
 *
 * Author: Eric Allman
 * Modified to use UNIX domain IPC by Ralph Campbell
 * Patched March 12, 1996 by A. Ian Vogelesang <vogelesang@hdshq.com>
 *  - to correct the handling of message & format string truncation,
 *  - to visibly tag truncated records to facilitate
 *    investigation of such Bad Things with grep, and,
 *  - to correct the handling of case where "write"
 *    returns after writing only part of the message.
 * Rewritten by Martin Mares <mj@atrey.karlin.mff.cuni.cz> on May 14, 1997
 *  - better buffer overrun checks.
 *  - special handling of "%m" removed as we use GNU sprintf which handles
 *    it automatically.
 *  - Major code cleanup.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <stdlib.h>

// Windows
#include <winsock2.h>

// Platform
#include <errno.h>
#include <pthread.h>


#include "syslog.h"

#define SYSLOG_HOSTNAME "127.0.0.1"
#define SYSLOG_PORT 514

static pthread_mutex_t mylock;
static struct sockaddr_in serveraddr;
static int serverlen = 0;
static char tbuf[2048]; /* syslogd is unable to handle longer messages */

/* !glibc_compat: glibc uses argv[0] by default
 * (default: if there was no openlog or if openlog passed NULL),
 * not string "syslog"
 */
static const char *LogTag = "syslog";   /* string to tag the entry with */
static int       LogFile = -1;          /* fd for log */
static unsigned int connected;             /* have done connect */
/* all bits in option argument for openlog fit in 8 bits */
static unsigned int LogStat = 0;           /* status bits, set by openlog */
/* default facility code if openlog is not called */
/* (this fits in 8 bits even without >> 3 shift, but playing extra safe) */
static unsigned int LogFacility = LOG_USER >> 3;
/* bits mask of priorities to be logged (eight prios - 8 bits is enough) */
static unsigned int LogMask = 0xff;

static void
closelog_intern(int sig)
{
	/* mylock must be held by the caller */
	if (LogFile != -1) {
		(void) closesocket(LogFile);
	}
	LogFile = -1;
	connected = 0;
	if (sig == 0) { /* called from closelog()? - reset to defaults */
		LogStat = 0;
		LogTag = "syslog";
		LogFacility = LOG_USER >> 3;
		LogMask = 0xff;
	}
	
	WSACleanup();
}

static void
openlog_intern(const char *ident, int logstat, int logfac)
{
	int fd;
	int logType = SOCK_DGRAM; // UDP
	struct hostent *server;
	int rc;
	u_long non_blocking = 1;

	if (ident != NULL)
		LogTag = ident;
	LogStat = logstat;
	/* (we were checking also for logfac != 0, but it breaks
	 * openlog(xx, LOG_KERN) since LOG_KERN == 0) */
	if ((logfac & ~LOG_FACMASK) == 0) /* if we don't have invalid bits */
		LogFacility = (unsigned)logfac >> 3;

	fd = LogFile;
	if (fd == -1) {
		if (logstat & LOG_NDELAY) {
			WSADATA wsaData;
			WSAStartup(MAKEWORD(2, 2), &wsaData);
			LogFile = fd = socket(AF_INET, logType, 0);
			if (fd == INVALID_SOCKET) 
				fprintf(stderr, "socket function failed with error = %d\n", WSAGetLastError() );

			if (fd == -1) {
				return;
			}
			
			rc = ioctlsocket(fd, FIONBIO, &non_blocking);
			if (rc != NO_ERROR)
				fprintf(stderr, "ioctlsocket failed with error: %ld\n", rc);
		}
	}

	server = gethostbyname(SYSLOG_HOSTNAME);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", SYSLOG_HOSTNAME);
        exit(0);
    }
    /* build the server's Internet address */	
    memset((char *) &serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(SYSLOG_PORT);
	serveraddr.sin_addr.s_addr = *(u_long *) server->h_addr_list[0];
	serverlen = sizeof(serveraddr);
	connected = 1;
}

/*
 * OPENLOG -- open system log
 */
WISEPLATFORM_API void
openlog(const char *ident, int logstat, int logfac)
{
	pthread_mutex_init(&mylock, NULL);
	pthread_mutex_lock(&mylock);
	openlog_intern(ident, logstat, logfac);
	pthread_mutex_unlock(&mylock);
}

/*
 * syslog, vsyslog --
 *     print message on log file; output is intended for syslogd(8).
 */
static void
__vsyslog(int pri, const char *fmt, va_list ap)
{
	register char *p;
	char *last_chr, *head_end, *end, *stdp;
	time_t now;
	int saved_errno;
	int rc;

	/* Just throw out this message if pri has bad bits. */
	if ((pri & ~(LOG_PRIMASK|LOG_FACMASK)) != 0)
		return;

	saved_errno = errno;

	pthread_mutex_lock(&mylock);

	/* See if we should just throw out this message according to LogMask. */
	if ((LogMask & LOG_MASK(LOG_PRI(pri))) == 0)
		goto getout;
	if (LogFile < 0 || !connected)
		openlog_intern(NULL, LogStat | LOG_NDELAY, (int)LogFacility << 3);

	/* Set default facility if none specified. */
	if ((pri & LOG_FACMASK) == 0)
		pri |= ((int)LogFacility << 3);

	/* Build the message. We know the starting part of the message can take
	 * no longer than 64 characters plus length of the LogTag. So it's
	 * safe to test only LogTag and use normal sprintf everywhere else.
	 */
	(void)time(&now);
	stdp = p = tbuf + sprintf(tbuf, "<%d>%.15s ", pri, ctime(&now) + 4);
	/*if (LogTag) - always true */ {
		if (strlen(LogTag) < sizeof(tbuf) - 64)
			p += sprintf(p, "%s", LogTag);
		else
			p += sprintf(p, "<BUFFER OVERRUN ATTEMPT>");
	}
	if (LogStat & LOG_PID)
		p += sprintf(p, "[%d]", getpid());
	/*if (LogTag) - always true */ {
		*p++ = ':';
		*p++ = ' ';
	}
	head_end = p;

	/* We format the rest of the message. If the buffer becomes full, we mark
	 * the message as truncated. Note that we require at least 2 free bytes
	 * in the buffer as we might want to add "\r\n" there.
	 */

	end = tbuf + sizeof(tbuf) - 1;
	errno = saved_errno;
	p += vsnprintf(p, end - p, fmt, ap);
	if (p >= end || p < head_end) {	/* Returned -1 in case of error... */
		static char truncate_msg[12]; /* no NUL! */
		memcpy(truncate_msg, "[truncated] ", sizeof(truncate_msg));
		memmove(head_end + sizeof(truncate_msg), head_end,
				end - head_end - sizeof(truncate_msg));
		memcpy(head_end, truncate_msg, sizeof(truncate_msg));
		if (p < head_end) {
			while (p < end && *p) {
				p++;
			}
		}
		else {
			p = end - 1;
		}

	}
	last_chr = p;

	/* Output to stderr if requested. */
	if (LogStat & LOG_PERROR) {
		*last_chr = '\n';
		(void)_write(STDERR_FILENO, stdp, last_chr - stdp + 1);
	}

	/* Output the message to the local logger using NUL as a message delimiter. */
	p = tbuf;
	*last_chr = '\0';
	if (LogFile >= 0) {
		do {
			/* can't just use write, it can result in SIGPIPE */
			rc = sendto(LogFile, p, last_chr + 1 - p, 0, (struct sockaddr *)&serveraddr, serverlen);
			if (rc == SOCKET_ERROR) {
				fprintf(stderr, "sendto failed with error: %d\n", WSAGetLastError());
				/* I don't think looping forever on EAGAIN is a good idea.
				 * Imagine that syslogd is SIGSTOPed... */
				if (/* (errno != EAGAIN) && */ (errno != EINTR)) {
					closelog_intern(1); /* 1: do not reset LogXXX globals to default */
					goto write_err;
				}
				rc = 0;
			}
			p += rc;
		} while (p <= last_chr);
		goto getout;
	}

 write_err:
	/*
	 * Output the message to the console; don't worry about blocking,
	 * if console blocks everything will.  Make sure the error reported
	 * is the one from the syslogd failure.
	 */
	/* should mode be O_WRONLY | O_NOCTTY? -- Uli */
	/* yes, but in Linux "/dev/console" never becomes ctty anyway -- vda */
	if (LogStat & LOG_CONS) {
		p = strchr(tbuf, '>') + 1;
		last_chr[0] = '\r';
		last_chr[1] = '\n';

		fprintf(stderr, "%s\n", p);
	}

 getout:
	pthread_mutex_unlock(&mylock);
}

WISEPLATFORM_API void 
syslog(int pri, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	__vsyslog(pri, fmt, ap);
	va_end(ap);
}

/*
 * CLOSELOG -- close the system log
 */
WISEPLATFORM_API void 
closelog(void)
{
	pthread_mutex_lock(&mylock);
	closelog_intern(0); /* 0: reset LogXXX globals to default */
	pthread_mutex_unlock(&mylock);
	pthread_mutex_destroy(&mylock);
}

/* setlogmask -- set the log mask level */
WISEPLATFORM_API int 
setlogmask(int pmask)
{
	int omask;

	omask = LogMask;
	if (pmask != 0) {
/*		pthread_mutex_lock(&mylock);*/
		LogMask = pmask;
/*		pthread_mutex_unlock(&mylock);*/
	}
	return omask;
}

#if 0
void main()
{
	setlogmask (LOG_UPTO (LOG_NOTICE));
	openlog ("terry.log", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
	syslog (LOG_NOTICE, "Hello world");
	//syslog (LOG_INFO, "A tree falls in a forest");
	closelog ();
}
#endif
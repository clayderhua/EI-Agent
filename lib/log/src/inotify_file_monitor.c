#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <pthread.h>
#include <linux/limits.h>
#include <string.h>

#define LOG_TAG "LOGC"
#include "Log.h"

void* file_monitor_thread(void *arg)
{
	int fd, wd = -1;
	char buf[4096];
	const struct inotify_event *event;
	int len;
	char *ptr;
	static uint32_t mask = IN_CREATE | IN_DELETE | IN_MODIFY | IN_CLOSE_WRITE;
	file_monitor_st* fm = (file_monitor_st*) arg;
	char filename[PATH_MAX];

	if (strlen(fm->file) >= PATH_MAX) {
		LOGE("filename buffer is not engouth, %d, %d\n", strlen(fm->file), PATH_MAX);
		return NULL;
	}

	fd = inotify_init();
	if (fd == -1) {
		LOGE("inotify_init1 fail, errno=%d", errno);
		return NULL;
	}

	// find parent folder
	ptr = strrchr(fm->file, FILE_SEPARATOR);
	if (ptr == NULL) {
		strcpy(filename, ".");
	} else {
		strncpy(filename, fm->file, ptr - fm->file);
		filename[ptr - fm->file] = '\0';
	}
	// add to notify
	wd = inotify_add_watch(fd, filename, mask);
	if (wd == -1) {
		LOGE("Cannot watch '%s': errno=%d", "log.ini", errno);
		return NULL;
	}

	// find log file name
	strcpy(filename, ptr+1);

	while (fm->running) {
		len = read(fd, buf, sizeof(buf));
		if (len == -1 && errno != EAGAIN) { // error
			perror("read");
			return NULL;
		}

		if (len <= 0) // EOF
			break;

		for (ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len) {
			event = (const struct inotify_event *) ptr;
			if (event->wd != wd) {
				continue;
			}
			if (event->len == 0 || strcmp(event->name, filename) != 0) {
				continue;
			}
			if (event->mask & (IN_CLOSE_WRITE|IN_MODIFY)) {
				if (fm->fun) {
					fm->fun(arg);
				}
				continue;
			}
		}
	}
	close(fd);

	LOGE("file_monitor_thread end");
	return 0;
}

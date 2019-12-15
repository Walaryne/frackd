#include <errno.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <stdalign.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <syslog.h>

#define MAX_EVENTS 16
#define MAX_WATCH_DESCRIPTORS 512

#ifdef USE_SYSLOG

#define WARN(...) syslog(LOG_DAEMON | LOG_INFO, __VA_ARGS__)

#else

#define WARN(...) dprintf(STDERR_FILENO, "FRACKD: " __VA_ARGS__)

#endif

struct wdexpair {
	int wd;
	char **executable;
};

int readfrackrc(char **paths, char **executables) {
	int watchcount = 0;
	FILE *file = NULL;
	char *home = getenv("HOME");
	char *filename = "/.frackrc";
	char path[(strlen(home) + strlen(filename)) + 2];
	char *watch, *executable;

	strcpy(path, home);
	strcat(path, filename);

	if((file = fopen(path, "r")) == NULL) {
		perror("fopen");
		WARN("This error was likely caused " \
			"due to a missing .frackrc in your home folder.\n");
		exit(1);
	}

	while(1) {
		if(watchcount == MAX_WATCH_DESCRIPTORS) {
			WARN("WARNING: Maximum of %d watch(es) has been reached!",
				MAX_WATCH_DESCRIPTORS);
			return watchcount;
		}

		//WARNING!
		//UNFREED ALLOCATIONS!
		//Might be a problem later.
		if((fscanf(file, "%ms %ms", &watch, &executable)) < 2) {
			break;
		}

		paths[watchcount] = watch;
		executables[watchcount] = executable;

		++watchcount;
	}

	return watchcount;
}

void handle_inotify(int infd, struct wdexpair *wep) {
	alignas(struct inotify_event) char buf[4096];
	struct inotify_event *event;
	char *ptr;

	int len = read(infd, &buf, sizeof buf);

	for(ptr = buf; ptr < (buf + len); ptr += sizeof(struct inotify_event) + event->len) {
		event = (struct inotify_event *) ptr;
		if(event->mask & IN_CLOSE_WRITE) {
			//HACK: watch descriptors start at 1, so just do -1!
			//If a rc-reload is ever created for new-style daemon support,
			//a qsort() and bsearch() will be needed!
			system(*(wep[event->wd - 1].executable));
		}
	}
	return;
}


//TODO Write better logging system; stop using perror!
int main(int argc, char **argv) {

	daemon(1, 0);

	WARN("...frackd is starting...\n");

	struct epoll_event ev, events[MAX_EVENTS];
	struct wdexpair wep[MAX_WATCH_DESCRIPTORS];
	int infd, epfd, wpc, wd[MAX_WATCH_DESCRIPTORS];
	char *watchpaths[MAX_WATCH_DESCRIPTORS];
	char *executables[MAX_WATCH_DESCRIPTORS];
	int running = 1;

	if((infd = inotify_init1(IN_NONBLOCK)) == -1) {
		perror("inotify_init1");
		exit(1);
	}

	if((epfd = epoll_create1(0)) == -1) {
		perror("epoll_create1");
		exit(1);
	}

	ev.events = EPOLLIN;
	ev.data.fd = infd;

	if(epoll_ctl(epfd, EPOLL_CTL_ADD, infd, &ev)) {
		perror("epoll_ctl");
		exit(1);
	}

	if((wpc = readfrackrc(watchpaths, executables)) == 0) {
		WARN(".frackrc was malformed or empty\n");
		exit(1);
	}

	for(int temp, i = 0; i < wpc; ++i) {
		temp = inotify_add_watch(infd, watchpaths[i], IN_CLOSE_WRITE);
		wep[i].executable = &executables[i];

		if(temp < 0) {
			perror("inotify_add_watch");
			exit(1);
		}
	}

	while(running) {
		WARN("...frackd successfully started...\n");
		epoll_wait(epfd, events, MAX_EVENTS, -1);

		for(int n = 0; n < MAX_EVENTS; ++n) {
			if(events[n].data.fd == infd) {
				handle_inotify(infd, wep);
			}
		}
	}
}

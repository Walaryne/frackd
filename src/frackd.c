#include <errno.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <stdalign.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>

#define MAX_EVENTS 16
#define MAX_WATCH_DESCRIPTORS 512

struct wdexpair {
	int wd;
	char **executable;
};

int readfrackrc(char **paths, char **executables) {
	int watchcount = 0;

	FILE *file = NULL;
	char *home = getenv("HOME");
	char *filename = "/.frackrc";
	char path[sizeof home + sizeof filename];
	char *watch, *executable;

	strcat(path, home);
	strcat(path, filename);

	if((file = fopen(path, "r")) == NULL) {
		perror("fopen");
		dprintf(STDERR_FILENO, "This error was likely caused " \
			"due to a missing .frackrc in your home folder.\n");
		dprintf(STDERR_FILENO, "%s\n", path);
		exit(1);
	}

	while(1) {
		if(watchcount == MAX_WATCH_DESCRIPTORS) {
			dprintf(STDERR_FILENO, "WARNING: Maximum of %d watch(es) has been reached!",
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
			system(*(wep[event->wd - 1].executable));
		}
	}
	return;
}

int main(int argc, char **argv) {

	//daemon(1, 0);

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

	//TODO Error checking here too!
	epoll_ctl(epfd, EPOLL_CTL_ADD, infd, &ev);

	if((wpc = readfrackrc(watchpaths, executables)) == 0) {
		dprintf(STDERR_FILENO, ".frackrc was malformed or empty\n");
		exit(1);
	}

	for(int i = 0; i < wpc; ++i) {
		wep[i].executable = &executables[i];
		wep[i].wd = inotify_add_watch(infd, watchpaths[i], IN_CLOSE_WRITE);
	}

	while(running) {
		epoll_wait(epfd, events, MAX_EVENTS, -1);

		for(int n = 0; n < MAX_EVENTS; ++n) {
			if(events[n].data.fd == infd) {
				handle_inotify(infd, wep);
			}
		}
	}
}

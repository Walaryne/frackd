#include "frackd.h"

//FIXME This could allow a memory leak if return value isn't checked properly.
char *tildeexpansion(char *str, char *home) {
    unsigned long slen = strlen(str);
    unsigned long hlen = strlen(home);
    char *ptr = memchr(str, '~', slen);
    //Easy check to see if ~ is the first character, and whether it was
    //found in the first place.
    if(ptr != NULL && str == ptr) {
        char *buf = malloc(((slen - 1) + hlen + 2) * sizeof(char));
        ++ptr;
        strcpy(buf, home);
        strcat(buf, ptr);
        return buf;
    } else {
        return str;
    }
}

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
		WARN_PE("fopen");
		WARN_LOG("This error was likely caused " \
			"due to a missing .frackrc in your home folder.\n");
		exit(1);
	}

	while(1) {
		if(watchcount == MAX_WATCH_DESCRIPTORS) {
			WARN_LOG("WARNING: Maximum of %d watch(es) has been reached!\n",
			        MAX_WATCH_DESCRIPTORS);
			return watchcount;
		}

		//WARNING!
		//UNFREED ALLOCATIONS!
		//Might be a problem later.
		if((fscanf(file, "%ms %ms", &watch, &executable)) < 2) {
			break;
		}

		if((paths[watchcount] = tildeexpansion(watch, home)) != watch) {
		    free(watch);
		}

		if((executables[watchcount] = tildeexpansion(executable, home)) != executable) {
		    free(executable);
		}

		++watchcount;
	}

	return watchcount;
}

void handle_inotify(int infd, char **lut, rlim_t lutmax) {
	alignas(struct inotify_event) char buf[4096];
	struct inotify_event *event;
	char *ptr;

	int len = read(infd, &buf, sizeof buf);

	for(ptr = buf; ptr < (buf + len); ptr += sizeof(struct inotify_event) + event->len) {
        event = (struct inotify_event *) ptr;

        if (event->wd > lutmax) {
            WARN_LOG("FATAL ERROR: inotify watch descriptor number busted lookup table size.\n" \
                "This should never happen; if it does, send a bug report asap.\n");
            exit(1);
        }

        if (event->mask & IN_CLOSE_WRITE) {
            //Got rid of the hack, now using a lookup table.
            system(lut[event->wd]);
        }
    }
}

int main(int argc, char **argv) {

	DAEMON(1, 0);

	WARN_LOG("...frackd is starting...\n");

	struct epoll_event ev, events[MAX_EVENTS];
	struct rlimit rl;
	int infd, epfd, wpc;
	char *watchpaths[MAX_WATCH_DESCRIPTORS];
	char *executables[MAX_WATCH_DESCRIPTORS];
	char **lut;
	int lutmax;

	if((infd = inotify_init1(IN_NONBLOCK)) == -1) {
		WARN_PE("inotify_init1");
		exit(1);
	}

	if((epfd = epoll_create1(0)) == -1) {
		WARN_PE("epoll_create1");
		exit(1);
	}

	ev.events = EPOLLIN;
	ev.data.fd = infd;

	if(epoll_ctl(epfd, EPOLL_CTL_ADD, infd, &ev)) {
		WARN_PE("epoll_ctl");
		exit(1);
	}

	if((wpc = readfrackrc(watchpaths, executables)) == 0) {
		WARN_LOG("FATAL ERROR: .frackrc was empty or malformed\n");
		exit(1);
	}

	//TODO Check and see if RLIMIT_NOFILE is a sane source for malloc sizing.
	getrlimit(RLIMIT_NOFILE, &rl);
	lutmax = rl.rlim_cur;
	lut = malloc(lutmax * sizeof lut);

	for(int temp, i = 0; i < wpc; ++i) {
		temp = inotify_add_watch(infd, watchpaths[i], IN_CLOSE_WRITE);

		if(temp < 0) {
			//perror("inotify_add_watch");
			WARN_PE("inotify_add_watch");
			exit(1);
		}

		lut[temp] = executables[i];

		//Watchpaths aren't needed later at the moment, so free them for the sake of extra RAM.
		free(watchpaths[i]);
	}
	
	WARN_LOG("...frackd successfully started...\n");

	while(1) {
		epoll_wait(epfd, events, MAX_EVENTS, -1);

		for(int n = 0; n < MAX_EVENTS; ++n) {
			if(events[n].data.fd == infd) {
				handle_inotify(infd, lut, lutmax);
			}
		}
	}
}

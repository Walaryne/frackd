#include "frackd.h"

//FIXME This could allow a memory leak if return value isn't checked properly.
char *spechandler(char *str, char *home) {
    unsigned long slen = strlen(str);
    unsigned long hlen = strlen(home);
    char quotechar = 0;
    char *tmp = str;

    //Check and see if the first character is a quote of some sort
    //If so, silently ignore it.
    switch(*tmp) {
        case '"':
        case '\'':
        case '`': {
            quotechar = *tmp;
            ++tmp;
        }
        default: {
            break;
        }
    }

    //Easy check to see if ~ is the first character.
    if(*tmp == '~') {
        if(!home) {
            WARN_LOG("HOME environment variable is not defined, tilde expansion is disabled.\n");
            WARN_LOG("Terminating.\n");
            exit(1);
        }

        char *buf = malloc(sizeof(char) * ((slen - 1) + hlen + 4));
        ++tmp;

        //Sloppy, but stops strcpy from clobbering the manually set quote character.
        //TODO Find a better way.
        if(quotechar) {
            *buf = quotechar;
            ++buf;
            strcpy(buf, home);
            strcat(buf, tmp);
            --buf;
        } else {
            strcpy(buf, home);
            strcat(buf, tmp);
        }
        return buf;
    } else {
        return str;
    }
}

int readfrackrc(char **paths, char **executables) {
	int watchcount = 0;
	FILE *file = NULL;
	char *home = getenv("HOME");
	char *hfrc = "/.frackrc";
	char *etfrc = "/etc/frackrc";
	//Oversize the path array, saves extra logic
	char path[(strlen(home) + strlen(hfrc)) + strlen(etfrc)];
	char *watch, *executable;

	if(home) {
        strcpy(path, home);
        strcat(path, hfrc);

        if(access(path, F_OK)) {
            WARN_LOG(".frackrc was not found in home folder, using /etc/frackrc\n");
            memset(path, 0, sizeof path / sizeof path[0]);
            strcpy(path, etfrc);
        }
	} else {
        WARN_LOG("HOME environment variable was not defined, using /etc/frackrc\n");
        strcpy(path, etfrc);
	}

	if((file = fopen(path, "r")) == NULL) {
		WARN_PE("fopen");
		WARN_LOG("This error was likely caused " \
		    "by a missing or unreadable frackrc file.\n");
		WARN_LOG("Path was %s\n", path);
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
		if((fscanf(file, "%m[^:]:%m[^\n]", &watch, &executable)) < 2) {
			break;
		}

		if((paths[watchcount] = spechandler(watch, home)) != watch) {
		    free(watch);
		}

		if((executables[watchcount] = spechandler(executable, home)) != executable) {
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
            WARN_LOG("FATAL ERROR: inotify watch descriptor number busted lookup table size.\n");
            WARN_LOG("This should never happen; if it does, send a bug report asap.\n");
            exit(1);
        }

        if (event->mask & IN_CLOSE_WRITE) {
            //Got rid of the hack, now using a lookup table.
            system(lut[event->wd]);
        }
    }
}

int main(int argc, char **argv) {

    //Check to see if we are root, if so, exit immediately.
    if(!getuid()) {
        WARN_LOG("Running frackd as root is DANGEROUS!\n");
        WARN_LOG("Please rerun using your normal account.\n");
        WARN_LOG("Root support will be added later on!\n");
        exit(1);
    }

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

	if(!(wpc = readfrackrc(watchpaths, executables))) {
		WARN_LOG("FATAL ERROR: .frackrc was empty or malformed\n");
		exit(1);
	}

	//TODO Check and see if RLIMIT_NOFILE is a sane source for malloc sizing.
	getrlimit(RLIMIT_NOFILE, &rl);
	lutmax = rl.rlim_cur;
	lut = malloc(sizeof lut * lutmax);

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

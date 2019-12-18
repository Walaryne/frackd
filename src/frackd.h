#include <errno.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <stdalign.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <syslog.h>

#define MAX_EVENTS 16
#define MAX_WATCH_DESCRIPTORS 512

#ifdef NO_DAEMON

#define DAEMON(x, y)

#else

#define DAEMON(x, y) daemon(x, y)
#define USE_SYSLOG

#endif

#ifdef USE_SYSLOG

#define WARN(...) syslog(LOG_DAEMON | LOG_INFO, __VA_ARGS__)

#else

#define WARN(...) dprintf(STDERR_FILENO, __VA_ARGS__)

#endif

#define WARN_LOG(...) WARN("FRACKD_LOG: " __VA_ARGS__)

#define WARN_PE(x) WARN("FRACKD_ERROR: " x ": " "%s\n", strerror(errno))

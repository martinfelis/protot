#pragma once

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

inline void gGetFileModTime (const char* filename, int *sec, int *nsec) {
	struct stat attr;

	bool stat_result = stat(filename, &attr);
	if (!stat_result) {
		*sec = -1;
		*nsec = -1;
	}

	*sec = attr.st_mtime;
	*nsec = attr.st_mtim.tv_nsec;
}

inline double gGetCurrentTime () {
	struct timespec spec;
	clock_gettime(CLOCK_REALTIME, &spec);

	return static_cast<double>(spec.tv_sec) + spec.tv_nsec * 1.0e-9;
}

extern double gTimeAtStart;

inline double gGetTimeSinceStart () {
	return gGetCurrentTime() - gTimeAtStart;
}

inline void gLog (const char* format, ...) {
	fprintf (stdout, "%11.6f: ", gGetTimeSinceStart());
	va_list argptr;
	va_start(argptr, format);
	vfprintf(stdout, format, argptr);
	va_end(argptr);
	fprintf (stdout, "\n");
}

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdarg.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int32_t mousedX;
	int32_t mousedY;
	int32_t mouseX;
	int32_t mouseY;
	uint8_t mouseButton;
	int32_t mouseScroll;
	char key;
} GuiInputState;

void GuiInputState_Init (GuiInputState* input_state);

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

	return (double)(spec.tv_sec) + spec.tv_nsec * 1.0e-9;
}

extern double gTimeAtStart;

inline double gGetTimeSinceStart () {
	return gGetCurrentTime() - gTimeAtStart;
}

extern FILE *gLogFile;

void LoggingInit();

const int cLogBufferSize = 1024;

inline void gLog (const char* format, ...) {
	assert(gLogFile != NULL);


	fprintf (stdout, "%11.6f: ", gGetTimeSinceStart());
	fprintf (gLogFile,"%11.6f: ", gGetTimeSinceStart());
 
	va_list argptr;

	char buffer[cLogBufferSize];
	va_start(argptr, format);
	vsnprintf(buffer, cLogBufferSize, format, argptr);
	va_end(argptr);
	
	fprintf(stdout, "%s\n", buffer);
	fprintf(gLogFile, "%s\n", buffer);

	fflush(gLogFile);
}

inline float gRandomFloat() {
	return ((float)(rand()) / (float)(RAND_MAX));
}

#ifdef __cplusplus
}
#endif

#include "Utils.h"

#include <fstream>
#include <cstdio>
#include <cstdlib>

FILE *gLogFile = NULL;

void LoggingInit() {
	// check if protot.log exists and rename it to protot.log.1 if found
	std::ifstream source ("protot.log", std::ios::binary);

	if (source.good()) {
		std::ofstream destination ("protot.log.1", std::ios::binary | std::ios::trunc);

		destination << source.rdbuf();
	}

	source.close();

	gLogFile = fopen("protot.log", "w+");
}


#pragma once

#if defined(_WIN32)
#include <process.h>
#define DATADRIVEN_PROCESS_ID() _getpid()
#else
#include <unistd.h>
#define DATADRIVEN_PROCESS_ID() getpid()
#endif

#ifndef __ERROR_H__
#define __ERROR_H__
#include <stdio.h>
#include <assert.h>

extern int activate_debug;
#define ERROR(format, ...) do { fprintf(stderr, format, ##__VA_ARGS__); assert(0); } while (0)

#ifndef NDEBUG
#define WARN(format, ...) do { fprintf(stderr, format, ##__VA_ARGS__); } while (0)
#else
#define WARN(format, ...)
#define NDEBUG_OPCODES
#define NDEBUG_MEMORY
#define NDEBUG_GPU
#define NDEBUG_KEYBOARD
#endif

#ifndef NDEBUG_OPCODES
#define DEBUG_OPCODES(format, ...) do { if (activate_debug) { fprintf(stdout, format, ##__VA_ARGS__); fflush(stdout); } } while (0)
#else
#define DEBUG_OPCODES(format, ...)
#endif

#ifndef NDEBUG_MEMORY
#define DEBUG_MEMORY(format, ...) do { if (activate_debug) { fprintf(stdout, format, ##__VA_ARGS__); fflush(stdout); } } while (0)
#else
#define DEBUG_MEMORY(format, ...)
#endif

#ifndef NDEBUG_GPU
#define DEBUG_GPU(format, ...) do { if (activate_debug) { fprintf(stdout, format, ##__VA_ARGS__); fflush(stdout); } } while (0)
#else
#define DEBUG_GPU(format, ...)
#endif

#ifndef NDEBUG_KEYBOARD
#define DEBUG_KEYBOARD(format, ...) do { if (activate_debug) { fprintf(stdout, format, ##__VA_ARGS__); fflush(stdout); } } while (0)
#else
#define DEBUG_KEYBOARD(format, ...)
#endif

#endif     // __ERROR_H__

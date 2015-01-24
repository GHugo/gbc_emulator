#ifndef __ERROR_H__
#define __ERROR_H__
#include <stdio.h>
#include <assert.h>

#define ERROR(format, ...) do { fprintf(stderr, format, ##__VA_ARGS__); assert(0); } while (0)
#define WARN(format, ...) do { fprintf(stderr, format, ##__VA_ARGS__); } while (0)

extern int activate_debug;
#define DEBUG(format, ...) do { if (activate_debug) { fprintf(stdout, format, ##__VA_ARGS__); fflush(stdout); } } while (0)
#endif     // __ERROR_H__

#ifndef __ERROR_H__
#define __ERROR_H__
#include <stdio.h>
#include <assert.h>

#define ERROR(format, ...) do { fprintf(stderr, format, ##__VA_ARGS__); assert(0); } while (0)
#define WARN(format, ...) do { fprintf(stderr, format, ##__VA_ARGS__); } while (0)
#endif     // __ERROR_H__

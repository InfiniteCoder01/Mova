#pragma once
#include <stdio.h>
#include <stdlib.h>

#ifndef DEBUG
#define MV_LOG(...);
#define MV_WARN(...);
#define MV_ERR(...);
#define MV_FATALERR(...);
#define MV_ASSERT(expr, ...);
#define MV_WASSERT(expr, ...);
#else
// clang-format off
#define MV_LOG(...) { printf("Log %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); }
#define MV_WARN(...) { printf("Warning: "); printf(__VA_ARGS__); printf("\n"); }
#define MV_ERR(...) { fprintf(stderr, "Error %s:%d: ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }
#define MV_FATALERR(...) { MV_ERR(__VA_ARGS__); fflush(stderr); exit(-1); }
#define MV_ASSERT(expr, ...) if (!(expr)) { fprintf(stderr, "Assertion failed at %s:%d: ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); fflush(stderr); exit(-1); }
#define MV_WASSERT(expr, ...) if (!(expr)) { MV_WARN(__VA_ARGS__); }
// clang-format on
#endif

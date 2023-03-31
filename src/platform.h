#pragma once

// #if !defined(NDEBUG) && !defined(DEBUG)
// #define DEBUG
// #endif

#if defined(_WIN32) || defined(_WIN64)
#define __WINDOWS__
#elif defined(__linux__)
#define __LINUX__
#endif

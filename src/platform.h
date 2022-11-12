#pragma once

// #if defined(WIN32) || defined(_WIN32) || defined(__WIN32) || defined(WIN64) || defined(_WIN64) || defined(__WIN64) && !defined(__CYGWIN__)
#if defined(_WIN32) || defined(_WIN64)
#define __WINDOWS__
#endif

#define MVAPI

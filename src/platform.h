#pragma once

#if !defined(NDEBUG)
#define DEBUG
#endif

#if defined(_WIN32) || defined(_WIN64)
#define __WINDOWS__
#endif

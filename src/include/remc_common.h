#ifndef REMC_COMMON_H_
#define REMC_COMMON_H_

#if defined(__linux__)
#  define REMC_PLATFORM_LINUX 1
#  define REMC_PLATFORM_WIN32 0
#elif defined(_WIN32)
#  define REMC_PLATFORM_LINUX 0
#  define REMC_PLATFORM_WIN32 1
#endif

#if REMC_PLATFORM_WIN32
#  include <Windows.h>
#endif

#endif // REMC_COMMON_H_

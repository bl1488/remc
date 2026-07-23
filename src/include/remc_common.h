#ifndef REMC_COMMON_H_
#define REMC_COMMON_H_

#if defined(__linux__)
#  define REMC_PLATFORM_LINUX 1
#  define REMC_PALTFORM_WIN32 0
#elif defined(_WIN32)
#  define REMC_PLATFORM_LINUX 0
#  define REMC_PALTFORM_WIN32 1
#endif

#endif // REMC_COMMON_H_

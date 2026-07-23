#ifndef NET_COMMON_H_
#define NET_COMMON_H_

#include "include/remc_common.h"
#include <sodium/core.h>

#if REMC_PLATFORM_LINUX
#  include <fcntl.h>
#  include <cerrno>
#elif REMC_PALTFORM_WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
#endif

namespace remc::net {

// general port for server/client side
constexpr int GENERAL_PORT = 1488;

inline int GetLastNetError() noexcept {
#if REMC_PLATFORM_LINUX
   return errno;
#else
   return ::WSAGetLastError();
#endif
}

} // namespace remc::net

#endif // NET_COMMON_H_

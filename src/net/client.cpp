#include "client.h"

namespace remc::net {

//
// Client
//
//
void Client::Connect(const char* addr, const char* port) {
   tcp::resolver res(GetExecutor());
   auto endp = res.resolve(addr, port);
   // tmp socket
   tcp::socket sock(GetExecutor());
   asio::connect(sock, endp);
   // init session
   session_ = std::make_shared<SessionClient>(std::move(sock), *pool_);
}

} // namespace remc::net

#include "include/remc-spdlog.h"
#include "net/net-common.h"
#include "net/client.h"
#include "crypto/sk/sk.h"

int main(int argc, char** argv) {
   std::setlocale(LC_ALL, "");
   
   // checking keys table.
   // if returns false, the binary is not patched
   if (!remc::crypto::IsKeyTablePatched()) {
      GlobalLogError("global key table is not patched");
      return 0;
   }
   // init sodium for crypto module
   if (::sodium_init() < 0) {
      GlobalLogError("sodium init error");
      return 0;
   }

   std::string server_addr = "127.0.0.1";
   unsigned short port     = remc::net::GENERAL_PORT;
   if (argc > 1) {
      server_addr = argv[1];
      if (argc > 2)
         port = std::stoi(argv[2]);
   }
   else GlobalLogInfo("start on localhost: {}:{}", server_addr, port);

   // external pool
   asio::thread_pool pool(5);

   remc::net::InternalClient client(pool);   
   try {
      client.Connect(server_addr.c_str(), port);
   }
   catch (const std::exception& ex) {
      GlobalLogError("connection to {} failed: {}", 
         server_addr, ex.what());
      // stop it ourselves cuz the thread_pool 
      // would cause a SEGFAULT
      client.Stop();
      return 0;
   }

   // init session
   auto session = client.GetSession().lock();
   session->Read();

   // test loop
   for (std::string buffer;;) {
      std::cout << "enter a message: ";
      std::getline(std::cin >> std::ws, buffer);
   }

   client.Stop();

   pool.stop();
   pool.join();

   return 0;
}

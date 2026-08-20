#include "include/remc_spdlog.h"
#include "net/server.h"
#include "crypto/secure_key.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
   std::setlocale(LC_ALL, "");

   // init sodium
   if (::sodium_init() < 0) {
      GlobalLogError("sodium init error");
      return 0;
   }

#if 0

   asio::thread_pool pool(1);
   remc::net::Server server(12345, pool);
   
   server.Run();
   if (!server.IsRunning()) {
      std::cout << "server is not running!\n";
      return 0;
   }

   std::string buffer;
   while (true) {
      std::cout << "enter a message: ";
      std::getline(std::cin >> std::ws, buffer);

      if (buffer == "exit")
         break;
      else if (buffer == "3") {
         auto& session = server.BeginList()->get()->SessionListBegin()->second;
         std::cout << session->GetMessageCounter() << '\n'
            << session->KeysInfo().GetKeyAsHexString(session->KeysInfo().GetSharedKey()) << '\n';
      }
   }

   server.Stop();

   pool.join();

#endif

   return 0;
}

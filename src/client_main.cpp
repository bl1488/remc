#include "net/net_common.h"
#include "net/client.h"

#include <iostream>

int main(int argc, char** argv) {
   asio::thread_pool pool(1);

   asio::io_context io;
   auto gwork = asio::make_work_guard(io);
   std::thread t([&io]{
      io.run();
   });

   remc::net::Client client(&io, pool);
   client.Connect("127.0.0.1", std::to_string(remc::net::GENERAL_PORT).c_str());

   auto session = client.GetSession().lock();

   std::cout << "looping..." << '\n';
   while (true) {
      std::cout << "string: ";
      std::string str;
      std::getline(std::cin >> std::ws, str);

      if (str == "exit") break;
      else {
         session->Write(str);
      }
   }

   gwork.reset();
   t.join();

   pool.join();
}

#include "include/remc_spdlog.h"
#include "net/client.h"
#include "net/common.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
   // init global logger
   remc::InitFileConsoleLogger(remc::DEFAULT_GLOBAL_LOGGER_NAME, 
                               remc::DEFAULT_LOGFILE_PATH);
   // init sodium
   if (::sodium_init() < 0) {
      GlobalLogError("sodium init error");
      return 0;
   }

   asio::thread_pool pool(1);

   asio::io_context io;
   auto gwork = asio::make_work_guard(io);
   std::thread t([&io]{
      io.run();
   });

   remc::net::Client client(&io, pool);
   client.Connect("127.0.0.1", 12345);

   auto session = client.GetSession().lock();
   session->Read();
   // handshake
   // send public key
   session->Write(
      remc::net::Packet::FLAG_TYPE_HANDSHAKE, 
      std::string(session->KeysInfo().GetKeyAsString(session->KeysInfo().GetPublicKey())),
      // message_id callback
      [](std::shared_ptr<remc::net::Packet> packet, std::shared_ptr<remc::net::SessionClient> session) {
         if (packet->header.message_size == 32) {
            bool flag = session->KeysInfo().ComputeSharedKey({ packet->payload.data(), packet->header.message_size });
            if (!flag)
                 GlobalLogError("error computing shared key");
            else GlobalLogInfo("shared key successfully computed");
         }
         else GlobalLogError("cant compute shared_key ({} != 32)", packet->header.message_size);
      }
   );

   std::string buffer;
   while (true) {
      std::cout << "enter a message: ";
      std::getline(std::cin >> std::ws, buffer);

      if (buffer == "exit") 
         break;
      else if (buffer == "1") session->Write(remc::net::Packet::FLAG_NO_CRYPTO, "hello world");
      else if (buffer == "2") {
         session->Write(remc::net::Packet::FLAG_TEST_MESSAGE, "hello world 123",
         [](std::shared_ptr<remc::net::Packet> packet, std::shared_ptr<remc::net::SessionClient> session) {
            std::cout << "cb hello world\n";
         });
      }
      else if (buffer == "3") {
         std::cout << "keys: \n" << session->KeysInfo().GetKeyAsHexString(session->KeysInfo().GetPublicKey()) << '\n'
            << session->KeysInfo().GetKeyAsHexString(session->KeysInfo().GetSharedKey()) << '\n';
      }
      else {
         session->Write(0, buffer);
      }
   }

   gwork.reset();
   t.join();

   pool.join();

   return 0;
}

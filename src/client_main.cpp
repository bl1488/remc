#include "include/remc_spdlog.h"
#include "include/remc_utils.h"
#include "net/client.h"
#include "net/packet.h"

#include <exception>

int main(int argc, char** argv) {
   std::setlocale(LC_ALL, "");
   
   // init sodium
   if (::sodium_init() < 0) {
      GlobalLogError("sodium init error");
      return 0;
   }

   std::string server_addr = "127.0.0.1";
   unsigned short port     = 12345;
   // args
   if (argc > 1) {
      server_addr = argv[1];
      if (argc > 2)
         port = std::stoi(argv[2]);
   }

   // external pool for client
   asio::thread_pool pool(3);

   remc::net::InternalClient client(pool);
   try {
      client.Connect(server_addr.c_str(), port);
   }
   catch (const std::exception& ex) {
      GlobalLogError("connection to {} failed\n", server_addr);
      return 0;
   }

   // init session
   auto session = client.GetSession().lock();
   session->Read();
   
   // handshake
   session->Write(
      remc::net::Packet::Handshake, 
      std::string(session->KeysInfo().GetKeyAsString(session->KeysInfo().GetPublicKey())),
      // callback
      // getting server public key here
      [](remc::net::Packet packet, std::shared_ptr<remc::net::SessionClient> session) {
         if (packet.header.message_size == 32) {
            if (!session->KeysInfo().ComputeSharedKey({ packet.payload.data(), packet.header.message_size }))
                 GlobalLogError("error computing shared key");
            else GlobalLogInfo("shared key successfully computed");
         }
         else GlobalLogError("cant compute shared key ({} != 32)", packet.header.message_size);
      }
   );

   // test loop
   for (std::string buffer;;) {
      std::cout << "enter a message: ";
      std::getline(std::cin >> std::ws, buffer);

      if (buffer == "exit") {
         GlobalLogInfo("exiting...");
         break;
      }
      else if (buffer == "1") {
         session->Write(remc::net::Packet::NoCrypto, 
            std::format("no crypto message: {}", std::to_string(remc::utils::Random())));
      }
      else if (buffer == "2") {
         session->Write(
            remc::net::Packet::TestMessage, 
            std::format("hello world: {}", remc::utils::Random()),
            // callback
            [](remc::net::Packet packet, [[maybe_unused]] std::shared_ptr<remc::net::SessionClient> session) {
               std::cout << "version.........: " << packet.header.version      << '\n'
                         << "flags...........: " << packet.header.flags        << '\n'
                         << "timestamp.......: " << packet.header.timestamp    << '\n'
                         << "message-id......: " << packet.header.message_id   << '\n'
                         << "message-size....: " << packet.header.message_size << '\n'
                         << "nonce...........: " << packet.header.nonce        << '\n';
               std::cout << std::format("message.........: '{}'\n", packet.GetPayloadAsString());
            }
         );
      }
      else if (buffer == "3") {
         std::cout << "session information:"                                             << '\n' 
            << session->KeysInfo().GetKeyAsHexString(session->KeysInfo().GetPublicKey()) << '\n'
            << session->KeysInfo().GetKeyAsHexString(session->KeysInfo().GetSharedKey()) << '\n'
            << session->GetMessageCounter()                                              << '\n';
      }
      else {
         if (!session->Write(remc::net::Packet::NoFlags, buffer))
            GlobalLogWarning("Write failed");
      }
   }

   client.Stop();
   
   pool.join();

   return 0;
}

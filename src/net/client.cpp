#include "client.h"
#include "include/remc_spdlog.h"
#include "net/common.h"

namespace remc::net {

//
// SessionClient
//
bool SessionClient::Write(uint32_t flags, std::string_view payload, WriteCallbackType cb) {
   if (payload.size() > global::TCP_PAYLOAD_SIZE_MAX)
      return false;

   std::vector<std::byte> vec(payload.size());
   std::memcpy(vec.data(), payload.data(), payload.size());

   return Write(flags, std::move(vec), std::move(cb));
}

bool SessionClient::Write(uint32_t flags, std::vector<std::byte> payload, WriteCallbackType cb) {
   // invalid payload data
   // not slicing and return false
   if (payload.size() > global::TCP_PAYLOAD_SIZE_MAX)
      return false;

   // post to parent worker context
   asio::post(socket_.get_executor(), 
   [self = this->shared_from_this(), flags, payload = std::move(payload), cb = std::move(cb)] mutable {
      // create packet
      std::size_t message_id = utils::Random<uint64_t>();
      auto packet = CreatePacket(
         payload, 
         1,
         flags, 
         self->message_counter_,
         message_id,
         self->keys_.GetSharedKey()
      );
      if (!packet) {
         auto& err = packet.error();
         GlobalLogDebug("packet creation failed: {}:{}", err.CodeAsString(), err.Message());
         return;
      }
      // set callback on this message_id
      if (cb)
         self->task_queue_.emplace(message_id, std::move(cb));

      bool flag = self->message_queue_.IsEmpty();
      self->message_queue_.Push<uint16_t>(packet.value());
      if (flag)
         self->WriteImpl(std::move(cb));
   });

   return true;
}

void SessionClient::WriteImpl(WriteCallbackType cb) {
   auto opt = message_queue_.Pop<uint16_t>();
   if (!opt) {
      GlobalLogDebug("message queue Pop() failed");
      return;
   }
   auto buffer = std::make_shared<std::vector<std::byte>>(opt.value());

   asio::async_write(socket_, asio::buffer(*buffer), 
   [self = this->shared_from_this(), cb = std::move(cb)] (const asio::error_code& ec, [[maybe_unused]] size_t length) {
      if (!ec) {
         // increment common counter
         ++self->message_counter_;
         // refresh timestamp
         self->last_timestamp_ = std::time(nullptr);
         // remove message
         if (!self->message_queue_.IsEmpty())
            self->WriteImpl(std::move(cb));
      }
      else {
         GlobalLogInfo("connection was lost [{}:{}]", ec.value(), ec.message());
         // maybe reconnect?
      }
   });
}

void SessionClient::Read() {
   auto buffer = std::make_shared<std::vector<std::byte>>(global::TCP_TOTAL_PACKET_SIZE);
   // async read
   socket_.async_read_some(asio::buffer(*buffer),
   [self = shared_from_this(), buffer] (const asio::error_code& ec, [[maybe_unused]] std::size_t length) mutable {
      if (ec) {
         // todo: handle error
         // ...
         return;
      }
      // increment counter for poly1305 nonce
      ++self->message_counter_;
      // refresh timestamp
      self->last_timestamp_ = std::time(nullptr);

      // parse packet and retrieve the message_id to invoke 
      // the callback, if one exists
      auto p = ReadPacket(*buffer, self->message_counter_, self->keys_.GetSharedKey());
      if (p) {
         auto packet = std::make_shared<Packet>(p.value());
         if (self->task_queue_.contains(p->header.message_id)) {
            asio::post(self->pool_, 
            [self = self->shared_from_this(), packet, cb = std::move(self->task_queue_[packet->header.message_id])] {
                  cb(packet, self);
            });
         }               
      }
      else GlobalLogDebug("ReadPacket() failed");

      self->Read();
   });
}

} // namespace remc::net

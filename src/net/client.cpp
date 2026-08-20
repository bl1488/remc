#include "client.h"
#include "packet.h"
#include "include/remc_spdlog.h"

namespace remc::net {

// instance
template class SessionBase<SessionClient>;
   
//
// SessionClient
//
bool SessionClient::Write(Packet::Flags flags, std::string_view payload, WriteCallbackType cb) {
   if (payload.size() > global::TCP_PAYLOAD_SIZE_MAX)
      return false;

   std::vector<std::byte> vec(payload.size());
   std::memcpy(vec.data(), payload.data(), payload.size());

   return Write(flags, std::move(vec), std::move(cb));
}

bool SessionClient::Write(Packet::Flags flags, std::vector<std::byte> payload, WriteCallbackType cb) {
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
         GlobalLogDebug("packet creation failed: {}:{}", err.GetCodeAsString(), err.Message());
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

   // async write
   asio::async_write(socket_, asio::buffer(*buffer), 
   [self = this->shared_from_this(), cb = std::move(cb)] (const asio::error_code& ec, [[maybe_unused]] size_t length) {
      if (!ec) {
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

      // post to thread pool
      asio::post(self->pool_, [self, buffer = std::move(buffer)] {
         // parse packet and retrieve the message_id to invoke the callback, if one exists
         auto packet = ReadPacket(*buffer, self->message_counter_, self->keys_.GetSharedKey());
         if (packet) {
            // find with iterator
            auto iter = self->task_queue_.find(packet->header.message_id);
            if (iter != self->task_queue_.end()) {
               // callback can throw an exception so we erase/move it before
               auto callback = std::move(iter->second);
               self->task_queue_.erase(iter);
               // user callback call
               callback(std::move(packet.value()), self);
            }
            // else: no callback for this message_id
         }
         else {
            auto& err = packet.error();
            GlobalLogDebug("ReadPacket() failed with: {} ({})", err.Message(), err.GetCodeAsInt());
         }
      });
      self->Read();
   });
}

} // namespace remc::net

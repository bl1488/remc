#ifndef REMC_CLIENT_H_
#define REMC_CLIENT_H_

#include "net/common.h"
#include "session_base.h"
#include "include/remc_spdlog.h"
#include "include/ring_buffer.h"

#include <functional>

#include <absl/container/flat_hash_map.h>

namespace remc::net {

// ===== SessionClient =====
//
class SessionClient final : public SessionBase<SessionClient> {
public:
   using WriteCallbackType = std::function<void(std::shared_ptr<Packet>, 
                                                std::shared_ptr<SessionClient>)>;
public:
   SessionClient(tcp::socket&& socket, asio::thread_pool& pool) : 
      SessionBase(std::move(socket)),
      message_queue_(global::RING_BUFFER_CAPACITY),
      pool_(pool) {}

public:
   bool Write(uint32_t flags, std::vector<std::byte> payload, WriteCallbackType cb = nullptr);

   bool Write(uint32_t flags, std::string_view       payload, WriteCallbackType cb = nullptr);

   void Read() {
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

   tcp::socket&       GetSocket()       noexcept { return socket_; }

   const tcp::socket& GetSocket() const noexcept { return socket_; }

private:
   void WriteImpl(WriteCallbackType cb);

private:
// std::deque<std::vector<std::byte>> 
//                    message_queue_;
   containers::RingBuffer message_queue_;
   absl::flat_hash_map<std::size_t, WriteCallbackType>
                          task_queue_;
   asio::thread_pool&     pool_;
};

// ===== Client =====
//
class Client {
public:
   Client(asio::io_context* io, asio::thread_pool& pool) : pool_(&pool) {
      if (io) 
           io_external_ = io;
      else io_internal_ = std::make_unique<asio::io_context>();
   }

public:
   void Connect(const char* addr, unsigned short port);

   asio::io_context& GetExecutor() 
      noexcept { return io_internal_ ? *io_internal_ : *io_external_; }

   asio::thread_pool& GetPool()                    noexcept { return *pool_;   }

   std::weak_ptr<SessionClient> GetSession() const noexcept { return session_; }

private:
   std::unique_ptr<asio::io_context>
                      io_internal_;
   asio::io_context*  io_external_;
   asio::thread_pool* pool_;
   // session
   std::shared_ptr<SessionClient>
                      session_;
};

// instance here
// src: session_base.cpp
extern template class SessionBase<SessionClient>;

} // namespace remc::net

#endif // REMC_CLIENT_H_

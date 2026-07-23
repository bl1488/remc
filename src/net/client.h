#ifndef CLIENT_H_
#define CLIENT_H_

#include "session_base.h"

namespace remc::net {

//
// SessionClient
//
//
class SessionClient final : public SessionBase<SessionClient> {
public:
   SessionClient(tcp::socket&& socket, asio::thread_pool& pool) : 
      SessionBase(std::move(socket)), pool_(pool) {}

public:
   template<typename TaskType>
   void DoRead(TaskType&& task) noexcept {
      auto buffer = std::make_shared<std::string>(MAX_MESSAGE_LENGTH, 0);
      socket_.async_read_some(asio::buffer(*buffer),
      [self = shared_from_this(), buffer, task = std::move(task)] (const asio::error_code& ec, std::size_t length) mutable {
         if (!ec) {
            // refresh timestamp
            self->last_timestamp_ = std::time(nullptr);
            if (length > MAX_MESSAGE_LENGTH)
               buffer->resize(MAX_MESSAGE_LENGTH);
            // copy task then add to the pool  
            auto task_copy = task;
            asio::post(self->pool_, task_copy);
            // call next
            self->DoRead(std::move(task));
         }
         else {
            // todo: handle error
            // ...
         }
      });
   }

   tcp::socket& GetSocket() noexcept { return socket_; }

   const tcp::socket& GetSocket() 
      const noexcept { return socket_; }

private:
   asio::thread_pool& pool_;
};

//
// Client
//
//
class Client {
public:
   Client(asio::io_context* io, asio::thread_pool& pool) : pool_(&pool) {
      if (io) 
           io_external_ = io;
      else io_internal_ = std::make_unique<asio::io_context>();
   }

public:
   void Connect(const char* addr, const char* port);

   asio::io_context& GetExecutor() 
      noexcept { return io_internal_ ? *io_internal_ : *io_external_; }

   asio::thread_pool& GetPool() noexcept { return *pool_; }

   std::weak_ptr<SessionClient> GetSession() 
      const noexcept { return session_; }

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
extern template class SessionBase<SessionClient>;

} // namespace remc::net

#endif // CLIENT_H_

#ifndef REMC_CLIENT_H_
#define REMC_CLIENT_H_

#include "asio/execution/occupancy.hpp"
#include "net/session-base.h"
#include "net/packet.h"
#include "include/ring-buffer.h"

#include <functional>

#include <asio/executor_work_guard.hpp>
#include <asio/io_context.hpp>
#include <absl/container/flat_hash_map.h>
#include <thread>

namespace remc::net {

class SessionClient;

// extern instance
// src: server.cpp
extern template class SessionBase<SessionClient>;
   
// ===== SessionClient =====
//
class SessionClient : public SessionBase<SessionClient> {
public:
   // signature of callback function to be specified
   using WriteCallbackType = std::function<void(Packet, std::shared_ptr<SessionClient>)>;
public:
   SessionClient(tcp::socket&& socket, asio::thread_pool& pool) : 
      SessionBase(std::move(socket)),
      message_queue_(global::RING_BUFFER_CAPACITY),
      pool_(pool) {}

public:
   bool Write(
      Packet::Flags          flags, 
      std::vector<std::byte> payload, 
      WriteCallbackType      cb = nullptr);

   bool Write(
      Packet::Flags     flags, 
      std::string_view  payload, 
      WriteCallbackType cb = nullptr);

   void Read();

   tcp::socket& GetSocket() noexcept { return socket_; }

private:
   void WriteImpl(WriteCallbackType cb);

private:
   containers::RingBuffer message_queue_;
   absl::flat_hash_map<std::size_t, WriteCallbackType>
                          task_queue_;
   asio::thread_pool&     pool_;
};

// ===== Client =====
//
class ClientBase {
public:
   // number of threads must be > 1.
   // i dont want to throw an exception so be careful in release mode
   ClientBase(asio::thread_pool& pool) : pool_(pool) {
      assert(asio::query(pool.get_executor(), asio::execution::occupancy) > 1
         && "insufficient number of threads in thread_pool");
   }

   ClientBase(const ClientBase&)                = delete;
   ClientBase& operator=(const ClientBase&&)    = delete;
   ClientBase(ClientBase&&) noexcept            = delete;
   ClientBase& operator=(ClientBase&&) noexcept = delete;

   ~ClientBase() = default;

public:
   void Connect(const char* addr, unsigned short port) {
      tcp::resolver res(GetExecutor());
      tcp::socket sock(GetExecutor());
      
      asio::connect(sock, res.resolve(addr, std::to_string(port)));
      // init session
      session_ = std::make_shared<SessionClient>(std::move(sock), pool_);
   }

   asio::thread_pool& GetPool() noexcept { return pool_; }

   std::weak_ptr<SessionClient> GetSession() 
      const noexcept { return session_; }
   
   virtual asio::io_context& GetExecutor() noexcept = 0;

protected:
   // external thread pool
   asio::thread_pool& pool_;
   // session
   std::shared_ptr<SessionClient> session_;
};

// ===== ExternalClient =====
//
// works on external io_context
class ExternalClient : public ClientBase {
public:
   ExternalClient(asio::io_context& external_io, asio::thread_pool& external_pool) : 
      ClientBase(external_pool), io_(external_io) {}

public:
   asio::io_context& GetExecutor() noexcept override { return io_; }

private:
   asio::io_context& io_;
};

// ===== InternalClient =====
//
// works on internal io_context
class InternalClient : public ClientBase {
public:
   InternalClient(asio::thread_pool& external_pool) : 
      ClientBase(external_pool), work_guard_(io_.get_executor()) {
      Run();
   }

   ~InternalClient() { Stop(); }

public:
   void Run() { asio::post(pool_, [this]{ io_.run(); }); }

   void Stop() noexcept { 
      work_guard_.reset();
      while (!io_.stopped())
         std::this_thread::yield();
   }

   asio::io_context& GetExecutor() noexcept override { return io_; }

private:
   asio::io_context io_;
   asio::executor_work_guard<asio::io_context::executor_type>
                    work_guard_;
};

} // namespace remc::net

#endif // REMC_CLIENT_H_

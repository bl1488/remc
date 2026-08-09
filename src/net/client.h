#ifndef REMC_CLIENT_H_
#define REMC_CLIENT_H_

#include "net/common.h"
#include "session_base.h"
#include "include/ring_buffer.h"

#include <concepts>
#include <functional>

#include <asio/executor_work_guard.hpp>
#include <asio/io_context.hpp>
#include <absl/container/flat_hash_map.h>

namespace remc::net {

// ===== SessionClient =====
//
class SessionClient final : public SessionBase<SessionClient> {
public:
   // signature of callback function to be specified
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
template<typename DerivedType>
class ClientBase {
protected:
   // thread pool should be > 1
   ClientBase(asio::thread_pool& pool) : pool_(pool) {
      static_assert(std::is_base_of_v<ClientBase<DerivedType>, DerivedType>);
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

   std::weak_ptr<SessionClient> GetSession() const noexcept { return session_; }
   
   asio::io_context& GetExecutor() noexcept {
      return reinterpret_cast<DerivedType*>(this)->GetExecutorImpl();
   }

protected:
   // external thread pool
   asio::thread_pool& pool_;
   // session
   std::shared_ptr<SessionClient>
                      session_;
};

// ===== ExternalClient =====
//
class ExternalClient : public ClientBase<ExternalClient> {
public:
   ExternalClient(asio::io_context& external_io, asio::thread_pool& external_pool) 
      : ClientBase(external_pool), io_(external_io) {}

public:
   asio::io_context& GetExecutorImpl() noexcept { return io_; }

private:
   asio::io_context& io_;
};

// ===== InternalClient =====
//
class InternalClient : public ClientBase<InternalClient> {
public:
   InternalClient(asio::thread_pool& external_pool) 
      : ClientBase(external_pool), work_guard_(io_.get_executor()) 
   {
      Run();
   }

public:
   void Run() { asio::post(pool_, [this]() { io_.run(); }); }

   void Stop() noexcept { work_guard_.reset(); }

   asio::io_context& GetExecutorImpl() noexcept { return io_; }

private:
   asio::io_context io_;
   asio::executor_work_guard<asio::io_context::executor_type>
                    work_guard_;
};

// instance here
// src: session_base.cpp
extern template class SessionBase<SessionClient>;

} // namespace remc::net

#endif // REMC_CLIENT_H_

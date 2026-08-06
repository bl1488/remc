#ifndef REMC_SERVER_H_
#define REMC_SERVER_H_

#include "session_base.h"
#include "net/common.h"
#include "include/ring_buffer.h"
#include "include/remc_spdlog.h"

#include <atomic>

#include <absl/container/flat_hash_map.h>

namespace remc::net {

class Worker;

// ===== SessionServer =====
//
class SessionServer final : public SessionBase<SessionServer> {
public:
   SessionServer(tcp::socket&& socket, Worker* parent_worker) :
      SessionBase(std::move(socket)),
      message_queue_(global::RING_BUFFER_CAPACITY),
      parent_worker_(parent_worker), 
      is_resetting_(false) {}

public:
   void Close();

   template<typename TaskType> 
      requires std::invocable<TaskType&&, std::vector<std::byte>, 
                                          std::shared_ptr<SessionServer>>
   void ResetReadCallback(TaskType&& task) {
      // set flag
      is_resetting_.store(true, std::memory_order_release);
      // stop prev gen then start new
      socket_.cancel();
      asio::post(socket_.get_executor(), [self = shared_from_this(), task = std::forward<TaskType>(task)] {
         self->is_resetting_.store(false, std::memory_order_release);
         self->Read(std::move(task));
      });
   }

   template<typename TaskType>
   void Read(TaskType&& task) { ReadImpl(std::forward<TaskType>(task), parent_worker_); }

   bool Write(uint32_t flags, uint64_t message_id, std::vector<std::byte> payload);

   bool Write(uint32_t flags, uint64_t message_id, std::string_view       payload);

private:
   template<typename TaskType, typename WorkerType> 
      requires std::invocable<TaskType&&, std::vector<std::byte>, 
                                          std::shared_ptr<SessionServer>>
   void ReadImpl(TaskType&& task, WorkerType wptr) {
      auto buffer = std::make_shared<std::vector<std::byte>>(global::TCP_TOTAL_PACKET_SIZE);
      // async read
      socket_.async_read_some(asio::buffer(*buffer), 
      [self = shared_from_this(), buffer, wptr, task = std::move(task)] 
      (const asio::error_code& ec, [[maybe_unused]] std::size_t length) mutable {
         if (ec) {
            // die here for next callbacks gen
            if (ec == asio::error::operation_aborted)
               return;
            // just close session
            wptr->RemoveSession(self->session_id_);
            return;
         }
         // increment message_counter for poly1305 nonce
         ++self->message_counter_;
         // refresh timestamp
         self->last_timestamp_ = std::time(nullptr);
         
         // copy task then add to the pool  
         auto task_copy = task;
         if (wptr)
            wptr->AddTaskToPool(std::move(task_copy), std::move(*buffer), self);

         // time to refresh callback?
         // old gen: cb [1] -> cb [2] -> ... -> return here
         // new gen: ... cb [n] -> new cb [1] -> new cb [2] -> ...
         if (self->is_resetting_.load(std::memory_order_acquire))
            return;
         
         self->Read(std::move(task));
      });
   }

   void WriteImpl();

private:
   containers::RingBuffer  message_queue_;
   Worker*                 parent_worker_;
   std::atomic<bool>       is_resetting_;
};

// ===== Worker =====
//
class Worker final : public std::enable_shared_from_this<Worker> {
public:
   Worker(asio::thread_pool& thread_pool) : thread_flag_(false), thread_pool_(thread_pool) {
      // reserve for max count
      session_list_.reserve(global::WORKER_MAX_CLIENT_COUNT);
   }

public:
   template<typename TaskType, typename... TaskArgsType>
   void AddTaskToPool(TaskType task, TaskArgsType&&... args) {
      asio::post(thread_pool_, [task = std::move(task), ...args = std::forward<TaskArgsType>(args)]() mutable {
         task(std::move(args)...);
      });
   }

   auto SessionListBegin()        
      const noexcept { return session_list_.begin(); }

   auto SessionListEnd()          
      const noexcept { return session_list_.end();   }

   std::size_t GetClientCount()  
      const noexcept { return session_list_.size();  }

   auto GetSessionById(size_t id) 
      const noexcept { return session_list_.at(id);  }

   void AddClient(tcp::socket&& socket);

   void RemoveSession(std::size_t id);

   void Run();

   void Stop();

private:
   void Heartbeat();

private:
   absl::flat_hash_map<size_t, std::shared_ptr<SessionServer>>
                      session_list_;
   asio::io_context   io_;
   // io_context thread
   std::thread        thread_;
   std::atomic<bool>  thread_flag_;
   // external thread pool ref
   asio::thread_pool& thread_pool_;
};

// ===== Server =====
//
class Server : std::enable_shared_from_this<Server> {
public:
   static constexpr int MAX_WORKERS_NUMBER = 2;
public:
   Server(int port, asio::thread_pool& pool) : 
      acceptor_(io_, tcp::endpoint(tcp::v4(), port)),  thread_flag_(false), thread_pool_(pool) 
   {
      for (int i = 0; i < MAX_WORKERS_NUMBER; ++i)
         worker_list_.push_back(std::make_shared<Worker>(pool));
   }

   Server(const Server&)     = delete;
   Server(Server&&)          = delete;
   Server& operator=(Server) = delete;

public:
   void Run();

   void Stop();

   auto BeginList() const noexcept { return worker_list_.begin(); }

   auto EndList()   const noexcept { return worker_list_.end();   }

   bool IsRunning() const noexcept { return thread_flag_;         }

private:
   void Acceptor();

   [[nodiscard]] std::shared_ptr<Worker> PeekWorker() const noexcept;

private:
   asio::io_context   io_;
   tcp::acceptor      acceptor_;
   // io_context thread
   std::thread        thread_;
   std::atomic<bool>  thread_flag_;
   // worker list
   std::vector<std::shared_ptr<Worker>>
                      worker_list_;
   // thread pool ref
   asio::thread_pool& thread_pool_;
};

// extern instance here
// src: serssion_base.cpp
extern template class SessionBase<SessionServer>;

} // namespace remc::net

#endif // REMC_SERVER_H_

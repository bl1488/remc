#ifndef SERVER_H_
#define SERVER_H_

#include "session_base.h"

#include <absl/container/flat_hash_map.h>

namespace remc::net {

class Worker;

//
// SessionServer
//
//
class SessionServer final : public SessionBase<SessionServer> {
public:
   SessionServer(tcp::socket&& socket, Worker* parent_worker) :
      SessionBase(std::move(socket)), parent_worker_(parent_worker), is_resetting_(false) {}

public:
   void Close() noexcept;

   // reset callback for read
   template<typename TaskType>
   void ResetRead(TaskType&& task) {
      // set flag
      is_resetting_.store(true, std::memory_order_release);
      // stop prev gen then start new
      socket_.cancel();
      asio::post(socket_.get_executor(), [self = shared_from_this(), task = std::forward<TaskType>(task)] {
         self->is_resetting_.store(false, std::memory_order_release);
         self->DoRead(std::move(task));
      });
   }

   template<typename TaskType>
   void DoRead(TaskType&& task) { DoReadImpl(std::forward<TaskType>(task), parent_worker_); }

private:
   template<typename TaskType, typename WorkerType = std::shared_ptr<Worker>> 
      requires std::invocable<TaskType&&, std::shared_ptr<std::string>, 
                                          std::shared_ptr<SessionServer>>
   void DoReadImpl(TaskType&& task, WorkerType wptr) {
      auto buffer = std::make_shared<std::string>(MAX_MESSAGE_LENGTH, 0);
      socket_.async_read_some(asio::buffer(*buffer), 
      [self = shared_from_this(), buffer, wptr, task = std::move(task)] (const asio::error_code& ec, std::size_t length) mutable {
         if (!ec) {
            // refresh timestamp
            self->last_timestamp_ = std::time(nullptr);
            buffer->resize(length);
            // copy task then add to the pool  
            auto task_copy = task;
            wptr->AddTaskToPool(std::move(task_copy), std::move(buffer), self);
            // time to refresh callback?
            if (self->is_resetting_.load(std::memory_order_acquire))
               return;
            // call next
            self->DoRead(std::move(task));
         }
         else {
            // die here for next callbacks generation
            if (ec == asio::error::operation_aborted)
               return;
            // just close session
            wptr->RemoveSession(self->session_id_);
         }
      });
   }

private:
   Worker* parent_worker_;
   std::atomic<bool>
           is_resetting_;
};

//
// Worker
//
//
class Worker : public std::enable_shared_from_this<Worker> {
public:
   // seconds
   static constexpr int HEARTBEAT_TIMEOUT = 30;
   static constexpr int HEARTBEAT_PERIOD  = 5;
   // max client count per worker
   static constexpr int MAX_CLIENT_COUNT  = 4096;
public:
   Worker(asio::thread_pool& thread_pool) : thread_flag_(false), thread_pool_(thread_pool) {
      // reserve for max count
      session_list_.reserve(MAX_CLIENT_COUNT);
   }

public:
   template<typename TaskType, typename... TaskArgsType>
   void AddTaskToPool(TaskType task, TaskArgsType&&... args) {
      asio::post(thread_pool_, [task = std::move(task), ...args = std::forward<TaskArgsType>(args)]() mutable {
         task(std::move(args)...);
      });
   }

   void AddClient(tcp::socket&& socket) noexcept;

   void RemoveSession(std::size_t id)   noexcept;

   void Run()  noexcept;

   void Stop() noexcept;

   auto SessionBegin() const noexcept { return session_list_.begin(); }

   auto SessionEnd()   const noexcept { return session_list_.end();   }

   std::size_t GetClientCount() 
      const noexcept { return session_list_.size(); }

   auto GetSessionById(size_t id) 
      const noexcept { return session_list_.at(id); }

private:
   void Heartbeat() noexcept;

private:
   absl::flat_hash_map<size_t, std::shared_ptr<SessionServer>>
                      session_list_;
   asio::io_context   io_;
   // io_context thread
   std::thread        thread_;
   std::atomic<bool>  thread_flag_;
   // pointer to thread pool for heavy tasks
   asio::thread_pool& thread_pool_;
};

//
// Server
//
//
class Server : std::enable_shared_from_this<Server> {
public:
   static constexpr int MAX_WORKER_NUMBER = 2;
public:
   Server(int port, asio::thread_pool& pool) : 
      acceptor_(io_, tcp::endpoint(tcp::v4(), port)),  thread_flag_(false), thread_pool_(pool) 
   {
      for (int i = 0; i < MAX_WORKER_NUMBER; ++i)
         worker_list_.push_back(std::make_shared<Worker>(pool));
   }

   Server(const Server&)     = delete;

   Server(Server&&)          = delete;

   Server& operator=(Server) = delete;

public:
   void Run()  noexcept;

   void Stop() noexcept;

   auto BeginWorker() const noexcept { return worker_list_.begin(); }

   auto EndWorker()   const noexcept { return worker_list_.end();   }

private:
   void Acceptor() noexcept;

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

// instance here
extern template class SessionBase<SessionServer>;

} // namespace remc::net

#endif // SERVER_H_

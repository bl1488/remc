#include "server.h"
#include "include/remc_spdlog.h"

namespace remc::net {

//
// Worker
//
//
void Worker::AddClient(tcp::socket&& socket) noexcept {
   asio::post(io_, [self = shared_from_this(), socket = std::move(socket)] mutable {
      // is enough space for new session?
      if (self->session_list_.size() + 1 <= MAX_CLIENT_COUNT) {
         // create session
         auto session = std::make_shared<SessionServer>(std::move(socket), &(*self));
         if (session) {
            size_t id = session->GetSessionId();
            // add to the table
            self->session_list_.emplace(session->GetSessionId(), std::move(session));
            // start session and specify the callback
            self->session_list_[id]->Read([](std::shared_ptr<std::string> message, std::shared_ptr<SessionServer> session) {
               std::cout << "from: " << session->GetSessionId() 
                  << "\nmessage: "   << *message << '\n'
                  << "\nsize: "      << message->size()  << '\n';
            });
            GlobalLogDebug("session <{}> created", id);
         }
         else {
            GlobalLogError("session creation failed");
            // kill
            socket.shutdown(tcp::socket::shutdown_both);
            socket.close();
         }
      }
   });
}

void Worker::RemoveSession(std::size_t id) noexcept {
   asio::post(io_, [self = shared_from_this(), id] {
      if (self->session_list_.contains(id)) {
         self->session_list_[id]->Close();
         self->session_list_.erase(id);
         GlobalLogInfo("session <{}> dead", id);
      }
   });
}

void Worker::Run() noexcept {
   if (!thread_flag_.exchange(true)) {
      // init heartbeat
      Heartbeat();
      // start thread
      thread_ = std::thread([this]{ io_.run(); });
      GlobalLogInfo("worker initialized");
   }
}

void Worker::Stop() noexcept {
   if (thread_flag_.exchange(false)) {
      // stop context
      io_.stop();
      // stop sessions
      for (auto it = session_list_.begin(); it != session_list_.end(); ++it)
         it->second->Close();
      // stop thread
      if (thread_.joinable()) thread_.join();
      GlobalLogInfo("worker stopped");
   }
}

void Worker::Heartbeat() noexcept {
   auto timer = std::make_shared<asio::steady_timer>(io_, std::chrono::seconds(HEARTBEAT_PERIOD));
   timer->async_wait([self = shared_from_this(), timer](const asio::error_code& ec) {
      if (!ec) {
         std::vector<std::shared_ptr<SessionServer>> vec;
         std::time_t now = std::time(nullptr);
         // just cheking last session timestamp then add them to the vector
         for (auto it = self->session_list_.begin(); it != self->session_list_.end(); ++it) {
            if (now - it->second->GetTimestamp() > HEARTBEAT_TIMEOUT)
               vec.push_back(it->second);
         }
         // kill sessions
         for (auto& i : vec)
            self->RemoveSession(i->GetSessionId());
      }
      self->Heartbeat();
   });
}

//
// SessionServer
//
//
void SessionServer::Close() noexcept {
   asio::dispatch(socket_.get_executor(), [self = shared_from_this(), this] {
      if (self->socket_.is_open()) {
         self->socket_.shutdown(tcp::socket::shutdown_both);
         self->socket_.close();
         self->message_queue_.clear();
         GlobalLogDebug("session <{}> closed", session_id_);
      }
   });
}

//
// Server
//
//
void Server::Run() noexcept {
   if (!thread_flag_.exchange(true)) {
      // init workers
      for (auto& i : worker_list_)
         i->Run();
      // init acceptor
      Acceptor();
      // start context
      thread_ = std::thread([this]{ io_.run(); });
      GlobalLogInfo("server initialized");
   }
}

void Server::Stop() noexcept {
   if (thread_flag_.exchange(false)) {
      // stop context
      io_.stop();
      // stop thread
      if (thread_.joinable())
         thread_.join();
      // stop workers
      for (auto& i : worker_list_) i->Stop();
      GlobalLogInfo("server stopped");
   }
}

void Server::Acceptor() noexcept {
   acceptor_.async_accept([this](const asio::error_code& ec, tcp::socket socket) {
      if (!ec) {
         GlobalLogDebug("new connection <{}:{}>",
            socket.remote_endpoint().address().to_string(), socket.remote_endpoint().port());
         auto worker = PeekWorker();
         if (worker)
            worker->AddClient(std::move(socket));
         else GlobalLogError("no available workers");
      }
      Acceptor();
   });
}

[[nodiscard]] std::shared_ptr<Worker> Server::PeekWorker() const noexcept {
   if constexpr (MAX_WORKER_NUMBER == 1) {
      // is enough space?
      return worker_list_[0]->GetClientCount() >= Worker::MAX_CLIENT_COUNT ? 
         nullptr : worker_list_[0];
   }
   auto ptr = worker_list_.begin();
   for (auto it = worker_list_.begin() + 1; it != worker_list_.end(); ++it) {
      // determine based on smallest number of clients
      ptr = it->get()->GetClientCount() < ptr->get()->GetClientCount() ?
         it : ptr;
   }
   return *ptr;
}

} // namespace remc::net

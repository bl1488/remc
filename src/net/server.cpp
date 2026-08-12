#include "server.h"
#include "asio/cancellation_signal.hpp"
#include "include/remc_spdlog.h"

namespace remc::net {

// default read callback.
// todo: 
//    - handle others header::flags 
void DefaultReadCallback(std::vector<std::byte> buffer, std::shared_ptr<SessionServer> session) {
   assert(session);

   auto p = ReadPacket(std::move(buffer), session->GetMessageCounter() - 1, session->KeysInfo().GetSharedKey());
   if (!p) {
      GlobalLogError("cb: ReadPacket() failed");
      return;
   }

   auto& packet = p.value();

   if (packet.header.flags == Packet::FLAG_TYPE_HANDSHAKE) {
      GlobalLogDebug("flag: FLAG_TYPE_HANDSHAKE");
      // send my public to client
      session->Write(Packet::FLAG_TYPE_HANDSHAKE, 
                     packet.header.message_id, 
                     std::string(session->KeysInfo().GetKeyAsString(session->KeysInfo().GetPublicKey())) );
      // compute shared key
      if (!session->KeysInfo().ComputeSharedKey(packet.payload))
         GlobalLogError("cb: error computing shared key");
   }
   else if (packet.header.flags == Packet::FLAG_TEST_MESSAGE) {
      GlobalLogDebug("flag: FLAG_TYPE_HANDSHAKE");
      session->Write(Packet::FLAG_TEST_MESSAGE,
                     packet.header.message_id,
                     std::string("asnwer from server ") + std::to_string(session->GetMessageCounter()));
   }

#ifndef NDEBUG
   std::cout << "version.........: " << packet.header.version      << '\n'
             << "flags...........: " << packet.header.flags        << '\n'
             << "timestamp.......: " << packet.header.timestamp    << '\n'
             << "message-id......: " << packet.header.message_id   << '\n'
             << "message-size....: " << packet.header.message_size << '\n'
             << "nonce...........: " << packet.header.nonce        << '\n';

   auto payload = packet.GetPayloadAsString();
   std::cout << std::format("message.........: '{}' : {}\n", payload.data(), payload.size()) << '\n';
#endif
}

//
// Worker
//
void Worker::AddClient(tcp::socket&& socket) {
   asio::post(io_, [self = shared_from_this(), socket = std::move(socket)] mutable {
      // is enough space for new session?
      if (self->session_list_.size() + 1 > global::WORKER_MAX_CLIENT_COUNT)
         return;
      // create session
      auto session = std::make_shared<SessionServer>(std::move(socket), &(*self));
      if (session) {
         size_t id = session->GetSessionId();
         // add to the table
         self->session_list_.emplace(session->GetSessionId(), std::move(session));
         // start session and specify the callback
         // callback signature: (std::string, std::shared_ptr<SessionServer>)
         self->session_list_[id]->Read(DefaultReadCallback);
         GlobalLogDebug("session <{}> created", id);
      }
      else {
         GlobalLogError("session creation failed");
         // kill
         socket.shutdown(tcp::socket::shutdown_both);
         socket.close();
      }
   });
}

void Worker::RemoveSession(std::size_t id) {
   asio::post(io_, [self = shared_from_this(), id] {
      if (self->session_list_.contains(id)) {
         self->session_list_[id]->Close();
         self->session_list_.erase(id);
         GlobalLogInfo("session <{}> dead", id);
      }
   });
}

void Worker::Run() {
   if (!thread_flag_.exchange(true)) {
      // init heartbeat
      Heartbeat();
      // start thread
      thread_ = std::thread([this]{ io_.run(); });
      
      GlobalLogDebug("worker running");
   }
}

void Worker::Stop() {
   if (thread_flag_.exchange(false)) {
      // stop context
      io_.stop();
      // stop sessions
      for (auto it = session_list_.begin(); it != session_list_.end(); ++it)
         it->second->Close();
      // stop thread
      if (thread_.joinable()) thread_.join();

      GlobalLogDebug("worker stopped");
   }
}

void Worker::Heartbeat() {
   auto timer = std::make_shared<asio::steady_timer>(io_, std::chrono::seconds(global::WORKER_HEARTBEAT_PERIOD));
   timer->async_wait([self = shared_from_this(), timer](const asio::error_code& ec) {
      if (!ec) {
         std::vector<std::shared_ptr<SessionServer>> vec;
         // current timestamp for comapration
         std::time_t now = std::time(nullptr);
         // just compare session timestamp with current timestamp
         // then add them to the vector
         for (auto it = self->session_list_.begin(); it != self->session_list_.end(); ++it) {
            if (now - it->second->GetTimestamp() > global::WORKER_HEARTBEAT_TIMEOUT)
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
void SessionServer::Close() {
   asio::dispatch(socket_.get_executor(), [self = shared_from_this()] {
      if (self->socket_.is_open()) {
         self->socket_.shutdown(tcp::socket::shutdown_both);
         self->socket_.close();
         self->message_queue_.Clear();
         GlobalLogDebug("session <{}> closed", self->session_id_);
      }
   });
}

bool SessionServer::Write(uint32_t flags, uint64_t message_id, std::string_view payload) {
   if (payload.size() > global::TCP_PAYLOAD_SIZE_MAX)
      return false;

   std::vector<std::byte> vec(payload.size());
   std::memcpy(vec.data(), payload.data(), payload.size());

   return Write(flags, message_id, std::move(vec));
}

bool SessionServer::Write(uint32_t flags, uint64_t message_id, std::vector<std::byte> payload) {
   // invalid payload data
   // not slicing and return false
   if (payload.size() > global::TCP_PAYLOAD_SIZE_MAX)
      return false;

   // post to parent worker context
   asio::post(socket_.get_executor(), 
   [self = this->shared_from_this(), flags, message_id, payload = std::move(payload)] {
      // create packet
      auto packet = CreatePacket(
         { const_cast<std::byte*>(payload.data()), payload.size() }, 
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
         
      bool flag = self->message_queue_.IsEmpty();
      self->message_queue_.Push<uint16_t>(packet.value());
      if (flag)
         self->WriteImpl();
   });

   return true;
}

void SessionServer::WriteImpl() {
   auto opt = message_queue_.Pop<uint16_t>();
   if (!opt) {
      GlobalLogDebug("message queue Pop() failed");
      return;
   }
   auto buffer = std::make_shared<std::vector<std::byte>>(opt.value());

   asio::async_write(socket_, asio::buffer(*buffer), 
   [self = this->shared_from_this()] (const asio::error_code& ec, [[maybe_unused]] size_t length) {
      if (ec) {
         // todo: handle error
         // ...
         return;
      }
      // increment counter for poly1305 nonce
      ++self->message_counter_;
      // refresh timestamp
      self->last_timestamp_ = std::time(nullptr);

      if (!self->message_queue_.IsEmpty())
         self->WriteImpl();
   });
}

//
// Server
//
void Server::Run() {
   if (!thread_flag_.exchange(true)) {
      // init workers
      for (auto& i : worker_list_)
         i->Run();
      // init acceptor
      Acceptor();
      // start io_context
      thread_ = std::thread([this]{ io_.run(); });

      GlobalLogInfo("server initialized");
   }
}

void Server::Stop() {
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

void Server::Acceptor() {
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
   if constexpr (MAX_WORKERS_NUMBER == 1) {
      // is enough space?
      return worker_list_[0]->GetClientCount() >= global::WORKER_MAX_CLIENT_COUNT ? 
         nullptr : worker_list_[0];
   }
   else {
      auto ptr = worker_list_.begin();
      for (auto it = worker_list_.begin() + 1; it != worker_list_.end(); ++it) {
         // determine based on smallest number of clients
         ptr = it->get()->GetClientCount() < ptr->get()->GetClientCount() ?
            it : ptr;
      }
      return *ptr;
   }
}

} // namespace remc::net

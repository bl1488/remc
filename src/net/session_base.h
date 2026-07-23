#ifndef SESSION_BASE_H_
#define SESSION_BASE_H_

#include "include/remc_utils.h"
#include "keys_wrapper.h"

#include <ctime>
#include <memory>
#include <deque>
#include <type_traits>
#include <utility>
#include <string>

#include <asio.hpp>

using asio::ip::tcp;

namespace remc::net {

//
// SessionBase
//
//
template<typename DerivedType>
class SessionBase : public std::enable_shared_from_this<DerivedType> {
public:
   static constexpr int MAX_MESSAGE_LENGTH = 512;
protected:
   SessionBase(tcp::socket&& socket) : 
      socket_(std::move(socket)),
      session_id_(GenerateSessionId()), 
      last_timestamp_(std::time(nullptr)) {}

   // deleted
   SessionBase(const SessionBase&)     = delete;
   SessionBase(SessionBase&& other)    = delete;
   SessionBase& operator=(SessionBase) = delete;

   ~SessionBase() = default;

public:
   void Write(std::string message) noexcept {
      // is message valid?
      if (message.empty()) return;
      // if by .size() do resize
      else if (message.size() > MAX_MESSAGE_LENGTH)
         message.resize(MAX_MESSAGE_LENGTH);

      // post to parent worker context
      asio::post(socket_.get_executor(), 
      [self = std::static_pointer_cast<SessionBase>(this->shared_from_this()), msg = std::move(message)] {
         // todo: create packet
         // ...
         bool flag = self->message_queue_.empty();
         self->message_queue_.emplace_back(std::move(msg));
         if (flag)
            self->WriteImpl();
      });
   }

   // CRTP
   template<typename TaskType>
   void Read(TaskType&& task) {
      // check here
      static_assert(std::is_base_of_v<SessionBase<DerivedType>, DerivedType>);
      static_cast<DerivedType*>(this)->DoRead(std::forward<TaskType>(task)); 
   }

   uint64_t GetSessionId()    const noexcept { return session_id_; }

   std::time_t GetTimestamp() const noexcept { return last_timestamp_; }

   // just wrappers
   std::array<uint8_t, crypto_scalarmult_BYTES> GetPublicKey() 
      const noexcept { return keys_.GetPublicKey(); }

   std::string GetPublicKeyAsString()
      const noexcept { return keys_.GetKeyAsString(keys_.GetPublicKey()); }

protected:
   void WriteImpl() noexcept {
      asio::async_write(socket_, asio::buffer(message_queue_.front()), 
      [self = std::static_pointer_cast<SessionBase>(this->shared_from_this())] (const asio::error_code& ec, [[maybe_unused]] size_t length) {
         if (!ec) {
            // refresh timestamp
            self->last_timestamp_ = std::time(nullptr);
            // remove message
            self->message_queue_.pop_front();
            if (!self->message_queue_.empty())
               self->WriteImpl();
         }
         else {
            // todo: handle error
            // ...
         }
      });
   }

   uint64_t GenerateSessionId() const noexcept {
      // no need to mix (splitmix64 etc.) because Xoshiro256 algo
      return utils::Random<uint64_t>();
   }

protected:
   // socket from acceptor
   tcp::socket socket_;
   // message queue for write operations
   std::deque<std::string>
               message_queue_;
   // private and shared keys
   KeysWrapper keys_;
   // session id for managment by worker or smth else
   uint64_t    session_id_;
   // session lifetime (heartbeat)
   std::time_t last_timestamp_;
};

} // namespace remc::net

#endif // SESSION_BASE_H_

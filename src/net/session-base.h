#ifndef REMC_SESSION_BASE_H_
#define REMC_SESSION_BASE_H_

#include "include/remc-utils.h"
#include "crypto/keys.h"

#include <memory>
#include <ctime>
#include <utility>

#include <asio.hpp>

using asio::ip::tcp;

namespace remc::net {

// ===== SessionBase =====
//
// CRTP interface
template<typename DerivedType>
class SessionBase : public std::enable_shared_from_this<DerivedType> {
protected:
   // getting an external socket.
   // client: from connect
   // server: from acceptor
   SessionBase(tcp::socket&& socket) : 
      socket_(std::move(socket)),
      session_id_(GenerateSessionId()),
      last_timestamp_(std::time(nullptr)) {}

   // if, for example, you want to move a session to another worker (server-side),
   // you need to recreate the socket.
   SessionBase(const SessionBase&)     = delete;
   SessionBase(SessionBase&& other)    = delete;
   SessionBase& operator=(SessionBase) = delete;

   ~SessionBase() = default;

public:
   constexpr std::uint64_t GetSessionId() 
      const noexcept { return session_id_;      }

   constexpr std::time_t   GetTimestamp() 
      const noexcept { return last_timestamp_;  }

   constexpr std::uint64_t GetMessageCounter() 
      const noexcept { return message_counter_; }

   // returns ref to member.
   // work with keys using this function
   constexpr crypto::SessionsKeys& KeysInfo() noexcept { return keys_; }

protected:
   // call this when creating session.
   // no need to split-mix or smth else because Random<> uses xoshiro256 algo.
   std::uint64_t GenerateSessionId() const noexcept {
      return utils::Random<std::uint64_t>();
   }

protected:
   tcp::socket          socket_;
   // session private and shared key
   crypto::SessionsKeys keys_;
   // session id for managment by worker or smth else
   std::uint64_t        session_id_{};
   // nonce for cc20-poly1305.
   // increment this in Read()
   std::uint64_t        message_counter_{};
   // session lifetime (heartbeat)
   std::time_t          last_timestamp_{};
};

} // namespace remc::net

#endif // REMC_SESSION_BASE_H_

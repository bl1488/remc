#ifndef REMC_SESSION_BASE_H_
#define REMC_SESSION_BASE_H_

#include "include/remc_utils.h"
#include "crypto/keys_wrapper.h"

#if __cpp_lib_expected < 202202L
#  error "std::expected not supported, __cpp_lib_expected = " __cpp_lib_expected
#endif

#include <expected>
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
   SessionBase(tcp::socket&& socket) : 
      socket_(std::move(socket)),
      session_id_(GenerateSessionId()),
      last_timestamp_(std::time(nullptr)) {}

   SessionBase(const SessionBase&)     = delete;
   SessionBase(SessionBase&& other)    = delete;
   SessionBase& operator=(SessionBase) = delete;

   ~SessionBase() = default;

public:
   constexpr uint64_t    GetSessionId() 
      const noexcept { return session_id_;      }

   constexpr std::time_t GetTimestamp() 
      const noexcept { return last_timestamp_;  }

   constexpr uint64_t    GetMessageCounter() 
      const noexcept { return message_counter_; }

   // returns ref to member
   constexpr crypto::KeysWrapper& KeysInfo() noexcept { return keys_; }

protected:
   uint64_t GenerateSessionId() const noexcept {
      // no need to mix because xoshiro algo
      return utils::Random<uint64_t>();
   }

protected:
   // socket from acceptor
   tcp::socket socket_;
   // private and shared keys
   crypto::KeysWrapper keys_;
   // session id for managment by worker or smth else
   std::size_t session_id_{};
   // nonce for cc20-poly1305
   std::size_t message_counter_{};
   // session lifetime (heartbeat)
   std::time_t last_timestamp_{};
};

} // namespace remc::net

#endif // REMC_SESSION_BASE_H_

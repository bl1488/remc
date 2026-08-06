#ifndef REMC_SESSION_BASE_H_
#define REMC_SESSION_BASE_H_

#include "net/common.h"
#include "include/remc_utils.h"
#include "keys_wrapper.h"
#include "crypto/cc20poly1305.h"

#include <memory>
#include <ctime>
#include <utility>
#include <string>

#include <asio.hpp>

using asio::ip::tcp;

namespace remc::net {

// ===== SessionBase =====
//
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
   uint64_t    GetSessionId()      const noexcept { return session_id_;      }

   std::time_t GetTimestamp()      const noexcept { return last_timestamp_;  }

   uint64_t    GetMessageCounter() const noexcept { return message_counter_; }

   KeysWrapper& KeysInfo()               noexcept { return keys_;            }

protected:
   uint64_t GenerateSessionId() const noexcept {
      // no need to mix because xoshiro algo
      return utils::Random<uint64_t>();
   }

protected:
   // socket from acceptor
   tcp::socket socket_;
   // private and shared keys
   KeysWrapper keys_;
   // session id for managment by worker or smth else
   uint64_t    session_id_;
   // nonce for cc20-poly1305
   uint64_t    message_counter_{};
   // session lifetime (heartbeat)
   std::time_t last_timestamp_;
};

std::vector<std::byte> CreatePacket(std::span<std::byte> 
                                              payload,
                                    uint16_t  version, 
                                    uint32_t  flags, 
                                    uint64_t  nonce,
                                    uint64_t  message_id,
                                    std::span<const std::byte>
                                              shared_key);

std::optional<Packet> ReadPacket(std::vector<std::byte> 
                                          buffer,
                                 uint64_t nonce, 
                                 std::span<const std::byte> 
                                          shared_key);

} // namespace remc::net

#endif // REMC_SESSION_BASE_H_

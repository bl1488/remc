#ifndef REMC_SESSION_BASE_H_
#define REMC_SESSION_BASE_H_

#include "net/common.h"
#include "include/remc_utils.h"
#include "crypto/keys_wrapper.h"

#include <expected>
#include <memory>
#include <ctime>
#include <span>
#include <utility>

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

   crypto::KeysWrapper& KeysInfo()       noexcept { return keys_;            }

protected:
   uint64_t GenerateSessionId() const noexcept {
      // no need to mix because xoshiro algo
      return utils::Random<uint64_t>();
   }

protected:
   // socket from acceptor
   tcp::socket socket_;
   // private and shared keys
   crypto::KeysWrapper 
               keys_;
   // session id for managment by worker or smth else
   uint64_t    session_id_;
   // nonce for cc20-poly1305
   uint64_t    message_counter_{};
   // session lifetime (heartbeat)
   std::time_t last_timestamp_;
};

// ===== PacketError =====
// use for return value in Create/Read-Packet
class PacketError {
public:
   enum class Code : uint8_t {
      NO_ERROR,
      ERROR_INVALID_TIMESTAMP,
      ERROR_INVALID_MESSAGE_LENGTH,
      ERROR_INVALID_TAG,
      ERROR_INVALID_SERIALIZATION,
      ERROR_INVALID_ARGUMENTS
   };
public:
   PacketError(Code code, std::string error_message) : 
      code_(code), message_(std::move(error_message)) {}

public:
   std::string_view Message() const noexcept { return message_; }

   Code GetCode()             const noexcept { return code_;    }

   const char* CodeAsString() const noexcept {
      switch (code_) {
      case Code::NO_ERROR:                     return "NO_ERROR";
      case Code::ERROR_INVALID_TIMESTAMP:      return "ERROR_INVALID_TIMESTAMP";
      case Code::ERROR_INVALID_MESSAGE_LENGTH: return "ERROR_INVALID_MESSAGE_LENGTH";
      case Code::ERROR_INVALID_TAG:            return "ERROR_INVALID_TAG";
      case Code::ERROR_INVALID_SERIALIZATION:  return "ERROR_INVALID_SERIALIZATION";
      case Code::ERROR_INVALID_ARGUMENTS:      return "ERROR_INVALID_ARGUMENTS";
      }
      return "unknown error";
   }

private:
   Code        code_;
   std::string message_;
};

std::expected<std::vector<std::byte>, PacketError> CreatePacket(
   std::span<std::byte>       payload,
   uint16_t                   version, 
   uint32_t                   flags, 
   uint64_t                   nonce,
   uint64_t                   message_id,
   std::span<const std::byte> shared_key
);

std::expected<Packet, PacketError> ReadPacket(
   std::vector<std::byte>     buffer,
   uint64_t                   nonce, 
   std::span<const std::byte> shared_key
);

} // namespace remc::net

#endif // REMC_SESSION_BASE_H_

#ifndef REMC_NET_PACKET_H_
#define REMC_NET_PACKET_H_

// fix libstdc++
#if __cpp_lib_expected < 202202L
#  error "std::expected not supported, __cpp_lib_expected = " __cpp_lib_expected
#endif

#include <expected>

namespace remc::net {

// ===== Packet =====
//
class Packet {
public:
   struct Header {
      uint16_t version;
      uint32_t flags;
      uint64_t timestamp;
      uint64_t message_id;
      uint32_t message_size;
      uint64_t nonce;
   };
   enum Flags : unsigned {
      NoFlags     = 0,
      NoCrypto    = 1u,       // crypt/decrypt payload
      Handshake   = 1u << 1,  // handshake
      //////////
      TestMessage = 0xff
   };

public:
   Packet()  = default;
   ~Packet() = default;

   Packet(const Packet&)                = default;
   Packet& operator=(const Packet&)     = default;
   Packet(Packet&&) noexcept            = default;
   Packet& operator=(Packet&&) noexcept = default;

public:
   // tag stored in the last 16 bytes of payload
   std::span<const std::byte> GetTag() const noexcept;

   std::string GetPayloadAsString() const;

public:
   Header header{};
   std::vector<std::byte> payload;
};

// ===== PacketError =====
//
// use for return value in Create/Read-Packet
class PacketError {
public:
   enum class ErrorCode : uint8_t {
      NoError,
      InvalidTimestamp,
      InvalidMessageLength,
      InvalidTag,
      InvalidSerialization,
      InvalidArguments
   };
public:
   PacketError(ErrorCode code, std::string error_message) :
      code_(code), message_(std::move(error_message)) {}

   PacketError(ErrorCode code, const char* error_message) : 
      code_(code), message_(error_message) {}

public:
   std::string_view Message()    const noexcept { return message_; }
   
   constexpr ErrorCode GetCode() const noexcept { return code_;    }
      
   constexpr static uint8_t GetCodeAsInt(ErrorCode ec) noexcept { 
      return static_cast<uint8_t>(ec); 
   }
   constexpr uint8_t GetCodeAsInt() const noexcept { 
      return static_cast<uint8_t>(code_); 
   }

   constexpr static const char* GetCodeAsString(ErrorCode ec) noexcept {
      switch (ec) {
      case ErrorCode::NoError:              return "NoError";
      case ErrorCode::InvalidTimestamp:     return "InvalidTimestamp";
      case ErrorCode::InvalidMessageLength: return "InvalidMessageLength";
      case ErrorCode::InvalidTag:           return "InvalidTag";
      case ErrorCode::InvalidSerialization: return "InvalidSerialization";
      case ErrorCode::InvalidArguments:     return "InvalidArguments";
      }
      return "unknown error code";
   }
   constexpr const char* GetCodeAsString() const noexcept {
      return GetCodeAsString(code_);
   }

private:
   ErrorCode   code_;
   std::string message_;
};

///////////////

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

///////////////

// global consts
// for test
namespace global {

// buffers limit
static constexpr std::size_t TCP_PAYLOAD_SIZE_MAX  = 512;
static constexpr std::size_t TCP_TOTAL_PACKET_SIZE = TCP_PAYLOAD_SIZE_MAX + sizeof(Packet::Header) + 16;

// timestamp limit after which the operation is considered invalid.
// seconds
static constexpr std::time_t TIMESTAMP_LIMIT = 30;

static constexpr std::size_t RING_BUFFER_CAPACITY = TCP_TOTAL_PACKET_SIZE * 10;

} // namespace global

} // namespace remc::net

#endif // REMC_NET_PACKET_H_

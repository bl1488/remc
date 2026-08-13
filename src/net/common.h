#ifndef REMC_NET_COMMON_H_
#define REMC_NET_COMMON_H_

#include <cstdint>
#include <span>
#include <string>
#include <vector>

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
   
   enum Flags : uint16_t {
      FLAG_NO_CRYPTO      = 0b01,  // crypt/decrypt payload
      FLAG_TYPE_HANDSHAKE = 0b11,  // handshake
      FLAG_TEST_MESSAGE   = 0b11111111111111
   };

public:
   // tag stored in the last 16 bytes of payload
   std::span<const std::byte> GetTag() const noexcept {
      if (payload.size() >= 16)
         return { payload.data() + payload.size() - 16, 16 };
      return {};
   }

   std::string GetPayloadAsString() const noexcept {
      return std::string(reinterpret_cast<const char*>(payload.data()), 
                         payload.size());
   }

public:
   Header                 header;
   std::vector<std::byte> payload;
};

namespace global {

// global vars for tests

// timestamp limit after which the operation is considered invalid.
// seconds
static constexpr std::time_t TIMESTAMP_LIMIT          = 30; 

static constexpr std::size_t TCP_PAYLOAD_SIZE_MAX     = 512;
static constexpr std::size_t TCP_TOTAL_PACKET_SIZE    = TCP_PAYLOAD_SIZE_MAX + sizeof(Packet::Header) + 16;

// seconds
static constexpr std::time_t WORKER_HEARTBEAT_TIMEOUT = TIMESTAMP_LIMIT;
static constexpr std::time_t WORKER_HEARTBEAT_PERIOD  = 5;
// max client count per worker
static constexpr std::size_t WORKER_MAX_CLIENT_COUNT  = 4096;

static constexpr std::size_t RING_BUFFER_CAPACITY     = TCP_TOTAL_PACKET_SIZE * 10;

} // namespace remc::net::global

} // namespace remc::net

#endif // REMC_NET_COMMON_H_

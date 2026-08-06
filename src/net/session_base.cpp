#include "session_base.h"
#include "bitsery/serializer.h"
#include "crypto/cc20poly1305.h"
#include "server.h"
#include "client.h"

#include <optional>

#include <bitsery/bitsery.h>
#include <bitsery/deserializer.h>
#include <bitsery/details/adapter_common.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/vector.h>

namespace remc::net {

// ==============================
//       bitsery instances
// ==============================
template<typename S>
void serialize(S& s, remc::net::Packet::Header& header) {
   s.value2b(header.version);
   s.value4b(header.flags);
   s.value8b(header.timestamp);
   s.value8b(header.message_id);
   s.value4b(header.message_size);
   s.value8b(header.nonce);
}

template <typename S>
void serialize(S& s, Packet& packet) {
   // header
   serialize(s, packet.header);
   // payload
   s.container1b(packet.payload, packet.header.message_size);
}

std::vector<std::byte> CreatePacket(const std::span<std::byte> 
                                             payload,
                                    uint16_t version,
                                    uint32_t flags, 
                                    uint64_t nonce,
                                    uint64_t message_id,
                                    std::span<const std::byte>
                                             shared_key) 
{
   using BufferType = std::vector<std::byte>;
   using OutAdapter = bitsery::OutputBufferAdapter<BufferType>;

   Packet packet;
   packet.header.version      = version;
   packet.header.flags        = flags;
   packet.header.timestamp    = static_cast<uint64_t>(std::time(nullptr));
   packet.header.message_id   = message_id;
   packet.header.nonce        = nonce;
   packet.header.message_size = static_cast<uint32_t>(
      flags & Packet::Flags::FLAG_NO_CRYPTO ? payload.size() : payload.size() + 16);

   packet.payload.resize(packet.header.message_size);

   // serialized header = AAD
   BufferType ser_header;
   bitsery::quickSerialization<OutAdapter>(ser_header, packet.header);

   if (!(flags & Packet::Flags::FLAG_NO_CRYPTO)) {
      crypto::AEADChaCha20Poly1305 cc20(const_cast<std::byte*>(shared_key.data()), shared_key.size());
      cc20.Encrypt(payload,
                   ser_header,
                   crypto::AEADChaCha20Poly1305::Nonce96(nonce, 0),
                   packet.payload);
   }
   else std::memcpy(packet.payload.data(), payload.data(), packet.header.message_size);
   
   BufferType result;
   bitsery::quickSerialization<OutAdapter>(result, packet);
   
   return result;
}

std::optional<Packet> ReadPacket(std::vector<std::byte> 
                                          buffer,
                                 uint64_t nonce,
                                 std::span<const std::byte>
                                          shared_key) 
{
   using BufferType = std::vector<std::byte>;
   using InAdapter  = bitsery::InputBufferAdapter<BufferType>;
   using OutAdapter = bitsery::OutputBufferAdapter<BufferType>;

   Packet packet;

   auto state = bitsery::quickDeserialization<InAdapter>({ buffer.begin(), buffer.size() }, packet);
   if (state.first != bitsery::ReaderError::NoError) {
      GlobalLogDebug("deserialization failed");
      return std::nullopt;
   }

   // check packet valid
   // timestamp
   std::time_t now = std::time(nullptr);
   if (now - packet.header.timestamp > global::TIMESTAMP_LIMIT) {
      GlobalLogError("invalid packet timestamp");
      return std::nullopt;
   }
   // message size
   if (packet.header.message_size != packet.payload.size()) {
      GlobalLogDebug("message size mismatch: {}:{}", packet.header.message_size, packet.payload.size());
      return std::nullopt;
   }

   BufferType ser_header;
   bitsery::quickSerialization<OutAdapter>(ser_header, packet.header);
   
   // decrypt payload?
   if (!(packet.header.flags & Packet::Flags::FLAG_NO_CRYPTO)) {
      std::vector<std::byte> payload;
      payload.resize(packet.header.message_size - 16);

      crypto::AEADChaCha20Poly1305 cc20(const_cast<std::byte*>(shared_key.data()), shared_key.size());
      if (!cc20.Decrypt(packet.payload,
                        ser_header, 
                        crypto::AEADChaCha20Poly1305::Nonce96(nonce, 0),
                        payload)) {
         GlobalLogDebug("packet AEAD missmatch");
         return std::nullopt;
      }
      packet.payload = std::move(payload);
   }
   // else: just do nothing
   // payload already decrypted

   return packet;
}
   
// instance Server/Client here
template class SessionBase<SessionServer>;
template class SessionBase<SessionClient>;

} // namespace remc::net

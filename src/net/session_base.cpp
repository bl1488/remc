#include "session_base.h"
#include "crypto/cc20poly1305.h"
#include "net/common.h"
#include "server.h"
#include "client.h"

#include <bitsery/serializer.h>
#include <bitsery/bitsery.h>
#include <bitsery/deserializer.h>
#include <bitsery/details/adapter_common.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/vector.h>

namespace remc::net {

//
// bitsery instances
// 
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

//
// CreatePacket
//
std::expected<std::vector<std::byte>, PacketError> CreatePacket(
   const std::span<std::byte> 
            payload,
   uint16_t version,
   uint32_t flags, 
   uint64_t nonce,
   uint64_t message_id,
   std::span<const std::byte>
            shared_key) 
{
   // check arguments valid
   if (payload.size() > global::TCP_PAYLOAD_SIZE_MAX || shared_key.empty()) {
      return std::unexpected(PacketError(
         PacketError::Code::ERROR_INVALID_ARGUMENTS,
         std::format("invalid arguments: (size:{}) (key:{})", payload.size(), reinterpret_cast<const void*>(shared_key.data()))
      ));
   }

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

   // AAD
   BufferType ser_header;
   bitsery::quickSerialization<OutAdapter>(ser_header, packet.header);

   if (!(flags & Packet::Flags::FLAG_NO_CRYPTO)) {
      crypto::AEADChaCha20Poly1305 cc20(const_cast<std::byte*>(shared_key.data()), shared_key.size());
      cc20.Encrypt(
         payload,
         ser_header,
         crypto::AEADChaCha20Poly1305::Nonce96(nonce, 0),
         packet.payload
      );
   }
   else std::memcpy(packet.payload.data(), payload.data(), packet.header.message_size);
   
   BufferType result;
   bitsery::quickSerialization<OutAdapter>(result, packet);
   
   return result;
}

//
// ReadPacket
//
std::expected<Packet, PacketError> ReadPacket(
   std::vector<std::byte> 
            buffer,
   uint64_t nonce,
   std::span<const std::byte>
            shared_key)
{
   // check arguments valid
   if (shared_key.empty()) {
      return std::unexpected(PacketError(
         PacketError::Code::ERROR_INVALID_ARGUMENTS,
         std::format("invalid arguments: (key:{})", reinterpret_cast<const void*>(shared_key.data()))
      ));
   }

   using BufferType = std::vector<std::byte>;
   using InAdapter  = bitsery::InputBufferAdapter<BufferType>;
   using OutAdapter = bitsery::OutputBufferAdapter<BufferType>;

   Packet packet;

   auto state = bitsery::quickDeserialization<InAdapter>({ buffer.begin(), buffer.size() }, packet);
   if (state.first != bitsery::ReaderError::NoError) {
      return std::unexpected(PacketError(
         PacketError::Code::ERROR_INVALID_SERIALIZATION,
         std::format("deserialization error: {}", static_cast<int>(state.first)).c_str()
      ));
   }
   // AAD
   BufferType ser_header;
   bitsery::quickSerialization<OutAdapter>(ser_header, packet.header);
   
   // decrypt payload?
   if (!(packet.header.flags & Packet::FLAG_NO_CRYPTO)) {
      std::vector<std::byte> payload;
      payload.resize(packet.header.message_size - 16);

      crypto::AEADChaCha20Poly1305 cc20(const_cast<std::byte*>(shared_key.data()), shared_key.size());
      if (!cc20.Decrypt(packet.payload,
                        ser_header, 
                        crypto::AEADChaCha20Poly1305::Nonce96(nonce, 0),
                        payload)) 
      {
         return std::unexpected(PacketError(
            PacketError::Code::ERROR_INVALID_TAG,
            "AEAD mismatch"
         ));
      }
      packet.payload = std::move(payload);
      packet.header.message_size -= 16;
   }
   // else: just do nothing
   // payload already decrypted

   // check packet valid
   // timestamp
   std::time_t now = std::time(nullptr);
   if (now - packet.header.timestamp > global::TIMESTAMP_LIMIT) {
      return std::unexpected(PacketError(
         PacketError::Code::ERROR_INVALID_TIMESTAMP,
         std::format("invalid packet timestamp ({})", packet.header.timestamp)
      ));
   }
   // message size
   if (packet.header.message_size != packet.payload.size()) {
      return std::unexpected(PacketError(
         PacketError::Code::ERROR_INVALID_TIMESTAMP,
         std::format("message size mismatch: {}:{}", packet.header.message_size, packet.payload.size())
      ));
   }

   return packet;
}

// instance Server/Client here
template class SessionBase<SessionServer>;
template class SessionBase<SessionClient>;

} // namespace remc::net

// Poly1305 impl from Bitcoin Core
// URL: https://github.com/bitcoin/bitcoin/blob/master/src/crypto/poly1305.h
//
#ifndef CRYPTO_POLY1305_H_
#define CRYPTO_POLY1305_H_

#include <cassert>
#include <cstddef>
#include <span>
#include <cstdint>

namespace remc::crypto {

constexpr int POLY1305_BLOCK_SIZE = 16;

// Based on the public domain implementation by Andrew Moon
// poly1305-donna-32.h from https://github.com/floodyberry/poly1305-donna
typedef struct {
   uint32_t r[5];
   uint32_t h[5];
   uint32_t pad[4];
   size_t   leftover;
   uint8_t  buffer[POLY1305_BLOCK_SIZE];
   uint8_t  final;
} poly1305_context;

void poly1305_init(poly1305_context   *st, const unsigned char key[32])          noexcept;

void poly1305_update(poly1305_context *st, const unsigned char *m, size_t bytes) noexcept;

void poly1305_finish(poly1305_context *st, unsigned char mac[16])                noexcept;

// C++ wrapper with std::byte span interface around poly1305_donna code.
class Poly1305 {
public:
   // Length of the output produced by Finalize().
   static constexpr unsigned TAGLEN{16};
   // Length of the keys expected by the constructor.
   static constexpr unsigned KEYLEN{32};
public:
   // Construct a Poly1305 object with a given 32-byte key.
   Poly1305(std::span<const std::byte> key) noexcept {
      assert(key.size() == KEYLEN);
      poly1305_init(&m_ctx, reinterpret_cast<const uint8_t*>(key.data()));
   }

   // My overload
   Poly1305(void* ptr, std::size_t size) noexcept :
      Poly1305({ reinterpret_cast<const std::byte*>(ptr), size }) {}

public:
   // Process message bytes.
   Poly1305& Update(std::span<const std::byte> msg) noexcept {
      poly1305_update(&m_ctx,  reinterpret_cast<const uint8_t*>(msg.data()), msg.size());
      return *this;
   }

   // Write authentication tag to 16-byte out.
   void Finalize(std::span<std::byte> out) noexcept {
      assert(out.size() == TAGLEN);
      poly1305_finish(&m_ctx,  reinterpret_cast<uint8_t*>(out.data()));
   }

private:
   poly1305_context m_ctx;
};

} // namespace remc::crypto

#endif // CRYPTO_POLY1305_H_
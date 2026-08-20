// ChaCha20 impl from Bitcoin Core
// URL: https://github.com/bitcoin/bitcoin/blob/master/src/crypto/chacha20.h
//
#ifndef CRYPTO_CHACHA20_H_
#define CRYPTO_CHACHA20_H_

#include "include/remc_utils.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <cassert>
#include <array>
#include <utility>

namespace remc::crypto {

// The 128-bit input is here implemented as a 96-bit nonce and a 32-bit block
// counter, as in RFC8439 Section 2.3. When the 32-bit block counter overflows
// the first 32-bit part of the nonce is automatically incremented, making it
// conceptually compatible with variants that use a 64/64 split instead.

// ChaCha20 cipher that only operates on multiples of 64 bytes
class ChaCha20Aligned {
public:
    // Expected key length in constructor and SetKey
   static constexpr unsigned KEYLEN{32};
   // Block size (inputs/outputs to Keystream / Crypt should be multiples of this)
   static constexpr unsigned BLOCKLEN{64};
   // Type for 96-bit nonces used by the Set function below
   //
   // The first field corresponds to the LE32-encoded first 4 bytes of the nonce, also referred
   // to as the '32-bit fixed-common part' in Example 2.8.2 of RFC8439
   //
   // The second field corresponds to the LE64-encoded last 8 bytes of the nonce
   using Nonce96 = std::pair<uint32_t, uint64_t>;   
public:
   // deleted
   ChaCha20Aligned() noexcept = delete;

   // Initialize a cipher with specified 32-byte key
   ChaCha20Aligned(std::span<const std::byte> key) noexcept { SetKey(key); }

   // Destructor to clean up private memory
   ~ChaCha20Aligned() { remc::utils::SecZeroMemory(input, sizeof(input)); }

public:
   // Set 32-byte key, and seek to nonce 0 and block position 0
   void SetKey(std::span<const std::byte> key) noexcept;

   // Set the 96-bit nonce and 32-bit block counter
   //
   // Block_counter selects a position to seek to (to byte BLOCKLEN*block_counter). After 256 GiB,
   // the block counter overflows, and nonce.first is incremented
   void Seek(Nonce96 nonce, uint32_t block_counter) noexcept;

   // outputs the keystream into out, whose length must be a multiple of BLOCKLEN
   void Keystream(std::span<std::byte> out) noexcept;

   // en/deciphers the message <input> and write the result into <output>
   // The size of input and output must be equal, and be a multiple of BLOCKLEN.
   void Crypt(std::span<const std::byte> input, std::span<std::byte> output) noexcept;

private:
    uint32_t input[12];
};

// Unrestricted ChaCha20 cipher
class ChaCha20 {
public:
   // Expected key length in constructor and SetKey
   static constexpr unsigned KEYLEN = ChaCha20Aligned::KEYLEN;
   // 96-bit nonce type
   using Nonce96 = ChaCha20Aligned::Nonce96;
public:
   // For safety, disallow initialization without key
   ChaCha20() noexcept = delete;
   // Initialize a cipher with specified 32-byte key
   ChaCha20(std::span<const std::byte> key) noexcept : m_aligned(key) {}
   // my overload
   ChaCha20(void* data, std::size_t size) noexcept : 
      m_aligned({ reinterpret_cast<const std::byte*>(data), size }) {}
   
   // Destructor to clean up private memory
   ~ChaCha20() { remc::utils::SecZeroMemory(m_buffer.data(), m_buffer.size()); }

public:
   // Set 32-byte key, and seek to nonce 0 and block position 0
   void SetKey(std::span<const std::byte> key)      noexcept;
   // my overload
   void SetKey(void* data, std::size_t size) noexcept {
      SetKey({ reinterpret_cast<const std::byte*>(data), size });
   }

   // Set the 96-bit nonce and 32-bit block counter. See ChaCha20Aligned::Seek
   void Seek(Nonce96 nonce, uint32_t block_counter) noexcept {
      m_aligned.Seek(nonce, block_counter);
      m_bufleft = 0;
   }

   // en/deciphers the message <in_bytes> and write the result into <out_bytes>
   // The size of in_bytes and out_bytes must be equal
   void Crypt(std::span<const std::byte> in_bytes, std::span<std::byte> out_bytes) noexcept;
   // my overload
   // out string should be resized like in
   void Crypt(const std::string& in, std::string& out) noexcept {
      Crypt({ reinterpret_cast<const std::byte*>(in.data()), in.size() }, 
            { reinterpret_cast<std::byte*>(out.data()), out.size() });
   }

   // outputs the keystream to out
   void Keystream(std::span<std::byte> out) noexcept;

private:
    ChaCha20Aligned m_aligned;
    std::array<std::byte, ChaCha20Aligned::BLOCKLEN> 
                    m_buffer;
    unsigned        m_bufleft{0};
};

} // namespace remc::crypto

#endif // CRYPTO_CHACHA20_H_

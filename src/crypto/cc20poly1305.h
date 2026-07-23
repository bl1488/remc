// ChaCha20Poly1305 impl from Bitcoin Core
// URL: https://github.com/bitcoin/bitcoin/blob/master/src/crypto/chacha20poly1305.h
//
#ifndef CRYPTO_CC20_POLY1305_H_
#define CRYPTO_CC20_POLY1305_H_

#include "chacha20.h"
#include "poly1305.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <cassert>

namespace remc::crypto {

//
// AEADChaCha20Poly1305
// The AEAD_CHACHA20_POLY1305 authenticated encryption algorithm from RFC8439 section 2.8.
//
class AEADChaCha20Poly1305 {
public:
    // Expected size of key argument in constructor.
    static constexpr unsigned KEYLEN = 32;
    // Expansion when encrypting.
    static constexpr unsigned EXPANSION = Poly1305::TAGLEN;
    // 96-bit nonce type
    using Nonce96 = ChaCha20::Nonce96;
public:
    // Initialize an AEAD instance with a specified 32-byte key.
    AEADChaCha20Poly1305(std::span<const std::byte> key) noexcept : m_chacha20(key) { 
        assert(key.size() == KEYLEN); 
    }

    // My overload
    AEADChaCha20Poly1305(void* key, std::size_t size) noexcept :
        AEADChaCha20Poly1305({ reinterpret_cast<const std::byte*>(key), size }) {}

public:
    // Switch to another 32-byte key.
    void SetKey(std::span<const std::byte> key) noexcept {
        assert(key.size() == KEYLEN);
        m_chacha20.SetKey(key);
    }

    // Encrypt a message with a specified 96-bit nonce and aad.
    //
    // Requires cipher.size() = plain.size() + EXPANSION.
    //
    void Encrypt(std::span<const std::byte> plain, std::span<const std::byte> aad, Nonce96 nonce, std::span<std::byte> cipher) 
        noexcept { Encrypt(plain, {}, aad, nonce, cipher); }

    // My overload
    void Encrypt(const std::string& test, const std::string& add, Nonce96 nonce, std::string& out) noexcept { 
        Encrypt({ reinterpret_cast<const std::byte*>(test.data()), test.size() }, 
                { reinterpret_cast<const std::byte*>(add.data()),  add.size()  }, nonce, 
                { reinterpret_cast<std::byte*>      (out.data()),  out.size()  });
    }

    // Encrypt a message (given split into plain1 + plain2) with a specified 96-bit nonce and aad.
    //
    // Requires cipher.size() = plain1.size() + plain2.size() + EXPANSION.
    //
    void Encrypt(std::span<const std::byte> plain1, 
                 std::span<const std::byte> plain2, 
                 std::span<const std::byte> aad, 
                 Nonce96                    nonce, 
                 std::span<std::byte>       cipher) noexcept;

    // Decrypt a message with a specified 96-bit nonce and aad. Returns true if valid.
    //
    // Requires cipher.size() = plain.size() + EXPANSION.
    //
    bool Decrypt(std::span<const std::byte> cipher, std::span<const std::byte> aad, Nonce96 nonce, std::span<std::byte> plain) 
        noexcept { return Decrypt(cipher, aad, nonce, plain, {}); }

    // Decrypt a message with a specified 96-bit nonce and aad and split the result. Returns true if valid.
    //
    // Requires cipher.size() = plain1.size() + plain2.size() + EXPANSION.
    //
    bool Decrypt(std::span<const std::byte> cipher, 
                 std::span<const std::byte> aad, 
                 Nonce96                    nonce, 
                 std::span<std::byte>       plain1, 
                 std::span<std::byte>       plain2) noexcept;

    // Get a number of keystream bytes from the underlying stream cipher.
    //
    // This is equivalent to Encrypt() with plain set to that many zero bytes, and dropping the
    // last EXPANSION bytes off the result.
    //
    void Keystream(Nonce96 nonce, std::span<std::byte> keystream) noexcept;

private:
    // Internal stream cipher.
    ChaCha20 m_chacha20;
};

// 
// FSChaCha20Poly1305
// Forward-secure wrapper around AEADChaCha20Poly1305.
//
// This implements an AEAD which automatically increments the nonce on every encryption or
// decryption, and cycles keys after a predetermined number of encryptions or decryptions.
//
// See BIP324 for details.
//
class FSChaCha20Poly1305 {
public:
    // Length of keys expected by the constructor.
    static constexpr auto KEYLEN    = AEADChaCha20Poly1305::KEYLEN;
    // Expansion when encrypting.
    static constexpr auto EXPANSION = AEADChaCha20Poly1305::EXPANSION;    
    // Default rekey interval.
    static constexpr auto INTERVAL  = 214;
public:
    // Deleted
    // No copy or move to protect the secret.
    FSChaCha20Poly1305(const FSChaCha20Poly1305&)            = delete;
    FSChaCha20Poly1305(FSChaCha20Poly1305&&)                 = delete;
    FSChaCha20Poly1305& operator=(const FSChaCha20Poly1305&) = delete;
    FSChaCha20Poly1305& operator=(FSChaCha20Poly1305&&)      = delete;

    // Construct an FSChaCha20Poly1305 cipher that rekeys every rekey_interval operations.
    FSChaCha20Poly1305(std::span<const std::byte> key, uint32_t rekey_interval) noexcept :
        m_aead(key), m_rekey_interval(rekey_interval) {}

    // My overload
    FSChaCha20Poly1305(void* key, std::size_t key_size, uint32_t rekey_interval = INTERVAL) noexcept :
        FSChaCha20Poly1305({ reinterpret_cast<const std::byte*>(key), key_size }, rekey_interval) {}

public:
    // Encrypt a message with a specified aad.
    //
    // Requires cipher.size() = plain.size() + EXPANSION.
    //
    void Encrypt(std::span<const std::byte> plain, std::span<const std::byte> aad, std::span<std::byte> cipher)
        noexcept { Encrypt(plain, {}, aad, cipher); }

    // Encrypt a message (given split into plain1 + plain2) with a specified aad.
    //
    // Requires cipher.size() = plain.size() + EXPANSION.
    //
    void Encrypt(std::span<const std::byte> plain1, 
                 std::span<const std::byte> plain2, 
                 std::span<const std::byte> aad, 
                 std::span<std::byte>       cipher) noexcept;

    // My overload
    //
    // Client can decrypt message by reading the `message_length` field from packet.
    // aad = str.size()
    //
    void Encrypt(const std::string& text, std::span<const std::byte> aad, std::string& out) noexcept {
        Encrypt({ reinterpret_cast<const std::byte*>(text.data()), text.size() }, aad,
                { reinterpret_cast<std::byte*>(out.data()),        out.size()  });
    }

    // Decrypt a message with a specified aad. Returns true if valid.
    //
    // Requires cipher.size() = plain.size() + EXPANSION.
    //
    bool Decrypt(std::span<const std::byte> cipher, std::span<const std::byte> aad, std::span<std::byte> plain) 
        noexcept { return Decrypt(cipher, aad, plain, {}); }

    // Decrypt a message with a specified aad and split the result. Returns true if valid.
    //
    // Requires cipher.size() = plain1.size() + plain2.size() + EXPANSION.
    //
    bool Decrypt(std::span<const std::byte> cipher, 
                 std::span<const std::byte> aad, 
                 std::span<std::byte>       plain1, 
                 std::span<std::byte>       plain2) noexcept;

    // My overload
    bool Decrypt(const std::string& cipher, std::span<const std::byte> aad, std::string& out) noexcept {
        return Decrypt({ reinterpret_cast<const std::byte*>(cipher.data()), cipher.size() }, aad,
                       { reinterpret_cast<std::byte*>(out.data()),          out.size()    });
    }

private:
    // Update counters (and if necessary, key) to transition to the next message.
    void NextPacket() noexcept;

private:
    // Internal AEAD.
    AEADChaCha20Poly1305 m_aead;
    // Every how many iterations this cipher rekeys.
    const uint32_t m_rekey_interval;
    // The number of encryptions/decryptions since the last rekey.
    uint32_t m_packet_counter = 0;
    // The number of rekeys performed so far.
    uint64_t m_rekey_counter  = 0;
};

} // namespace remc::crypto

#endif // CRYPTO_CC20_POLY1305_H_

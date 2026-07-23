#ifndef KEYS_WRAPPER_H_
#define KEYS_WRAPPER_H_

#include <cstring>
#include <array>
#include <string>

#include <sodium.h>

namespace remc::net {

//
// KeysWrapper
//
//
class KeysWrapper {
public:
   KeysWrapper() noexcept : shared_key_({}) {
      // generate private key
      ::randombytes_buf(private_key_.data(), private_key_.size());
   }

   KeysWrapper(KeysWrapper&& other) noexcept :
      private_key_(std::move(other.private_key_)), shared_key_(std::move(other.shared_key_)) {};

   KeysWrapper& operator=(KeysWrapper&& other) noexcept;

   // deleted
   KeysWrapper(const KeysWrapper&)            = delete;
   KeysWrapper& operator=(const KeysWrapper&) = delete;

   ~KeysWrapper() noexcept;

public:
   std::array<uint8_t, crypto_scalarmult_BYTES> GetPublicKey() const noexcept;

   const std::array<uint8_t, crypto_scalarmult_BYTES>& GetSharedKey() 
      const noexcept { return shared_key_; }

   [[nodiscard]] bool ComputeSharedKey(
      const std::array<uint8_t, crypto_scalarmult_BYTES>& other_public_key) noexcept;

   std::string GetKeyAsString(const std::array<uint8_t, crypto_scalarmult_BYTES>& key) const;

private:
   std::array<uint8_t, crypto_scalarmult_BYTES> private_key_, 
                                                shared_key_;
};

} // namespace remc::net

#endif // KEYS_WRAPPER_H_

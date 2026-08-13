#ifndef REMC_KEYS_WRAPPER_H_
#define REMC_KEYS_WRAPPER_H_

#include <cstring>
#include <array>
#include <string>
#include <span>
#include <string_view>

#include <sodium.h>

namespace remc::crypto {

// ===== KeysWrapper =====
//
class KeysWrapper {
public:
   KeysWrapper() {
      // generate private key
      ::randombytes_buf(private_key_.data(), private_key_.size());
   }

   KeysWrapper(KeysWrapper&& other) noexcept :
      private_key_(std::move(other.private_key_)), shared_key_(std::move(other.shared_key_)) {};

   KeysWrapper& operator=(KeysWrapper&& other) noexcept;

   KeysWrapper(const KeysWrapper&)            = delete;
   KeysWrapper& operator=(const KeysWrapper&) = delete;

   ~KeysWrapper() noexcept;

public:
   std::array<std::byte, crypto_scalarmult_BYTES> GetPublicKey() const noexcept;

   const std::array<std::byte, crypto_scalarmult_BYTES>& GetSharedKey() 
      const noexcept { return shared_key_; }

   [[nodiscard]] bool ComputeSharedKey(std::span<const std::byte> other_public_key) noexcept;

   static std::string GetKeyAsHexString(std::span<const std::byte> key);
   
   std::string_view   GetKeyAsString(const std::array<std::byte, crypto_scalarmult_BYTES>& key) const;

private:
   std::array<std::byte, crypto_scalarmult_BYTES> private_key_, 
                                                  shared_key_{};
};

} // namespace remc::net

#endif // REMC_KEYS_WRAPPER_H_

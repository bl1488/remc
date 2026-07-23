#include "keys_wrapper.h"

#include <format>

#include <sodium/crypto_scalarmult.h>
#include <sodium/utils.h>

namespace remc::net {

//
// KeysWrapper impl
//
//
KeysWrapper::~KeysWrapper() noexcept {
   ::sodium_memzero(private_key_.data(), private_key_.size());
   ::sodium_memzero(shared_key_.data(),  shared_key_.size());
   // dont need to use asm volatile or smth else for DSE
   // because sodium did this already
}

KeysWrapper& KeysWrapper::operator=(KeysWrapper&& other) noexcept {
   if (private_key_.data() != other.private_key_.data()) {
      private_key_ = std::move(other.private_key_);
      shared_key_  = std::move(other.shared_key_);
   }
   return *this;
}

// compute public from private
std::array<uint8_t, crypto_scalarmult_BYTES> KeysWrapper::GetPublicKey() const noexcept {
   std::array<uint8_t, crypto_scalarmult_BYTES> public_key;
   ::crypto_scalarmult_base(public_key.data(), private_key_.data());

   return public_key;
}

// compute shared key from public key of other side
[[nodiscard]] bool KeysWrapper::ComputeSharedKey(
   const std::array<uint8_t, crypto_scalarmult_BYTES>& other_public_key) noexcept
{
   std::array<uint8_t, crypto_scalarmult_BYTES> tmp;
   // compute and check key valid
   if (::crypto_scalarmult(tmp.data(), private_key_.data(), other_public_key.data()))
      return false;
   // hash shared
   ::crypto_generichash(shared_key_.data(), shared_key_.size(),
                        tmp.data(),         tmp.size(),
                        nullptr,            0);
   // erase tmp for safety
   ::sodium_memzero(tmp.data(), tmp.size());

   return true;
}

std::string KeysWrapper::GetKeyAsString(const std::array<uint8_t, crypto_scalarmult_BYTES>& key) const {
   std::string result;
   result.reserve(crypto_scalarmult_BYTES);
   for (const auto& i : key)
      result += std::format("{:02x}", i);
   
   return result;
}

} // namespace remc::net

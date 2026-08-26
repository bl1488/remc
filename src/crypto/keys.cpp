#include "crypto/keys.h"
#include "include/remc-utils.h"

#include <sodium/crypto_scalarmult.h>
#include <sodium/utils.h>

//
// SessionsKeys
//
remc::crypto::SessionsKeys::~SessionsKeys() noexcept {
   ::sodium_memzero(private_key_.data(), private_key_.size());
   ::sodium_memzero(shared_key_.data(),  shared_key_.size());
   // dont need to use asm volatile or smth else for DSE
   // because sodium did this already
}

remc::crypto::SessionsKeys& 
remc::crypto::SessionsKeys::operator=(
   SessionsKeys&& other) noexcept 
{
   if (private_key_.data() != other.private_key_.data()) {
      private_key_ = std::move(other.private_key_);
      shared_key_  = std::move(other.shared_key_);
   }
   return *this;
}

// compute public from private
std::array<std::byte, crypto_scalarmult_BYTES> 
remc::crypto::SessionsKeys::GetPublicKey() const noexcept {
   std::array<std::byte, crypto_scalarmult_BYTES> public_key;
   ::crypto_scalarmult_base(
      reinterpret_cast<uint8_t*>(public_key.data()), 
      reinterpret_cast<const uint8_t*>(private_key_.data())
   );
   return public_key;
}

// compute shared key from public key of other side
[[nodiscard]] bool 
remc::crypto::SessionsKeys::ComputeSharedKey(
   std::span<const std::byte> other_public_key) noexcept 
{
   assert(other_public_key.size() == crypto_scalarmult_BYTES);

   std::array<std::byte, crypto_scalarmult_BYTES> tmp;
   // compute and check key valid
   if (::crypto_scalarmult(reinterpret_cast<uint8_t*>(tmp.data()), 
                           reinterpret_cast<uint8_t*>(private_key_.data()), 
                           reinterpret_cast<const uint8_t*>(other_public_key.data()))) 
   {
      return false;
   }
   // hash shared
   ::crypto_generichash(
      reinterpret_cast<uint8_t*>(shared_key_.data()), shared_key_.size(),
      reinterpret_cast<uint8_t*>(tmp.data()),         tmp.size(),
      nullptr,                                        0
   );
   ::sodium_memzero(tmp.data(), tmp.size());

   return true;
}

std::string 
remc::crypto::SessionsKeys::GetKeyAsHexString(
   std::span<const std::byte> key) 
{
   return key.size() == crypto_scalarmult_BYTES ? 
      utils::BufferToHexString(key) : std::string{};
}

std::string_view 
remc::crypto::SessionsKeys::GetKeyAsString(
   const std::array<std::byte, crypto_scalarmult_BYTES>& key) const 
{
   return std::string_view{ 
      reinterpret_cast<const char*>(key.data()), key.size() };
}

#include "sk.h"
#include "include/remc_common.h"
#include "include/remc_spdlog.h"

#include <exception>
#include <fstream>

#include <nlohmann/json.hpp>
#include <sodium.h>
#include <sodium/crypto_sign.h>
#include <sodium/utils.h>

std::string remc::crypto::details::BufferToHexString(const void* buffer, std::size_t n) {
   assert(buffer);

   std::string str;
   str.resize(n * 2 + 1);
   ::sodium_bin2hex(str.data(), str.size(), 
                    static_cast<const std::uint8_t*>(buffer), n);
   str.resize(n * 2);

   return str;
}

std::vector<std::uint8_t> remc::crypto::details::HexStringToBuffer(const std::string& hex) {
   std::size_t size = hex.size() / 2;
   std::vector<std::uint8_t> buffer(size);

   int res = ::sodium_hex2bin(
      buffer.data(), buffer.size(), 
      hex.data(),    hex.size(), 
      nullptr, 
      &size, 
      nullptr
   );
   assert(res == 0 && "invalid hex format");

   return buffer;
}

bool remc::crypto::details::CreateKeysFile(std::size_t n) {
   nlohmann::json json_client, json_server;

   std::ofstream file_server("private-keys.json"), 
                 file_client("public-keys.json");
   if (!file_server || !file_client) {
      GlobalLogError("file opening error: {}", remc::GetLastSystemError());
      return false;
   }

   if (auto pairs = CreateKeyPairs(n); pairs.has_value()) {
      std::size_t counter = 1;
      for (const auto& [pk, sk] : *pairs) {
         try {
            json_client.push_back([counter, &pk]() -> nlohmann::json {
               nlohmann::json tmp;
               tmp["index"] = counter;
               tmp["key"]   = BufferToHexString(pk.data(), pk.size());
               return tmp;
            }());
            json_server.push_back([counter, &sk]() -> nlohmann::json {
               nlohmann::json tmp;
               tmp["index"] = counter;
               tmp["key"]   = BufferToHexString(sk.data(), sk.size());
               return tmp;
            }());
         }
         catch (const std::exception& ex) {
            GlobalLogError("json expection: {}", ex.what());
            return false;
         }
         ++counter;
      }
   }
   else {
      GlobalLogError("key pairs creation failed");
      return false;
   }
   file_server << json_server.dump(4);
   file_client << json_client.dump(4);

   return true;
}

namespace remc::crypto::global {

#if defined(__GNUC__) || defined(__clang__)
   __attribute__((section(".keys_data")))
#elif defined(_MSC_VER)
#  pragma section(".keys_data", read, write)
   __declspec(allocate(".keys_data")) 
#endif

// global keys data table
static volatile details::KeysDataSection KEYS_DATA;

} // namespace remc::crypto::global

std::vector<std::uint8_t> remc::crypto::details::GetKeyByIndex(std::uint32_t index) {
   if (index > global::KEYS_DATA.keys_number)
      return {};
   auto* ptr = global::KEYS_DATA.keys[index].key;
   return std::vector<std::uint8_t>(ptr, ptr + crypto_sign_PUBLICKEYBYTES);
}

using remc::crypto::details::KeyPairType;

[[nodiscard]] std::optional<std::vector<KeyPairType>>
remc::crypto::details::CreateKeyPairs(std::size_t n) {
   std::vector<std::pair<std::vector<std::uint8_t>, std::vector<std::uint8_t>>>
      result;
   result.reserve(n);
   // buffers for public/private key
   std::uint8_t pk[crypto_sign_PUBLICKEYBYTES]{},
                sk[crypto_sign_SECRETKEYBYTES]{};

   for (std::size_t i = 0; i < n; ++i) {
      if (::crypto_sign_keypair(pk, sk)) {
         GlobalLogError("key pair creation failed at {}", i);
         return std::nullopt;
      }
      result.emplace_back(
         std::vector<std::uint8_t>(pk, pk + crypto_sign_PUBLICKEYBYTES), 
         std::vector<std::uint8_t>(sk, sk + crypto_sign_SECRETKEYBYTES));
   }
   ::sodium_memzero(sk, sizeof(sk));
   
   return result;
}

bool remc::crypto::details::PatchBinary(const char* file_name, const std::string& json) {
   std::fstream file(file_name, 
      std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
   if (!file) {
      GlobalLogError("file opening error: {}", remc::GetLastSystemError());
      return false;
   }

   std::size_t file_size = file.tellg();
   std::string file_data;
   file_data.resize(std::size_t(file_size + 1));

   file.seekp(std::ios::beg);
   if (!file.read(file_data.data(), file_size)) {
      GlobalLogError("file reading error: {}", remc::GetLastSystemError());
      return false;
   }

   // find signature
   std::size_t keys_data_pos = file_data.find(
      "\xFF\xFA\xAF\x01\x02\x03\xCC\x1A\x7B\xCC\xFF\x12", 13);
   if (keys_data_pos == std::string::npos) {
      GlobalLogError("file signature not found");
      return false;
   }
   auto keys_data = reinterpret_cast<details::KeysDataSection*>(file_data.data() + keys_data_pos);

   if (keys_data->keys_number != global::KEYS_NUMBER) {
      GlobalLogError("keys number mismatch {}:{}", 
         keys_data->keys_number, global::KEYS_NUMBER);
      return false;
   }

   file.close();

   return true;
}

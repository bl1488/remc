#include "sk.h"
#include "include/remc_common.h"
#include "include/remc_spdlog.h"

#include <exception>
#include <fstream>

#include <nlohmann/json.hpp>
#include <sodium.h>
#include <sodium/crypto_sign.h>
#include <sodium/utils.h>

using remc::crypto::details::KeyPairType;

////////////////////////////////////

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

////////////////////////////////////

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

// default name for files:
// - public-keys.json  (client)
// - private-keys.json (server)
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

std::vector<std::uint8_t> remc::crypto::details::GetKeyByIndex(std::uint32_t index) {
   if (index > global::KEYS_DATA.keys_number)
      return {};
   auto* ptr = global::KEYS_DATA.keys[index].key;
   return std::vector<std::uint8_t>(ptr, ptr + crypto_sign_PUBLICKEYBYTES);
}

[[nodiscard]] std::optional<std::vector<KeyPairType>>
remc::crypto::details::CreateKeyPairs(std::size_t n) {
   std::vector<std::pair<std::vector<std::uint8_t>, std::vector<std::uint8_t>>>
      result;
   result.reserve(n);

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

// simply copy the first bytes of the table. 
// if tmp > 0 the table is patched
bool remc::crypto::details::IsKeyTablePatched() noexcept {
   std::size_t tmp;
   std::memcpy(&tmp, (const void*)global::KEYS_DATA.keys, sizeof(tmp));
   return tmp;
}

[[nodiscard]] std::optional<nlohmann::json> 
remc::crypto::details::ReadKeysJsonFile(const std::string& file_name) {
   std::ifstream file(file_name);
   if (!file.is_open()) {
      GlobalLogError(
         "{}: file opening error: {}", __func__, remc::GetLastSystemError());
      return std::nullopt;
   }

   nlohmann::json json;
   try {
      json = nlohmann::json::parse(file);
      // [ ... ]
      if (!json.is_array()) {
         GlobalLogError(
            "{}: json file is not array", __func__);
         return std::nullopt;
      }
      // check json valid.
      // iterate through each element and check fields
      std::size_t index = 1;
      for (const auto& i : json) {
         if (!i.is_object()) {
            GlobalLogError(
               "{}: element at index {} is not a json object", __func__, index);
            return std::nullopt;
         }
         // "index": <digit> && "key": <hex string>
         // example: {
         //   "index": 8,
         //   "key": "bc0840e7c7acbe7d95c2a32cbe2bba073967462250625f6b73ed07f116f8e60f"
         // }
         if ((!i.contains("index") || !i["index"].is_number_unsigned()) ||
             (!i.contains("key")   || !i["key"].is_string())) {
            GlobalLogError(
               "{}: element at index {} has invalid format", __func__, index);
            return std::nullopt;
         }
         ++index;
      }
   }
   catch (const std::exception& ex) {
      GlobalLogError("{}: json exception: {}", __func__, ex.what());
      return std::nullopt;
   }

   return json;
}

bool remc::crypto::details::PatchBinary(
   const std::string& file_name,
   const std::string& json_file_name) 
{
   std::fstream file(file_name, 
      std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
   if (!file.is_open()) {
      GlobalLogError("{}: file opening error: {}", 
         __func__, remc::GetLastSystemError());
      return false;
   }

   std::size_t file_size = file.tellg();
   std::string file_data;
   file_data.resize(file_size);
   // set file position to the beginning (cuz std::ios::ate), 
   // and then read the data
   file.seekg(0, std::ios::beg);
   if (!file.read(file_data.data(), file_size)) {
      GlobalLogError("{}: file reading error: {}", 
         __func__, remc::GetLastSystemError());
      return false;
   }

   // split the string literals because signature string itself resides in the .rdata
   // section, and the find() method detects it before .keys_data section.
   std::string signature = "\xFF\xFA\xAF\x01\x02\xFC";
   signature += "\xCC\x1A\x7B\xCA\xFF\x12";
   // find signature
   std::size_t keys_data_pos = file_data.find(signature);
   if (keys_data_pos == std::string::npos) {
      GlobalLogError("{}: file signature not found", __func__);
      return false;
   }
   auto* keys_data = reinterpret_cast<details::KeysDataSection*>(
      file_data.data() + keys_data_pos);

   // read json file
   nlohmann::json json; {
      auto j = ReadKeysJsonFile(json_file_name);
      if (!j) {
         GlobalLogError("{}: error reading json file", __func__);
         return false;
      }
      json = std::move(j.value());
   }
   // keys_number should be equal to json.size() cuz with diff sizes,
   // we would go beyond the boundaries of the .keys_data section
   if (keys_data->keys_number == json.size()) {
      std::size_t index = 0;
      // convert json to binary
      for (const auto& i : json) {
         try {
            keys_data->keys[index].index = i["index"];
            std::memcpy(keys_data->keys[index].key, 
                        HexStringToBuffer(i["key"]).data(), 
                        crypto_sign_PUBLICKEYBYTES);
         }
         catch (const std::exception& ex) {
            GlobalLogError("{}: json exception: {}", 
               __func__, ex.what());
            return false;
         }
         ++index;
      }
   }
   else {
      GlobalLogError("{}: keys number mismatch {}:{}", 
         __func__, keys_data->keys_number, json.size());
      return false;
   }

   // write to file
   // close prev
   file.close();
   // rewrite with new buffer
   file.open(file_name, std::ios::out | std::ios::trunc | std::ios::binary);
   if (!file.is_open()) {
      GlobalLogError("{}: file reopening error: {}", 
         __func__, remc::GetLastSystemError());
      return false;
   }
   file << file_data;

   return true;
}

remc::crypto::details::KeysDataSection* 
remc::crypto::details::GetKeysDataSection() noexcept {
   return reinterpret_cast<KeysDataSection*>((void*)&global::KEYS_DATA);
}

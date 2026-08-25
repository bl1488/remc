#ifndef REMC_SECURE_KEY_H
#define REMC_SECURE_KEY_H

// =====< REMC secure key impl >=====
//
// Implementation of a system of self-signed TLS certificates
// for client-server validation.

#include <vector>

#include <nlohmann/json.hpp>
#include <sodium.h>

namespace remc::crypto {

namespace global {

// number of keys
constexpr std::uint32_t KEYS_NUMBER = 100;

} // namespace remc::crypto::global

namespace details {

struct KeyDataNode {
   std::uint32_t index;
   std::uint8_t  key[crypto_sign_PUBLICKEYBYTES];
};

// structure that will be located in the .keys_data section
//
// signature:
// FF FA AF 01 02 03 CC 1A 7B CC FF 12
struct KeysDataSection {
   std::uint8_t  signature[12] = { 
      0xFF, 0xFA, 0xAF, 0x01, 0x02, 0x03, 0xCC, 0x1A, 0x7B, 0xCC, 0xFF, 0x12 
   };
   std::uint32_t keys_number = global::KEYS_NUMBER;
   KeyDataNode   keys[global::KEYS_NUMBER]{};
};

// creates files for the server and client sides that are subsequently
// used for a pseudo-TLS handshake.
// args:
// accepts as input N asymmetric pairs to be written to files
bool 
CreateKeysFile(
   std::size_t n = global::KEYS_NUMBER
);

// returns key at specified index in the table from the .keys_data section.
// used for internal access to keys
std::vector<std::uint8_t>
GetKeyByIndex(
   std::uint32_t index
);

std::string 
BufferToHexString(
   const void* buffer, 
   std::size_t n
);

std::vector<std::uint8_t>
HexStringToBuffer(
   const std::string& str
);

using KeyType     = std::vector<std::uint8_t>;
using KeyPairType = std::pair<KeyType, KeyType>;

[[nodiscard]] std::optional<std::vector<KeyPairType>>
CreateKeyPairs(
   std::size_t n = global::KEYS_NUMBER
);

// a function for binary patching an executable file.
// after compilation, we patch the client binary by reading a .json file containing keys 
// and populating a table in the .keys_data section.
bool
PatchBinary(
   const char* file_name,
   const std::string& json
);

} // namespace remc::crypto::details

} // namespace remc::crypto

#endif // REMC_SECURE_KEY_H_

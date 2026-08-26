#ifndef REMC_SECURE_KEY_H
#define REMC_SECURE_KEY_H

// =====< SK (secure-key) system >=====
//
// SK is required for the TLS handshake with the server.
// in the first packet the server will send a packet in the following format:
// { 
//    "index":     <...>,
//    "signature": <...>,
//    "key":       <...>
// }
//
// the client takes the corresponding index from its global key table and calculates the signature.
// if they match, the handshake originated from a valid server.

#include <vector>

#include <nlohmann/json.hpp>
#include <sodium.h>

namespace remc::crypto {

namespace global {

// global var that specifies the number of keys to be
// embedded in the executable file
constexpr std::uint32_t KEYS_NUMBER = 100;

} // namespace remc::crypto::global

namespace details {

////////////////////////////////////

#pragma pack(push, 1) // no padding here

struct KeyDataNode {
   std::uint32_t index;
   std::uint8_t  key[crypto_sign_PUBLICKEYBYTES];
};

// structure that will be located in the .keys_data section
//
// signature:
// FF FA AF 01 02 FC CC 1A 7B CA FF 12
struct KeysDataSection {
   std::uint8_t  signature[12] = { 
      0xFF, 0xFA, 0xAF, 0x01, 0x02, 0xFC, 0xCC, 0x1A, 0x7B, 0xCA, 0xFF, 0x12 
   };
   std::uint32_t keys_number = global::KEYS_NUMBER;
   KeyDataNode   keys[global::KEYS_NUMBER]{};
};

#pragma pack(pop)

////////////////////////////////////

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
   const std::string& hex_string
);

// reads the specified JSON file containing keys and returns an 
// nlohmann::json object if file passes validation
[[nodiscard]] std::optional<nlohmann::json>
ReadKeysJsonFile(
   const std::string& file_name
);

using KeyType     = std::vector<std::uint8_t>;
using KeyPairType = std::pair<KeyType, KeyType>;

// creates N key pairs from two std::vectors
[[nodiscard]] std::optional<std::vector<KeyPairType>>
CreateKeyPairs(
   std::size_t n = global::KEYS_NUMBER
);

// returns a pointer to the global key table
KeysDataSection* GetKeysDataSection() noexcept;

// checks the initialization of the key table.
// if returns false, it means the binary was not patched.
bool IsKeyTablePatched() noexcept;

// a function for binary patching an executable file.
// after compilation, we patch the client binary by reading a .json file containing keys 
// and populating a table in the .keys_data section.
bool
PatchBinary(
   const std::string& file_name,
   const std::string& json_file_name
);

} // namespace remc::crypto::details

} // namespace remc::crypto

#endif // REMC_SECURE_KEY_H_

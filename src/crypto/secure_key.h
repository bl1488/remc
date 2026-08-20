#ifndef REMC_SECURE_KEY_H
#define REMC_SECURE_KEY_H

// =====< REMC secure key impl >=====
//
// Implementation of a system of self-signed TLS certificates
// for client-server validation.

#include <nlohmann/json.hpp>

namespace remc::crypto {

void GenerateAssimetricKeysFile(std::size_t n, const std::string& file_path);

} // namespace remc::crypto

#endif // REMC_SECURE_KEY_H_

#include "secure_key.h"
#include "include/remc_spdlog.h"

#include <fstream>

#include <sodium.h>
#include <nlohmann/json.hpp>
#include <sodium/crypto_sign.h>

namespace remc::crypto {

bool CreateAsymmetricKeyFile(std::size_t n, const std::string &file_path) {
   nlohmann::json json;

   unsigned char pk[crypto_sign_PUBLICKEYBYTES]{};
   unsigned char sk[crypto_sign_SECRETKEYBYTES]{};

   for (std::size_t i = 0; i < n; ++i) {
      if (!::crypto_sign_keypair(pk, sk)) {
         GlobalLogError("key pair generation failed");
         return false;
      }
   }

   return true;
}

} // namespace remc::crypto

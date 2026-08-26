#include <gtest/gtest.h>

bool InitCryptoTests() {
   // init sodium
   if (::sodium_init() < 0) {
      std::cerr << "Sodium lib init failed!\n";
      return false;
   }
   return true;
}

#include <iostream>

#include <gtest/gtest.h>

#ifdef HAS_CRYPTO_TESTS
   extern bool InitCryptoTests();
#endif

#ifdef HAS_NET_TESTS
   extern bool InitNetTests();
#endif

int main(int argc, char **argv) {
   ::testing::InitGoogleTest(&argc, argv);

#ifdef HAS_CRYPTO_TESTS
   if (!InitCryptoTests()) {
      std::cerr << "InitCryptoTests failed" << '\n';
      return 0;
   }
#endif

#ifdef HAS_NET_TESTS
   if (!InitNetTests()) {
      std::cerr << "InitNetTests failed" << '\n';
      return 0;
   }
#endif

   return RUN_ALL_TESTS();
}

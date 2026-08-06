#include "crypto/cc20poly1305.h"
#include "crypto/chacha20.h"
#include "crypto/cc20poly1305.h"
#include "net/keys_wrapper.h"

#include <gtest/gtest.h>
#include <sodium/randombytes.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
   // Init sodium
   if (::sodium_init() < 0) {
      std::cerr << "Sodium lib init failed!\n";
      return 0;
   }
   // init gtest
   ::testing::InitGoogleTest();
   
   return RUN_ALL_TESTS();
}

// Common wrapper
class KeysWrapperFixture : public ::testing::Test {
protected:
   static void SetUpTestSuite() {
      // Generate keys
      (void)server_key.ComputeSharedKey(client_key.GetPublicKey());
      (void)client_key.ComputeSharedKey(server_key.GetPublicKey());
   }

protected:
   static constexpr const char* TEST_TEXT = "hello world 12345";
   static remc::net::KeysWrapper server_key, 
                                 client_key;
};

remc::net::KeysWrapper KeysWrapperFixture::server_key;
remc::net::KeysWrapper KeysWrapperFixture::client_key;

TEST_F(KeysWrapperFixture, ChaCha20_BasicCheck) {
   remc::crypto::ChaCha20 cipher((void*)server_key.GetSharedKey().data(), server_key.GetSharedKey().size());

   std::string a, b;
   a.resize(std::strlen(TEST_TEXT));
   b.resize(std::strlen(TEST_TEXT));   

   // Crypt src string
   cipher.Crypt(TEST_TEXT, a);
   // Reset state for decrpyt
   cipher.SetKey((void*)client_key.GetSharedKey().data(), client_key.GetSharedKey().size());
   // Decrypt crypted string
   cipher.Crypt(a, b);

   EXPECT_EQ(a.size(), b.size());
   EXPECT_STREQ(TEST_TEXT, b.c_str());
}

TEST_F(KeysWrapperFixture, FSCC20Poly1305_BasicCheck) {
   // AAD for example
   int AAD = std::strlen(TEST_TEXT);
   std::span<const std::byte> AAD_span = { reinterpret_cast<const std::byte*>(&AAD), sizeof(AAD) };

   // Server side
   remc::crypto::FSChaCha20Poly1305 poly_server((void*)server_key.GetSharedKey().data(), 
                                                       server_key.GetSharedKey().size());
   std::string to_client;
   to_client.resize(std::strlen(TEST_TEXT) + remc::crypto::FSChaCha20Poly1305::EXPANSION);

   poly_server.Encrypt(TEST_TEXT, AAD_span, to_client);
   // sending...

   // Client side
   remc::crypto::FSChaCha20Poly1305 poly_client((void*)client_key.GetSharedKey().data(), 
                                                       client_key.GetSharedKey().size());
   // reading...
   std::string from_server;
   // Get 'message_length' field from packet
   from_server.resize(to_client.size() - remc::crypto::FSChaCha20Poly1305::EXPANSION);

   EXPECT_EQ(poly_client.Decrypt(to_client, AAD_span, from_server), true);
   // double check
   EXPECT_STREQ(TEST_TEXT, from_server.c_str());
}

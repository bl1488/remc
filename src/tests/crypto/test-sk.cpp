#include "crypto/sk/sk.h"

#include <bit>
#include <gtest/gtest.h>

TEST(SK, SK_BasicTest) {
   EXPECT_TRUE(remc::crypto::CreateKeysFile());

   nlohmann::json pk_json, sk_json; {
      auto pk = remc::crypto::ReadKeysJsonFile("public-keys.json");
      EXPECT_TRUE(pk.has_value());
      auto sk = remc::crypto::ReadKeysJsonFile("private-keys.json");
      EXPECT_TRUE(sk.has_value());

      pk_json = std::move(pk.value());
      sk_json = std::move(sk.value());
   }

   EXPECT_EQ(pk_json.size(), sk_json.size());
   EXPECT_EQ(pk_json.size(), remc::crypto::global::KEYS_NUMBER);

   EXPECT_TRUE(remc::crypto::IsKeyTablePatched());

   auto* keys_data = remc::crypto::GetGlobalKeysDataTable();

   bool flag = false;
   for (std::uint32_t i = 0; i < keys_data->keys_number; ++i) {
      if (keys_data->keys[i].index != i + 1 || std::memcmp(
            "\x00\x00\x00\x00\x00\x00\x00\x00", keys_data->keys[i].key, 8) == 0) {
         flag = true;
         break;
      }
   }
   EXPECT_FALSE(flag);

   std::remove("public-keys.json");
   std::remove("private-keys.json");
}

TEST(SK, SK_AdditionalChecks) {
   auto* keys_data = remc::crypto::GetGlobalKeysDataTable();

   EXPECT_EQ(keys_data->keys_number, remc::crypto::global::KEYS_NUMBER);

   std::string signature = "\xFF\xFA\xAF\x01\x02\xFC";
   signature += "\xCC\x1A\x7B\xCA\xFF\x12";
   EXPECT_FALSE(std::memcmp(keys_data->signature, 
                            signature.data(), 
                            sizeof(keys_data->signature)));

   auto vec = remc::crypto::HexStringToBuffer("010203");
   EXPECT_EQ(vec.size(), 3);

   std::uint32_t tmp = 0;
   std::memcpy(&tmp, vec.data(), 3);
   if (std::endian::native == std::endian::little)
        EXPECT_EQ(tmp, 0x030201);
   else EXPECT_EQ(tmp, 0x010203);

   auto str = remc::crypto::BufferToHexString(&tmp, sizeof(tmp));
   EXPECT_EQ(str, "01020300");
}

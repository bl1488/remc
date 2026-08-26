#include "sk.h"
#include "include/remc-common.h"

#include <iostream>
#include <format>

// a small patcher that runs after the client is compiled to populate its key section. 
// use --create once just to generate the keys.
//
// !!! if you generate a new file, dont forget to patch the client. otherwise, your handshake will fail !!!

static void Usage() {
   std::cout << 
      "usage:\n"
      "\tbin-patcher <binary-file> <json-file>\n"
      "\tbin-patcher --create\n";
}

int main(int argc, char** argv) {
   if (argc == 2) {
      std::string arg = argv[1];
      if (arg != "--create") {
         std::cerr << std::format("unknown flag {}\n", arg);
         Usage();
         return 0;
      }
      else if (!remc::crypto::CreateKeysFile()) {
         std::cerr << std::format("file creation failed (sys-err:{})", 
            remc::GetLastSystemError());
         return 0;
      }
      std::cout << "files successfully created!\n";
   }
   else if (argc == 3) {
      if (!remc::crypto::PatchBinary(argv[1], argv[2])) {
         std::cerr << std::format("file patching failed (sys-err:{})", 
            remc::GetLastSystemError());
         return 0;
      }
      std::cout << std::format("file {} successfull patched!\n", argv[1]);
   }
   else {
      std::cerr << "invalid arguments!\n";
      Usage();
   }

   return 0;
}

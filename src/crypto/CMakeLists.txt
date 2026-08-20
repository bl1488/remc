# module
add_library(ModuleCrypto STATIC EXCLUDE_FROM_ALL
   chacha20.cpp
   poly1305.cpp
   cc20poly1305.cpp
   keys_wrapper.cpp
   secure_key.cpp)

target_include_directories(ModuleCrypto PUBLIC 
   ${CMAKE_SOURCE_DIR}/src
   ${CMAKE_SOURCE_DIR}/libs
   ${CMAKE_SOURCE_DIR}/libs/spdlog)

# libs
find_package(PkgConfig   REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium IMPORTED_TARGET)

target_link_libraries(ModuleCrypto     PRIVATE 
   PkgConfig::SODIUM)

target_precompile_headers(ModuleCrypto PUBLIC 
   <sodium.h>)
   
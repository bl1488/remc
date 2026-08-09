# module
add_library(ModuleCrypto STATIC EXCLUDE_FROM_ALL
   chacha20.cpp
   poly1305.cpp
   cc20poly1305.cpp
   keys_wrapper.cpp)

target_include_directories(ModuleCrypto PUBLIC 
   ${CMAKE_SOURCE_DIR}/src)

# libs
find_package(PkgConfig   REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium IMPORTED_TARGET)

target_link_libraries(ModuleCrypto     PRIVATE 
   PkgConfig::SODIUM)

target_precompile_headers(ModuleCrypto PUBLIC 
   <sodium.h>)
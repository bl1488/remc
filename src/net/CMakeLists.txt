if (NOT BUILD_CRYPTO_MODULE)
   message(FATAL_ERROR "CryptoModule is required to build NetModule")
endif()

# module
add_library(ModuleNet STATIC EXCLUDE_FROM_ALL 
   session_base.cpp 
   server.cpp
   client.cpp)

target_include_directories(ModuleNet PUBLIC 
   ${CMAKE_SOURCE_DIR}/src
   ${CMAKE_SOURCE_DIR}/libs/
   ${CMAKE_SOURCE_DIR}/libs/spdlog 
   ${CMAKE_SOURCE_DIR}/libs/asio)

# libs
find_package(absl      REQUIRED CONFIG)
find_package(PkgConfig REQUIRED)

target_compile_definitions(ModuleNet PUBLIC ASIO_STANDALONE)

target_link_libraries(ModuleNet      PUBLIC 
   ModuleCrypto 
   absl::flat_hash_map)
# windows
if (WIN32)
   target_link_libraries(ModuleNet PUBLIC ws2_32 mswsock)
endif()

target_precompile_headers(ModuleNet  PUBLIC 
   <asio.hpp> 
   <spdlog/spdlog.h>)
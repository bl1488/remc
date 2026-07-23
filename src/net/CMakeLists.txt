cmake_minimum_required(VERSION 3.20)

# module
add_library(ModuleNet STATIC EXCLUDE_FROM_ALL 
   session_base.cpp keys_wrapper.cpp)

if (BUILD_SERVER)
   target_sources(ModuleNet PUBLIC server.cpp)
endif()

if (BUILD_CLIENT)
   target_sources(ModuleNet PUBLIC client.cpp)
endif()

#====================
#    MODULE DEFS
#====================
target_compile_definitions(ModuleNet PUBLIC ASIO_STANDALONE)

target_include_directories(ModuleNet PUBLIC 
   ${CMAKE_SOURCE_DIR}/src 
   ${CMAKE_SOURCE_DIR}/libs/spdlog 
   ${CMAKE_SOURCE_DIR}/libs/asio)

target_link_libraries(ModuleNet PRIVATE absl::flat_hash_map PkgConfig::SODIUM)
# Some optimization
target_precompile_headers(ModuleNet PUBLIC 
   <asio.hpp> <sodium.h> <spdlog/spdlog.h>)

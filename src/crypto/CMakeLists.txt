cmake_minimum_required(VERSION 3.20)

# module
add_library(ModuleCrypto STATIC EXCLUDE_FROM_ALL
   ${CMAKE_SOURCE_DIR}/src/include/remc_utils.h
   chacha20.cpp
   poly1305.cpp
   cc20poly1305.cpp)

target_include_directories(ModuleCrypto PUBLIC ${CMAKE_SOURCE_DIR}/src)

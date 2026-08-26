cmake_minimum_required(VERSION 3.20)

project(remc VERSION 1.0)

# ====================
#       OPTIONS
# ====================
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD            23)
set(CMAKE_CXX_STANDARD_REQUIRED   ON)

set(REMC_BUILD_TYPE "DEBUG" 
   CACHE STRING "Build type: DEBUG, RELEASE, SAN, TSAN")

option(BUILD_CRYPTO_MODULE "Build remc-crypto module." ON)
option(BUILD_GUI_MODULE    "Build remc-gui module."    OFF)
option(BUILD_NET_MODULE    "Build remc-net module."    ON)
option(BUILD_TEST_MODULE   "Build test module."        OFF)
option(BUILD_SERVER        "Build server project."     OFF)
option(BUILD_CLIENT        "Build client project."     OFF)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
   # fix libstdc++
   add_compile_definitions(__cpp_concepts=202002L)
   add_compile_options(
      -Wall
      -Wextra
      -Wshadow
      -Wpedantic
      -Wdouble-promotion
      -Wunused-parameter
      -Wno-unused-private-field
      -fomit-frame-pointer)
   # build types
   # release
   if     (REMC_BUILD_TYPE STREQUAL "RELEASE")
      add_compile_options(
         -O2
         -flto=auto
         -march=native)
      add_link_options(-flto=auto -fuse-ld=lld)
   # sanitizers build
   # using: ASan, UBSan, UAFSan
   elseif (REMC_BUILD_TYPE STREQUAL "SAN")
      add_compile_options(
         -O1
         -g
         -fsanitize=address,undefined
         -fsanitize-address-use-after-return=always
         -D_GLIBCXX_DEBUG)
      add_link_options(-fsanitize=address,undefined)
   # sanitizer build for threads (TSan)
   elseif (REMC_BUILD_TYPE STREQUAL "TSAN")
      add_compile_options(
         -O1
         -g
         -fsanitize=thread
         -D_GLIBCXX_DEBUG)
      add_link_options(-fsanitize=thread)
   # debug
   else()
      add_compile_options(-O0 -g)
   endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
   # TODO: MSVC options
   # ...
endif()

# ====================
#       MODULES
# ====================
# crypto
if (BUILD_CRYPTO_MODULE)
   add_subdirectory(src/crypto)
endif()
# net
if (BUILD_NET_MODULE)
   add_subdirectory(src/net)
endif()
# tests
if (BUILD_TEST_MODULE)
   add_subdirectory(src/tests)
endif()
# gui
if (BUILD_GUI_MODULE)
   add_subdirectory(src/gui)
endif()

message("=======================================")
message("REMC_BUILD_TYPE...................${REMC_BUILD_TYPE}")
message("BUILD_CRYPTO_MODULE...............${BUILD_CRYPTO_MODULE}")
message("BUILD_GUI_MODULE..................${BUILD_GUI_MODULE}")
message("BUILD_NET_MODULE..................${BUILD_NET_MODULE}")
message("BUILD_TEST_MODULE.................${BUILD_TEST_MODULE}")
message("BUILD_SERVER......................${BUILD_SERVER}")
message("BUILD_CLIENT......................${BUILD_CLIENT}")
message("=======================================")

# ====================
#     EXECUTABLES
# ====================
if (BUILD_SERVER)
   add_executable(remc-server 
      src/server-main.cpp)
   target_link_libraries(remc-server PRIVATE 
      ModuleNet)
endif()

if (BUILD_CLIENT)
   add_executable(remc-client
      src/client-main.cpp)
   target_link_libraries(remc-client PRIVATE
      ModuleNet)
endif()

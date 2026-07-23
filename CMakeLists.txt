cmake_minimum_required(VERSION 3.20)

project(remc VERSION 1.0)

#====================
#      OPTIONS
#====================
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD            23)
set(CMAKE_CXX_STANDARD_REQUIRED   ON)

set(REMC_BUILD_TYPE "ASAN_UBSAN" CACHE STRING "Build type: DEBUG, RELEASE, VALGRIND, ASAN_UBSAN, TSAN")

option(BUILD_CRYPTO_MODULE "Build remc-crypto module." ON)
option(BUILD_GUI_MODULE    "Build remc-gui module."    OFF)
option(BUILD_NET_MODULE    "Build remc-net module."    ON)
option(BUILD_TEST_MODULE   "Build test module."        OFF)
option(BUILD_SERVER        "Build server project."     ON)
option(BUILD_CLIENT        "Build client project."     ON)

#====================
#    CC OPTIONS
#====================
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
   # Default options
   add_compile_options(-Wall -Wextra -Werror -Wshadow -Wno-unused-private-field)
   # Build type
   if     (REMC_BUILD_TYPE STREQUAL "RELEASE")
      add_compile_options(-O3)
   elseif (REMC_BUILD_TYPE STREQUAL "VALGRIND")
      add_compile_options(-O2 -g)
   elseif (REMC_BUILD_TYPE STREQUAL "ASAN_UBSAN")
      add_compile_options(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -D_GLIBCXX_DEBUG)
      add_link_options(-fsanitize=address,undefined)
   elseif (REMC_BUILD_TYPE STREQUAL "TSAN")
      add_compile_options(-O1 -g -fsanitize=thread -fno-omit-frame-pointer)
      add_link_options(-fsanitize=thread)
   else()
      # Debug
      message(WARNING "Wrong REMC_BUILD_TYPE='${REMC_BUILD_TYPE}'! Switching to Debug mode...")
      add_compile_options(-O0 -g)
   endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
# TODO: MSVC options
# ...
endif()

#====================
#        LIBS
#====================
find_package(absl        REQUIRED CONFIG)
find_package(PkgConfig   REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium IMPORTED_TARGET)

if (BUILD_CRYPTO_MODULE)
   add_subdirectory(src/crypto)
endif()

if (BUILD_NET_MODULE)
   add_subdirectory(src/net)
endif()

if (BUILD_TEST_MODULE)
   enable_testing()
   add_subdirectory(src/tests)
endif()

if (BUILD_GUI_MODULE)
# TODO: gui
# ...
endif()

# debug print
message("====================================================")
message("REMC_BUILD_TYPE...................${REMC_BUILD_TYPE}")
message("BUILD_CRYPTO_MODULE...............${BUILD_CRYPTO_MODULE}")
message("BUILD_GUI_MODULE..................${BUILD_GUI_MODULE}")
message("BUILD_NET_MODULE..................${BUILD_NET_MODULE}")
message("BUILD_TEST_MODULE.................${BUILD_TEST_MODULE}")
message("BUILD_SERVER......................${BUILD_SERVER}")
message("BUILD_CLIENT......................${BUILD_CLIENT}")
message("====================================================")

#====================
#    EXECUTABLES
#====================
if (NOT BUILD_SERVER AND NOT BUILD_CLIENT AND NOT BUILD_TEST_MODULE)
   message(FATAL_ERROR "At least one of the BUILD_SERVER or BUILD_CLIENT parameters must be enabled. 
      Alternatively, build using BUILD_TEST_MODULE. ")
endif()

if (BUILD_SERVER)
   add_executable(remc-server src/server_main.cpp)
   target_link_libraries(remc-server PRIVATE ModuleCrypto ModuleNet)
endif()

if (BUILD_CLIENT)
   add_executable(remc-client src/client_main.cpp)
endif()

include(FetchContent)

# Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
  googletest 
  URL https://github.com/google/googletest/archive/v1.14.0.zip)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# crypto
if (BUILD_CRYPTO_MODULE)
  add_executable(test-crypto 
    crypto_test.cpp ${CMAKE_SOURCE_DIR}/src/net/keys_wrapper.cpp)  
  target_link_libraries(test-crypto PRIVATE gtest PkgConfig::SODIUM ModuleCrypto)

  gtest_discover_tests(test-crypto)
endif()

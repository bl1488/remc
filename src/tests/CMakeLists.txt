enable_testing()

include(GoogleTest)
include(FetchContent)
# windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
  googletest 
  URL https://github.com/google/googletest/archive/v1.14.0.zip)
FetchContent_MakeAvailable(googletest)

set(MAIN_TEST_NAME main-test)

# main test executable
add_executable(${MAIN_TEST_NAME} 
   main.cpp)
target_link_libraries(${MAIN_TEST_NAME} PRIVATE 
   gtest)
target_include_directories(${MAIN_TEST_NAME} PRIVATE 
   ${CMAKE_SOURCE_DIR}/src)

# crypto
if (BUILD_CRYPTO_MODULE)
   target_sources(${MAIN_TEST_NAME} PRIVATE 
      crypto/test-main-crypto.cpp
      crypto/test-sk.cpp
      crypto/test-keys.cpp)
   target_link_libraries(${MAIN_TEST_NAME} PRIVATE 
      ModuleCrypto)
   target_compile_definitions(${MAIN_TEST_NAME} PRIVATE HAS_CRYPTO_TESTS)
endif()
# net
if (BUILD_NET_MODULE)
   target_sources(${MAIN_TEST_NAME} PRIVATE 
      net/test-main-net.cpp)
   target_link_libraries(${MAIN_TEST_NAME} PRIVATE 
      ModuleNet)
   target_compile_definitions(${MAIN_TEST_NAME} PRIVATE HAS_NET_TESTS)
endif()

gtest_discover_tests(${MAIN_TEST_NAME})

# gui
if (BUILD_GUI_MODULE)
   add_executable(gui-main 
      gui/test-main-gui.cpp)
   target_link_libraries(gui-main PRIVATE
      GuiModule)
endif()

enable_testing()

include(GoogleTest)
include(FetchContent)
# windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
  googletest 
  URL https://github.com/google/googletest/archive/v1.14.0.zip)
FetchContent_MakeAvailable(googletest)

# main test executable
add_executable(tests-main 
   main.cpp)
target_link_libraries(tests-main         PRIVATE 
   gtest)
target_include_directories(tests-main    PRIVATE 
   ${CMAKE_SOURCE_DIR}/src)

# crypto
if (BUILD_CRYPTO_MODULE)
   target_sources(tests-main             PRIVATE 
      crypto/crypto_test_main.cpp)
   target_link_libraries(tests-main      PRIVATE 
      ModuleCrypto)
   target_compile_definitions(tests-main PRIVATE HAS_CRYPTO_TESTS)
endif()
# net
if (BUILD_NET_MODULE)
   target_sources(tests-main             PRIVATE 
      net/net_test_main.cpp)
   target_link_libraries(tests-main      PRIVATE 
      ModuleNet)
   target_compile_definitions(tests-main PRIVATE HAS_NET_TESTS)
endif()

gtest_discover_tests(tests-main)

# gui
if (BUILD_GUI_MODULE)
   add_executable(gui-main 
      gui/gui_test.cpp)
   target_link_libraries(gui-main        PRIVATE
      GuiModule)
endif()

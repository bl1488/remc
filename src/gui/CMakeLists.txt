set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# module
add_library(GuiModule STATIC EXCLUDE_FROM_ALL
   gui-main.cpp
   details.cpp
   menu-sessions.cpp
   menu-help.cpp
   menu-home.cpp)
target_link_libraries(GuiModule PUBLIC
   Qt6::Widgets)

target_include_directories(GuiModule PUBLIC 
   ${CMAKE_SOURCE_DIR}/src
   ${CMAKE_SOURCE_DIR}/libs/
   ${CMAKE_SOURCE_DIR}/libs/spdlog)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# module
add_library(GuiModule STATIC EXCLUDE_FROM_ALL
   details.cpp
   gui_main.cpp
   menu_sessions.cpp
   menu_help.cpp
   menu_home.cpp)
target_link_libraries(GuiModule      PUBLIC
   Qt6::Widgets)

target_include_directories(GuiModule PUBLIC 
   ${CMAKE_SOURCE_DIR}/src
   ${CMAKE_SOURCE_DIR}/libs/
   ${CMAKE_SOURCE_DIR}/libs/spdlog)

project(Keybow_plugin_repl CXX)
cmake_minimum_required(VERSION 3.13)

add_library(repl SHARED main.cpp main.hpp)
target_include_directories(repl PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(repl PROPERTIES CXX_STANDARD 20 PREFIX "")

target_link_libraries(repl sol2 liblua readline)

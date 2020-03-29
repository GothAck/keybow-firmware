project(Keybow_plugin_bluetooth_hid CXX)
cmake_minimum_required(VERSION 3.13)

add_subdirectory(vendor/sdbus-cpp)


add_custom_command(
  OUTPUT sdp_record.xml.h
  COMMAND xxd -i sdp_record.xml > ${CMAKE_CURRENT_BINARY_DIR}/sdp_record.xml.h
  COMMAND touch -r sdp_record.xml ${CMAKE_CURRENT_BINARY_DIR}/sdp_record.xml.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS sdp_record.xml
)


add_library(bthid SHARED main.cpp main.hpp sdp_record.xml.h)
target_include_directories(bthid PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(bthid PROPERTIES CXX_STANDARD 20 PREFIX "")

target_link_libraries(bthid sdbus-c++-objlib systemd sol2 liblua )

project(Keybow CXX)
cmake_minimum_required(VERSION 3.13)

add_subdirectory(vendor/sol2)
add_subdirectory(vendor/plog)

add_library(libkeybow STATIC
  lua.cpp
  lua.hpp
  usb.cpp
  usb.hpp
  lights.cpp
  lights.hpp
  keys.cpp
  keys.hpp
  luakeybow.cpp
  luakeybow.hpp
  )

add_executable(keybow
  main.cpp
  )


set_property(TARGET keybow PROPERTY CXX_STANDARD 20)
set_property(TARGET libkeybow PROPERTY CXX_STANDARD 20)

target_include_directories(libkeybow
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor
  ${CMAKE_CURRENT_SOURCE_DIR}/../lua-5.3.5/src
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/libusbgx/include
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/bcm2835/src
)

target_link_directories(libkeybow
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../lua-5.3.5/src
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/libusbgx/src/.libs
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/bcm2835/src
)

target_link_libraries(keybow -static libkeybow)
target_link_libraries(libkeybow -static sol2 plog lua usbgx config docopt pthread png z bcm2835 dl stdc++fs)

target_compile_definitions(libkeybow PRIVATE -Dcimg_display=0 -Dcimg_use_png)

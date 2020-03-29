project(Keybow CXX)
cmake_minimum_required(VERSION 3.13)


add_subdirectory(vendor/sol2)
add_subdirectory(vendor/plog)


# Interface library for lua
add_library(liblua INTERFACE)
target_include_directories(liblua INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../lua-5.3.5/src)
target_link_directories(liblua INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../lua-5.3.5/src)
target_link_libraries(liblua INTERFACE lua)

# Interface library for libusbgx
add_library(libusbgx INTERFACE)
target_include_directories(libusbgx INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/libusbgx/include)
target_link_directories(libusbgx INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/libusbgx/src/.libs)
target_link_libraries(libusbgx INTERFACE usbgx)

# Interface library for bcm2835
add_library(libbcm2835 INTERFACE)
target_include_directories(libbcm2835 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/bcm2835/src)
target_link_directories(libbcm2835 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/bcm2835/src)
target_link_libraries(libbcm2835 INTERFACE bcm2835)

# Our library
add_library(libkeybow OBJECT
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
  lua_impl.hpp)
set_property(TARGET libkeybow PROPERTY CXX_STANDARD 20)
target_link_libraries(libkeybow -Wl,-Bstatic sol2 plog liblua libusbgx libbcm2835 docopt png config z -Wl,-Bdynamic pthread stdc++fs dl)
target_include_directories(libkeybow PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor)
target_compile_definitions(libkeybow PRIVATE -Dcimg_display=0 -Dcimg_use_png)

# Our executable
add_executable(keybow
  main.cpp)
set_property(TARGET keybow PROPERTY CXX_STANDARD 20)
target_link_libraries(keybow libkeybow)

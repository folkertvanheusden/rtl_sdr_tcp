project(rtl_tcp_client)
cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(
  rtl_tcp_client
  error.cpp
  main.cpp
  net.cpp
)

set(CMAKE_BUILD_TYPE Debug)

include(FindPkgConfig)

pkg_check_modules(SAMPLERATE REQUIRED samplerate)
target_link_libraries(rtl_tcp_client ${SAMPLERATE_LIBRARIES})
target_include_directories(rtl_tcp_client PUBLIC ${SAMPLERATE_INCLUDE_DIRS})
target_compile_options(rtl_tcp_client PUBLIC ${SAMPLERATE_CFLAGS_OTHER})

pkg_check_modules(SNDFILE REQUIRED sndfile)
target_link_libraries(rtl_tcp_client ${SNDFILE_LIBRARIES})
target_include_directories(rtl_tcp_client PUBLIC ${SNDFILE_INCLUDE_DIRS})
target_compile_options(rtl_tcp_client PUBLIC ${SNDFILE_CFLAGS_OTHER})

project(rtl_sdr_to_sip)
cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(
  rtl_sdr_to_sip
  error.cpp
  main.cpp
  net.cpp
  rtl_sdr_tcp.cpp
)

set(CMAKE_BUILD_TYPE RelWithDebInfo)
#set(CMAKE_BUILD_TYPE Debug)

#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

add_compile_options(-Wall -pedantic)

include(FindPkgConfig)

pkg_check_modules(SAMPLERATE REQUIRED samplerate)
target_link_libraries(rtl_sdr_to_sip ${SAMPLERATE_LIBRARIES})
target_include_directories(rtl_sdr_to_sip PUBLIC ${SAMPLERATE_INCLUDE_DIRS})
target_compile_options(rtl_sdr_to_sip PUBLIC ${SAMPLERATE_CFLAGS_OTHER})

pkg_check_modules(HAPPY REQUIRED libhappy)
target_link_libraries(rtl_sdr_to_sip ${HAPPY_LIBRARIES})
target_include_directories(rtl_sdr_to_sip PUBLIC ${HAPPY_INCLUDE_DIRS})
target_compile_options(rtl_sdr_to_sip PUBLIC ${HAPPY_CFLAGS_OTHER})

target_link_libraries(rtl_sdr_to_sip -liniparser)

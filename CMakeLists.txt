cmake_minimum_required(VERSION 3.0)
project(CVM)

include(cotire.cmake)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -std=c++14 -DPRILIB=PriLib")

include_directories(prilib/include)
aux_source_directory(prilib/source PRILIB_SOURCE_FILES)

include_directories(include)
aux_source_directory(source SOURCE_FILES)
aux_source_directory(source/runtime SOURCE_RUNTIME_FILES)

add_executable(cvm ${SOURCE_FILES} ${SOURCE_RUNTIME_FILES} ${PRILIB_SOURCE_FILES})

set_target_properties(cvm PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "include/basic.h")
cotire(cvm)

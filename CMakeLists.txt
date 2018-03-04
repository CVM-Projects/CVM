cmake_minimum_required(VERSION 3.0)
project(CVM)

include(cotire.cmake)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -std=c++14")

include_directories(prilib/include)
add_subdirectory(prilib prilib)

include_directories(include)
aux_source_directory(source SOURCE_FILES)
aux_source_directory(source/runtime SOURCE_RUNTIME_FILES)

add_executable(cvm ${SOURCE_FILES} ${SOURCE_RUNTIME_FILES} ${PRILIB_SOURCE_FILES} include)

target_link_libraries(cvm prilib)
target_link_libraries(cvm gmp gmpxx)

set_target_properties(cvm PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "include/basic.h")
cotire(cvm)

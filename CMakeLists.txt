cmake_minimum_required(VERSION 3.0)
project(CVM)

include(cotire.cmake)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

add_subdirectory(prilib prilib)
add_subdirectory(lcmm lcmm)

include_directories(include)
aux_source_directory(source SOURCE_FILES)
aux_source_directory(source/runtime SOURCE_RUNTIME_FILES)

add_executable(cvm ${SOURCE_FILES} ${SOURCE_RUNTIME_FILES} include)
set_property(TARGET cvm PROPERTY CXX_STANDARD 14)

target_link_libraries(cvm prilib)
target_link_libraries(cvm lcmm)
target_link_libraries(cvm gmp gmpxx)

set_target_properties(cvm PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "include/basic.h")
cotire(cvm)

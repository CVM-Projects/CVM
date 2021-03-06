cmake_minimum_required(VERSION 3.0)
project(CVM)

include(cotire.cmake)

#------------------------------------------------------------------
# * Find GMP/MPIR Library
#------------------------------------------------------------------
find_path(GMP_OR_MPIR_INCLUDE_PATH gmpxx.h)

find_library(MPIR_LIBRARY mpir)

if (NOT ${MPIR_LIBRARY} MATCHES MPIR_LIBRARY-NOTFOUND)
  set(GMP_OR_MPIR_LIBRARY ${MPIR_LIBRARY})
else()
  find_library(GMP_PATH gmp)
  find_library(GMPXX_PATH gmpxx)
  set(GMP_OR_MPIR_LIBRARY ${GMP_PATH} ${GMPXX_PATH})
endif()

message(GMP_OR_MPIR_INCLUDE_PATH " " ${GMP_OR_MPIR_INCLUDE_PATH})
message(GMP_OR_MPIR_LIBRARY " " ${GMP_OR_MPIR_LIBRARY})
#------------------------------------------------------------------

include_directories(GMP_OR_MPIR_INCLUDE_PATH)

add_subdirectory(prilib prilib)
add_subdirectory(lcmm lcmm)

include_directories(include)
include_directories(${GMP_OR_MPIR_INCLUDE_PATH})
aux_source_directory(source SOURCE_FILES)
aux_source_directory(source/runtime SOURCE_RUNTIME_FILES)
aux_source_directory(source/inststruct SOURCE_INSTSTRUCT_FILES)
aux_source_directory(source/parser SOURCE_PARSER_FILES)
aux_source_directory(source/compiler SOURCE_COMPILER_FILES)

add_executable(cvm ${SOURCE_FILES} ${SOURCE_RUNTIME_FILES} ${SOURCE_INSTSTRUCT_FILES} ${SOURCE_PARSER_FILES} ${SOURCE_COMPILER_FILES} include)
set_property(TARGET cvm PROPERTY CXX_STANDARD 17)

target_link_libraries(cvm prilib)
target_link_libraries(cvm lcmm)
target_link_libraries(cvm ${GMP_OR_MPIR_LIBRARY})

set_target_properties(cvm PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "include/basic.h")
cotire(cvm)

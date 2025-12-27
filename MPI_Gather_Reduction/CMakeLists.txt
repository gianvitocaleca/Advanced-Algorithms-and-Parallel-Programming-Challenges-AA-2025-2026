cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(mpi_demo VERSION 1.0)
enable_language(C CXX)


# look for the MPI dependency
find_package(MPI REQUIRED C)


# compile in Release mode, unless the user say otherwise
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "The type of build" FORCE)
  message(STATUS "Setting build type to '${CMAKE_BUILD_TYPE}' as none was specified")
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
endif()


# application headers
set(header_path "${CMAKE_CURRENT_SOURCE_DIR}/src")
list(APPEND header_files
  "${header_path}/data.hpp"
  "${header_path}/process.hpp"
  "${header_path}/mpi_error_check.h"
)

# application sources
set(source_path "${CMAKE_CURRENT_SOURCE_DIR}/src")
list(APPEND source_files
  "${source_path}/data.cpp"
  "${source_path}/main.cpp"
  "${source_path}/process.cpp"
  "${source_path}/mpi_error_check.c"
)


# define the building step
add_executable(main ${header_files} ${source_files})
target_include_directories(main PRIVATE "${header_path}")

# define the flag to set more warning message during the compilation
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  list(APPEND custom_c_flags "-Wall" "-Wextra" "-pedantic")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  list(APPEND custom_c_flags
    "-Weverything"
    "-pedantic"
  )
endif()
target_compile_options(main PRIVATE $<$<COMPILE_LANGUAGE:C>:${custom_c_flags}>)


# link against MPI
# NOTE: we don't want to use the C++ bindings. However, the find_package
#       seems to ignore the MPI_CXX_SKIP_MPICXX variable. Therefore, we need
#       to set them manually >.>
target_compile_definitions(main PUBLIC "OMPI_SKIP_MPICXX") # OpenMPI
target_compile_definitions(main PUBLIC "MPICH_SKIP_MPICXX") # MPICH
target_link_libraries(main PUBLIC MPI::MPI_C)
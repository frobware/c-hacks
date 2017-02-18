# Prevent building in the source tree.

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "
ERROR: CMake generation is not allowed within the source directory!

You *MUST* remove the CMakeCache.txt file and try again from another
directory:

   rm CMakeCache.txt
   mkdir ninja-build
   cd nina-build
   cmake -G Ninja ..")
endif()

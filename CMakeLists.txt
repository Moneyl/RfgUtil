﻿
# Top level project. Treating this like a VS solution
cmake_minimum_required (VERSION 3.8)
project ("RfgUtil")

set(CMAKE_CXX_STANDARD           20)
set(CMAKE_CXX_STANDARD_REQUIRED  ON)
set(CMAKE_CXX_EXTENSIONS         OFF)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(BUILD_RFGTOOLS_TEST_APP FALSE)
add_subdirectory("Dependencies/RfgToolsPlusPlus")
add_subdirectory("Dependencies/cxxopts")
add_subdirectory("RfgUtil")
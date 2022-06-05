
cmake_minimum_required (VERSION 3.8)
project (RfgUtil VERSION 1.0.0
                  DESCRIPTION "RfgUtil"
                  LANGUAGES CXX)

# Get list of source files in project src folders recursively
# Places in SOURCES variable
file(GLOB_RECURSE SOURCES 
    "${CMAKE_SOURCE_DIR}/RfgUtil/*.cpp"
    "${CMAKE_SOURCE_DIR}/RfgUtil/*.h"
    ${CMAKE_SOURCE_DIR}/Dependencies/RfgToolsPlusPlus/Dependencies/BinaryTools/*.h
)

# Add source to this project's executable.
add_executable (RfgUtil ${SOURCES})

# Add include directories
target_include_directories(RfgUtil SYSTEM PRIVATE
    ${CMAKE_SOURCE_DIR}/
    ${CMAKE_SOURCE_DIR}/RfgUtil/
    ${CMAKE_SOURCE_DIR}/Dependencies/RfgToolsPlusPlus/Common/
    ${CMAKE_SOURCE_DIR}/Dependencies/RfgToolsPlusPlus/
    ${CMAKE_SOURCE_DIR}/Dependencies/RfgToolsPlusPlus/Dependencies/BinaryTools/
    ${CMAKE_SOURCE_DIR}/Dependencies/RfgToolsPlusPlus/RfgTools++/
    ${CMAKE_SOURCE_DIR}/Dependencies/RfgToolsPlusPlus/RfgTools++/RfgTools++/
    ${CMAKE_SOURCE_DIR}/Dependencies/cxxopts/include/
)

# Add statically linked libs
target_link_libraries(RfgUtil PRIVATE Common)
target_link_libraries(RfgUtil PRIVATE RfgTools++)
target_link_libraries(RfgUtil PRIVATE cxxopts)
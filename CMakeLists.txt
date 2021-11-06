cmake_minimum_required(VERSION 3.19.2)

set(CMAKE_CXX_STANDARD 14)

project(zeroone)

set(LIBRARY_OUTPUT_NAME ${PROJECT_NAME})

add_subdirectory(zeroone)

add_subdirectory(external/diatonic)

target_include_directories(
  ${PROJECT_NAME} 
  PUBLIC zeroone/include/ 
  PUBLIC external/diatonic/include
)

target_link_directories(
  ${PROJECT_NAME}
  PRIVATE zeroone
  PRIVATE external/diatonic
)

target_link_libraries("${PROJECT_NAME}" PUBLIC RtMidi diatonic)
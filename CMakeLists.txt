cmake_minimum_required(VERSION 3.19.2)

set(CMAKE_CXX_STANDARD 14)

project(zeroone)

set(LIBRARY_OUTPUT_NAME ${PROJECT_NAME})

add_subdirectory(zeroone)

add_subdirectory(external/diatonic)

add_subdirectory(external/rtmidi)

target_include_directories(
  ${PROJECT_NAME} 
  PUBLIC zeroone/include/ 
  PUBLIC external/diatonic/include
  PUBLIC external/rtmidi/
)

target_link_directories(
  ${PROJECT_NAME}
  PUBLIC zeroone
  PUBLIC external/diatonic
  PUBLIC external/rtmidi
)

target_link_libraries("${PROJECT_NAME}" PUBLIC diatonic RtMidi)
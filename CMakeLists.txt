cmake_minimum_required(VERSION 3.19.2)

set(CMAKE_CXX_STANDARD 14)

project(zeroone)

add_library(
  ${PROJECT_NAME} SHARED
  src/zoengine.h
  src/zoengine.cpp
  src/notes.hpp
  src/taskpool.hpp
  src/instrument.hpp
  src/generator.hpp
  src/expression.hpp
)

add_subdirectory(external/diatonic)
add_subdirectory(external/rtmidi)

target_include_directories(
  ${PROJECT_NAME} 
  PUBLIC external/diatonic/include/
  PRIVATE external/rtmidi/
)

target_link_directories(
  ${PROJECT_NAME}
  PUBLIC external/diatonic/src/
  PRIVATE external/rtmidi/
)

target_link_libraries(
  ${PROJECT_NAME}
  LINK_PUBLIC diatonic 
  LINK_PRIVATE rtmidi
)

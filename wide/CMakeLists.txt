cmake_minimum_required(VERSION 3.19.2)

set(CMAKE_CXX_STANDARD 14)

project(wideAPI)

set(LIBRARY_OUTPUT_NAME ${PROJECT_NAME})

add_library(
  ${LIBRARY_OUTPUT_NAME} SHARED
  src/notes.hpp
  src/taskpool.hpp
  src/generator.hpp
  src/instrument.hpp
  src/expression.hpp
  src/wide.h
  src/wide.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC include/)
target_link_libraries("${PROJECT_NAME}" PUBLIC RtMidi)
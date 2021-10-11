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

# set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${PROJECT_NAME} PUBLIC include/)

# add_executable("${PROJECT_NAME}" wide.cpp)
target_link_libraries("${PROJECT_NAME}" PUBLIC RtMidi)

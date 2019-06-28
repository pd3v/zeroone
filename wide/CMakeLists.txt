set (CMAKE_CXX_STANDARD 11)

# Create wide’s static library
ADD_LIBRARY( wide STATIC
sequencer.cpp
instrument.cpp
generator.cpp
expression.cpp
)

cmake_minimum_required (VERSION 2.8)

SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} --std=c++11" )

project (PiCam)

SET( SOURCE_FILES
  main.cpp
)

add_executable(PiCam ${SOURCE_FILES})
target_link_libraries(PiCam pthread)


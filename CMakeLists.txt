#we check the version of cmake currently used to make sure it match our requirement
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
 
#The name of our project, this correspond to the solution in Visual Studio
PROJECT(PiCam)

MESSAGE(STATUS "CMAKE_FIND_ROOT_PATH is: " ${CMAKE_FIND_ROOT_PATH})

IF (NOT DEFINED MAJOR)
  SET(MAJOR 0)
ENDIF()

IF (NOT DEFINED MINOR)
  SET(MINOR 0)
ENDIF()

IF (NOT DEFINED FIX)
  SET(FIX 0)
ENDIF()

IF (NOT DEFINED CPACK_PACKAGE_VERSION)
  SET(CPACK_PACKAGE_VERSION ${MAJOR}.${MINOR}.${FIX}.0)
ENDIF()

ADD_DEFINITIONS(-D_PACKAGE_VERSION=${CPACK_PACKAGE_VERSION})

SET(Boost_USE_STATIC_LIBS ON)
FIND_PACKAGE(Boost 1.55.0 REQUIRED system date_time regex)
MESSAGE(STATUS "Boost_LIBRARY_DIRS is: " ${Boost_LIBRARY_DIRS})

SET(Internal_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include)

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)

IF(MSVC)
  #Common definitions for all projects
  ADD_DEFINITIONS(
      -D_SCL_SECURE_NO_WARNINGS -D_WIN32_WINNT=0x0601 -D_WINSOCK_DEPRECATED_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS
  )
ENDIF()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} --std=c++11" )
ENDIF()

#in visual studio, display folder hierarchy in solution. Otherwise, all project appears all at the first hierarchy level
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

#subdirectory to traverse to find other CMakelists (for each projects)
ADD_SUBDIRECTORY(PiCam PiCam)



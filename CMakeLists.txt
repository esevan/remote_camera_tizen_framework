# Reqired CMake version and project name
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(remote-key-framework)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "\${prefix}")
SET(LIBDIR "\${prefix}/lib")
SET(INCLUDEDIR "\${prefix}/include")
SET(VERSION 1.0)

set(PROJECT_MAJOR_VERSION "1")
set(PROJECT_MINOR_VERSION "0")
set(PROJECT_RELEASE_VERSION "1")
set(CMAKE_VERBOSE_MAKEFILE OFF)

# Build flags
ADD_DEFINITIONS(-Wall -O3) 
#ADD_DEFINITIONS(-Wall -O3 -omit-frame-pointer) 
#ADD_DEFINITIONS(-Wall -g -D_DEBUG)

# Add projects in subdirectory
ADD_SUBDIRECTORY(server)

cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

# This file modifies the default compiler flags for certain compilers
set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_SOURCE_DIR}/cmake/CompilerDefaults.txt)

project(hdfstream VERSION 0.0.1)
enable_language(C)
include(CTest)
include(GNUInstallDirs)

# Specify location of custom configuration, if any
set(CONFIG_DIR "" CACHE PATH "Location of custom configuration files")
if(CONFIG_DIR)
  set(HDFSTREAM_CONFIG "${CONFIG_DIR}")
else()
  set(HDFSTREAM_CONFIG "${CMAKE_BINARY_DIR}/webapp/src/main/example/")
endif()

# Specify maximum shared memory buffer size
set(MAX_BUFFER_SIZE 2097152 CACHE STRING "Maximum size of the read buffer in bytes")

# Specify where to write example data
set(EXAMPLE_DATA_DIR "${CMAKE_BINARY_DIR}/data/" CACHE PATH "Where to write example data files")

# Always add runtime library path to executables
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Find the HDF5 library
find_package(HDF5 1.12 COMPONENTS C REQUIRED)

# Find the msgpack C library. Library name depends on version.
find_package(msgpack-c QUIET)
if(NOT msgpack-c_FOUND)
  find_package(msgpack REQUIRED)
  add_library(msgpack-c ALIAS msgpackc)
endif()

# Figure out flags for pthreads
set (THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

# Build the C library
add_subdirectory(./tests)
add_subdirectory(./src/reader)
add_subdirectory(./src/lib)

# Generate test data config files
add_subdirectory(./data/tests/basic)
add_subdirectory(./data/tests/permissions)
add_subdirectory(./data/tests/soft_links)
add_subdirectory(./data/tests/metadata)
add_subdirectory(./data/tests/cache)

set(BUILD_WEB_APP TRUE CACHE BOOL "Whether to build the java libraries and web application")
if(BUILD_WEB_APP)

  # Set up example data, if necessary
  if(NOT CONFIG_DIR)
    add_subdirectory(./data/pynbody)
    add_subdirectory(./data/swiftsimio)
  endif()

  # Check if we have java and native interface
  find_package(Java COMPONENTS Development)
  find_package(JNI COMPONENTS JVM)

  # Set java release version for JNI library. Should be the same as in
  # virtual_directory/pom.xml and webapp/pom.xml.
  set(CMAKE_JAVA_COMPILE_FLAGS "--release=11")

  if(${JNI_FOUND} AND ${Java_FOUND})

    # Look for maven executable
    set (MAVEN_DIR /usr/bin CACHE STRING "Maven bin directory (should contain mvn executable)")
    find_program(MVN_EXECUTABLE NAMES mvn HINTS ${MAVEN_DIR})

    # Store downloaded dependencies in the build dir
    set(MVN_REPO_DIR -Dmaven.repo.local=${CMAKE_BINARY_DIR}/mvn_repo)

    # Build the java native interface for the C library
    add_subdirectory(./jni)

    # Build the virtual_directory library and the web app if we have maven
    if(MVN_EXECUTABLE)
      add_subdirectory(virtual_directory)
      add_subdirectory(webapp)
    endif()

  endif()
endif()


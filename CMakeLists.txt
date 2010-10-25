
project(OpenColorIO)
set(OCIO_VERSION_MAJOR 0)
set(OCIO_VERSION_MINOR 7)
set(OCIO_VERSION_PATCH 0)
set(OCIO_VERSION "${OCIO_VERSION_MAJOR}.${OCIO_VERSION_MINOR}.${OCIO_VERSION_PATCH}")

cmake_minimum_required(VERSION 2.6)
set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/share/cmake )
if(NOT DEFINED CMAKE_FIRST_RUN)
    SET(CMAKE_FIRST_RUN 1 CACHE INTERNAL "")
endif()

###############################################################################
### GLOBAL ###

enable_language(CXX)

ENABLE_TESTING()

if(APPLE)
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES x86_64 CACHE STRING
            "Setting OSX Architectures, options are: ppc;i386;ppc64;x86_64"
            FORCE)
    endif()
    if(CMAKE_FIRST_RUN)
        message(STATUS "Setting OSX Architectures to: ${CMAKE_OSX_ARCHITECTURES}")
    endif()
endif()

# Set the default built type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)
if(CMAKE_FIRST_RUN)
    message(STATUS "Setting Build Type to: ${CMAKE_BUILD_TYPE}")
endif()
   
# Set the default namespace
if(NOT OCIO_NAMESPACE)
  set(OCIO_NAMESPACE OpenColorIO CACHE STRING
      "Specify the master OCIO C++ namespace: Options include OpenColorIO OpenColorIO_<YOURFACILITY> etc."
      FORCE)
endif(NOT OCIO_NAMESPACE)

add_definitions("-DOCIO_NAMESPACE=${OCIO_NAMESPACE}")
if(CMAKE_FIRST_RUN)
    message(STATUS "Setting Namespace to: ${OCIO_NAMESPACE}")
endif()

# Enable a bunch of compiler warnings...
# http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wshadow -Wconversion -Wcast-qual -Wformat=2")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Weffc++ -Winline ")

###############################################################################
### YAML ###

include(ExternalProject)
set(YAML_CPP_VERSION r423)
ExternalProject_Add(YAML_CPP_LOCAL
    URL ${CMAKE_SOURCE_DIR}/ext/yaml-cpp-${YAML_CPP_VERSION}.tar.gz
    BINARY_DIR ext/build/yaml-cpp
    PATCH_COMMAND patch -p1 < ${CMAKE_SOURCE_DIR}/ext/yaml-cpp-${YAML_CPP_VERSION}.patch
    INSTALL_DIR ext/dist
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}/ext/dist -DYAML_CPP_BUILD_TOOLS:BOOL=FALSE
)
set(YAML_CPP_INCLUDE_DIRS ${PROJECT_BINARY_DIR}/ext/dist/include)
set(YAML_CPP_LIBRARY_DIRS ${PROJECT_BINARY_DIR}/ext/dist/lib)
set(YAML_CPP_STATIC_LIBRARIES ${PROJECT_BINARY_DIR}/ext/dist/lib/libyaml-cpp.a)

###############################################################################
### BOOST ###

set (Boost_ADDITIONAL_VERSIONS "1.43" "1.43.0" "1.42" "1.42.0" 
                               "1.41" "1.41.0" "1.40" "1.40.0"
                               "1.39" "1.39.0" "1.38" "1.38.0"
                               "1.37" "1.37.0" "1.34.1" "1_34_1")
set (Boost_USE_STATIC_LIBS ON) # TODO: make this a option
set (Boost_USE_MULTITHREADED ON)
find_package (Boost 1.34 COMPONENTS unit_test_framework)

###############################################################################
### SSE ###

option(USE_SSE "Set to OFF to disable SSE optimizations." ON)
if(USE_SSE)
    if(CMAKE_FIRST_RUN)
        message(STATUS "SSE Runtime: ENABLED")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
    add_definitions("-DUSE_SSE")
else()
    message(STATUS "SSE Runtime: DISABLED")
endif()

###############################################################################
### CORE ###

add_subdirectory (src/core)
add_subdirectory (src/core_tests)

###############################################################################
### TEST BED ###

add_subdirectory (src/testbed)

###############################################################################
### NUKE ###

SET( Nuke_INSTALL_PATH "/net/apps/rhel40m64/foundry/nuke6.0.5/include" )
find_package( Nuke )
if( NUKE_FOUND )
    if(CMAKE_FIRST_RUN)
        message(STATUS "    will build the Nuke plugins against ${Nuke_LIBRARY_DIR}")
    endif()
    add_subdirectory (src/nuke)
else()
    if(CMAKE_FIRST_RUN)
        message(STATUS "    try setting NDK_PATH environment variable, if you want to build the nuke plugins")
    endif()
endif()

###############################################################################
### PYGLUE ###

add_subdirectory (src/pyglue)

###############################################################################
### CPACK ###

set(CPACK_PACKAGE_VERSION_MAJOR ${OCIO_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${OCIO_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${OCIO_VERSION_PATCH})
set(CPACK_GENERATOR None)
set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_SOURCE_PACKAGE_FILE_NAME "ocio.${OCIO_VERSION}")
include(CPack)

###############################################################################
### CTEST ###

add_custom_target(test_verbose
                  COMMAND ctest -VV
                  DEPENDS ocio_core_tests
                  COMMENT "Running ctest with verbose output")

# Log CMake first run done
SET(CMAKE_FIRST_RUN 0 CACHE INTERNAL "")


project(OpenColorIO)
set(OCIO_VERSION_MAJOR 0)
set(OCIO_VERSION_MINOR 7)
set(OCIO_VERSION_PATCH 4)

# This does not include the SOVERSION override, on purpose, so that the
# OCIO_VERSION value will be an accurate reflection of the underlying library version.
set(OCIO_VERSION "${OCIO_VERSION_MAJOR}.${OCIO_VERSION_MINOR}.${OCIO_VERSION_PATCH}")

if (NOT SOVERSION)
    set (SOVERSION ${OCIO_VERSION_MAJOR} CACHE STRING "Set the SO version in the SO name of the output library")
endif ()
if (NOT ${SOVERSION} STREQUAL ${OCIO_VERSION_MAJOR})
    set (OCIO_VERSION_MAJOR ${SOVERSION})
endif ()


cmake_minimum_required(VERSION 2.8)
set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/share/cmake )
if(NOT DEFINED CMAKE_FIRST_RUN)
    SET(CMAKE_FIRST_RUN 1 CACHE INTERNAL "")
endif()

###############################################################################
### GLOBAL ###

include(ParseArguments)
include(OCIOMacros)

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

OCIOFindPython()

# Enable a bunch of compiler warnings...
# http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wshadow -Wconversion -Wcast-qual -Wformat=2")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Weffc++ -Winline ")

set(EXTDIST_ROOT ${CMAKE_BINARY_DIR}/ext/dist)
set(EXTDIST_BINPATH ${EXTDIST_ROOT}/bin)
set(EXTDIST_PYTHONPATH ${EXTDIST_ROOT}/lib/python${PYTHON_VERSION}/site-packages)
set(PYTHONPATH ${EXTDIST_PYTHONPATH}:$ENV{PYTHONPATH})

if(CMAKE_FIRST_RUN)
    message(STATUS "Setting Namespace to: ${OCIO_NAMESPACE}")
    message(STATUS "Setting EXTDIST_BINPATH: ${EXTDIST_BINPATH}")
    message(STATUS "Setting EXTDIST_PYTHONPATH: ${EXTDIST_PYTHONPATH}")
endif()

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
### Documentation ###

if(NOT MAKE_DOCS)
    set(MAKE_DOCS NO CACHE BOOL
      "Specify whether to build documentation."
      FORCE)
endif()

if(CMAKE_FIRST_RUN)
    message(STATUS "Generate Documentation: ${MAKE_DOCS}")
endif()

if(MAKE_DOCS)
    add_subdirectory (docs)
endif()

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
### APPS ###

# Should we include OpenGL?
if(NOT USE_OPENGL)
    set(USE_OPENGL NO CACHE BOOL
      "Specify whether to build everything that requires OpenGL."
      FORCE)
endif()


if(CMAKE_FIRST_RUN AND NOT USE_OPENGL)
    message(STATUS "USE_OPENGL not set. OpenGL example applications will not be built.")
endif()

if(USE_OPENGL)
    if(APPLE)
        INCLUDE_DIRECTORIES ( /System/Library/Frameworks )
    endif()
    
    # Find OpenGL
    find_package(OpenGL)
    if(OPENGL_FOUND)
        message(STATUS "Found OpenGL library ${OPENGL_LIBRARY}")
        message(STATUS "Found OpenGL includes ${OPENGL_INCLUDE_DIR}")
    else()
        message (STATUS "OpenGL not found")
    endif()
    
    # Find GLUT
    find_package(GLUT)
    if(GLUT_FOUND)
        message(STATUS "Found GLUT library ${GLUT_LIBRARY}")
    else()
        message (STATUS "GLUT not found")
    endif()
    
    # Find GLEW
    set (GLEW_VERSION 1.5.1)
    FIND_PATH( GLEW_INCLUDES GL/glew.h
        /usr/include
        /usr/local/include
        /sw/include
        /opt/local/include
        DOC "The directory where GL/glew.h resides")
    FIND_LIBRARY( GLEW_LIBRARIES
        NAMES GLEW glew
        PATHS
        /usr/lib64
        /usr/lib
        /usr/local/lib64
        /usr/local/lib
        /sw/lib
        /opt/local/lib
        DOC "The GLEW library")
    if (GLEW_INCLUDES AND GLEW_LIBRARIES)
        set (GLEW_FOUND TRUE)
        message(STATUS "Found GLEW library ${GLEW_LIBRARIES}")
        message (STATUS "Found GLEW includes ${GLEW_INCLUDES}")
    else ()
        message (STATUS "GLEW not found")
    endif ()
endif(USE_OPENGL)

if(CMAKE_FIRST_RUN AND NOT USE_OIIO)
    message(STATUS "USE_OIIO not set. OpenImageIO example applications will not be built.")
endif()

if(USE_OIIO)
    if( OIIO_PATH )
    message(STATUS "OIIO path excplicitly specified: ${OIIO_PATH}")
    endif()
    FIND_PATH( OIIO_INCLUDES OpenImageIO/version.h
        ${OIIO_PATH}/include/
        /usr/include
        /usr/local/include
        /sw/include
        /opt/local/include
        DOC "The directory where OpenImageIO/version.h resides")
    FIND_LIBRARY( OIIO_LIBRARIES
        NAMES OIIO OpenImageIO
        PATHS
        ${OIIO_PATH}/lib/
        /usr/lib64
        /usr/lib
        /usr/local/lib64
        /usr/local/lib
        /sw/lib
        /opt/local/lib
        DOC "The OIIO library")
        
    if (OIIO_INCLUDES AND OIIO_LIBRARIES)
        set (OIIO_FOUND TRUE)
        message(STATUS "Found OIIO library ${OIIO_LIBRARIES}")
        message (STATUS "Found OIIO includes ${OIIO_INCLUDES}")
    else ()
        message (STATUS "OIIO not found")
    endif ()
endif(USE_OIIO)

add_subdirectory (src/apps/ociocheck)
add_subdirectory (src/apps/ocioconvert)
add_subdirectory (src/apps/ociodisplay)

###############################################################################
### NUKE ###

find_package( Nuke )
if( NUKE_FOUND )
    if(CMAKE_FIRST_RUN)
        message(STATUS "    will build the Nuke plugins against ${Nuke_LIBRARY_DIR}")
    endif()
    add_subdirectory (src/nuke)
else()
    if(CMAKE_FIRST_RUN)
        message(STATUS "    if you want to build the Nuke plugins, add the flag -D NUKE_INSTALL_PATH=... or set the NDK_PATH environment variable")
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

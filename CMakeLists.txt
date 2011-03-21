project(OpenColorIO)
set(OCIO_VERSION_MAJOR 0)
set(OCIO_VERSION_MINOR 7)
set(OCIO_VERSION_PATCH 7)

# This does not include the SOVERSION override, on purpose, so that the
# OCIO_VERSION value will be an accurate reflection of the underlying library version.
set(OCIO_VERSION "${OCIO_VERSION_MAJOR}.${OCIO_VERSION_MINOR}.${OCIO_VERSION_PATCH}")

if(NOT SOVERSION)
    set(SOVERSION ${OCIO_VERSION_MAJOR} CACHE STRING "Set the SO version in the SO name of the output library")
endif()
if(NOT ${SOVERSION} STREQUAL ${OCIO_VERSION_MAJOR})
    set(OCIO_VERSION_MAJOR ${SOVERSION})
endif()


cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/share/cmake)
if(NOT DEFINED CMAKE_FIRST_RUN)
    SET(CMAKE_FIRST_RUN 1 CACHE INTERNAL "")
endif()

###############################################################################
### GLOBAL ###

include(ParseArguments)
include(OCIOMacros)
include(ExternalProject)

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

# Enable a bunch of compiler warnings...
# http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wshadow -Wconversion -Wcast-qual -Wformat=2")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Weffc++ -Winline ")

# Find Python, used for (possibly) building pyglue, and now to
# construct the external project path
OCIOFindPython()

set(EXTDIST_ROOT ${CMAKE_BINARY_DIR}/ext/dist)
set(EXTDIST_BINPATH ${EXTDIST_ROOT}/bin)
if(PYTHON_OK)
    set(EXTDIST_PYTHONPATH ${EXTDIST_ROOT}/lib/python${PYTHON_VERSION}/site-packages)
    set(PYTHONPATH ${EXTDIST_PYTHONPATH}:$ENV{PYTHONPATH})
endif()

if(CMAKE_FIRST_RUN)
    message(STATUS "Setting Namespace to: ${OCIO_NAMESPACE}")
    message(STATUS "Setting EXTDIST_BINPATH: ${EXTDIST_BINPATH}")
    message(STATUS "Setting EXTDIST_PYTHONPATH: ${EXTDIST_PYTHONPATH}")
endif()

###############################################################################
### tinyxml ###

set(TINYXML_VERSION 2_6_1)
ExternalProject_Add(tinyxml
    URL ${CMAKE_SOURCE_DIR}/ext/tinyxml_${TINYXML_VERSION}.tar.gz
    PATCH_COMMAND patch -f -p1 < ${CMAKE_SOURCE_DIR}/ext/tinyxml_${TINYXML_VERSION}.patch
    BINARY_DIR ext/build/tinyxml
    INSTALL_DIR ext/dist
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}/ext/dist
)
set(TINYXML_STATIC_LIBRARIES ${PROJECT_BINARY_DIR}/ext/dist/lib/libtinyxml.a)

###############################################################################
### YAML ###

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
### LCMS ###

set(LCMS_VERSION 2.1)
ExternalProject_Add(LCMS
    URL ${CMAKE_SOURCE_DIR}/ext/lcms2-${LCMS_VERSION}.tar.gz
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ./configure --prefix=${PROJECT_BINARY_DIR}/ext/dist --without-jpeg --without-tiff --without-zlib
    BUILD_COMMAND make
    INSTALL_COMMAND make install
)
set(LCMS_INCLUDE_DIRS ${PROJECT_BINARY_DIR}/ext/dist/include)
set(LCMS_STATIC_LIBRARIES ${PROJECT_BINARY_DIR}/ext/dist/lib/liblcms2.a)

###############################################################################
### Externals ###

set(EXTERNAL_INCLUDE_DIRS ${PROJECT_BINARY_DIR}/ext/dist/include)
set(EXTERNAL_COMPILE_FLAGS "-DTIXML_USE_STL -fPIC -fvisibility-inlines-hidden -fvisibility=hidden")
set(EXTERNAL_LINK_FLAGS "")
set(EXTERNAL_LIBRARY_DIRS ${PROJECT_BINARY_DIR}/ext/dist/lib)
set(EXTERNAL_LIBRARIES
    ${TINYXML_STATIC_LIBRARIES}
    ${YAML_CPP_STATIC_LIBRARIES}
    ${LCMS_STATIC_LIBRARIES})

###############################################################################
### Documentation ###

if(NOT MAKE_DOCS)
    set(MAKE_DOCS NO CACHE BOOL
      "Specify whether to build documentation."
      FORCE)
endif()

messageonce("Generate Documentation: ${MAKE_DOCS}")

if(MAKE_DOCS)
    if(PYTHON_OK)
        add_subdirectory(docs)
    else()
        message(STATUS "Building the documentation requires Python, but locating Python failed: ${PYTHON_ERR}")
    endif()
endif()

###############################################################################
### SSE ###

option(USE_SSE "Set to OFF to disable SSE optimizations." ON)
if(USE_SSE)
    messageonce("SSE Runtime: ENABLED")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
    add_definitions("-DUSE_SSE")
else()
    messageonce("SSE Runtime: DISABLED")
endif()

###############################################################################
### TRUELIGHT ###

find_package(Truelight)
if(TRUELIGHT_FOUND)
    messageonce("Will build the truelight transform support against ${Truelight_LIBRARY_DIR}")
    add_definitions(-DOCIO_TRUELIGHT_SUPPORT)
    set(EXTERNAL_INCLUDE_DIRS ${EXTERNAL_INCLUDE_DIRS} ${Truelight_INCLUDE_DIR})
    set(EXTERNAL_COMPILE_FLAGS "${EXTERNAL_COMPILE_FLAGS} ${Truelight_COMPILE_FLAGS}")
    set(EXTERNAL_LINK_FLAGS "${EXTERNAL_LINK_FLAGS} ${Truelight_LINK_FLAGS}")
    set(EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES} ${Truelight_LIBRARIES})
else()
    messageonce("Not building truelight transform support. Add the flag -D TRUELIGHT_INSTALL_PATH=... or set the TRUELIGHT_ROOT environment variable")
endif()

###############################################################################
### CORE ###

add_subdirectory(src/core)
add_subdirectory(src/core_tests)

###############################################################################
### TEST BED ###

add_subdirectory(src/testbed)

###############################################################################
### APPS ###

# Try to find OpenImageIO (OIIO) and OpenGL stuff
OCIOFindOpenImageIO()
OCIOFindOpenGL()

# ocioconvert: performs file format conversion and colorspace
# transforms (uses OpenImageIO)
if(OIIO_FOUND)
    add_subdirectory(src/apps/ocioconvert)
else()
    messageonce("Not building ocioconvert. Requirement(s) found: OIIO:${OIIO_FOUND}")
endif()

# ociodisplay, displays color-transformed images (uses OpenImageIO,
# and OpenGL)
if(OPENGL_FOUND AND GLUT_FOUND AND GLEW_FOUND AND OIIO_FOUND)
    add_subdirectory(src/apps/ociodisplay)
else()
    messageonce("Not building ociodisplay. Requirement(s) found, OpenGL:${OPENGL_FOUND}, GLUT:${GLUT_FOUND}, GLEW:${GLEW_FOUND}, OIIO:${OIIO_FOUND}")
endif()

# ociocheck: verifies an OCIO config
add_subdirectory(src/apps/ociocheck)

# ocio2icc builds ICC profiles from an OCIO color transform
# (uses a bundled version of LittleCMS)
add_subdirectory(src/apps/ocio2icc)

# ociobakelut writes out luts
add_subdirectory(src/apps/ociobakelut)

###############################################################################
### NUKE ###

find_package(Nuke)
if(NUKE_FOUND)
    messageonce("Will build the Nuke plugins against ${Nuke_LIBRARY_DIR}")
    add_subdirectory(src/nuke)
else()
    messageonce("Not building Nuke plugins. Add the flag -D NUKE_INSTALL_PATH=... or set the NDK_PATH environment variable")
endif()

###############################################################################
### PYGLUE ###

if(PYTHON_OK)
    add_subdirectory(src/pyglue)
else()
    messageonce("Python bindings will not be built: ${PYTHON_ERR}")
endif()

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

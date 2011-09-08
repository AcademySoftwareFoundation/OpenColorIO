project(OpenColorIO)
set(OCIO_VERSION_MAJOR 0)
set(OCIO_VERSION_MINOR 9)
set(OCIO_VERSION_PATCH 0)

cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/share/cmake)
if(NOT DEFINED CMAKE_FIRST_RUN)
    SET(CMAKE_FIRST_RUN 1 CACHE INTERNAL "")
endif()

###############################################################################
### GLOBAL ###

option(OCIO_BUILD_SHARED "Set to OFF to disable building the shared core library" ON)
option(OCIO_BUILD_STATIC "Set to OFF to disable building the static core library" ON)
option(OCIO_BUILD_TRUELIGHT "Set to OFF to disable truelight" ON)
option(OCIO_BUILD_APPS "Set to OFF to disable command-line apps" ON)
option(OCIO_BUILD_NUKE "Set to OFF to disable building nuke plugins" ON)
option(OCIO_BUILD_DOCS "Specify whether to build documentation" OFF)
option(OCIO_BUILD_TESTS "Specify whether to build unittests" OFF)
option(OCIO_BUILD_PYGLUE "Specify whether to build python bindings" ON)

option(OCIO_USE_SSE "Specify whether to enable SSE CPU performance optimizations" ON)
option(OCIO_USE_BOOST_PTR "Set to ON to enable boost shared_ptr (necessary when tr1 is not available)" OFF)

# This does not include the SOVERSION override, on purpose, so that the
# OCIO_VERSION value will be an accurate reflection of the underlying library version.
set(OCIO_VERSION "${OCIO_VERSION_MAJOR}.${OCIO_VERSION_MINOR}.${OCIO_VERSION_PATCH}")

if(NOT SOVERSION)
    set(SOVERSION ${OCIO_VERSION_MAJOR} CACHE STRING "Set the SO version in the SO name of the output library")
endif()

###############################################################################


include(ParseArguments)
include(OCIOMacros)
include(ExternalProject)

enable_language(CXX)

ENABLE_TESTING()

if(APPLE OR IPHONE)
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
messageonce(STATUS "Setting Namespace to: ${OCIO_NAMESPACE}")

# If CMAKE_INSTALL_EXEC_PREFIX is not specified, install binaries
# directly into the regular install prefix
if(NOT CMAKE_INSTALL_EXEC_PREFIX)
    messageonce("Exec prefix not specified, defaulting to ${CMAKE_INSTALL_PREFIX}")
    set(CMAKE_INSTALL_EXEC_PREFIX ${CMAKE_INSTALL_PREFIX})
endif()

messageonce("Use Boost Ptr: ${OCIO_USE_BOOST_PTR}")
if(OCIO_USE_BOOST_PTR)
    set(Boost_ADDITIONAL_VERSIONS "1.45" "1.44" "1.43" "1.43.0" "1.42"
                                  "1.42.0" "1.41" "1.41.0" "1.40"
                                  "1.40.0" "1.39" "1.39.0" "1.38"
                                  "1.38.0" "1.37" "1.37.0" "1.34.1"
                                  "1_34_1")
    set(Boost_USE_MULTITHREADED ON)
    find_package(Boost 1.34)
    if(NOT Boost_FOUND)
        message(FATAL_ERROR "OCIO_USE_BOOST_PTR is specified, but a boost installation could not be found.")
    else()
        set(OCIO_USE_BOOST_PTR 1)
    endif()

    set(EXTERNAL_INCLUDE_DIRS ${EXTERNAL_INCLUDE_DIRS} ${Boost_INCLUDE_DIR})
    # set(EXTERNAL_LIBRARIES ${EXTERNAL_LIBRARIES} ${Boost_LIB_DIAGNOSTIC_DEFINITIONS})
else()
    set(OCIO_USE_BOOST_PTR 0)
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    # Enable a bunch of compiler warnings...
    # http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wshadow -Wconversion -Wcast-qual -Wformat=2")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
endif(CMAKE_COMPILER_IS_GNUCXX)

if(WIN32)
    if(PYTHON_VERSION AND PYTHON_INCLUDE AND PYTHON_LIB)
        set(EXTDIST_PYTHONPATH ${PYTHON_LIB}/python${PYTHON_VARIANT_PATH}/site-packages)
        set(PYTHONPATH ${EXTDIST_PYTHONPATH};$ENV{PYTHONPATH})
        set(PYTHON_OK YES)
        message(STATUS "Setting Python path to: ${PYTHONPATH}")
    endif()
else()
    # Find Python, used for (possibly) building pyglue, and now to
    # construct the external project path
    OCIOFindPython()

    set(EXTDIST_ROOT ${CMAKE_BINARY_DIR}/ext/dist)
    set(EXTDIST_BINPATH ${EXTDIST_ROOT}/bin)
    if(PYTHON_OK)
        set(EXTDIST_PYTHONPATH ${EXTDIST_ROOT}/lib/python${PYTHON_VERSION}/site-packages)
        set(PYTHONPATH ${EXTDIST_PYTHONPATH}:$ENV{PYTHONPATH})
    endif()
endif()

messageonce("Setting EXTDIST_BINPATH: ${EXTDIST_BINPATH}")
messageonce("Setting EXTDIST_PYTHONPATH: ${EXTDIST_PYTHONPATH}")

###############################################################################
### tinyxml ###

set(TINYXML_VERSION 2_6_1)
set(TINYXML_CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}/ext/dist)
if(CMAKE_TOOLCHAIN_FILE)
    set(TINYXML_CMAKE_ARGS ${TINYXML_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
endif()
ExternalProject_Add(tinyxml
    URL ${CMAKE_SOURCE_DIR}/ext/tinyxml_${TINYXML_VERSION}.tar.gz
    PATCH_COMMAND patch -f -p1 < ${CMAKE_SOURCE_DIR}/ext/tinyxml_${TINYXML_VERSION}.patch
    BINARY_DIR ext/build/tinyxml
    INSTALL_DIR ext/dist
    CMAKE_ARGS ${TINYXML_CMAKE_ARGS}
)
if(WIN32)
    set(TINYXML_STATIC_LIBRARIES ${PROJECT_BINARY_DIR}/ext/dist/lib/tinyxml.lib)
else()
    set(TINYXML_STATIC_LIBRARIES ${PROJECT_BINARY_DIR}/ext/dist/lib/libtinyxml.a)
endif()

###############################################################################
### YAML ###

set(YAML_CPP_VERSION r482)
set(YAML_CPP_CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}/ext/dist -DYAML_CPP_BUILD_TOOLS:BOOL=FALSE)
if(CMAKE_TOOLCHAIN_FILE)
    set(YAML_CPP_CMAKE_ARGS ${YAML_CPP_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
endif()
ExternalProject_Add(YAML_CPP_LOCAL
    URL ${CMAKE_SOURCE_DIR}/ext/yaml-cpp-${YAML_CPP_VERSION}.tar.gz
    BINARY_DIR ext/build/yaml-cpp
    PATCH_COMMAND patch -p1 < ${CMAKE_SOURCE_DIR}/ext/yaml-cpp-${YAML_CPP_VERSION}.patch
    INSTALL_DIR ext/dist
    CMAKE_ARGS ${YAML_CPP_CMAKE_ARGS}
)
set(YAML_CPP_INCLUDE_DIRS ${PROJECT_BINARY_DIR}/ext/dist/include)
set(YAML_CPP_LIBRARY_DIRS ${PROJECT_BINARY_DIR}/ext/dist/lib)
if(WIN32)
    set(YAML_CPP_STATIC_DEBUG_LIBRARIES ${PROJECT_BINARY_DIR}/ext/dist/lib/libyaml-cppmdd.lib)
    set(YAML_CPP_STATIC_OPTIMIZED_LIBRARIES ${PROJECT_BINARY_DIR}/ext/dist/lib/libyaml-cppmd.lib)
else()
    set(YAML_CPP_STATIC_GENERAL_LIBRARIES ${PROJECT_BINARY_DIR}/ext/dist/lib/libyaml-cpp.a)
endif()

###############################################################################
### Externals ###

set(EXTERNAL_INCLUDE_DIRS ${EXTERNAL_INCLUDE_DIRS} ${PROJECT_BINARY_DIR}/ext/dist/include)
set(EXTERNAL_COMPILE_FLAGS "-DTIXML_USE_STL")
if(CMAKE_COMPILER_IS_GNUCXX)
    set(EXTERNAL_COMPILE_FLAGS "${EXTERNAL_COMPILE_FLAGS} -fPIC -fvisibility-inlines-hidden -fvisibility=hidden")
endif()
set(EXTERNAL_LINK_FLAGS "")
set(EXTERNAL_LIBRARY_DIRS ${PROJECT_BINARY_DIR}/ext/dist/lib)
set(EXTERNAL_DEBUG_LIBRARIES ${YAML_CPP_STATIC_DEBUG_LIBRARIES})
set(EXTERNAL_OPTIMIZED_LIBRARIES ${YAML_CPP_STATIC_OPTIMIZED_LIBRARIES})
set(EXTERNAL_GENERAL_LIBRARIES ${TINYXML_STATIC_LIBRARIES} ${YAML_CPP_STATIC_GENERAL_LIBRARIES})

###############################################################################
### Documentation ###

messageonce("Generate Documentation: ${OCIO_BUILD_DOCS}")

if(OCIO_BUILD_DOCS)
    if(PYTHON_OK)
        add_subdirectory(docs)
    else()
        message(STATUS "Building the documentation requires Python, but locating Python failed: ${PYTHON_ERR}")
    endif()
endif()

###############################################################################
### SSE ###

messageonce("SSE Optimizations: ${OCIO_USE_SSE}")

if(OCIO_USE_SSE)
    if(WIN32)
        # SSE instructions are automatically compiled into 64-bit applications so enabling the option is redundant and
        # actually produces an unknown option warning in Visual Studio.
        if(NOT CMAKE_CL_64)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
        endif()
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
    endif()
    add_definitions("-DUSE_SSE")
endif()

###############################################################################
### TRUELIGHT ###

if(OCIO_BUILD_TRUELIGHT)
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
endif()

###############################################################################
### CORE ###

add_subdirectory(src/core)

messageonce("Build Unit Tests: ${OCIO_BUILD_TESTS}")
if(OCIO_BUILD_TESTS)
    add_subdirectory(src/core_tests)
endif()

###############################################################################
### APPS ###

if(OCIO_BUILD_APPS)
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
endif()

###############################################################################
### NUKE ###

if(OCIO_BUILD_NUKE)
    find_package(Nuke)
    if(NUKE_FOUND)
        messageonce("Will build the Nuke plugins against ${Nuke_LIBRARY_DIR}")
        add_subdirectory(src/nuke)
    else()
        messageonce("Not building Nuke plugins. Add the flag -D NUKE_INSTALL_PATH=... or set the NDK_PATH environment variable")
    endif()
endif()

###############################################################################
### PYGLUE ###

if(OCIO_BUILD_PYGLUE)
    if(PYTHON_OK)
        add_subdirectory(src/pyglue)
    else()
        messageonce("Python bindings will not be built: ${PYTHON_ERR}")
    endif()
endif()

###############################################################################
### Configure env script ###
configure_file(${CMAKE_SOURCE_DIR}/share/ocio/setup_ocio.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/share/ocio/setup_ocio.sh @ONLY)

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/share/ocio/setup_ocio.sh DESTINATION share/ocio/)

###############################################################################
### CPACK ###

set(CPACK_PACKAGE_VERSION_MAJOR ${SOVERSION})
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

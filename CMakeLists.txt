
project(OpenColorIO)
cmake_minimum_required(VERSION 2.6)
set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/share/cmake )
if(NOT DEFINED CMAKE_FIRST_RUN)
    SET(CMAKE_FIRST_RUN 1 CACHE INTERNAL "")
endif()

###############################################################################
### GLOBAL ###

enable_language(CXX)

if(APPLE)
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES i386;x86_64)
    endif()
    if(CMAKE_FIRST_RUN)
        message(STATUS "Setting OSX Architectures to: ${CMAKE_OSX_ARCHITECTURES}")
        message(STATUS "    possible options CMAKE_OSX_ARCHITECTURES:STRING=ppc;i386;ppc64;x86_64")
    endif()
endif()

# http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wshadow -Winline -Wconversion -Wcast-qual -Wformat=2")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Weffc++")

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
#
# PYTHON
#

set(PYHELP "path to Python executable (also used to find headers against which to compile Python bindings)")
set(PYTHON_HEADER Python.h)
set(PYTHON python CACHE FILEPATH ${PYHELP})
execute_process(COMMAND ${PYTHON} -c "from distutils import sysconfig; print sysconfig.get_python_inc()"
    OUTPUT_VARIABLE PYTHON_INCLUDE
    RESULT_VARIABLE PYTHON_RETURNVALUE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(PYTHON_OK NO)

if(${PYTHON_RETURNVALUE} EQUAL 0)
    execute_process(COMMAND ${PYTHON} -c "from distutils import sysconfig; print sysconfig.get_python_version()"
        OUTPUT_VARIABLE PYTHON_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(EXISTS "${PYTHON_INCLUDE}/Python.h")
        set(PYTHON_OK YES)
    else()
        set(PYTHON_ERR "${PYTHON_HEADER} not found in ${PYTHON_INCLUDE}.")
    endif()
elseif(${PYTHON_RETURNVALUE} GREATER 0)
    set(PYTHON_ERR "${PYTHON} returned ${PYTHON_RETURNVALUE} trying to determine header location.")
else()
    set(PYTHON_ERR "${PYTHON}: ${PYTHON_RETURNVALUE}.")
endif()

if(PYTHON_OK)
    message(STATUS "Python ${PYTHON_VERSION} okay, will build the Python bindings against ${PYTHON_INCLUDE}")

    add_library(PyOpenColorIO MODULE
        src/pyglue/PyCDLTransform.cpp
        src/pyglue/PyColorSpace.cpp
        src/pyglue/PyConstants.cpp
        src/pyglue/PyConfig.cpp
        src/pyglue/PyDisplayTransform.cpp
        src/pyglue/PyFileTransform.cpp
        src/pyglue/PyGroupTransform.cpp
        src/pyglue/PyMain.cpp
        src/pyglue/PyUtil.cpp
    )
    include_directories(
        ${PYTHON_INCLUDE}
        ${OpenColorIO_SOURCE_DIR}/export/
    )
    # Exclude the 'lib' prefix from the name.
    set_property(TARGET PyOpenColorIO
        PROPERTY PREFIX ""
    )
    target_link_libraries(PyOpenColorIO OpenColorIO)

    if(APPLE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -undefined dynamic_lookup")
    endif()

    add_custom_target(PyOpenColorIOTest
        COMMAND env PYTHONPATH=${OpenColorIO_LIBRARY_OUTPUT_DIRECTORY} ${PYTHON} ${OpenColorIO_SOURCE_DIR}/src/testbed/main.py
        DEPENDS PyOpenColorIO
        SOURCES src/testbed/main.py
        VERBATIM
    )
else()
    message(STATUS "Python bindings will not be built: ${PYTHON_ERR}")
endif()

# Log CMake first run done
SET(CMAKE_FIRST_RUN 0 CACHE INTERNAL "")

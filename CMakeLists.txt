
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
### PYGLUE ###

add_subdirectory (src/pyglue)


# Log CMake first run done
SET(CMAKE_FIRST_RUN 0 CACHE INTERNAL "")

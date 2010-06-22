# TODO: Opt vs. Debug

cmake_minimum_required(VERSION 2.6)
project(OpenColorSpace)

enable_language(CXX)

###############################################################################
#
# CORE
#

add_library(OpenColorSpace SHARED
    src/core/ColorSpace.cpp
    src/core/Config.cpp
    src/core/CoordinateOps.cpp
    src/core/Exception.cpp
    src/core/FileFormat3DL.cpp
    src/core/FileFormatSpi1D.cpp
    src/core/FileFormatSpi3D.cpp
    src/core/FileFormatSpiMtx.cpp
    src/core/FileTransform.cpp
    src/core/GroupTransform.cpp
    src/core/HwRenderDesc.cpp
    src/core/ImageDesc.cpp
    src/core/ImagePacking.cpp
    src/core/Lut1DOp.cpp
    src/core/Lut3DOp.cpp
    src/core/MathUtils.cpp
    src/core/MatrixOps.cpp
    src/core/Op.cpp
    src/core/ParseUtils.cpp
    src/core/PathUtils.cpp
    src/core/Processor.cpp
    src/core/ScanlineHelper.cpp
    src/core/Transform.cpp
    src/core/XmlIO.cpp

    src/core/tinyxml/tinystr.cpp
    src/core/tinyxml/tinyxml.cpp
    src/core/tinyxml/tinyxmlerror.cpp
    src/core/tinyxml/tinyxmlparser.cpp

    src/core/pystring/pystring.cpp
)

include_directories(
    ${OpenColorSpace_SOURCE_DIR}/export/
)

add_executable(OpenColorSpaceTest
    src/testbed/main.cpp
)

target_link_libraries(OpenColorSpaceTest OpenColorSpace)

# http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wshadow -Winline -Wconversion -Wcast-qual -Wformat=2")

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Weffc++")

option(USE_SSE "Set to OFF to disable SSE optimizations." ON)
if(USE_SSE)
    message(STATUS "SSE Runtime: ENABLED")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
    add_definitions("-DUSE_SSE")
else()
    message(STATUS "SSE Runtime: DISABLED")
endif()





###############################################################################
#
# NUKE
#

set(NUKE "/net/apps/rhel40m64/foundry/nuke6.0.5/include" CACHE FILEPATH "path to Nuke headers against which to compile Nuke plugins")
set(NUKE_HEADER DDImage/Op.h)
if(EXISTS "${NUKE}/${NUKE_HEADER}")
    message(STATUS "Nuke path okay, time to set up for building Nuke plugins")

    add_library(NukeColorSpaceConversion MODULE
        src/nuke/ColorSpaceConversion/ColorSpaceConversion.cpp
    )
    include_directories(
        src/nuke/ColorSpaceConversion/
        ${NUKE}
    )
    target_link_libraries(NukeColorSpaceConversion OpenColorSpace)

    add_custom_target(Nuke
        DEPENDS NukeColorSpaceConversion
    )
else()
    message(STATUS "Nuke plugins will not be built: ${NUKE_HEADER} not found in ${NUKE}")
endif()





###############################################################################
#
# PYTHON
#

set(PYVER 2.6)
set(PYHELP "path to Python headers against which to compile Python bindings")
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(PYTHON "/System/Library/Frameworks/Python.framework/Versions/${PYVER}/include/python${PYVER}/" CACHE FILEPATH ${PYHELP})
else()
    set(PYTHON "/usr/include/python${PYVER}/" CACHE FILEPATH ${PYHELP})
endif()

set(PYTHON_HEADER Python.h)
if(EXISTS "${PYTHON}/Python.h")
    message(STATUS "Python path okay, time to set up for building the Python bindings")

    add_library(PyOpenColorSpace MODULE
        src/pyglue/PyColorSpace.cpp
        src/pyglue/PyConfig.cpp
        src/pyglue/PyFileTransform.cpp
        src/pyglue/PyGroupTransform.cpp
        src/pyglue/PyMain.cpp
        src/pyglue/PyUtil.cpp
    )
    include_directories(
        ${PYTHON}
    )
    # Exclude the 'lib' prefix from the name.
    set_property(TARGET PyOpenColorSpace
        PROPERTY PREFIX ""
    )
    target_link_libraries(PyOpenColorSpace OpenColorSpace)

    if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -undefined dynamic_lookup")
    endif()

    add_custom_target(PyOpenColorSpaceTest
        COMMAND env PYTHONPATH=${OpenColorSpace_LIBRARY_OUTPUT_DIRECTORY} python ${OpenColorSpace_SOURCE_DIR}/src/testbed/main.py
        DEPENDS PyOpenColorSpace
        SOURCES src/testbed/main.py
        VERBATIM
    )
else()
    message(STATUS "Python bindings will not be built: ${PYTHON_HEADER} not found in ${PYTHON}")
endif()


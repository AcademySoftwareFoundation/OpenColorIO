cmake_minimum_required(VERSION 2.6)
project(OpenColorIO)

enable_language(CXX)

if(APPLE)
#    set(CMAKE_OSX_ARCHITECTURES ppc;i386;ppc64;x86_64)
    set(CMAKE_OSX_ARCHITECTURES i386;x86_64)
endif()

###############################################################################
#
# CORE
#

add_library(OpenColorIO SHARED
    src/core/CDLTransform.cpp
    src/core/ColorSpace.cpp
    src/core/ColorSpaceTransform.cpp
    src/core/Config.cpp
    src/core/DisplayTransform.cpp
    src/core/Exception.cpp
    src/core/ExponentOps.cpp
    src/core/FileFormat3DL.cpp
    src/core/FileFormatSpi1D.cpp
    src/core/FileFormatSpi3D.cpp
    src/core/FileFormatSpiMtx.cpp
    src/core/FileTransform.cpp
    src/core/GroupTransform.cpp
    src/core/GpuAllocationNoOp.cpp
    src/core/GpuShaderDesc.cpp
    src/core/GpuShaderUtils.cpp
    src/core/HashUtils.cpp
    src/core/ImageDesc.cpp
    src/core/ImagePacking.cpp
    src/core/LogOps.cpp
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

    src/core/md5/md5.cpp
    
    src/core/pystring/pystring.cpp
    
    src/core/tinyxml/tinystr.cpp
    src/core/tinyxml/tinyxml.cpp
    src/core/tinyxml/tinyxmlerror.cpp
    src/core/tinyxml/tinyxmlparser.cpp
)

include_directories(
    ${OpenColorIO_SOURCE_DIR}/export/
)

add_executable(OpenColorIOTest
    src/testbed/main.cpp
)

target_link_libraries(OpenColorIOTest OpenColorIO)

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
    message(STATUS "Nuke path okay, will build the Nuke plugins against ${NUKE}")

    add_library(NukeOCIOColorSpace MODULE
        src/nuke/ColorSpace/ColorSpace.cpp
    )
    include_directories(
        src/nuke/ColorSpace/
        ${NUKE}
    )
    target_link_libraries(NukeOCIOColorSpace OpenColorIO)

    add_library(NukeOCIODisplay MODULE
        src/nuke/Display/Display.cpp
    )
    include_directories(
        src/nuke/Display/
        ${NUKE}
    )
    target_link_libraries(NukeOCIODisplay OpenColorIO)

    if(APPLE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -undefined dynamic_lookup")
    endif()

    add_custom_target(Nuke
        DEPENDS NukeOCIOColorSpace
        DEPENDS NukeOCIODisplay
    )
else()
    message(STATUS "Nuke plugins will not be built: ${NUKE_HEADER} not found in ${NUKE}")
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


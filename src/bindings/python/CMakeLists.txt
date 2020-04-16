# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

###############################################################################
# External dependencies

find_package(PythonInterp 2.7 REQUIRED)
find_package(PythonLibs 2.7 REQUIRED)

# pybind11
# https://github.com/pybind/pybind11
set(pybind11_MIN_VERSION 2.4.3)
find_package(pybind11 ${pybind11_MIN_VERSION} REQUIRED)

###############################################################################
# PyDoc generation
# file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include")
# set(PYDOC_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/include/PyDoc.h")
# add_custom_command(OUTPUT ${PYDOC_OUTPUT}
# 	COMMAND ${PYTHON_EXECUTABLE} createPyDocH.py ${PYDOC_OUTPUT}
# 	WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
# )

###############################################################################
# Python libs

set(SOURCES
	PyOpenColorIO.cpp
	PyUtils.cpp
	PyTypes.cpp
	PyFormatMetadata.cpp
	PyDynamicProperty.cpp
	PyTransform.cpp
	PyAllocationTransform.cpp
	PyCDLTransform.cpp
	PyColorSpaceTransform.cpp
	PyDisplayTransform.cpp
	PyExponentTransform.cpp
	PyExponentWithLinearTransform.cpp
	PyExposureContrastTransform.cpp
	PyFileTransform.cpp
	PyFixedFunctionTransform.cpp
	PyGroupTransform.cpp
	PyLogAffineTransform.cpp
	PyLogCameraTransform.cpp
	PyLogTransform.cpp
	PyLookTransform.cpp
	PyLut1DTransform.cpp
	PyLut3DTransform.cpp
	PyMatrixTransform.cpp
	PyRangeTransform.cpp
	PyConfig.cpp
	PyFileRules.cpp
	PyColorSpace.cpp
	PyColorSpaceSet.cpp
	PyLook.cpp
	PyViewTransform.cpp
	PyProcessor.cpp
	PyCPUProcessor.cpp
	PyGPUProcessor.cpp
	PyProcessorMetadata.cpp
	PyBaker.cpp
	PyImageDesc.cpp
	PyPackedImageDesc.cpp
	PyPlanarImageDesc.cpp
	PyGpuShaderCreator.cpp
	PyGpuShaderDesc.cpp
	PyContext.cpp
)

add_library(PyOpenColorIO MODULE ${SOURCES} ${PYDOC_OUTPUT})
set_target_properties(PyOpenColorIO PROPERTIES
	PREFIX ""
)

if(WIN32)
	# Windows uses .pyd extension for python modules
	set_target_properties(PyOpenColorIO PROPERTIES
		SUFFIX ".pyd"
	)
endif()

set_target_properties(PyOpenColorIO PROPERTIES
	COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

if(NOT BUILD_SHARED_LIBS)
	target_compile_definitions(PyOpenColorIO
		PRIVATE
			OpenColorIO_SKIP_IMPORTS
	)
endif()

if(UNIX)
	# A 'module' is a dynamic library on Linux (i.e. '-fPIC' needed),
	# but a static library on Windows.

	# If supported for the target machine, emit position-independent code
	# suitable for dynamic linking.
	set_property(TARGET PyOpenColorIO PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

# OSX demands that the linker resolve all symbols at build time
# we pass this flag to allow dynamic linking
if(APPLE)
	set_target_properties(PyOpenColorIO PROPERTIES
		LINK_FLAGS "-undefined dynamic_lookup"
	)
endif()

target_include_directories(PyOpenColorIO
	PRIVATE
		PyOpenColorIO
		${CMAKE_CURRENT_BINARY_DIR}/include
		${CMAKE_CURRENT_SOURCE_DIR}
		${pybind11_INCLUDE_DIR}
	SYSTEM
		${PYTHON_INCLUDE_DIRS}
)

# pybind11 recommends intentionally NOT linking against libpython on Linux and 
# macOS, to prevent segfaults when potentially working with multiple Python 
# installations. See note in:
#   https://pybind11.readthedocs.io/en/stable/compiling.html#building-manually
set(PYOCIO_LINK_PUBLIC public_api)
if(WIN32)
	list(INSERT PYOCIO_LINK_PUBLIC 0 ${PYTHON_LIBRARIES})
endif()

target_link_libraries(PyOpenColorIO
	PUBLIC
		${PYOCIO_LINK_PUBLIC}
	PRIVATE
		OpenColorIO
		utils::strings
		pybind11::module
)

target_compile_definitions(PyOpenColorIO
	PRIVATE
		PYOCIO_NAME=PyOpenColorIO
)

if(WIN32)
    set(PYTHON_VARIANT_PATH "lib${LIB_SUFFIX}/site-packages")
else()
    set(PYTHON_VARIANT_PATH "lib${LIB_SUFFIX}/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages")
endif()

install(TARGETS PyOpenColorIO
    LIBRARY DESTINATION ${PYTHON_VARIANT_PATH}
)

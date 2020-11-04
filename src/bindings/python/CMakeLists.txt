# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

if(OCIO_BUILD_DOCS)

    ###########################################################################
    ### Setup PYTHONPATH ###

    include(GetPythonPreCommand)

	# Sets Python_PRE_CMD, which sets PATH and PYTHONPATH for Python
    get_python_pre_command()

	###########################################################################
	### Extract docstrings from C++ headers for Python bindings ###

	set(PYOCIO_DOCSTRINGS_DIR "${CMAKE_BINARY_DIR}/docs/_doxygen")
	set(PYOCIO_DOCSTRINGS_H "${PYOCIO_DOCSTRINGS_DIR}/docstrings.h")
    set(DOXYGEN_INDEX_XML "${PYOCIO_DOCSTRINGS_DIR}/xml/index.xml")
	set(EXTRACT_DOCSTRINGS_PY "${CMAKE_SOURCE_DIR}/share/docs/extract_docstrings.py")

	# Run docstring extraction if docstrings.h is behind doxygen XML
	add_custom_command(OUTPUT ${PYOCIO_DOCSTRINGS_H}
		COMMAND
			${Python_PRE_CMD} "${Python_EXECUTABLE}" "${EXTRACT_DOCSTRINGS_PY}" xml docstrings.h
		WORKING_DIRECTORY
			${PYOCIO_DOCSTRINGS_DIR}
		DEPENDS
			doxygen_extraction
            ${DOXYGEN_INDEX_XML}
			${EXTRACT_DOCSTRINGS_PY}
		COMMENT "Extracting Python docstrings from C++ headers"
	)

	add_custom_target(docstring_extraction
		DEPENDS ${PYOCIO_DOCSTRINGS_H}
	)

else()

	# Use stand-in docstrings.h file when docs are not built
	set(PYOCIO_DOCSTRINGS_DIR "${CMAKE_SOURCE_DIR}/share/docs")

	message(WARNING 
		"Building PyOpenColorIO with OCIO_BUILD_DOCS disabled will result in "
		"incomplete Python docstrings."
	)

endif()

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
	PyBuiltinTransform.cpp
	PyBuiltinTransformRegistry.cpp
	PyCDLTransform.cpp
	PyColorSpaceTransform.cpp
	PyDisplayViewTransform.cpp
	PyExponentTransform.cpp
	PyExponentWithLinearTransform.cpp
	PyExposureContrastTransform.cpp
	PyFileTransform.cpp
	PyFixedFunctionTransform.cpp
	PyGradingData.cpp
	PyGradingPrimaryTransform.cpp
	PyGradingRGBCurveTransform.cpp
	PyGradingToneTransform.cpp
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
	PySystemMonitors.cpp
	PyViewingRules.cpp
)

add_library(PyOpenColorIO MODULE ${SOURCES})

if(OCIO_BUILD_DOCS)
	add_dependencies(PyOpenColorIO
		docstring_extraction
	)
endif()

set_target_properties(PyOpenColorIO PROPERTIES
	PREFIX ""
)

if(WIN32)
	# Windows uses .pyd extension for python modules
	set_target_properties(PyOpenColorIO PROPERTIES
		SUFFIX ".pyd"
	)
endif()

# NOTE: Depending of the compiler version pybind11 2.4.3 does not compile with C++17 so revert to c++11

set(APP_CXX_STANDARD ${CMAKE_CXX_STANDARD})
if(${CMAKE_CXX_STANDARD} GREATER_EQUAL 17)
	set(APP_CXX_STANDARD 11)
endif()

set_target_properties(PyOpenColorIO
	PROPERTIES
		COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}"
		CXX_STANDARD ${APP_CXX_STANDARD}
)

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
		${PYOCIO_DOCSTRINGS_DIR}
	SYSTEM
		${Python_INCLUDE_DIRS}
)

# pybind11 recommends intentionally NOT linking against libpython on Linux and 
# macOS, to prevent segfaults when potentially working with multiple Python 
# installations. See note in:
#   https://pybind11.readthedocs.io/en/stable/compiling.html#building-manually
set(PYOCIO_LINK_PUBLIC public_api)
if(WIN32)
	list(INSERT PYOCIO_LINK_PUBLIC 0 ${Python_LIBRARIES})
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
		PY_VERSION_MAJOR=${Python_VERSION_MAJOR}
		PY_VERSION_MINOR=${Python_VERSION_MINOR}
		PY_VERSION_PATCH=${Python_VERSION_PATCH}
)

if(WIN32)
    set(_Python_VARIANT_PATH "lib${LIB_SUFFIX}/site-packages")
else()
    set(_Python_VARIANT_PATH "lib${LIB_SUFFIX}/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages")
endif()

install(TARGETS PyOpenColorIO
    LIBRARY DESTINATION ${_Python_VARIANT_PATH}
)

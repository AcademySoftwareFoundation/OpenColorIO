# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
        PyOpenColorIO_oglapphelpers.cpp
        PyOpenColorIO_glsl.cpp
        PyOpenColorIO_oglapp.cpp
)

add_library(PyOpenColorIO_oglapphelpers MODULE ${SOURCES})
set_target_properties(PyOpenColorIO_oglapphelpers PROPERTIES
	PREFIX ""
)

if(WIN32)
	# Windows uses .pyd extension for python modules
        set_target_properties(PyOpenColorIO_oglapphelpers PROPERTIES
		SUFFIX ".pyd"
	)
endif()

set_target_properties(PyOpenColorIO_oglapphelpers PROPERTIES
        COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

if(NOT BUILD_SHARED_LIBS)
        target_compile_definitions(PyOpenColorIO_oglapphelpers
		PRIVATE
			OpenColorIO_SKIP_IMPORTS
	)
endif()

if(UNIX)
	# A 'module' is a dynamic library on Linux (i.e. '-fPIC' needed),
	# but a static library on Windows.

	# If supported for the target machine, emit position-independent code
	# suitable for dynamic linking.
        set_property(TARGET PyOpenColorIO_oglapphelpers PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

# OSX demands that the linker resolve all symbols at build time
# we pass this flag to allow dynamic linking
if(APPLE)
        set_target_properties(PyOpenColorIO_oglapphelpers PROPERTIES
		LINK_FLAGS "-undefined dynamic_lookup"
	)
endif()

target_include_directories(PyOpenColorIO_oglapphelpers
	PRIVATE
		PyOpenColorIO
		${CMAKE_CURRENT_BINARY_DIR}/include
		${CMAKE_CURRENT_SOURCE_DIR}
		${pybind11_INCLUDE_DIR}
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

target_link_libraries(PyOpenColorIO_oglapphelpers
	PUBLIC
		${PYOCIO_LINK_PUBLIC}
	PRIVATE
		OpenColorIO
                oglapphelpers
		utils::strings
		pybind11::module
)

target_compile_definitions(PyOpenColorIO_oglapphelpers
	PRIVATE
                PYOCIO_NAME=PyOpenColorIO_oglapphelpers
)

if(WIN32)
    set(_Python_VARIANT_PATH "lib${LIB_SUFFIX}/site-packages")
else()
    set(_Python_VARIANT_PATH "lib${LIB_SUFFIX}/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages")
endif()

install(TARGETS PyOpenColorIO_oglapphelpers
    LIBRARY DESTINATION ${_Python_VARIANT_PATH}
)

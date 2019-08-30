# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

find_package(PythonInterp 2.7 REQUIRED)
find_package(PythonLibs 2.7 REQUIRED)
find_package(Pybind11 2.3.0 REQUIRED)

set(SOURCES
	PyOpenColorIO.cpp
	PyStatic.cpp
	PyTransform.cpp
)

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
	SYSTEM
		${PYTHON_INCLUDE_DIRS}
		${PYBIND11_INCLUDE_DIR}
)
target_link_libraries(PyOpenColorIO 
	PUBLIC
		${PYTHON_LIBRARIES}
		OpenColorIO
	PRIVATE
		pybind11::pybind11
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
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/PyOpenColorIO/PyOpenColorIO.h
    DESTINATION include/OpenColorIO/PyOpenColorIO
)

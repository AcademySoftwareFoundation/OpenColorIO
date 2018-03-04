find_package(PythonLibs REQUIRED)
find_package(PythonInterp REQUIRED)


set(SOURCES
	PyAllocationTransform.cpp
	PyBaker.cpp
	PyCDLTransform.cpp
	PyColorSpace.cpp
	PyColorSpaceTransform.cpp
	PyConfig.cpp
	PyConstants.cpp
	PyContext.cpp
	PyDisplayTransform.cpp
	PyExponentTransform.cpp
	PyFileTransform.cpp
	PyGpuShaderDesc.cpp
	PyGroupTransform.cpp
	PyLogTransform.cpp
	PyLook.cpp
	PyLookTransform.cpp
	PyMain.cpp
	PyMatrixTransform.cpp
	PyProcessor.cpp
	PyProcessorMetadata.cpp
	PyTransform.cpp
	PyUtil.cpp
)

###############################################################################
# PyDoc generation
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include")
set(PYDOC_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/include/PyDoc.h")
add_custom_command(OUTPUT ${PYDOC_OUTPUT}
	COMMAND ${PYTHON_EXECUTABLE} createPyDocH.py ${PYDOC_OUTPUT}
	WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

###############################################################################
# Python libs

add_library(PyOpenColorIO ${SOURCES} ${PYDOC_OUTPUT})
set_target_properties(PyOpenColorIO PROPERTIES
	PREFIX ""
)
target_include_directories(PyOpenColorIO
	PRIVATE
		PyOpenColorIO
		${CMAKE_CURRENT_BINARY_DIR}/include
		${CMAKE_CURRENT_SOURCE_DIR}
	SYSTEM
		${PYTHON_INCLUDE_DIRS}
)
target_link_libraries(PyOpenColorIO 
	PUBLIC
		${PYTHON_LIBRARIES}
		OpenColorIO
)


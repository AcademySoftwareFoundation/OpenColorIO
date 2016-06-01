if(CMAKE_FIRST_RUN)
    message(STATUS "Python library to include 'lib' prefix: ${OCIO_PYGLUE_LIB_PREFIX}")
endif()


if(CMAKE_FIRST_RUN)
    message(STATUS "Python ${PYTHON_VERSION} okay (UCS: ${PYTHON_UCS}), will build the Python bindings against ${PYTHON_INCLUDE}")
    message(STATUS "Python variant path is ${PYTHON_VARIANT_PATH}")
endif()
    
if(CMAKE_COMPILER_IS_GNUCXX)
    # Python breaks strict-aliasing rules. Disable the warning here.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-strict-aliasing -Wno-missing-field-initializers")
endif()

if(NOT WIN32)
    find_package(PythonLibs)
    if(NOT PYTHONLIBS_FOUND)
        message(FATAL "Python libraries were not found, exiting.")
    endif()
    SET(PYTHON_INCLUDES ${PYTHON_INCLUDE_DIRS})
endif()

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/src/pyglue/PyDoc.h
                  COMMAND ${PYTHON} ${CMAKE_SOURCE_DIR}/src/pyglue/createPyDocH.py ${CMAKE_BINARY_DIR}/src/pyglue/PyDoc.h
                  COMMENT "Creating PyDoc.h")

file( GLOB pyglue_src_files "${CMAKE_SOURCE_DIR}/src/pyglue/*.cpp" )

add_library(PyOpenColorIO MODULE ${pyglue_src_files} ${CMAKE_BINARY_DIR}/src/pyglue/PyDoc.h)

add_definitions(-DPYOCIO_NAME=PyOpenColorIO)

if(OCIO_USE_BOOST_PTR)
    include_directories(
        ${PYTHON_INCLUDE}
        ${Boost_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/export/
        ${CMAKE_BINARY_DIR}/export/
        ${CMAKE_CURRENT_BINARY_DIR}
    )
else()
    include_directories(
        ${PYTHON_INCLUDE}
        ${CMAKE_SOURCE_DIR}/export/
        ${CMAKE_BINARY_DIR}/export/
        ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()

# Exclude the 'lib' prefix from the name.
if(NOT OCIO_PYGLUE_LIB_PREFIX)
    set_property(TARGET PyOpenColorIO
        PROPERTY PREFIX ""
    )
endif()

if(WIN32)
    if(OCIO_PYGLUE_LINK)
    target_link_libraries(PyOpenColorIO OpenColorIO
        debug ${PYTHON_LIB}/python26_d.lib
        optimized ${PYTHON_LIB}/python26.lib)
    else()
    target_link_libraries(PyOpenColorIO OpenColorIO)
    endif(OCIO_PYGLUE_LINK)
else()
    if(OCIO_PYGLUE_LINK)
        message(STATUS "Linking python bindings against: ${PYTHON_LIBRARY}")
        message(STATUS "Python library dirs: ${PYTHON_LIBRARY_DIRS}")
        link_directories(${PYTHON_LIBRARY})
        target_link_libraries(PyOpenColorIO OpenColorIO
            ${PYTHON_LIBRARIES})
    else()
        target_link_libraries(PyOpenColorIO OpenColorIO)
    endif(OCIO_PYGLUE_LINK)
endif()

# PyOpenColorIO so serves dual roles:
# - First, to provide the C API function necessary to act as a cpython module, 
#   (extern "C" PyMODINIT_FUNC initPyOpenColorIO(void)
# - Second, to act as a normal shared library, providing the C++ API functions 
#   to convert between C++ and python representation of the OCIO object.
#
# To fulfill this second role, as a shared libary, we must continue to define
# so version.
#
# TODO: This wont let the normal shared library symbols work on OSX.
# We should explore whether this should be built as a normal dylib, instead
# of as a bundle. See https://github.com/imageworks/OpenColorIO/pull/175

if(OCIO_PYGLUE_SONAME)
    message(STATUS "Setting PyOCIO SOVERSION to: ${SOVERSION}")
    set_target_properties(PyOpenColorIO PROPERTIES
        VERSION ${OCIO_VERSION}
        SOVERSION ${SOVERSION}
    )
endif()

if(APPLE)
    set_target_properties(PyOpenColorIO PROPERTIES
    	LINK_FLAGS "-undefined dynamic_lookup")
endif()

add_subdirectory(tests)

message("PYTHON_VARIANT_PATH: ${PYTHON_VARIANT_PATH}")

install(TARGETS PyOpenColorIO DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/${PYTHON_VARIANT_PATH})

install(FILES ${CMAKE_SOURCE_DIR}/export/PyOpenColorIO/PyOpenColorIO.h
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/PyOpenColorIO/)

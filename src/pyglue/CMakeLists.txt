# Set the default built type
if(NOT PYTHON_INCLUDE_LIB_PREFIX)
  set(PYTHON_INCLUDE_LIB_PREFIX NO CACHE BOOL
      "Specify whether the python library should be prefixed with 'lib'."
      FORCE)
endif(NOT PYTHON_INCLUDE_LIB_PREFIX)

if(CMAKE_FIRST_RUN)
    message(STATUS "Python library to include 'lib' prefix: ${PYTHON_INCLUDE_LIB_PREFIX}")
endif()

if(CMAKE_FIRST_RUN)
    message(STATUS "Python ${PYTHON_VERSION} okay (UCS: ${PYTHON_UCS}), will build the Python bindings against ${PYTHON_INCLUDE}")
endif()
    
if(CMAKE_COMPILER_IS_GNUCXX)
    # Python breaks strict-aliasing rules. Disable the warning here.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-strict-aliasing")
endif()

file( GLOB pyglue_src_files "${CMAKE_SOURCE_DIR}/src/pyglue/*.cpp" )

add_library(PyOpenColorIO MODULE ${pyglue_src_files} )
if(OCIO_USE_BOOST_PTR)
    include_directories(
        ${PYTHON_INCLUDE}
        ${Boost_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/export/
        ${CMAKE_BINARY_DIR}/export/
    )
else()
    include_directories(
        ${PYTHON_INCLUDE}
        ${CMAKE_SOURCE_DIR}/export/
        ${CMAKE_BINARY_DIR}/export/
    )
endif()

# Exclude the 'lib' prefix from the name.
if(NOT PYTHON_INCLUDE_LIB_PREFIX)
    set_property(TARGET PyOpenColorIO
        PROPERTY PREFIX ""
    )
endif()

if(WIN32)
    target_link_libraries(PyOpenColorIO OpenColorIO
        debug ${PYTHON_LIB}/python26_d.lib
        optimized ${PYTHON_LIB}/python26.lib)
else()
    target_link_libraries(PyOpenColorIO OpenColorIO)
endif()

if (SOVERSION)
message(STATUS "Setting PyOCIO SOVERSION to: ${SOVERSION}")
set_target_properties(PyOpenColorIO PROPERTIES
    VERSION ${OCIO_VERSION}
    SOVERSION ${SOVERSION}
)
endif ()


if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -undefined dynamic_lookup")
endif()


if(PYTHON_RESPECT_UCS)
    # Respect Python UCS (unicode content system) version, and install
    # into "lib/python2.6/ucs4" or similar.
    set(PYTHON_VARIANT_PATH "python${PYTHON_VERSION}/${PYTHON_UCS}")
else()
    # Ignore UCS version and install into lib/python2.6
    set(PYTHON_VARIANT_PATH "python${PYTHON_VERSION}")
endif()

message("PYTHON_VARIANT_PATH: ${PYTHON_VARIANT_PATH}")

install(TARGETS PyOpenColorIO DESTINATION ${CMAKE_INSTALL_EXEC_PREFIX}/lib/${PYTHON_VARIANT_PATH})

install(FILES ${CMAKE_SOURCE_DIR}/export/PyOpenColorIO/PyOpenColorIO.h
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/PyOpenColorIO/)

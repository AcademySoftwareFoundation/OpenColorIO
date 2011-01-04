# Set the default built type
if(NOT PYTHON_INCLUDE_LIB_PREFIX)
  set(PYTHON_INCLUDE_LIB_PREFIX NO CACHE BOOL
      "Specify whether the python library should be prefixed with 'lib'."
      FORCE)
endif(NOT PYTHON_INCLUDE_LIB_PREFIX)

if(CMAKE_FIRST_RUN)
    message(STATUS "Python library to include 'lib' prefix: ${PYTHON_INCLUDE_LIB_PREFIX}")
endif()

if(PYTHON_OK)
    
    if(CMAKE_FIRST_RUN)
        message(STATUS "Python ${PYTHON_VERSION} okay, will build the Python bindings against ${PYTHON_INCLUDE}")
    endif()
    
    # Python breaks strict-aliasing rules. Disable the warning here.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-strict-aliasing")
    
    file( GLOB pyglue_src_files "${CMAKE_SOURCE_DIR}/src/pyglue/*.cpp" )
    
    add_library(PyOpenColorIO MODULE ${pyglue_src_files} )
    include_directories(
        ${PYTHON_INCLUDE}
        ${CMAKE_SOURCE_DIR}/export/
        ${CMAKE_BINARY_DIR}/export/
        ${Boost_INCLUDE_DIR}
    )
    
    # Exclude the 'lib' prefix from the name.
    if(NOT PYTHON_INCLUDE_LIB_PREFIX)
        set_property(TARGET PyOpenColorIO
            PROPERTY PREFIX ""
        )
    endif()
    
    target_link_libraries(PyOpenColorIO OpenColorIO)

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

    add_custom_target(PyOpenColorIOTest
        COMMAND env PYTHONPATH=${OpenColorIO_LIBRARY_OUTPUT_DIRECTORY} ${PYTHON} ${CMAKE_SOURCE_DIR}/src/testbed/main.py
        DEPENDS PyOpenColorIO
        SOURCES ${CMAKE_SOURCE_DIR}/src/testbed/main.py
        VERBATIM
    )
    
    install(TARGETS PyOpenColorIO DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python${PYTHON_VERSION})
    install(FILES ${CMAKE_SOURCE_DIR}/export/PyOpenColorIO/PyOpenColorIO.h
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/PyOpenColorIO/)
    
else()
    message(STATUS "Python bindings will not be built: ${PYTHON_ERR}")
endif()


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
    
    if(CMAKE_FIRST_RUN)
        message(STATUS "Python ${PYTHON_VERSION} okay, will build the Python bindings against ${PYTHON_INCLUDE}")
    endif()
    
    add_library(PyOpenColorIO MODULE
        PyCDLTransform.cpp
        PyColorSpace.cpp
        PyConstants.cpp
        PyConfig.cpp
        PyDisplayTransform.cpp
        PyFileTransform.cpp
        PyGroupTransform.cpp
        PyMain.cpp
        PyTransform.cpp
        PyProcessor.cpp
        PyUtil.cpp
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
        SOURCES ${OpenColorIO_SOURCE_DIR}/src/testbed/main.py
        VERBATIM
    )
    
    install(TARGETS PyOpenColorIO DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python)
    
else()
    message(STATUS "Python bindings will not be built: ${PYTHON_ERR}")
endif()

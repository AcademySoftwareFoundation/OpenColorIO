MACRO(messageonce MSG)
    if(CMAKE_FIRST_RUN)
        message(STATUS ${MSG})
    endif()
ENDMACRO()

MACRO(OCIOFindOpenGL)
    if(APPLE)
        INCLUDE_DIRECTORIES(/System/Library/Frameworks)
    endif()

    # Find OpenGL
    find_package(OpenGL)
    if(OPENGL_FOUND)
        message(STATUS "Found OpenGL library ${OPENGL_LIBRARY}")
        message(STATUS "Found OpenGL includes ${OPENGL_INCLUDE_DIR}")
    else()
        message(STATUS "OpenGL not found")
    endif()

    # Find GLUT
    find_package(GLUT)
    if(GLUT_FOUND)
        message(STATUS "Found GLUT library ${GLUT_LIBRARY}")
    else()
        message(STATUS "GLUT not found")
    endif()

    # Find GLEW
    set(GLEW_VERSION 1.5.1)
    FIND_PATH(GLEW_INCLUDES GL/glew.h
        /usr/include
        /usr/local/include
        /sw/include
        /opt/local/include
        DOC "The directory where GL/glew.h resides")
    FIND_LIBRARY(GLEW_LIBRARIES
        NAMES GLEW glew
        PATHS
        /usr/lib64
        /usr/lib
        /usr/local/lib64
        /usr/local/lib
        /sw/lib
        /opt/local/lib
        DOC "The GLEW library")
    if(GLEW_INCLUDES AND GLEW_LIBRARIES)
        set(GLEW_FOUND TRUE)
        message(STATUS "Found GLEW library ${GLEW_LIBRARIES}")
        message(STATUS "Found GLEW includes ${GLEW_INCLUDES}")
    else()
        message(STATUS "GLEW not found")
        set(GLEW_FOUND FALSE)
    endif()
ENDMACRO()

MACRO(OCIOFindOpenImageIO)
    if(OIIO_PATH)
        message(STATUS "OIIO path explicitly specified: ${OIIO_PATH}")
    endif()
    if(OIIO_INCLUDE_PATH)
        message(STATUS "OIIO INCLUDE_PATH explicitly specified: ${OIIO_INCLUDE_PATH}")
    endif()
    if(OIIO_LIBRARY_PATH)
        message(STATUS "OIIO LIBRARY_PATH explicitly specified: ${OIIO_LIBRARY_PATH}")
    endif()
    FIND_PATH( OIIO_INCLUDES OpenImageIO/version.h
        ${OIIO_INCLUDE_PATH}
        ${OIIO_PATH}/include/
        /usr/include
        /usr/local/include
        /sw/include
        /opt/local/include
        DOC "The directory where OpenImageIO/version.h resides")
    FIND_LIBRARY(OIIO_LIBRARIES
        NAMES OIIO OpenImageIO
        PATHS
        ${OIIO_LIBRARY_PATH}
        ${OIIO_PATH}/lib/
        /usr/lib64
        /usr/lib
        /usr/local/lib64
        /usr/local/lib
        /sw/lib
        /opt/local/lib
        DOC "The OIIO library")

    if(OIIO_INCLUDES AND OIIO_LIBRARIES)
        set(OIIO_FOUND TRUE)
        message(STATUS "Found OIIO library ${OIIO_LIBRARIES}")
        message(STATUS "Found OIIO includes ${OIIO_INCLUDES}")
    else()
        set(OIIO_FOUND FALSE)
        message(STATUS "OIIO not found. Specify OIIO_PATH to locate it")
    endif()
ENDMACRO()

MACRO(OCIOFindPython)
    # Set the default python runtime
    if(NOT PYTHON)
        set(PYHELP "path to Python executable (also used to find headers against which to compile Python bindings)")
        set(PYTHON_HEADER Python.h)
        set(PYTHON python CACHE FILEPATH ${PYHELP} FORCE)
    endif(NOT PYTHON)
    
    if(CMAKE_FIRST_RUN)
        message(STATUS "Setting python bin to: ${PYTHON}")
    endif()
    
    execute_process(COMMAND ${PYTHON} -c "from distutils import sysconfig; print ':'.join(set(sysconfig.get_config_var('INCLDIRSTOMAKE').split()))"
        OUTPUT_VARIABLE PYTHON_INCLUDE_RAW
        RESULT_VARIABLE PYTHON_RETURNVALUE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(PYTHON_OK NO)
    
    if(${PYTHON_RETURNVALUE} EQUAL 0)
        file(TO_CMAKE_PATH "${PYTHON_INCLUDE_RAW}" PYTHON_INCLUDE)
        execute_process(COMMAND ${PYTHON} -c "from distutils import sysconfig; print sysconfig.get_python_version()"
            OUTPUT_VARIABLE PYTHON_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(PYTHON_OK YES)
    elseif(${PYTHON_RETURNVALUE} GREATER 0)
        set(PYTHON_ERR "${PYTHON} returned ${PYTHON_RETURNVALUE} trying to determine header location.")
    else()
        set(PYTHON_ERR "${PYTHON}: ${PYTHON_RETURNVALUE}.")
    endif()
ENDMACRO()

MACRO(ExtractRst INFILE OUTFILE)
   add_custom_command(
      OUTPUT ${OUTFILE}
      COMMAND ${CMAKE_SOURCE_DIR}/share/sphinx/ExtractRstFromSource.py ${INFILE} ${OUTFILE}
      DEPENDS ${INFILE}
      COMMENT "Extracting reStructuredText from ${INFILE}"
   )
ENDMACRO()


MACRO(CopyFiles TARGET)
    # parse the macro arguments
    PARSE_ARGUMENTS(COPYFILES
        "OUTDIR;"
        "NOTUSED" ${ARGN})
    
    # get the list of sources from the args
    set(FILES ${COPYFILES_DEFAULT_ARGS})
    
    # find the shader compiler
    if(NOT COPYFILES_OUTDIR)
        set(COPYFILES_OUTDIR ${CMAKE_CURRENT_BINARY_DIR})
    endif()
    
    set(${TARGET}_OUTPUT)
    foreach(FILE ${FILES})
        get_filename_component(_FILE_NAME ${FILE} NAME)
        set(OUTFILENAME "${COPYFILES_OUTDIR}/${_FILE_NAME}")
        list(APPEND ${TARGET}_OUTPUT ${OUTFILENAME})
        add_custom_command(
            OUTPUT ${OUTFILENAME}
            COMMAND ${CMAKE_COMMAND} -E copy ${FILE} ${OUTFILENAME}
            DEPENDS ${FILE}
            COMMENT "Copying file ${FILE}\n    to ${OUTFILENAME}"
        )
    endforeach()
ENDMACRO()

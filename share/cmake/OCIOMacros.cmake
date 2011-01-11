
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
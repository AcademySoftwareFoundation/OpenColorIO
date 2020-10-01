# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

###############################################################################
### Python ###

if(${Python_VERSION_MAJOR} GREATER_EQUAL 3)
    if(NOT OCIO_BUILD_PYTHON)
        message(FATAL_ERROR "Doc generation requires that the python bindings be built")
    endif()

    ###############################################################################
    ### Required dependencies ###

    # Doxygen
    # https://github.com/doxygen/doxygen
    find_package(Doxygen REQUIRED)

    # Sphinx
    # https://pypi.python.org/pypi/Sphinx
    find_package(Sphinx REQUIRED)

    include(FindPythonPackage)

    # testresources
    # https://pypi.org/project/testresources/
    find_python_package(testresources REQUIRED)

    # Recommonmark
    # https://pypi.org/project/recommonmark/
    find_python_package(recommonmark REQUIRED)

    # Sphinx Press Theme
    # https://pypi.org/project/sphinx-press-theme/
    find_python_package(sphinx-press-theme REQUIRED)

    # Breathe
    # https://pypi.org/project/breathe/
    find_python_package(breathe REQUIRED)

    ###############################################################################
    ### Setup PYTHONPATH ###

    # Override PYTHONPATH prior to running Sphinx. This makes the PyOpenColorIO
    # build available, prioritizes ext/ installed Python dependencies, and
    # propagates the system PYTHONPATH.
    if(WIN32)
        # Use Windows path separators since this is being passed through to cmd
        file(TO_NATIVE_PATH ${CMAKE_BINARY_DIR} WIN_BINARY_DIR)

        set(DLL_PATH "${WIN_BINARY_DIR}\\src\\OpenColorIO")
        if(MSVC_IDE)
            set(DLL_PATH "${DLL_PATH}\\${CMAKE_BUILD_TYPE}")
        endif()

        string(CONCAT PATH_SET
            "PATH="
            "${DLL_PATH}"
            "\\\\;"
            "%PATH%"
        )

        set(PYD_PATH "${WIN_BINARY_DIR}\\src\\bindings\\python")
        if(MSVC_IDE)
            set(PYD_PATH "${PYD_PATH}\\${CMAKE_BUILD_TYPE}")
        endif()

        string(CONCAT PYTHONPATH_SET
            "PYTHONPATH="
            "${PYD_PATH}"
            "\\\\;"
            "${WIN_BINARY_DIR}\\ext\\dist\\lib${LIB_SUFFIX}\\site-packages"
            "\\\\;"
            "%PYTHONPATH%"
        )
        # Mimic *nix:
        #   '> PYTHONPATH=XXX CMD'
        # on Windows with:
        #   '> set PYTHONPATH=XXX \n call CMD'
        # '\n' is here because '\\&' does not work.
        set(PYT_PRE_CMD set ${PATH_SET} "\n" set ${PYTHONPATH_SET} "\n" call)

    else()
        set(_Python_VARIANT "${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}")
        string(CONCAT PYT_PRE_CMD
            "PYTHONPATH="
            "${CMAKE_BINARY_DIR}/src/bindings/python"
            ":"
            "${CMAKE_BINARY_DIR}/ext/dist/lib${LIB_SUFFIX}/python${_Python_VARIANT}/site-packages"
            ":"
            "$ENV{PYTHONPATH}"
        )
    endif()

    ###############################################################################
    ### Copy templates to build area ###

    # NOTE: The *.in extension is omitted from conf.py since RTD uses this file 
    #       without CMake configuration.
    message(STATUS "Substitute sphinx conf.py variables")
    configure_file(${CMAKE_SOURCE_DIR}/docs/conf.py
        ${CMAKE_BINARY_DIR}/docs/conf.py @ONLY)

    # Find and configure all *.in files in documentation
    message(STATUS "Substitute documentation variables")
    file(GLOB_RECURSE DOC_IN_FILES "${CMAKE_SOURCE_DIR}/docs/**.in")
    foreach(in_file ${DOC_IN_FILES})
        string(REPLACE "${CMAKE_SOURCE_DIR}/" "${CMAKE_BINARY_DIR}/" 
            out_file ${in_file})
        string(REPLACE ".in" "" 
            out_file ${out_file})
        configure_file(${in_file} ${out_file} @ONLY)
    endforeach()

    file(GLOB_RECURSE DOC_SOURCES "${CMAKE_SOURCE_DIR}/docs/*")
    list(APPEND DOC_SOURCES
        ${CMAKE_SOURCE_DIR}/README.md
        ${CMAKE_SOURCE_DIR}/INSTALL.md
        ${CMAKE_SOURCE_DIR}/CHANGELOG.md
        ${CMAKE_SOURCE_DIR}/LICENSE
    )
    if(OCIO_BUILD_NUKE)
        list(APPEND DOC_SOURCES ${CMAKE_SOURCE_DIR}/share/nuke/ocionuke/viewer.py)
    endif()

    add_custom_target(doc_copy
        COMMENT "Copying doc files to staging area"
    )

    foreach(f ${DOC_SOURCES})
        string(REGEX REPLACE "^${CMAKE_SOURCE_DIR}/" "" relpath ${f})
        string(REGEX REPLACE "[/. ]" "_" tgtname ${relpath})
        string(REGEX MATCH "^docs" IN_DOCS ${relpath})

        set(SRC_PATH ${CMAKE_SOURCE_DIR}/${relpath})
        set(DST_PATH ${CMAKE_BINARY_DIR}/${relpath})
        if(NOT IN_DOCS)
            get_filename_component(F_NAME ${relpath} NAME)
            set(DST_PATH "${CMAKE_BINARY_DIR}/docs/${F_NAME}")
        endif()

        add_custom_command(OUTPUT ${DST_PATH}
            COMMAND ${CMAKE_COMMAND} -E copy ${SRC_PATH} ${DST_PATH}
            DEPENDS ${SRC_PATH}
        )
        add_custom_target("copy_${tgtname}" DEPENDS ${DST_PATH})
        add_dependencies(doc_copy "copy_${tgtname}")
    endforeach()

    ###############################################################################
    ### Extract rst from c++ headers ###

    get_target_property(DOXYGEN_EXECUTABLE Doxygen::doxygen IMPORTED_LOCATION)

    add_custom_target(doxygen_extraction
        COMMAND
            "${DOXYGEN_EXECUTABLE}" "${CMAKE_BINARY_DIR}/docs/Doxyfile"
        DEPENDS
            ${CMAKE_BINARY_DIR}/docs/Doxyfile
        COMMENT "Extracting .xml file from C++ headers"
    )

    ###############################################################################
    ### HTML doc target ###

    add_custom_target(docs ALL
        COMMAND
            ${PYT_PRE_CMD} "${Sphinx_EXECUTABLE}" -b html . "${CMAKE_CURRENT_BINARY_DIR}/build-html"
        DEPENDS
            ${CMAKE_BINARY_DIR}/docs/conf.py
        COMMENT "Building html docs"
    )

    add_dependencies(docs
        OpenColorIO
        PyOpenColorIO
        doc_copy
        doxygen_extraction
    )

    ###############################################################################
    ### Installation ###

    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build-html/
            DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/OpenColorIO/html
            PATTERN .* EXCLUDE
    )

    ###############################################################################
    ### PDF target ###

    find_package(LATEX)
    package_root_message(LATEX)

    if(PDFLATEX_COMPILER)

        add_custom_target(latex
            COMMAND
                ${PYT_PRE_CMD} "${Sphinx_EXECUTABLE}" -b latex . "${CMAKE_CURRENT_BINARY_DIR}/build-latex"
            DEPENDS
                ${CMAKE_BINARY_DIR}/docs/conf.py
            COMMENT "Building latex doc"
        )

        add_dependencies(latex
            OpenColorIO
            PyOpenColorIO
            rst_extraction
            doc_copy
            Sphinx
            sphinx-press-theme
            recommonmark
        )


        add_custom_target(pdf ALL
            COMMAND ${PDFLATEX_COMPILER} OpenColorIO.tex
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build-latex
            COMMENT "Building pdf doc"
        )

        add_dependencies(pdf
            latex
        )

        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/build-latex/OpenColorIO.pdf
                DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/OpenColorIO/)

    endif()

else()
    message(Warning "Skipping local documentation generation, requires Python >= 3.x")
endif()

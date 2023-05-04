# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

###############################################################################
### Python ###

if(${Python_VERSION_MAJOR} GREATER_EQUAL 3)
    if(NOT OCIO_BUILD_PYTHON)
        message(FATAL_ERROR "Doc generation requires that the python bindings be built")
    endif()

    ###########################################################################
    ### Required dependencies ###

    # Doxygen
    # https://github.com/doxygen/doxygen
    find_package(Doxygen REQUIRED)

    # Sphinx
    # https://pypi.python.org/pypi/Sphinx
    find_package(Sphinx REQUIRED)

    include(FindPythonPackage)

    # six
    # https://pypi.org/project/six/
    find_python_package(six REQUIRED)

    # testresources
    # https://pypi.org/project/testresources/
    find_python_package(testresources REQUIRED)

    # Recommonmark
    # https://pypi.org/project/recommonmark/
    find_python_package(recommonmark REQUIRED)

    # Sphinx Press Theme
    # https://pypi.org/project/sphinx-press-theme/
    find_python_package(sphinx-press-theme REQUIRED)

    # sphinx-tabs
    # https://pypi.org/project/sphinx-tabs/
    find_python_package(sphinx-tabs REQUIRED)

    # Breathe
    # https://pypi.org/project/breathe/
    find_python_package(breathe REQUIRED)
    
    ###########################################################################
    ### Setup PYTHONPATH ###

    include(GetPythonPreCommand)

    # Sets Python_PRE_CMD, which sets PATH and PYTHONPATH for Sphinx
    get_python_pre_command()

    ###########################################################################
    ### Copy templates to build area ###

    # Find and configure all *.in files in documentation. conf.py needs 
    # configuration, but omits *.in to work in RTD builds.
    message(STATUS "Substitute documentation variables")
    file(GLOB_RECURSE DOC_IN_FILES 
        "${PROJECT_SOURCE_DIR}/docs/**.in"
        "${PROJECT_SOURCE_DIR}/docs/conf.py"
    )
    foreach(in_file ${DOC_IN_FILES})
        string(REPLACE "${PROJECT_SOURCE_DIR}/" "${PROJECT_BINARY_DIR}/"
            out_file ${in_file})
        string(REPLACE ".in" "" 
            out_file ${out_file})
        configure_file(${in_file} ${out_file} @ONLY)
    endforeach()

    # Copy documentation source to build location
    file(GLOB_RECURSE DOC_SOURCES "${PROJECT_SOURCE_DIR}/docs/*")
    list(APPEND DOC_SOURCES
        ${PROJECT_SOURCE_DIR}/README.md
        ${PROJECT_SOURCE_DIR}/INSTALL.md
        ${PROJECT_SOURCE_DIR}/CHANGELOG.md
        ${PROJECT_SOURCE_DIR}/LICENSE
    )
    if(OCIO_BUILD_NUKE)
        list(APPEND DOC_SOURCES ${PROJECT_SOURCE_DIR}/share/nuke/ocionuke/viewer.py)
    endif()

    # Don't copy configured files. In particular this prevents overwriting conf.py,
    # since it isn't renamed during configuration.
    list(REMOVE_ITEM DOC_SOURCES ${DOC_IN_FILES})

    add_custom_target(doc_copy
        COMMENT "Copying doc files to staging area"
    )

    foreach(f ${DOC_SOURCES})
        string(REGEX REPLACE "^${PROJECT_SOURCE_DIR}/" "" relpath ${f})
        string(REGEX REPLACE "[/. ]" "_" tgtname ${relpath})
        string(REGEX MATCH "^docs" IN_DOCS ${relpath})

        set(SRC_PATH ${PROJECT_SOURCE_DIR}/${relpath})
        set(DST_PATH ${PROJECT_BINARY_DIR}/${relpath})
        if(NOT IN_DOCS)
            get_filename_component(F_NAME ${relpath} NAME)
            set(DST_PATH "${PROJECT_BINARY_DIR}/docs/${F_NAME}")
        endif()

        add_custom_command(OUTPUT ${DST_PATH}
            COMMAND ${CMAKE_COMMAND} -E copy ${SRC_PATH} ${DST_PATH}
            DEPENDS ${SRC_PATH}
        )
        add_custom_target("copy_${tgtname}" DEPENDS ${DST_PATH})
        add_dependencies(doc_copy "copy_${tgtname}")
    endforeach()

    ###########################################################################
    ### Extract XML from C++ headers ###

    get_target_property(DOXYGEN_EXECUTABLE Doxygen::doxygen IMPORTED_LOCATION)

    file(GLOB_RECURSE DOXYGEN_SOURCES 
        "${PROJECT_SOURCE_DIR}/include/*.h"
        "${PROJECT_BINARY_DIR}/include/*.h"
    )
    set(DOXYGEN_INDEX_XML "${PROJECT_BINARY_DIR}/docs/_doxygen/xml/index.xml")

    # Run doxygen if index.xml is behind OCIO headers or doxygen config
    add_custom_command(OUTPUT ${DOXYGEN_INDEX_XML}
        COMMAND "${DOXYGEN_EXECUTABLE}" "${PROJECT_BINARY_DIR}/docs/Doxyfile"
        DEPENDS
            ${PROJECT_BINARY_DIR}/docs/Doxyfile
            ${DOXYGEN_SOURCES}
        COMMENT "Extracting XML files from C++ headers"
    )

    add_custom_target(doxygen_extraction
        DEPENDS ${DOXYGEN_INDEX_XML}
    )

    ###########################################################################
    ### HTML doc target ###

    file(GLOB_RECURSE SPHINX_EXTENSIONS
        "${PROJECT_SOURCE_DIR}/share/docs/*.py"
    )

    # Run Sphinx
    add_custom_target(docs ALL
        COMMAND
            ${Python_PRE_CMD} "${Sphinx_EXECUTABLE}" -b html . "${CMAKE_CURRENT_BINARY_DIR}/build-html"
        DEPENDS
            ${PROJECT_BINARY_DIR}/docs/conf.py
            ${SPHINX_EXTENSIONS}
            ${DOXYGEN_INDEX_XML}
        COMMENT "Building html docs"
    )

    add_dependencies(docs
        OpenColorIO
        PyOpenColorIO
        doc_copy
        doxygen_extraction
    )

    ###########################################################################
    ### Installation ###

    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build-html/
            DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/OpenColorIO/html
            PATTERN .* EXCLUDE
    )

else()
    message(Warning "Skipping local documentation generation, requires Python >= 3.x")
endif()

# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

###############################################################################
### Python ###
find_package(Python REQUIRED COMPONENTS Interpreter)
if(NOT OCIO_BUILD_PYTHON)
    message(FATAL_ERROR "Doc generation requires that the python bindings be built")
endif()

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
        "${WIN_BINARY_DIR}\\ext\\dist\\lib\\site-packages"
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

message(STATUS "Create sphinx conf.py from conf.py.in")
configure_file(conf.py.in ${CMAKE_BINARY_DIR}/docs/conf.py @ONLY)

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

set(EXTRACT_COMMAND "${CMAKE_SOURCE_DIR}/share/sphinx/ExtractRstFromSourceCPP.py")
set(OCIO_HEADER_DIR "${CMAKE_SOURCE_DIR}/include/OpenColorIO")
set(RST_DESTINATION "${CMAKE_BINARY_DIR}/docs/developers/api/")
add_custom_target(rst_extraction
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RST_DESTINATION}
    COMMAND ${Python_EXECUTABLE} ${EXTRACT_COMMAND} ${OCIO_HEADER_DIR}/OpenColorIO.h ${RST_DESTINATION}/OpenColorIO.rst
    COMMAND ${Python_EXECUTABLE} ${EXTRACT_COMMAND} ${OCIO_HEADER_DIR}/OpenColorTransforms.h ${RST_DESTINATION}/OpenColorTransforms.rst
    COMMAND ${Python_EXECUTABLE} ${EXTRACT_COMMAND} ${OCIO_HEADER_DIR}/OpenColorTypes.h ${RST_DESTINATION}/OpenColorTypes.rst
    COMMENT "Extracting .rst files from C++ headers"
)

###############################################################################
### HTML doc target ###

add_custom_target(docs ALL
    COMMAND
        ${PYT_PRE_CMD} "${Sphinx_EXECUTABLE}" -b html . "${CMAKE_CURRENT_BINARY_DIR}/build-html"
    DEPENDS
        OpenColorIO
        PyOpenColorIO
        ${CMAKE_BINARY_DIR}/docs/conf.py
        rst_extraction
        doc_copy
        Sphinx
    COMMENT "Building html docs"
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
            OpenColorIO
            PyOpenColorIO
            ${CMAKE_BINARY_DIR}/docs/conf.py
            rst_extraction
            doc_copy
            Sphinx
        COMMENT "Building latex doc"
    )

    add_custom_target(pdf ALL
        DEPENDS latex
        COMMAND ${PDFLATEX_COMPILER} OpenColorIO.tex
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build-latex
        COMMENT "Building pdf doc"
    )

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/build-latex/OpenColorIO.pdf
            DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/OpenColorIO/)

endif()

include(ExternalProject)
include(FindPythonPackage)

find_package(PythonInterp 2.7 REQUIRED)
if(NOT OCIO_BUILD_PYTHON)
    message(FATAL_ERROR "Doc generation requires that the python bindings be built")
endif()

###############################################################################
### Setup PYTHONPATH ###

set(EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
set(PYTHON_VARIANT "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")

if(WIN32)
    # Need to escape the ";" path separator at each level of expansion
    set(PATH_SEP "\\\\\\;")
else()
    set(PATH_SEP ":")
endif()

# Add built PyOpenColorIO to PYTHONPATH
set(PYTHONPATH "${CMAKE_BINARY_DIR}/src/bindings/python")

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
    if(WIN32)
        string(APPEND PYTHONPATH
            "${PATH_SEP}${EXT_DIST_ROOT}/lib${LIB_SUFFIX}/site-packages")
    else()
        string(APPEND PYTHONPATH
            "${PATH_SEP}${EXT_DIST_ROOT}/lib${LIB_SUFFIX}/python${PYTHON_VARIANT}/site-packages")
    endif()
endif()

if(DEFINED ENV{PYTHONPATH})
    string(APPEND PYTHONPATH "${PATH_SEP}$ENV{PYTHONPATH}")
endif()

if(WIN32)
    # Unfortunately some windows tools require to have
    #  the paths with the '\' (i.e. not working with '//').
    string(REGEX REPLACE "/" "\\\\" PYTHONPATH ${PYTHONPATH})

    # Workaround to mimic *nix '> PYTHONPATH=XXX CMD' 
    #  on windows, it becomes  '> set PYTHONPATH=XXX \n call CMD'
    #   '\n' is here because '\\&' does not work.
    set(PYT_PRE_CMD set PYTHONPATH=${PYTHONPATH} "\n" call )
else()
    set(PYT_PRE_CMD PYTHONPATH=${PYTHONPATH})
endif()

message(STATUS ${PYT_PRE_CMD})

###############################################################################
### Copy templates to build area ###

message(STATUS "Create sphinx conf.py from conf.py.in")
configure_file(conf.py.in ${CMAKE_BINARY_DIR}/docs/conf.py @ONLY)

file(GLOB_RECURSE DOC_SOURCES "${CMAKE_SOURCE_DIR}/docs/*")
list(APPEND DOC_SOURCES 
    ${CMAKE_SOURCE_DIR}/README.md
    ${CMAKE_SOURCE_DIR}/INSTALL.md
    ${CMAKE_SOURCE_DIR}/CHANGELOG.md
    ${CMAKE_SOURCE_DIR}/LICENSE.md
    ${CMAKE_SOURCE_DIR}/share/nuke/ocionuke/viewer.py
)

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
    COMMAND ${PYTHON_EXECUTABLE} ${EXTRACT_COMMAND} ${OCIO_HEADER_DIR}/OpenColorIO.h ${RST_DESTINATION}/OpenColorIO.rst
    COMMAND ${PYTHON_EXECUTABLE} ${EXTRACT_COMMAND} ${OCIO_HEADER_DIR}/OpenColorTransforms.h ${RST_DESTINATION}/OpenColorTransforms.rst
    COMMAND ${PYTHON_EXECUTABLE} ${EXTRACT_COMMAND} ${OCIO_HEADER_DIR}/OpenColorTypes.h ${RST_DESTINATION}/OpenColorTypes.rst
    COMMENT "Extracting .rst files from C++ headers"
)

###############################################################################
### HTML doc target ###

add_custom_target(docs ALL
    COMMAND 
        ${PYT_PRE_CMD} "${SPHINX_EXECUTABLE}" -b html . "${CMAKE_CURRENT_BINARY_DIR}/build-html"
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
if(PDFLATEX_COMPILER)
    
    add_custom_target(latex
        COMMAND 
            ${PYT_PRE_CMD} "${SPHINX_EXECUTABLE}" -b latex . "${CMAKE_CURRENT_BINARY_DIR}/build-latex"
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

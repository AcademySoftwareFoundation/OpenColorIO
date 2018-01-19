
###############################################################################
### External Doc Apps ###

if (WIN32)
    # Workaround to mimic *nix '> PYTHONPATH=XXX CMD' 
    #  on windows, it becomes  '> set PYTHONPATH=XXX \n call CMD'
    #   '\n' is here because '\\&' does not work.
    set(PYT_PRE_CMD set PYTHONPATH=${PYTHONPATH} "\n" call )
    # Unfortunately some windows tools require to have
    #  the paths with the '\' (not working with '//').
    set(PYT_LIB_OUTPUT ${EXTDIST_ROOT})
    string(REGEX REPLACE "/" "\\\\" PYT_LIB_OUTPUT ${PYT_LIB_OUTPUT})
    set(PYT_EXTDIST_BINPATH ${EXTDIST_BINPATH})
    string(REGEX REPLACE "/" "\\\\" PYT_EXTDIST_BINPATH ${PYT_EXTDIST_BINPATH})
else()
    set(PYT_PRE_CMD PYTHONPATH=${PYTHONPATH})
    set(PYT_LIB_OUTPUT ${EXTDIST_ROOT})
    set(PYT_EXTDIST_BINPATH ${EXTDIST_BINPATH})
endif()

# setuptools
# https://pypi.python.org/pypi/setuptools
set(SETUPTOOLS_VERSION 1.1.6)

ExternalProject_Add(setuptools
    URL ${CMAKE_SOURCE_DIR}/ext/setuptools-${SETUPTOOLS_VERSION}.tar.gz
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E make_directory ${EXTDIST_PYTHONPATH}
    BUILD_COMMAND ${PYT_PRE_CMD} ${PYTHON} setup.py build
    INSTALL_COMMAND ${PYT_PRE_CMD} ${PYTHON} setup.py install --prefix=${PYT_LIB_OUTPUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/setuptools-prefix/src/setuptools
)

# docutils
# https://pypi.python.org/pypi/docutils
set(DOCUTILS_VERSION 0.11)
ExternalProject_Add(docutils
    DEPENDS setuptools
    URL ${CMAKE_SOURCE_DIR}/ext/docutils-${DOCUTILS_VERSION}.tar.gz
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E make_directory ${EXTDIST_PYTHONPATH}
    BUILD_COMMAND ${PYT_PRE_CMD} ${PYTHON} setup.py build
    INSTALL_COMMAND ${PYT_PRE_CMD} ${PYTHON} setup.py install --prefix=${PYT_LIB_OUTPUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docutils-prefix/src/docutils
)

# jinja2
# https://pypi.python.org/pypi/Jinja2
set(JINJA2_VERSION 2.7.1)
ExternalProject_Add(Jinja2
    DEPENDS setuptools
    URL ${CMAKE_SOURCE_DIR}/ext/Jinja2-${JINJA2_VERSION}.tar.gz
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E make_directory ${EXTDIST_PYTHONPATH}
    BUILD_COMMAND ${PYT_PRE_CMD} ${PYTHON} setup.py build
    INSTALL_COMMAND ${PYT_PRE_CMD} ${PYTHON} setup.py install --prefix=${PYT_LIB_OUTPUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Jinja2-prefix/src/Jinja2
)

# Pygments
# https://pypi.python.org/pypi/Pygments
set(PYGMENTS_VERSION 1.6)
ExternalProject_Add(Pygments
    DEPENDS setuptools
    URL ${CMAKE_SOURCE_DIR}/ext/Pygments-${PYGMENTS_VERSION}.tar.gz
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E make_directory ${EXTDIST_PYTHONPATH}
    BUILD_COMMAND ${PYT_PRE_CMD} ${PYTHON} setup.py build
    INSTALL_COMMAND ${PYT_PRE_CMD} ${PYTHON} setup.py install --prefix=${PYT_LIB_OUTPUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Pygments-prefix/src/Pygments
)

# sphinx
# https://pypi.python.org/pypi/Sphinx
set(SPHINX_VERSION 1.2b3)
ExternalProject_Add(Sphinx
    DEPENDS setuptools docutils Jinja2 Pygments
    URL ${CMAKE_SOURCE_DIR}/ext/Sphinx-${SPHINX_VERSION}.tar.gz
    PATCH_COMMAND patch -p1 < ${CMAKE_SOURCE_DIR}/ext/Sphinx-${SPHINX_VERSION}.patch
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E make_directory ${EXTDIST_PYTHONPATH}
    BUILD_COMMAND ${PYT_PRE_CMD} ${PYTHON} setup.py build
    INSTALL_COMMAND ${PYT_PRE_CMD} ${PYTHON} setup.py install --prefix=${PYT_LIB_OUTPUT} --install-scripts=${PYT_EXTDIST_BINPATH}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Sphinx-prefix/src/Sphinx
)

###############################################################################
### Create Doc Targets ###

message(STATUS "Create sphinx conf.py from conf.py.in")
configure_file(${CMAKE_SOURCE_DIR}/docs/conf.py.in
    ${CMAKE_BINARY_DIR}/docs/conf.py @ONLY)

message(STATUS "Copying doc to staging area")
file(
    COPY ${CMAKE_SOURCE_DIR}/docs/
    DESTINATION ${CMAKE_BINARY_DIR}/docs

    # Ignore dot files like emacs' temporary .#somefile.rst which
    # break Sphinx (and must be manually removed from the destination)
    PATTERN ".*" EXCLUDE
    )

message(STATUS "Copy extra doc files to staging area")
list(APPEND DOCFILES ${CMAKE_SOURCE_DIR}/README.md)
list(APPEND DOCFILES ${CMAKE_SOURCE_DIR}/INSTALL)
list(APPEND DOCFILES ${CMAKE_SOURCE_DIR}/ChangeLog)
list(APPEND DOCFILES ${CMAKE_SOURCE_DIR}/LICENSE)
list(APPEND DOCFILES ${CMAKE_SOURCE_DIR}/share/nuke/ocionuke/viewer.py)
CopyFiles(RSTDOC ${DOCFILES})

message(STATUS "Extracting .rst files from C++ headers")
ExtractRstCPP(${CMAKE_SOURCE_DIR}/export/OpenColorIO/OpenColorIO.h ${CMAKE_BINARY_DIR}/docs/developers/api/OpenColorIO.rst)
ExtractRstCPP(${CMAKE_SOURCE_DIR}/export/OpenColorIO/OpenColorTransforms.h ${CMAKE_BINARY_DIR}/docs/developers/api/OpenColorTransforms.rst)
ExtractRstCPP(${CMAKE_SOURCE_DIR}/export/OpenColorIO/OpenColorTypes.h ${CMAKE_BINARY_DIR}/docs/developers/api/OpenColorTypes.rst)

if(OCIO_BUILD_PYGLUE)
    set(DEPLIBS OpenColorIO PyOpenColorIO)
else()
    set(DEPLIBS OpenColorIO)
endif()

add_custom_target(doc ALL
    COMMAND ${PYT_PRE_CMD} ${EXTDIST_BINPATH}/sphinx-build -b html . ${CMAKE_CURRENT_BINARY_DIR}/build-html
    DEPENDS
        ${DEPLIBS}
        ${CMAKE_BINARY_DIR}/docs/conf.py
        developers/api/OpenColorIO.rst
        developers/api/OpenColorTransforms.rst
        developers/api/OpenColorTypes.rst
        ${RSTDOC_OUTPUT}
    COMMENT "Building html docs"
    SOURCES ${DOCFILES})

# note: ExternalProject will not build when added to a add_custom_target this
# works around this problem. This seems to be fixed in the cmake ^HEAD
add_dependencies(doc Sphinx) 

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build-html/
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/html
        PATTERN .* EXCLUDE
)

if(TARGET doc)
    message(STATUS "Copying admin doc files to deploy area")
    file(COPY ${CMAKE_SOURCE_DIR}/docs/.nojekyll
              ${CMAKE_SOURCE_DIR}/docs/google2560df9c4b755196.html
         DESTINATION ${CMAKE_BINARY_DIR}/docs/build-html)
endif()

find_package(LATEX)
if(PDFLATEX_COMPILER)
    
    add_custom_target(latex
        COMMAND ${PYT_PRE_CMD} ${EXTDIST_BINPATH}/sphinx-build -b latex . ${CMAKE_CURRENT_BINARY_DIR}/build-latex
        DEPENDS
            OpenColorIO
            ${CMAKE_BINARY_DIR}/docs/conf.py
            developers/api/OpenColorIO.rst
            developers/api/OpenColorTransforms.rst
            developers/api/OpenColorTypes.rst
            ${RSTDOC_OUTPUT}
        COMMENT "Building latex doc"
        SOURCES ${DOCFILES})
    add_dependencies(latex Sphinx)
    
    add_custom_target(pdf ALL
        COMMAND ${PDFLATEX_COMPILER} OpenColorIO.tex
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build-latex
        COMMENT "Building pdf doc"
        SOURCES ${DOCFILES})
    add_dependencies(pdf latex)
    
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/build-latex/OpenColorIO.pdf
            DESTINATION ${CMAKE_INSTALL_DOCDIR})
    
endif()

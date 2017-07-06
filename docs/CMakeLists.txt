
###############################################################################
### External Doc Apps ###

# setuptools
# https://pypi.python.org/pypi/setuptools
set(SETUPTOOLS_VERSION 1.1.6)
ExternalProject_Add(setuptools
    URL ${CMAKE_SOURCE_DIR}/ext/setuptools-${SETUPTOOLS_VERSION}.tar.gz
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E make_directory ${EXTDIST_PYTHONPATH}
    BUILD_COMMAND PYTHONPATH=${PYTHONPATH} ${PYTHON} setup.py build
    INSTALL_COMMAND PYTHONPATH=${PYTHONPATH} ${PYTHON} setup.py install --prefix=${EXTDIST_ROOT}
)

# docutils
# https://pypi.python.org/pypi/docutils
set(DOCUTILS_VERSION 0.11)
ExternalProject_Add(docutils
    DEPENDS setuptools
    URL ${CMAKE_SOURCE_DIR}/ext/docutils-${DOCUTILS_VERSION}.tar.gz
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E make_directory ${EXTDIST_PYTHONPATH}
    BUILD_COMMAND PYTHONPATH=${PYTHONPATH} ${PYTHON} setup.py build
    INSTALL_COMMAND PYTHONPATH=${PYTHONPATH} ${PYTHON} setup.py install --prefix=${EXTDIST_ROOT}
)

# jinja2
# https://pypi.python.org/pypi/Jinja2
set(JINJA2_VERSION 2.7.1)
ExternalProject_Add(Jinja2
    DEPENDS setuptools
    URL ${CMAKE_SOURCE_DIR}/ext/Jinja2-${JINJA2_VERSION}.tar.gz
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E make_directory ${EXTDIST_PYTHONPATH}
    BUILD_COMMAND PYTHONPATH=${PYTHONPATH} ${PYTHON} setup.py build
    INSTALL_COMMAND PYTHONPATH=${PYTHONPATH} ${PYTHON} setup.py install --prefix=${EXTDIST_ROOT}
)

# Pygments
# https://pypi.python.org/pypi/Pygments
set(PYGMENTS_VERSION 1.6)
ExternalProject_Add(Pygments
    DEPENDS setuptools
    URL ${CMAKE_SOURCE_DIR}/ext/Pygments-${PYGMENTS_VERSION}.tar.gz
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E make_directory ${EXTDIST_PYTHONPATH}
    BUILD_COMMAND PYTHONPATH=${PYTHONPATH} ${PYTHON} setup.py build
    INSTALL_COMMAND PYTHONPATH=${PYTHONPATH} ${PYTHON} setup.py install --prefix=${EXTDIST_ROOT}
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
    BUILD_COMMAND PYTHONPATH=${PYTHONPATH} ${PYTHON} setup.py build
    INSTALL_COMMAND PYTHONPATH=${PYTHONPATH} ${PYTHON} setup.py install --prefix=${EXTDIST_ROOT} --install-scripts=${EXTDIST_ROOT}/bin
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
ExtractRstCPP(${CMAKE_SOURCE_DIR}/export/OpenColorIO/OpenColorIO.h developers/api/OpenColorIO.rst)
ExtractRstCPP(${CMAKE_SOURCE_DIR}/export/OpenColorIO/OpenColorTransforms.h developers/api/OpenColorTransforms.rst)
ExtractRstCPP(${CMAKE_SOURCE_DIR}/export/OpenColorIO/OpenColorTypes.h developers/api/OpenColorTypes.rst)

if(OCIO_BUILD_PYGLUE)
    set(DEPLIBS OpenColorIO PyOpenColorIO)
else()
    set(DEPLIBS OpenColorIO)
endif()

add_custom_target(doc ALL
    COMMAND PYTHONPATH=${PYTHONPATH} ${EXTDIST_BINPATH}/sphinx-build -b html . ${CMAKE_CURRENT_BINARY_DIR}/build-html
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
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/OpenColorIO/html
        PATTERN .* EXCLUDE
)

find_package(LATEX)
if(PDFLATEX_COMPILER)
    
    add_custom_target(latex
        COMMAND PYTHONPATH=${PYTHONPATH} ${EXTDIST_BINPATH}/sphinx-build -b latex . ${CMAKE_CURRENT_BINARY_DIR}/build-latex
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
            DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/OpenColorIO/)
    
endif()

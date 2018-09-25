include(ExternalProject)

find_package(PythonInterp 2.7 REQUIRED)
if(NOT OCIO_BUILD_PYTHON)
	message(FATAL_ERROR "Doc generation requires that the python bindings be built")
endif()

set(PATCH_COMMAND patch)

###############################################################################
### External Doc Apps ###

set(EXTDIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
set(PYTHON_VARIANT "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
set(PYTHONPATH "${EXTDIST_ROOT}/lib${LIB_SUFFIX}/python${PYTHON_VARIANT}/site-packages")

set(PYTHON_CMD PYTHONPATH=${PYTHONPATH} ${PYTHON_EXECUTABLE})

add_custom_target(pythonpath_create
	COMMAND ${CMAKE_COMMAND} -E make_directory ${PYTHONPATH}
)

# setuptools
# https://pypi.python.org/pypi/setuptools
set(SETUPTOOLS_VERSION 1.1.6)
ExternalProject_Add(setuptools
	DEPENDS pythonpath_create
    URL ${CMAKE_SOURCE_DIR}/ext/setuptools-${SETUPTOOLS_VERSION}.tar.gz
    BUILD_IN_SOURCE 1
	CONFIGURE_COMMAND ""
    BUILD_COMMAND ${PYTHON_CMD} setup.py build
    INSTALL_COMMAND ${PYTHON_CMD} setup.py install --prefix=${EXTDIST_ROOT}
)

# docutils
# https://pypi.python.org/pypi/docutils
set(DOCUTILS_VERSION 0.11)
ExternalProject_Add(docutils
    DEPENDS setuptools
    URL ${CMAKE_SOURCE_DIR}/ext/docutils-${DOCUTILS_VERSION}.tar.gz
	CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE 1
    BUILD_COMMAND ${PYTHON_CMD} setup.py build
    INSTALL_COMMAND ${PYTHON_CMD} setup.py install --prefix=${EXTDIST_ROOT}
)

# MarkupSafe
# https://pypi.python.org/pypi/MarkupSafe
set(MARKUPSAFE_VERSION 1.0)
ExternalProject_Add(MarkupSafe
    DEPENDS setuptools
	URL ${CMAKE_SOURCE_DIR}/ext/MarkupSafe-${MARKUPSAFE_VERSION}.tar.gz
	CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE 1
    BUILD_COMMAND ${PYTHON_CMD} setup.py build
    INSTALL_COMMAND ${PYTHON_CMD} setup.py install --prefix=${EXTDIST_ROOT}
)

# jinja2
# https://pypi.python.org/pypi/Jinja2
set(JINJA2_VERSION 2.7.1)
ExternalProject_Add(Jinja2
	DEPENDS setuptools MarkupSafe
    URL ${CMAKE_SOURCE_DIR}/ext/Jinja2-${JINJA2_VERSION}.tar.gz
	CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE 1
    BUILD_COMMAND ${PYTHON_CMD} setup.py build
    INSTALL_COMMAND ${PYTHON_CMD} setup.py install --prefix=${EXTDIST_ROOT}
)

# Pygments
# https://pypi.python.org/pypi/Pygments
set(PYGMENTS_VERSION 1.6)
ExternalProject_Add(Pygments
    DEPENDS setuptools
    URL ${CMAKE_SOURCE_DIR}/ext/Pygments-${PYGMENTS_VERSION}.tar.gz
	CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE 1
    BUILD_COMMAND ${PYTHON_CMD} setup.py build
    INSTALL_COMMAND ${PYTHON_CMD} setup.py install --prefix=${EXTDIST_ROOT}
)

# sphinx
# https://pypi.python.org/pypi/Sphinx
set(SPHINX_VERSION 1.2b3)
ExternalProject_Add(Sphinx
    DEPENDS setuptools docutils Jinja2 Pygments
    URL ${CMAKE_SOURCE_DIR}/ext/Sphinx-${SPHINX_VERSION}.tar.gz
    PATCH_COMMAND ${PATCH_COMMAND} -p1 < ${CMAKE_SOURCE_DIR}/ext/Sphinx-${SPHINX_VERSION}.patch
	CONFIGURE_COMMAND ""
    BUILD_IN_SOURCE 1
    BUILD_COMMAND ${PYTHON_CMD} setup.py build
    INSTALL_COMMAND ${PYTHON_CMD} setup.py install --prefix=${EXTDIST_ROOT} --install-scripts=${EXTDIST_ROOT}/bin
)

###############################################################################
### Copy templates to build area ###

message(STATUS "Create sphinx conf.py from conf.py.in")
configure_file(conf.py.in ${CMAKE_BINARY_DIR}/docs/conf.py @ONLY)

file(GLOB_RECURSE DOC_SOURCES "${CMAKE_SOURCE_DIR}/docs/*")
list(APPEND DOC_SOURCES 
	${CMAKE_SOURCE_DIR}/README.md
	${CMAKE_SOURCE_DIR}/INSTALL
	${CMAKE_SOURCE_DIR}/ChangeLog
	${CMAKE_SOURCE_DIR}/LICENSE
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

# Add built PyOpenColorIO to PYTHONPATH and ensure that doc target depends on PyOpenColorIO
set(PYTHONPATH "${PYTHONPATH}:${CMAKE_BINARY_DIR}/src/python")


###############################################################################
### HTML doc target ###

add_custom_target(docs ALL
    COMMAND PYTHONPATH=${PYTHONPATH} ${EXTDIST_ROOT}/bin/sphinx-build -b html . ${CMAKE_CURRENT_BINARY_DIR}/build-html
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
        COMMAND PYTHONPATH=${PYTHONPATH} ${EXTDIST_ROOT}/bin/sphinx-build -b latex . ${CMAKE_CURRENT_BINARY_DIR}/build-latex
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

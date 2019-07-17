###############################################################################
### External C/C++ dependencies ###

# Expat
# https://github.com/libexpat/libexpat
set(EXPAT_MIN_VERSION 2.2.5)
find_package(Expat ${EXPAT_MIN_VERSION} REQUIRED)

# yaml-cpp
# https://github.com/jbeder/yaml-cpp
set(YAMLCPP_MIN_VERSION 0.3.0)
find_package(YamlCpp ${YAMLCPP_MIN_VERSION} REQUIRED)

# IlmBase (Half part only)
# https://github.com/openexr/openexr
set(ILMBASE_MIN_VERSION 2.3.0)
find_package(IlmBase ${ILMBASE_MIN_VERSION} REQUIRED)

# Pystring
# https://github.com/imageworks/pystring
set(PYSTRING_MIN_VERSION 1.1.3)
find_package(Pystring ${PYSTRING_MIN_VERSION} REQUIRED)

if(OCIO_BUILD_APPS)
	# LCMS2
	# https://github.com/mm2/Little-CMS
	set(LCMS2_MIN_VERSION 2.2)
	find_package(LCMS2 ${LCMS2_MIN_VERSION} REQUIRED)
endif()

# Sample ICC (modified)
# http://sampleicc.sourceforge.net
add_library(sampleicc::sampleicc INTERFACE IMPORTED GLOBAL)
set_target_properties(sampleicc::sampleicc PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/sampleicc/src/include"
)

###############################################################################
### External Python dependencies ###

if(OCIO_BUILD_DOCS)
	include(FindPythonPackage)

	# setuptools
	# https://pypi.python.org/pypi/setuptools
	set(SETUPTOOLS_MIN_VERSION 1.1.6)
	find_python_package(setuptools ${SETUPTOOLS_MIN_VERSION} REQUIRED)

	# docutils
	# https://pypi.python.org/pypi/docutils
	set(DOCUTILS_MIN_VERSION 0.14)
	find_python_package(docutils ${DOCUTILS_MIN_VERSION} REQUIRED)
	add_dependencies(docutils setuptools)

	# MarkupSafe
	# https://pypi.python.org/pypi/MarkupSafe
	set(MARKUPSAFE_MIN_VERSION 1.1.1)
	find_python_package(MarkupSafe ${MARKUPSAFE_MIN_VERSION} REQUIRED)
	add_dependencies(MarkupSafe setuptools)

	# Jinja2
	# https://pypi.python.org/pypi/Jinja2
	set(JINJA2_MIN_VERSION 2.10.1)
	find_python_package(Jinja2 ${JINJA2_MIN_VERSION} REQUIRED)
	add_dependencies(Jinja2 setuptools MarkupSafe)

	# Pygments
	# https://pypi.python.org/pypi/Pygments
	set(PYGMENTS_MIN_VERSION 2.4.2)
	find_python_package(Pygments ${PYGMENTS_MIN_VERSION} REQUIRED)
	add_dependencies(Pygments setuptools)

	# Sphinx
	# https://pypi.python.org/pypi/Sphinx
	set(SPHINX_MIN_VERSION 1.8.5)
	find_package(Sphinx ${SPHINX_MIN_VERSION} REQUIRED)
	add_dependencies(Sphinx setuptools docutils Jinja2 Pygments)

	# Make sphinx-build path available in other modules
	set(SPHINX_EXECUTABLE "${SPHINX_EXECUTABLE}" CACHE STRING "Path to sphinx-build")
endif()

###############################################################################
### sampleicc ###
add_library(sampleicc INTERFACE IMPORTED GLOBAL)
set_property(TARGET sampleicc PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/sampleicc/src/include")

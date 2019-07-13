# Add target to install a Python package with pip.
#
# Parameters:
#   package - Python package name from PyPi
#   min_version - Minimum allowed (and default) version
#   prefix - Install prefix
#
# Output targets:
#   <package> - Install target
#
# Usage:
#   pip_install(package min_version prefix)
#   pip_install(Sphinx 2.1.2 "ext/dist")
#
# Pre-defined variables that affect macro behavior:
#   GET_<PACKAGE>_MIN_VERSION - Request a specific version
#

find_package(PythonInterp 2.7 QUIET)

macro(pip_install package min_version prefix)
    string(TOUPPER ${package} _PKG_UPPER)
    set(${_PKG_UPPER}_VERSION ${min_version})
    
    # Was a specific version requested?
    if(GET_${_PKG_UPPER}_VERSION)
        if(NOT GET_${_PKG_UPPER}_VERSION VERSION_GREATER_EQUAL ${min_version})
            message(WARNING 
                "The minimum supported ${package} version is ${min_version} "
                "(version ${GET_${_PKG_UPPER}_VERSION} was requested)."
            )
        else()
            set(${_PKG_UPPER}_VERSION ${GET_${_PKG_UPPER}_VERSION})
        endif()
    endif()

    # Package install location
    if(WIN32)
        set(_SITE_PKGS_DIR "${prefix}/lib${LIB_SUFFIX}/site-packages")
    else()
        set(_PY_VERSION "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
        set(_SITE_PKGS_DIR "${prefix}/lib${LIB_SUFFIX}/python${_PY_VERSION}/site-packages")
    endif()

    # pip install target
    message(STATUS "Installing ${package} (version ${${_PKG_UPPER}_VERSION}): ${_SITE_PKGS_DIR}")
    add_custom_target(${package})
    add_custom_command(TARGET ${package} PRE_BUILD
        COMMAND pip install --quiet
                            --disable-pip-version-check
                            --install-option="--prefix=${prefix}"
                            -I ${package}==${${_PKG_UPPER}_VERSION}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endmacro()

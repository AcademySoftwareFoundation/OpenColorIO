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
#   pip_install(Sphinx 2.1.2 "ext/dist")
#
# Pre-defined variables that affect macro behavior:
#   GET_<PACKAGE>_MIN_VERSION - Request a specific version
#

macro(pip_install package min_version prefix)
	string(TOUPPER ${package} _PACKAGE_UPPER)

    # Which version should be installed?
    if(GET_${_PACKAGE_UPPER}_VERSION
            AND GET_${_PACKAGE_UPPER}_VERSION VERSION_GREATER_EQUAL ${min_version})
        set(${_PACKAGE_UPPER}_VERSION ${GET_${_PACKAGE_UPPER}_VERSION})
    else()
        if(GET_${_PACKAGE_UPPER}_VERSION)
            message(WARNING "Minimum supported ${package} version is ${min_version}")
        endif()
        set(${_PACKAGE_UPPER}_VERSION ${min_version})
    endif()

    # Setup install target command (to run at build time)
    message(STATUS "${package} ${_PACKAGE_UPPER}_VERSION will be installed to: ${prefix}")
    add_custom_command(TARGET ${package} PRE_BUILD
        COMMAND pip install --quiet
                            --disable-pip-version-check
                            --install-option="--prefix=${prefix}"
                            -I ${package}==${_PACKAGE_UPPER}_VERSION}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    add_custom_target(${package})
macro()

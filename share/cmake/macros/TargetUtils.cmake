# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Imported from the OpenImageIO project.
#

###########################################################################
# Macro to install targets to the appropriate locations.  Use this instead
# of the install(TARGETS ...) signature. Note that it adds it to the
# export targets list for when we generate config files.
#
# Usage:
#
#    install_targets (target1 [target2 ...])
#
macro (install_targets)
    install (TARGETS ${ARGN}
             EXPORT ${PROJECT_NAME}_EXPORTED_TARGETS
             RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT user
             LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT user
             ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT developer)
endmacro()

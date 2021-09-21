# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

#
# The order defines the architecture of the project i.e. layers and dependencies.
#

add_subdirectory(apps)          # Layer for the apps i.e. can only use the public API and
                                # 'libutils', 'utils' & 'apputils' libraries for implementation
                                # only.

add_subdirectory(bindings)      # Layer for the various bindings i.e. can only use the public API
                                # and 'libutils', 'utils' & 'apputils' libraries for implementation 
                                # only.

add_subdirectory(libutils)      # Layer for the libraries built on top of the OpenColorIO library
                                # i.e. can only use the public API, 'utils' and 'apputils' libraries
                                # for implementation only.

add_subdirectory(apputils)      # Various app. helpers used by any upper layers.

add_subdirectory(OpenColorIO)   # The OpenColorIO library i.e. can only use 'utils' libraries for
                                # implementation only.

add_subdirectory(utils)         # Various string helpers used by any upper layers. But they cannot
                                # be part of the public API.

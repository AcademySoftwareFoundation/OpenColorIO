# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# The presence of OpenImageIO allows additional ocio apps to be built.
# These apps may also require the building of oglapphelpers from libutils
# but because the builder also needs GLEW, we only conditionally build
# this lib

# OpenImageIO
# https://github.com/OpenImageIO/oiio
find_package(OpenImageIO 2.1.9 CONFIG)
package_root_message(OpenImageIO)

# The order defines the architecture of the project i.e. layers and dependencies.

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

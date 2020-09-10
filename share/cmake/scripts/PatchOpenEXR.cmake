# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# Disable OpenEXR in the root CMakeLists.txt file, so that only IlmBase is 
# configured.
file(READ CMakeLists.txt _OpenEXR_CMAKE_DATA)
string(REGEX REPLACE "^(.*)(add_subdirectory\\(OpenEXR\\))(.*)$" "\\1#\\2\\3" 
    _OpenEXR_CMAKE_DATA "${_OpenEXR_CMAKE_DATA}")
file(WRITE CMakeLists.txt "${_OpenEXR_CMAKE_DATA}")

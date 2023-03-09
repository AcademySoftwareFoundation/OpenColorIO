# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import os, sys, platform

#
# Python 3.8+ has stopped loading DLLs from PATH environment variable on Windows.
#
# This code reproduce the old behavior (loading DLLs from PATH) by doing the following:
# 1 - Tokenizing PATH
# 2 - Checking that the directories exist and are not "."
# 3 - Add them to the DLL load path.
#
# The behavior described above is opt-out which means that it is activated by default. 
# A user can opt-out and use the default behavior of Python 3.8+ by setting OCIO_PYTHON_LOAD_DLLS_FROM_PATH 
# environment variable to 0.
#

if sys.version_info >= (3, 8) and platform.system() == "Windows" and os.getenv("OCIO_PYTHON_LOAD_DLLS_FROM_PATH", "1") == "1":
    for path in os.getenv("PATH", "").split(os.pathsep):
        if os.path.exists(path) and path != ".":
            os.add_dll_directory(path)

from .PyOpenColorIO import *
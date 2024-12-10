# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import os, sys, platform

#
# Python wheel module is dynamically linked to the OCIO DLL present in the bin folder.
#

if platform.system() == "Windows":
    here = os.path.abspath(os.path.dirname(__file__))
    bin_dir = os.path.join(here, "bin")
    if os.path.exists(bin_dir):
        if sys.version_info >= (3, 8):
            os.add_dll_directory(bin_dir)
        else:
            os.environ['PATH'] = '{0};{1}'.format(bin_dir, os.getenv('PATH', ''))

#
# Python 3.8+ has stopped loading DLLs from PATH environment variable on Windows.
#
# This code reproduce the old behavior (loading DLLs from PATH) by doing the following:
# 1 - Tokenizing PATH
# 2 - Checking that the directories exist and are not "."
# 3 - Add them to the DLL load path.
#
# The behavior described above is opt-in which means that it is deactivated by default.
# A user can opt-in by setting OCIO_PYTHON_LOAD_DLLS_FROM_PATH environment variable to 1.
# Please note that OCIO 2.4.0 and earlier had this behavior opt-out but it was changed
# in 2.4.1 after it was found problematic in uncontrolled environment (causing segfault
# on PyOpenColorIO import) as well as for security reasons.
#

if sys.version_info >= (3, 8) and platform.system() == "Windows":
    if os.getenv("OCIO_PYTHON_LOAD_DLLS_FROM_PATH", "0") == "1":
        for path in os.getenv("PATH", "").split(os.pathsep):
            if os.path.exists(path) and path != ".":
                os.add_dll_directory(path)

del os, sys, platform

#
# Import compiled module.
#

from .PyOpenColorIO import __author__, __email__, __license__, __copyright__, __version__, __status__, __doc__

from .PyOpenColorIO import *

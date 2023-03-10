# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import logging
import pytest
import os
import sys

logging.basicConfig(
    level=logging.INFO,
    format="[%(levelname)s] %(name)s: %(message)s",
)

# We are called from CTest if called with arguments (build_tree, build_type)
if len(sys.argv) > 1:
    build_location = sys.argv[1]
    os.environ["TEST_DATAFILES_DIR"] = os.path.join(build_location, 'testdata')

    opencolorio_dir = os.path.join(build_location, 'src', 'OpenColorIO')
    pyopencolorio_dir = os.path.join(build_location, 'src', 'bindings', 'python')

    if os.name == 'nt':
        # On Windows we must append the build type to the build dirs and add the main library to PATH
        # Note: Only when compiling within Microsoft Visual Studio editor i.e. not on command line.
        if len(sys.argv) == 3:
            opencolorio_dir = os.path.join(opencolorio_dir, sys.argv[2])
            pyopencolorio_dir = os.path.join(pyopencolorio_dir, sys.argv[2])

        # Python 3.8+ does no longer look for DLLs in PATH environment variable
        if hasattr(os, 'add_dll_directory'):
            os.add_dll_directory(opencolorio_dir)
        else:
            os.environ['PATH'] = '{0};{1}'.format(
                opencolorio_dir, os.getenv('PATH', ''))
    elif sys.platform == 'darwin':
        # On OSX we must add the main library location to DYLD_LIBRARY_PATH
        os.environ['DYLD_LIBRARY_PATH'] = '{0}:{1}'.format(
            opencolorio_dir, os.getenv('DYLD_LIBRARY_PATH', ''))

    sys.path.insert(0, pyopencolorio_dir)
# Else it probably means direct invocation from installed package
else:
    here = os.path.dirname(__file__)
    os.environ["TEST_DATAFILES_DIR"] = os.path.join(here, 'data', 'files')
    sys.path.insert(0, here)


if __name__ == '__main__':
    exit_code = pytest.main([os.path.dirname(__file__)])
    sys.exit(exit_code)

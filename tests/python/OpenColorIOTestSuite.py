# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest
import os
import sys
from fnmatch import filter as fnfilter

build_location = sys.argv[1]

opencolorio_sse = sys.argv[2].lower() == 'true'

opencolorio_dir = os.path.join(build_location, 'src', 'OpenColorIO')
pyopencolorio_dir = os.path.join(build_location, 'src', 'bindings', 'python')

if os.name == 'nt':
    # On Windows we must append the build type to the build dirs and add the main library to PATH
    # Note: Only when compiling within Microsoft Visual Studio editor i.e. not on command line.
    if len(sys.argv) == 4:
        opencolorio_dir = os.path.join(opencolorio_dir, sys.argv[3])
        pyopencolorio_dir = os.path.join(pyopencolorio_dir, sys.argv[3])

    os.environ['PATH'] = '{0};{1}'.format(
        opencolorio_dir, os.getenv('PATH', ''))
elif sys.platform == 'darwin':
    # On OSX we must add the main library location to DYLD_LIBRARY_PATH
    os.environ['DYLD_LIBRARY_PATH'] = '{0}:{1}'.format(
        opencolorio_dir, os.getenv('DYLD_LIBRARY_PATH', ''))

sys.path.insert(0, pyopencolorio_dir)
import PyOpenColorIO as OCIO

from BakerTest import *

def suite():
    """Load unittest.TestCase objects from *Test.py files within ./tests/Python

    :return: unittest test suite of TestCase objects.
    :rtype: unittest.TestSuite
    """

    # top level directory cached on loader instance
    suite = unittest.TestSuite()
    loader = unittest.TestLoader()

    this_dir = os.path.dirname(__file__)
    test_modules = [filename.replace('.py', '') for filename in fnfilter(
        os.listdir(this_dir), '*Test.py')]
    map(__import__, test_modules)

    for mod in [sys.modules[modname] for modname in test_modules]:
        try:
            suite.addTest(loader.loadTestsFromModule(mod))
        except TypeError:
            if mod.__name__ == 'BakerTest':
                suite.addTest(BakerTest("test_interface", opencolorio_sse))

    return suite


if __name__ == '__main__':
    runner = unittest.TextTestRunner(verbosity=2)
    test_suite = suite()
    result = runner.run(test_suite)
    if result.wasSuccessful() == False:
        sys.exit(1)
    sys.exit(0)

# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest
import os
import sys

build_location = sys.argv[1]
os.environ["BUILD_LOCATION"] = build_location

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

import ColorSpaceTest
import BuiltinTransformRegistryTest
import BuiltinTransformTest
import CDLTransformTest
import LookTest
#from MainTest import *
#from ConstantsTest import *
#from ConfigTest import *
#from ContextTest import *
#from GpuShaderDescTest import *
#from Baker import *
#from TransformsTest import *
#from RangeTransformTest import *

def suite():
    """Load unittest.TestCase objects from *Test.py files within ./tests/Python

    :return: unittest test suite of TestCase objects.
    :rtype: unittest.TestSuite
    """

    # top level directory cached on loader instance
    suite = unittest.TestSuite()
    loader = unittest.TestLoader()

    suite.addTest(loader.loadTestsFromModule(ColorSpaceTest))
    suite.addTest(loader.loadTestsFromModule(BuiltinTransformRegistryTest))
    suite.addTest(loader.loadTestsFromModule(BuiltinTransformTest))
    suite.addTest(loader.loadTestsFromModule(CDLTransformTest))
    suite.addTest(loader.loadTestsFromModule(LookTest))
    #suite.addTest(MainTest("test_interface"))
    #suite.addTest(ConstantsTest("test_interface"))
    #suite.addTest(ConfigTest("test_interface"))
    #suite.addTest(ConfigTest("test_is_editable"))
    #suite.addTest(ContextTest("test_interface"))
    #suite.addTest(RangeTransformTest("test_interface"))
    #suite.addTest(RangeTransformTest("test_equality"))
    #suite.addTest(RangeTransformTest("test_validation"))
    #suite.addTest(TransformsTest("test_interface"))

    # Processor
    # ProcessorMetadata
    #suite.addTest(GpuShaderDescTest("test_interface"))
    #suite.addTest(BakerTest("test_interface", opencolorio_sse))
    # PackedImageDesc
    # PlanarImageDesc

    return suite


if __name__ == '__main__':
    runner = unittest.TextTestRunner(verbosity=2)
    test_suite = suite()
    result = runner.run(test_suite)
    if result.wasSuccessful() == False:
        sys.exit(1)
    sys.exit(0)

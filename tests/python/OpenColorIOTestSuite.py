# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import logging
import unittest
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
        # PyOpenColorIO __init__.py file handle os.add_dll_directory()
        os.environ['PATH'] = '{0};{1}'.format(opencolorio_dir, os.getenv('PATH', ''))
    elif sys.platform == 'darwin':
        # On OSX we must add the main library location to DYLD_LIBRARY_PATH
        os.environ['DYLD_LIBRARY_PATH'] = '{0}:{1}'.format(
            opencolorio_dir, os.getenv('DYLD_LIBRARY_PATH', ''))

    sys.path.insert(0, pyopencolorio_dir)
# Else it probably means direct invocation from cibuildwheel
else:
    here = os.path.dirname(__file__)
    os.environ["TEST_DATAFILES_DIR"] = os.path.join(os.path.dirname(here), 'data', 'files')
    sys.path.insert(0, here)

import PyOpenColorIO as OCIO

import AllocationTransformTest
import BakerTest
import BuiltinConfigRegistryTest
import BuiltinTransformRegistryTest
import BuiltinTransformTest
import CDLTransformTest
import ColorSpaceHelpersTest
import ColorSpaceTest
import ColorSpaceTransformTest
import ConfigTest
import ConstantsTest
import ContextTest
import CPUProcessorTest
import DisplayViewHelpersTest
import DisplayViewTransformTest
import ExponentTransformTest
import ExponentWithLinearTransformTest
import ExposureContrastTransformTest
import FileRulesTest
import FileTransformTest
import FixedFunctionTransformTest
import FormatMetadataTest
import GpuShaderDescTest
import GradingDataTest
import GradingPrimaryTransformTest
import GradingRGBCurveTransformTest
import GradingToneTransformTest
import GroupTransformTest
import LegacyViewingPipelineTest
import LogCameraTransformTest
import LogTransformTest
import LookTest
import LookTransformTest
import Lut1DTransformTest
import Lut3DTransformTest
import MatrixTransformTest
import MixingHelpersTest
import NamedTransformTest
import OCIOZArchiveTest
import OpenColorIOTest
import ProcessorCacheTest
import ProcessorTest
import RangeTransformTest
import TransformsTest
import ViewingRulesTest
import ViewTransformTest

def suite():
    """Load unittest.TestCase objects from *Test.py files within ./tests/Python

    :return: unittest test suite of TestCase objects.
    :rtype: unittest.TestSuite
    """

    # top level directory cached on loader instance
    suite = unittest.TestSuite()
    loader = unittest.TestLoader()

    suite.addTest(loader.loadTestsFromModule(AllocationTransformTest))
    suite.addTest(loader.loadTestsFromModule(BakerTest))
    suite.addTest(loader.loadTestsFromModule(BuiltinConfigRegistryTest))
    suite.addTest(loader.loadTestsFromModule(BuiltinTransformRegistryTest))
    suite.addTest(loader.loadTestsFromModule(BuiltinTransformTest))
    suite.addTest(loader.loadTestsFromModule(CDLTransformTest))
    suite.addTest(loader.loadTestsFromModule(ColorSpaceHelpersTest))
    suite.addTest(loader.loadTestsFromModule(ColorSpaceTest))
    suite.addTest(loader.loadTestsFromModule(ColorSpaceTransformTest))
    suite.addTest(loader.loadTestsFromModule(ConfigTest))
    suite.addTest(loader.loadTestsFromModule(ConstantsTest))
    suite.addTest(loader.loadTestsFromModule(ContextTest))
    suite.addTest(loader.loadTestsFromModule(CPUProcessorTest))
    suite.addTest(loader.loadTestsFromModule(DisplayViewHelpersTest))
    suite.addTest(loader.loadTestsFromModule(DisplayViewTransformTest))
    suite.addTest(loader.loadTestsFromModule(ExponentTransformTest))
    suite.addTest(loader.loadTestsFromModule(ExponentWithLinearTransformTest))
    suite.addTest(loader.loadTestsFromModule(ExposureContrastTransformTest))
    suite.addTest(loader.loadTestsFromModule(FileTransformTest))
    suite.addTest(loader.loadTestsFromModule(FileRulesTest))
    suite.addTest(loader.loadTestsFromModule(FixedFunctionTransformTest))
    suite.addTest(loader.loadTestsFromModule(FormatMetadataTest))
    suite.addTest(loader.loadTestsFromModule(GpuShaderDescTest))
    suite.addTest(loader.loadTestsFromModule(GradingDataTest))
    suite.addTest(loader.loadTestsFromModule(GradingPrimaryTransformTest))
    suite.addTest(loader.loadTestsFromModule(GradingRGBCurveTransformTest))
    suite.addTest(loader.loadTestsFromModule(GradingToneTransformTest))
    suite.addTest(loader.loadTestsFromModule(GroupTransformTest))
    suite.addTest(loader.loadTestsFromModule(LegacyViewingPipelineTest))
    suite.addTest(loader.loadTestsFromModule(LogCameraTransformTest))
    suite.addTest(loader.loadTestsFromModule(LogTransformTest))
    suite.addTest(loader.loadTestsFromModule(LookTest))
    suite.addTest(loader.loadTestsFromModule(LookTransformTest))
    suite.addTest(loader.loadTestsFromModule(Lut1DTransformTest))
    suite.addTest(loader.loadTestsFromModule(Lut3DTransformTest))
    suite.addTest(loader.loadTestsFromModule(MatrixTransformTest))
    suite.addTest(loader.loadTestsFromModule(MixingHelpersTest))
    suite.addTest(loader.loadTestsFromModule(NamedTransformTest))
    suite.addTest(loader.loadTestsFromModule(OCIOZArchiveTest))
    suite.addTest(loader.loadTestsFromModule(OpenColorIOTest))
    suite.addTest(loader.loadTestsFromModule(ProcessorCacheTest))
    suite.addTest(loader.loadTestsFromModule(ProcessorTest))
    suite.addTest(loader.loadTestsFromModule(RangeTransformTest))
    suite.addTest(loader.loadTestsFromModule(TransformsTest))
    suite.addTest(loader.loadTestsFromModule(ViewingRulesTest))
    suite.addTest(loader.loadTestsFromModule(ViewTransformTest))

    return suite


if __name__ == '__main__':
    runner = unittest.TextTestRunner(verbosity=2)
    test_suite = suite()
    result = runner.run(test_suite)
    if result.wasSuccessful() == False:
        sys.exit(1)
    sys.exit(0)

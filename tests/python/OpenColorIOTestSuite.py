# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import logging
import unittest
import os
import sys
import argparse

logging.basicConfig(
    level=logging.INFO,
    format="[%(levelname)s] %(name)s: %(message)s",
)

# A single test suite may be run manually as follows (assuming you are in the typical build dir):
# python ../tests/python/OpenColorIOTestSuite.py $PWD --run_only ColorSpaceTest 

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
        # But add it here unconditionally to allow test to work when OCIO_PYTHON_LOAD_DLLS_FROM_PATH
        # is disabled.
        if sys.version_info >= (3, 8):
            os.add_dll_directory(opencolorio_dir)
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
import GradingHueCurveTransformTest
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

def get_test_modules():
    return [
        ("AllocationTransformTest", AllocationTransformTest),
        ("BakerTest", BakerTest),
        ("BuiltinConfigRegistryTest", BuiltinConfigRegistryTest),
        ("BuiltinTransformRegistryTest", BuiltinTransformRegistryTest),
        ("BuiltinTransformTest", BuiltinTransformTest),
        ("CDLTransformTest", CDLTransformTest),
        ("ColorSpaceHelpersTest", ColorSpaceHelpersTest),
        ("ColorSpaceTest", ColorSpaceTest),
        ("ColorSpaceTransformTest", ColorSpaceTransformTest),
        ("ConfigTest", ConfigTest),
        ("ConstantsTest", ConstantsTest),
        ("ContextTest", ContextTest),
        ("CPUProcessorTest", CPUProcessorTest),
        ("DisplayViewHelpersTest", DisplayViewHelpersTest),
        ("DisplayViewTransformTest", DisplayViewTransformTest),
        ("ExponentTransformTest", ExponentTransformTest),
        ("ExponentWithLinearTransformTest", ExponentWithLinearTransformTest),
        ("ExposureContrastTransformTest", ExposureContrastTransformTest),
        ("FileTransformTest", FileTransformTest),
        ("FileRulesTest", FileRulesTest),
        ("FixedFunctionTransformTest", FixedFunctionTransformTest),
        ("FormatMetadataTest", FormatMetadataTest),
        ("GpuShaderDescTest", GpuShaderDescTest),
        ("GradingDataTest", GradingDataTest),
        ("GradingPrimaryTransformTest", GradingPrimaryTransformTest),
        ("GradingHueCurveTransformTest", GradingHueCurveTransformTest),
        ("GradingRGBCurveTransformTest", GradingRGBCurveTransformTest),
        ("GradingToneTransformTest", GradingToneTransformTest),
        ("GroupTransformTest", GroupTransformTest),
        ("LegacyViewingPipelineTest", LegacyViewingPipelineTest),
        ("LogCameraTransformTest", LogCameraTransformTest),
        ("LogTransformTest", LogTransformTest),
        ("LookTest", LookTest),
        ("LookTransformTest", LookTransformTest),
        ("Lut1DTransformTest", Lut1DTransformTest),
        ("Lut3DTransformTest", Lut3DTransformTest),
        ("MatrixTransformTest", MatrixTransformTest),
        ("MixingHelpersTest", MixingHelpersTest),
        ("NamedTransformTest", NamedTransformTest),
        ("OCIOZArchiveTest", OCIOZArchiveTest),
        ("OpenColorIOTest", OpenColorIOTest),
        ("ProcessorCacheTest", ProcessorCacheTest),
        ("ProcessorTest", ProcessorTest),
        ("RangeTransformTest", RangeTransformTest),
        ("TransformsTest", TransformsTest),
        ("ViewingRulesTest", ViewingRulesTest),
        ("ViewTransformTest", ViewTransformTest),
    ]

def suite(run_only=None):
    """Load unittest.TestCase objects from *Test.py files within ./tests/Python

    :param run_only: Optional name of a single test module to run.
    :return: unittest test suite of TestCase objects.
    :rtype: unittest.TestSuite
    """
    suite = unittest.TestSuite()
    loader = unittest.TestLoader()
    modules = get_test_modules()
    if run_only:
        modules = [m for m in modules if m[0] == run_only]
        if not modules:
            raise ValueError(f"No test module named '{run_only}' found.")
    for name, module in modules:
        suite.addTest(loader.loadTestsFromModule(module))
    return suite

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="OpenColorIO Python Test Suite")
    parser.add_argument('--run_only', type=str, help="Only run the specified test module (e.g., ColorSpaceTest)")
    args, unknown = parser.parse_known_args()
    # Remove argparse args from sys.argv so unittest doesn't get confused
    sys.argv = [sys.argv[0]] + unknown

    try:
        test_suite = suite(run_only=args.run_only)
    except ValueError as e:
        print(e)
        sys.exit(1)

    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(test_suite)
    if not result.wasSuccessful():
        sys.exit(1)
    sys.exit(0)

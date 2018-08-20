
import unittest, os, sys
sys.path.append(os.path.join(sys.argv[1], "src", "pyglue"))
import PyOpenColorIO as OCIO

from MainTest import *
from ConstantsTest import *
from ConfigTest import *
from ContextTest import *
from LookTest import *
from ColorSpaceTest import *
from GpuShaderDescTest import *
from Baker import *
from TransformsTest import *
from RangeTransformTest import *

class FooTest(unittest.TestCase):
    
    def test_interface(self):
        pass

def suite():
    suite = unittest.TestSuite()
    suite.addTest(MainTest("test_interface"))
    suite.addTest(ConstantsTest("test_interface"))
    suite.addTest(ConfigTest("test_interface"))
    suite.addTest(ConfigTest("test_is_editable"))
    suite.addTest(ContextTest("test_interface"))
    suite.addTest(LookTest("test_interface"))
    suite.addTest(ColorSpaceTest("test_interface"))
    suite.addTest(TransformsTest("test_interface"))
    # Processor
    # ProcessorMetadata
    suite.addTest(GpuShaderDescTest("test_interface"))
    suite.addTest(BakerTest("test_interface"))
    # PackedImageDesc
    # PlanarImageDesc
    suite.addTest(RangeTransformTest("test_interface"))
    suite.addTest(RangeTransformTest("test_equality"))
    suite.addTest(RangeTransformTest("test_validation"))
    return suite

if __name__ == '__main__':
    runner = unittest.TextTestRunner(verbosity=2)
    test_suite = suite()
    result = runner.run(test_suite)
    if result.wasSuccessful() == False:
        sys.exit(1)
    sys.exit(0)


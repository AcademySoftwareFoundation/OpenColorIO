
import unittest, os, sys
sys.path.append(os.path.join(sys.argv[1], "src", "pyglue"))
import PyOpenColorIO as OCIO

class ColorSpaceTest(unittest.TestCase):
    
    def test_interface(self):
        cs = OCIO.ColorSpace()
        cs.setName("mynewcolspace")
        self.assertEqual("mynewcolspace", cs.getName())
        cs.setFamily("fam1")
        self.assertEqual("fam1", cs.getFamily())
        cs.setEqualityGroup("match1")
        self.assertEqual("match1", cs.getEqualityGroup())
        cs.setDescription("this is a test")
        self.assertEqual("this is a test", cs.getDescription())
        cs.setBitDepth(OCIO.Constants.BIT_DEPTH_F16)
        self.assertEqual(OCIO.Constants.BIT_DEPTH_F16, cs.getBitDepth())
        cs.setIsData(False)
        self.assertEqual(False, cs.isData())
        cs.setAllocation(OCIO.Constants.ALLOCATION_LG2)
        self.assertEqual(OCIO.Constants.ALLOCATION_LG2, cs.getAllocation())
        cs.setAllocationVars([0.1, 0.2, 0.3])
        self.assertEqual(3, len(cs.getAllocationVars()))
        lt = OCIO.LogTransform()
        lt.setBase(10.0)
        cs.setTransform(lt, OCIO.Constants.COLORSPACE_DIR_TO_REFERENCE)
        ott = cs.getTransform(OCIO.Constants.COLORSPACE_DIR_TO_REFERENCE)
        self.assertEquals(10.0, ott.getBase())

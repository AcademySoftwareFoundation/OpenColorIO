
import unittest, os, sys
sys.path.append(os.path.join(sys.argv[1], "src", "pyglue"))
import PyOpenColorIO as OCIO

class CDLTransformTest(unittest.TestCase):

    def test_interface(self):

        cdl = OCIO.CDLTransform()

        id = cdl.getID();
        self.assertEqual(id, "")
        cdl.setID("abcd")
        self.assertEqual(cdl.getID(), "abcd")

        description = cdl.getDescription();
        self.assertEqual(description, "")
        cdl.setDescription("abcdefg")
        self.assertEqual(cdl.getDescription(), "abcdefg")

        slope  = cdl.getSlope()
        self.assertEqual(len(slope), 3)
        self.assertEqual(slope[0], 1.0)
        self.assertEqual(slope[1], 1.0)
        self.assertEqual(slope[2], 1.0)

        slope[1] = 2.0
        cdl.setSlope(slope)

        slope  = cdl.getSlope()
        self.assertEqual(len(slope), 3)
        self.assertEqual(slope[0], 1.0)
        self.assertEqual(slope[1], 2.0)
        self.assertEqual(slope[2], 1.0)

        offset  = cdl.getOffset()
        self.assertEqual(len(offset), 3)
        self.assertEqual(offset[0], 0.0)
        self.assertEqual(offset[1], 0.0)
        self.assertEqual(offset[2], 0.0)

        offset[1] = 2.0
        cdl.setOffset(offset)

        offset  = cdl.getOffset()
        self.assertEqual(len(offset), 3)
        self.assertEqual(offset[0], 0.0)
        self.assertEqual(offset[1], 2.0)
        self.assertEqual(offset[2], 0.0)

        power  = cdl.getPower()
        self.assertEqual(len(power), 3)
        self.assertEqual(power[0], 1.0)
        self.assertEqual(power[1], 1.0)
        self.assertEqual(power[2], 1.0)

        power[1] = 2.0
        cdl.setPower(power)

        power  = cdl.getPower()
        self.assertEqual(len(power), 3)
        self.assertEqual(power[0], 1.0)
        self.assertEqual(power[1], 2.0)
        self.assertEqual(power[2], 1.0)

        saturation = cdl.getSat()
        self.assertEqual(saturation, 1.0)
        cdl.setSat(2.0)
        self.assertEqual(cdl.getSat(), 2.0)



    def test_equality(self):

        cdl1 = OCIO.CDLTransform()
        self.assertTrue(cdl1.equals(cdl1))

        cdl2 = OCIO.CDLTransform()

        self.assertTrue(cdl1.equals(cdl2))
        self.assertTrue(cdl2.equals(cdl1))

        cdl1.setSat(0.12601234)

        self.assertFalse(cdl1.equals(cdl2))
        self.assertFalse(cdl2.equals(cdl1))

        cdl2.setSat(0.12601234)

        self.assertTrue(cdl1.equals(cdl2))
        self.assertTrue(cdl2.equals(cdl1))


    def test_validation(self):

        cdl1 = OCIO.CDLTransform()
        cdl1.validate()

        cdl1.setSat(1.12)
        cdl1.validate()

        # Test one faulty saturation

        cdl1.setSat(-1.12)
        with self.assertRaises(Exception):
            cdl1.validate()

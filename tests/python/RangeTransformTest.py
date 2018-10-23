
import unittest, os, sys
sys.path.append(os.path.join(sys.argv[1], "src", "pyglue"))
import PyOpenColorIO as OCIO

class RangeTransformTest(unittest.TestCase):

    def test_interface(self):

        rt = OCIO.RangeTransform()
        self.assertEqual(rt.hasMinInValue(), False)
        self.assertEqual(rt.hasMaxInValue(), False)
        self.assertEqual(rt.hasMinOutValue(), False)
        self.assertEqual(rt.hasMaxOutValue(), False)

        rt.setMinInValue(0.12301234)
        self.assertEqual(0.12301234, rt.getMinInValue())
        self.assertEqual(rt.hasMinInValue(), True)

        rt.setMaxInValue(0.12401234)
        self.assertEqual(0.12401234, rt.getMaxInValue())
        self.assertEqual(rt.hasMaxInValue(), True)

        rt.setMinOutValue(0.12501234)
        self.assertEqual(0.12501234, rt.getMinOutValue())
        self.assertEqual(rt.hasMinOutValue(), True)

        rt.setMaxOutValue(0.12601234)
        self.assertEqual(0.12601234, rt.getMaxOutValue())
        self.assertEqual(rt.hasMaxOutValue(), True)

        self.assertEqual(0.12301234, rt.getMinInValue())
        self.assertEqual(0.12401234, rt.getMaxInValue())
        self.assertEqual(0.12501234, rt.getMinOutValue())
        self.assertEqual(0.12601234, rt.getMaxOutValue())

        self.assertEqual(rt.hasMinInValue(),  True)
        self.assertEqual(rt.hasMaxInValue(),  True)
        self.assertEqual(rt.hasMinOutValue(), True)
        self.assertEqual(rt.hasMaxOutValue(), True)

        rt.unsetMinInValue()
        self.assertEqual(rt.hasMinInValue(),  False)
        self.assertEqual(rt.hasMaxInValue(),  True)
        self.assertEqual(rt.hasMinOutValue(), True)
        self.assertEqual(rt.hasMaxOutValue(), True)

        rt.unsetMaxInValue()
        self.assertEqual(rt.hasMinInValue(), False)
        self.assertEqual(rt.hasMaxInValue(), False)
        self.assertEqual(rt.hasMinOutValue(), True)
        self.assertEqual(rt.hasMaxOutValue(), True)

        rt.unsetMinOutValue()
        self.assertEqual(rt.hasMinInValue(),  False)
        self.assertEqual(rt.hasMaxInValue(),  False)
        self.assertEqual(rt.hasMinOutValue(), False)
        self.assertEqual(rt.hasMaxOutValue(), True)

        rt.unsetMaxOutValue()
        self.assertEqual(rt.hasMinInValue(),  False)
        self.assertEqual(rt.hasMaxInValue(),  False)
        self.assertEqual(rt.hasMinOutValue(), False)
        self.assertEqual(rt.hasMaxOutValue(), False)


    def test_equality(self):

        rt1 = OCIO.RangeTransform()
        self.assertTrue(rt1.equals(rt1))

        rt2 = OCIO.RangeTransform()

        self.assertTrue(rt1.equals(rt2))
        self.assertTrue(rt2.equals(rt1))

        rt1.setMinInValue(0.12601234)

        self.assertFalse(rt1.equals(rt2))
        self.assertFalse(rt2.equals(rt1))

        rt2.setMinInValue(0.12601234)

        self.assertTrue(rt1.equals(rt2))
        self.assertTrue(rt2.equals(rt1))


    def test_validation(self):

        rt1 = OCIO.RangeTransform()
        rt1.validate()

        # Test some valid ranges

        rt1.setMinInValue(0.12601234)
        rt1.setMinOutValue(0.12601234)
        rt1.validate()

        rt1.setMaxInValue(1.12601234)
        rt1.setMaxOutValue(1.12601234)
        rt1.validate()

        # Test one faulty range

        rt1.setMaxInValue(0.12601234)
        with self.assertRaises(Exception):
            rt1.validate()

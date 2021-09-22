# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest, os, sys
import PyOpenColorIO as OCIO

class RangeTransformTest(unittest.TestCase):

    def test_range_transform_interface(self):
        """
        Test empty contructor and accessors.
        """
        rt = OCIO.RangeTransform()
        self.assertEqual(rt.getTransformType(), OCIO.TRANSFORM_TYPE_RANGE)

        self.assertEqual(rt.getStyle(), OCIO.RANGE_CLAMP)
        self.assertEqual(rt.hasMinInValue(), False)
        self.assertEqual(rt.hasMaxInValue(), False)
        self.assertEqual(rt.hasMinOutValue(), False)
        self.assertEqual(rt.hasMaxOutValue(), False)

        rt.setStyle(OCIO.RANGE_NO_CLAMP)
        self.assertEqual(rt.getStyle(), OCIO.RANGE_NO_CLAMP)
        rt.setStyle(OCIO.RANGE_CLAMP)

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

    def test_range_transform_equality(self):
        """
        Test range transform equality
        """
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


    def test_range_transform_validation(self):
        """
        Test validate() function.
        """
        rt1 = OCIO.RangeTransform()
        with self.assertRaises(Exception):
            rt1.validate()

        # Test some valid ranges

        rt1.setMinInValue(0.12601234)
        rt1.setMinOutValue(0.12601234)
        rt1.validate()

        rt1.setMaxInValue(1.45)
        rt1.setMaxOutValue(1.78)
        rt1.validate()

        # Test one faulty range (max has to be greater than min)

        rt1.setMaxInValue(0.12601234)
        with self.assertRaises(OCIO.Exception):
            rt1.validate()

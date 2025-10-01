# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest
import os
import sys

import PyOpenColorIO as OCIO
from UnitTestUtils import assertEqualHueCurve

class GradingHueCurveTransformTest(unittest.TestCase):

    valDefaultLin = OCIO.GradingHueCurve(OCIO.GRADING_LIN)
    valDefaultLog = OCIO.GradingHueCurve(OCIO.GRADING_LOG)

    def test_transform_type(self):
        """
        Test the getTransformType() method.
        """
        gct = OCIO.GradingHueCurveTransform()
        self.assertEqual(gct.getTransformType(), OCIO.TRANSFORM_TYPE_GRADING_HUE_CURVE)

    def test_contructor(self):
        """
        Test GradingHueCurveTransform constructor without and with keywords.
        """

        gct = OCIO.GradingHueCurveTransform()
        self.assertEqual(gct.getStyle(), OCIO.GRADING_LOG)
        assertEqualHueCurve(self, gct.getValue(), self.valDefaultLog)
        self.assertEqual(gct.isDynamic(), False)
        self.assertEqual(gct.getRGBToHSY(), OCIO.HSY_TRANSFORM_1)
        self.assertEqual(gct.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)

        gct = OCIO.GradingHueCurveTransform(OCIO.GRADING_LIN)
        self.assertEqual(gct.getStyle(), OCIO.GRADING_LIN)
        assertEqualHueCurve(self, gct.getValue(), self.valDefaultLin)
        self.assertEqual(gct.isDynamic(), False)
        self.assertEqual(gct.getRGBToHSY(), OCIO.HSY_TRANSFORM_1)
        self.assertEqual(gct.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)

        vals = OCIO.GradingHueCurve(OCIO.GRADING_LOG)
        vals.sat_lum = OCIO.GradingBSplineCurve(4, OCIO.SAT_LUM)
        cpts = vals.sat_lum.getControlPoints()
        cpts[0] = OCIO.GradingControlPoint(0.0, 0.1)
        cpts[1] = OCIO.GradingControlPoint(0.1, 0.5)
        cpts[2] = OCIO.GradingControlPoint(0.4, 0.6)
        cpts[3] = OCIO.GradingControlPoint(0.6, 0.7)
        gct = OCIO.GradingHueCurveTransform(style=OCIO.GRADING_VIDEO, values=vals,
                                            dynamic=True, dir=OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(gct.getStyle(), OCIO.GRADING_VIDEO)
        self.assertEqual(gct.isDynamic(), True)
        self.assertEqual(gct.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)
        assertEqualHueCurve(self, gct.getValue(), vals)

    def test_style(self):
        """
        Test setStyle() and getStyle().
        """

        gct = OCIO.GradingHueCurveTransform(OCIO.GRADING_LOG)
        for style in OCIO.GradingStyle.__members__.values():
            gct.setStyle(style)
            self.assertEqual(gct.getStyle(), style)

    def test_misc(self):
        """
        Test miscellaneous getters/setters.
        """

        gct = OCIO.GradingHueCurveTransform(OCIO.GRADING_LOG)
        self.assertEqual(gct.getRGBToHSY(), OCIO.HSY_TRANSFORM_1)
        gct.setRGBToHSY(OCIO.HSY_TRANSFORM_NONE)
        self.assertEqual(gct.getRGBToHSY(), OCIO.HSY_TRANSFORM_NONE)
        self.assertEqual(gct.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        gct.setDirection(OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(gct.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)

    def test_values(self):
        """
        Test setValue() and getValue().
        """

        gct = OCIO.GradingHueCurveTransform(OCIO.GRADING_LOG)
        gct.setValue(self.valDefaultLin)
        assertEqualHueCurve(self, gct.getValue(), self.valDefaultLin)
        self.assertEqual(gct.getValue(), self.valDefaultLin)

    def test_slopes(self):
        """
        Test slopes.
        """

        gct = OCIO.GradingHueCurveTransform(OCIO.GRADING_LOG)
        self.assertEqual(gct.slopesAreDefault(OCIO.HUE_HUE), True)

        gct.setSlope(OCIO.HUE_HUE, 3, 1.1)
        self.assertEqual(gct.slopesAreDefault(OCIO.HUE_HUE), False)
        self.assertEqual(gct.slopesAreDefault(OCIO.HUE_SAT), True)

        s1 = gct.getSlope(OCIO.HUE_HUE, 3)
        self.assertAlmostEqual(s1, 1.1, delta=1e-6)
        with self.assertRaises(OCIO.Exception):
            # Length of slopes must match control points.
            gct.setSlope(OCIO.HUE_HUE, 6, 1.1)

    def test_dynamic(self):
        """
        Test isDynamic() and makeDynamic().
        """

        gct = OCIO.GradingHueCurveTransform(OCIO.GRADING_LOG)
        self.assertEqual(gct.isDynamic(), False)
        gct.makeDynamic()
        self.assertEqual(gct.isDynamic(), True)

    def test_validation(self):
        """
        Test validate() and setValue().
        """

        gct = OCIO.GradingHueCurveTransform(OCIO.GRADING_LOG)
        gct.validate()

        # 3rd control point x is lower than 2nd control point x.
        vals = OCIO.GradingHueCurve(OCIO.GRADING_LOG)
        vals.sat_lum = OCIO.GradingBSplineCurve([0, 0, 0.5, 0.2, 0.2, 0.5, 1, 1], OCIO.SAT_LUM)
        
        with self.assertRaises(OCIO.Exception):
            gct.setValue(vals);

    def test_apply_inverse(self):
        """
        Test applying transform with inversion.
        """

        gct = OCIO.GradingHueCurveTransform(OCIO.GRADING_LOG)
        vals = OCIO.GradingHueCurve(OCIO.GRADING_LOG)
        vals.hue_lum = OCIO.GradingBSplineCurve([0, 2., 0.9, 2.], OCIO.HUE_LUM)
        gct.setValue(vals)
        vals2 = gct.getValue()
        self.assertEqual(vals, vals2)

        cfg = OCIO.Config().CreateRaw()
        proc = cfg.getProcessor(gct)
        cpu = proc.getDefaultCPUProcessor()

        # Apply the transform and keep the result.
        pixel = [0.7, 0.5, 0.1]
        rgb1 = cpu.applyRGB(pixel)

        # The processing did something.
        self.assertAlmostEqual( 0.8, rgb1[0], delta=1e-5)
        self.assertAlmostEqual( 0.6, rgb1[1], delta=1e-5)
        self.assertAlmostEqual( 0.2, rgb1[2], delta=1e-5)

        # Invert.
        gct.setDirection(OCIO.TRANSFORM_DIR_INVERSE)
        proc = cfg.getProcessor(gct)
        cpu = proc.getDefaultCPUProcessor()
        pixel2 = cpu.applyRGB(rgb1)

        # Invert back to original value.
        self.assertAlmostEqual(pixel[0], pixel2[0], delta=1e-5)
        self.assertAlmostEqual(pixel[1], pixel2[1], delta=1e-5)
        self.assertAlmostEqual(pixel[2], pixel2[2], delta=1e-5)

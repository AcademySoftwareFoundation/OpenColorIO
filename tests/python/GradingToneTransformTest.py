# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest
import os
import sys

import PyOpenColorIO as OCIO
from UnitTestUtils import assertEqualTone

class GradingToneTransformTest(unittest.TestCase):

    valsDefaultLog = OCIO.GradingTone(OCIO.GRADING_LOG)
    valsDefaultLin = OCIO.GradingTone(OCIO.GRADING_LIN)

    def test_transform_type(self):
        """
        Test the getTransformType() method.
        """
        gtt = OCIO.GradingToneTransform()
        self.assertEqual(gtt.getTransformType(), OCIO.TRANSFORM_TYPE_GRADING_TONE)

    def test_contructor(self):
        """
        Test GradingToneTransform constructor without and with keywords.
        """

        gtt = OCIO.GradingToneTransform()
        self.assertEqual(gtt.getStyle(), OCIO.GRADING_LOG)
        assertEqualTone(self, gtt.getValue(), self.valsDefaultLog)
        self.assertEqual(gtt.isDynamic(), False)
        self.assertEqual(gtt.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)

        gtt = OCIO.GradingToneTransform(OCIO.GRADING_LIN)
        self.assertEqual(gtt.getStyle(), OCIO.GRADING_LIN)
        assertEqualTone(self, gtt.getValue(), self.valsDefaultLin)
        self.assertEqual(gtt.isDynamic(), False)
        self.assertEqual(gtt.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)

        vals = OCIO.GradingTone(OCIO.GRADING_VIDEO)
        vals.scontrast = 0.1
        gtt = OCIO.GradingToneTransform(style=OCIO.GRADING_VIDEO, values=vals,
                                           dynamic=True, dir=OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(gtt.getStyle(), OCIO.GRADING_VIDEO)
        self.assertEqual(gtt.isDynamic(), True)
        self.assertEqual(gtt.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)
        assertEqualTone(self, gtt.getValue(), vals)

        gtt = OCIO.GradingToneTransform(style=OCIO.GRADING_LOG,
                                           dynamic=False, dir=OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(gtt.getStyle(), OCIO.GRADING_LOG)
        self.assertEqual(gtt.isDynamic(), False)
        self.assertEqual(gtt.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)
        assertEqualTone(self, gtt.getValue(), self.valsDefaultLog)

        # Most values have to be in [0.01, 1.99].
        vals.whites.red = 2.1
        with self.assertRaises(OCIO.Exception):
            OCIO.GradingToneTransform(values=vals)

        # Gamma has to be above lower bound.
        vals.whites.red = 1.1
        vals.midtones.blue = 0.0001
        with self.assertRaises(OCIO.Exception):
            OCIO.GradingToneTransform(values=vals)

    def test_style(self):
        """
        Test setStyle() and getStyle().
        """

        gtt = OCIO.GradingToneTransform(OCIO.GRADING_LOG)
        for style in OCIO.GradingStyle.__members__.values():
            gtt.setStyle(style)
            self.assertEqual(gtt.getStyle(), style)

    def test_values(self):
        """
        Test setValue() and getValue().
        """

        gtt = OCIO.GradingToneTransform(OCIO.GRADING_LOG)
        vals = OCIO.GradingTone(OCIO.GRADING_LOG)
        vals.midtones = OCIO.GradingRGBMSW(1.1, 0.9, 1.2, 1, 0.1, 1.2)
        vals.scontrast = 1.1
        gtt.setValue(vals)
        assertEqualTone(self, gtt.getValue(), vals)
        vals.shadows = OCIO.GradingRGBMSW(1, 1.1, 0.5, 1, 1.2, 0)
        with self.assertRaises(AssertionError):
            assertEqualTone(self, gtt.getValue(), vals)
        gtt.setValue(vals)
        assertEqualTone(self, gtt.getValue(), vals)

    def test_dynamic(self):
        """
        Test isDynamic() and makeDynamic().
        """

        gtt = OCIO.GradingToneTransform(OCIO.GRADING_LOG)
        self.assertEqual(gtt.isDynamic(), False)
        gtt.makeDynamic()
        self.assertEqual(gtt.isDynamic(), True)
        gtt.makeNonDynamic()
        self.assertEqual(gtt.isDynamic(), False)

    def test_validation(self):
        """
        Test validate().
        """

        gtt = OCIO.GradingToneTransform(OCIO.GRADING_LOG)
        gtt.validate()

    def test_apply_inverse(self):
        """
        Test applying transform with inversion.
        """

        gtt = OCIO.GradingToneTransform(OCIO.GRADING_LOG)
        vals = OCIO.GradingTone(OCIO.GRADING_LOG)
        vals.midtones = OCIO.GradingRGBMSW(1.6, 0.5, 1.5, 0.7, 0.1, 1.2)
        vals.scontrast = 1.4
        gtt.setValue(vals)

        cfg = OCIO.Config().CreateRaw()
        proc = cfg.getProcessor(gtt)
        cpu = proc.getDefaultCPUProcessor()

        # Apply the transform and keep the result.
        pixel = [0.48, 0.18, 0.18]
        rgb1 = cpu.applyRGB(pixel)

        # The processing did something.
        self.assertAlmostEqual(0.645454, rgb1[0], delta=1e-5)
        self.assertAlmostEqual(0.076331, rgb1[1], delta=1e-5)
        self.assertAlmostEqual(0.130564, rgb1[2], delta=1e-5)

        # Invert.
        gtt.setDirection(OCIO.TRANSFORM_DIR_INVERSE)
        proc = cfg.getProcessor(gtt)
        cpu = proc.getDefaultCPUProcessor()
        pixel2 = cpu.applyRGB(rgb1)

        # Invert back to original value.
        self.assertAlmostEqual(pixel[0], pixel2[0], delta=1e-5)
        self.assertAlmostEqual(pixel[1], pixel2[1], delta=1e-5)
        self.assertAlmostEqual(pixel[2], pixel2[2], delta=1e-5)

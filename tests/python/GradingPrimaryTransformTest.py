# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest
import os
import sys

import PyOpenColorIO as OCIO
from UnitTestUtils import assertEqualPrimary

class GradingPrimaryTransformTest(unittest.TestCase):

    valsDefault = OCIO.GradingPrimary(OCIO.GRADING_LIN)
    valsDefaultLog = OCIO.GradingPrimary(OCIO.GRADING_LOG)

    def test_transform_type(self):
        """
        Test the getTransformType() method.
        """
        gpt = OCIO.GradingPrimaryTransform()
        self.assertEqual(gpt.getTransformType(), OCIO.TRANSFORM_TYPE_GRADING_PRIMARY)

    def test_contructor(self):
        """
        Test GradingPrimaryTransform constructor without and with keywords.
        """

        gpt = OCIO.GradingPrimaryTransform()
        self.assertEqual(gpt.getStyle(), OCIO.GRADING_LOG)
        assertEqualPrimary(self, gpt.getValue(), self.valsDefaultLog)
        self.assertEqual(gpt.isDynamic(), False)
        self.assertEqual(gpt.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)

        gpt = OCIO.GradingPrimaryTransform(OCIO.GRADING_LIN)
        self.assertEqual(gpt.getStyle(), OCIO.GRADING_LIN)
        assertEqualPrimary(self, gpt.getValue(), self.valsDefault)
        self.assertEqual(gpt.isDynamic(), False)
        self.assertEqual(gpt.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)

        vals = OCIO.GradingPrimary(OCIO.GRADING_LOG)
        vals.clampBlack = 0.1
        gpt = OCIO.GradingPrimaryTransform(style=OCIO.GRADING_VIDEO, values=vals,
                                           dynamic=True, dir=OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(gpt.getStyle(), OCIO.GRADING_VIDEO)
        self.assertEqual(gpt.isDynamic(), True)
        self.assertEqual(gpt.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)
        assertEqualPrimary(self, gpt.getValue(), vals)

        gpt = OCIO.GradingPrimaryTransform(style=OCIO.GRADING_LOG,
                                           dynamic=False, dir=OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(gpt.getStyle(), OCIO.GRADING_LOG)
        self.assertEqual(gpt.isDynamic(), False)
        self.assertEqual(gpt.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)
        assertEqualPrimary(self, gpt.getValue(), self.valsDefaultLog)

        # White has to be bigger than white.
        vals.clampBlack = 0.9
        vals.clampWhite = 0.1
        with self.assertRaises(OCIO.Exception):
            OCIO.GradingPrimaryTransform(values=vals)

        # Gamma has to be above lower bound.
        vals.clampBlack = 0.1
        vals.clampWhite = 0.9
        vals.gamma.blue = 0.001
        with self.assertRaises(OCIO.Exception):
            OCIO.GradingPrimaryTransform(values=vals)

    def test_style(self):
        """
        Test setStyle() and getStyle().
        """

        gpt = OCIO.GradingPrimaryTransform(OCIO.GRADING_LOG)
        for style in OCIO.GradingStyle.__members__.values():
            gpt.setStyle(style)
            self.assertEqual(gpt.getStyle(), style)

    def test_values(self):
        """
        Test setValue() and getValue().
        """

        gpt = OCIO.GradingPrimaryTransform(OCIO.GRADING_LOG)
        vals = OCIO.GradingPrimary(OCIO.GRADING_LOG)
        vals.contrast = OCIO.GradingRGBM(1.1, 0.9, 1.2, 1)
        vals.clampBlack = 0.1
        gpt.setValue(vals)
        assertEqualPrimary(self, gpt.getValue(), vals)
        vals.offset = OCIO.GradingRGBM(0, 0.1, -0.1, 0)
        with self.assertRaises(AssertionError):
            assertEqualPrimary(self, gpt.getValue(), vals)
        gpt.setValue(vals)
        assertEqualPrimary(self, gpt.getValue(), vals)

    def test_dynamic(self):
        """
        Test isDynamic() and makeDynamic().
        """

        gpt = OCIO.GradingPrimaryTransform(OCIO.GRADING_LOG)
        self.assertFalse(gpt.isDynamic())
        gpt.makeDynamic()
        self.assertTrue(gpt.isDynamic())
        gpt.makeNonDynamic()
        self.assertFalse(gpt.isDynamic())

    def test_validation(self):
        """
        Test validate().
        """

        gpt = OCIO.GradingPrimaryTransform(OCIO.GRADING_LOG)
        gpt.validate()

    def test_apply_dynamic(self):
        """
        Test applying transform with dynamic properties.
        """

        gpt1 = OCIO.GradingPrimaryTransform(OCIO.GRADING_LOG)
        gpt1.makeDynamic()
        gpt2 = OCIO.GradingPrimaryTransform(OCIO.GRADING_LOG)
        val2 = OCIO.GradingPrimary(OCIO.GRADING_LOG)
        val2.gamma = OCIO.GradingRGBM(1.2, 1.4, 1.1, 1.0)
        val2.saturation = 1.5
        gpt2.setValue(val2)

        group = OCIO.GroupTransform()
        group.appendTransform(gpt1)
        group.appendTransform(gpt2)

        cfg = OCIO.Config().CreateRaw()
        proc = cfg.getProcessor(group)
        cpu = proc.getDefaultCPUProcessor()

        self.assertTrue(cpu.isDynamic())
        self.assertTrue(cpu.hasDynamicProperty(OCIO.DYNAMIC_PROPERTY_GRADING_PRIMARY))

        dp = cpu.getDynamicProperty(OCIO.DYNAMIC_PROPERTY_GRADING_PRIMARY)
        self.assertEqual(dp.getType(), OCIO.DYNAMIC_PROPERTY_GRADING_PRIMARY)

        # Apply the transform and keep the result.
        pixel = [0.48, 0.18, 0.18]
        rgb1 = cpu.applyRGB(pixel)

        # Change the transform through the dynamic property.
        val1 = OCIO.GradingPrimary(OCIO.GRADING_LOG)
        val1.gamma = OCIO.GradingRGBM(1.1, 1.2, 1.3, 1.0)
        dp.setGradingPrimary(val1)

        # Apply the transform to the same pixel and verify result is different.
        rgb2 = cpu.applyRGB(pixel)
        self.assertNotEqual(rgb1[0], rgb2[0])
        self.assertNotEqual(rgb1[1], rgb2[1])
        self.assertNotEqual(rgb1[2], rgb2[2])

    def test_apply_inverse(self):
        """
        Test applying transform with inversion.
        """

        gpt = OCIO.GradingPrimaryTransform(OCIO.GRADING_LOG)
        val = OCIO.GradingPrimary(OCIO.GRADING_LOG)
        val.gamma = OCIO.GradingRGBM(1.2, 1.4, 1.1, 0.7)
        val.saturation = 1.5
        gpt.setValue(val)

        cfg = OCIO.Config().CreateRaw()
        proc = cfg.getProcessor(gpt)
        cpu = proc.getDefaultCPUProcessor()

        # Apply the transform and keep the result.
        pixel = [0.48, 0.18, 0.18]
        rgb1 = cpu.applyRGB(pixel)

        # The processing did something.
        self.assertAlmostEqual(0.515640, rgb1[0], delta=1e-5)
        self.assertAlmostEqual(0.150299, rgb1[1], delta=1e-5)
        self.assertAlmostEqual(0.051360, rgb1[2], delta=1e-5)

        # Invert.
        gpt.setDirection(OCIO.TRANSFORM_DIR_INVERSE)
        proc = cfg.getProcessor(gpt)
        cpu = proc.getDefaultCPUProcessor()
        pixel2 = cpu.applyRGB(rgb1)

        # Invert back to original value.
        self.assertAlmostEqual(pixel[0], pixel2[0], delta=1e-5)
        self.assertAlmostEqual(pixel[1], pixel2[1], delta=1e-5)
        self.assertAlmostEqual(pixel[2], pixel2[2], delta=1e-5)

# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest
import os
import sys

import PyOpenColorIO as OCIO


class ColorSpaceTransformTest(unittest.TestCase):

    TEST_COLORSPACE = ['abc', 'raw', '$test']

    def test_constructor(self):
        """
        Test ColorSpaceTransform constructor without and with keywords.
        """
        ct = OCIO.ColorSpaceTransform()
        self.assertEqual("", ct.getSrc())
        self.assertEqual("", ct.getDst())
        self.assertEqual(OCIO.TRANSFORM_DIR_FORWARD, ct.getDirection())
        self.assertEqual(True, ct.getDataBypass())

        ct = OCIO.ColorSpaceTransform("src", "dst")
        self.assertEqual("src", ct.getSrc())
        self.assertEqual("dst", ct.getDst())
        self.assertEqual(OCIO.TRANSFORM_DIR_FORWARD, ct.getDirection())
        self.assertEqual(True, ct.getDataBypass())

        ct = OCIO.ColorSpaceTransform("src", "dst", OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual("src", ct.getSrc())
        self.assertEqual("dst", ct.getDst())
        self.assertEqual(OCIO.TRANSFORM_DIR_INVERSE, ct.getDirection())
        self.assertEqual(True, ct.getDataBypass())

        ct = OCIO.ColorSpaceTransform("src", "dst", OCIO.TRANSFORM_DIR_INVERSE, False)
        self.assertEqual("src", ct.getSrc())
        self.assertEqual("dst", ct.getDst())
        self.assertEqual(OCIO.TRANSFORM_DIR_INVERSE, ct.getDirection())
        self.assertEqual(False, ct.getDataBypass())

        ct = OCIO.ColorSpaceTransform(src="src", dst="dst", dataBypass=False)
        self.assertEqual("src", ct.getSrc())
        self.assertEqual("dst", ct.getDst())
        self.assertEqual(OCIO.TRANSFORM_DIR_FORWARD, ct.getDirection())
        self.assertEqual(False, ct.getDataBypass())

        ct = OCIO.ColorSpaceTransform(src="src", dst="dst",
                                      dir=OCIO.TRANSFORM_DIR_INVERSE,
                                      dataBypass=False)
        self.assertEqual("src", ct.getSrc())
        self.assertEqual("dst", ct.getDst())
        self.assertEqual(OCIO.TRANSFORM_DIR_INVERSE, ct.getDirection())
        self.assertEqual(False, ct.getDataBypass())

        # Src & dst can't be omitted and can't be empty.
        with self.assertRaises(OCIO.Exception):
            ct = OCIO.ColorSpaceTransform(dst="dst")
        with self.assertRaises(OCIO.Exception):
            ct = OCIO.ColorSpaceTransform(src="src")
        with self.assertRaises(OCIO.Exception):
            ct = OCIO.ColorSpaceTransform(src="", dst="dst")
        with self.assertRaises(OCIO.Exception):
            ct = OCIO.ColorSpaceTransform(src="src", dst="")
        with self.assertRaises(OCIO.Exception):
            ct = OCIO.ColorSpaceTransform("src", "")

    def test_src(self):
        """
        Test setSrc() and getSrc().
        """
        ct = OCIO.ColorSpaceTransform()
        for src in self.TEST_COLORSPACE:
            ct.setSrc(src)
            self.assertEqual(src, ct.getSrc())

    def test_dst(self):
        """
        Test setDst() and getDst().
        """
        ct = OCIO.ColorSpaceTransform()
        for dst in self.TEST_COLORSPACE:
            ct.setDst(dst)
            self.assertEqual(dst, ct.getDst())

    def test_data_bypass(self):
        """
        Test setDataBypass() and getDataBypass().
        """
        ct = OCIO.ColorSpaceTransform()
        ct.setDataBypass(False)
        self.assertEqual(False, ct.getDataBypass())
        ct.setDataBypass(True)
        self.assertEqual(True, ct.getDataBypass())

    def test_direction(self):
        """
        Test the setDirection() and getDirection() methods.
        """

        ct = OCIO.ColorSpaceTransform()
        for direction in OCIO.TransformDirection.__members__.values():
            # Setting the unknown direction preserves the current direction.
            if direction != OCIO.TRANSFORM_DIR_UNKNOWN:
                ct.setDirection(direction)
                self.assertEqual(direction, ct.getDirection())

    def test_validate(self):
        """
        Test the validate().
        """
        ct = OCIO.ColorSpaceTransform()
        ct.setSrc("src")
        ct.setDst("dst")
        ct.validate()

        # Src has to be non-empty.
        ct.setSrc("")
        with self.assertRaises(OCIO.Exception):
            ct.validate()
        ct.setSrc("src")

        # Dst has to be non-empty.
        ct.setDst("")
        with self.assertRaises(OCIO.Exception):
            ct.validate()
        ct.setDst("dst")

        # Direction can't be unknown.
        for direction in OCIO.TransformDirection.__members__.values():
            ct.setDirection(direction)
            if direction != OCIO.TRANSFORM_DIR_UNKNOWN:
                self.assertIsNone(ct.validate())
            else:
                with self.assertRaises(OCIO.Exception):
                   ct.validate()

# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import logging
import unittest

logger = logging.getLogger(__name__)

try:
    import numpy as np
except ImportError:
    logger.warning(
        "NumPy could not be imported. "
        "Test case will lack significant coverage!"
    )
    np = None

import PyOpenColorIO as OCIO


class Lut1DTransformTest(unittest.TestCase):

    def test_default_constructor(self):
        """
        Test the default constructor.
        """
        lut = OCIO.Lut1DTransform()
        self.assertEqual(lut.getLength(), 2)
        self.assertEqual(lut.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        self.assertEqual(lut.getHueAdjust(), OCIO.HUE_NONE)
        self.assertFalse(lut.getInputHalfDomain())
        self.assertFalse(lut.getOutputRawHalfs())
        self.assertEqual(lut.getInterpolation(), OCIO.INTERP_DEFAULT)
        self.assertEqual(lut.getFileOutputBitDepth(), OCIO.BIT_DEPTH_UNKNOWN)
        r, g, b = lut.getValue(0)
        self.assertEqual([r, g, b], [0, 0, 0])
        r, g, b = lut.getValue(1)
        self.assertEqual([r, g, b], [1, 1, 1])

    def test_direction(self):
        """
        Test the setDirection() and getDirection() methods.
        """

        lut = OCIO.Lut1DTransform()

        for direction in OCIO.TransformDirection.__members__.values():
            lut.setDirection(direction)
            self.assertEqual(lut.getDirection(), direction)

        # Wrong type tests.
        for invalid in (None, 1, 'test'):
            with self.assertRaises(TypeError):
                lut.setDirection(invalid)

    def test_format_metadata(self):
        """
        Test the getFormatMetadata() method.
        """

        lut = OCIO.Lut1DTransform()
        format_metadata = lut.getFormatMetadata()
        self.assertIsInstance(format_metadata, OCIO.FormatMetadata)
        self.assertEqual(format_metadata.getElementName(), 'ROOT')
        self.assertEqual(format_metadata.getName(), '')
        self.assertEqual(format_metadata.getID(), '')
        format_metadata.setName('name')
        format_metadata.setID('id')
        self.assertEqual(format_metadata.getName(), 'name')
        self.assertEqual(format_metadata.getID(), 'id')


    def test_constructor_with_keywords(self):
        """
        Test Lut1DTransform constructor with keywords and validate its values.
        """
        lut = OCIO.Lut1DTransform(
             length=65536,
             inputHalfDomain=True,
             outputRawHalfs=True,
             fileOutputBitDepth=OCIO.BIT_DEPTH_UINT10,
             hueAdjust=OCIO.HUE_DW3,
             interpolation=OCIO.INTERP_BEST,
             direction=OCIO.TRANSFORM_DIR_INVERSE) 
        self.assertEqual(lut.getLength(), 65536)
        self.assertEqual(lut.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(lut.getHueAdjust(), OCIO.HUE_DW3)
        self.assertTrue(lut.getInputHalfDomain())
        self.assertTrue(lut.getOutputRawHalfs())
        self.assertEqual(lut.getInterpolation(), OCIO.INTERP_BEST)
        self.assertEqual(lut.getFileOutputBitDepth(), OCIO.BIT_DEPTH_UINT10)

        lut = OCIO.Lut1DTransform(
             length=4,
             direction=OCIO.TRANSFORM_DIR_INVERSE) 
        self.assertEqual(lut.getLength(), 4)
        self.assertEqual(lut.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(lut.getHueAdjust(), OCIO.HUE_NONE)
        self.assertFalse(lut.getInputHalfDomain())
        self.assertFalse(lut.getOutputRawHalfs())
        self.assertEqual(lut.getInterpolation(), OCIO.INTERP_DEFAULT)
        self.assertEqual(lut.getFileOutputBitDepth(), OCIO.BIT_DEPTH_UNKNOWN)

    def test_constructor_with_positional(self):
        """
        Test Lut1DTransform constructor without keywords and validate its values.
        """
        lut = OCIO.Lut1DTransform(65536, True, True, OCIO.BIT_DEPTH_UINT10,
                                  OCIO.HUE_DW3, OCIO.INTERP_BEST,
                                  OCIO.TRANSFORM_DIR_INVERSE) 
        self.assertEqual(lut.getLength(), 65536)
        self.assertEqual(lut.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(lut.getHueAdjust(), OCIO.HUE_DW3)
        self.assertTrue(lut.getInputHalfDomain())
        self.assertTrue(lut.getOutputRawHalfs())
        self.assertEqual(lut.getInterpolation(), OCIO.INTERP_BEST)
        self.assertEqual(lut.getFileOutputBitDepth(), OCIO.BIT_DEPTH_UINT10)

    def test_array(self):
        """
        Get & set Lut array values.
        """
        lut = OCIO.Lut1DTransform(length=3)
        r, g, b = lut.getValue(0)
        self.assertEqual([r, g, b], [0, 0, 0])
        r, g, b = lut.getValue(1)
        self.assertEqual([r, g, b], [0.5, 0.5, 0.5])
        r, g, b = lut.getValue(2)
        self.assertEqual([r, g, b], [1, 1, 1])

        lut.setValue(0, 0.1, 0.2, 0.3)
        r, g, b = lut.getValue(0)
        # Values are stored as float.
        self.assertAlmostEqual(r, 0.1, delta=1e-6)
        self.assertAlmostEqual(g, 0.2, delta=1e-6)
        self.assertAlmostEqual(b, 0.3, delta=1e-6)

        if not np:
            logger.warning("NumPy not found. Skipping part of test!")
            return

        data = lut.getData()
        expected = np.array([0.1, 0.2, 0.3,
                             0.5, 0.5, 0.5,
                             1., 1., 1.]).astype(np.float32)
        self.assertEqual(data.all(), expected.all())
        data[6] = 0.9
        data[7] = 1.1
        data[8] = 1.2
        lut.setData(data)

        r, g, b = lut.getValue(2)
        self.assertAlmostEqual(r, 0.9, delta=1e-6)
        self.assertAlmostEqual(g, 1.1, delta=1e-6)
        self.assertAlmostEqual(b, 1.2, delta=1e-6)

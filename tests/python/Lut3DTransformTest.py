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

class Lut3DTransformTest(unittest.TestCase):

    def test_default_constructor(self):
        """
        Test the default constructor.
        """
        lut = OCIO.Lut3DTransform()
        self.assertEqual(lut.getGridSize(), 2)
        self.assertEqual(lut.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        self.assertEqual(lut.getInterpolation(), OCIO.INTERP_DEFAULT)
        self.assertEqual(lut.getFileOutputBitDepth(), OCIO.BIT_DEPTH_UNKNOWN)
        r, g, b = lut.getValue(0, 0, 0)
        self.assertEqual([r, g, b], [0, 0, 0])
        r, g, b = lut.getValue(1, 1, 1)
        self.assertEqual([r, g, b], [1, 1, 1])

    def test_direction(self):
        """
        Test the setDirection() and getDirection() methods.
        """

        lut = OCIO.Lut3DTransform()

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

        lut = OCIO.Lut3DTransform()
        format_metadata = lut.getFormatMetadata()
        self.assertIsInstance(format_metadata, OCIO.FormatMetadata)
        self.assertEqual(format_metadata.getElementName(), 'ROOT')
        self.assertEqual(format_metadata.getName(), '')
        self.assertEqual(format_metadata.getID(), '')
        format_metadata.setName('name')
        format_metadata.setID('id')
        self.assertEqual(format_metadata.getName(), 'name')
        self.assertEqual(format_metadata.getID(), 'id')

    def test_file_output_bit_depth(self):
        """
        Test get/setFileOutputBitDepth.
        """
        lut = OCIO.Lut3DTransform()
        self.assertEqual(lut.getFileOutputBitDepth(), OCIO.BIT_DEPTH_UNKNOWN)
        lut.setFileOutputBitDepth(OCIO.BIT_DEPTH_UINT10)
        self.assertEqual(lut.getFileOutputBitDepth(), OCIO.BIT_DEPTH_UINT10)

    def test_grid_size(self):
        """
        Test get/setGridSize.
        """
        lut = OCIO.Lut3DTransform()
        self.assertEqual(lut.getGridSize(), 2)
        lut.setValue(0, 0, 0, 0.1, 0.2, 0.3)
        lut.setGridSize(3)
        self.assertEqual(lut.getGridSize(), 3)
        # Changing the length reset LUT values to identity.
        r, g, b = lut.getValue(0, 0, 0)
        self.assertEqual([r, g, b], [0, 0, 0])

    def test_interpolation(self):
        """
        Test get/setInterpolation.
        """
        lut = OCIO.Lut3DTransform()
        self.assertEqual(lut.getInterpolation(), OCIO.INTERP_DEFAULT)
        lut.setInterpolation(OCIO.INTERP_TETRAHEDRAL)
        self.assertEqual(lut.getInterpolation(), OCIO.INTERP_TETRAHEDRAL)

    def test_constructor_with_keywords(self):
        """
        Test Lut3DTransform constructor with keywords and validate its values.
        """
        lut = OCIO.Lut3DTransform(
             gridSize=5,
             fileOutputBitDepth=OCIO.BIT_DEPTH_UINT10,
             interpolation=OCIO.INTERP_BEST,
             direction=OCIO.TRANSFORM_DIR_INVERSE) 
        self.assertEqual(lut.getGridSize(), 5)
        self.assertEqual(lut.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(lut.getInterpolation(), OCIO.INTERP_BEST)
        self.assertEqual(lut.getFileOutputBitDepth(), OCIO.BIT_DEPTH_UINT10)

        lut = OCIO.Lut3DTransform(
             gridSize=4,
             direction=OCIO.TRANSFORM_DIR_INVERSE) 
        self.assertEqual(lut.getGridSize(), 4)
        self.assertEqual(lut.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(lut.getInterpolation(), OCIO.INTERP_DEFAULT)
        self.assertEqual(lut.getFileOutputBitDepth(), OCIO.BIT_DEPTH_UNKNOWN)

    def test_constructor_with_positional(self):
        """
        Test Lut3DTransform constructor without keywords and validate its values.
        """
        lut = OCIO.Lut3DTransform(10, OCIO.BIT_DEPTH_UINT10, OCIO.INTERP_BEST,
                                  OCIO.TRANSFORM_DIR_INVERSE) 
        self.assertEqual(lut.getGridSize(), 10)
        self.assertEqual(lut.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(lut.getInterpolation(), OCIO.INTERP_BEST)
        self.assertEqual(lut.getFileOutputBitDepth(), OCIO.BIT_DEPTH_UINT10)

    def test_array(self):
        """
        Get & set Lut array values.
        """
        lut = OCIO.Lut3DTransform(gridSize=3)
        r, g, b = lut.getValue(0, 0, 0)
        self.assertEqual([r, g, b], [0, 0, 0])
        r, g, b = lut.getValue(1, 1, 1)
        self.assertEqual([r, g, b], [0.5, 0.5, 0.5])
        r, g, b = lut.getValue(2, 2, 2)
        self.assertEqual([r, g, b], [1, 1, 1])

        lut.setValue(0, 0, 0, 0.1, 0.2, 0.3)
        r, g, b = lut.getValue(0, 0, 0)
        # Values are stored as float.
        self.assertAlmostEqual(r, 0.1, delta=1e-6)
        self.assertAlmostEqual(g, 0.2, delta=1e-6)
        self.assertAlmostEqual(b, 0.3, delta=1e-6)

        if not np:
            logger.warning("NumPy not found. Skipping part of test!")
            return

        data = lut.getData()
        expected = np.array([0.1, 0.2, 0.3, 0.,  0.,   .5, 0.,  0.,  1.,
                             0.,  0.5, 0.,  0.,  0.5, 0.5, 0.,  0.5, 1.,
                             0.,  1.,  0.,  0.,  1.,  0.5, 0.,  1.,  1.,
                             0.5, 0.,  0.,  0.5, 0.,  0.5, 0.5, 0. , 1.,
                             0.5, 0.5, 0.,  0.5, 0.5, 0.5, 0.5, 0.5, 1.,
                             0.5, 1.,  0.,  0.5, 1.,  0.5, 0.5, 1.,  1.,
                             1.,  0.,  0.,  1.,  0.,  0.5, 1.,  0.,  1.,
                             1.,  0.5, 0.,  1.,  0.5, 0.5, 1.,  0.5, 1.,
                             1.,  1.,  0.,  1.,  1.,  0.5, 1.,  1.,  1.]).astype(np.float32)
        self.assertEqual(data.all(), expected.all())
        data[6] = 0.9
        data[7] = 1.1
        data[8] = 1.2
        lut.setData(data)

        r, g, b = lut.getValue(0, 0, 2)
        self.assertAlmostEqual(r, 0.9, delta=1e-6)
        self.assertAlmostEqual(g, 1.1, delta=1e-6)
        self.assertAlmostEqual(b, 1.2, delta=1e-6)

    def test_equals(self):
        """
        Test equals.
        """
        lut = OCIO.Lut3DTransform()
        lut2 = OCIO.Lut3DTransform()
        self.assertTrue(lut.equals(lut2))
        lut.setValue(0, 1, 1, 0.1, 0.2, 0.3)
        self.assertFalse(lut.equals(lut2))

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
from TransformsBaseTest import TransformsBaseTest


class Lut3DTransformTest(unittest.TestCase, TransformsBaseTest):

    def setUp(self):
        self.tr = OCIO.Lut3DTransform()

    def test_default_constructor(self):
        """
        Test the default constructor.
        """

        self.assertEqual(self.tr.getGridSize(), 2)
        self.assertEqual(self.tr.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        self.assertEqual(self.tr.getInterpolation(), OCIO.INTERP_DEFAULT)
        self.assertEqual(self.tr.getFileOutputBitDepth(),
                         OCIO.BIT_DEPTH_UNKNOWN)
        r, g, b = self.tr.getValue(0, 0, 0)
        self.assertEqual([r, g, b], [0, 0, 0])
        r, g, b = self.tr.getValue(1, 1, 1)
        self.assertEqual([r, g, b], [1, 1, 1])

    def test_format_metadata(self):
        """
        Test the getFormatMetadata() method.
        """

        format_metadata = self.tr.getFormatMetadata()
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
        Test the setFileOutputBitDepth() and getFileOutputBitDepth() methods.
        """

        self.assertEqual(self.tr.getFileOutputBitDepth(),
                         OCIO.BIT_DEPTH_UNKNOWN)
        self.tr.setFileOutputBitDepth(OCIO.BIT_DEPTH_UINT10)
        self.assertEqual(self.tr.getFileOutputBitDepth(),
                         OCIO.BIT_DEPTH_UINT10)

    def test_grid_size(self):
        """
        Test the setGridSize() and getGridSize() methods.
        """

        self.assertEqual(self.tr.getGridSize(), 2)
        self.tr.setValue(0, 0, 0, 0.1, 0.2, 0.3)
        self.tr.setGridSize(3)
        self.assertEqual(self.tr.getGridSize(), 3)
        # Changing the length reset LUT values to identity.
        r, g, b = self.tr.getValue(0, 0, 0)
        self.assertEqual([r, g, b], [0, 0, 0])

    def test_interpolation(self):
        """
        Test the setInterpolation() and getInterpolation() methods.
        """

        self.assertEqual(self.tr.getInterpolation(), OCIO.INTERP_DEFAULT)
        self.tr.setInterpolation(OCIO.INTERP_TETRAHEDRAL)
        self.assertEqual(self.tr.getInterpolation(), OCIO.INTERP_TETRAHEDRAL)

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
        Test the setValue() and getValue() methods.
        """

        self.tr = OCIO.Lut3DTransform(gridSize=3)
        r, g, b = self.tr.getValue(0, 0, 0)
        self.assertEqual([r, g, b], [0, 0, 0])
        r, g, b = self.tr.getValue(1, 1, 1)
        self.assertEqual([r, g, b], [0.5, 0.5, 0.5])
        r, g, b = self.tr.getValue(2, 2, 2)
        self.assertEqual([r, g, b], [1, 1, 1])

        self.tr.setValue(0, 0, 0, 0.1, 0.2, 0.3)
        r, g, b = self.tr.getValue(0, 0, 0)
        # Values are stored as float.
        self.assertAlmostEqual(r, 0.1, delta=1e-6)
        self.assertAlmostEqual(g, 0.2, delta=1e-6)
        self.assertAlmostEqual(b, 0.3, delta=1e-6)

        if not np:
            logger.warning("NumPy not found. Skipping part of test!")
            return

        data = self.tr.getData()
        expected = np.array([0.1, 0.2, 0.3, 0.,  0.,   .5, 0.,  0.,  1.,
                             0.,  0.5, 0.,  0.,  0.5, 0.5, 0.,  0.5, 1.,
                             0.,  1.,  0.,  0.,  1.,  0.5, 0.,  1.,  1.,
                             0.5, 0.,  0.,  0.5, 0.,  0.5, 0.5, 0., 1.,
                             0.5, 0.5, 0.,  0.5, 0.5, 0.5, 0.5, 0.5, 1.,
                             0.5, 1.,  0.,  0.5, 1.,  0.5, 0.5, 1.,  1.,
                             1.,  0.,  0.,  1.,  0.,  0.5, 1.,  0.,  1.,
                             1.,  0.5, 0.,  1.,  0.5, 0.5, 1.,  0.5, 1.,
                             1.,  1.,  0.,  1.,  1.,  0.5, 1.,  1.,  1.]).astype(np.float32)
        self.assertEqual(data.all(), expected.all())
        data[6] = 0.9
        data[7] = 1.1
        data[8] = 1.2
        self.tr.setData(data)

        r, g, b = self.tr.getValue(0, 0, 2)
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

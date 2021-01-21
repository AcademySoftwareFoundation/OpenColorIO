# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest

import PyOpenColorIO as OCIO

class MatrixTransformTest(unittest.TestCase):

    def test_interface(self):
        """
        Test constructor and accessors.
        """
        mt = OCIO.MatrixTransform()
        mmt = OCIO.MatrixTransform()
        mt.setMatrix([0.1, 0.2, 0.3, 0.4,
                     0.5, 0.6, 0.7, 0.8,
                     0.9, 1.0, 1.1, 1.2,
                     1.3, 1.4, 1.5, 1.6])
        mt.setOffset([0.1, 0.2, 0.3, 0.4])
        self.assertEqual(False, mt.equals(mmt))
        m44_1 = mt.getMatrix()
        offset_1 = mt.getOffset()
        self.assertAlmostEqual(0.3, m44_1[2], delta=1e-7)
        self.assertAlmostEqual(0.2, offset_1[1], delta=1e-7)
        mfit = mt.Fit([0.1, 0.1, 0.1, 0.1],
                      [0.9, 0.9, 0.9, 0.9],
                      [0.0, 0.0, 0.0, 0.0],
                      [1.1, 1.1, 1.1, 1.1])

        m44_2 = mfit.getMatrix()
        offset_2 = mfit.getOffset()
        self.assertAlmostEqual(1.375, m44_2[0], delta=1e-7)
        mid = mt.Identity()
        m44_3 = mid.getMatrix()
        offset_3 = mid.getOffset()
        self.assertAlmostEqual(0.0, m44_3[1], delta=1e-7)
        msat = mt.Sat(0.5, [0.2126, 0.7152, 0.0722])
        m44_2 = msat.getMatrix()
        offset_2 = msat.getOffset()
        self.assertAlmostEqual(0.3576, m44_2[1], delta=1e-7)
        mscale = mt.Scale([0.9, 0.8, 0.7, 1.])
        m44_2 = mscale.getMatrix()
        offset_2 = mscale.getOffset()
        self.assertAlmostEqual(0.9, m44_2[0], delta=1e-7)
        mview = mt.View([1, 1, 1, 0], [0.2126, 0.7152, 0.0722])
        m44_2 = mview.getMatrix()
        offset_2 = mview.getOffset()
        self.assertAlmostEqual(0.0722, m44_2[2], delta=1e-7)

        mt4 = OCIO.MatrixTransform([0.1, 0.2, 0.3, 0.4,
                                    0.5, 0.6, 0.7, 0.8,
                                    0.9, 1.0, 1.1, 1.2,
                                    1.3, 1.4, 1.5, 1.6],
                                   [0.1, 0.2, 0.3, 0.4],
                                   OCIO.TRANSFORM_DIR_INVERSE)
        m44_4 = mt4.getMatrix()
        offset_4 = mt4.getOffset()
        for i in range(0, 16):
            self.assertAlmostEqual(float(i+1)/10.0, m44_4[i], delta=1e-7)
        for i in range(0, 4):
            self.assertAlmostEqual(float(i+1)/10.0, offset_4[i], delta=1e-7)
        self.assertEqual(mt4.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)

        mt5 = OCIO.MatrixTransform(matrix=[0.1, 0.2, 0.3, 0.4,
                                           0.5, 0.6, 0.7, 0.8,
                                           0.9, 1.0, 1.1, 1.2,
                                           1.3, 1.4, 1.5, 1.6],
                                   offset=[0.1, 0.2, 0.3, 0.4],
                                   direction=OCIO.TRANSFORM_DIR_INVERSE)
        m44_5 = mt5.getMatrix()
        offset_5 = mt5.getOffset()
        for i in range(0, 16):
            self.assertAlmostEqual(float(i+1)/10.0, m44_5[i], delta=1e-7)
        for i in range(0, 4):
            self.assertAlmostEqual(float(i+1)/10.0, offset_5[i], delta=1e-7)
        self.assertEqual(mt5.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)

    def test_direction(self):
        """
        Test the setDirection() and getDirection() methods.
        """

        mt = OCIO.MatrixTransform()

        # Default initialized direction is forward.
        self.assertEqual(mt.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)

        for direction in OCIO.TransformDirection.__members__.values():
            mt.setDirection(direction)
            self.assertEqual(mt.getDirection(), direction)

        # Wrong type tests.
        for invalid in (None, 1, 'test'):
            with self.assertRaises(TypeError):
                mt.setDirection(invalid)


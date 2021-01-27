# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest
import math

import PyOpenColorIO as OCIO


class LogCameraTransformTest(unittest.TestCase):

    LIN_SB = [0.1, 0.2, 0.3]

    def test_constructor(self):
        """
        Test the LogCameraTransform constructors.
        """

        # Required parameter missing.
        with self.assertRaises(TypeError):
            OCIO.LogCameraTransform()

        LIN_SB = [0.1, 0.2, 0.3]
        lct = OCIO.LogCameraTransform(LIN_SB)
        self.assertEqual(lct.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        self.assertEqual(lct.getBase(), 2)
        self.assertEqual(lct.getLogSideSlopeValue(), [1, 1, 1])
        self.assertEqual(lct.getLogSideOffsetValue(), [0, 0, 0])
        self.assertEqual(lct.getLinSideSlopeValue(), [1, 1, 1])
        self.assertEqual(lct.getLinSideOffsetValue(), [0, 0, 0])
        self.assertEqual(lct.getLinSideBreakValue(), LIN_SB)
        self.assertFalse(lct.isLinearSlopeValueSet())
        linearSlope = lct.getLinearSlopeValue()
        self.assertEqual(len(linearSlope), 3)
        for i in range(3):
            self.assertTrue(math.isnan(linearSlope[0]))
        lct.validate()

        BASE = 2.5
        lct = OCIO.LogCameraTransform(LIN_SB, BASE)

        self.assertEqual(lct.getBase(), BASE)

        LOG_SLP = [1.1, 1.2, 1.3]
        lct = OCIO.LogCameraTransform(LIN_SB, BASE, LOG_SLP)
        self.assertEqual(lct.getLogSideSlopeValue(), LOG_SLP)

        LOG_OFF = [0.01, 0.02, 0.03]
        lct = OCIO.LogCameraTransform(LIN_SB, BASE, LOG_SLP, LOG_OFF)
        self.assertEqual(lct.getLogSideOffsetValue(), LOG_OFF)

        LIN_SLP = [1.3, 1.2, 1.1]
        lct = OCIO.LogCameraTransform(LIN_SB, BASE, LOG_SLP,
                                      LOG_OFF, LIN_SLP)
        self.assertEqual(lct.getLinSideSlopeValue(), LIN_SLP)

        LIN_OFF = [0.02, 0.03, 0.01]
        lct = OCIO.LogCameraTransform(LIN_SB, BASE, LOG_SLP, LOG_OFF,
                                      LIN_SLP, LIN_OFF)
        self.assertEqual(lct.getLinSideOffsetValue(), LIN_OFF)

        LINEAR_SLP = [0.9, 0.8, 0.7]
        lct = OCIO.LogCameraTransform(LIN_SB, BASE, LOG_SLP, LOG_OFF,
                                      LIN_SLP, LIN_OFF, LINEAR_SLP)
        self.assertTrue(lct.isLinearSlopeValueSet())
        self.assertEqual(lct.getLinearSlopeValue(), LINEAR_SLP)
        lct.validate()

        lct = OCIO.LogCameraTransform(LIN_SB, BASE, LOG_SLP, LOG_OFF,
                                      LIN_SLP, LIN_OFF, LINEAR_SLP,
                                      OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(lct.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)

        self.assertEqual(str(lct), '<LogCameraTransform direction=inverse, '
                                   'base=2.5, logSideSlope=1.1 1.2 1.3, '
                                   'logSideOffset=0.01 0.02 0.03, linSideSlope=1.3 1.2 1.1, '
                                   'linSideOffset=0.02 0.03 0.01, linSideBreak=0.1 0.2 0.3, '
                                   'linearSlope=0.9 0.8 0.7>')

        LIN_SB = [0.15, 0.2, 0.3]
        BASE = 10
        LOG_SLP = [1.3, 1.2, 1.1]
        LOG_OFF = [0.02, 0, 0.03]
        LIN_SLP = [1.1, 0.9, 1.1]
        LIN_OFF = [0, 0.01, 0.01]
        LINEAR_SLP = [0.8, 0.9, 0.7]
        lct = OCIO.LogCameraTransform(linSideBreak=LIN_SB)
        self.assertEqual(lct.getLinSideBreakValue(), LIN_SB)
        lct = OCIO.LogCameraTransform(linSideBreak=LIN_SB, base=BASE)
        self.assertEqual(lct.getBase(), BASE)
        lct = OCIO.LogCameraTransform(linSideBreak=LIN_SB, logSideSlope=LOG_SLP)
        self.assertEqual(lct.getLogSideSlopeValue(), LOG_SLP)
        lct = OCIO.LogCameraTransform(linSideBreak=LIN_SB, logSideOffset=LOG_OFF)
        self.assertEqual(lct.getLogSideOffsetValue(), LOG_OFF)
        lct = OCIO.LogCameraTransform(linSideBreak=LIN_SB, linSideSlope=LIN_SLP)
        self.assertEqual(lct.getLinSideSlopeValue(), LIN_SLP)
        lct = OCIO.LogCameraTransform(linSideBreak=LIN_SB, linSideOffset=LIN_OFF)
        self.assertEqual(lct.getLinSideOffsetValue(), LIN_OFF)

        lct = OCIO.LogCameraTransform(linSideBreak=LIN_SB, linearSlope=LINEAR_SLP)
        self.assertEqual(lct.getLinearSlopeValue(), LINEAR_SLP)

        LIN_SB=[0.010591, 0.010591, 0.010591]
        LOG_SLP=[0.24719, 0.24719, 0.24719]
        LOG_OFF=[0.385537, 0.385537, 0.385537]
        LIN_SLP=[5.55556, 5.55556, 5.55556]
        LIN_OFF=[0.0522723, 0.0522723, 0.0522723]

        lct = OCIO.LogCameraTransform(linSideOffset=LIN_OFF,
                                      linSideSlope=LIN_SLP,
                                      direction=OCIO.TRANSFORM_DIR_FORWARD,
                                      logSideSlope=LOG_SLP,
                                      logSideOffset=LOG_OFF,
                                      linSideBreak=LIN_SB)
        self.assertEqual(lct.getBase(), 2) # Default value
        self.assertEqual(lct.getLogSideSlopeValue(), LOG_SLP)
        self.assertEqual(lct.getLogSideOffsetValue(), LOG_OFF)
        self.assertEqual(lct.getLinSideSlopeValue(), LIN_SLP)
        self.assertEqual(lct.getLinSideOffsetValue(), LIN_OFF)
        self.assertEqual(lct.getLinSideBreakValue(), LIN_SB)

        lct = OCIO.LogCameraTransform(base=BASE,
                                      linSideBreak=LIN_SB,
                                      logSideSlope=LOG_SLP,
                                      logSideOffset=LOG_OFF,
                                      linSideSlope=LIN_SLP,
                                      linSideOffset=LIN_OFF,
                                      direction=OCIO.TRANSFORM_DIR_FORWARD)
        self.assertEqual(lct.getBase(), BASE)
        self.assertEqual(lct.getLogSideSlopeValue(), LOG_SLP)
        self.assertEqual(lct.getLogSideOffsetValue(), LOG_OFF)
        self.assertEqual(lct.getLinSideSlopeValue(), LIN_SLP)
        self.assertEqual(lct.getLinSideOffsetValue(), LIN_OFF)
        self.assertEqual(lct.getLinSideBreakValue(), LIN_SB)

        # Wrong type tests.

        for invalid in (None, 'test', [0, 0]):
            with self.assertRaises(TypeError):
                OCIO.LogCameraTransform(linSideBreak=invalid)

        for invalid in (None, 'test', [0, 0], [0, 0, 0, 0]):
            with self.assertRaises(TypeError):
                OCIO.LogCameraTransform(linSideBreak=LIN_SB, logSideOffset=invalid)

    def test_transform_type(self):
        """
        Test the getTransformType() method.
        """

        lct = OCIO.LogCameraTransform(self.LIN_SB)
        self.assertEqual(lct.getTransformType(), OCIO.TRANSFORM_TYPE_LOG_CAMERA)

    def test_direction(self):
        """
        Test the setDirection() and getDirection() methods.
        """

        lct = OCIO.LogCameraTransform(self.LIN_SB)

        # Default initialized direction is forward.
        self.assertEqual(lct.getDirection(),
                         OCIO.TRANSFORM_DIR_FORWARD)

        for direction in OCIO.TransformDirection.__members__.values():
            lct.setDirection(direction)
            self.assertEqual(lct.getDirection(), direction)

        # Wrong type tests.
        for invalid in (None, 1, 'test'):
            with self.assertRaises(TypeError):
                lct.setDirection(invalid)

    def test_format_metadata(self):
        """
        Test the getFormatMetadata() method.
        """

        lct = OCIO.LogCameraTransform(self.LIN_SB)
        format_metadata = lct.getFormatMetadata()
        self.assertIsInstance(format_metadata, OCIO.FormatMetadata)
        self.assertEqual(format_metadata.getID(), '')
        self.assertEqual(format_metadata.getName(), '')
        format_metadata.setID('ID')
        format_metadata.setName('Name')

        fm = lct.getFormatMetadata()
        self.assertEqual(fm.getID(), 'ID')
        self.assertEqual(fm.getName(), 'Name')

    def test_base(self):
        """
        Test the setBase() and getBase() methods.
        """

        lct = OCIO.LogCameraTransform(self.LIN_SB)

        # Default initialized base is 2.
        self.assertEqual(lct.getBase(), 2)

        BASE = 2.5
        lct.setBase(BASE)
        self.assertEqual(lct.getBase(), BASE)

        # Wrong type tests.
        for invalid in (None, 'test'):
            with self.assertRaises(TypeError):
                lct.setBase(invalid)

    def test_logSideSlope(self):
        """
        Test the setLogSideSlopeValue() and getLogSideSlopeValue() methods.
        """

        lct = OCIO.LogCameraTransform(self.LIN_SB)

        # Default initialized slope is [1, 1, 1].
        self.assertEqual(lct.getLogSideSlopeValue(), [1, 1, 1])

        LOG_SLP = [1.1, 1.2, 1.3]
        lct.setLogSideSlopeValue(LOG_SLP)
        self.assertEqual(lct.getLogSideSlopeValue(), LOG_SLP)

        # Wrong type tests.
        for invalid in (None, 'test', 1, [1, 1], [1, 1, 1, 1]):
            with self.assertRaises(TypeError):
                lct.setLogSideSlopeValue(invalid)

    def test_logSideOffset(self):
        """
        Test the setLogSideOffsetValue() and getLogSideOffsetValue() methods.
        """

        lct = OCIO.LogCameraTransform(self.LIN_SB)

        # Default initialized offset is [0, 0, 0].
        self.assertEqual(lct.getLogSideOffsetValue(), [0, 0, 0])

        LOG_OFF = [0.1, 0.2, 0.3]
        lct.setLogSideOffsetValue(LOG_OFF)
        self.assertEqual(lct.getLogSideOffsetValue(), LOG_OFF)

        # Wrong type tests.
        for invalid in (None, 'test', 0, [0, 0], [0, 0, 0, 0]):
            with self.assertRaises(TypeError):
                lct.setLogSideOffsetValue(invalid)

    def test_linSideSlope(self):
        """
        Test the setLinSideSlopeValue() and getLinSideSlopeValue() methods.
        """

        lct = OCIO.LogCameraTransform(self.LIN_SB)

        # Default initialized slope is [1, 1, 1].
        self.assertEqual(lct.getLinSideSlopeValue(), [1, 1, 1])

        LIN_SLP = [1.1, 1.2, 1.3]
        lct.setLinSideSlopeValue(LIN_SLP)
        self.assertEqual(lct.getLinSideSlopeValue(), LIN_SLP)

        # Wrong type tests.
        for invalid in (None, 'test', 1, [1, 1], [1, 1, 1, 1]):
            with self.assertRaises(TypeError):
                lct.setLinSideSlopeValue(invalid)

    def test_linSideOffset(self):
        """
        Test the setLinSideOffsetValue() and getLinSideOffsetValue() methods.
        """

        lct = OCIO.LogCameraTransform(self.LIN_SB)

        # Default initialized offset is [0, 0, 0].
        self.assertEqual(lct.getLinSideOffsetValue(), [0, 0, 0])

        LIN_OFF = [0.1, 0.2, 0.3]
        lct.setLinSideOffsetValue(LIN_OFF)
        self.assertEqual(lct.getLinSideOffsetValue(), LIN_OFF)

        # Wrong type tests.
        for invalid in (None, 'test', 0, [0, 0], [0, 0, 0, 0]):
            with self.assertRaises(TypeError):
                lct.setLinSideOffsetValue(invalid)

    def test_linSideBreak(self):
        """
        Test the setLinSideBreakValue() and getLinSideBreakValue() methods.
        """

        LIN_SB = [0.1, 0.1, 0.1]
        lct = OCIO.LogCameraTransform(LIN_SB)
        self.assertEqual(lct.getLinSideBreakValue(), LIN_SB)

        LIN_SB = [0.3, 0.1, 0.2]
        lct.setLinSideBreakValue(LIN_SB)
        self.assertEqual(lct.getLinSideBreakValue(), LIN_SB)

        # Wrong type tests.
        for invalid in (None, 'test', 0, [0, 0], [0, 0, 0, 0]):
            with self.assertRaises(TypeError):
                lct.setLinSideBreakValue(invalid)

    def test_linearSlope(self):
        """
        Test the setLinearSlopeValue(), getLinearSlopeValue(), isLinearSlopeValueSet(),
        and unsetLinearSlopeValue() methods.
        """

        lct = OCIO.LogCameraTransform(self.LIN_SB)

        # Default is not defined.
        self.assertFalse(lct.isLinearSlopeValueSet())

        LINEAR_SLP = [1.1, 1.2, 1.3]
        lct.setLinearSlopeValue(LINEAR_SLP)

        self.assertEqual(lct.getLinearSlopeValue(), LINEAR_SLP)
        self.assertTrue(lct.isLinearSlopeValueSet())

        lct.unsetLinearSlopeValue()
        self.assertFalse(lct.isLinearSlopeValueSet())

        # Wrong type tests.
        for invalid in (None, 'test', 0, [0, 0], [0, 0, 0, 0]):
            with self.assertRaises(TypeError):
                lct.setLinearSlopeValue(invalid)

    def test_validate(self):
        """
        Test the validate() method.
        """

        lct = OCIO.LogCameraTransform(self.LIN_SB)
        lct.validate()

        # Components of logSideSlope can't be 0. 
        lct.setLogSideSlopeValue([0, 0, 0])
        with self.assertRaises(OCIO.Exception):
            lct.validate()
        lct.setLogSideSlopeValue([0.1, 0.1, 0])
        with self.assertRaises(OCIO.Exception):
            lct.validate()
        lct.setLogSideSlopeValue([0.1, 0, 0.1])
        with self.assertRaises(OCIO.Exception):
            lct.validate()
        lct.setLogSideSlopeValue([0, 0.1, 0.1])
        with self.assertRaises(OCIO.Exception):
            lct.validate()

        lct.setLogSideSlopeValue([0.1, 0.1, 0.1])

        # Components of linSideSlope can't be 0. 
        lct.setLinSideSlopeValue([0, 0, 0])
        with self.assertRaises(OCIO.Exception):
            lct.validate()
        lct.setLinSideSlopeValue([0.1, 0.1, 0])
        with self.assertRaises(OCIO.Exception):
            lct.validate()
        lct.setLinSideSlopeValue([0.1, 0, 0.1])
        with self.assertRaises(OCIO.Exception):
            lct.validate()
        lct.setLinSideSlopeValue([0, 0.1, 0.1])
        with self.assertRaises(OCIO.Exception):
            lct.validate()

        lct.setLinSideSlopeValue([0.1, 0.1, 0.1])

        lct = OCIO.LogCameraTransform([0, 0, 0])
        lct.validate()

        lct.setLinearSlopeValue([0, 0, 0])
        lct.validate()

        # Base can't be 0 or 1.
        for invalid in (0, 1):
            lct.setBase(invalid)
            with self.assertRaises(OCIO.Exception):
                lct.validate()


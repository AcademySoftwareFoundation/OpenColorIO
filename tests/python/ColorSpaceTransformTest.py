# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest

import PyOpenColorIO as OCIO
from UnitTestUtils import TEST_NAMES


class ColorSpaceTransformTest(unittest.TestCase):
    TEST_DIRECTION = OCIO.TRANSFORM_DIR_FORWARD
    TEST_SRC = 'foo'
    TEST_DST = 'bar'

    def setUp(self):
        self.cs_tr = OCIO.ColorSpaceTransform()

    def tearDown(self):
        self.cs_tr = None

    def test_transform_type(self):
        """
        Test the getTransformType() method.
        """
        self.assertEqual(self.cs_tr.getTransformType(), OCIO.TRANSFORM_TYPE_COLORSPACE)

    def test_src(self):
        """
        Test the setSrc() and getSrc() methods.
        """

        # Default initialized src value is ""
        self.assertEqual(self.cs_tr.getSrc(), '')

        for name in TEST_NAMES:
            self.cs_tr.setSrc(name)
            self.assertEqual(name, self.cs_tr.getSrc())

        # Wrong type tests.
        for invalid in (None, 1):
            with self.assertRaises(TypeError):
                self.cs_tr.setSrc(invalid)

    def test_dst(self):
        """
        Test the setDst() and getDst() methods.
        """

        # Default initialized src value is ""
        self.assertEqual(self.cs_tr.getDst(), '')

        for name in TEST_NAMES:
            self.cs_tr.setDst(name)
            self.assertEqual(name, self.cs_tr.getDst())

        # Wrong type tests.
        for invalid in (None, 1):
            with self.assertRaises(TypeError):
                self.cs_tr.setDst(invalid)

    def test_direction(self):
        """
        Test the setDirection() and getDirection() methods.
        """

        # Default initialized direction is forward.
        self.assertEqual(self.cs_tr.getDirection(),
                         OCIO.TRANSFORM_DIR_FORWARD)

        for direction in OCIO.TransformDirection.__members__.values():
            self.cs_tr.setDirection(direction)
            self.assertEqual(self.cs_tr.getDirection(), direction)

        # Wrong type tests.
        for invalid in (None, 1):
            with self.assertRaises(TypeError):
                self.cs_tr.setDirection(invalid)

    def test_constructor_with_keyword(self):
        """
        Test ColorSpaceTransform constructor with keywords and validate its values.
        """

        # With keywords in their proper order.
        cs_tr = OCIO.ColorSpaceTransform(src=self.TEST_SRC,
                                         dst=self.TEST_DST,
                                         direction=self.TEST_DIRECTION)

        self.assertEqual(cs_tr.getDirection(), self.TEST_DIRECTION)
        self.assertEqual(cs_tr.getSrc(), self.TEST_SRC)
        self.assertEqual(cs_tr.getDst(), self.TEST_DST)

        # With keywords not in their proper order.
        cs_tr2 = OCIO.ColorSpaceTransform(direction=self.TEST_DIRECTION,
                                          src=self.TEST_SRC,
                                          dst=self.TEST_DST)

        self.assertEqual(cs_tr2.getDirection(), self.TEST_DIRECTION)
        self.assertEqual(cs_tr2.getSrc(), self.TEST_SRC)
        self.assertEqual(cs_tr2.getDst(), self.TEST_DST)

    def test_constructor_with_positional(self):
        """
        Test ColorSpaceTransform constructor without keywords and validate its values.
        """

        cs_tr = OCIO.ColorSpaceTransform(self.TEST_SRC,
                                         self.TEST_DST,
                                         self.TEST_DIRECTION)

        self.assertEqual(cs_tr.getDirection(), self.TEST_DIRECTION)
        self.assertEqual(cs_tr.getSrc(), self.TEST_SRC)
        self.assertEqual(cs_tr.getDst(), self.TEST_DST)

    def test_constructor_wrong_parameter_type(self):
        """
        Test ColorSpaceTransform constructor with a wrong parameter type.
        """

        for invalid in (None, 1):
            with self.assertRaises(TypeError):
                cs_tr = OCIO.ColorSpaceTransform(invalid)

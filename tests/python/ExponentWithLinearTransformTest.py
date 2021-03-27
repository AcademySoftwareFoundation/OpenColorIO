# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest

import PyOpenColorIO as OCIO
from TransformsBaseTest import TransformsBaseTest


class ExponentWithLinearTransformTest(unittest.TestCase, TransformsBaseTest):
    TEST_ID = 'sample exponent linear'
    TEST_GAMMA = [1, 2, 3, 4]
    TEST_OFFSET = [0.1, 0.2, 0.3, 0.4]
    TEST_NEGATIVE_STYLE = OCIO.NEGATIVE_MIRROR
    TEST_DIRECTION = OCIO.TRANSFORM_DIR_INVERSE

    def setUp(self):
        self.tr = OCIO.ExponentWithLinearTransform()

    def test_transform_type(self):
        """
        Test the getTransformType() method.
        """
        self.assertEqual(self.tr.getTransformType(),
                         OCIO.TRANSFORM_TYPE_EXPONENT_WITH_LINEAR)

    def test_format_metadata(self):
        """
        Test the getFormatMetadata() method.
        """

        format_metadata = self.tr.getFormatMetadata()
        format_metadata.setName(self.TEST_ID)
        self.assertIsInstance(format_metadata, OCIO.FormatMetadata)
        self.assertEqual(format_metadata.getName(), self.TEST_ID)

    def test_gamma(self):
        """
        Test the setGamma() and getGamma() methods.
        """

        # Default initialized vars value is [1, 1, 1, 1]
        self.assertEqual(self.tr.getGamma(), [1, 1, 1, 1])

        self.tr.setGamma(self.TEST_GAMMA)
        self.assertEqual(self.tr.getGamma(), self.TEST_GAMMA)

        # Wrong type tests.
        for invalid in (None, 'hello', [1, 2, 3]):
            with self.assertRaises(TypeError):
                self.tr.setGamma(invalid)

    def test_validate_gamma(self):
        """
        Test the validate() method.
        """

        # Validate should pass with default values.
        self.tr.validate()

        # Validate should pass with positive values.
        self.tr.setGamma([1, 2, 3, 4])
        self.tr.validate()

        # Validate should fail with lower bound value of 1.
        self.tr.setGamma([-1, -2, -3, -4])
        with self.assertRaises(OCIO.Exception):
            self.tr.validate()

        # Validate should fail with higher bound value of 10.
        self.tr.setGamma([11, 1, 1, 1])
        with self.assertRaises(OCIO.Exception):
            self.tr.validate()

    def test_negative_style(self):
        """
        Test the setNegativeStyle() and getNegativeStyle() methods.
        """

        # Default initialized negative style is clamp.
        self.assertEqual(self.tr.getNegativeStyle(), OCIO.NEGATIVE_LINEAR)

        # These negative extrapolations are not valid for
        # MonCurve exponent style.
        exception_negatives = [OCIO.NEGATIVE_CLAMP, OCIO.NEGATIVE_PASS_THRU]

        for negative_style in OCIO.NegativeStyle.__members__.values():
            if negative_style not in exception_negatives:
                self.tr.setNegativeStyle(negative_style)
                self.assertEqual(
                    self.tr.getNegativeStyle(), negative_style)
            else:
                with self.assertRaises(OCIO.Exception):
                    self.tr.setNegativeStyle(negative_style)

    def test_offset(self):
        """
        Test the setOffset() and getOffset() methods.
        """

        # Default initialized offset values are [0.0, 0.0, 0.0, 0.0]
        self.assertListEqual(self.tr.getOffset(), [0.0, 0.0, 0.0, 0.0])

        # Test by setting offset values to TEST_OFFSET.
        self.tr.setOffset(self.TEST_OFFSET)
        self.assertListEqual(self.tr.getOffset(), self.TEST_OFFSET)

    def test_validate_offset(self):
        """
        Test the validate() method.
        """

        # Validate should pass with default values.
        self.tr.validate()

        # Validate should pass with positive values.
        self.tr.setOffset([0.1, 0.2, 0.3, 0.4])
        self.tr.validate()

        # Validate should fail with lower bound value of 0.
        self.tr.setOffset([-1, -2, -3, -4])
        with self.assertRaises(OCIO.Exception):
            self.tr.validate()

        # Validate should fail with higher bound value of 0.9.
        self.tr.setOffset([1, 1, 1, 1])
        with self.assertRaises(OCIO.Exception):
            self.tr.validate()

    def test_constructor_with_keyword(self):
        """
        Test ExponentWithLinearTransform constructor with keywords and validate its values.
        """

        # With keywords in their proper order.
        exp_tr = OCIO.ExponentWithLinearTransform(
            gamma=self.TEST_GAMMA,
            offset=self.TEST_OFFSET,
            negativeStyle=self.TEST_NEGATIVE_STYLE,
            direction=self.TEST_DIRECTION)

        self.assertEqual(exp_tr.getGamma(), self.TEST_GAMMA)
        self.assertEqual(exp_tr.getOffset(), self.TEST_OFFSET)
        self.assertEqual(exp_tr.getNegativeStyle(), self.TEST_NEGATIVE_STYLE)
        self.assertEqual(exp_tr.getDirection(), self.TEST_DIRECTION)

        # With keywords not in their proper order.
        exp_tr2 = OCIO.ExponentWithLinearTransform(
            direction=self.TEST_DIRECTION,
            negativeStyle=self.TEST_NEGATIVE_STYLE,
            gamma=self.TEST_GAMMA,
            offset=self.TEST_OFFSET)

        self.assertEqual(exp_tr2.getGamma(), self.TEST_GAMMA)
        self.assertEqual(exp_tr2.getOffset(), self.TEST_OFFSET)
        self.assertEqual(exp_tr2.getNegativeStyle(), self.TEST_NEGATIVE_STYLE)
        self.assertEqual(exp_tr2.getDirection(), self.TEST_DIRECTION)

    def test_constructor_with_positional(self):
        """
        Test ExponentWithLinearTransform constructor without keywords and validate its values.
        """

        exp_tr = OCIO.ExponentWithLinearTransform(
            self.TEST_GAMMA,
            self.TEST_OFFSET,
            self.TEST_NEGATIVE_STYLE,
            self.TEST_DIRECTION)

        self.assertEqual(exp_tr.getGamma(), self.TEST_GAMMA)
        self.assertEqual(exp_tr.getOffset(), self.TEST_OFFSET)
        self.assertEqual(exp_tr.getNegativeStyle(), self.TEST_NEGATIVE_STYLE)
        self.assertEqual(exp_tr.getDirection(), self.TEST_DIRECTION)

    def test_constructor_wrong_parameter_type(self):
        """
        Test ExponentWithLinearTransform constructor with a wrong parameter type.
        """

        for invalid in (None, 1, self.TEST_ID):
            with self.assertRaises(TypeError):
                exp_tr = OCIO.ExponentWithLinearTransform(invalid)

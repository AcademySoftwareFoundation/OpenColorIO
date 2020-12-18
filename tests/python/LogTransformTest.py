# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest

import PyOpenColorIO as OCIO


class LogTransformTest(unittest.TestCase):
    TEST_ID = 'sample log'
    TEST_NAME = 'name of log'
    TEST_BASE = 10
    TEST_DIRECTION = OCIO.TRANSFORM_DIR_INVERSE

    def setUp(self):
        self.log_tr = OCIO.LogTransform()

    def tearDown(self):
        self.log_tr = None

    def test_transform_type(self):
        """
        Test the getTransformType() method.
        """
        self.assertEqual(self.log_tr.getTransformType(), OCIO.TRANSFORM_TYPE_LOG)

    def test_base(self):
        """
        Test the setBase() and getBase() methods.
        """

        # Default initialized base is 2.
        self.assertEqual(self.log_tr.getBase(), 2)

        self.log_tr.setBase(self.TEST_BASE)
        self.assertEqual(self.log_tr.getBase(), self.TEST_BASE)

        # Wrong type tests.
        for invalid in (None, 'test'):
            with self.assertRaises(TypeError):
                self.log_tr.setBase(invalid)

    def test_direction(self):
        """
        Test the setDirection() and getDirection() methods.
        """

        # Default initialized direction is forward.
        self.assertEqual(self.log_tr.getDirection(),
                         OCIO.TRANSFORM_DIR_FORWARD)

        for direction in OCIO.TransformDirection.__members__.values():
            self.log_tr.setDirection(direction)
            self.assertEqual(self.log_tr.getDirection(), direction)

        # Wrong type tests.
        for invalid in (None, 1, 'test'):
            with self.assertRaises(TypeError):
                self.log_tr.setDirection(invalid)

    def test_format_metadata(self):
        """
        Test the getFormatMetadata() method.
        """

        format_metadata = self.log_tr.getFormatMetadata()
        self.assertIsInstance(format_metadata, OCIO.FormatMetadata)
        self.assertEqual(format_metadata.getID(), '')
        self.assertEqual(format_metadata.getName(), '')
        format_metadata.setID(self.TEST_ID)
        format_metadata.setName(self.TEST_NAME)
        self.assertEqual(format_metadata.getID(), self.TEST_ID)
        self.assertEqual(format_metadata.getName(), self.TEST_NAME)

    def test_validate(self):
        """
        Test the validate() method.
        """

        # Validate should pass with default values.
        self.log_tr.validate()

        # Validate should pass with positive values.
        self.log_tr.setBase(5)
        self.log_tr.validate()

        # Fail validation tests.
        for invalid in (1, -1):
            self.log_tr.setBase(invalid)
            with self.assertRaises(OCIO.Exception):
                self.log_tr.validate()

    def test_constructor_with_keyword(self):
        """
        Test LogTransform constructor with keywords and validate its values.
        """

        # With keywords in their proper order.
        log_tr = OCIO.LogTransform(
            base=self.TEST_BASE,
            direction=self.TEST_DIRECTION)

        self.assertEqual(log_tr.getBase(), self.TEST_BASE)
        self.assertEqual(log_tr.getDirection(), self.TEST_DIRECTION)

        # With keywords not in their proper order.
        log_tr2 = OCIO.LogTransform(
            direction=self.TEST_DIRECTION,
            base=self.TEST_BASE)

        self.assertEqual(log_tr2.getBase(), self.TEST_BASE)
        self.assertEqual(log_tr2.getDirection(), self.TEST_DIRECTION)

    def test_constructor_with_positional(self):
        """
        Test LogTransform constructor without keywords and validate its values.
        """

        log_tr = OCIO.LogTransform(
            self.TEST_BASE,
            self.TEST_DIRECTION)

        self.assertEqual(log_tr.getBase(), self.TEST_BASE)
        self.assertEqual(log_tr.getDirection(), self.TEST_DIRECTION)

    def test_constructor_wrong_parameter_type(self):
        """
        Test LogTransform constructor with a wrong parameter type.
        """

        for invalid in (None, 'test'):
            with self.assertRaises(TypeError):
                log_tr = OCIO.LogTransform(invalid)

# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest

import PyOpenColorIO as OCIO


class GroupTransformTest(unittest.TestCase):
    TEST_DIRECTION = OCIO.TRANSFORM_DIR_INVERSE

    def setUp(self):
        self.group_tr = OCIO.GroupTransform()

    def tearDown(self):
        self.group_tr = None

    def test_transform_type(self):
        """
        Test the getTransformType() method.
        """
        self.assertEqual(self.group_tr.getTransformType(), OCIO.TRANSFORM_TYPE_GROUP)

    def test_append_transform(self):
        """
        Test the appendTransform() method.
        """

        # Default GroupTransform length should be 0 without append.
        self.assertEqual(self.group_tr.__len__(), 0)

        matrix_tr = OCIO.MatrixTransform()
        ff_tr = OCIO.FixedFunctionTransform()

        self.group_tr.appendTransform(matrix_tr)
        self.group_tr.appendTransform(ff_tr)

        self.assertEqual(self.group_tr.__len__(), 2)

        iterator = self.group_tr.__iter__()
        for i in [matrix_tr, ff_tr]:
            self.assertEqual(i, next(iterator))

    def test_prepend_transform(self):
        """
        Test the prependTransform() method.
        """

        # Default GroupTransform length should be 0 without prepend.
        self.assertEqual(self.group_tr.__len__(), 0)

        matrix_tr = OCIO.MatrixTransform()
        ff_tr = OCIO.FixedFunctionTransform()

        self.group_tr.prependTransform(matrix_tr)
        self.group_tr.prependTransform(ff_tr)

        self.assertEqual(self.group_tr.__len__(), 2)

        # FixedFunctionTransform goes in front due to prepend.
        iterator = self.group_tr.__iter__()
        for i in [ff_tr, matrix_tr]:
            self.assertEqual(i, next(iterator))

    def test_format_metadata(self):
        """
        Test the getFormatMetadata() method.
        """

        format_metadata = self.group_tr.getFormatMetadata()
        self.assertIsInstance(format_metadata, OCIO.FormatMetadata)
        self.assertEqual(format_metadata.getName(), 'ROOT')

    def test_direction(self):
        """
        Test the setDirection() and getDirection() methods.
        """

        # Default initialized direction is forward.
        self.assertEqual(self.group_tr.getDirection(),
                         OCIO.TRANSFORM_DIR_FORWARD)

        for direction in OCIO.TransformDirection.__members__.values():
            # Setting the unknown direction preserves the current direction.
            if direction != OCIO.TRANSFORM_DIR_UNKNOWN:
                self.group_tr.setDirection(direction)
                self.assertEqual(self.group_tr.getDirection(), direction)

        # Wrong type tests.
        for invalid in (None, 1, 'test'):
            with self.assertRaises(TypeError):
                self.group_tr.setDirection(invalid)

    def test_validate_direction(self):
        """
        Test the validate() method for direction.
        Direction must be forward or inverse.
        """

        self.group_tr.setDirection(OCIO.TRANSFORM_DIR_FORWARD)
        self.assertIsNone(self.group_tr.validate())

    def test_constructor_with_keywords(self):
        """
        Test GroupTransform constructor with keywords and validate its values.
        """

        tr_list = [OCIO.MatrixTransform(), OCIO.FixedFunctionTransform()]

        group_tr = OCIO.GroupTransform(
            transforms=tr_list,
            direction=self.TEST_DIRECTION)

        self.assertEqual(group_tr.__len__(), 2)
        self.assertEqual(group_tr.getDirection(), self.TEST_DIRECTION)

        # With keywords not in their proper order.
        group_tr2 = OCIO.GroupTransform(
            direction=self.TEST_DIRECTION,
            transforms=tr_list)

        self.assertEqual(group_tr2.__len__(), 2)
        self.assertEqual(group_tr2.getDirection(), self.TEST_DIRECTION)

    def test_constructor_with_positional(self):
        """
        Test GroupTransform constructor without keywords and validate its values.
        """

        tr_list = [OCIO.MatrixTransform(), OCIO.FixedFunctionTransform()]

        group_tr = OCIO.GroupTransform(tr_list, self.TEST_DIRECTION)

        self.assertEqual(group_tr.__len__(), 2)
        self.assertEqual(group_tr.getDirection(), self.TEST_DIRECTION)

    def test_constructor_wrong_parameter_type(self):
        """
        Test GroupTransform constructor with a wrong parameter type.
        """

        for invalid in (None, 1):
            with self.assertRaises(TypeError):
                group_tr = OCIO.FixedFunctionTransform(invalid)

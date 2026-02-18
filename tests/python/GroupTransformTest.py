# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest

import PyOpenColorIO as OCIO
from TransformsBaseTest import TransformsBaseTest


class GroupTransformTest(unittest.TestCase, TransformsBaseTest):
    TEST_DIRECTION = OCIO.TRANSFORM_DIR_INVERSE

    def setUp(self):
        self.tr = OCIO.GroupTransform()

    def test_transform_type(self):
        """
        Test the getTransformType() method.
        """
        self.assertEqual(self.tr.getTransformType(), OCIO.TRANSFORM_TYPE_GROUP)

    def test_append_transform(self):
        """
        Test the appendTransform() method.
        """

        # Default GroupTransform length should be 0 without append.
        self.assertEqual(self.tr.__len__(), 0)

        matrix_tr = OCIO.MatrixTransform()
        ff_tr = OCIO.FixedFunctionTransform(
            OCIO.FIXED_FUNCTION_ACES_RED_MOD_03)

        self.tr.appendTransform(matrix_tr)
        self.tr.appendTransform(ff_tr)

        self.assertEqual(self.tr.__len__(), 2)

        iterator = self.tr.__iter__()
        for i in [matrix_tr, ff_tr]:
            self.assertEqual(i, next(iterator))

    def test_prepend_transform(self):
        """
        Test the prependTransform() method.
        """

        # Default GroupTransform length should be 0 without prepend.
        self.assertEqual(self.tr.__len__(), 0)

        matrix_tr = OCIO.MatrixTransform()
        ff_tr = OCIO.FixedFunctionTransform(
            OCIO.FIXED_FUNCTION_ACES_RED_MOD_03)

        self.tr.prependTransform(matrix_tr)
        self.tr.prependTransform(ff_tr)

        self.assertEqual(self.tr.__len__(), 2)

        # FixedFunctionTransform goes in front due to prepend.
        iterator = self.tr.__iter__()
        for i in [ff_tr, matrix_tr]:
            self.assertEqual(i, next(iterator))

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

    def test_validate_direction(self):
        """
        Test the validate() method for direction.
        Direction must be forward or inverse.
        """

        self.tr.setDirection(OCIO.TRANSFORM_DIR_FORWARD)
        self.assertIsNone(self.tr.validate())

    def test_constructor_with_keywords(self):
        """
        Test GroupTransform constructor with keywords and validate its values.
        """

        tr_list = [OCIO.MatrixTransform(),
                   OCIO.FixedFunctionTransform(OCIO.FIXED_FUNCTION_ACES_RED_MOD_03)]

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

        tr_list = [OCIO.MatrixTransform(),
                   OCIO.FixedFunctionTransform(OCIO.FIXED_FUNCTION_ACES_RED_MOD_03)]

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

    def test_write_clf(self):
        """
        Test write().
        """
        config = OCIO.Config.CreateRaw()
        grp = OCIO.GroupTransform()
        mat = OCIO.MatrixTransform()
        mat.setOffset([0.1, 0.2, 0.3, 0])
        fmd = mat.getFormatMetadata()
        fmd.setID('matID')
        fmd.setName('matName')
        fmd.addChildElement('Description', 'Sample matrix.')
        grp.appendTransform(mat)

        range = OCIO.RangeTransform()
        range.setMinInValue(0.1)
        range.setMinOutValue(0.2)
        range.setMaxInValue(1.1)
        range.setMaxOutValue(1.4)
        fmd = range.getFormatMetadata()
        fmd.setID('rangeID')
        fmd.setName('rangeName')
        fmd.addChildElement('Description', 'Sample range.')
        grp.appendTransform(range)

        fmd = grp.getFormatMetadata()
        fmd.setID('clfID')
        fmd.addChildElement('Description', 'Sample clf file.')

        self.assertEqual(grp.write(formatName='Academy/ASC Common LUT Format', config=config),
                         """<?xml version="1.0" encoding="UTF-8"?>
<ProcessList compCLFversion="3" xmlns="http://www.smpte-ra.org/ns/2136-1/2024" id="clfID">
    <Description>Sample clf file.</Description>
    <Matrix id="matID" name="matName" inBitDepth="32f" outBitDepth="32f">
        <Description>Sample matrix.</Description>
        <Array dim="3 4">
                  1                   0                   0                 0.1
                  0                   1                   0                 0.2
                  0                   0                   1                 0.3
        </Array>
    </Matrix>
    <Range id="rangeID" name="rangeName" inBitDepth="32f" outBitDepth="32f">
        <Description>Sample range.</Description>
        <minInValue> 0.1 </minInValue>
        <maxInValue> 1.1 </maxInValue>
        <minOutValue> 0.2 </minOutValue>
        <maxOutValue> 1.4 </maxOutValue>
    </Range>
</ProcessList>
""")

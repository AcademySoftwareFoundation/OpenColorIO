# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest
import os
import sys

import PyOpenColorIO as OCIO
from UnitTestUtils import TEST_DATAFILES_DIR, TEST_NAMES, TEST_DESCS


DEFAULT_CDL = """<ColorCorrection id="">
    <SOPNode>
        <Slope>1 1 1</Slope>
        <Offset>0 0 0</Offset>
        <Power>1 1 1</Power>
    </SOPNode>
    <SatNode>
        <Saturation>1</Saturation>
    </SatNode>
</ColorCorrection>"""


class CDLTransformTest(unittest.TestCase):
    # Default CDL values on initialization.
    DEFAULT_CDL_SLOPE = [1.0, 1.0, 1.0]
    DEFAULT_CDL_OFFSET = [0.0, 0.0, 0.0]
    DEFAULT_CDL_POWER = [1.0, 1.0, 1.0]
    DEFAULT_CDL_SAT = 1.0

    # Values to test setter and getter.
    TEST_CDL_ID = 'shot 042'
    TEST_CDL_DESC = 'Cool look for forest scenes.'
    TEST_CDL_SLOPE = [1.5, 2, 2.5]
    TEST_CDL_OFFSET = [3, 3.5, 4]
    TEST_CDL_POWER = [4.5, 5, 5.5]
    TEST_CDL_SAT = 6
    TEST_CDL = """<ColorCorrection id="{0}">
    <SOPNode>
        <Description>{1}</Description>
        <Slope>{2}</Slope>
        <Offset>{3}</Offset>
        <Power>{4}</Power>
    </SOPNode>
    <SatNode>
        <Saturation>{5}</Saturation>
    </SatNode>
</ColorCorrection>""".format(TEST_CDL_ID,
           TEST_CDL_DESC,
           ' '.join(map(str, TEST_CDL_SLOPE)),
           ' '.join(map(str, TEST_CDL_OFFSET)),
           ' '.join(map(str, TEST_CDL_POWER)),
           str(TEST_CDL_SAT))

    def setUp(self):
        self.cdl_tr = OCIO.CDLTransform()

    def tearDown(self):
        self.cdl_tr = None

    def test_id(self):
        """
        Test the setID() and getID() methods.
        """

        # Default initialized id value is ""
        self.assertEqual(self.cdl_tr.getID(), '')

        # Test id name setter and getter.
        for id_ in TEST_NAMES:
            self.cdl_tr.setID(id_)
            self.assertEqual(id_, self.cdl_tr.getID())

    def test_description(self):
        """
        Test the setDescription() and getDescription() methods.
        """

        # Default initialized description value is ""
        self.assertEqual(self.cdl_tr.getDescription(), '')

        # Test description setter and getter.
        for desc in TEST_DESCS:
            self.cdl_tr.setDescription(desc)
            self.assertEqual(desc, self.cdl_tr.getDescription())

    def test_slope(self):
        """
        Test the setSlope() and getSlope() methods.
        """

        # Default initialized slope values are [1.0, 1.0, 1.0]
        slope = self.cdl_tr.getSlope()
        self.assertEqual(len(slope), 3)
        self.assertListEqual(self.DEFAULT_CDL_SLOPE, slope)

        # Test by setting slope values to TEST_CDL_SLOPE.
        self.cdl_tr.setSlope(self.TEST_CDL_SLOPE)
        slope = self.cdl_tr.getSlope()
        self.assertEqual(len(slope), 3)
        self.assertListEqual(self.TEST_CDL_SLOPE, slope)

    def test_offset(self):
        """
        Test the setOffset() and getOffset() methods.
        """

        # Default initialized offset values are [0.0, 0.0, 0.0]
        offset = self.cdl_tr.getOffset()
        self.assertEqual(len(offset), 3)
        self.assertListEqual(self.DEFAULT_CDL_OFFSET, offset)

        # Test by setting offset values to TEST_CDL_OFFSET.
        self.cdl_tr.setOffset(self.TEST_CDL_OFFSET)
        offset = self.cdl_tr.getOffset()
        self.assertEqual(len(offset), 3)
        self.assertListEqual(self.TEST_CDL_OFFSET, offset)

    def test_power(self):
        """
        Test the setPower() and getPower() methods.
        """

        # Default initialized power values are [0.0, 0.0, 0.0]
        power = self.cdl_tr.getPower()
        self.assertEqual(len(power), 3)
        self.assertListEqual(self.DEFAULT_CDL_POWER, power)

        # Test by setting power values to TEST_CDL_POWER.
        self.cdl_tr.setPower(self.TEST_CDL_POWER)
        power = self.cdl_tr.getPower()
        self.assertEqual(len(power), 3)
        self.assertListEqual(self.TEST_CDL_POWER, power)

    def test_saturation(self):
        """
        Test the setSat() and getSat() methods.
        """

        # Default initialized saturation value is 1.0
        saturation = self.cdl_tr.getSat()
        self.assertEqual(self.DEFAULT_CDL_SAT, saturation)

        # Test by setting saturation value to TEST_CDL_SAT.
        self.cdl_tr.setSat(self.TEST_CDL_SAT)
        self.assertEqual(self.cdl_tr.getSat(), self.TEST_CDL_SAT)

    def test_direction(self):
        """
        Test the setDirection() and getDirection() methods.
        """

        # Default initialized direction is forward.
        self.assertEqual(self.cdl_tr.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)

        for direction in OCIO.TransformDirection.__members__.values():
            # Setting the unknown direction preserves the current direction.
            if direction != OCIO.TRANSFORM_DIR_UNKNOWN:
                self.cdl_tr.setDirection(direction)
                self.assertEqual(self.cdl_tr.getDirection(), direction)

    def test_style(self):
        """
        Test the setStyle() and getStyle() methods.
        """

        # Default initialized direction is forward.
        self.assertEqual(self.cdl_tr.getStyle(), OCIO.CDL_NO_CLAMP)

        for style in OCIO.CDLStyle.__members__.values():
            self.cdl_tr.setStyle(style)
            self.assertEqual(self.cdl_tr.getStyle(), style)

    def test_xml(self):
        """
        Test the setXML() and getXML() methods.
        """

        self.cdl_tr.setXML(self.TEST_CDL)
        self.assertEqual(self.cdl_tr.getID(), self.TEST_CDL_ID)
        self.assertEqual(self.cdl_tr.getDescription(), self.TEST_CDL_DESC)
        self.assertListEqual(self.cdl_tr.getSlope(), self.TEST_CDL_SLOPE)
        self.assertListEqual(self.cdl_tr.getOffset(), self.TEST_CDL_OFFSET)
        self.assertListEqual(self.cdl_tr.getPower(), self.TEST_CDL_POWER)
        self.assertEqual(self.cdl_tr.getSat(), self.TEST_CDL_SAT)

        # Test the XML produced by this CDLTransform object.
        self.assertEqual(self.cdl_tr.getXML(), self.TEST_CDL)

    def test_createfromfile_cc(self):
        """
        Test CreateFromFile() method with a cc file.
        """

        # Try env var first to get test file path.
        test_file = '%s/cdl_test1.cc' % TEST_DATAFILES_DIR

        # Test cc file.
        cdl = OCIO.CDLTransform.CreateFromFile(test_file, 'foo')
        self.assertEqual(cdl.getID(), 'foo')
        self.assertEqual(cdl.getDescription(), 'this is a description')
        self.assertListEqual(cdl.getSlope(), [1.1, 1.2, 1.3])
        self.assertListEqual(cdl.getOffset(), [2.1, 2.2, 2.3])
        self.assertListEqual(cdl.getPower(), [3.1, 3.2, 3.3])
        self.assertEqual(cdl.getSat(), 0.7)

    def test_createfromfile_ccc(self):
        """
        Test CreateFromFile() method with a ccc file.
        """

        # Try env var first to get test file path.
        test_file = '%s/cdl_test1.ccc' % TEST_DATAFILES_DIR

        # Test 4th member of the ccc file.
        cdl1 = OCIO.CDLTransform.CreateFromFile(test_file, '3')
        self.assertEqual(cdl1.getID(), '')
        self.assertListEqual(cdl1.getSlope(), [4.0, 5.0, 6.0])
        self.assertListEqual(cdl1.getOffset(), [0.0, 0.0, 0.0])
        self.assertListEqual(cdl1.getPower(), [0.9, 1.0, 1.2])
        self.assertEqual(cdl1.getSat(), 1.0)

        # Test a specified id member of the ccc file.
        cdl2 = OCIO.CDLTransform.CreateFromFile(test_file, 'cc0003')
        self.assertEqual(cdl2.getID(), 'cc0003')
        self.assertEqual(cdl2.getDescription(), 'golden')
        self.assertListEqual(cdl2.getSlope(), [1.2, 1.1, 1.0])
        self.assertListEqual(cdl2.getOffset(), [0.0, 0.0, 0.0])
        self.assertListEqual(cdl2.getPower(), [0.9, 1.0, 1.2])
        self.assertEqual(cdl2.getSat(), 1.0)

    def test_createfromfile_cdl(self):
        """
        Test CreateFromFile() method with a cdl file.
        """

        # Try env var first to get test file path.
        test_file = '%s/cdl_test1.cdl' % TEST_DATAFILES_DIR

        # Test a specified id member of the cdl file.
        cdl = OCIO.CDLTransform.CreateFromFile(test_file, 'cc0003')
        self.assertEqual(cdl.getID(), 'cc0003')
        self.assertEqual(cdl.getDescription(), 'golden')
        self.assertListEqual(cdl.getSlope(), [1.2, 1.1, 1.0])
        self.assertListEqual(cdl.getOffset(), [0.0, 0.0, 0.0])
        self.assertListEqual(cdl.getPower(), [0.9, 1.0, 1.2])
        self.assertEqual(cdl.getSat(), 1.0)


    def test_validate_slope(self):
        """
        Test the validate() method for slope values. Values must be above 0.
        """

        self.cdl_tr.setSlope(self.TEST_CDL_SLOPE)
        self.assertIsNone(self.cdl_tr.validate())

        # Exception validation test.
        self.cdl_tr.setSlope([-1, -2, -3])
        with self.assertRaises(OCIO.Exception):
            self.cdl_tr.validate()

    def test_validate_saturation(self):
        """
        Test the validate() method for saturation value. Value must be above 0.
        """

        self.cdl_tr.setSat(self.TEST_CDL_SAT)
        self.assertIsNone(self.cdl_tr.validate())

        # Exception validation test
        self.cdl_tr.setSat(-0.1)
        with self.assertRaises(OCIO.Exception):
            self.cdl_tr.validate()

    def test_validate_power(self):
        """
        Test the validate() method for power values. Values must be above 0.
        """

        self.cdl_tr.setPower(self.TEST_CDL_POWER)
        self.assertIsNone(self.cdl_tr.validate())

        # Exception validation test
        self.cdl_tr.setPower([-1, -2, -3])
        with self.assertRaises(OCIO.Exception):
            self.cdl_tr.validate()

    def test_validate_direction(self):
        """
        Test the validate() method for direction.
        Direction must be forward or inverse.
        """

        self.cdl_tr.setDirection(OCIO.TRANSFORM_DIR_FORWARD)
        self.assertIsNone(self.cdl_tr.validate())

        # As the CDL Transform does not support the unknown direction,
        # it preserves the original direction.
        self.cdl_tr.setDirection(OCIO.TRANSFORM_DIR_UNKNOWN)
        self.assertIsNone(self.cdl_tr.validate())

    def test_equality(self):
        """
        Test the equals() method.
        """

        cdl1 = OCIO.CDLTransform()
        self.assertTrue(cdl1.equals(cdl1))

        cdl2 = OCIO.CDLTransform()
        self.assertTrue(cdl1.equals(cdl2))
        self.assertTrue(cdl2.equals(cdl1))

        cdl1.setSat(0.12601234)
        self.assertFalse(cdl1.equals(cdl2))
        self.assertFalse(cdl2.equals(cdl1))

        cdl2.setSat(0.12601234)
        self.assertTrue(cdl1.equals(cdl2))
        self.assertTrue(cdl2.equals(cdl1))

    def test_constructor_with_keyword(self):
        """
        Test CDLTransform constructor with keywords and validate its values.
        """

        # With keywords in their proper order.
        cdl_tr = OCIO.CDLTransform(slope=self.TEST_CDL_SLOPE,
                                   offset=self.TEST_CDL_OFFSET,
                                   power=self.TEST_CDL_POWER,
                                   sat=self.TEST_CDL_SAT,
                                   id=self.TEST_CDL_ID,
                                   desc=self.TEST_CDL_DESC)

        self.assertEqual(self.TEST_CDL_ID, cdl_tr.getID())
        self.assertEqual(self.TEST_CDL_DESC, cdl_tr.getDescription())
        self.assertEqual(self.TEST_CDL_SLOPE, cdl_tr.getSlope())
        self.assertEqual(self.TEST_CDL_OFFSET, cdl_tr.getOffset())
        self.assertEqual(self.TEST_CDL_POWER, cdl_tr.getPower())
        self.assertEqual(self.TEST_CDL_SAT, cdl_tr.getSat())
        self.assertEqual(self.TEST_CDL, cdl_tr.getXML())

        # With keyword not in their proper order.
        cdl_tr2 = OCIO.CDLTransform(id=self.TEST_CDL_ID,
                                    desc=self.TEST_CDL_DESC,
                                    slope=self.TEST_CDL_SLOPE,
                                    offset=self.TEST_CDL_OFFSET,
                                    power=self.TEST_CDL_POWER,
                                    sat=self.TEST_CDL_SAT)

        self.assertEqual(self.TEST_CDL_ID, cdl_tr2.getID())
        self.assertEqual(self.TEST_CDL_DESC, cdl_tr2.getDescription())
        self.assertEqual(self.TEST_CDL_SLOPE, cdl_tr2.getSlope())
        self.assertEqual(self.TEST_CDL_OFFSET, cdl_tr2.getOffset())
        self.assertEqual(self.TEST_CDL_POWER, cdl_tr2.getPower())
        self.assertEqual(self.TEST_CDL_SAT, cdl_tr2.getSat())
        self.assertEqual(self.TEST_CDL, cdl_tr2.getXML())

    def test_constructor_without_keyword(self):
        """
        Test CDLTransform constructor without keywords and validate its values.
        """

        cdl_tr = OCIO.CDLTransform(self.TEST_CDL_SLOPE,
                                   self.TEST_CDL_OFFSET,
                                   self.TEST_CDL_POWER,
                                   self.TEST_CDL_SAT,
                                   self.TEST_CDL_ID,
                                   self.TEST_CDL_DESC)

        self.assertEqual(self.TEST_CDL_ID, cdl_tr.getID())
        self.assertEqual(self.TEST_CDL_DESC, cdl_tr.getDescription())
        self.assertEqual(self.TEST_CDL_SLOPE, cdl_tr.getSlope())
        self.assertEqual(self.TEST_CDL_OFFSET, cdl_tr.getOffset())
        self.assertEqual(self.TEST_CDL_POWER, cdl_tr.getPower())
        self.assertEqual(self.TEST_CDL_SAT, cdl_tr.getSat())
        self.assertEqual(self.TEST_CDL, cdl_tr.getXML())

    def test_constructor_without_parameter(self):
        """
        Test CDLTransform default constructor and validate its values.
        """

        cdl_tr = OCIO.CDLTransform()

        self.assertEqual(cdl_tr.getID(), '')
        self.assertEqual(cdl_tr.getDescription(), '')
        self.assertEqual(cdl_tr.getSlope(), [1.0, 1.0, 1.0])
        self.assertEqual(cdl_tr.getOffset(), [0.0, 0.0, 0.0])
        self.assertEqual(cdl_tr.getPower(), [1.0, 1.0, 1.0])
        self.assertEqual(cdl_tr.getSat(), 1.0)
        self.assertEqual(cdl_tr.getXML(), DEFAULT_CDL)

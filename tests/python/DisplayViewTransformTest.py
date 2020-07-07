# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest
import os
import sys

import PyOpenColorIO as OCIO
from UnitTestUtils import TEST_DATAFILES_DIR, TEST_NAMES, TEST_DESCS


class DisplayViewTransformTest(unittest.TestCase):

    TEST_SRC     = ['abc', 'raw', '$test']
    TEST_DISPLAY = ['display1', 'display2']
    TEST_VIEW    = ['view1', 'view2', 'film']

    def setUp(self):
        self.dv_tr = OCIO.DisplayViewTransform()

    def tearDown(self):
        self.dv_tr = None

    def test_src(self):
        """
        Test the setSrc() and getSrc() methods.
        """

        # Default initialized src value is "".
        self.assertEqual('', self.dv_tr.getSrc())

        # Test src setter and getter.
        for src in self.TEST_SRC:
            self.dv_tr.setSrc(src)
            self.assertEqual(src, self.dv_tr.getSrc())

    def test_display(self):
        """
        Test the setDisplay() and getDisplay() methods.
        """

        # Default initialized display value is ""
        self.assertEqual('', self.dv_tr.getDisplay())

        # Test display setter and getter.
        for display in self.TEST_DISPLAY:
            self.dv_tr.setDisplay(display)
            self.assertEqual(display, self.dv_tr.getDisplay())

    def test_view(self):
        """
        Test the setView() and getView() methods.
        """

        # Default initialized view value is "".
        self.assertEqual('', self.dv_tr.getView())

        # Test view setter and getter.
        for view in self.TEST_VIEW:
            self.dv_tr.setView(view)
            self.assertEqual(view, self.dv_tr.getView())

    def test_looksBypass(self):
        """
        Test the setLooksBypass() and getLooksBypass() methods.
        """

        # Default initialized lookBypass value is False.
        self.assertEqual(False, self.dv_tr.getLooksBypass())

        # Test lookBypass setter and getter.
        self.dv_tr.setLooksBypass(True)
        self.assertEqual(True, self.dv_tr.getLooksBypass())
        self.dv_tr.setLooksBypass(False)
        self.assertEqual(False, self.dv_tr.getLooksBypass())

    def test_dataBypass(self):
        """
        Test the setDataBypass() and getDataBypass() methods.
        """

        # Default initialized dataBypass value is True.
        self.assertEqual(True, self.dv_tr.getDataBypass())

        # Test dataBypass setter and getter.
        self.dv_tr.setDataBypass(False)
        self.assertEqual(False, self.dv_tr.getDataBypass())
        self.dv_tr.setDataBypass(True)
        self.assertEqual(True, self.dv_tr.getDataBypass())


    def test_direction(self):
        """
        Test the setDirection() and getDirection() methods.
        """

        # Default initialized direction is forward.
        self.assertEqual(OCIO.TRANSFORM_DIR_FORWARD, self.dv_tr.getDirection())

        for direction in OCIO.TransformDirection.__members__.values():
            # Setting the unknown direction preserves the current direction.
            if direction != OCIO.TRANSFORM_DIR_UNKNOWN:
                self.dv_tr.setDirection(direction)
                self.assertEqual(direction, self.dv_tr.getDirection())

    def test_validate_src(self):
        """
        Test the validate() method for src value. Value must not be empty.
        """

        self.dv_tr.setSrc('source')
        self.dv_tr.setDisplay('display')
        self.dv_tr.setView('view')
        self.assertIsNone(self.dv_tr.validate())

        # Exception validation test.
        self.dv_tr.setSrc('')
        with self.assertRaises(OCIO.Exception):
            self.dv_tr.validate()

    def test_validate_display(self):
        """
        Test the validate() method for display value. Value must not be empty.
        """

        self.dv_tr.setSrc('source')
        self.dv_tr.setDisplay('display')
        self.dv_tr.setView('view')
        self.assertIsNone(self.dv_tr.validate())

        # Exception validation test.
        self.dv_tr.setDisplay('')
        with self.assertRaises(OCIO.Exception):
            self.dv_tr.validate()

    def test_validate_view(self):
        """
        Test the validate() method for view value. Value must not be empty.
        """

        self.dv_tr.setSrc('source')
        self.dv_tr.setDisplay('display')
        self.dv_tr.setView('view')
        self.assertIsNone(self.dv_tr.validate())

        # Exception validation test.
        self.dv_tr.setView('')
        with self.assertRaises(OCIO.Exception):
            self.dv_tr.validate()


    def test_validate_direction(self):
        """
        Test the validate() method for direction.
        Direction must be forward or inverse.
        """

        self.dv_tr.setSrc('source')
        self.dv_tr.setDisplay('display')
        self.dv_tr.setView('view')

        for direction in OCIO.TransformDirection.__members__.values():
            self.dv_tr.setDirection(direction)
            if direction != OCIO.TRANSFORM_DIR_UNKNOWN:
                self.assertIsNone(self.dv_tr.validate())
            else:
                with self.assertRaises(OCIO.Exception):
                   self.dv_tr.validate()

    def test_constructor_with_keyword(self):
        """
        Test DisplayViewTransform constructor with keywords and validate its values.
        """

        # With keywords in their proper order.
        dv_tr = OCIO.DisplayViewTransform(src=self.TEST_SRC[0],
                                          display=self.TEST_DISPLAY[0],
                                          view=self.TEST_VIEW[0],
                                          looksBypass=True,
                                          dataBypass=False,
                                          direction=OCIO.TRANSFORM_DIR_INVERSE)

        self.assertEqual(self.TEST_SRC[0], dv_tr.getSrc())
        self.assertEqual(self.TEST_DISPLAY[0], dv_tr.getDisplay())
        self.assertEqual(self.TEST_VIEW[0], dv_tr.getView())
        self.assertEqual(True, dv_tr.getLooksBypass())
        self.assertEqual(False, dv_tr.getDataBypass())
        self.assertEqual(OCIO.TRANSFORM_DIR_INVERSE, dv_tr.getDirection())

        # With keyword not in their proper order.
        dv_tr2 = OCIO.DisplayViewTransform(direction=OCIO.TRANSFORM_DIR_FORWARD,
                                           view=self.TEST_VIEW[1],
                                           src=self.TEST_SRC[1],
                                           looksBypass=True,
                                           display=self.TEST_DISPLAY[1],
                                           dataBypass=True)

        self.assertEqual(self.TEST_SRC[1], dv_tr2.getSrc())
        self.assertEqual(self.TEST_DISPLAY[1], dv_tr2.getDisplay())
        self.assertEqual(self.TEST_VIEW[1], dv_tr2.getView())
        self.assertEqual(True, dv_tr2.getLooksBypass())
        self.assertEqual(True, dv_tr2.getDataBypass())
        self.assertEqual(OCIO.TRANSFORM_DIR_FORWARD, dv_tr2.getDirection())

    def test_constructor_without_keyword(self):
        """
        Test DisplayViewTransform constructor without keywords and validate its values.
        """

        dv_tr = OCIO.DisplayViewTransform(self.TEST_SRC[0],
                                          self.TEST_DISPLAY[0],
                                          self.TEST_VIEW[0],
                                          True,
                                          False,
                                          OCIO.TRANSFORM_DIR_INVERSE)

        self.assertEqual(self.TEST_SRC[0], dv_tr.getSrc())
        self.assertEqual(self.TEST_DISPLAY[0], dv_tr.getDisplay())
        self.assertEqual(self.TEST_VIEW[0], dv_tr.getView())
        self.assertEqual(True, dv_tr.getLooksBypass())
        self.assertEqual(False, dv_tr.getDataBypass())
        self.assertEqual(OCIO.TRANSFORM_DIR_INVERSE, dv_tr.getDirection())

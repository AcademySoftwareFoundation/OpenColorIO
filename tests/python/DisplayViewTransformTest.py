# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest

import PyOpenColorIO as OCIO
from UnitTestUtils import TEST_DATAFILES_DIR, TEST_NAMES, TEST_DESCS
from TransformsBaseTest import TransformsBaseTest


class DisplayViewTransformTest(unittest.TestCase, TransformsBaseTest):

    TEST_SRC = ['abc', 'raw', '$test']
    TEST_DISPLAY = ['display1', 'display2']
    TEST_VIEW = ['view1', 'view2', 'film']

    def setUp(self):
        self.tr = OCIO.DisplayViewTransform()

    def test_transform_type(self):
        """
        Test the getTransformType() method.
        """
        self.assertEqual(self.tr.getTransformType(),
                         OCIO.TRANSFORM_TYPE_DISPLAY_VIEW)

    def test_src(self):
        """
        Test the setSrc() and getSrc() methods.
        """

        # Default initialized src value is "".
        self.assertEqual('', self.tr.getSrc())

        # Test src setter and getter.
        for src in self.TEST_SRC:
            self.tr.setSrc(src)
            self.assertEqual(src, self.tr.getSrc())

    def test_display(self):
        """
        Test the setDisplay() and getDisplay() methods.
        """

        # Default initialized display value is ""
        self.assertEqual('', self.tr.getDisplay())

        # Test display setter and getter.
        for display in self.TEST_DISPLAY:
            self.tr.setDisplay(display)
            self.assertEqual(display, self.tr.getDisplay())

    def test_view(self):
        """
        Test the setView() and getView() methods.
        """

        # Default initialized view value is "".
        self.assertEqual('', self.tr.getView())

        # Test view setter and getter.
        for view in self.TEST_VIEW:
            self.tr.setView(view)
            self.assertEqual(view, self.tr.getView())

    def test_looksBypass(self):
        """
        Test the setLooksBypass() and getLooksBypass() methods.
        """

        # Default initialized lookBypass value is False.
        self.assertEqual(False, self.tr.getLooksBypass())

        # Test lookBypass setter and getter.
        self.tr.setLooksBypass(True)
        self.assertEqual(True, self.tr.getLooksBypass())
        self.tr.setLooksBypass(False)
        self.assertEqual(False, self.tr.getLooksBypass())

    def test_dataBypass(self):
        """
        Test the setDataBypass() and getDataBypass() methods.
        """

        # Default initialized dataBypass value is True.
        self.assertEqual(True, self.tr.getDataBypass())

        # Test dataBypass setter and getter.
        self.tr.setDataBypass(False)
        self.assertEqual(False, self.tr.getDataBypass())
        self.tr.setDataBypass(True)
        self.assertEqual(True, self.tr.getDataBypass())

    def test_validate_src(self):
        """
        Test the validate() method for src value. Value must not be empty.
        """

        self.tr.setSrc('source')
        self.tr.setDisplay('display')
        self.tr.setView('view')
        self.assertIsNone(self.tr.validate())

        # Exception validation test.
        self.tr.setSrc('')
        with self.assertRaises(OCIO.Exception):
            self.tr.validate()

    def test_validate_display(self):
        """
        Test the validate() method for display value. Value must not be empty.
        """

        self.tr.setSrc('source')
        self.tr.setDisplay('display')
        self.tr.setView('view')
        self.assertIsNone(self.tr.validate())

        # Exception validation test.
        self.tr.setDisplay('')
        with self.assertRaises(OCIO.Exception):
            self.tr.validate()

    def test_validate_view(self):
        """
        Test the validate() method for view value. Value must not be empty.
        """

        self.tr.setSrc('source')
        self.tr.setDisplay('display')
        self.tr.setView('view')
        self.assertIsNone(self.tr.validate())

        # Exception validation test.
        self.tr.setView('')
        with self.assertRaises(OCIO.Exception):
            self.tr.validate()

    def test_validate_direction(self):
        """
        Test the validate() method for direction.
        Direction must be forward or inverse.
        """

        self.tr.setSrc('source')
        self.tr.setDisplay('display')
        self.tr.setView('view')

        for direction in OCIO.TransformDirection.__members__.values():
            self.tr.setDirection(direction)
            self.assertIsNone(self.tr.validate())

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

    def test_processor(self):
        """
        Test creating a processor from a display view and a named transform.
        """
        SIMPLE_PROFILE = """ocio_profile_version: 2

roles:
  default: raw

displays:
  sRGB:
    - !<View> {name: Raw, colorspace: raw}
  display:
    - !<View> {name: view, view_transform: display_vt, display_colorspace: displayCSOut}
    - !<View> {name: viewCSNT, colorspace: nt_inverse}
    - !<View> {name: viewVTNT, view_transform: nt_forward, display_colorspace: displayCSOut}

view_transforms:
  - !<ViewTransform>
    name: display_vt
    to_display_reference: !<CDLTransform> {name: display vt to ref, sat: 1.5}
    from_display_reference: !<CDLTransform> {name: display vt from ref, sat: 1.5}

display_colorspaces:
  - !<ColorSpace>
    name: displayCSOut
    to_display_reference: !<CDLTransform> {name: out cs to ref, sat: 1.5}
    from_display_reference: !<CDLTransform> {name: out cs from ref, sat: 1.5}

colorspaces:
  - !<ColorSpace>
    name: raw
    isdata: true

  - !<ColorSpace>
    name: in
    to_reference: !<MatrixTransform> {offset: [0.11, 0.12, 0.13, 0]}

named_transforms:
  - !<NamedTransform>
    name: nt_forward
    transform: !<CDLTransform> {name: forward, sat: 1.5}

  - !<NamedTransform>
    name: nt_inverse
    inverse_transform: !<CDLTransform> {name: inverse, sat: 1.5}
"""
        # Create a config.
        cfg = OCIO.Config().CreateFromStream(SIMPLE_PROFILE)

        # Src can't be a named transform.
        dv_tr = OCIO.DisplayViewTransform(src='nt_forward',
                                          display='display',
                                          view='view')
        with self.assertRaises(OCIO.Exception):
            proc = cfg.getProcessor(dv_tr)

        # Dst is a named transform.
        dv_tr.setSrc('in')
        dv_tr.setView('viewCSNT')
        proc = cfg.getProcessor(dv_tr)
        group = proc.createGroupTransform()
        groupTransformsList = list(group)
        self.assertEqual(len(groupTransformsList), 1)
        self.assertIsInstance(groupTransformsList[0], OCIO.CDLTransform)
        metadata = groupTransformsList[0].getFormatMetadata()
        atts = metadata.getAttributes()
        self.assertEqual('inverse', next(atts)[1])

        # View transform is a named transform.
        dv_tr.setSrc('in')
        dv_tr.setView('viewVTNT')
        proc = cfg.getProcessor(dv_tr)
        group = proc.createGroupTransform()
        groupTransformsList = list(group)
        self.assertEqual(len(groupTransformsList), 2)
        self.assertIsInstance(groupTransformsList[0], OCIO.CDLTransform)
        metadata = groupTransformsList[0].getFormatMetadata()
        atts = metadata.getAttributes()
        self.assertEqual('forward', next(atts)[1])
        self.assertIsInstance(groupTransformsList[1], OCIO.CDLTransform)
        metadata = groupTransformsList[1].getFormatMetadata()
        atts = metadata.getAttributes()
        self.assertEqual('out cs from ref', next(atts)[1])

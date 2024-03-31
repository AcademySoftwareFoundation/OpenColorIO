# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import os
import unittest

import PyOpenColorIO as OCIO
from UnitTestUtils import TEST_DATAFILES_DIR, TEST_NAMES
from TransformsBaseTest import TransformsBaseTest


class FileTransformTest(unittest.TestCase, TransformsBaseTest):
    TEST_DIRECTION = OCIO.TRANSFORM_DIR_FORWARD
    TEST_INTERPOLATION = OCIO.Interpolation.INTERP_LINEAR
    TEST_ID = 'sample file'
    TEST_SRC = 'foo'
    TEST_DST = 'bar'
    DEFAULT_FORMATS = [('flame', '3dl'),
                       ('lustre', '3dl'),
                       ('ColorCorrection', 'cc'),
                       ('ColorCorrectionCollection', 'ccc'),
                       ('ColorDecisionList', 'cdl'),
                       ('Academy/ASC Common LUT Format', 'clf'),
                       ('Color Transform Format', 'ctf'),
                       ('cinespace', 'csp'),
                       ('Discreet 1D LUT', 'lut'),
                       ('houdini', 'lut'),
                       ('International Color Consortium profile', 'icc'),
                       ('Image Color Matching profile', 'icm'),
                       ('ICC profile', 'pf'),
                       ('iridas_cube', 'cube'),
                       ('iridas_itx', 'itx'),
                       ('iridas_look', 'look'),
                       ('pandora_mga', 'mga'),
                       ('pandora_m3d', 'm3d'),
                       ('resolve_cube', 'cube'),
                       ('spi1d', 'spi1d'),
                       ('spi3d', 'spi3d'),
                       ('spimtx', 'spimtx'),
                       ('truelight', 'cub'),
                       ('nukevf', 'vf')]

    def setUp(self):
        self.tr = OCIO.FileTransform()

    def test_transform_type(self):
        """
        Test the getTransformType() method.
        """
        self.assertEqual(self.tr.getTransformType(), OCIO.TRANSFORM_TYPE_FILE)

    def test_cccid(self):
        """
        Test the setCCCId() and getCCCId() methods.
        """

        # Default initialized id value is ""
        self.assertEqual(self.tr.getCCCId(), '')

        # Test id name setter and getter.
        for id_ in TEST_NAMES:
            self.tr.setCCCId(id_)
            self.assertEqual(self.tr.getCCCId(), id_)

        # Wrong type tests.
        for invalid in (None, 1):
            with self.assertRaises(TypeError):
                self.tr.setCCCId(invalid)

    def test_cdlstyle(self):
        """
        Test the setCDLStyle() and getCDLStyle() methods.
        """

        # Default initialized CDL Style value is no clamp.
        self.assertEqual(self.tr.getCDLStyle(),
                         OCIO.CDLStyle.CDL_NO_CLAMP)

        for cdl_style in OCIO.CDLStyle.__members__.values():
            self.tr.setCDLStyle(cdl_style)
            self.assertEqual(self.tr.getCDLStyle(), cdl_style)

        # Wrong type tests.
        for invalid in (None, 1):
            with self.assertRaises(TypeError):
                self.tr.setCDLStyle(invalid)

    def test_is_format_extension_supported(self):
        """
        Test the IsFormatExtensionSupported() method.
        """
        self.assertFalse(self.tr.IsFormatExtensionSupported('foo'))
        self.assertFalse(self.tr.IsFormatExtensionSupported('bar'))
        self.assertFalse(self.tr.IsFormatExtensionSupported('.'))
        self.assertTrue(self.tr.IsFormatExtensionSupported('cdl'))
        self.assertTrue(self.tr.IsFormatExtensionSupported('.cdl'))
        self.assertTrue(self.tr.IsFormatExtensionSupported('Cdl'))
        self.assertTrue(self.tr.IsFormatExtensionSupported('.Cdl'))
        self.assertTrue(self.tr.IsFormatExtensionSupported('3dl'))
        self.assertTrue(self.tr.IsFormatExtensionSupported('.3dl'))

    def test_getformats(self):
        """
        Test the getFormats() method.
        """

        format_iterator = OCIO.FileTransform.getFormats()

        for name, ext in self.DEFAULT_FORMATS:
            format_name, format_ext = next(format_iterator)
            self.assertEqual(format_name, name)
            self.assertEqual(format_ext, ext)

        self.assertEqual(format_iterator.__len__(), 24)

    def test_interpolation(self):
        """
        Test the setInterpolation() and getInterpolation() methods.
        """

        # Default initialized interpolation is default.
        self.assertEqual(self.tr.getInterpolation(), OCIO.INTERP_DEFAULT)

        for interpolation in OCIO.Interpolation.__members__.values():
            self.tr.setInterpolation(interpolation)
            self.assertEqual(self.tr.getInterpolation(), interpolation)

        # Wrong type tests.
        for invalid in (None, 1):
            with self.assertRaises(TypeError):
                self.tr.setInterpolation(invalid)

    def test_src(self):
        """
        Test the setSrc() and getSrc() methods.
        """

        # Default initialized src value is ""
        self.assertEqual(self.tr.getSrc(), '')

        for name in TEST_NAMES:
            self.tr.setSrc(name)
            self.assertEqual(name, self.tr.getSrc())

        # Wrong type tests.
        for invalid in (None, 1):
            with self.assertRaises(TypeError):
                self.tr.setSrc(invalid)

    def test_constructor_with_keyword(self):
        """
        Test FileTransform constructor with keywords and validate its values.
        """

        # With keywords in their proper order.
        file_tr = OCIO.FileTransform(
            src=self.TEST_SRC,
            cccId=self.TEST_ID,
            interpolation=self.TEST_INTERPOLATION,
            direction=self.TEST_DIRECTION)

        self.assertEqual(file_tr.getSrc(), self.TEST_SRC)
        self.assertEqual(file_tr.getCCCId(), self.TEST_ID)
        self.assertEqual(file_tr.getInterpolation(), self.TEST_INTERPOLATION)
        self.assertEqual(file_tr.getDirection(), self.TEST_DIRECTION)

        # With keywords not in their proper order.
        file_tr2 = OCIO.FileTransform(
            cccId=self.TEST_ID,
            direction=self.TEST_DIRECTION,
            src=self.TEST_SRC,
            interpolation=self.TEST_INTERPOLATION
        )

        self.assertEqual(file_tr2.getSrc(), self.TEST_SRC)
        self.assertEqual(file_tr2.getCCCId(), self.TEST_ID)
        self.assertEqual(file_tr2.getInterpolation(), self.TEST_INTERPOLATION)
        self.assertEqual(file_tr2.getDirection(), self.TEST_DIRECTION)

    def test_constructor_with_positional(self):
        """
        Test FileTransform constructor without keywords and validate its values.
        """

        file_tr = OCIO.FileTransform(
            self.TEST_SRC,
            self.TEST_ID,
            self.TEST_INTERPOLATION,
            self.TEST_DIRECTION)

        self.assertEqual(file_tr.getSrc(), self.TEST_SRC)
        self.assertEqual(file_tr.getCCCId(), self.TEST_ID)
        self.assertEqual(file_tr.getInterpolation(), self.TEST_INTERPOLATION)
        self.assertEqual(file_tr.getDirection(), self.TEST_DIRECTION)

    def test_constructor_wrong_parameter_type(self):
        """
        Test FileTransform constructor with a wrong parameter type.
        """

        for invalid in (None, 1):
            with self.assertRaises(TypeError):
                file_tr = OCIO.ExponentTransform(invalid)

    def test_get_processor(self):
        """
        Test that interpolation values of default and unknown do not cause a problem for
        getProcessor.
        """

        config = OCIO.Config.CreateRaw()
        test_file = os.path.join(TEST_DATAFILES_DIR, 'lut1d_1.spi1d')
        file_tr = OCIO.FileTransform(src=test_file)
        processor = config.getProcessor(file_tr)
        # INTERP_UNKNOWN will be ignored by the LUT and a warning will be logged.
        curLogLevel = OCIO.GetLoggingLevel()
        OCIO.SetLoggingLevel(OCIO.LOGGING_LEVEL_NONE)
        file_tr.setInterpolation(OCIO.INTERP_UNKNOWN)
        processor = config.getProcessor(file_tr)
        OCIO.SetLoggingLevel(curLogLevel)

# TODO:
# No CDLStyle parameter in constructor?

# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest

import PyOpenColorIO as OCIO
from TransformsBaseTest import TransformsBaseTest


class ExposureContrastTransformTest(unittest.TestCase, TransformsBaseTest):
    TEST_ID = 'sample exponent linear'
    TEST_NAME = 'name'
    TEST_VALUES = [0.1, 2, -0.5, 10]
    TEST_STYLE = OCIO.EXPOSURE_CONTRAST_LOGARITHMIC
    TEST_EXPOSURE = 1.1
    TEST_CONTRAST = 0.5
    TEST_GAMMA = 1.5
    TEST_PIVOT = 0.3
    TEST_LOGEXPOSURESTEP = 0.1
    TEST_LOGMIDGRAY = 0.6
    TEST_DIRECTION = OCIO.TRANSFORM_DIR_INVERSE
    TEST_INVALIDS = (None, 'hello', [1, 2, 3])

    def setUp(self):
        self.tr = OCIO.ExposureContrastTransform()

    def test_transform_type(self):
        """
        Test the getTransformType() method.
        """
        self.assertEqual(self.tr.getTransformType(),
                         OCIO.TRANSFORM_TYPE_EXPOSURE_CONTRAST)

    def test_style(self):
        """
        Test the setStyle() and getStyle() methods.
        """

        # Default initialized style is linear.
        self.assertEqual(self.tr.getStyle(), OCIO.EXPOSURE_CONTRAST_LINEAR)

        for style in OCIO.ExposureContrastStyle.__members__.values():
            self.tr.setStyle(style)
            self.assertEqual(self.tr.getStyle(), style)

    def test_exposure(self):
        """
        Test the setExposure() and getExposure() methods.
        """

        # Default initialized exposure is 0.
        self.assertEqual(self.tr.getExposure(), 0)

        for exposure in self.TEST_VALUES:
            self.tr.setExposure(exposure)
            self.assertEqual(self.tr.getExposure(), exposure)

        # Wrong type tests.
        for invalid in self.TEST_INVALIDS:
            with self.assertRaises(TypeError):
                self.tr.setExposure(invalid)

    def test_exposure_dynamic(self):
        """
        Test the isExposureDynamic() and makeExposureDynamic() methods.
        """

        # Default initialized property is False.
        self.assertEqual(self.tr.isExposureDynamic(), False)

        self.tr.makeExposureDynamic()
        self.assertEqual(self.tr.isExposureDynamic(), True)

    def test_contrast(self):
        """
        Test the setContrast() and getContrast() methods.
        """

        # Default initialized contrast is 1.
        self.assertEqual(self.tr.getContrast(), 1)

        for contrast in self.TEST_VALUES:
            self.tr.setContrast(contrast)
            self.assertEqual(self.tr.getContrast(), contrast)

        # Wrong type tests.
        for invalid in self.TEST_INVALIDS:
            with self.assertRaises(TypeError):
                self.tr.setContrast(invalid)

    def test_contrast_dynamic(self):
        """
        Test the isContrastDynamic() and makeContrastDynamic() methods.
        """

        # Default initialized property is False.
        self.assertEqual(self.tr.isContrastDynamic(), False)

        self.tr.makeContrastDynamic()
        self.assertEqual(self.tr.isContrastDynamic(), True)

    def test_gamma(self):
        """
        Test the setGamma() and getGamma() methods.
        """

        # Default initialized gamma is 1.
        self.assertEqual(self.tr.getGamma(), 1)

        for gamma in self.TEST_VALUES:
            self.tr.setGamma(gamma)
            self.assertEqual(self.tr.getGamma(), gamma)

        # Wrong type tests.
        for invalid in self.TEST_INVALIDS:
            with self.assertRaises(TypeError):
                self.tr.setGamma(invalid)

    def test_gamma_dynamic(self):
        """
        Test the isGammaDynamic() and makeGammaDynamic() methods.
        """

        # Default initialized property is False.
        self.assertEqual(self.tr.isGammaDynamic(), False)

        self.tr.makeGammaDynamic()
        self.assertEqual(self.tr.isGammaDynamic(), True)

    def test_pivot(self):
        """
        Test the setPivot() and getPivot() methods.
        """

        # Default initialized pivot is 0.18.
        self.assertEqual(self.tr.getPivot(), 0.18)

        for pivot in self.TEST_VALUES:
            self.tr.setPivot(pivot)
            self.assertEqual(self.tr.getPivot(), pivot)

        # Wrong type tests.
        for invalid in self.TEST_INVALIDS:
            with self.assertRaises(TypeError):
                self.tr.setPivot(invalid)

    def test_log_exposure_step(self):
        """
        Test the setLogExposureStep() and getLogExposureStep() methods.
        """

        # Default initialized log exposure step is 0.088.
        self.assertEqual(self.tr.getLogExposureStep(), 0.088)

        for exp_step in self.TEST_VALUES:
            self.tr.setLogExposureStep(exp_step)
            self.assertEqual(self.tr.getLogExposureStep(), exp_step)

        # Wrong type tests.
        for invalid in self.TEST_INVALIDS:
            with self.assertRaises(TypeError):
                self.tr.setLogExposureStep(invalid)

    def test_log_mid_gray(self):
        """
        Test the setLogMidGray() and getLogMidGray() methods.
        """

        # Default initialized log mid gray value is 0.435.
        self.assertEqual(self.tr.getLogMidGray(), 0.435)

        for mid_gray in self.TEST_VALUES:
            self.tr.setLogMidGray(mid_gray)
            self.assertEqual(self.tr.getLogMidGray(), mid_gray)

        # Wrong type tests.
        for invalid in self.TEST_INVALIDS:
            with self.assertRaises(TypeError):
                self.tr.setLogMidGray(invalid)

    def test_format_metadata(self):
        """
        Test the getFormatMetadata() method.
        """

        format_metadata = self.tr.getFormatMetadata()
        self.assertIsInstance(format_metadata, OCIO.FormatMetadata)
        self.assertEqual(format_metadata.getID(), '')
        self.assertEqual(format_metadata.getName(), '')
        format_metadata.setID(self.TEST_ID)
        format_metadata.setName(self.TEST_NAME)
        self.assertEqual(format_metadata.getID(), self.TEST_ID)
        self.assertEqual(format_metadata.getName(), self.TEST_NAME)

    def test_validate_direction(self):
        """
        Test the validate() method for direction.
        Direction must be forward or inverse.
        """

        self.tr.setDirection(OCIO.TRANSFORM_DIR_FORWARD)
        self.assertIsNone(self.tr.validate())

    def test_constructor_with_keywords(self):
        """
        Test ExposureContrastTransform constructor with keywords and validate its values.
        """

        exp_tr = OCIO.ExposureContrastTransform(
            style=self.TEST_STYLE,
            exposure=self.TEST_EXPOSURE,
            contrast=self.TEST_CONTRAST,
            gamma=self.TEST_GAMMA,
            pivot=self.TEST_PIVOT,
            logExposureStep=self.TEST_LOGEXPOSURESTEP,
            logMidGray=self.TEST_LOGMIDGRAY,
            dynamicExposure=True,
            dynamicContrast=True,
            dynamicGamma=True,
            direction=self.TEST_DIRECTION)

        self.assertEqual(exp_tr.getStyle(), self.TEST_STYLE)
        self.assertEqual(exp_tr.getExposure(), self.TEST_EXPOSURE)
        self.assertEqual(exp_tr.getContrast(), self.TEST_CONTRAST)
        self.assertEqual(exp_tr.getGamma(), self.TEST_GAMMA)
        self.assertEqual(exp_tr.getPivot(), self.TEST_PIVOT)
        self.assertEqual(exp_tr.getLogExposureStep(),
                         self.TEST_LOGEXPOSURESTEP)
        self.assertEqual(exp_tr.getLogMidGray(), self.TEST_LOGMIDGRAY)
        self.assertTrue(exp_tr.isExposureDynamic())
        self.assertTrue(exp_tr.isContrastDynamic())
        self.assertTrue(exp_tr.isGammaDynamic())
        self.assertEqual(exp_tr.getDirection(), self.TEST_DIRECTION)

        # With keywords not in their proper order.
        exp_tr2 = OCIO.ExposureContrastTransform(
            direction=self.TEST_DIRECTION,
            dynamicGamma=True,
            dynamicContrast=True,
            dynamicExposure=True,
            logMidGray=self.TEST_LOGMIDGRAY,
            style=self.TEST_STYLE,
            exposure=self.TEST_EXPOSURE,
            contrast=self.TEST_CONTRAST,
            gamma=self.TEST_GAMMA,
            pivot=self.TEST_PIVOT,
            logExposureStep=self.TEST_LOGEXPOSURESTEP)

        self.assertEqual(exp_tr2.getStyle(), self.TEST_STYLE)
        self.assertEqual(exp_tr2.getExposure(), self.TEST_EXPOSURE)
        self.assertEqual(exp_tr2.getContrast(), self.TEST_CONTRAST)
        self.assertEqual(exp_tr2.getGamma(), self.TEST_GAMMA)
        self.assertEqual(exp_tr2.getPivot(), self.TEST_PIVOT)
        self.assertEqual(exp_tr2.getLogExposureStep(),
                         self.TEST_LOGEXPOSURESTEP)
        self.assertEqual(exp_tr2.getLogMidGray(), self.TEST_LOGMIDGRAY)
        self.assertTrue(exp_tr2.isExposureDynamic())
        self.assertTrue(exp_tr2.isContrastDynamic())
        self.assertTrue(exp_tr2.isGammaDynamic())
        self.assertEqual(exp_tr2.getDirection(), self.TEST_DIRECTION)

    def test_constructor_with_positional(self):
        """
        Test ExposureContrastTransform constructor without keywords and validate its values.
        """

        exp_tr = OCIO.ExposureContrastTransform(
            self.TEST_STYLE,
            self.TEST_EXPOSURE,
            self.TEST_CONTRAST,
            self.TEST_GAMMA,
            self.TEST_PIVOT,
            self.TEST_LOGEXPOSURESTEP,
            self.TEST_LOGMIDGRAY,
            True,
            True,
            True,
            self.TEST_DIRECTION)

        self.assertEqual(exp_tr.getStyle(), self.TEST_STYLE)
        self.assertEqual(exp_tr.getExposure(), self.TEST_EXPOSURE)
        self.assertEqual(exp_tr.getContrast(), self.TEST_CONTRAST)
        self.assertEqual(exp_tr.getGamma(), self.TEST_GAMMA)
        self.assertEqual(exp_tr.getPivot(), self.TEST_PIVOT)
        self.assertEqual(exp_tr.getLogExposureStep(),
                         self.TEST_LOGEXPOSURESTEP)
        self.assertEqual(exp_tr.getLogMidGray(), self.TEST_LOGMIDGRAY)
        self.assertTrue(exp_tr.isExposureDynamic())
        self.assertTrue(exp_tr.isContrastDynamic())
        self.assertTrue(exp_tr.isGammaDynamic())
        self.assertEqual(exp_tr.getDirection(), self.TEST_DIRECTION)

    def test_constructor_wrong_parameter_type(self):
        """
        Test ExposureContrastTransform constructor with a wrong parameter type.
        """

        for invalid in (None, 1, self.TEST_ID):
            with self.assertRaises(TypeError):
                exp_tr = OCIO.ExponentWithLinearTransform(invalid)

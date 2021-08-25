# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest
import os
import sys

import PyOpenColorIO as OCIO
from UnitTestUtils import SAMPLE_CONFIG, TEST_DATAFILES_DIR


class DisplayViewHelpersTest(unittest.TestCase):
    def setUp(self):
        self.cfg = OCIO.Config().CreateFromStream(SAMPLE_CONFIG)

    def tearDown(self):
        self.cfg = None

    def test_get_processor(self):
        """
        Test the GetProcessor() function.
        """
        proc = OCIO.DisplayViewHelpers.GetProcessor(config = self.cfg, workingSpaceName = 'lin_1',
                                                    displayName = 'DISP_1', viewName = 'VIEW_1')
        grp = proc.createGroupTransform()
        self.assertEqual(len(grp), 3)
        t = grp[0]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_EXPOSURE_CONTRAST)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        self.assertTrue(t.isExposureDynamic())
        self.assertTrue(t.isContrastDynamic())
        self.assertFalse(t.isGammaDynamic())
        self.assertFalse(t.isGammaDynamic())
        self.assertEqual(t.getPivot(), 0.18)
        self.assertEqual(t.getExposure(), 0.)
        self.assertEqual(t.getContrast(), 1.)
        self.assertEqual(t.getGamma(), 1.)
        self.assertEqual(t.getStyle(), OCIO.EXPOSURE_CONTRAST_LINEAR)
        t = grp[1]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_EXPONENT)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(t.getValue(), [2.6, 2.6, 2.6, 1])
        t = grp[2]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_EXPOSURE_CONTRAST)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        self.assertFalse(t.isExposureDynamic())
        self.assertFalse(t.isContrastDynamic())
        self.assertTrue(t.isGammaDynamic())

    def test_get_identity_processor(self):
        """
        Test the GetIdentityProcessor() function.
        """
        proc = OCIO.DisplayViewHelpers.GetIdentityProcessor(config = self.cfg)
        grp = proc.createGroupTransform()
        self.assertEqual(len(grp), 2)
        t = grp[0]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_EXPOSURE_CONTRAST)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        self.assertTrue(t.isExposureDynamic())
        self.assertTrue(t.isContrastDynamic())
        self.assertFalse(t.isGammaDynamic())
        self.assertEqual(t.getPivot(), 0.18)
        self.assertEqual(t.getExposure(), 0.)
        self.assertEqual(t.getContrast(), 1.)
        self.assertEqual(t.getGamma(), 1.)
        self.assertEqual(t.getStyle(), OCIO.EXPOSURE_CONTRAST_LINEAR)
        t = grp[1]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_EXPOSURE_CONTRAST)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        self.assertFalse(t.isExposureDynamic())
        self.assertFalse(t.isContrastDynamic())
        self.assertTrue(t.isGammaDynamic())
        self.assertEqual(t.getPivot(), 1.)
        self.assertEqual(t.getExposure(), 0.)
        self.assertEqual(t.getContrast(), 1.)
        self.assertEqual(t.getGamma(), 1.)
        self.assertEqual(t.getStyle(), OCIO.EXPOSURE_CONTRAST_VIDEO)

    def test_add_display_view(self):
        """
        Test the AddDisplayView() and RemoveDisplayView() functions.
        """
        filePath = os.path.join(TEST_DATAFILES_DIR, 'lut1d_green.ctf')

        self.assertEqual(len(self.cfg.getViews('DISP_1')), 3);

        OCIO.DisplayViewHelpers.AddDisplayView(config = self.cfg,
                                               displayName = 'DISP_1', viewName = 'VIEW_5',
                                               lookName = 'look_3', colorSpaceName = 'view_5',
                                               colorSpaceCategories = 'file-io, working-space',
                                               transformFilePath = filePath,
                                               connectionColorSpaceName = 'lut_input_1')

        # Check view.

        views = self.cfg.getViews('DISP_1')
        self.assertEqual(len(views), 4)
        view = views[3]
        self.assertEqual(view, 'VIEW_5')
        self.assertEqual(self.cfg.getDisplayViewColorSpaceName('DISP_1', 'VIEW_5'), 'view_5')
        self.assertEqual(self.cfg.getDisplayViewLooks('DISP_1', 'VIEW_5'), 'look_3')

        # Check color space.

        cs = self.cfg.getColorSpace('view_5')
        # Since some categories are already used in the config, they are added to the color space.
        self.assertTrue(cs.hasCategory('file-io'))
        self.assertTrue(cs.hasCategory('working-space'))
        self.assertEqual(cs.getFamily(), '');
        self.assertEqual(cs.getDescription(), '');

        proc = OCIO.DisplayViewHelpers.GetProcessor(config = self.cfg, workingSpaceName = 'lin_1',
                                                    displayName = 'DISP_1', viewName = 'VIEW_5')
        grp = proc.createGroupTransform()
        self.assertEqual(len(grp), 7)

        # The E/C op.

        t = grp[0]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_EXPOSURE_CONTRAST)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        self.assertTrue(t.isExposureDynamic())
        self.assertTrue(t.isContrastDynamic())
        self.assertFalse(t.isGammaDynamic())
        self.assertEqual(t.getPivot(), 0.18)
        self.assertEqual(t.getExposure(), 0.)
        self.assertEqual(t.getContrast(), 1.)
        self.assertEqual(t.getGamma(), 1.)
        self.assertEqual(t.getStyle(), OCIO.EXPOSURE_CONTRAST_LINEAR)

        # Working color space (i.e. lin_1) to the 'look' process color space (i.e. log_1).

        t = grp[1]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_LOG)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        self.assertEqual(t.getBase(), 2.)

        # 'look' color processing i.e. look_3.

        t = grp[2]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_CDL)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        self.assertEqual(t.getSlope(), [1., 2., 1.])
        self.assertEqual(t.getPower(), [1., 1., 1.])
        self.assertEqual(t.getSat(), 1.)

        # 'look' process color space (i.e. log_1) to 'reference'.

        t = grp[3]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_LOG)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(t.getBase(), 2.)

        # 'reference' to the display color space (i.e. view_3).
        # The 'view_3' color space is a group transform containing:
        #  1. 'reference' to connection color space i.e. lut_1.
        #  2. The user 1D LUT.

        t = grp[4]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_EXPONENT)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(t.getValue(), [2.6, 2.6, 2.6, 1.])

        t = grp[5]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_LUT1D)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        self.assertEqual(t.getValue(0), (0., 0., 0.))

        rgb = t.getValue(1)
        self.assertEqual(rgb[0], 0.)
        self.assertAlmostEqual(rgb[1], 33./1023., delta=1e-8)
        self.assertEqual(rgb[2], 0.)

        rgb = t.getValue(2)
        self.assertEqual(rgb[0], 0.)
        self.assertAlmostEqual(rgb[1], 66./1023., delta=1e-8)
        self.assertEqual(rgb[2], 0.)

        # The E/C op.

        t = grp[6]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_EXPOSURE_CONTRAST)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        self.assertFalse(t.isExposureDynamic())
        self.assertFalse(t.isContrastDynamic())
        self.assertTrue(t.isGammaDynamic())
        self.assertEqual(t.getPivot(), 1.)
        self.assertEqual(t.getExposure(), 0.)
        self.assertEqual(t.getContrast(), 1.)
        self.assertEqual(t.getGamma(), 1.)
        self.assertEqual(t.getStyle(), OCIO.EXPOSURE_CONTRAST_VIDEO)

        # Some faulty scenarios.

        # Color space already exists.
        with self.assertRaises(OCIO.Exception):
            OCIO.DisplayViewHelpers.AddDisplayView(config = self.cfg, colorSpaceName = 'view_5',
                                                   displayName = 'DISP_1', viewName = 'VIEW_5',
                                                   transformFilePath = filePath,
                                                   connectionColorSpaceName = 'lut_input_1')

        # View is empty.
        with self.assertRaises(TypeError):
            OCIO.DisplayViewHelpers.AddDisplayView(config = self.cfg, colorSpaceName = 'view_51',
                                                   displayName = 'DISP_1',
                                                   transformFilePath = filePath,
                                                   connectionColorSpaceName = 'lut_input_1')

        # Connection CS does not exist.
        with self.assertRaises(OCIO.Exception):
            OCIO.DisplayViewHelpers.AddDisplayView(config = self.cfg, colorSpaceName = 'view_51',
                                                   displayName = 'DISP_1', viewName = 'VIEW_51',
                                                   transformFilePath = filePath,
                                                   connectionColorSpaceName = 'unknown')

        # Remove added view.
        OCIO.DisplayViewHelpers.RemoveDisplayView(config = self.cfg,
                                                  displayName = 'DISP_1', viewName = 'VIEW_5')
        self.assertEqual(len(self.cfg.getViews('DISP_1')), 3);

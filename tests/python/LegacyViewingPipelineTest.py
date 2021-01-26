# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest
import os
import sys

import PyOpenColorIO as OCIO
from UnitTestUtils import SAMPLE_CONFIG


class LegacyViewingPipelineTest(unittest.TestCase):

    def test_looks_override(self):
        """
        Test the get/setLooksOverride and get/setLooksOverrideEnabled functions.
        """
        pipeline = OCIO.LegacyViewingPipeline()
        self.assertFalse(pipeline.getLooksOverrideEnabled())
        pipeline.setLooksOverrideEnabled(True)
        self.assertTrue(pipeline.getLooksOverrideEnabled())
        pipeline.setLooksOverrideEnabled(None)
        self.assertFalse(pipeline.getLooksOverrideEnabled())
        with self.assertRaises(TypeError):
            pipeline.setLooksOverrideEnabled()
        with self.assertRaises(TypeError):
            pipeline.setLooksOverrideEnabled(OCIO.TRANSFORM_DIR_FORWARD)
        with self.assertRaises(TypeError):
            pipeline.setLooksOverrideEnabled('no')

        self.assertEqual(pipeline.getLooksOverride(), '')
        pipeline.setLooksOverride('test')
        self.assertEqual(pipeline.getLooksOverride(), 'test')
        pipeline.setLooksOverride('')
        self.assertEqual(pipeline.getLooksOverride(), '')
        with self.assertRaises(TypeError):
            pipeline.setLooksOverride(None)
        with self.assertRaises(TypeError):
            pipeline.setLooksOverride()
        with self.assertRaises(TypeError):
            pipeline.setLooksOverride(OCIO.TRANSFORM_DIR_FORWARD)
        with self.assertRaises(TypeError):
            pipeline.setLooksOverride(False)

    def test_display_view_transform(self):
        """
        Test the get/setDisplayViewTransform functions.
        """
        pipeline = OCIO.LegacyViewingPipeline()
        self.assertEqual(pipeline.getDisplayViewTransform(), None)
        dvt = OCIO.DisplayViewTransform(src='test', display='display', view='view')
        pipeline.setDisplayViewTransform(dvt)
        self.assertEqual(pipeline.getDisplayViewTransform().getSrc(), 'test')
        pipeline.setDisplayViewTransform(None)
        self.assertEqual(pipeline.getDisplayViewTransform(), None)

        with self.assertRaises(TypeError):
            pipeline.setDisplayViewTransform()
        with self.assertRaises(TypeError):
            pipeline.setDisplayViewTransform(OCIO.TRANSFORM_DIR_FORWARD)
        with self.assertRaises(TypeError):
            pipeline.setDisplayViewTransform(False)

    def test_linear_cc(self):
        """
        Test the get/setLinearCC functions.
        """
        pipeline = OCIO.LegacyViewingPipeline()
        self.assertEqual(pipeline.getLinearCC(), None)
        bi = OCIO.BuiltinTransform()
        bi.setStyle('IDENTITY')
        pipeline.setLinearCC(bi)
        t = pipeline.getLinearCC()
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_BUILTIN)
        t.__class__ = OCIO.BuiltinTransform
        self.assertEqual(t.getStyle(), 'IDENTITY')
        pipeline.setLinearCC(None)
        self.assertEqual(pipeline.getLinearCC(), None)

        with self.assertRaises(TypeError):
            pipeline.setLinearCC()
        with self.assertRaises(TypeError):
            pipeline.setLinearCC(OCIO.TRANSFORM_DIR_FORWARD)
        with self.assertRaises(TypeError):
            pipeline.setLinearCC(False)

    def test_color_timing_cc(self):
        """
        Test the get/setColorTimingCC functions.
        """
        pipeline = OCIO.LegacyViewingPipeline()
        self.assertEqual(pipeline.getColorTimingCC(), None)
        bi = OCIO.BuiltinTransform()
        bi.setStyle('IDENTITY')
        pipeline.setColorTimingCC(bi)
        t = pipeline.getColorTimingCC()
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_BUILTIN)
        t.__class__ = OCIO.BuiltinTransform
        self.assertEqual(t.getStyle(), 'IDENTITY')
        pipeline.setColorTimingCC(None)
        self.assertEqual(pipeline.getColorTimingCC(), None)

        with self.assertRaises(TypeError):
            pipeline.setColorTimingCC()
        with self.assertRaises(TypeError):
            pipeline.setColorTimingCC(OCIO.TRANSFORM_DIR_FORWARD)
        with self.assertRaises(TypeError):
            pipeline.setColorTimingCC(False)

    def test_channel_view(self):
        """
        Test the get/setChannelView functions.
        """
        pipeline = OCIO.LegacyViewingPipeline()
        self.assertEqual(pipeline.getChannelView(), None)
        mat = OCIO.MatrixTransform()
        mat.setOffset([0.1, 0.2, 0.3, 0.])
        pipeline.setChannelView(mat)
        self.assertEqual(pipeline.getChannelView().getOffset(), [0.1, 0.2, 0.3, 0.])
        pipeline.setChannelView(None)
        self.assertEqual(pipeline.getChannelView(), None)

        with self.assertRaises(TypeError):
            pipeline.setChannelView()
        with self.assertRaises(TypeError):
            pipeline.setChannelView(OCIO.TRANSFORM_DIR_FORWARD)
        with self.assertRaises(TypeError):
            pipeline.setChannelView(False)

    def test_display_cc(self):
        """
        Test the get/setDisplayCC functions.
        """
        pipeline = OCIO.LegacyViewingPipeline()
        self.assertEqual(pipeline.getDisplayCC(), None)
        bi = OCIO.BuiltinTransform()
        bi.setStyle('IDENTITY')
        pipeline.setDisplayCC(bi)
        t = pipeline.getDisplayCC()
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_BUILTIN)
        t.__class__ = OCIO.BuiltinTransform
        self.assertEqual(t.getStyle(), 'IDENTITY')
        pipeline.setDisplayCC(None)
        self.assertEqual(pipeline.getDisplayCC(), None)

        with self.assertRaises(TypeError):
            pipeline.setDisplayCC()
        with self.assertRaises(TypeError):
            pipeline.setDisplayCC(OCIO.TRANSFORM_DIR_FORWARD)
        with self.assertRaises(TypeError):
            pipeline.setDisplayCC(False)

    def test_get_processor_errors(self):
        """
        Test the getProcessor function failures.
        """
        cfg = OCIO.Config().CreateRaw()
        pipeline = OCIO.LegacyViewingPipeline()

        # No DisplayViewTransform.
        with self.assertRaises(OCIO.Exception):
            pipeline.getProcessor(cfg)

        # DisplayViewTransform refers to elements that are not part of config.

        dvt = OCIO.DisplayViewTransform(src='colorspace1', display='sRGB', view='view1')
        pipeline.setDisplayViewTransform(dvt)
        with self.assertRaises(OCIO.Exception):
            pipeline.getProcessor(cfg)

        # Add display and view.
        cfg.addDisplayView(display='sRGB', view='view1', colorSpaceName='colorspace1')

        with self.assertRaises(OCIO.Exception):
            pipeline.getProcessor(cfg)

        # Add color space.
        cs = OCIO.ColorSpace(name='colorspace1')
        cfg.addColorSpace(cs)

        # Processor can now be created.
        pipeline.getProcessor(cfg)

        mat = OCIO.MatrixTransform()
        mat.setOffset([0.1, 0.2, 0.3, 0.])
        pipeline.setLinearCC(mat)

        # Scene_linear role is missing.
        with self.assertRaises(OCIO.Exception):
            pipeline.getProcessor(cfg)

        cfg.setRole(role='scene_linear', colorSpaceName='colorspace1')
        # Processor can now be created.
        pipeline.getProcessor(cfg)

        # Print the pipeline.
        self.assertEqual(str(pipeline), ('DisplayViewTransform: <DisplayViewTransform direction=forward, '
            'src=colorspace1, display=sRGB, view=view1, , looksBypass=1>, '
            'LinearCC: <MatrixTransform direction=forward, fileindepth=unknown, '
            'fileoutdepth=unknown, matrix=1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1, offset=0.1 0.2 0.3 0>'))

    def test_get_processor(self):
        """
        Test the getProcessor function.
        """
        cfg = OCIO.Config().CreateFromStream(SAMPLE_CONFIG)
        dvt = OCIO.DisplayViewTransform(src='in_1', display='DISP_2', view='VIEW_2')
        pipeline = OCIO.LegacyViewingPipeline()
        pipeline.setDisplayViewTransform(dvt)

        mat = OCIO.MatrixTransform.Scale([1.1, 1.2, 1.1, 1.])
        pipeline.setChannelView(mat)
        ff = OCIO.FixedFunctionTransform(OCIO.FIXED_FUNCTION_ACES_RED_MOD_03)
        pipeline.setLinearCC(ff)

        proc = pipeline.getProcessor(cfg)
        grp = proc.createGroupTransform()
        self.assertEqual(len(grp), 8)

        # Color space conversion from in_1 to scene_linear role (lin_1 color space).

        t = grp[0]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_EXPONENT)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        self.assertEqual(t.getValue(), [2.6, 2.6, 2.6, 1.])

        # LinearCC transform.

        t = grp[1]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_FIXED_FUNCTION)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)

        # Apply the looks, channel view, and view transform.

        # Lin_1 to look3 process space (log_1).

        t = grp[2]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_LOG)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        self.assertEqual(t.getBase(), 2.)

        # Look_3 transform.

        t = grp[3]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_CDL)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        self.assertEqual(t.getSlope(), [1., 2., 1.])

        # Look_3 & look_4 have the same process space, no color space conversion.

        # Look_4 transform.

        t = grp[4]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_CDL)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(t.getSlope(), [1.2, 2.2, 1.2])

        # Channel View transform (no color space conversion).

        t = grp[5]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_MATRIX)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        m44 = t.getMatrix()
        self.assertEqual(m44[0],  1.1)
        self.assertEqual(m44[1],  0)
        self.assertEqual(m44[2],  0)
        self.assertEqual(m44[3],  0)
        self.assertEqual(m44[5],  1.2)
        self.assertEqual(m44[10],  1.1)

        # Look_4 process color space (log_1) to reference.

        t = grp[6]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_LOG)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(t.getBase(), 2.)

        # Reference to view_2 color space.

        t = grp[7]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_EXPONENT)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(t.getValue(), [2.4, 2.4, 2.4, 1.])

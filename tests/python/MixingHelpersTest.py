# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest
import os
import sys

import PyOpenColorIO as OCIO
from UnitTestUtils import SAMPLE_CONFIG

def test_percent_1000(a, b):
    # Helper function to test sliders.
    return abs(a - int(100000 * b)) <= 1

class MixingHelpersTest(unittest.TestCase):

    def setUp(self):
        self.cfg = OCIO.Config().CreateFromStream(SAMPLE_CONFIG)

    def tearDown(self):
        self.cfg = None

    def test_encoding(self):
        """
        Test MixingColorSpaceManager encodings.
        """
        mix = OCIO.MixingColorSpaceManager(self.cfg)

        self.assertEqual(mix.getSelectedMixingEncodingIdx(), 0)
        encodings = mix.getMixingEncodings()
        self.assertEqual(len(encodings), 2)
        self.assertEqual(encodings[0], 'RGB')
        self.assertEqual(encodings[1], 'HSV')

        mix.setSelectedMixingEncoding('HSV')
        self.assertEqual(mix.getSelectedMixingEncodingIdx(), 1)
        mix.setSelectedMixingEncodingIdx(0)
        self.assertEqual(mix.getSelectedMixingEncodingIdx(), 0)

        with self.assertRaises(OCIO.Exception):
            mix.setSelectedMixingEncoding('HS')

        for param in [None, 1, OCIO.TRANSFORM_DIR_FORWARD]:
            with self.assertRaises(TypeError):
                mix.setSelectedMixingEncoding(param)

        for param in [None, 'test']:
            with self.assertRaises(TypeError):
                mix.setSelectedMixingEncodingIdx(param)

        # Print the MixingColorSpaceManager.
        self.assertEqual(str(mix),
            ('config: 667ca4dc5b3779e570229fb7fd9cffe1:6001c324468d497f99aa06d3014798d8, '
            'slider: [minEdge: 0, maxEdge: 0.833864], mixingSpaces: [Rendering Space, '
            'Display Space], selectedMixingSpaceIdx: 0, selectedMixingEncodingIdx: 0'))

        mix = None

    def test_mixing_space(self):
        """
        Test MixingColorSpaceManager mixing spaces for a config without ROLE_COLOR_PICKING role.
        """
        mix = OCIO.MixingColorSpaceManager(self.cfg)

        self.assertEqual(mix.getSelectedMixingSpaceIdx(), 0)
        mixSpaces = mix.getMixingSpaces()
        self.assertEqual(len(mixSpaces), 2)
        self.assertEqual(mixSpaces[0], 'Rendering Space')
        self.assertEqual(mixSpaces[1], 'Display Space')

        mix.setSelectedMixingSpace('Display Space')
        self.assertEqual(mix.getSelectedMixingSpaceIdx(), 1)
        mix.setSelectedMixingSpaceIdx(0)
        self.assertEqual(mix.getSelectedMixingSpaceIdx(), 0)

        with self.assertRaises(OCIO.Exception):
            mix.setSelectedMixingSpace('DisplaySpace')

        for param in [None, 1, OCIO.TRANSFORM_DIR_FORWARD]:
            with self.assertRaises(TypeError):
                mix.setSelectedMixingSpace(param)

        for param in [None, 'test']:
            with self.assertRaises(TypeError):
                mix.setSelectedMixingSpaceIdx(param)

        mix = None

    def test_get_processor(self):
        """
        Test getProcessor() function without ROLE_COLOR_PICKING role.
        """

        mix = OCIO.MixingColorSpaceManager(self.cfg)

        # Using default encoding (RGB) and mixing space (rendering space).

        proc = mix.getProcessor(workingSpaceName = 'lin_1', displayName = 'DISP_1', viewName = 'VIEW_1')
        grp = proc.createGroupTransform()
        self.assertEqual(len(grp), 1)
        self.assertEqual(grp[0].getTransformType(), OCIO.TRANSFORM_TYPE_MATRIX)

        # Same call without parameter names.
        proc = mix.getProcessor('lin_1', 'DISP_1', 'VIEW_1')
        grp = proc.createGroupTransform()
        self.assertEqual(len(grp), 1)
        self.assertEqual(grp[0].getTransformType(), OCIO.TRANSFORM_TYPE_MATRIX)

        # Same call with different a order for parameter names.
        proc = mix.getProcessor(viewName = 'VIEW_1', workingSpaceName = 'lin_1', displayName = 'DISP_1')
        grp = proc.createGroupTransform()
        self.assertEqual(len(grp), 1)
        self.assertEqual(grp[0].getTransformType(), OCIO.TRANSFORM_TYPE_MATRIX)

        # Change encoding to HSV.

        mix.setSelectedMixingEncoding('HSV')
        proc = mix.getProcessor(displayName = 'DISP_1', viewName = 'VIEW_1', workingSpaceName = 'lin_1')
        grp = proc.createGroupTransform()
        self.assertEqual(len(grp), 2)
        self.assertEqual(grp[0].getTransformType(), OCIO.TRANSFORM_TYPE_MATRIX)
        t = grp[1]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_FIXED_FUNCTION)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        self.assertEqual(t.getStyle(), OCIO.FIXED_FUNCTION_RGB_TO_HSV)

        # Change mixing space to 'Display Space'.

        mix.setSelectedMixingSpace('Display Space')

        proc = mix.getProcessor(displayName = 'DISP_1', viewName = 'VIEW_1', workingSpaceName = 'lin_1')
        grp = proc.createGroupTransform()
        self.assertEqual(len(grp), 2)
        t = grp[0]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_EXPONENT)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(t.getValue(), [2.6, 2.6, 2.6, 1.])
        t = grp[1]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_FIXED_FUNCTION)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        self.assertEqual(t.getStyle(), OCIO.FIXED_FUNCTION_RGB_TO_HSV)

        proc = mix.getProcessor(displayName = 'DISP_1', viewName = 'VIEW_1', workingSpaceName = 'lin_1',
            direction = OCIO.TRANSFORM_DIR_INVERSE)
        grp = proc.createGroupTransform()
        self.assertEqual(len(grp), 2)
        t = grp[1]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_EXPONENT)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        self.assertEqual(t.getValue(), [2.6, 2.6, 2.6, 1.])
        t = grp[0]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_FIXED_FUNCTION)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(t.getStyle(), OCIO.FIXED_FUNCTION_RGB_TO_HSV)

        with self.assertRaises(TypeError):
            proc = mix.getProcessor('lin_1', None, 'VIEW_1')

        with self.assertRaises(TypeError):
            proc = mix.getProcessor(workingSpaceName = 'lin_1', displayName = 666, viewName = 'VIEW_1')

        with self.assertRaises(TypeError):
            proc = mix.getProcessor(workingSpaceName = 'lin_1', displayName = 'DISP_1',
                                    viewName = OCIO.TRANSFORM_TYPE_LOG)

        with self.assertRaises(TypeError):
            proc = mix.getProcessor(workingSpaceName = 'lin_1', displayName = 'DISP_1')

        with self.assertRaises(TypeError):
            proc = mix.getProcessor(workingSpaceName = 'lin_1', displayName = 'DISP_1',
                                    errorName = 'VIEW_1')

        with self.assertRaises(TypeError):
            proc = mix.getProcessor(workingSpaceName = 'lin_1', displayName = 'DISP_1',
                                    viewName = 'VIEW_1', tooMany = True)

        with self.assertRaises(OCIO.Exception):
            proc = mix.getProcessor('lin_1', '', 'VIEW_1')

        with self.assertRaises(OCIO.Exception):
            proc = mix.getProcessor('', 'DISP_1', 'VIEW_1')

        with self.assertRaises(OCIO.Exception):
            proc = mix.getProcessor('not found', 'DISP_1', 'VIEW_1')

        mix = None

    def test_color_picking(self):
        """
        Test getProcessor() function with ROLE_COLOR_PICKING role. 
        """
        mix = OCIO.MixingColorSpaceManager(self.cfg)
        mixSpaces = mix.getMixingSpaces()
        self.assertEqual(len(mixSpaces), 2)

        self.cfg.setRole(OCIO.ROLE_COLOR_PICKING, 'log_1')
        mix.refresh(self.cfg)
        mixSpaces = mix.getMixingSpaces()
        self.assertEqual(len(mixSpaces), 1)
        self.assertEqual(mixSpaces[0], 'color_picking (log_1)')

        proc = mix.getProcessor(workingSpaceName = 'lin_1', displayName = 'DISP_1', viewName = 'VIEW_1')
        grp = proc.createGroupTransform()
        self.assertEqual(len(grp), 1)
        t = grp[0]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_LOG)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        self.assertEqual(t.getBase(), 2.)

        mix.setSelectedMixingEncodingIdx(1) # i.e. HSV

        proc = mix.getProcessor(displayName = 'DISP_1', viewName = 'VIEW_1', workingSpaceName = 'lin_1')
        grp = proc.createGroupTransform()
        self.assertEqual(len(grp), 2)
        t = grp[0]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_LOG)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        self.assertEqual(t.getBase(), 2.)
        t = grp[1]
        self.assertEqual(t.getTransformType(), OCIO.TRANSFORM_TYPE_FIXED_FUNCTION)
        self.assertEqual(t.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
        self.assertEqual(t.getStyle(), OCIO.FIXED_FUNCTION_RGB_TO_HSV)

        with self.assertRaises(OCIO.Exception):
            mix.setSelectedMixingSpaceIdx(1)

        # Print the MixingColorSpaceManager.
        self.assertEqual(str(mix),
            ('config: 779ee109137ff44de9afbf320685f4a2:6001c324468d497f99aa06d3014798d8, '
            'slider: [minEdge: 0, maxEdge: 1], mixingSpaces: [color_picking (log_1)], '
            'selectedMixingSpaceIdx: 0, selectedMixingEncodingIdx: 1, colorPicking'))

        mix = None

    def test_mixing_slider(self):
        """
        """
        mix = OCIO.MixingColorSpaceManager(self.cfg)

        slider = mix.getSlider(sliderMixingMinEdge=0.0, sliderMixingMaxEdge=1.0)

        # Print the slider.
        self.assertEqual(str(slider), 'minEdge: 0, maxEdge: 0.833864')

        # Set encoding.

        mix.setSelectedMixingEncodingIdx(1) # i.e. HSV

        # Needs linear to perceptually linear adjustment.

        mix.setSelectedMixingSpaceIdx(0) # i.e. Rendering Space
        self.assertEqual(mix.getSelectedMixingSpaceIdx(), 0)

        slider.setSliderMinEdge(0.0)
        slider.setSliderMaxEdge(1.0)

        self.assertTrue(test_percent_1000(    0, slider.getSliderMinEdge()))
        self.assertTrue(test_percent_1000(83386, slider.getSliderMaxEdge()))

        self.assertTrue(test_percent_1000(37923, slider.mixingToSlider(mixingUnits=0.1)))
        self.assertTrue(test_percent_1000(80144, slider.mixingToSlider(0.5)))

        self.assertTrue(test_percent_1000(10000, slider.sliderToMixing(sliderUnits=0.379232)))
        self.assertTrue(test_percent_1000(50000, slider.sliderToMixing(0.801448)))

        slider.setSliderMinEdge(-0.2)
        slider.setSliderMaxEdge(5.)

        self.assertTrue(test_percent_1000( 3792, slider.mixingToSlider(-0.1)))
        self.assertTrue(test_percent_1000(31573, slider.mixingToSlider( 0.1)))
        self.assertTrue(test_percent_1000(58279, slider.mixingToSlider( 0.5)))
        self.assertTrue(test_percent_1000(90744, slider.mixingToSlider( 3.0)))

        self.assertTrue(test_percent_1000(-10000, slider.sliderToMixing(0.037927)))
        self.assertTrue(test_percent_1000( 10000, slider.sliderToMixing(0.315733)))
        self.assertTrue(test_percent_1000( 50000, slider.sliderToMixing(0.582797)))
        self.assertTrue(test_percent_1000(300000, slider.sliderToMixing(0.907444)))

        # Does not need any linear to perceptually linear adjustment.

        mix.setSelectedMixingSpaceIdx(1) # i.e. Display Space

        # Print the slider.
        self.assertEqual(str(slider), 'minEdge: -0.2, maxEdge: 5')

        slider.setSliderMinEdge(0.0)
        slider.setSliderMaxEdge(1.0)

        self.assertTrue(test_percent_1000(     0, slider.getSliderMinEdge()))
        self.assertTrue(test_percent_1000(100000, slider.getSliderMaxEdge()))

        self.assertTrue(test_percent_1000(10000, slider.mixingToSlider(0.1)))
        self.assertTrue(test_percent_1000(50000, slider.mixingToSlider(0.5)))

        self.assertTrue(test_percent_1000(37923, slider.sliderToMixing(0.379232)))
        self.assertTrue(test_percent_1000(80144, slider.sliderToMixing(0.801448)))

        slider.setSliderMinEdge(-0.2)
        slider.setSliderMaxEdge(5.)

        self.assertTrue(test_percent_1000(     0, slider.mixingToSlider(slider.getSliderMinEdge())))
        self.assertTrue(test_percent_1000(100000, slider.mixingToSlider(slider.getSliderMaxEdge())))

        self.assertTrue(test_percent_1000( 1923, slider.mixingToSlider(-0.1)))
        self.assertTrue(test_percent_1000( 5769, slider.mixingToSlider( 0.1)))
        self.assertTrue(test_percent_1000(13461, slider.mixingToSlider( 0.5)))
        self.assertTrue(test_percent_1000(61538, slider.mixingToSlider( 3.0)))

        self.assertTrue(test_percent_1000( -277,  slider.sliderToMixing(0.037927)))
        self.assertTrue(test_percent_1000(144181, slider.sliderToMixing(0.315733)))
        self.assertTrue(test_percent_1000(283054, slider.sliderToMixing(0.582797)))
        self.assertTrue(test_percent_1000(451870, slider.sliderToMixing(0.907444)))

        # Change encoding.

        mix.setSelectedMixingEncodingIdx(0) # i.e. RGB

        # Needs linear to perceptually linear adjustment.
    
        mix.setSelectedMixingSpaceIdx(0) # i.e. Rendering Space

        slider.setSliderMinEdge(0.0)
        slider.setSliderMaxEdge(1.0)

        self.assertTrue(test_percent_1000(    0, slider.getSliderMinEdge()))
        self.assertTrue(test_percent_1000(83386, slider.getSliderMaxEdge()))

        self.assertTrue(test_percent_1000(37923, slider.mixingToSlider(0.1)))
        self.assertTrue(test_percent_1000(80144, slider.mixingToSlider(0.5)))

        self.assertTrue(test_percent_1000(10000, slider.sliderToMixing(0.379232)))
        self.assertTrue(test_percent_1000(50000, slider.sliderToMixing(0.801448)))

        slider.setSliderMinEdge(-0.2)
        slider.setSliderMaxEdge(5.)

        self.assertTrue(test_percent_1000( 3792, slider.mixingToSlider(-0.1)))
        self.assertTrue(test_percent_1000(31573, slider.mixingToSlider( 0.1)))
        self.assertTrue(test_percent_1000(58279, slider.mixingToSlider( 0.5)))
        self.assertTrue(test_percent_1000(90744, slider.mixingToSlider( 3.0)))

        self.assertTrue(test_percent_1000(-10000, slider.sliderToMixing(0.037927)))
        self.assertTrue(test_percent_1000( 10000, slider.sliderToMixing(0.315733)))
        self.assertTrue(test_percent_1000( 50000, slider.sliderToMixing(0.582797)))
        self.assertTrue(test_percent_1000(300000, slider.sliderToMixing(0.907444)))

        # Does not need any linear to perceptually linear adjustment.

        mix.setSelectedMixingSpaceIdx(1) # i.e. Display Space

        slider.setSliderMinEdge(0.0)
        slider.setSliderMaxEdge(1.0)

        self.assertTrue(test_percent_1000(     0, slider.getSliderMinEdge()))
        self.assertTrue(test_percent_1000(100000, slider.getSliderMaxEdge()))

        self.assertTrue(test_percent_1000(10000, slider.mixingToSlider(0.1)))
        self.assertTrue(test_percent_1000(50000, slider.mixingToSlider(0.5)))

        self.assertTrue(test_percent_1000(37923, slider.sliderToMixing(0.379232)))
        self.assertTrue(test_percent_1000(80144, slider.sliderToMixing(0.801448)))

        slider.setSliderMinEdge(-0.2)
        slider.setSliderMaxEdge(5.)

        self.assertTrue(test_percent_1000(     0, slider.mixingToSlider(slider.getSliderMinEdge())))
        self.assertTrue(test_percent_1000(100000, slider.mixingToSlider(slider.getSliderMaxEdge())))

        self.assertTrue(test_percent_1000( 1923, slider.mixingToSlider(-0.1)))
        self.assertTrue(test_percent_1000( 5769, slider.mixingToSlider( 0.1)))
        self.assertTrue(test_percent_1000(13461, slider.mixingToSlider( 0.5)))
        self.assertTrue(test_percent_1000(61538, slider.mixingToSlider( 3.0)))

        self.assertTrue(test_percent_1000( -277,  slider.sliderToMixing(0.037927)))
        self.assertTrue(test_percent_1000(144181, slider.sliderToMixing(0.315733)))
        self.assertTrue(test_percent_1000(283054, slider.sliderToMixing(0.582797)))
        self.assertTrue(test_percent_1000(451870, slider.sliderToMixing(0.907444)))

        # Update with ROLE_COLOR_PICKING role.

        self.cfg.setRole(OCIO.ROLE_COLOR_PICKING, 'log_1')
        mix.refresh(self.cfg)

        mix.setSelectedMixingEncodingIdx(1) # i.e. HSV
        mix.setSelectedMixingSpaceIdx(0) # i.e. Color Picker role

        slider = mix.getSlider(0.0, 1.0)
        self.assertTrue(test_percent_1000(50501, slider.mixingToSlider(0.50501)))
        self.assertTrue(test_percent_1000(50501, slider.sliderToMixing(0.50501)))

        mix.setSelectedMixingEncodingIdx(0) # i.e. RGB
        mix.setSelectedMixingSpaceIdx(0) # i.e. Color Picker role

        self.assertTrue(test_percent_1000(50501, slider.mixingToSlider(0.50501)))
        self.assertTrue(test_percent_1000(50501, slider.sliderToMixing(0.50501)))


        mix = None

# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest
import os
import sys

import PyOpenColorIO as OCIO
from UnitTestUtils import SAMPLE_CONFIG


class ColorSpaceHelpersTest(unittest.TestCase):
    def setUp(self):
        self.cfg = OCIO.Config().CreateFromStream(SAMPLE_CONFIG)

    def tearDown(self):
        self.cfg = None

    def test_menu_parameters(self):
        """
        Test the constructor() and accessors.
        """
        params = OCIO.ColorSpaceMenuParameters(self.cfg)
        self.assertEqual(params.getConfig().getCacheID(), self.cfg.getCacheID())
        self.assertEqual(len(params.getRole()), 0)
        self.assertEqual(len(params.getAppCategories()), 0)
        self.assertEqual(len(params.getUserCategories()), 0)
        self.assertEqual(len(params.getEncodings()), 0)
        self.assertEqual(params.getSearchReferenceSpaceType(), OCIO.SEARCH_REFERENCE_SPACE_ALL)
        self.assertFalse(params.getIncludeRoles())
        self.assertFalse(params.getIncludeNamedTransforms())

        params = OCIO.ColorSpaceMenuParameters(config = self.cfg)
        self.assertTrue(params.getConfig())
        self.assertEqual(params.getConfig().getCacheID(), self.cfg.getCacheID())

        params = OCIO.ColorSpaceMenuParameters(config = self.cfg,
                                               role = 'role',
                                               appCategories = 'categories',
                                               userCategories = 'user',
                                               encodings = 'video',
                                               searchReferenceSpaceType = OCIO.SEARCH_REFERENCE_SPACE_DISPLAY,
                                               includeRoles = True,
                                               includeNamedTransforms = True)
        self.assertTrue(params.getConfig())
        self.assertEqual(params.getConfig().getCacheID(), self.cfg.getCacheID())
        self.assertEqual(params.getRole(), 'role')
        self.assertEqual(params.getAppCategories(), 'categories')
        self.assertEqual(params.getUserCategories(), 'user')
        self.assertEqual(params.getEncodings(), 'video')
        self.assertEqual(params.getSearchReferenceSpaceType(), OCIO.SEARCH_REFERENCE_SPACE_DISPLAY)
        self.assertTrue(params.getIncludeRoles())
        self.assertTrue(params.getIncludeNamedTransforms())

        params.setRole('')
        self.assertEqual(params.getRole(), '')
        params.setAppCategories('cat1, cat2, cat3')
        self.assertEqual(params.getAppCategories(), 'cat1, cat2, cat3')
        params.setUserCategories('user1, user2')
        self.assertEqual(params.getUserCategories(), 'user1, user2')
        params.setEncodings('film')
        self.assertEqual(params.getEncodings(), 'film')
        params.setSearchReferenceSpaceType(OCIO.SEARCH_REFERENCE_SPACE_SCENE)
        self.assertEqual(params.getSearchReferenceSpaceType(), OCIO.SEARCH_REFERENCE_SPACE_SCENE)
        params.setIncludeRoles(False)
        self.assertFalse(params.getIncludeRoles())
        params.setIncludeRoles(True)
        self.assertTrue(params.getIncludeRoles())
        params.setIncludeRoles(False)
        params.setIncludeRoles()
        self.assertTrue(params.getIncludeRoles())
        params.setIncludeNamedTransforms()
        self.assertTrue(params.getIncludeNamedTransforms())

    def test_menu_creation_colorspaces(self):
        """
        Test the constructor() with only colorspaces.
        """

        # Parameters are needed.

        with self.assertRaises(TypeError):
            OCIO.ColorSpaceMenuHelper(None)

        # Create with sample config.

        params = OCIO.ColorSpaceMenuParameters(config = self.cfg)
        menu = OCIO.ColorSpaceMenuHelper(params)
        
        # [raw, lin_1, lin_2, log_1, in_1, in_2, in_3, view_1, view_2, view_3, lut_input_1, lut_input_2, lut_input_3,
        # display_lin_1, display_lin_2, display_log_1]
        self.assertEqual(menu.getNumColorSpaces(), 16)
        self.assertEqual(menu.getName(0), 'raw')
        self.assertEqual(menu.getUIName(0), 'raw')
        self.assertEqual(menu.getDescription(0), 'A raw color space. Conversions to and from this space are no-ops.')
        self.assertEqual(menu.getFamily(0), 'Raw')
        hlevels = menu.getHierarchyLevels(0)
        self.assertEqual(len(hlevels), 1)
        self.assertEqual(next(hlevels), 'Raw')

        self.assertEqual(menu.getName(1), 'lin_1')
        self.assertEqual(menu.getName(2), 'lin_2')
        self.assertEqual(menu.getName(3), 'log_1')
        self.assertEqual(menu.getName(4), 'in_1')

        hlevels = menu.getHierarchyLevels(4)
        self.assertEqual(len(hlevels), 3)
        self.assertEqual(next(hlevels), 'Input')
        self.assertEqual(next(hlevels), 'Camera')
        self.assertEqual(next(hlevels), 'Acme')

        self.assertEqual(menu.getName(5), 'in_2')
        self.assertEqual(menu.getName(6), 'in_3')
        self.assertEqual(menu.getName(7), 'view_1')
        self.assertEqual(menu.getName(8), 'view_2')
        self.assertEqual(menu.getName(9), 'view_3')
        self.assertEqual(menu.getName(10), 'lut_input_1')
        self.assertEqual(menu.getName(11), 'lut_input_2')
        self.assertEqual(menu.getName(12), 'lut_input_3')
        self.assertEqual(menu.getName(13), 'display_lin_1')
        self.assertEqual(menu.getName(14), 'display_lin_2')
        self.assertEqual(menu.getName(15), 'display_log_1')
        # Out of range index.
        self.assertEqual(menu.getName(16), '')
        self.assertEqual(menu.getUIName(16), '')
        self.assertEqual(menu.getDescription(16), '')
        self.assertEqual(menu.getFamily(16), '')
        hlevels = menu.getHierarchyLevels(16)
        self.assertEqual(len(hlevels), 0)

        self.assertEqual(str(menu),
            'config: 667ca4dc5b3779e570229fb7fd9cffe1:6001c324468d497f99aa06d3014798d8, '
            'includeColorSpaces: true, includeRoles: false, includeNamedTransforms: false, '
            'color spaces = [raw, lin_1, lin_2, log_1, in_1, in_2, in_3, view_1, view_2, view_3, '
            'lut_input_1, lut_input_2, lut_input_3, display_lin_1, display_lin_2, display_log_1]')


    def test_menu_creation_single_role(self):
        """
        Test the constructor() with single role.
        """
        params = OCIO.ColorSpaceMenuParameters(config = self.cfg, role = 'default')
        menu = OCIO.ColorSpaceMenuHelper(params)
        self.assertEqual(menu.getNumColorSpaces(), 1)
        self.assertEqual(menu.getName(0), 'raw')
        self.assertEqual(menu.getUIName(0), 'default (raw)')
        self.assertEqual(menu.getDescription(0), '')
        self.assertEqual(menu.getFamily(0), '')
        hlevels = menu.getHierarchyLevels(0)
        self.assertEqual(len(hlevels), 0)
        # Out of range index.
        self.assertEqual(menu.getName(1), '')
        self.assertEqual(menu.getUIName(1), '')
        self.assertEqual(menu.getDescription(1), '')
        self.assertEqual(menu.getFamily(1), '')
        hlevels = menu.getHierarchyLevels(1)
        self.assertEqual(len(hlevels), 0)

        # Other paramters are ignored if role exist.
        params.setAppCategories('file-io')
        menu = OCIO.ColorSpaceMenuHelper(params)
        self.assertEqual(menu.getNumColorSpaces(), 1)
        params.setAppCategories('unknown')
        menu = OCIO.ColorSpaceMenuHelper(params)
        self.assertEqual(menu.getNumColorSpaces(), 1)
        params.setAppCategories('file-io')
        params.setIncludeRoles()
        menu = OCIO.ColorSpaceMenuHelper(params)
        self.assertEqual(menu.getNumColorSpaces(), 1)

        # Other parameters are used if role does not exist.
        params = OCIO.ColorSpaceMenuParameters(config = self.cfg, role = 'non-existing',
                                               appCategories = 'working-space')
        menu = OCIO.ColorSpaceMenuHelper(params)
        self.assertEqual(menu.getNumColorSpaces(), 7)

    def test_menu_creation_catgeories(self):
        """
        Test the constructor() with categories.
        """
        params = OCIO.ColorSpaceMenuParameters(config = self.cfg,
                                               appCategories = 'working-space, file-io')
        menu = OCIO.ColorSpaceMenuHelper(params)
        # [lin_1, lin_2, log_1, in_1, in_2, in_3, lut_input_3, display_lin_1, display_lin_2, display_log_1]
        self.assertEqual(menu.getNumColorSpaces(), 10)
        self.assertEqual(menu.getName(0), 'lin_1')
        self.assertEqual(menu.getName(1), 'lin_2')
        self.assertEqual(menu.getName(2), 'log_1')
        self.assertEqual(menu.getName(3), 'in_1')
        self.assertEqual(menu.getName(4), 'in_2')
        self.assertEqual(menu.getName(5), 'in_3')
        self.assertEqual(menu.getName(6), 'lut_input_3')
        self.assertEqual(menu.getName(7), 'display_lin_1')
        self.assertEqual(menu.getName(8), 'display_lin_2')
        self.assertEqual(menu.getName(9), 'display_log_1')

        params.setSearchReferenceSpaceType(OCIO.SEARCH_REFERENCE_SPACE_SCENE)
        menu = OCIO.ColorSpaceMenuHelper(params)
        self.assertEqual(menu.getNumColorSpaces(), 7)
        params.setSearchReferenceSpaceType(OCIO.SEARCH_REFERENCE_SPACE_DISPLAY)
        menu = OCIO.ColorSpaceMenuHelper(params)
        self.assertEqual(menu.getNumColorSpaces(), 3)

        params.setEncodings('sdr-video')
        params.setSearchReferenceSpaceType(OCIO.SEARCH_REFERENCE_SPACE_SCENE)
        menu = OCIO.ColorSpaceMenuHelper(params)
        self.assertEqual(menu.getNumColorSpaces(), 2)
        self.assertEqual(menu.getName(0), 'in_1')
        self.assertEqual(menu.getName(1), 'in_2')
        params.setSearchReferenceSpaceType(OCIO.SEARCH_REFERENCE_SPACE_DISPLAY)
        menu = OCIO.ColorSpaceMenuHelper(params)
        self.assertEqual(menu.getNumColorSpaces(), 1)
        self.assertEqual(menu.getName(0), 'display_lin_2')
        params.setSearchReferenceSpaceType(OCIO.SEARCH_REFERENCE_SPACE_ALL)
        menu = OCIO.ColorSpaceMenuHelper(params)
        self.assertEqual(menu.getNumColorSpaces(), 3)

    def test_menu_creation_include_roles(self):
        """
        Test the constructor() with extra roles.
        """
        params = OCIO.ColorSpaceMenuParameters(config = self.cfg, includeRoles = True)
        menu = OCIO.ColorSpaceMenuHelper(params)
        self.assertEqual(menu.getNumColorSpaces(), 20)
        self.assertEqual(menu.getName(0), 'raw')
        self.assertEqual(menu.getName(1), 'lin_1')
        # Skip up to roles.
        self.assertEqual(menu.getName(16), 'default')
        self.assertEqual(menu.getName(17), 'reference')
        self.assertEqual(menu.getName(18), 'rendering')
        self.assertEqual(menu.getName(19), 'scene_linear')
        self.assertEqual(menu.getUIName(16), 'default (raw)')
        self.assertEqual(menu.getUIName(17), 'reference (lin_1)')
        self.assertEqual(menu.getUIName(18), 'rendering (lin_1)')
        self.assertEqual(menu.getUIName(19), 'scene_linear (lin_1)')
        hlevels = menu.getHierarchyLevels(16)
        self.assertEqual(len(hlevels), 1)
        self.assertEqual(next(hlevels), 'Roles')

        params = OCIO.ColorSpaceMenuParameters(config = self.cfg, includeRoles = True,
                                               appCategories='file-io, working-space')
        menu = OCIO.ColorSpaceMenuHelper(params)
        # [lin_1, lin_2, log_1, in_1, in_2, in_3, lut_input_3, display_lin_1, display_lin_2, display_log_1, default, reference, rendering, scene_linear]
        self.assertEqual(menu.getNumColorSpaces(), 14)
        self.assertEqual(menu.getName(0), 'lin_1')
        self.assertEqual(menu.getName(1), 'lin_2')
        self.assertEqual(menu.getName(2), 'log_1')
        self.assertEqual(menu.getName(3), 'in_1')
        self.assertEqual(menu.getName(4), 'in_2')
        self.assertEqual(menu.getName(5), 'in_3')
        self.assertEqual(menu.getName(6), 'lut_input_3')
        self.assertEqual(menu.getName(7), 'display_lin_1')
        self.assertEqual(menu.getName(8), 'display_lin_2')
        self.assertEqual(menu.getName(9), 'display_log_1')
        self.assertEqual(menu.getName(10), 'default')
        self.assertEqual(menu.getName(11), 'reference')
        self.assertEqual(menu.getName(12), 'rendering')
        self.assertEqual(menu.getName(13), 'scene_linear')
        self.assertEqual(menu.getUIName(10), 'default (raw)')
        self.assertEqual(menu.getUIName(11), 'reference (lin_1)')
        self.assertEqual(menu.getUIName(12), 'rendering (lin_1)')
        self.assertEqual(menu.getUIName(13), 'scene_linear (lin_1)')

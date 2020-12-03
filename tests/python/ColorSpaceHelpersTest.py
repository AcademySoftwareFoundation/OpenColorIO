# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest
import os
import sys

import PyOpenColorIO as OCIO
from UnitTestUtils import SIMPLE_CONFIG


class ColorSpaceHelpersTest(unittest.TestCase):
    def setUp(self):
        self.cfg = OCIO.Config().CreateFromStream(SIMPLE_CONFIG)

    def tearDown(self):
        self.cfg = None

    def test_menu_creation_no_extras(self):
        """
        Test the constructor() with no extras.
        """
        menu = OCIO.ColorSpaceMenuHelper(config = self.cfg)
        self.assertEqual(menu.getNumColorSpaces(), 6)
        self.assertEqual(menu.getName(0), 'raw')
        self.assertEqual(menu.getUIName(0), 'raw')
        self.assertEqual(menu.getDescription(0), 'A raw color space. Conversions to and from this space are no-ops.')
        self.assertEqual(menu.getFamily(0), 'raw')
        hlevels = menu.getHierarchyLevels(0)
        self.assertEqual(len(hlevels), 1)
        self.assertEqual(next(hlevels), 'raw')

        self.assertEqual(menu.getName(1), 'lnh')
        self.assertEqual(menu.getName(2), 'vd8')
        self.assertEqual(menu.getName(3), 'c1')
        self.assertEqual(menu.getName(4), 'c2')
        self.assertEqual(menu.getName(5), 'c3')
        # Out of range index.
        self.assertEqual(menu.getName(6), '')
        self.assertEqual(menu.getUIName(6), '')
        self.assertEqual(menu.getDescription(6), '')
        self.assertEqual(menu.getFamily(6), '')
        hlevels = menu.getHierarchyLevels(6)
        self.assertEqual(len(hlevels), 0)

        self.assertEqual(str(menu), 'Config: '
            '$6c4f503625d2f1f417522c7f220f1434:$4dd1c89df8002b409e089089ce8f24e7, '
            'includeFlag: [color spaces], color spaces = [raw, lnh, vd8, c1, c2, c3]')


    def test_menu_creation_single_role(self):
        """
        Test the constructor() with single role.
        """
        menu = OCIO.ColorSpaceMenuHelper(config = self.cfg, role = 'default')
        self.assertEqual(menu.getNumColorSpaces(), 1)
        self.assertEqual(menu.getName(0), 'default')
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
        menu = OCIO.ColorSpaceMenuHelper(config = self.cfg, role = 'default',
                                         categories='rendering')
        self.assertEqual(menu.getNumColorSpaces(), 1)
        menu = OCIO.ColorSpaceMenuHelper(config = self.cfg, role = 'default', categories='unknown')
        self.assertEqual(menu.getNumColorSpaces(), 1)
        menu = OCIO.ColorSpaceMenuHelper(config = self.cfg, role = 'default',
                                         categories='rendering',
                                         includeFlag=OCIO.ColorSpaceMenuHelper.INCLUDE_ROLES)
        self.assertEqual(menu.getNumColorSpaces(), 1)

        # Other paramters are used if role does not exist.
        menu = OCIO.ColorSpaceMenuHelper(config = self.cfg, role = 'non-existing',
                                         categories='rendering')
        self.assertEqual(menu.getNumColorSpaces(), 2)

    def test_menu_creation_catgeories(self):
        """
        Test the constructor() with categories.
        """
        menu = OCIO.ColorSpaceMenuHelper(config = self.cfg, categories='linear, sample')
        self.assertEqual(menu.getNumColorSpaces(), 2)
        self.assertEqual(menu.getName(0), 'c1')
        self.assertEqual(menu.getName(1), 'c3')

    def test_menu_creation_extra_roles(self):
        """
        Test the constructor() with extra roles.
        """
        menu = OCIO.ColorSpaceMenuHelper(config = self.cfg,
                                         includeFlag=OCIO.ColorSpaceMenuHelper.INCLUDE_ROLES)
        self.assertEqual(menu.getNumColorSpaces(), 8)
        self.assertEqual(menu.getName(0), 'raw')
        self.assertEqual(menu.getName(1), 'lnh')
        self.assertEqual(menu.getName(2), 'vd8')
        self.assertEqual(menu.getName(3), 'c1')
        self.assertEqual(menu.getName(4), 'c2')
        self.assertEqual(menu.getName(5), 'c3')
        self.assertEqual(menu.getName(6), 'default')
        self.assertEqual(menu.getName(7), 'scene_linear')
        self.assertEqual(menu.getUIName(6), 'default (raw)')
        self.assertEqual(menu.getUIName(7), 'scene_linear (lnh)')
        hlevels = menu.getHierarchyLevels(6)
        self.assertEqual(len(hlevels), 1)
        self.assertEqual(next(hlevels), 'Roles')

        menu = OCIO.ColorSpaceMenuHelper(config = self.cfg, categories='linear, sample',
                                         includeFlag=OCIO.ColorSpaceMenuHelper.INCLUDE_ROLES)
        self.assertEqual(menu.getNumColorSpaces(), 4)
        self.assertEqual(menu.getName(0), 'c1')
        self.assertEqual(menu.getName(1), 'c3')
        self.assertEqual(menu.getName(2), 'default')
        self.assertEqual(menu.getName(3), 'scene_linear')
        self.assertEqual(menu.getUIName(2), 'default (raw)')
        self.assertEqual(menu.getUIName(3), 'scene_linear (lnh)')

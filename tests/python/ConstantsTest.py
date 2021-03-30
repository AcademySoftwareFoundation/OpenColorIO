# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest, os, sys
import PyOpenColorIO as OCIO

class ConstantsTest(unittest.TestCase):

    def test_string_constants(self):
        """
        Verify string constants exposed by OCIO.
        """
        # EnvironmentMode (env. variables).
        self.assertEqual(OCIO.OCIO_CONFIG_ENVVAR, 'OCIO')
        self.assertEqual(OCIO.OCIO_ACTIVE_DISPLAYS_ENVVAR, 'OCIO_ACTIVE_DISPLAYS')
        self.assertEqual(OCIO.OCIO_ACTIVE_VIEWS_ENVVAR, 'OCIO_ACTIVE_VIEWS')
        self.assertEqual(OCIO.OCIO_INACTIVE_COLORSPACES_ENVVAR, 'OCIO_INACTIVE_COLORSPACES')
        self.assertEqual(OCIO.OCIO_OPTIMIZATION_FLAGS_ENVVAR, 'OCIO_OPTIMIZATION_FLAGS')
        self.assertEqual(OCIO.OCIO_USER_CATEGORIES_ENVVAR, 'OCIO_USER_CATEGORIES')

        # Cache (env. variables).
        self.assertEqual(OCIO.OCIO_DISABLE_ALL_CACHES, 'OCIO_DISABLE_ALL_CACHES')
        self.assertEqual(OCIO.OCIO_DISABLE_PROCESSOR_CACHES, 'OCIO_DISABLE_PROCESSOR_CACHES')
        self.assertEqual(OCIO.OCIO_DISABLE_CACHE_FALLBACK, 'OCIO_DISABLE_CACHE_FALLBACK')

        # Roles.
        self.assertEqual(OCIO.ROLE_DEFAULT, 'default')
        self.assertEqual(OCIO.ROLE_REFERENCE, 'reference')
        self.assertEqual(OCIO.ROLE_DATA, 'data')
        self.assertEqual(OCIO.ROLE_COLOR_PICKING, 'color_picking')
        self.assertEqual(OCIO.ROLE_SCENE_LINEAR, 'scene_linear')
        self.assertEqual(OCIO.ROLE_COMPOSITING_LOG, 'compositing_log')
        self.assertEqual(OCIO.ROLE_COLOR_TIMING, 'color_timing')
        self.assertEqual(OCIO.ROLE_TEXTURE_PAINT, 'texture_paint')
        self.assertEqual(OCIO.ROLE_MATTE_PAINT, 'matte_paint')
        self.assertEqual(OCIO.ROLE_RENDERING, 'rendering')
        self.assertEqual(OCIO.ROLE_INTERCHANGE_SCENE, 'aces_interchange')
        self.assertEqual(OCIO.ROLE_INTERCHANGE_DISPLAY, 'cie_xyz_d65_interchange')

        # Shared View.
        self.assertEqual(OCIO.OCIO_VIEW_USE_DISPLAY_NAME, '<USE_DISPLAY_NAME>')

        # FormatMetadata.
        self.assertEqual(OCIO.METADATA_DESCRIPTION, 'Description')
        self.assertEqual(OCIO.METADATA_INFO, 'Info')
        self.assertEqual(OCIO.METADATA_INPUT_DESCRIPTOR, 'InputDescriptor')
        self.assertEqual(OCIO.METADATA_OUTPUT_DESCRIPTOR, 'OutputDescriptor')
        self.assertEqual(OCIO.METADATA_NAME, 'name')
        self.assertEqual(OCIO.METADATA_ID, 'id')

        # FileRules.
        self.assertEqual(OCIO.DEFAULT_RULE_NAME, 'Default')
        self.assertEqual(OCIO.FILE_PATH_SEARCH_RULE_NAME, 'ColorSpaceNamePathSearch')



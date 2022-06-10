# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from re import S
import unittest

import PyOpenColorIO as OCIO

class BuiltinConfigRegistryTest(unittest.TestCase):
    # BuiltinRegistry singleton
    REGISTRY = None

    @classmethod
    def setUpClass(cls):
        cls.REGISTRY = OCIO.BuiltinConfigRegistry()

    def test_builtin_configs(self):
        """
        Test the built-in configurations
        """

        self.assertEqual(self.REGISTRY.getNumBuiltinConfigs(), 1)

        self.assertEqual(
            self.REGISTRY.getBuiltinConfigName(0), 
            "cg-config-v0.1.0_aces-v1.3_ocio-v2.1.1"
        )

        # Test that both methods returns the same config since CG.h cannot be included here.
        self.assertEqual(
            self.REGISTRY.getBuiltinConfig(0), 
            self.REGISTRY.getBuiltinConfigByName("cg-config-v0.1.0_aces-v1.3_ocio-v2.1.1")
        )

        self.assertEqual(
            self.REGISTRY.getDefaultBuiltinConfigName(),
            "cg-config-v0.1.0_aces-v1.3_ocio-v2.1.1"
        )

        self.assertTrue(self.REGISTRY.isBuiltinConfigRecommended(0))

        # ********************************
        # Testing some expected failures.
        # ********************************
        
        # Test isBuiltinConfigRecommended using an invalid config index.
        with self.assertRaisesRegex(OCIO.Exception, "Config index is out of range."):
            self.REGISTRY.isBuiltinConfigRecommended(999)

        # Test getBuiltinConfigName using an invalid config index.
        with self.assertRaisesRegex(OCIO.Exception, "Config index is out of range."):
            self.REGISTRY.getBuiltinConfigName(999)

        # Test getBuiltinConfig using an invalid config index.
        with self.assertRaisesRegex(OCIO.Exception, "Config index is out of range."):
            self.REGISTRY.getBuiltinConfig(999)

        # Test getBuiltinConfigByName using an unknown config name.
        with self.assertRaisesRegex(
            OCIO.Exception, "Could not find 'I do not exist' in the built-in configurations."
        ):
            self.REGISTRY.getBuiltinConfigByName("I do not exist")
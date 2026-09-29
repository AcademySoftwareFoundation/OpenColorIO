# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest

import PyOpenColorIO as OCIO


class ConfigCompatibilityHelpersTest(unittest.TestCase):
    def test_check_compatibility(self):
        """
        Test the ConfigCompatibilityHelpers.CheckCompatibility() function.
        """
        cfg = OCIO.Config()

        # An empty config does not meet the HDR display support requirements.
        self.assertFalse(
            OCIO.ConfigCompatibilityHelpers.CheckCompatibility(
                cfg, OCIO.CONFIG_HDR_DISPLAY_SUPPORT_26
            )
        )

        # Built-in configs version 4.0 and higher meet the requirements.
        cfg = OCIO.Config.CreateFromBuiltinConfig("ocio://default")

        self.assertTrue(
            OCIO.ConfigCompatibilityHelpers.CheckCompatibility(
                cfg, OCIO.CONFIG_HDR_DISPLAY_SUPPORT_26
            )
        )


if __name__ == "__main__":
    unittest.main()

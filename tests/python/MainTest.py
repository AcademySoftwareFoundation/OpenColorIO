# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest, os, sys
import PyOpenColorIO as OCIO

class MainTest(unittest.TestCase):

    FOO ="""ocio_profile_version: 2

search_path: \"\"
strictparsing: false
luma: [0.2126, 0.7152, 0.0722]

roles:
  default: raw

file_rules:
  - !<Rule> {name: ColorSpaceNamePathSearch}
  - !<Rule> {name: Default, colorspace: default}

displays:
  sRGB:
    - !<View> {name: Raw, colorspace: raw}

active_displays: []
active_views: []

colorspaces:
  - !<ColorSpace>
    name: raw
    family: raw
    equalitygroup: \"\"
    bitdepth: 32f
    description: |
      A raw color space. Conversions to and from this space are no-ops.
    isdata: true
    allocation: uniform
"""

    def test_interface(self):

        OCIO.ClearAllCaches()
        self.assertEqual(OCIO.LOGGING_LEVEL_INFO, OCIO.GetLoggingLevel())
        OCIO.SetLoggingLevel(OCIO.LOGGING_LEVEL_NONE)
        self.assertEqual(OCIO.LOGGING_LEVEL_NONE, OCIO.GetLoggingLevel())
        foo = OCIO.GetCurrentConfig()
        self.assertEqual(self.FOO, foo.serialize())
        OCIO.SetLoggingLevel(OCIO.LOGGING_LEVEL_INFO)
        bar = OCIO.Config().CreateFromStream(foo.serialize())
        OCIO.SetCurrentConfig(bar)
        wee = OCIO.GetCurrentConfig()


import unittest, os, sys
import PyOpenColorIO as OCIO

class MainTest(unittest.TestCase):
    
    FOO ="""ocio_profile_version: 1

search_path: \"\"
strictparsing: false
luma: [0.2126, 0.7152, 0.0722]

roles:
  default: raw

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
        #self.assertEqual("1.0.8", OCIO.version)
        #self.assertEqual(16779264, OCIO.hexversion)
        self.assertEqual(OCIO.Constants.LOGGING_LEVEL_INFO, OCIO.GetLoggingLevel())
        OCIO.SetLoggingLevel(OCIO.Constants.LOGGING_LEVEL_NONE)
        self.assertEqual(OCIO.Constants.LOGGING_LEVEL_NONE, OCIO.GetLoggingLevel())
        foo = OCIO.GetCurrentConfig()
        self.assertEqual(self.FOO, foo.serialize())
        OCIO.SetLoggingLevel(OCIO.Constants.LOGGING_LEVEL_INFO)
        bar = OCIO.Config().CreateFromStream(foo.serialize())
        OCIO.SetCurrentConfig(bar)
        wee = OCIO.GetCurrentConfig()

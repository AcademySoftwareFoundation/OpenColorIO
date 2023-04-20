# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest, os, sys
import PyOpenColorIO as OCIO

from UnitTestUtils import (TEST_DATAFILES_DIR)

class ProcessorCacheTest(unittest.TestCase):
    def test_processor_cache(self):
      CONFIG = """ocio_profile_version: 2

search_path: """ + TEST_DATAFILES_DIR + """
strictparsing: true
luma: [0.2126, 0.7152, 0.0722]

environment: {CS3: lut1d_green.ctf}

roles:
  default: cs1

displays:
  disp1:
    - !<View> {name: view1, colorspace: cs3}

colorspaces:
  - !<ColorSpace>
    name: cs1

  - !<ColorSpace>
    name: cs2
    from_scene_reference: !<MatrixTransform> {offset: [0.11, 0.12, 0.13, 0]}

  - !<ColorSpace>
    name: cs3
    from_scene_reference: !<FileTransform> {src: $CS3}
"""

      cfg = OCIO.Config.CreateFromStream(CONFIG)
      cfg.validate()

      # Test that clearProcessorCache clears the Processor cache.
      
      # Create two processors and confirm that it is the same object as expected
      procA = cfg.getProcessor("cs3", "disp1", "view1", OCIO.TRANSFORM_DIR_FORWARD)
      procB = cfg.getProcessor("cs3", "disp1", "view1", OCIO.TRANSFORM_DIR_FORWARD)

      # Comparing the address of both Processor objects to confirm if they are the same or not.
      self.assertEqual(procA, procB)

      cfg.clearProcessorCache()

      # Create a third processor and confirm that it is different from the previous two as the
      # the processor cache was cleared.
      procC = cfg.getProcessor("cs3", "disp1", "view1", OCIO.TRANSFORM_DIR_FORWARD)

      self.assertNotEqual(procC, procA)


      # Test that disable and re-enable the cache, using setProcessorCacheFlags, does not
      # clear the Processor cache.

      procD = cfg.getProcessor("cs3", "disp1", "view1", OCIO.TRANSFORM_DIR_FORWARD)

      # Disable and re-enable the processor cache.
      cfg.setProcessorCacheFlags(OCIO.PROCESSOR_CACHE_OFF)
      cfg.setProcessorCacheFlags(OCIO.PROCESSOR_CACHE_ENABLED)

      # Confirm that the processor is the same.
      procE = cfg.getProcessor("cs3", "disp1", "view1", OCIO.TRANSFORM_DIR_FORWARD)

      self.assertEqual(procD, procE)
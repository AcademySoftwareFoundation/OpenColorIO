# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# TODO: Add getFormatMetadata tests.

import unittest, os, sys
import PyOpenColorIO as OCIO

class BakerTest(unittest.TestCase):

    SIMPLE_PROFILE = """ocio_profile_version: 1

strictparsing: false

colorspaces:

  - !<ColorSpace>
    name: lnh
    bitdepth: 16f
    isdata: false
    allocation: lg2

  - !<ColorSpace>
    name: test
    bitdepth: 8ui
    isdata: false
    allocation: uniform
    to_reference: !<ExponentTransform> {value: [2.2, 2.2, 2.2, 1]}"""

    EXPECTED_LUT = """CSPLUTV100
3D

BEGIN METADATA
END METADATA

4
0.000977 0.039373 1.587401 64.000000
0.000000 0.333333 0.666667 1.000000
4
0.000977 0.039373 1.587401 64.000000
0.000000 0.333333 0.666667 1.000000
4
0.000977 0.039373 1.587401 64.000000
0.000000 0.333333 0.666667 1.000000

2 2 2
0.042823 0.042823 0.042823
6.622026 0.042823 0.042823
0.042823 6.622026 0.042823
6.622026 6.622026 0.042823
0.042823 0.042823 6.622026
6.622026 0.042823 6.622026
0.042823 6.622026 6.622026
6.622026 6.622026 6.622026

"""

    def test_interface(self):
        """
        Test similar to C++ CPU test.
        """
        bake = OCIO.Baker()
        cfg = OCIO.Config().CreateFromStream(self.SIMPLE_PROFILE)
        cs = cfg.getColorSpaces()
        self.assertEqual(len(cs), 2)
        bake.setConfig(cfg)
        cfg2 = bake.getConfig()
        cs2 = cfg2.getColorSpaces()
        self.assertEqual(len(cs2), 2)

        bake.setFormat("cinespace")
        self.assertEqual("cinespace", bake.getFormat())
        bake.setInputSpace("lnh")
        self.assertEqual("lnh", bake.getInputSpace())
        bake.setLooks("foo, +bar")
        self.assertEqual("foo, +bar", bake.getLooks())
        bake.setLooks("")
        bake.setTargetSpace("test")
        self.assertEqual("test", bake.getTargetSpace())
        bake.setShaperSize(4)
        self.assertEqual(4, bake.getShaperSize())
        bake.setCubeSize(2)
        self.assertEqual(2, bake.getCubeSize())
        output = bake.bake()
        lines = output.splitlines()
        expected_lines = self.EXPECTED_LUT.splitlines()
        self.assertEqual(len(lines), len(expected_lines))
        # Text compare for the first lines.
        for i in range(6):
            self.assertEqual(lines[i], expected_lines[i])
        # Compare values after (results might be slightly different on some plaforms).
        for i in range(6, len(lines)):
            # Skip blank lines.
            if lines[i] == '':
                continue
            # Line 16 is the cube size.
            if i == 16:
                self.assertEqual(lines[i], expected_lines[i])
                continue
            lf = lines[i].split(' ')
            elf = expected_lines[i].split(' ')
            for j in range(len(lf)):
                self.assertAlmostEqual(float(lf[j]), float(elf[j]), delta = 0.00001)
        fmts = bake.getFormats()
        self.assertEqual(len(fmts), 10)
        self.assertEqual("cinespace", fmts[4][0])
        self.assertEqual("3dl", fmts[1][1])


import unittest, os, sys
sys.path.append(os.path.join(sys.argv[1], "src", "pyglue"))
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
this is some metadata!
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
        bake = OCIO.Baker()
        bakee = bake.createEditableCopy()
        cfg = OCIO.Config().CreateFromStream(self.SIMPLE_PROFILE)
        self.assertEqual(2, cfg.getNumColorSpaces())
        bakee.setConfig(cfg)
        cfg2 = bakee.getConfig()
        self.assertEqual(2, cfg2.getNumColorSpaces())
        bakee.setFormat("cinespace")
        self.assertEqual("cinespace", bakee.getFormat())
        bakee.setType("3D")
        self.assertEqual("3D", bakee.getType())
        bakee.setMetadata("this is some metadata!")
        self.assertEqual("this is some metadata!", bakee.getMetadata())
        bakee.setInputSpace("lnh")
        self.assertEqual("lnh", bakee.getInputSpace())
        bakee.setLooks("foo, +bar")
        self.assertEqual("foo, +bar", bakee.getLooks())
        bakee.setLooks("")
        bakee.setTargetSpace("test")
        self.assertEqual("test", bakee.getTargetSpace())
        bakee.setShaperSize(4)
        self.assertEqual(4, bakee.getShaperSize())
        bakee.setCubeSize(2)
        self.assertEqual(2, bakee.getCubeSize())
        output = bakee.bake()
        self.assertEqual(self.EXPECTED_LUT, output)
        self.assertEqual(7, bakee.getNumFormats())
        self.assertEqual("cinespace", bakee.getFormatNameByIndex(2))
        self.assertEqual("3dl", bakee.getFormatExtensionByIndex(1))

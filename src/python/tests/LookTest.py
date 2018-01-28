
import unittest, os, sys
sys.path.append(os.path.join(sys.argv[1], "src", "pyglue"))
import PyOpenColorIO as OCIO

class LookTest(unittest.TestCase):
    
    def test_interface(self):
        lk = OCIO.Look()
        lk.setName("coollook")
        self.assertEqual("coollook", lk.getName())
        lk.setProcessSpace("somespace")
        self.assertEqual("somespace", lk.getProcessSpace())
        lk.setDescription("this is a test")
        self.assertEqual("this is a test", lk.getDescription())
        et = OCIO.ExponentTransform()
        et.setValue([0.1, 0.2, 0.3, 0.4])
        lk.setTransform(et)
        oet = lk.getTransform()
        vals = oet.getValue()
        self.assertAlmostEqual(0.2, vals[1], delta=1e-8)
        iet = OCIO.ExponentTransform()
        iet.setValue([-0.1, -0.2, -0.3, -0.4])
        lk.setInverseTransform(iet)
        ioet = lk.getInverseTransform()
        vals2 = ioet.getValue()
        self.assertAlmostEqual(-0.2, vals2[1], delta=1e-8)

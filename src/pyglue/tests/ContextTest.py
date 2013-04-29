
import unittest, os, sys
sys.path.append(os.path.join(sys.argv[1], "src", "pyglue"))
import PyOpenColorIO as OCIO

class ContextTest(unittest.TestCase):
    
    def test_interface(self):
        cont = OCIO.Context()
        cont.setSearchPath("testing123")
        cont.setWorkingDir("/dir/123")
        self.assertEqual("$af84c0ff921e48843d711a761e05b80f", cont.getCacheID())
        self.assertEqual("testing123", cont.getSearchPath())
        self.assertEqual("/dir/123", cont.getWorkingDir())
        cont.setStringVar("TeSt", "foobar")
        self.assertEqual("foobar", cont.getStringVar("TeSt"))
        self.assertEqual(1, cont.getNumStringVars())
        self.assertEqual("TeSt", cont.getStringVarNameByIndex(0))
        cont.loadEnvironment()
        self.assertNotEqual(0, cont.getNumStringVars())
        cont.setStringVar("TEST1", "foobar")
        self.assertEqual("/foo/foobar/bar", cont.resolveStringVar("/foo/${TEST1}/bar"))
        try:
            cont.setSearchPath("testing123")
            foo = cont.resolveFileLocation("test.lut")
            print foo
        except OCIO.ExceptionMissingFile, e:
            #print e
            pass


import unittest, os, sys
import PyOpenColorIO as OCIO

class ContextTest(unittest.TestCase):
    
    def test_interface(self):
        cont = OCIO.Context()
        cont.setSearchPath("testing123")
        cont.setWorkingDir("/dir/123")
        self.assertEqual("$4c2d66a612fc25ddd509591e1dead57b", cont.getCacheID())
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
        cont.clearStringVars()
        self.assertEqual(0, cont.getNumStringVars())
        self.assertEqual(OCIO.Constants.ENV_ENVIRONMENT_LOAD_PREDEFINED, cont.getEnvironmentMode())
        cont.setEnvironmentMode(OCIO.Constants.ENV_ENVIRONMENT_LOAD_ALL)
        self.assertEqual(OCIO.Constants.ENV_ENVIRONMENT_LOAD_ALL, cont.getEnvironmentMode())
        try:
            cont.setSearchPath("testing123")
            foo = cont.resolveFileLocation("test.lut")
            print(foo)
        except OCIO.ExceptionMissingFile as e:
            #print e
            pass

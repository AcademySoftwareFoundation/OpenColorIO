# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

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
        cont["TeSt"] = "foobar"
        self.assertEqual("foobar", cont["TeSt"])
        self.assertEqual(1, len(cont))
        self.assertEqual("TeSt", cont.getStringVarNameByIndex(0))
        cont.loadEnvironment()
        self.assertNotEqual(0, cont.getNumStringVars())
        cont.setStringVar("TEST1", "foobar")
        self.assertEqual("/foo/foobar/bar", cont.resolveStringVar("/foo/${TEST1}/bar"))
        cont.clearStringVars()
        self.assertEqual(0, cont.getNumStringVars())
        self.assertEqual(OCIO.ENV_ENVIRONMENT_LOAD_PREDEFINED, cont.getEnvironmentMode())
        cont.setEnvironmentMode(OCIO.ENV_ENVIRONMENT_LOAD_ALL)
        self.assertEqual(OCIO.ENV_ENVIRONMENT_LOAD_ALL, cont.getEnvironmentMode())
        cont.clearSearchPaths()
        self.assertEqual(0, cont.getNumSearchPaths())
        cont.addSearchPath("First/ Path")
        self.assertEqual(1, cont.getNumSearchPaths())
        cont.addSearchPath("D:\\Second\\Path\\")
        self.assertEqual(2, cont.getNumSearchPaths())
        self.assertEqual("First/ Path", cont.getSearchPathByIndex(0))
        self.assertEqual("D:\\Second\\Path\\", cont.getSearchPathByIndex(1))
        try:
            cont.setSearchPath("testing123")
            foo = cont.resolveFileLocation("test.lut")
            print(foo)
        except OCIO.ExceptionMissingFile as e:
            #print e
            pass

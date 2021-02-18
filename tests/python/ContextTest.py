# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest, os, sys
import PyOpenColorIO as OCIO

class ContextTest(unittest.TestCase):

    def test_interface(self):
        """
        Construct and use Context.
        """
        cont = OCIO.Context()
        cont.setSearchPath('testing123')
        cont.setWorkingDir('/dir/123')
        self.assertEqual('$4c2d66a612fc25ddd509591e1dead57b', cont.getCacheID())
        self.assertEqual('testing123', cont.getSearchPath())
        self.assertEqual('/dir/123', cont.getWorkingDir())
        cont['TeSt'] = 'foobar'
        self.assertEqual('foobar', cont['TeSt'])
        self.assertEqual(1, len(cont))
        cont_iter = iter(cont)
        self.assertEqual(len(cont_iter), 1)
        self.assertEqual(cont_iter[0], 'TeSt')
        cont.loadEnvironment()
        self.assertEqual(len(cont), 1)
        cont['TEST1'] = 'foobar'
        self.assertEqual(len(cont), 2)
        self.assertEqual('/foo/foobar/bar', cont.resolveStringVar('/foo/${TEST1}/bar'))
        cont.clearStringVars()
        self.assertEqual(len(cont), 0)
        self.assertEqual(OCIO.ENV_ENVIRONMENT_LOAD_PREDEFINED, cont.getEnvironmentMode())
        cont.setEnvironmentMode(OCIO.ENV_ENVIRONMENT_LOAD_ALL)
        self.assertEqual(OCIO.ENV_ENVIRONMENT_LOAD_ALL, cont.getEnvironmentMode())
        cont.clearSearchPaths()
        sp = cont.getSearchPaths()
        self.assertEqual(len(sp), 0)
        cont.addSearchPath('First/ Path')
        self.assertEqual(len(sp), 1)
        cont.addSearchPath('D:\\Second\\Path\\')
        self.assertEqual(len(sp), 2)
        self.assertEqual(next(sp), 'First/ Path')
        self.assertEqual(next(sp), 'D:\\Second\\Path\\')

        cont.setSearchPath('testing123')
        with self.assertRaises(OCIO.ExceptionMissingFile):
            foo = cont.resolveFileLocation('test.lut')

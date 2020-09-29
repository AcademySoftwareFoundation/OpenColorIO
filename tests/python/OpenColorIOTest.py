# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.


import unittest
import os
import sys

import PyOpenColorIO as OCIO


class OpenColorIOTest(unittest.TestCase):

    def test_env_variable(self):
        """
        Test Get/SetEnvVariable().
        """
        self.assertFalse(OCIO.IsEnvVariablePresent('MY_ENVAR'))
        OCIO.SetEnvVariable('MY_ENVAR', 'TOTO')
        self.assertTrue(OCIO.IsEnvVariablePresent('MY_ENVAR'))
        self.assertEqual(OCIO.GetEnvVariable('MY_ENVAR'), 'TOTO')

        OCIO.UnsetEnvVariable('MY_ENVAR')
        self.assertFalse(OCIO.IsEnvVariablePresent('MY_ENVAR'))

        OCIO.SetEnvVariable(value='TOTO', name='MY_ENVAR')
        self.assertTrue(OCIO.IsEnvVariablePresent(name='MY_ENVAR'))
        self.assertEqual(OCIO.GetEnvVariable(name='MY_ENVAR'), 'TOTO')

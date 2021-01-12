# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.


import unittest
import os
import sys
import inspect

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

    def test_bindings_transforms(self):
        """
        Tests polymorphism issue where transforms are cast as parent class when using GroupTransforms.
        Flagged in https://github.com/AcademySoftwareFoundation/OpenColorIO/issues/1211
        """
        allTransformsAsGroup = OCIO.GroupTransform()
        # Search for all transform types in order to handle future transforms
        for n, c in inspect.getmembers(OCIO):
            if hasattr(c, 'getTransformType'):
                try:
                    # Attempt to construct each Transform subclass, raising exception in order to filter
                    # the parent OCIO.Transform class
                    allTransformsAsGroup.appendTransform(c())
                except TypeError as e:
                    # Ensure we only catch and filter for this specific error
                    self.assertEqual(
                        str(e),
                        'PyOpenColorIO.Transform: No constructor defined!',
                        'Unintended Error Raised: {0}'.format(e)
                    )
        for transform in allTransformsAsGroup:
            # Ensure no transforms have been cast as parent transform
            self.assertNotEqual(
                type(transform),
                OCIO.Transform,
                """Transform has unintentionally been cast as parent class!
                transform.getTransformType(): {0}
                type(transform): {1}
                """.format(transform.getTransformType(), type(transform))
            )

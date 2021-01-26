# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest, os, sys
import PyOpenColorIO as OCIO
import inspect

class TransformsTest(unittest.TestCase):

    def test_binding_group_polymorphism(self):
        """
        Tests polymorphism issue where transforms are cast as parent class when using
        GroupTransforms. Flagged in https://github.com/AcademySoftwareFoundation/OpenColorIO/issues/1211
        """
        # Default arguments for Transforms that can't be instantiated without arguments.
        default_args = {
            OCIO.FixedFunctionTransform: {
                'style': OCIO.FIXED_FUNCTION_RGB_TO_HSV
            },
            OCIO.LogCameraTransform: {
                'linSideBreak': [0.5, 0.5, 0.5]
            }
        }

        allTransformsAsGroup = OCIO.GroupTransform()
        # Search for all transform types in order to handle future transforms
        for n, c in inspect.getmembers(OCIO):
            if hasattr(c, 'getTransformType'):
                try:
                    # Attempt to construct each Transform subclass, raising exception in order to
                    # filter the parent OCIO.Transform class
                    if c in default_args:
                        # Plug in kwargs if needed
                        allTransformsAsGroup.appendTransform(c(**default_args[c]))
                    else:
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

                Are there pybind polymorphic_type_hooks in src/bindings/PyOpenColorIO.h for this transform?
                """.format(transform.getTransformType(), type(transform))
            )

# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest

import PyOpenColorIO as OCIO


class BuiltinTransformTest(unittest.TestCase):
    # BuiltinTransformRegistry singleton
    REGISTRY = None

    # Default values
    DEFAULT_STYLE = 'IDENTITY'
    DEFAULT_DESC = ''
    DEFAULT_STR = '<BuiltinTransform style = IDENTITY, direction = forward>'

    # One reference style
    EXAMPLE_STYLE = 'ACES-AP0_to_CIE-XYZ-D65_BFD'
    EXAMPLE_DESC = (
        'Convert ACES AP0 primaries to CIE XYZ with a D65 white point with '
        'Bradford adaptation')

    @classmethod
    def setUpClass(cls):
        cls.REGISTRY = OCIO.BuiltinTransformRegistry()

    def setUp(self):
        self.builtin_tr = OCIO.BuiltinTransform()

    def cleanUp(self):
        self.builtin_tr = None

    def test_style(self):
        # Default style is identity
        self.assertEqual(self.builtin_tr.getStyle(), self.DEFAULT_STYLE)
        self.assertEqual(self.builtin_tr.getDescription(), self.DEFAULT_DESC)

        # Set/get all styles from registry
        for style, desc in self.REGISTRY.getBuiltins():
            self.builtin_tr.setStyle(style)
            self.assertEqual(self.builtin_tr.getStyle(), style)
            self.assertEqual(self.builtin_tr.getDescription(), desc)
            self.assertIsNone(self.builtin_tr.validate())  # Does not raise

        # Invalid style raises OCIO Exception
        with self.assertRaises(OCIO.Exception):
            self.builtin_tr.setStyle("invalid")

        # Safe invalid type handling
        for invalid in (None, 1, True):
            with self.assertRaises(TypeError):
                self.builtin_tr.setStyle(invalid)

    def test_direction(self):
        # All transform directions are supported
        for direction in OCIO.TransformDirection.__members__.values():
            self.builtin_tr.setDirection(direction)
            self.assertEqual(self.builtin_tr.getDirection(), direction)

            if direction != OCIO.TRANSFORM_DIR_UNKNOWN:
                # Does not raise
                self.assertIsNone(self.builtin_tr.validate())
            else:
                # TRANSFORM_DIR_UNKNOWN raises OCIO Exception
                with self.assertRaises(OCIO.Exception):
                    self.builtin_tr.validate()

    def test_constructor_keyword(self):
        # Keyword args in order
        builtin_tr1 = OCIO.BuiltinTransform(style=self.EXAMPLE_STYLE,
                                            dir=OCIO.TRANSFORM_DIR_FORWARD)

        self.assertEqual(builtin_tr1.getStyle(), self.EXAMPLE_STYLE)
        self.assertEqual(builtin_tr1.getDescription(), self.EXAMPLE_DESC)
        self.assertEqual(builtin_tr1.getDirection(), 
                         OCIO.TRANSFORM_DIR_FORWARD)

        # Keyword args out of order
        builtin_tr2 = OCIO.BuiltinTransform(dir=OCIO.TRANSFORM_DIR_FORWARD,
                                            style=self.EXAMPLE_STYLE)

        self.assertEqual(builtin_tr2.getStyle(), self.EXAMPLE_STYLE)
        self.assertEqual(builtin_tr2.getDescription(), self.EXAMPLE_DESC)
        self.assertEqual(builtin_tr2.getDirection(), 
                         OCIO.TRANSFORM_DIR_FORWARD)

    def test_constructor_positional(self):
        # Positional args
        builtin_tr = OCIO.BuiltinTransform(self.EXAMPLE_STYLE, 
                                           OCIO.TRANSFORM_DIR_FORWARD)

        self.assertEqual(builtin_tr.getStyle(), self.EXAMPLE_STYLE)
        self.assertEqual(builtin_tr.getDescription(), self.EXAMPLE_DESC)
        self.assertEqual(builtin_tr.getDirection(), 
                         OCIO.TRANSFORM_DIR_FORWARD)

    def test_str(self):
        # str() and print() result matches C++ << operator
        self.assertEqual(str(self.builtin_tr), self.DEFAULT_STR)

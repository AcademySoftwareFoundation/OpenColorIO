# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import collections
import unittest

import PyOpenColorIO as OCIO


class BuiltinTransformRegistryTest(unittest.TestCase):
    # BuiltinTransformRegistry singleton
    REGISTRY = None

    @classmethod
    def setUpClass(cls):
        cls.REGISTRY = OCIO.BuiltinTransformRegistry()

    def test_iterable(self):
        # Iterate all styles, validating all are non-empty strings
        all_styles = []
        for style in self.REGISTRY:
            self.assertIsInstance(style, str)
            self.assertIsInstance(self.REGISTRY[style], str)
            all_styles.append(style)

        # All styles were iterated over, and __len__ and list() behave
        self.assertEqual(len(all_styles), len(self.REGISTRY))
        self.assertListEqual(all_styles, list(self.REGISTRY))

        # Test iterator instance
        iterator = iter(self.REGISTRY)
        self.assertIsInstance(iterator, collections.Iterable)

        # Iterator size is available
        self.assertEqual(len(iterator), len(self.REGISTRY))
        
        # Iterator supports range-based loop and indexing
        values = list(iterator)
        for i in range(len(iterator)):
            # Item at index matches list result
            self.assertEqual(iterator[i], values[i])
            self.assertIsInstance(iterator[i], str)

    def test_get_builtins(self):
        # tuple iterator (like dict.items())
        for item in self.REGISTRY.getBuiltins():
            self.assertIsInstance(item, tuple)
            self.assertEqual(len(item), 2)
            self.assertIsInstance(item[0], str)
            self.assertIsInstance(item[1], str)

        # tuple unpacking support
        for style, desc in self.REGISTRY.getBuiltins():
            # __getitem__ has correct result
            self.assertEqual(self.REGISTRY[style], desc)
            self.assertIsInstance(style, str)
            self.assertIsInstance(desc, str)

        # Test iterator instance
        iterator = self.REGISTRY.getBuiltins()
        self.assertIsInstance(iterator, collections.Iterable)

        # Iterator size is available
        self.assertEqual(len(iterator), len(self.REGISTRY))
        
        # Iterator supports range-based loop and indexing
        values = list(iterator)
        for i in range(len(iterator)):
            # Item at index matches list result
            self.assertEqual(iterator[i], values[i])

            self.assertIsInstance(iterator[i], tuple)
            self.assertEqual(len(iterator[i]), 2)
            self.assertIsInstance(iterator[i][0], str)
            self.assertIsInstance(iterator[i][1], str)

    def test_contains(self):
        # Valid __contains__ for all styles
        for style in self.REGISTRY:
            self.assertTrue(style in self.REGISTRY)
            self.assertFalse(style not in self.REGISTRY)

        # Invalid __contains__ for non-styles
        self.assertTrue("invalid" not in self.REGISTRY)
        self.assertFalse("invalid" in self.REGISTRY)

    def test_multi_reference(self):
        # Registry is a singleton. Make sure multiple Python 
        # instances can be held.
        instances = []
        for i in range(10):
            instances.append(OCIO.BuiltinTransformRegistry())

        # Other instances should still function after deleting one. The 
        # underlying C++ object is not deleted.
        instance_0 = instances.pop(0)
        self.assertEqual(len(instances), 9)
        del instance_0

        # Variable is no longer defined
        with self.assertRaises(NameError):
            len(instance_0)

        # Test underlying C++ reference validity by accessing registry 
        # data for each instance.
        for instance in instances:
            self.assertGreaterEqual(len(instance), 1)
            self.assertEqual(len(instance), len(self.REGISTRY))

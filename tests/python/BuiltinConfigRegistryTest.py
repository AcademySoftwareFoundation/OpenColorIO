# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

try:
    from collections.abc import Iterable
except ImportError:
    # Python 2
    from collections import Iterable

from re import S
import unittest
from UnitTestUtils import STRING_TYPES

import PyOpenColorIO as OCIO

class BuiltinConfigRegistryTest(unittest.TestCase):
    # BuiltinRegistry singleton.
    REGISTRY = None

    @classmethod
    def setUpClass(cls):
        cls.REGISTRY = OCIO.BuiltinConfigRegistry()

    def test_builtin_config_iterable(self):
        all_names = []
        for name in self.REGISTRY:
            self.assertIsInstance(name, STRING_TYPES)
            self.assertIsInstance(self.REGISTRY[name], STRING_TYPES)
            all_names.append(name)
        
        # All names were iterated over, and __len__ and list() behave.
        self.assertEqual(len(all_names), len(self.REGISTRY))
        self.assertListEqual(all_names, list(self.REGISTRY))

        # Test iterator instance.
        iterator = iter(self.REGISTRY)
        self.assertIsInstance(iterator, Iterable)

        # Iterator size is available.
        self.assertEqual(len(iterator), len(self.REGISTRY))
        
        # Iterator supports range-based loop and indexing.
        values = list(iterator)
        for i in range(len(iterator)):
            # Item at index matches list result
            self.assertEqual(iterator[i], values[i])
            self.assertIsInstance(iterator[i], STRING_TYPES)

    def test_contains(self):
        # Valid __contains__ for all names.
        for name in self.REGISTRY:
            self.assertTrue(name in self.REGISTRY)
            self.assertFalse(name not in self.REGISTRY)

        # Invalid __contains__ for non-name.
        self.assertTrue("I do not exist" not in self.REGISTRY)

    def test_get_builtin_configs(self):
        # tuple iterator (like dict.items())
        for item in self.REGISTRY.getBuiltinConfigs():
            self.assertIsInstance(item, tuple)
            # check if there are four items per tuple.
            self.assertEqual(len(item), 4)
            # name
            self.assertIsInstance(item[0], STRING_TYPES)
            # uiname
            self.assertIsInstance(item[1], STRING_TYPES)
            # isRecommended
            self.assertIsInstance(item[2], bool)
            # isDefault
            self.assertIsInstance(item[3], bool)

        # tuple unpacking support.
        for name, uiname, isRecommended, isDefault in self.REGISTRY.getBuiltinConfigs():
            self.assertIsInstance(name, STRING_TYPES)
            self.assertIsInstance(uiname, STRING_TYPES)
            self.assertIsInstance(isRecommended, bool)
            self.assertIsInstance(isDefault, bool)

        # Test iterator instance.
        iterator = self.REGISTRY.getBuiltinConfigs()
        self.assertIsInstance(iterator, Iterable)

        # Iterator size is available.
        self.assertEqual(len(iterator), len(self.REGISTRY))
        
        # Iterator supports range-based loop and indexing.
        values = list(iterator)
        for i in range(len(iterator)):
            # Item at index matches list result
            self.assertEqual(iterator[i], values[i])
            self.assertIsInstance(iterator[i], tuple)
            self.assertEqual(len(iterator[i]), 4)
            # name
            self.assertIsInstance(iterator[i][0], STRING_TYPES)
            # uiname
            self.assertIsInstance(iterator[i][1], STRING_TYPES)
            # isRecommended
            self.assertIsInstance(iterator[i][2], bool)
            # isDefault
            self.assertIsInstance(iterator[i][3], bool)


        # Config specific tests

        # Test number of configs.
        self.assertEqual(len(self.REGISTRY), 8)

        # Test for the default built-in config.
        self.assertEqual(
            self.REGISTRY.getDefaultBuiltinConfigName(),
            "cg-config-v4.0.0_aces-v2.0_ocio-v2.5"
        )

        # Test the CG configs.

        cfgidx = 0
        # Name
        self.assertEqual(values[cfgidx][0], "cg-config-v1.0.0_aces-v1.3_ocio-v2.1")
        # UI name
        self.assertEqual(values[cfgidx][1], 
            ("Academy Color Encoding System - CG Config [COLORSPACES v1.0.0] [ACES v1.3] "
            "[OCIO v2.1]"))
        # isRecommended
        self.assertEqual(values[cfgidx][2], False)
		# isDefault
        self.assertEqual(values[cfgidx][3], False)

        cfgidx += 1
        # Name
        self.assertEqual(values[cfgidx][0], "cg-config-v2.1.0_aces-v1.3_ocio-v2.3")
        # UI name
        self.assertEqual(values[cfgidx][1], 
            ("Academy Color Encoding System - CG Config [COLORSPACES v2.0.0] [ACES v1.3] "
            "[OCIO v2.3]"))
        # isRecommended
        self.assertEqual(values[cfgidx][2], False)
        # isDefault
        self.assertEqual(values[cfgidx][3], False)

        cfgidx += 1
        # Name
        self.assertEqual(values[cfgidx][0], "cg-config-v2.2.0_aces-v1.3_ocio-v2.4")
        # UI name
        self.assertEqual(values[cfgidx][1], 
            ("Academy Color Encoding System - CG Config [COLORSPACES v2.2.0] [ACES v1.3] "
            "[OCIO v2.4]"))
        # isRecommended
        self.assertEqual(values[cfgidx][2], False)
		# isDefault
        self.assertEqual(values[cfgidx][3], False)

        cfgidx += 1
        # Name
        self.assertEqual(values[cfgidx][0], "cg-config-v4.0.0_aces-v2.0_ocio-v2.5")
        # UI name
        self.assertEqual(values[cfgidx][1], 
            ("Academy Color Encoding System - CG Config [COLORSPACES v4.0.0] [ACES v2.0] "
            "[OCIO v2.5]"))
        # isRecommended
        self.assertEqual(values[cfgidx][2], True)
		# isDefault
        self.assertEqual(values[cfgidx][3], True)

        # Test the Studio configs

        cfgidx += 1
        # Test the Studio configs
        # Name
        self.assertEqual(values[cfgidx][0], "studio-config-v1.0.0_aces-v1.3_ocio-v2.1")
        # UI name
        self.assertEqual(values[cfgidx][1], 
            ("Academy Color Encoding System - Studio Config [COLORSPACES v1.0.0] [ACES v1.3] "
            "[OCIO v2.1]"))
        # isRecommended
        self.assertEqual(values[cfgidx][2], False)
		# isDefault
        self.assertEqual(values[cfgidx][3], False)

        cfgidx += 1
        # Name
        self.assertEqual(values[cfgidx][0], "studio-config-v2.1.0_aces-v1.3_ocio-v2.3")
        # UI name
        self.assertEqual(values[cfgidx][1], 
            ("Academy Color Encoding System - Studio Config [COLORSPACES v2.0.0] [ACES v1.3] "
            "[OCIO v2.3]"))
        # isRecommended
        self.assertEqual(values[cfgidx][2], False)
        # isDefault
        self.assertEqual(values[cfgidx][3], False)

        cfgidx += 1
        # Name
        self.assertEqual(values[cfgidx][0], "studio-config-v2.2.0_aces-v1.3_ocio-v2.4")
        # UI name
        self.assertEqual(values[cfgidx][1], 
            ("Academy Color Encoding System - Studio Config [COLORSPACES v2.2.0] [ACES v1.3] "
            "[OCIO v2.4]"))
        # isRecommended
        self.assertEqual(values[cfgidx][2], False)
        # isDefault
        self.assertEqual(values[cfgidx][3], False)

        cfgidx += 1
        # Name
        self.assertEqual(values[cfgidx][0], "studio-config-v4.0.0_aces-v2.0_ocio-v2.5")
        # UI name
        self.assertEqual(values[cfgidx][1], 
            ("Academy Color Encoding System - Studio Config [COLORSPACES v4.0.0] [ACES v2.0] "
            "[OCIO v2.5]"))
        # isRecommended
        self.assertEqual(values[cfgidx][2], True)
        # isDefault
        self.assertEqual(values[cfgidx][3], False)

    def test_multi_reference(self):
        # Registry is a singleton. Make sure multiple Python 
        # instances can be held.
        instances = []
        for i in range(10):
            instances.append(OCIO.BuiltinConfigRegistry())

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

# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import copy
import unittest
import os
import sys

import PyOpenColorIO as OCIO
from UnitTestUtils import SIMPLE_CONFIG, TEST_NAMES


class ViewingRulesTest(unittest.TestCase):
    def setUp(self):
        self.rules = OCIO.ViewingRules()

    def tearDown(self):
        self.rule = None

    def test_copy(self):
        """
        Test the deepcopy() method.
        """

        self.rules.insertRule(0, "rule1")
        self.rules.insertRule(1, "rule2")
        self.rules.insertRule(2, "rule3")

        self.rules.addColorSpace(0, "cs1")
        self.rules.addEncoding(1, "enc1")
        self.rules.setCustomKey(2, "key1", "value1")

        other = copy.deepcopy(self.rules)
        self.assertFalse(other is self.rules)

        self.assertEqual(other.getNumEntries(), self.rules.getNumEntries())
        for idx in range(self.rules.getNumEntries()):
            self.assertEqual(other.getName(idx), self.rules.getName(idx))
        self.assertEqual(list(other.getColorSpaces(0)), list(self.rules.getColorSpaces(0)))
        self.assertEqual(list(other.getEncodings(1)), list(self.rules.getEncodings(1)))
        self.assertEqual(list(other.getCustomKeyName(2, 0)), list(self.rules.getCustomKeyName(2, 0)))
        self.assertEqual(list(other.getCustomKeyValue(2, 0)), list(self.rules.getCustomKeyValue(2, 0)))

    def test_insert_remove(self):
        """
        Test the insertRule(), removeRule(), getNumEntrie(), getIndexForRule(), getName() methods.
        """

        # Empty object.
        self.assertEqual(0, self.rules.getNumEntries())

        # Add some rules.
        rule1 = "rule1"
        rule2 = "rule2"
        self.rules.insertRule(0, rule1)
        self.rules.insertRule(1, rule2)

        self.assertEqual(2, self.rules.getNumEntries())
        self.assertEqual(rule1, self.rules.getName(0))
        self.assertEqual(rule2, self.rules.getName(1))

        self.assertEqual(0, self.rules.getIndexForRule(rule1))
        self.assertEqual(1, self.rules.getIndexForRule(rule2))

        # Remove a rule.
        self.rules.removeRule(0)
        self.assertEqual(1, self.rules.getNumEntries())
        self.assertEqual(rule2, self.rules.getName(0))
        self.rules.insertRule(0, rule1)
        self.assertEqual(2, self.rules.getNumEntries())
        self.assertEqual(rule1, self.rules.getName(0))
        self.assertEqual(rule2, self.rules.getName(1))


        # Index has to be valid.
        rule3 = "rule3"
        with self.assertRaises(OCIO.Exception):
            self.rules.insertRule(4, rule3)

        # Rule has to be present.
        with self.assertRaises(OCIO.Exception):
            self.rules.getIndexForRule(rule3)

        # Index has to be valid.
        with self.assertRaises(OCIO.Exception):
            self.rules.getName(3)

        # Index has to be valid.
        with self.assertRaises(OCIO.Exception):
            self.rules.removeRule(3)

        # Rule name must be unique.
        with self.assertRaises(OCIO.Exception):
            self.rules.insertRule(0, rule2)

    def test_color_spaces(self):
        """
        Test the getColorSpaces(), addColorSpace() and removeColorSpace() methods.
        """

        # Add new rule.
        ruleCS = "rule_cs"
        self.rules.insertRule(0, ruleCS)

        # Test empty color space list.
        self.assertEqual(0, len(self.rules.getColorSpaces(0)))
        with self.assertRaises(IndexError):
            self.rules.getColorSpaces(0)[0]

        # Test with defined TEST_NAMES.
        for i, y in enumerate(TEST_NAMES):
            self.rules.addColorSpace(0, y)
            self.assertEqual(i + 1, len(self.rules.getColorSpaces(0)))

        iterator = self.rules.getColorSpaces(0)
        for a in TEST_NAMES:
            self.assertEqual(a, next(iterator))

        # Test the length of color spaces is zero after removing all color spaces.
        for i in range(len(TEST_NAMES)):
            self.rules.removeColorSpace(0, 0)
        self.assertEqual(0, len(self.rules.getColorSpaces(0)))

        # Testing individually adding and removing a color space.
        self.rules.addColorSpace(0, TEST_NAMES[0])
        self.assertEqual(1, len(self.rules.getColorSpaces(0)))
        self.rules.addColorSpace(0, TEST_NAMES[1])
        self.assertEqual(2, len(self.rules.getColorSpaces(0)))
        self.rules.removeColorSpace(0, 1)
        self.assertEqual(1, len(self.rules.getColorSpaces(0)))

        # Index has to be valid.
        with self.assertRaises(OCIO.Exception):
            self.rules.removeColorSpace(0, 1)

        # Can't add encoding if there are color spaces.
        with self.assertRaises(OCIO.Exception):
            self.rules.addEncoding(0, "NoMix")

    def test_encodings(self):
        """
        Test the getEncodings(), addEncoding() and removeEncoding() methods.
        """

        # Add new rule.
        ruleEnc = "rule_enc"
        self.rules.insertRule(0, ruleEnc)

        # Test empty encodings list.
        self.assertEqual(0, len(self.rules.getEncodings(0)))
        with self.assertRaises(IndexError):
            self.rules.getEncodings(0)[0]

        # Test with defined TEST_NAMES.
        for i, y in enumerate(TEST_NAMES):
            self.rules.addEncoding(0, y)
            self.assertEqual(i + 1, len(self.rules.getEncodings(0)))

        iterator = self.rules.getEncodings(0)
        for a in TEST_NAMES:
            self.assertEqual(a, next(iterator))

        # Test the length of encodings is zero after removing all encodings.
        for i in range(len(TEST_NAMES)):
            self.rules.removeEncoding(0, 0)
        self.assertEqual(0, len(self.rules.getEncodings(0)))

        # Testing individually adding and removing an encoding.
        self.rules.addEncoding(0, TEST_NAMES[0])
        self.assertEqual(1, len(self.rules.getEncodings(0)))
        self.rules.addEncoding(0, TEST_NAMES[1])
        self.assertEqual(2, len(self.rules.getEncodings(0)))
        self.rules.removeEncoding(0, 1)
        self.assertEqual(1, len(self.rules.getEncodings(0)))

        # Index has to be valid.
        with self.assertRaises(OCIO.Exception):
            self.rules.removeEncoding(0, 1)

        # Can't add color space if there are encodings.
        with self.assertRaises(OCIO.Exception):
            self.rules.addColorSpace(0, "NoMix")

    def test_custom_keys(self):
        """
        Test the getNumCustomKeys(), getCustomKeyName(), getCustomKeyValue() and setCustomKey() methods.
        """
        # Add new rule.
        rule = "rule"
        self.rules.insertRule(0, rule)

        # Set keys and values and verify they are set.
        self.assertEqual(0, self.rules.getNumCustomKeys(0))
        self.rules.setCustomKey(0, "key1", "value1")
        self.rules.setCustomKey(0, "key2", "value2")
        self.assertEqual(2, self.rules.getNumCustomKeys(0))
        self.assertEqual("key1", self.rules.getCustomKeyName(0, 0))
        self.assertEqual("value1", self.rules.getCustomKeyValue(0, 0))
        self.assertEqual("key2", self.rules.getCustomKeyName(0, 1))
        self.assertEqual("value2", self.rules.getCustomKeyValue(0, 1))
        # Replace a value.
        self.rules.setCustomKey(0, "key2", "value3")
        self.assertEqual(2, self.rules.getNumCustomKeys(0))
        self.assertEqual("value3", self.rules.getCustomKeyValue(0, 1))
        # Remove a key.
        self.rules.setCustomKey(0, "key1", "")
        self.assertEqual(1, self.rules.getNumCustomKeys(0))
        self.assertEqual("key2", self.rules.getCustomKeyName(0, 0))

        # Index has to be valid.
        with self.assertRaises(OCIO.Exception):
            self.rules.setCustomKey(42, "wrong", "")
        # Key name has to be non-empty.
        with self.assertRaises(OCIO.Exception):
            self.rules.setCustomKey(0, "", "")
        # Index has to be valid.
        with self.assertRaises(OCIO.Exception):
            self.rules.getCustomKeyName(0, 42)

    def test_config(self):
        """
        Test the viewing rules object from an OCIO config.
        """

        # Get simple config file from UnitTestUtils.py
        cfg = OCIO.Config().CreateFromStream(SIMPLE_CONFIG)

        # Test ViewingRules class object getters from config
        rules = cfg.getViewingRules()
        self.assertEqual(4, rules.getNumEntries())

        iterator = rules.getColorSpaces(0)
        self.assertEqual(1, len(iterator))
        self.assertEqual("c1", next(iterator))
        self.assertEqual(2, rules.getNumCustomKeys(0))
        self.assertEqual("key0", rules.getCustomKeyName(0, 0))
        self.assertEqual("value0", rules.getCustomKeyValue(0, 0))
        self.assertEqual("key1", rules.getCustomKeyName(0, 1))
        self.assertEqual("value1", rules.getCustomKeyValue(0, 1))
        iterator = rules.getColorSpaces(1)
        self.assertEqual(2, len(iterator))
        self.assertEqual("c2", next(iterator))
        self.assertEqual("c3", next(iterator))
        self.assertEqual(0, rules.getNumCustomKeys(1))
        iterator = rules.getEncodings(2)
        self.assertEqual(1, len(iterator))
        self.assertEqual("log", next(iterator))
        self.assertEqual(0, rules.getNumCustomKeys(2))
        iterator = rules.getEncodings(3)
        self.assertEqual(2, len(iterator))
        self.assertEqual("log", next(iterator))
        self.assertEqual("scene-linear", next(iterator))
        self.assertEqual(2, rules.getNumCustomKeys(3))
        self.assertEqual("key1", rules.getCustomKeyName(3, 0))
        self.assertEqual("value2", rules.getCustomKeyValue(3, 0))
        self.assertEqual("key2", rules.getCustomKeyName(3, 1))
        self.assertEqual("value0", rules.getCustomKeyValue(3, 1))

        # Add a new rule.
        rules.insertRule(0, 'newRule')
        rules.addEncoding(0, 'log')
        rules.addEncoding(0, 'scene-linear')

        # Set the modified rules, get them back and verify there are now 5 rules.
        cfg.setViewingRules(rules)
        rules = cfg.getViewingRules()
        self.assertEqual(5, rules.getNumEntries())

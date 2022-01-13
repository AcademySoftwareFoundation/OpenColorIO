# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import copy, unittest, os, sys
import PyOpenColorIO as OCIO

class FileRulesTest(unittest.TestCase):

    def test_copy(self):
        """
        Test the deepcopy() method.
        """
        rules = OCIO.FileRules()
        rules.insertRule(0, 'test0', 'colorspace', 'pattern', 'ext')
        rules.setCustomKey(0, 'key1', 'val1')
        rules.insertRule(1, 'test2', 'colorspace', 'regex')
        rules.setCustomKey(1, 'key2', 'val2')

        other = copy.deepcopy(rules)
        self.assertFalse(other is rules)

        self.assertEqual(other.getNumEntries(), rules.getNumEntries())
        for idx in range(other.getNumEntries()):
            self.assertEqual(other.getName(idx), rules.getName(idx))
            self.assertEqual(other.getPattern(idx), rules.getPattern(idx))
            self.assertEqual(other.getExtension(idx), rules.getExtension(idx))
            self.assertEqual(other.getRegex(idx), rules.getRegex(idx))
            self.assertEqual(other.getColorSpace(idx), rules.getColorSpace(idx))
            self.assertEqual(other.getNumCustomKeys(idx), rules.getNumCustomKeys(idx))
            for idx_inner in range(other.getNumCustomKeys(idx)):
                self.assertEqual(other.getCustomKeyName(idx, idx_inner), rules.getCustomKeyName(idx, idx_inner))
                self.assertEqual(other.getCustomKeyValue(idx, idx_inner), rules.getCustomKeyValue(idx, idx_inner))

    def test_default(self):
        """
        Construct and verify default values.
        """
        rules = OCIO.FileRules()
        self.assertTrue(rules.isDefault())
        self.assertEqual(rules.getNumEntries(), 1)
        self.assertEqual(rules.getName(0), OCIO.DEFAULT_RULE_NAME)
        self.assertEqual(rules.getPattern(0), '')
        self.assertEqual(rules.getExtension(0), '')
        self.assertEqual(rules.getRegex(0), '')
        self.assertEqual(rules.getColorSpace(0), 'default')

    def test_name(self):
        """
        Test getName().
        """
        rules = OCIO.FileRules()
        rules.insertRule(0, 'test', 'colorspace', 'regex')
        self.assertEqual(rules.getName(0), 'test')

        with self.assertRaises(OCIO.Exception):
            rules.getName(4)

        with self.assertRaises(TypeError):
            rules.getName('test')

    def test_pattern(self):
        """
        Test get/setPattern().
        """
        rules = OCIO.FileRules()
        # Inserting a rule at position 0 is moving Default rule at position 1.
        rules.insertRule(0, 'test', 'colorspace', 'pattern', 'ext')
        self.assertEqual(rules.getPattern(0), 'pattern')
        rules.setPattern(0, 'new pattern')
        self.assertEqual(rules.getPattern(0), 'new pattern')

        # Setting regex clears pattern.
        rules.setRegex(0, 'regex')
        self.assertEqual(rules.getPattern(0), '')

        with self.assertRaises(OCIO.Exception):
            rules.setPattern(4, 'invalid index')

        # Default rule only has a color space parameter.
        with self.assertRaises(OCIO.Exception):
            rules.setPattern(1, 'default does not use pattern')

        with self.assertRaises(OCIO.Exception):
            rules.getPattern(4)

        # Wrong strings.
        for invalid in (None, '', '[]', '[a-b'):
            with self.assertRaises(OCIO.Exception):
                rules.setPattern(0, invalid)

        # Wrong types.
        for invalid in (OCIO.TRANSFORM_DIR_INVERSE, 1):
            with self.assertRaises(TypeError):
                rules.setPattern(0, invalid)

    def test_extension(self):
        """
        Test get/setExtension().
        """
        rules = OCIO.FileRules()
        rules.insertRule(0, 'test', 'colorspace', 'pattern', 'ext')
        self.assertEqual(rules.getExtension(0), 'ext')
        rules.setExtension(0, 'ocio')
        self.assertEqual(rules.getExtension(0), 'ocio')

        # Setting regex clears extension.
        rules.setRegex(0, 'regex')
        self.assertEqual(rules.getExtension(0), '')

        with self.assertRaises(OCIO.Exception):
            rules.setExtension(4, 'invalid index')

        with self.assertRaises(OCIO.Exception):
            rules.setExtension(1, 'default does not use extension')

        with self.assertRaises(OCIO.Exception):
            rules.getExtension(4)

        # Wrong strings.
        for invalid in (None, '', '[]', 'jp[gG'):
            with self.assertRaises(OCIO.Exception):
                rules.setExtension(0, invalid)

        # Wrong types.
        for invalid in (OCIO.TRANSFORM_DIR_INVERSE, 1):
            with self.assertRaises(TypeError):
                rules.setExtension(0, invalid)

    def test_regex(self):
        """
        Test get/setRegex().
        """
        rules = OCIO.FileRules()
        rules.insertRule(0, 'test', 'colorspace', 'regex')
        self.assertEqual(rules.getRegex(0), 'regex')
        rules.setRegex(0, 'newregex')
        self.assertEqual(rules.getRegex(0), 'newregex')

        # Setting pattern clears regex.
        rules.setPattern(0, 'pattern')
        self.assertEqual(rules.getRegex(0), '')

        rules.setRegex(0, 'newregex')

        # Setting extension clears regex.
        rules.setExtension(0, 'ext')
        self.assertEqual(rules.getRegex(0), '')

        with self.assertRaises(OCIO.Exception):
            rules.setRegex(4, 'invalid index')

        with self.assertRaises(OCIO.Exception):
            rules.setRegex(1, 'default does not use regex')

        with self.assertRaises(OCIO.Exception):
            rules.getRegex(4)

        # Wrong strings.
        for invalid in (None, '', '(.*)(\bwhat'):
            with self.assertRaises(OCIO.Exception):
                rules.setRegex(0, invalid)

        # Wrong types.
        for invalid in (OCIO.TRANSFORM_DIR_INVERSE, 1):
            with self.assertRaises(TypeError):
                rules.setRegex(0, invalid)

    def test_color_space(self):
        """
        Test get/setColorSpace().
        """
        rules = OCIO.FileRules()
        rules.insertRule(0, 'test', 'colorspace', 'regex')
        self.assertEqual(rules.getColorSpace(0), 'colorspace')
        rules.setColorSpace(0, 'other')
        self.assertEqual(rules.getColorSpace(0), 'other')

        # Color space of default rule can be changed.
        rules.setColorSpace(1, 'raw')
        self.assertEqual(rules.getColorSpace(1), 'raw')

        with self.assertRaises(OCIO.Exception):
            rules.setColorSpace(4, 'invalid index')

        with self.assertRaises(OCIO.Exception):
            rules.getColorSpace(4)

        # Wrong strings.
        for invalid in (None, ''):
            with self.assertRaises(OCIO.Exception):
                rules.setColorSpace(0, invalid)

        # Wrong types.
        for invalid in (OCIO.TRANSFORM_DIR_INVERSE, 1):
            with self.assertRaises(TypeError):
                rules.setColorSpace(0, invalid)

    def test_custom_keys(self):
        """
        Test getNumCustomKeys(), getCustomKeyName(), getCustomKeyValue() and setCustomKey().
        """
        rules = OCIO.FileRules()
        rules.insertRule(0, 'test', 'colorspace', 'regex')

        self.assertEqual(rules.getNumCustomKeys(0), 0)

        rules.setCustomKey(0, 'key1', 'val1')
        self.assertEqual(rules.getNumCustomKeys(0), 1)
        self.assertEqual(rules.getCustomKeyName(0, 0), 'key1')
        self.assertEqual(rules.getCustomKeyValue(0, 0), 'val1')

        rules.setCustomKey(0, 'key1', 'new val1')
        self.assertEqual(rules.getNumCustomKeys(0), 1)
        self.assertEqual(rules.getCustomKeyValue(0, 0), 'new val1')

        rules.setCustomKey(0, 'key2', 'val2')
        self.assertEqual(rules.getNumCustomKeys(0), 2)
        self.assertEqual(rules.getCustomKeyName(0, 1), 'key2')
        self.assertEqual(rules.getCustomKeyValue(0, 1), 'val2')

        with self.assertRaises(OCIO.Exception):
            self.assertEqual(rules.getCustomKeyName(0, 2), '')
        with self.assertRaises(OCIO.Exception):
            self.assertEqual(rules.getCustomKeyValue(0, 2), '')

        with self.assertRaises(OCIO.Exception):
            self.assertEqual(rules.getCustomKeyName(4, 0), '')
        with self.assertRaises(OCIO.Exception):
            self.assertEqual(rules.getCustomKeyValue(4, 0), '')

    def test_insert_rule(self):
        """
        Test insertRule() & removeRule().
        """

        # Rules can be inserted with pattern & extension or with regex.

        rules = OCIO.FileRules()
        rules.insertRule(0, 'test', 'colorspace', 'regex')
        self.assertEqual(rules.getNumEntries(), 2)

        rules.insertRule(name='first', colorSpace='colorspace', regex='regex2', ruleIndex=0)
        self.assertEqual(rules.getNumEntries(), 3)

        self.assertEqual(rules.getName(0), 'first')
        self.assertEqual(rules.getName(1), 'test')
        self.assertEqual(rules.getName(2), OCIO.DEFAULT_RULE_NAME)

        rules.insertRule(name='second', ruleIndex=1, colorSpace='colorspace', regex='regex')
        self.assertEqual(rules.getNumEntries(), 4)

        self.assertEqual(rules.getName(0), 'first')
        self.assertEqual(rules.getName(1), 'second')
        self.assertEqual(rules.getName(2), 'test')
        self.assertEqual(rules.getName(3), OCIO.DEFAULT_RULE_NAME)

        # Default rule is always the last rule.
        with self.assertRaises(OCIO.Exception):
            rules.insertRule(ruleIndex=4, name='not after default',
                             colorSpace='colorspace2', regex='regex')

        # Rule names have to be unique.
        with self.assertRaises(OCIO.Exception):
            rules.insertRule(name='first', ruleIndex=2, colorSpace='colorspace2', regex='regex')

        # 'Default' is a reserved name.
        with self.assertRaises(OCIO.Exception):
            rules.insertRule(name=OCIO.DEFAULT_RULE_NAME, ruleIndex=1, colorSpace='colorspace',
                             regex='regex')

        # Default rule can't be removed.
        with self.assertRaises(OCIO.Exception):
            rules.removeRule(3)

        rules.removeRule(0)
        rules.removeRule(0)
        rules.removeRule(0)
        self.assertEqual(rules.getNumEntries(), 1)
        self.assertEqual(rules.getName(0), OCIO.DEFAULT_RULE_NAME)

        # File path search rule needs empty parameters.

        rules.insertRule(name=OCIO.FILE_PATH_SEARCH_RULE_NAME, ruleIndex=0, colorSpace='',
                         regex='')
        self.assertEqual(rules.getNumEntries(), 2)
        rules.removeRule(0)
        rules.insertRule(name=OCIO.FILE_PATH_SEARCH_RULE_NAME, ruleIndex=0, colorSpace='',
                         pattern='', extension='')
        self.assertEqual(rules.getNumEntries(), 2)
        rules.removeRule(0)

        # Parameters have to be empty: colorspace is not empty.
        with self.assertRaises(OCIO.Exception):
            rules.insertRule(name=OCIO.FILE_PATH_SEARCH_RULE_NAME, colorSpace='not empty', regex='',
                             ruleIndex=0)

        # Parameters have to be empty: regex is not empty.
        with self.assertRaises(OCIO.Exception):
            rules.insertRule(name=OCIO.FILE_PATH_SEARCH_RULE_NAME, colorSpace='', regex='not empty',
                             ruleIndex=0)

        # Parameters have to be empty: pattern is not empty.
        with self.assertRaises(OCIO.Exception):
            rules.insertRule(name=OCIO.FILE_PATH_SEARCH_RULE_NAME, colorSpace='', pattern='not empty',
                             extension='', ruleIndex=0)

        # Parameters have to be empty: extension is not empty.
        with self.assertRaises(OCIO.Exception):
            rules.insertRule(name=OCIO.FILE_PATH_SEARCH_RULE_NAME, colorSpace='', pattern='',
                             extension='not empty', ruleIndex=0)

        # There is a special function for file path search rule.
        rules.insertPathSearchRule(0)
        self.assertEqual(rules.getNumEntries(), 2)
        self.assertEqual(rules.getName(0), OCIO.FILE_PATH_SEARCH_RULE_NAME)
        self.assertEqual(rules.getName(1), OCIO.DEFAULT_RULE_NAME)

        rules.insertRule(name='first', colorSpace='colorspace',
                         pattern='pattern', extension='ext', ruleIndex=0)

        rules.insertRule(name='second', colorSpace='colorspace',
                         pattern='pattern2', extension='jpg', ruleIndex=1)

        self.assertEqual(rules.getNumEntries(), 4)

    def test_rule_priority(self):
        """
        Test increaseRulePriority() & decreaseRulePriority().
        """
        rules = OCIO.FileRules()
        rules.insertRule(0, 'A', 'colorspace', 'regex')
        rules.insertRule(1, 'B', 'colorspace', 'regex')
        rules.insertRule(2, 'C', 'colorspace', 'regex')
        self.assertEqual(rules.getName(0), 'A')
        self.assertEqual(rules.getName(1), 'B')
        self.assertEqual(rules.getName(2), 'C')

        rules.increaseRulePriority(1)
        self.assertEqual(rules.getName(0), 'B')
        self.assertEqual(rules.getName(1), 'A')
        self.assertEqual(rules.getName(2), 'C')

        rules.decreaseRulePriority(1)
        self.assertEqual(rules.getName(0), 'B')
        self.assertEqual(rules.getName(1), 'C')
        self.assertEqual(rules.getName(2), 'A')

        rules.decreaseRulePriority(0)
        self.assertEqual(rules.getName(0), 'C')
        self.assertEqual(rules.getName(1), 'B')
        self.assertEqual(rules.getName(2), 'A')

    def test_using_rules(self):
        """
        Test Config.setFileRiles() & Config.getColorSpaceFromFilepath().
        """
        cfg = OCIO.Config.CreateRaw()
        cs = OCIO.ColorSpace(name = 'cs1')
        cfg.addColorSpace(cs)
        cs = OCIO.ColorSpace(name = 'cs2')
        cfg.addColorSpace(cs)
        cs = OCIO.ColorSpace(name = 'cs3')
        cfg.addColorSpace(cs)

        rules = OCIO.FileRules()
        rules.insertRule(0, 'A', 'cs1', '*', 'jpg')
        rules.insertRule(1, 'B', 'cs2', '*', 'png')
        rules.insertRule(2, 'C', 'cs3', '*', 'exr')

        cfg.setFileRules(rules)
        cfg_rules = cfg.getFileRules()
        self.assertEqual(cfg_rules.getNumEntries(), 4)

        csName, ruleIndex = cfg.getColorSpaceFromFilepath(filePath='test.png')
        self.assertEqual(csName, 'cs2')
        self.assertEqual(ruleIndex, 1)

        csName, ruleIndex = cfg.getColorSpaceFromFilepath(filePath='pic.exr')
        self.assertEqual(csName, 'cs3')
        self.assertEqual(ruleIndex, 2)

        csName, ruleIndex = cfg.getColorSpaceFromFilepath(filePath='pic.txt')
        self.assertEqual(csName, 'default')
        self.assertEqual(ruleIndex, 3)

        rules.removeRule(0)
        rules.removeRule(0)
        rules.removeRule(0)

        rules.insertRule(0, 'exr files', 'cs1', '*', '[eE][xX][r]')
        cfg.setFileRules(rules)

        csName, ruleIndex = cfg.getColorSpaceFromFilepath('/An/Arbitrary/Path/MyFile.exr')
        self.assertEqual(ruleIndex, 0)
        csName, ruleIndex = cfg.getColorSpaceFromFilepath('/An/Arbitrary/Path/MyFile.eXr')
        self.assertEqual(ruleIndex, 0)
        csName, ruleIndex = cfg.getColorSpaceFromFilepath('/An/Arbitrary/Path/MyFile.EXR')
        self.assertEqual(ruleIndex, 1) # Default rule. R must be lower case.
        csName, ruleIndex = cfg.getColorSpaceFromFilepath('/An/Arbitrary/Path/MyFileexr')
        self.assertEqual(ruleIndex, 1) # Default rule.
        csName, ruleIndex = cfg.getColorSpaceFromFilepath('/An/Arbitrary/Path/MyFile.jpeg')
        self.assertEqual(ruleIndex, 1) # Default rule.
        csName, ruleIndex = cfg.getColorSpaceFromFilepath('/An/Arbitrary.exr/Path/MyFileexr')
        self.assertEqual(ruleIndex, 1) # Default rule.
        csName, ruleIndex = cfg.getColorSpaceFromFilepath('')
        self.assertEqual(ruleIndex, 1) # Default rule.

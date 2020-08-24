# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest
import os
import sys

import PyOpenColorIO as OCIO
from UnitTestUtils import TEST_NAMES, TEST_DESCS


class LookTest(unittest.TestCase):
    TEST_PROCESS_SPACES = ['raw', 'lnh', 'vd8', 'a.b.c.', '1-2-3-']
    TEST_EXP_VALUES = [0.1, 0.2, 0.3, 0.4]

    def setUp(self):
        self.look = OCIO.Look()

    def tearDown(self):
        self.look = None

    def test_name(self):
        """
        Test the setName() and getName() methods.
        """

        # Default initialized name value is ""
        self.assertEqual(self.look.getName(), '')

        for name in TEST_NAMES:
            self.look.setName(name)
            self.assertEqual(name, self.look.getName())

        # Wrong type tests.
        for invalid in (None, 1):
            with self.assertRaises(TypeError):
                self.look.setName(invalid)

    def test_process_space(self):
        """
        Test the setProcessSpace() and getProcessName() methods.
        """

        # Default initialized process space value is ""
        self.assertEqual(self.look.getProcessSpace(), '')

        for process_space in self.TEST_PROCESS_SPACES:
            self.look.setProcessSpace(process_space)
            self.assertEqual(process_space, self.look.getProcessSpace())

        # Wrong type tests.
        for invalid in (None, 1):
            with self.assertRaises(TypeError):
                self.look.setProcessSpace(invalid)

    def test_description(self):
        """
        Test the setDescription() and getDescription() methods.
        """

        # Default initialized description value is ""
        self.assertEqual(self.look.getDescription(), '')

        for desc in TEST_DESCS:
            self.look.setDescription(desc)
            self.assertEqual(desc, self.look.getDescription())

        # Wrong type tests.
        for invalid in (None, 1):
            with self.assertRaises(TypeError):
                self.look.setDescription(invalid)

    def test_transform(self):
        """
        Test the setTransform() and getTransform() methods.
        """

        # Default initialized transform value is None
        self.assertIsNone(self.look.getTransform())

        exp_tr = OCIO.ExponentTransform()
        exp_tr.setValue(self.TEST_EXP_VALUES)
        self.look.setTransform(exp_tr)
        out_exp_tr = self.look.getTransform()
        self.assertListEqual(out_exp_tr.getValue(), self.TEST_EXP_VALUES)

        # Wrong type tests.
        for invalid in (OCIO.ALLOCATION_UNIFORM, 1):
            with self.assertRaises(TypeError):
                self.look.setTransform(invalid)

    def test_inverse_transform(self):
        """
        Test the setInverseTransform() and getInverseTransform() methods.
        """

        # Default initialized inverse transform value is None
        self.assertIsNone(self.look.getInverseTransform())

        exp_tr = OCIO.ExponentTransform()
        inv_exp_values = [1.0 / v for v in self.TEST_EXP_VALUES]
        exp_tr.setValue(inv_exp_values)
        self.look.setInverseTransform(exp_tr)
        inv_oet = self.look.getInverseTransform()
        self.assertListEqual(inv_oet.getValue(), inv_exp_values)

        # Wrong type tests.
        for invalid in (OCIO.ALLOCATION_UNIFORM, 1):
            with self.assertRaises(TypeError):
                self.look.setInverseTransform(invalid)

    def test_constructor_with_keyword(self):
        """
        Test Look constructor with keywords and validate its values.
        """

        # With keywords in their proper order.
        exp_tr = OCIO.ExponentTransform()
        inv_exp_tr = OCIO.ExponentTransform()
        look = OCIO.Look(name='coollook',
                         processSpace='somespace',
                         transform=exp_tr,
                         inverseTransform=inv_exp_tr,
                         description='this is a test')

        self.assertEqual(look.getName(), 'coollook')
        self.assertEqual(look.getProcessSpace(), 'somespace')
        self.assertIsInstance(look.getTransform(), type(exp_tr))
        self.assertIsInstance(look.getInverseTransform(), type(inv_exp_tr))
        self.assertEqual(look.getDescription(), 'this is a test')

        # With keyword not in their proper order.
        exp_tr2 = OCIO.ExponentTransform()
        inv_exp_tr2 = OCIO.ExponentTransform()
        look2 = OCIO.Look(inverseTransform=inv_exp_tr,
                          description='this is a test',
                          name='coollook',
                          processSpace='somespace',
                          transform=exp_tr)

        self.assertEqual(look2.getName(), 'coollook')
        self.assertEqual(look2.getProcessSpace(), 'somespace')
        self.assertIsInstance(look2.getTransform(), type(exp_tr2))
        self.assertIsInstance(look2.getInverseTransform(), type(inv_exp_tr2))
        self.assertEqual(look2.getDescription(), 'this is a test')

    def test_constructor_with_positional(self):
        """
        Test Look constructor without keywords and validate its values.
        """

        exp_tr = OCIO.ExponentTransform()
        inv_exp_tr = OCIO.ExponentTransform()
        look = OCIO.Look('coollook',
                         'somespace',
                         exp_tr,
                         inv_exp_tr,
                         'this is a test')

        self.assertEqual(look.getName(), 'coollook')
        self.assertEqual(look.getProcessSpace(), 'somespace')
        self.assertIsInstance(look.getTransform(), type(exp_tr))
        self.assertIsInstance(look.getInverseTransform(), type(inv_exp_tr))
        self.assertEqual(look.getDescription(), 'this is a test')

    def test_constructor_wrong_parameter_type(self):
        """
        Test Look constructor with a wrong parameter type.
        """

        for invalid in (None, 1):
            with self.assertRaises(TypeError):
                look = OCIO.Look(invalid)

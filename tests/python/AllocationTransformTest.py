# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest

import PyOpenColorIO as OCIO
from TransformsBaseTest import TransformsBaseTest


class AllocationTransformTest(unittest.TestCase, TransformsBaseTest):
    TEST_ALLOCATION = OCIO.ALLOCATION_LG2
    TEST_VARS = [0, 1]
    TEST_DIRECTION = OCIO.TRANSFORM_DIR_INVERSE

    def setUp(self):
        self.tr = OCIO.AllocationTransform()

    def test_transform_type(self):
        """
        Test the getTransformType() method.
        """
        self.assertEqual(self.tr.getTransformType(),
                         OCIO.TRANSFORM_TYPE_ALLOCATION)

    def test_allocation(self):
        """
        Test the setAllocation() and getAllocation() methods.
        """

        # Default initialized allocation value is OCIO.ALLOCATION_UNIFORM
        self.assertEqual(self.tr.getAllocation(), OCIO.ALLOCATION_UNIFORM)

        for allocation in OCIO.Allocation.__members__.values():
            self.tr.setAllocation(allocation)
            self.assertEqual(self.tr.getAllocation(), allocation)

        # Wrong type tests.
        for invalid in (None, 1):
            with self.assertRaises(TypeError):
                self.tr.setAllocation(invalid)

    def test_vars(self):
        """
        Test the setVars() and getVars() methods.
        """

        # Default initialized vars value is []
        self.assertEqual(self.tr.getVars(), [])

        vars_array = []
        for i in range(1, 5):
            # Appending 0.1... 0.4 to vars_array
            vars_array.append(i/10)

            # setVars() only take in array in length of 2 or 3.
            if len(vars_array) < 2 or len(vars_array) > 3:
                with self.assertRaises(OCIO.Exception):
                    self.tr.setVars(vars_array)
            else:
                self.tr.setVars(vars_array)
                for i, var in enumerate(self.tr.getVars()):
                    self.assertAlmostEqual(vars_array[i], var, places=7)

        # Wrong type tests.
        for invalid in (None, 'hello'):
            with self.assertRaises(TypeError):
                self.tr.setVars(invalid)

    def test_constructor_with_keyword(self):
        """
        Test AllocationTransform constructor with keywords and validate its values.
        """

        # With keywords in their proper order.
        allo_tr = OCIO.AllocationTransform(allocation=self.TEST_ALLOCATION,
                                           vars=self.TEST_VARS,
                                           direction=self.TEST_DIRECTION)

        self.assertEqual(allo_tr.getAllocation(), self.TEST_ALLOCATION)
        for i, var in enumerate(allo_tr.getVars()):
            self.assertAlmostEqual(self.TEST_VARS[i], var, places=7)
        self.assertEqual(allo_tr.getDirection(), self.TEST_DIRECTION)

        # With keywords not in their proper order.
        allo_tr2 = OCIO.AllocationTransform(direction=self.TEST_DIRECTION,
                                            allocation=self.TEST_ALLOCATION,
                                            vars=self.TEST_VARS)

        self.assertEqual(allo_tr2.getAllocation(), self.TEST_ALLOCATION)
        for i, var in enumerate(allo_tr2.getVars()):
            self.assertAlmostEqual(self.TEST_VARS[i], var, places=7)
        self.assertEqual(allo_tr2.getDirection(), self.TEST_DIRECTION)

    def test_constructor_with_positional(self):
        """
        Test AllocationTransform constructor without keywords and validate its values.
        """

        allo_tr = OCIO.AllocationTransform(self.TEST_ALLOCATION,
                                           self.TEST_VARS,
                                           self.TEST_DIRECTION)

        self.assertEqual(allo_tr.getAllocation(), self.TEST_ALLOCATION)
        for i, var in enumerate(allo_tr.getVars()):
            self.assertAlmostEqual(self.TEST_VARS[i], var, places=7)
        self.assertEqual(allo_tr.getDirection(), self.TEST_DIRECTION)

    def test_constructor_wrong_parameter_type(self):
        """
        Test AllocationTransform constructor with a wrong parameter type.
        """

        for invalid in (None, 1):
            with self.assertRaises(TypeError):
                allo_tr = OCIO.AllocationTransform(invalid)

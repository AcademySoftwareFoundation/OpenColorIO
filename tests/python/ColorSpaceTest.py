# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest
import os
import sys

import PyOpenColorIO as OCIO
from UnitTestUtils import SIMPLE_CONFIG, TEST_NAMES, TEST_DESCS, TEST_CATEGORIES


class ColorSpaceTest(unittest.TestCase):
    def setUp(self):
        self.colorspace = OCIO.ColorSpace()
        self.log_tr = OCIO.LogTransform(10)

    def tearDown(self):
        self.colorspace = None
        self.log_tr = None

    def test_allocation(self):
        """
        Test the setAllocation() and getAllocation() methods.
        """

        # Known constants tests
        for i, allocation in enumerate(OCIO.Allocation.__members__.values()):
            self.colorspace.setAllocation(allocation)
            self.assertEqual(allocation, self.colorspace.getAllocation())

        # Wrong type tests (using TransformDirection instead.)
        for direction in OCIO.TransformDirection.__members__.values():
            with self.assertRaises(TypeError):
                self.colorspace.setAllocation(direction)

        # Wrong type tests (set to None.)
        with self.assertRaises(TypeError):
            self.colorspace.setAllocation(None)

    def test_allocation_vars(self):
        """
        Test the setAllocationVars() and getAllocationVars() methods.
        """

        # Array length tests
        alloc_vars = []
        for i in range(1, 5):
            # This will create [0.1] up to [0.1, 0.2, 0.3, 0.4]
            alloc_vars.append(0.1 * i)
            if i < 2 or i > 3:
                with self.assertRaises(OCIO.Exception):
                    self.colorspace.setAllocationVars(alloc_vars)
            else:
                self.colorspace.setAllocationVars(alloc_vars)
                self.assertEqual(len(alloc_vars), len(
                    self.colorspace.getAllocationVars()))

        # Wrong type tests
        wrong_alloc_vars = [['test'], 'test', 0.1, 1]
        for wrong_alloc_var in wrong_alloc_vars:
            with self.assertRaises(TypeError):
                self.colorspace.setAllocationVars(wrong_alloc_var)

    def test_bitdepth(self):
        """
        Test the setBitDepth() and getBitDepth() methods.
        """

        # Known constants tests
        for i, bit_depth in enumerate(OCIO.BitDepth.__members__.values()):
            self.colorspace.setBitDepth(bit_depth)
            self.assertEqual(bit_depth, self.colorspace.getBitDepth())

        # Wrong type tests (using TransformDirection instead.)
        for direction in OCIO.TransformDirection.__members__.values():
            with self.assertRaises(TypeError):
                self.colorspace.setBitDepth(direction)

        # Wrong type tests (set to None.)
        with self.assertRaises(TypeError):
            self.colorspace.setBitDepth(None)

    def test_category(self):
        """
        Test the hasCategory(), addCategory(), removeCategory(),
        getCategories() and clearCategories() methods.
        """

        # Test empty categories
        self.assertFalse(self.colorspace.hasCategory('ocio'))
        self.assertEqual(len(self.colorspace.getCategories()), 0)
        with self.assertRaises(IndexError):
            self.colorspace.getCategories()[0]

        # Test with defined TEST_CATEGORIES.
        for i, y in enumerate(TEST_CATEGORIES):
            self.assertEqual(len(self.colorspace.getCategories()), i)
            self.colorspace.addCategory(y)
            self.assertTrue(self.colorspace.hasCategory(y))

        # Test the output list is equal to TEST_CATEGORIES.
        self.assertListEqual(
            list(self.colorspace.getCategories()), TEST_CATEGORIES)

        # Test the length of list is equal to the length of TEST_CATEGORIES.
        self.assertEqual(len(self.colorspace.getCategories()),
                         len(TEST_CATEGORIES))

        iterator = self.colorspace.getCategories()
        for a in TEST_CATEGORIES:
            self.assertEqual(a, next(iterator))

        # Test the length of categories is zero after clearCategories()
        self.colorspace.clearCategories()
        self.assertEqual(len(self.colorspace.getCategories()), 0)

        # Testing individually adding and removing a category.
        self.colorspace.addCategory(TEST_CATEGORIES[0])
        self.assertEqual(len(self.colorspace.getCategories()), 1)
        self.colorspace.removeCategory(TEST_CATEGORIES[0])
        self.assertEqual(len(self.colorspace.getCategories()), 0)

    def test_config(self):
        """
        Test the ColorSpace object from an OCIO config.
        """

        # Get simple config file from Constants.py
        cfg = OCIO.Config().CreateFromStream(SIMPLE_CONFIG)

        # Test ColorSpace class object getters from config
        cs = cfg.getColorSpace('vd8')
        self.assertEqual(cs.getName(), 'vd8')
        self.assertEqual(cs.getDescription(), 'how many transforms can we use?\n')
        self.assertEqual(cs.getFamily(), 'vd8')
        self.assertEqual(cs.getAllocation(), OCIO.ALLOCATION_UNIFORM)
        self.assertEqual(cs.getAllocationVars(), [])
        self.assertEqual(cs.getEqualityGroup(), '')
        self.assertEqual(cs.getBitDepth(), OCIO.BIT_DEPTH_UINT8)
        self.assertFalse(cs.isData())

        to_ref = cs.getTransform(OCIO.COLORSPACE_DIR_TO_REFERENCE)
        self.assertIsInstance(to_ref, OCIO.GroupTransform)
        self.assertEqual(len(to_ref), 3)

    def test_constructor_with_keyword(self):
        """
        Test ColorSpace constructor with keywords and validate its values.
        """

        # With keywords in their proper order.
        cs = OCIO.ColorSpace(name='test',
                             family='ocio family',
                             equalityGroup='My_Equality',
                             description='This is a test colourspace!',
                             bitDepth=OCIO.BIT_DEPTH_F32,
                             isData=False,
                             allocation=OCIO.ALLOCATION_LG2,
                             allocationVars=[0.0, 1.0])

        self.assertEqual(cs.getName(), 'test')
        self.assertEqual(cs.getFamily(), 'ocio family')
        self.assertEqual(cs.getEqualityGroup(), 'My_Equality')
        self.assertEqual(cs.getDescription(), 'This is a test colourspace!')
        self.assertEqual(cs.getBitDepth(), OCIO.BIT_DEPTH_F32)
        self.assertFalse(cs.isData())
        self.assertEqual(cs.getAllocation(), OCIO.ALLOCATION_LG2)
        self.assertEqual(cs.getAllocationVars(), [0.0, 1.0])

        # With keyword not in their proper order.
        cs2 = OCIO.ColorSpace(family='ocio family',
                              name='test',
                              isData=False,
                              allocationVars=[0.0, 1.0],
                              allocation=OCIO.ALLOCATION_LG2,
                              description='This is a test colourspace!',
                              equalityGroup='My_Equality',
                              bitDepth=OCIO.BIT_DEPTH_F32,
                              )

        self.assertEqual(cs2.getName(), 'test')
        self.assertEqual(cs2.getFamily(), 'ocio family')
        self.assertEqual(cs2.getEqualityGroup(), 'My_Equality')
        self.assertEqual(cs2.getDescription(), 'This is a test colourspace!')
        self.assertEqual(cs2.getBitDepth(), OCIO.BIT_DEPTH_F32)
        self.assertFalse(cs2.isData())
        self.assertEqual(cs2.getAllocation(), OCIO.ALLOCATION_LG2)
        self.assertEqual(cs2.getAllocationVars(), [0.0, 1.0])

    def test_constructor_without_keyword(self):
        """
        Test ColorSpace constructor without keywords and validate its values.
        """

        cs = OCIO.ColorSpace(OCIO.REFERENCE_SPACE_SCENE,
                             'test',
                             'ocio family',
                             'My_Equality',
                             'This is a test colourspace!',
                             OCIO.BIT_DEPTH_F32,
                             False,
                             OCIO.ALLOCATION_LG2,
                             [0.0, 1.0])

        self.assertEqual(cs.getName(), 'test')
        self.assertEqual(cs.getFamily(), 'ocio family')
        self.assertEqual(cs.getEqualityGroup(), 'My_Equality')
        self.assertEqual(cs.getDescription(), 'This is a test colourspace!')
        self.assertEqual(cs.getBitDepth(), OCIO.BIT_DEPTH_F32)
        self.assertFalse(cs.isData())
        self.assertEqual(cs.getAllocation(), OCIO.ALLOCATION_LG2)
        self.assertEqual(cs.getAllocationVars(), [0.0, 1.0])

    def test_constructor_without_parameter(self):
        """
        Test ColorSpace default constructor and validate its values.
        """

        cs = OCIO.ColorSpace()

        self.assertEqual(cs.getName(), '')
        self.assertEqual(cs.getFamily(), '')
        self.assertEqual(cs.getEqualityGroup(), '')
        self.assertEqual(cs.getDescription(), '')
        self.assertEqual(cs.getBitDepth(), OCIO.BIT_DEPTH_UNKNOWN)
        self.assertFalse(cs.isData())
        self.assertEqual(cs.getAllocation(), OCIO.ALLOCATION_UNIFORM)
        self.assertEqual(cs.getAllocationVars(), [])

    def test_data(self):
        """
        Test the setIsData() and getIsData() methods.
        """

        # Boolean tests
        is_datas = [True, False]
        for is_data in is_datas:
            self.colorspace.setIsData(is_data)
            self.assertEqual(is_data, self.colorspace.isData())

        # Wrong type tests
        wrong_is_datas = [['test'], 'test']
        for wrong_is_data in wrong_is_datas:
            with self.assertRaises(TypeError):
                self.colorspace.setIsData(wrong_is_data)

    def test_description(self):
        """
        Test the setDescription() and getDescription() methods.
        """

        for desc in TEST_DESCS:
            self.colorspace.setDescription(desc)
            self.assertEqual(desc, self.colorspace.getDescription())

    def test_equality(self):
        """
        Test the setEqualityGroup() and getEqualityGroup() methods.
        """

        for name in TEST_NAMES:
            self.colorspace.setEqualityGroup(name)
            self.assertEqual(name, self.colorspace.getEqualityGroup())

    def test_family(self):
        """
        Test the setFamily() and getFamily() methods.
        """

        for name in TEST_NAMES:
            self.colorspace.setFamily(name)
            self.assertEqual(name, self.colorspace.getFamily())

    def test_name(self):
        """
        Test the setName() and getName() methods.
        """

        for name in TEST_NAMES:
            self.colorspace.setName(name)
            self.assertEqual(name, self.colorspace.getName())

    def test_transform(self):
        """
        Test the setTransform() and getTransform() methods.
        """

        # Known constants tests
        for i, direction in enumerate(OCIO.ColorSpaceDirection.__members__.values()):
            if direction == OCIO.COLORSPACE_DIR_UNKNOWN:
                with self.assertRaises(OCIO.Exception):
                    self.colorspace.setTransform(self.log_tr, direction)
            else:
                self.colorspace.setTransform(self.log_tr, direction)

            if direction == OCIO.COLORSPACE_DIR_UNKNOWN:
                with self.assertRaises(OCIO.Exception):
                    log_transform = self.colorspace.getTransform(
                        direction)
            else:
                log_transform = self.colorspace.getTransform(direction)
                self.assertIsInstance(log_transform, OCIO.LogTransform)
                self.assertEquals(self.log_tr.getBase(),
                                  log_transform.getBase())

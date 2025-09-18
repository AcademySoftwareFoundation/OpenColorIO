# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import copy
import unittest
import os
import sys

import PyOpenColorIO as OCIO
from UnitTestUtils import (SIMPLE_CONFIG, 
                           TEST_NAMES, 
                           TEST_DESCS, 
                           TEST_CATEGORIES, 
                           TEST_DATAFILES_DIR)


class ColorSpaceTest(unittest.TestCase):

    def setUp(self):
        self.colorspace = OCIO.ColorSpace()
        self.log_tr = OCIO.LogTransform(10)

    def tearDown(self):
        self.colorspace = None
        self.log_tr = None

    def test_copy(self):
        """
        Test the deepcopy() method.
        """
        self.colorspace.setName('colorspace1')
        self.colorspace.setFamily('family')
        self.colorspace.setEqualityGroup('group')
        self.colorspace.setDescription('description')
        self.colorspace.setBitDepth(OCIO.BIT_DEPTH_UINT8)
        self.colorspace.setEncoding('encoding')
        self.colorspace.setIsData(False)
        self.colorspace.setAllocation(OCIO.ALLOCATION_LG2)
        self.colorspace.setAllocationVars([-8, 5, 0.00390625])
        mat = OCIO.MatrixTransform()
        self.colorspace.setTransform(mat, OCIO.COLORSPACE_DIR_TO_REFERENCE)
        self.colorspace.setTransform(direction=OCIO.COLORSPACE_DIR_FROM_REFERENCE, transform=mat)
        self.colorspace.addAlias('alias')
        self.colorspace.addCategory('cat')
        self.colorspace.setInteropID('data')
        self.colorspace.setInterchangeAttribute('amf_transform_ids', 'urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.CG_to_ACES.a1.0.3')
        self.colorspace.setInterchangeAttribute('icc_profile_name', 'sRGB IEC61966-2.1')

        other = copy.deepcopy(self.colorspace)
        self.assertFalse(other is self.colorspace)

        self.assertEqual(other.getName(), self.colorspace.getName())
        self.assertEqual(other.getFamily(), self.colorspace.getFamily())
        self.assertEqual(other.getEqualityGroup(), self.colorspace.getEqualityGroup())
        self.assertEqual(other.getDescription(), self.colorspace.getDescription())
        self.assertEqual(other.getBitDepth(), self.colorspace.getBitDepth())
        self.assertEqual(other.getEncoding(), self.colorspace.getEncoding())
        self.assertEqual(other.isData(), self.colorspace.isData())
        self.assertEqual(other.getAllocation(), self.colorspace.getAllocation())
        self.assertEqual(other.getAllocationVars(), self.colorspace.getAllocationVars())
        self.assertTrue(other.getTransform(OCIO.COLORSPACE_DIR_TO_REFERENCE).equals(self.colorspace.getTransform(OCIO.COLORSPACE_DIR_TO_REFERENCE)))
        self.assertTrue(other.getTransform(OCIO.COLORSPACE_DIR_FROM_REFERENCE).equals(self.colorspace.getTransform(OCIO.COLORSPACE_DIR_FROM_REFERENCE)))
        self.assertEqual(list(other.getAliases()), list(self.colorspace.getAliases()))
        self.assertEqual(list(other.getCategories()), list(self.colorspace.getCategories()))
        self.assertEqual(other.getInteropID(), self.colorspace.getInteropID())
        self.assertEqual(other.getInterchangeAttribute('amf_transform_ids'), self.colorspace.getInterchangeAttribute('amf_transform_ids'))
        self.assertEqual(other.getInterchangeAttribute('icc_profile_name'), self.colorspace.getInterchangeAttribute('icc_profile_name'))
        self.assertEqual(other.getInterchangeAttributes(), self.colorspace.getInterchangeAttributes())
        self.assertListEqual(list(other.getInterchangeAttributes().items()), list(self.colorspace.getInterchangeAttributes().items()))

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

        # Get simple config file from UnitTestUtils.py
        cfg = OCIO.Config().CreateFromStream(SIMPLE_CONFIG)

        # Test ColorSpace class object getters from config
        cs = cfg.getColorSpace('vd8')
        self.assertEqual(cs.getName(), 'vd8')
        self.assertEqual(cs.getDescription(), 'how many transforms can we use?')
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
                             encoding='scene-linear',
                             equalityGroup='My_Equality',
                             description='This is a test colourspace!',
                             bitDepth=OCIO.BIT_DEPTH_F32,
                             isData=False,
                             allocation=OCIO.ALLOCATION_LG2,
                             allocationVars=[0.0, 1.0])

        self.assertEqual(cs.getName(), 'test')
        self.assertEqual(cs.getFamily(), 'ocio family')
        self.assertEqual(cs.getEncoding(), 'scene-linear')
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
                              encoding='scene-linear',
                              bitDepth=OCIO.BIT_DEPTH_F32)

        self.assertEqual(cs2.getName(), 'test')
        self.assertEqual(cs2.getFamily(), 'ocio family')
        self.assertEqual(cs2.getEncoding(), 'scene-linear')
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
                             ['alias1', 'alias2'],
                             'ocio family',
                             'scene-linear',
                             'My_Equality',
                             'This is a test colourspace!',
                             OCIO.BIT_DEPTH_F32,
                             False,
                             OCIO.ALLOCATION_LG2,
                             [0.0, 1.0])

        self.assertEqual(cs.getName(), 'test')
        aliases = cs.getAliases()
        self.assertEqual(len(aliases), 2)
        self.assertEqual(aliases[0], 'alias1')
        self.assertEqual(aliases[1], 'alias2')
        self.assertEqual(cs.getFamily(), 'ocio family')
        self.assertEqual(cs.getEncoding(), 'scene-linear')
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
        self.assertEqual(cs.getEncoding(), '')
        self.assertEqual(cs.getDescription(), '')
        self.assertEqual(cs.getBitDepth(), OCIO.BIT_DEPTH_UNKNOWN)
        self.assertFalse(cs.isData())
        self.assertEqual(cs.getAllocation(), OCIO.ALLOCATION_UNIFORM)
        self.assertEqual(cs.getAllocationVars(), [])
        self.assertEqual(cs.getInteropID(), '')
        self.assertEqual(cs.getInterchangeAttribute("amf_transform_ids"), '')
        self.assertEqual(cs.getInterchangeAttribute("icc_profile_name"), '')
        self.assertEqual(len(cs.getInterchangeAttributes()), 0)

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

    def test_encoding(self):
        """
        Test the setEncoding() and getEncoding() methods.
        """

        for name in TEST_NAMES:
            self.colorspace.setEncoding(name)
            self.assertEqual(name, self.colorspace.getEncoding())

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
            self.colorspace.setTransform(self.log_tr, direction)
            log_transform = self.colorspace.getTransform(direction)
            self.assertIsInstance(log_transform, OCIO.LogTransform)
            self.assertEqual(self.log_tr.getBase(), log_transform.getBase())

    def test_aliases(self):
        """
        Test ColorSpace aliases.
        """

        cs = OCIO.ColorSpace()
        self.assertEqual(cs.getName(), '')
        aliases = cs.getAliases()
        self.assertEqual(len(aliases), 0)

        cs.addAlias('alias1')
        aliases = cs.getAliases()
        self.assertEqual(len(aliases), 1)
        self.assertEqual(aliases[0], 'alias1')
        self.assertTrue(cs.hasAlias('alias1'))
        self.assertTrue(cs.hasAlias('aLiaS1'))
        self.assertFalse(cs.hasAlias('alias2'))

        cs.addAlias('alias2')
        aliases = cs.getAliases()
        self.assertEqual(len(aliases), 2)
        self.assertEqual(aliases[0], 'alias1')
        self.assertEqual(aliases[1], 'alias2')
        self.assertTrue(cs.hasAlias('alias2'))

        # Alias is already there, not added.

        cs.addAlias('Alias2')
        aliases = cs.getAliases()
        self.assertEqual(len(aliases), 2)
        self.assertEqual(aliases[0], 'alias1')
        self.assertEqual(aliases[1], 'alias2')

        # Name might remove an alias.

        cs.setName('name')
        aliases = cs.getAliases()
        self.assertEqual(len(aliases), 2)

        cs.setName('alias2')
        aliases = cs.getAliases()
        self.assertEqual(len(aliases), 1)
        self.assertEqual(aliases[0], 'alias1')
        self.assertFalse(cs.hasAlias('alias2'))

        # Removing an alias.

        cs.addAlias('to remove')
        aliases = cs.getAliases()
        self.assertEqual(len(aliases), 2)
        self.assertTrue(cs.hasAlias('to remove'))
        self.assertTrue(cs.hasAlias('to REMOVE'))

        cs.removeAlias('not found')
        aliases = cs.getAliases()
        self.assertEqual(len(aliases), 2)

        cs.removeAlias('to REMOVE')
        aliases = cs.getAliases()
        self.assertEqual(len(aliases), 1)
        self.assertFalse(cs.hasAlias('to remove'))

        cs.clearAliases()
        aliases = cs.getAliases()
        self.assertEqual(len(aliases), 0)
        self.assertFalse(cs.hasAlias('alias1'))

    def test_interop_id(self):
        """
        Test the setInteropID() and getInteropID() methods.
        """
        
        # Test default value (should be empty).
        self.assertEqual(self.colorspace.getInteropID(), '')
        
        # Test setting and getting a simple interop ID.
        test_id = 'lin_ap0_scene'
        self.colorspace.setInteropID(test_id)
        self.assertEqual(self.colorspace.getInteropID(), test_id)
        
        # Test setting and getting a different interop ID.
        test_id2 = 'srgb_ap1_scene'
        self.colorspace.setInteropID(test_id2)
        self.assertEqual(self.colorspace.getInteropID(), test_id2)
        
        # Test setting empty string.
        self.colorspace.setInteropID('')
        self.assertEqual(self.colorspace.getInteropID(), '')
        
        # Test setting None (should convert to empty string).
        self.colorspace.setInteropID('something')
        self.colorspace.setInteropID(None)
        self.assertEqual(self.colorspace.getInteropID(), '')
        
        # Test wrong type (should raise TypeError).
        with self.assertRaises(TypeError):
            self.colorspace.setInteropID(123)
        
        with self.assertRaises(TypeError):
            self.colorspace.setInteropID(['list'])

        # Test valid InteropID with one colon (not at the end).
        valid_with_colon = 'namespace:cs_name'
        self.colorspace.setInteropID(valid_with_colon)
        self.assertEqual(self.colorspace.getInteropID(), valid_with_colon)

        # Test invalid InteropID with multiple colons.
        with self.assertRaises(Exception) as context:
            self.colorspace.setInteropID('name:space:cs_name')
        self.assertIn("Only one ':' is allowed to separate the namespace and the color space.",
                      str(context.exception))

        # Test invalid InteropID with colon at the end.
        with self.assertRaises(Exception) as context:
            self.colorspace.setInteropID('namespace:')
        self.assertIn("If ':' is used, both the namespace and the color space parts must be non-empty.",
                      str(context.exception))

        # Test invalid InteropID with non-ASCII characters.
        with self.assertRaises(Exception) as context:
            self.colorspace.setInteropID('café_scene')  # Contains é (UTF-8)
        self.assertIn("contains invalid characters.", str(context.exception))

        with self.assertRaises(Exception) as context:
            self.colorspace.setInteropID('space±_name')  # Contains ± (ANSI 0xB1)
        self.assertIn("contains invalid characters.", str(context.exception))

    def test_interchange_attributes(self):
        """
        Test the setInterchangeAttribute() and getInterchangeAttribute() methods.
        """
        # amf_transform_ids

        # Test default value (should be empty).
        self.assertEqual(self.colorspace.getInterchangeAttribute('amf_transform_ids'), '')
        
        # Test setting and getting a single amf transform ID.
        single_id = 'urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.CG_to_ACES.a1.0.3'
        self.colorspace.setInterchangeAttribute('amf_transform_ids', single_id)
        self.assertEqual(self.colorspace.getInterchangeAttribute('amf_transform_ids'), single_id)
        self.assertEqual(len(self.colorspace.getInterchangeAttributes()), 1)
        self.assertEqual(self.colorspace.getInterchangeAttributes()["amf_transform_ids"], single_id)
        
        # Test setting and getting multiple transform IDs (newline-separated).
        multiple_ids = ('urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.CG_to_ACES.a1.0.3\n'
                       'urn:ampas:aces:transformId:v1.5:ACEScsc.Academy.ACES_to_CG.a1.0.3\n'
                       'urn:ampas:aces:transformId:v1.5:RRT.a1.0.3')
        self.colorspace.setInterchangeAttribute('amf_transform_ids', multiple_ids)
        self.assertEqual(self.colorspace.getInterchangeAttribute('amf_transform_ids'), multiple_ids)
        self.assertEqual(len(self.colorspace.getInterchangeAttributes()), 1)
        self.assertEqual(self.colorspace.getInterchangeAttributes()["amf_transform_ids"], multiple_ids)
        
        # Test setting empty string.
        self.colorspace.setInterchangeAttribute('amf_transform_ids', '')
        self.assertEqual(self.colorspace.getInterchangeAttribute('amf_transform_ids'), '')
        self.assertEqual(len(self.colorspace.getInterchangeAttributes()), 0)
       
        # Test setting None (should convert to empty string).
        self.colorspace.setInterchangeAttribute('amf_transform_ids', 'something')
        self.colorspace.setInterchangeAttribute('amf_transform_ids', None)
        self.assertEqual(self.colorspace.getInterchangeAttribute('amf_transform_ids'), '')
        self.assertEqual(len(self.colorspace.getInterchangeAttributes()), 0)

        # Test with different line endings.
        mixed_endings = 'id1\nid2\rid3\r\nid4'
        self.colorspace.setInterchangeAttribute('amf_transform_ids', mixed_endings)
        self.assertEqual(self.colorspace.getInterchangeAttribute('amf_transform_ids'), mixed_endings)
        
        # Test with leading/trailing whitespace.
        whitespace_ids = '  \n  id1  \n  id2  \n  '
        self.colorspace.setInterchangeAttribute('amf_transform_ids', whitespace_ids)
        self.assertEqual(self.colorspace.getInterchangeAttribute('amf_transform_ids'), whitespace_ids)
        
        # Test wrong type (should raise TypeError).
        with self.assertRaises(TypeError):
            self.colorspace.setInterchangeAttribute('amf_transform_ids', 123)
        
        with self.assertRaises(TypeError):
            self.colorspace.setInterchangeAttribute('amf_transform_ids', ['list', 'of', 'ids'])

        # clear amf_transform_ids for the next test
        self.colorspace.setInterchangeAttribute('amf_transform_ids', '')
        self.assertEqual(len(self.colorspace.getInterchangeAttributes()), 0)

        # icc_profile_name

        # Test default value (should be empty).
        self.assertEqual(self.colorspace.getInterchangeAttribute('icc_profile_name'), '')
 
        # Test setting and getting a simple profile name.
        profile_name = 'sRGB IEC61966-2.1'
        self.colorspace.setInterchangeAttribute('icc_profile_name', profile_name)
        self.assertEqual(self.colorspace.getInterchangeAttribute('icc_profile_name'), profile_name)
        self.assertEqual(len(self.colorspace.getInterchangeAttributes()), 1)
        self.assertEqual(self.colorspace.getInterchangeAttributes()["icc_profile_name"], profile_name)
        
        # Test setting and getting a different profile name.
        profile_name2 = 'Adobe RGB (1998)'
        self.colorspace.setInterchangeAttribute('icc_profile_name', profile_name2)
        self.assertEqual(self.colorspace.getInterchangeAttribute('icc_profile_name'), profile_name2)
        self.assertEqual(len(self.colorspace.getInterchangeAttributes()), 1)
        self.assertEqual(self.colorspace.getInterchangeAttributes()["icc_profile_name"], profile_name2)
        
        # Test with a more complex profile name.
        complex_name = 'Display P3 - Apple Cinema Display (Calibrated 2023-01-15)'
        self.colorspace.setInterchangeAttribute('icc_profile_name', complex_name)
        self.assertEqual(self.colorspace.getInterchangeAttribute('icc_profile_name'), complex_name)

        # Test setting empty string.
        self.colorspace.setInterchangeAttribute('icc_profile_name', '')
        self.assertEqual(self.colorspace.getInterchangeAttribute('icc_profile_name'), '')
        self.assertEqual(len(self.colorspace.getInterchangeAttributes()), 0)
        
        # Test setting None (should convert to empty string).
        self.colorspace.setInterchangeAttribute('icc_profile_name', 'something')
        self.colorspace.setInterchangeAttribute('icc_profile_name', None)
        self.assertEqual(self.colorspace.getInterchangeAttribute('icc_profile_name'), '')
        self.assertEqual(len(self.colorspace.getInterchangeAttributes()), 0)
        
        # Test with special characters and numbers.
        special_name = 'ProPhoto RGB v2.0 (γ=1.8) [Custom Profile #123]'
        self.colorspace.setInterchangeAttribute('icc_profile_name', special_name)
        self.assertEqual(self.colorspace.getInterchangeAttribute('icc_profile_name'), special_name)
        
        # Test with Unicode characters.
        unicode_name = 'Профиль RGB γ=2.2'
        self.colorspace.setInterchangeAttribute('icc_profile_name', unicode_name)
        self.assertEqual(self.colorspace.getInterchangeAttribute('icc_profile_name'), unicode_name)
        
        # Test wrong type (should raise TypeError).
        with self.assertRaises(TypeError):
            self.colorspace.setInterchangeAttribute('icc_profile_name', 123)
        
        with self.assertRaises(TypeError):
            self.colorspace.setInterchangeAttribute('icc_profile_name', ['profile', 'name'])

        # test both interchange attributes together.

        self.colorspace.setInterchangeAttribute('icc_profile_name', 'icc_value')
        self.colorspace.setInterchangeAttribute('amf_transform_ids', 'amf_value')
        self.assertEqual(len(self.colorspace.getInterchangeAttributes()), 2)
        self.assertEqual(self.colorspace.getInterchangeAttribute('icc_profile_name'), 'icc_value')
        self.assertEqual(self.colorspace.getInterchangeAttribute('amf_transform_ids'), 'amf_value')

        # unsupported interchange key

        # test that setting an unsupported key raises.
        with self.assertRaises(Exception) as context:
            self.colorspace.setInterchangeAttribute('this_should_fail', 'foo42')
        self.assertIn("Unknown attribute name 'this_should_fail'", str(context.exception))

        # test that getting an unsupported key raises.
        with self.assertRaises(Exception) as context:
            self.colorspace.getInterchangeAttribute('this_should_fail')
        self.assertIn("Unknown attribute name 'this_should_fail'", str(context.exception))

    def test_is_colorspace_linear(self):
        """
        Test isColorSpaceLinear.
        """
        SIMPLE_PROFILE = """ocio_profile_version: 2

description: Test config for the isColorSpaceLinear method.

environment:
  {}
search_path: "non_existing_path"
roles:
  aces_interchange: scene_linear-trans
  cie_xyz_d65_interchange: display_linear-enc
  color_timing: scene_linear-trans
  compositing_log: scene_log-enc
  default: display_data
  scene_linear: scene_linear-trans

displays:
  generic display:
    - !<View> {name: Raw, colorspace: scene_data}

# Make a few of the color spaces inactive, this should not affect the result.
inactive_colorspaces: [display_linear-trans, scene_linear-trans]

view_transforms:
  - !<ViewTransform>
    name: view_transform
    from_scene_reference: !<MatrixTransform> {}

# Display-referred color spaces.

display_colorspaces:
  - !<ColorSpace>
    name: display_data
    description: |
      Data space.
      Has a linear transform, which should never happen, but this will be ignored since 
      isdata is true.
    isdata: true
    encoding: data
    from_display_reference: !<MatrixTransform> {matrix: [ 3.240969941905, -1.537383177570, -0.498610760293, 0, -0.969243636281, 1.875967501508, 0.041555057407, 0, 0.055630079697, -0.203976958889, 1.056971514243, 0, 0, 0, 0, 1 ]}

  - !<ColorSpace>
    name: display_linear-enc
    description: |
      Encoding set to display-linear.
      Has a non-existent transform, but this should be ignored since the encoding takes precedence.
    isdata: false
    encoding: display-linear
    from_display_reference: !<FileTransform> {src: does-not-exist.lut}

  - !<ColorSpace>
    name: display_wrong-linear-enc
    description: |
      Encoding set to scene-linear.  This should never happen for a display space, but test it.
    isdata: false
    encoding: scene-linear

  - !<ColorSpace>
    name: display_video-enc
    description: |
      Encoding set to sdr-video.
      Has a linear transform, but this should be ignored since the encoding takes precedence.
    isdata: false
    encoding: sdr-video
    from_display_reference: !<MatrixTransform> {matrix: [ 3.240969941905, -1.537383177570, -0.498610760293, 0, -0.969243636281, 1.875967501508, 0.041555057407, 0, 0.055630079697, -0.203976958889, 1.056971514243, 0, 0, 0, 0, 1 ]}

  - !<ColorSpace>
    name: display_linear-trans
    description: |
      No encoding.  Transform is linear.
    isdata: false
    from_display_reference: !<GroupTransform>
      children:
        - !<MatrixTransform> {matrix: [ 3.240969941905, -1.537383177570, -0.498610760293, 0, -0.969243636281, 1.875967501508, 0.041555057407, 0, 0.055630079697, -0.203976958889, 1.056971514243, 0, 0, 0, 0, 1 ]}
        - !<CDLTransform> {slope: [0.1, 2, 3], style: noclamp}

  - !<ColorSpace>
    name: display_video-trans
    description: |
      No encoding.  Transform is non-linear.
    isdata: false
    from_display_reference: !<BuiltinTransform> {style: DISPLAY - CIE-XYZ-D65_to_sRGB}

# Scene-referred color spaces.

colorspaces:
  - !<ColorSpace>
    name: scene_data
    description: |
      Data space.
      Has a linear transform, which should never happen, but this will be ignored 
      since isdata is true.
    isdata: true
    encoding: data
    from_scene_reference: !<MatrixTransform> {matrix: [ 3.240969941905, -1.537383177570, -0.498610760293, 0, -0.969243636281, 1.875967501508, 0.041555057407, 0, 0.055630079697, -0.203976958889, 1.056971514243, 0, 0, 0, 0, 1 ]}

  - !<ColorSpace>
    name: scene_linear-enc
    description: |
      Encoding set to scene-linear.
      Has a non-linear transform, but this will be ignored since the encoding takes precedence.
    isdata: false
    encoding: scene-linear
    from_scene_reference: !<BuiltinTransform> {style: DISPLAY - CIE-XYZ-D65_to_sRGB}

  - !<ColorSpace>
    name: scene_wrong-linear-enc
    description: |
      Encoding set to display-linear.  This should never happen for a scene space, but test it.
    isdata: false
    encoding: display-linear

  - !<ColorSpace>
    name: scene_log-enc
    description: |
      Encoding set to log.
      Has a linear transform, but this will be ignored since the encoding takes precedence.
    isdata: false
    encoding: log
    from_scene_reference: !<MatrixTransform> {matrix: [ 3.240969941905, -1.537383177570, -0.498610760293, 0, -0.969243636281, 1.875967501508, 0.041555057407, 0, 0.055630079697, -0.203976958889, 1.056971514243, 0, 0, 0, 0, 1 ]}

  - !<ColorSpace>
    name: scene_linear-trans
    aliases: [scene_linear-trans-alias]
    description: |
      No encoding.  Transform is linear.
    isdata: false
    to_scene_reference: !<GroupTransform>
      children:
        - !<BuiltinTransform> {style: UTILITY - ACES-AP0_to_CIE-XYZ-D65_BFD}
        - !<MatrixTransform> {matrix: [ 3.240969941905, -1.537383177570, -0.498610760293, 0, -0.969243636281, 1.875967501508, 0.041555057407, 0, 0.055630079697, -0.203976958889, 1.056971514243, 0, 0, 0, 0, 1 ]}
        - !<MatrixTransform> {matrix: [ 3.240969941905, -1.537383177570, -0.498610760293, 0, -0.969243636281, 1.875967501508, 0.041555057407, 0, 0.055630079697, -0.203976958889, 1.056971514243, 0, 0, 0, 0, 1 ]}

  - !<ColorSpace>
    name: scene_nonlin-trans
    description: |
      No encoding.  Transform is non-linear because it clamps values outside [0,1].
    isdata: false
    to_scene_reference: !<GroupTransform>
      children:
        - !<MatrixTransform> {matrix: [ 3.240969941905, -1.537383177570, -0.498610760293, 0, -0.969243636281, 1.875967501508, 0.041555057407, 0, 0.055630079697, -0.203976958889, 1.056971514243, 0, 0, 0, 0, 1 ]}
        - !<RangeTransform> {min_in_value: 0., min_out_value: 0., max_in_value: 1., max_out_value: 1.}

  - !<ColorSpace>
    name: scene_ref
    description: |
      No encoding.  Considered linear since it is equivalent to the reference space.
    isdata: false
"""  
        # Create a config.
        cfg = OCIO.Config.CreateFromStream(SIMPLE_PROFILE)

        def test_scene_referred(self, cfg, cs_name, expected_value):
            cs = cfg.getColorSpace(cs_name)
            is_linear_to_scene_reference = cfg.isColorSpaceLinear(
                cs_name, 
                OCIO.REFERENCE_SPACE_SCENE
            )
            self.assertEqual(is_linear_to_scene_reference, expected_value)

        def test_display_referred(self, cfg, cs_name, expected_value):
            cs = cfg.getColorSpace(cs_name)
            is_linear_to_display_reference = cfg.isColorSpaceLinear(
                cs_name, 
                OCIO.REFERENCE_SPACE_DISPLAY
            )
            self.assertEqual(is_linear_to_display_reference, expected_value)

        # Test undefined color spaces.
        with self.assertRaises(OCIO.Exception):
            cfg.isColorSpaceLinear('colorspace_abc', OCIO.REFERENCE_SPACE_SCENE)
        with self.assertRaises(OCIO.Exception):
            cfg.isColorSpaceLinear('colorspace_abc', OCIO.REFERENCE_SPACE_DISPLAY)

        # Test the scene referred color spaces.
        test_scene_referred(self, cfg, "display_data", False)
        test_scene_referred(self, cfg, "display_linear-enc", False)
        test_scene_referred(self, cfg, "display_wrong-linear-enc", False)
        test_scene_referred(self, cfg, "display_video-enc", False)
        test_scene_referred(self, cfg, "display_linear-trans", False)
        test_scene_referred(self, cfg, "display_video-trans", False)

        test_scene_referred(self, cfg, "scene_data", False)
        test_scene_referred(self, cfg, "scene_linear-enc", True)
        test_scene_referred(self, cfg, "scene_wrong-linear-enc", False)
        test_scene_referred(self, cfg, "scene_log-enc", False)
        test_scene_referred(self, cfg, "scene_linear-trans", True)
        test_scene_referred(self, cfg, "scene_nonlin-trans", False)
        test_scene_referred(self, cfg, "scene_linear-trans-alias", True)
        test_scene_referred(self, cfg, "scene_ref", True)

        # Test the display referred color spaces.
        test_display_referred(self, cfg, "display_data", False)
        test_display_referred(self, cfg, "display_linear-enc", True)
        test_display_referred(self, cfg, "display_wrong-linear-enc", False)
        test_display_referred(self, cfg, "display_video-enc", False)
        test_display_referred(self, cfg, "display_linear-trans", True)
        test_display_referred(self, cfg, "display_video-trans", False)

        test_display_referred(self, cfg, "scene_data", False)
        test_display_referred(self, cfg, "scene_linear-enc", False)
        test_display_referred(self, cfg, "scene_wrong-linear-enc", False)
        test_display_referred(self, cfg, "scene_log-enc", False)
        test_display_referred(self, cfg, "scene_linear-trans", False)
        test_display_referred(self, cfg, "scene_nonlin-trans", False)
        test_display_referred(self, cfg, "scene_linear-trans-alias", False)
        test_display_referred(self, cfg, "scene_ref", False)
        
    def test_processor_to_known_colorspace(self):
        
        CONFIG = """ocio_profile_version: 2.0

roles:
  default: raw
  scene_linear: ref_cs

display_colorspaces:
  - !<ColorSpace>
    name: CIE-XYZ-D65
    description: The CIE XYZ (D65) display connection colorspace.
    isdata: false

  - !<ColorSpace>
    name: sRGB - Display CS
    description: Convert CIE XYZ (D65 white) to sRGB (piecewise EOTF)
    isdata: false
    from_display_reference: !<BuiltinTransform> {style: DISPLAY - CIE-XYZ-D65_to_sRGB}

colorspaces:
  # Put a couple of test color space first in the config since the heuristics stop upon success.

  - !<ColorSpace>
    name: File color space
    description: Verify that that FileTransforms load correctly when running the heuristics.
    isdata: false
    from_scene_reference: !<GroupTransform>
      children:
        - !<FileTransform> {src: lut1d_green.ctf}

  - !<ColorSpace>
    name: CS Transform color space
    description: Verify that that ColorSpaceTransforms load correctly when running the heuristics.
    isdata: false
    from_scene_reference: !<GroupTransform>
      children:
        - !<ColorSpaceTransform> {src: ref_cs, dst: not sRGB}

  - !<ColorSpace>
    name: raw
    description: A data colorspace (should not be used).
    isdata: true

  - !<ColorSpace>
    name: ref_cs
    description: The reference colorspace, ACES2065-1.
    isdata: false

  - !<ColorSpace>
    name: not sRGB
    description: A color space that misleadingly has sRGB in the name, even though it's not.
    isdata: false
    to_scene_reference: !<BuiltinTransform> {style: ACEScct_to_ACES2065-1}

  - !<ColorSpace>
    name: ACES cg
    description: An ACEScg space with an unusual spelling.
    interop_id: lin_ap1_scene
    isdata: false
    to_scene_reference: !<BuiltinTransform> {style: ACEScg_to_ACES2065-1}

  - !<ColorSpace>
    name: Linear ITU-R BT.709
    description: A linear Rec.709 space with an unusual spelling.
    interop_id: "mycompany:led_wall_1"
    isdata: false
    from_scene_reference: !<GroupTransform>
      name: AP0 to Linear Rec.709 (sRGB)
      children:
        - !<MatrixTransform> {matrix: [2.52168618674388, -1.13413098823972, -0.387555198504164, 0, -0.276479914229922, 1.37271908766826, -0.096239173438334, 0, -0.0153780649660342, -0.152975335867399, 1.16835340083343, 0, 0, 0, 0, 1]}

  - !<ColorSpace>
    name: sRGB Encoded AP1 - Texture
    description: Another space with "sRGB" in the name that is not actually an sRGB texture space.
    isdata: false
    from_scene_reference: !<GroupTransform>
      name: AP0 to sRGB Encoded AP1 - Texture
      children:
        - !<MatrixTransform> {matrix: [1.45143931614567, -0.23651074689374, -0.214928569251925, 0, -0.0765537733960206, 1.17622969983357, -0.0996759264375522, 0, 0.00831614842569772, -0.00603244979102102, 0.997716301365323, 0, 0, 0, 0, 1]}
        - !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055, direction: inverse}

  - !<ColorSpace>
    name: Texture -- sRGB
    description: An sRGB Texture space, spelled differently than in the built-in config.
    isdata: false
    from_scene_reference: !<GroupTransform>
      name: AP0 to sRGB Rec.709
      children:
        - !<MatrixTransform> {matrix: [2.52168618674388, -1.13413098823972, -0.387555198504164, 0, -0.276479914229922, 1.37271908766826, -0.096239173438334, 0, -0.0153780649660342, -0.152975335867399, 1.16835340083343, 0, 0, 0, 0, 1]}
        - !<ExponentWithLinearTransform> {gamma: 2.4, offset: 0.055, direction: inverse}

"""
        def check_processor(self, p):
            gt = p.createGroupTransform()
            self.assertEqual(len(gt), 4)

            self.assertAlmostEqual(gt[0].getLogSideSlopeValue()[0], 0.0570776, places=6)
            self.assertEqual(gt[0].getDirection(), OCIO.TRANSFORM_DIR_INVERSE)

            self.assertAlmostEqual(gt[1].getMatrix()[0], 0.6954522413574519, places=6)
            self.assertEqual(gt[1].getDirection(), OCIO.TRANSFORM_DIR_FORWARD)

            self.assertAlmostEqual(gt[2].getMatrix()[0], 1.45143931607166, places=6)
            self.assertEqual(gt[2].getDirection(), OCIO.TRANSFORM_DIR_FORWARD)

            self.assertAlmostEqual(gt[3].getValue()[0], 2.2, places=6)
            self.assertEqual(gt[3].getDirection(), OCIO.TRANSFORM_DIR_INVERSE)

        def check_processor_inv(self, p):
            gt = p.createGroupTransform()
            self.assertEqual(len(gt), 4)

            self.assertAlmostEqual(gt[0].getValue()[0], 2.2, places=6)
            self.assertEqual(gt[0].getDirection(), OCIO.TRANSFORM_DIR_FORWARD)

            self.assertAlmostEqual(gt[1].getMatrix()[0], 0.6954522413574519, places=6)
            self.assertEqual(gt[1].getDirection(), OCIO.TRANSFORM_DIR_FORWARD)

            self.assertAlmostEqual(gt[2].getMatrix()[0], 1.45143931607166, places=6)
            self.assertEqual(gt[2].getDirection(), OCIO.TRANSFORM_DIR_FORWARD)

            self.assertAlmostEqual(gt[3].getLogSideSlopeValue()[0], 0.0570776, places=6)
            self.assertEqual(gt[3].getDirection(), OCIO.TRANSFORM_DIR_FORWARD)
            cfg = OCIO.Config.CreateFromStream(CONFIG)

        cfg = OCIO.Config.CreateFromStream(CONFIG)

        cfg.addSearchPath(TEST_DATAFILES_DIR)

        # Make all color spaces suitable for the heuristics inactive.
        # The heuristics don't look at inactive color spaces.
        cfg.setInactiveColorSpaces("ACES cg, Linear ITU-R BT.709, Texture -- sRGB")

        src_csname = "not sRGB"
        builtin_csname = "Gamma 2.2 AP1 - Texture"

        # Test throw if no suitable spaces are present.
        with self.assertRaises(OCIO.Exception):
            p = OCIO.Config.GetProcessorToBuiltinColorSpace(cfg, src_csname, builtin_csname)

        # Test sRGB Texture space.
        cfg.setInactiveColorSpaces("ACES cg, Linear ITU-R BT.709")
        p = OCIO.Config.GetProcessorToBuiltinColorSpace(cfg, src_csname, builtin_csname)
        check_processor(self, p)

        # Test linear color space from_ref direction.
        cfg.setInactiveColorSpaces("ACES cg, Texture -- sRGB")
        p = OCIO.Config.GetProcessorToBuiltinColorSpace(cfg, src_csname, builtin_csname)
        check_processor(self, p)

        # Test linear color space to_ref direction.
        cfg.setInactiveColorSpaces("Linear ITU-R BT.709, Texture -- sRGB")
        p = OCIO.Config.GetProcessorToBuiltinColorSpace(cfg, src_csname, builtin_csname)
        check_processor(self, p)

        # Test sRGB Texture space.
        cfg.setInactiveColorSpaces("ACES cg, Linear ITU-R BT.709")
        p = OCIO.Config.GetProcessorFromBuiltinColorSpace(builtin_csname, cfg, src_csname)
        check_processor_inv(self, p)

        # Test linear color space from_ref direction.
        cfg.setInactiveColorSpaces("ACES cg, Texture -- sRGB")
        p = OCIO.Config.GetProcessorFromBuiltinColorSpace(builtin_csname, cfg, src_csname)
        check_processor_inv(self, p)

        # Test linear color space to_ref direction.
        cfg.setInactiveColorSpaces("Linear ITU-R BT.709, Texture -- sRGB")
        p = OCIO.Config.GetProcessorFromBuiltinColorSpace(builtin_csname, cfg, src_csname)
        check_processor_inv(self, p)


        editableCfg = copy.deepcopy(cfg)
        builtinConfig = OCIO.Config.CreateFromFile("ocio://default")

        #
        # Test IdentifyBuiltinColorSpace.
        #

        editableCfg.setInactiveColorSpaces("")

        csname = OCIO.Config.IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "ACEScg")
        self.assertEqual(csname, "ACES cg")

        csname = OCIO.Config.IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "sRGB - Texture")
        self.assertEqual(csname, "Texture -- sRGB")

        csname = OCIO.Config.IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "ACES2065-1")
        self.assertEqual(csname, "ref_cs")

        editableCfg.setInactiveColorSpaces("Texture -- sRGB, ref_cs")

        csname = OCIO.Config.IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "Linear Rec.709 (sRGB)")
        self.assertEqual(csname, "Linear ITU-R BT.709")

        csname = OCIO.Config.IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "ACEScct")
        self.assertEqual(csname, "not sRGB")

        csname = OCIO.Config.IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "lin_ap1")
        self.assertEqual(csname, "ACES cg")

        # Display-referred spaces are not supported unless the display-referred interchange
        # role is present.

        with self.assertRaises(OCIO.Exception) as cm:
            OCIO.Config.IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "sRGB - Display")
        self.assertEqual(
          str(cm.exception), 
          "The heuristics currently only support scene-referred color spaces. Please set the interchange roles."
        )

        # The next three cases directly use the interchange roles rather than heuristics.

        # With the required role, it then works.
        editableCfg.setRole("cie_xyz_d65_interchange", "CIE-XYZ-D65")
        csname = OCIO.Config.IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "sRGB - Display")
        self.assertEqual(csname, "sRGB - Display CS")

        # Must continue to work if the color space for the interchange role is inactive.
        editableCfg.setInactiveColorSpaces("CIE-XYZ-D65")
        csname = OCIO.Config.IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "sRGB - Display")
        self.assertEqual(csname, "sRGB - Display CS")

        # Test the scene-referred interchange role (and even make it inactive).
        editableCfg.setRole("aces_interchange", "ref_cs")
        editableCfg.setInactiveColorSpaces("ref_cs")
        csname = OCIO.Config.IdentifyBuiltinColorSpace(editableCfg, builtinConfig, "ACEScg")
        self.assertEqual(csname, "ACES cg")


        #
        # Test IdentifyInterchangeSpace.
        #

        # Uses "ACEScg" to find the reference.
        spaces = OCIO.Config.IdentifyInterchangeSpace(editableCfg, "Linear ITU-R BT.709",
                                                      builtinConfig, "lin_rec709_srgb") # Aliases work.
        self.assertEqual(spaces[0], "ref_cs")
        self.assertEqual(spaces[1], "ACES2065-1")

        # Set the interchange role.  In order to prove that it is being used rather than
        # the heuristics, set it to something wrong and check that it gets returned anyway.
        editableCfg.setRole("aces_interchange", "Texture -- sRGB")

        spaces = OCIO.Config.IdentifyInterchangeSpace(editableCfg, "Linear ITU-R BT.709", 
                                              builtinConfig, "lin_rec709_srgb")
        self.assertEqual(spaces[0], "Texture -- sRGB")
        self.assertEqual(spaces[1], "ACES2065-1")

        # Unset the interchange role, so the heuristics will be used for the other tests.
        editableCfg.setRole("aces_interchange", "")

        # Check what happens if a totally bogus config is passed for the built-in config.
        # (It fails in the first heuristic that tries to use one of the known built-in spaces.)
        rawCfg = OCIO.Config.CreateRaw()

        with self.assertRaises(OCIO.Exception) as cm:
          spaces = OCIO.Config.IdentifyInterchangeSpace(editableCfg, "Raw", rawCfg, "raw")
        self.assertEqual(
          str(cm.exception), 
          "Could not find destination color space 'sRGB - Texture'."
        )

        # Check what happens if the source color space doesn't exist.
        with self.assertRaises(OCIO.Exception) as cm:
          spaces = OCIO.Config.IdentifyInterchangeSpace(editableCfg, "Foo", rawCfg, "raw")
        self.assertEqual(
          str(cm.exception), 
          "Could not find source color space 'Foo'."
        )

        # Check what happens if the destination color space doesn't exist.
        with self.assertRaises(OCIO.Exception) as cm:
          spaces = OCIO.Config.IdentifyInterchangeSpace(editableCfg, "Foo", rawCfg, "")
        self.assertEqual(
          str(cm.exception), 
          "Could not find destination color space ''."
        )
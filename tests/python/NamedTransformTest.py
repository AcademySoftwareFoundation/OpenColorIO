# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest

import PyOpenColorIO as OCIO


class NamedTransformTest(unittest.TestCase):
    TEST_NAME = 'namedTransform'
    TEST_FAMILY = 'myFamily'
    TEST_DESCRIPTION = 'used for tests'
    TEST_CATEGORIES = ['good', 'not so good', 'bad']
    TEST_INVALIDS = (OCIO.TRANSFORM_DIR_INVERSE, 42, [1, 2, 3])

    def setUp(self):
        self.named_tr = OCIO.NamedTransform()

    def tearDown(self):
        self.named_tr = None

    def test_name(self):
        """
        Test the setName() and getName() methods.
        """

        # Default initialized name is empty.
        self.assertEqual(self.named_tr.getName(), '')

        self.named_tr.setName(self.TEST_NAME)
        self.assertEqual(self.named_tr.getName(), self.TEST_NAME)

        # Wrong type tests.
        for invalid in self.TEST_INVALIDS:
            with self.assertRaises(TypeError):
                self.named_tr.setName(invalid)

    def test_family(self):
        """
        Test the setFamily() and getFamily() methods.
        """

        # Default initialized family is empty.
        self.assertEqual(self.named_tr.getFamily(), '')

        self.named_tr.setFamily(self.TEST_FAMILY)
        self.assertEqual(self.named_tr.getFamily(), self.TEST_FAMILY)

        # Wrong type tests.
        for invalid in self.TEST_INVALIDS:
            with self.assertRaises(TypeError):
                self.named_tr.setFamily(invalid)

    def test_description(self):
        """
        Test the setDescription() and getDescription() methods.
        """

        # Default initialized description is empty.
        self.assertEqual(self.named_tr.getDescription(), '')

        self.named_tr.setDescription(self.TEST_DESCRIPTION)
        self.assertEqual(self.named_tr.getDescription(), self.TEST_DESCRIPTION)

        # Wrong type tests.
        for invalid in self.TEST_INVALIDS:
            with self.assertRaises(TypeError):
                self.named_tr.setDescription(invalid)

    def test_transform(self):
        """
        Test the setTransform() and getTransform() methods.
        """

        # Default initialized transform are None.
        self.assertEqual(self.named_tr.getTransform(OCIO.TRANSFORM_DIR_FORWARD), None)
        self.assertEqual(self.named_tr.getTransform(OCIO.TRANSFORM_DIR_INVERSE), None)

        offsetTest = [0.1, 0.2, 0.3, 0.4]
        mat_tr = OCIO.MatrixTransform(offset=offsetTest)
        self.named_tr.setTransform(mat_tr, OCIO.TRANSFORM_DIR_FORWARD)
        cur_tr = self.named_tr.getTransform(OCIO.TRANSFORM_DIR_FORWARD)
        self.assertIsInstance(cur_tr, OCIO.MatrixTransform)
        self.assertEqual(cur_tr.getOffset(), offsetTest)
        self.named_tr.setTransform(None, OCIO.TRANSFORM_DIR_FORWARD)
        self.assertEqual(self.named_tr.getTransform(OCIO.TRANSFORM_DIR_FORWARD), None)

        # Wrong type tests.
        for invalid in self.TEST_INVALIDS:
            with self.assertRaises(TypeError):
                self.named_tr.setTransform(invalid, OCIO.TRANSFORM_DIR_FORWARD)


    def test_constructor_with_keywords(self):
        """
        Test NamedTransform constructor with keywords and validate its values.
        """

        offsetTest = [0.1, 0.2, 0.3, 0.4]
        fwd_tr = OCIO.MatrixTransform(offset=offsetTest)
        inv_tr = OCIO.RangeTransform()

        named_tr = OCIO.NamedTransform(
            name = self.TEST_NAME,
            aliases=['alias1', 'alias2'],
            family = self.TEST_FAMILY,
            description = self.TEST_DESCRIPTION,
            forwardTransform = fwd_tr,
            inverseTransform = inv_tr,
            categories = self.TEST_CATEGORIES)

        self.assertEqual(named_tr.getName(), self.TEST_NAME)
        aliases = named_tr.getAliases()
        self.assertEqual(len(aliases), 2)
        self.assertEqual(named_tr.getFamily(), self.TEST_FAMILY)
        self.assertEqual(named_tr.getDescription(), self.TEST_DESCRIPTION)
        cur_tr = named_tr.getTransform(OCIO.TRANSFORM_DIR_FORWARD)
        self.assertIsInstance(cur_tr, OCIO.MatrixTransform)
        cur_tr = named_tr.getTransform(OCIO.TRANSFORM_DIR_INVERSE)
        self.assertIsInstance(cur_tr, OCIO.RangeTransform)
        catIt = named_tr.getCategories()
        cats = [cat for cat in catIt]
        self.assertEqual(cats, self.TEST_CATEGORIES)

        # With keywords not in their proper order.
        named_tr2 = OCIO.NamedTransform(
            categories = self.TEST_CATEGORIES,
            inverseTransform = inv_tr,
            forwardTransform = fwd_tr,
            description = self.TEST_DESCRIPTION,
            name = self.TEST_NAME,
            family = self.TEST_FAMILY)

        self.assertEqual(named_tr2.getName(), self.TEST_NAME)
        aliases = named_tr2.getAliases()
        self.assertEqual(len(aliases), 0)
        self.assertEqual(named_tr2.getFamily(), self.TEST_FAMILY)
        self.assertEqual(named_tr2.getDescription(), self.TEST_DESCRIPTION)
        cur_tr = named_tr2.getTransform(OCIO.TRANSFORM_DIR_FORWARD)
        self.assertIsInstance(cur_tr, OCIO.MatrixTransform)
        cur_tr = named_tr2.getTransform(OCIO.TRANSFORM_DIR_INVERSE)
        self.assertIsInstance(cur_tr, OCIO.RangeTransform)
        catIt = named_tr2.getCategories()
        cats = [cat for cat in catIt]
        self.assertEqual(cats, self.TEST_CATEGORIES)

    def test_constructor_with_positional(self):
        """
        Test NamedTransform constructor without keywords and validate its values.
        """

        fwd_tr = OCIO.MatrixTransform()
        inv_tr = OCIO.RangeTransform()

        named_tr = OCIO.NamedTransform(
            self.TEST_NAME,
            [],
            self.TEST_FAMILY,
            self.TEST_DESCRIPTION,
            fwd_tr,
            inv_tr,
            self.TEST_CATEGORIES)

        self.assertEqual(named_tr.getName(), self.TEST_NAME)
        aliases = named_tr.getAliases()
        self.assertEqual(len(aliases), 0)
        self.assertEqual(named_tr.getFamily(), self.TEST_FAMILY)
        self.assertEqual(named_tr.getDescription(), self.TEST_DESCRIPTION)
        cur_tr = named_tr.getTransform(OCIO.TRANSFORM_DIR_FORWARD)
        self.assertIsInstance(cur_tr, OCIO.MatrixTransform)
        cur_tr = named_tr.getTransform(OCIO.TRANSFORM_DIR_INVERSE)
        self.assertIsInstance(cur_tr, OCIO.RangeTransform)
        catIt = named_tr.getCategories()
        cats = [cat for cat in catIt]
        self.assertEqual(cats, self.TEST_CATEGORIES)

    def test_constructor_wrong_parameter_type(self):
        """
        Test NamedTransform constructor with a wrong parameter type.
        """

        for invalid in self.TEST_INVALIDS:
            with self.assertRaises(TypeError):
                named_tr = OCIO.NamedTransform(invalid)

    def test_processor(self):
        """
        Test creating a processor from named transforms and color spaces.
        """
        SIMPLE_PROFILE = """ocio_profile_version: 2

roles:
  default: raw

file_rules:
  - !<Rule> {name: ColorSpaceNamePathSearch}
  - !<Rule> {name: Default, colorspace: default}

displays:
  sRGB:
    - !<View> {name: Raw, colorspace: raw}

active_displays: []
active_views: []

display_colorspaces:
  - !<ColorSpace>
    name: dcs

colorspaces:
  - !<ColorSpace>
    name: raw

named_transforms:
  - !<NamedTransform>
    name: forward
    transform: !<MatrixTransform> {name: forward, offset: [0.1, 0.2, 0.3, 0.4]}

  - !<NamedTransform>
    name: inverse
    inverse_transform: !<MatrixTransform> {name: inverse, offset: [-0.2, -0.1, -0.1, 0]}

  - !<NamedTransform>
    name: both
    transform: !<MatrixTransform> {name: forward_both, offset: [0.1, 0.2, 0.3, 0.4]}
    inverse_transform: !<MatrixTransform> {name: inverse_both, offset: [-0.2, -0.1, -0.1, 0]}
"""

        # Create a config.
        cfg = OCIO.Config().CreateFromStream(SIMPLE_PROFILE)

        offsetF = [0.1, 0.2, 0.3, 0.4]
        offsetFInv = [x * -1. for x in offsetF]
        offsetI = [-0.2, -0.1, -0.1, 0.]
        offsetIInv = [x * -1. for x in offsetI]

        # Display color space to forward named transform.

        proc = cfg.getProcessor('dcs', 'forward')
        group = proc.createGroupTransform()
        groupTransformsList = list(group)
        self.assertEqual(len(groupTransformsList), 1);
        self.assertIsInstance(groupTransformsList[0], OCIO.MatrixTransform)
        metadata = groupTransformsList[0].getFormatMetadata()
        atts = metadata.getAttributes()
        
        self.assertEqual('forward', next(atts)[1] )
        self.assertEqual(groupTransformsList[0].getOffset(), offsetFInv)

        # Display color space to inverse named transform.

        proc = cfg.getProcessor('dcs', 'inverse')
        group = proc.createGroupTransform()
        groupTransformsList = list(group)
        self.assertEqual(len(groupTransformsList), 1);
        self.assertIsInstance(groupTransformsList[0], OCIO.MatrixTransform)
        metadata = groupTransformsList[0].getFormatMetadata()
        atts = metadata.getAttributes()
        
        self.assertEqual('inverse', next(atts)[1] )
        self.assertEqual(groupTransformsList[0].getOffset(), offsetI)

        # Both named transform to display color space.

        proc = cfg.getProcessor('both', 'dcs')
        group = proc.createGroupTransform()
        groupTransformsList = list(group)
        self.assertEqual(len(groupTransformsList), 1);
        self.assertIsInstance(groupTransformsList[0], OCIO.MatrixTransform)
        metadata = groupTransformsList[0].getFormatMetadata()
        atts = metadata.getAttributes()
        
        self.assertEqual('forward_both', next(atts)[1] )
        self.assertEqual(groupTransformsList[0].getOffset(), offsetF)

        # Forward named transforn to both named transform.

        proc = cfg.getProcessor('forward', 'both')
        group = proc.createGroupTransform()
        groupTransformsList = list(group)
        self.assertEqual(len(groupTransformsList), 2);
        self.assertIsInstance(groupTransformsList[0], OCIO.MatrixTransform)
        metadata = groupTransformsList[0].getFormatMetadata()
        atts = metadata.getAttributes()

        self.assertEqual('forward', next(atts)[1] )
        self.assertEqual(groupTransformsList[0].getOffset(), offsetF)

        self.assertIsInstance(groupTransformsList[1], OCIO.MatrixTransform)
        metadata = groupTransformsList[1].getFormatMetadata()
        atts = metadata.getAttributes()

        self.assertEqual('inverse_both', next(atts)[1] )
        self.assertEqual(groupTransformsList[1].getOffset(), offsetI)

    def test_aliases(self):
        """
        Test NamedTransform aliases.
        """

        named_tr = OCIO.NamedTransform()
        self.assertEqual(named_tr.getName(), '')
        aliases = named_tr.getAliases()
        self.assertEqual(len(aliases), 0)

        named_tr.addAlias('alias1')
        aliases = named_tr.getAliases()
        self.assertEqual(len(aliases), 1)
        self.assertEqual(aliases[0], 'alias1')

        named_tr.addAlias('alias2')
        aliases = named_tr.getAliases()
        self.assertEqual(len(aliases), 2)
        self.assertEqual(aliases[0], 'alias1')
        self.assertEqual(aliases[1], 'alias2')

        # Alias is already there, not added.

        named_tr.addAlias('Alias2')
        aliases = named_tr.getAliases()
        self.assertEqual(len(aliases), 2)
        self.assertEqual(aliases[0], 'alias1')
        self.assertEqual(aliases[1], 'alias2')

        # Name might remove an alias.

        named_tr.setName('name')
        aliases = named_tr.getAliases()
        self.assertEqual(len(aliases), 2)

        named_tr.setName('alias2')
        aliases = named_tr.getAliases()
        self.assertEqual(len(aliases), 1)
        self.assertEqual(aliases[0], 'alias1')

        # Removing an alias.

        named_tr.addAlias('to remove')
        aliases = named_tr.getAliases()
        self.assertEqual(len(aliases), 2)

        named_tr.removeAlias('not found')
        aliases = named_tr.getAliases()
        self.assertEqual(len(aliases), 2)

        named_tr.removeAlias('to REMOVE')
        aliases = named_tr.getAliases()
        self.assertEqual(len(aliases), 1)

        named_tr.clearAliases()
        aliases = named_tr.getAliases()
        self.assertEqual(len(aliases), 0)

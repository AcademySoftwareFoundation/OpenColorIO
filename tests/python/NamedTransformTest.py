# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import copy
import unittest

import PyOpenColorIO as OCIO


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
    aliases: [nt3, ntb]
    transform: !<MatrixTransform> {name: forward_both, offset: [0.1, 0.2, 0.3, 0.4]}
    inverse_transform: !<MatrixTransform> {name: inverse_both, offset: [-0.2, -0.1, -0.1, 0]}
"""


class NamedTransformTest(unittest.TestCase):
    TEST_NAME = 'namedTransform'
    TEST_FAMILY = 'myFamily'
    TEST_DESCRIPTION = 'used for tests'
    TEST_CATEGORIES = ['good', 'not so good', 'bad']
    TEST_INVALIDS = (OCIO.TRANSFORM_DIR_INVERSE, 42, [1, 2, 3])

    OFFSET_FWD = [0.1, 0.2, 0.3, 0.4]
    OFFSET_FWD_INV = [x * -1. for x in OFFSET_FWD]
    OFFSET_INV = [-0.2, -0.1, -0.1, 0.]
    OFFSET_INV_INV = [x * -1. for x in OFFSET_INV]

    def setUp(self):
        self.named_tr = OCIO.NamedTransform()

    def tearDown(self):
        self.named_tr = None

    def test_copy(self):
        """
        Test the deepcopy() method.
        """
        self.named_tr.setName('test name')
        self.named_tr.setFamily('test family')
        self.named_tr.setDescription('test description')
        self.named_tr.setEncoding('test encoding')
        mat = OCIO.MatrixTransform()
        self.named_tr.setTransform(mat, OCIO.TRANSFORM_DIR_FORWARD)
        self.named_tr.setTransform(direction=OCIO.TRANSFORM_DIR_INVERSE, transform=mat)
        self.named_tr.addCategory('cat1')
        self.named_tr.addAlias("alias1")

        other = copy.deepcopy(self.named_tr)
        self.assertFalse(other is self.named_tr)

        self.assertEqual(other.getName(), self.named_tr.getName())
        self.assertEqual(other.getFamily(), self.named_tr.getFamily())
        self.assertEqual(other.getDescription(), self.named_tr.getDescription())
        self.assertEqual(other.getEncoding(), self.named_tr.getEncoding())
        self.assertTrue(other.getTransform(OCIO.TRANSFORM_DIR_FORWARD).equals(self.named_tr.getTransform(OCIO.TRANSFORM_DIR_FORWARD)))
        self.assertTrue(other.getTransform(OCIO.TRANSFORM_DIR_INVERSE).equals(self.named_tr.getTransform(OCIO.TRANSFORM_DIR_INVERSE)))
        self.assertEqual(list(other.getCategories()), list(self.named_tr.getCategories()))
        self.assertEqual(list(other.getAliases()), list(self.named_tr.getAliases()))

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

    def test_static_get_transform(self):
        """
        Test NamedTransform.GetTransform() static method.
        """

        cfg = OCIO.Config.CreateRaw()

        mat_fwd = OCIO.MatrixTransform()
        mat_fwd.setOffset(self.OFFSET_FWD)
        named_tr_fwd = OCIO.NamedTransform()
        named_tr_fwd.setTransform(mat_fwd, OCIO.TRANSFORM_DIR_FORWARD)

        mat_inv = OCIO.MatrixTransform()
        mat_inv.setOffset(self.OFFSET_INV)
        named_tr_inv = OCIO.NamedTransform()
        named_tr_inv.setTransform(mat_inv, OCIO.TRANSFORM_DIR_INVERSE);

        # Forward transform from forward-only named transform
        tf = OCIO.NamedTransform.GetTransform(named_tr_fwd, OCIO.TRANSFORM_DIR_FORWARD)
        self.assertIsNotNone(tf)
        proc = cfg.getProcessor(tf, OCIO.TRANSFORM_DIR_FORWARD)
        group = proc.createGroupTransform()
        self.assertEqual(len(group), 1)
        self.assertIsInstance(group[0], OCIO.MatrixTransform)
        self.assertEqual(group[0].getOffset(), self.OFFSET_FWD)

        # Inverse transform from forward-only named transform
        tf = OCIO.NamedTransform.GetTransform(named_tr_fwd, OCIO.TRANSFORM_DIR_INVERSE)
        self.assertIsNotNone(tf)
        proc = cfg.getProcessor(tf, OCIO.TRANSFORM_DIR_FORWARD)
        group = proc.createGroupTransform()
        self.assertEqual(len(group), 1)
        self.assertIsInstance(group[0], OCIO.MatrixTransform)
        self.assertEqual(group[0].getOffset(), self.OFFSET_FWD_INV)

        # Forward transform from inverse-only named transform
        tf = OCIO.NamedTransform.GetTransform(named_tr_inv, OCIO.TRANSFORM_DIR_FORWARD)
        self.assertIsNotNone(tf)
        proc = cfg.getProcessor(tf, OCIO.TRANSFORM_DIR_FORWARD)
        group = proc.createGroupTransform()
        self.assertEqual(len(group), 1)
        self.assertIsInstance(group[0], OCIO.MatrixTransform)
        self.assertEqual(group[0].getOffset(), self.OFFSET_INV_INV)

        # Inverse transform from inverse-only named transform
        tf = OCIO.NamedTransform.GetTransform(named_tr_inv, OCIO.TRANSFORM_DIR_INVERSE)
        self.assertIsNotNone(tf)
        proc = cfg.getProcessor(tf, OCIO.TRANSFORM_DIR_FORWARD)
        group = proc.createGroupTransform()
        self.assertEqual(len(group), 1)
        self.assertIsInstance(group[0], OCIO.MatrixTransform)
        self.assertEqual(group[0].getOffset(), self.OFFSET_INV)

    def test_processor_from_nt(self):

        cfg = OCIO.Config().CreateFromStream(SIMPLE_PROFILE)
        nt = cfg.getNamedTransform("forward")

        # Named transform from NamedTransform object and forward direction.
        proc = cfg.getProcessor(nt, OCIO.TRANSFORM_DIR_FORWARD)
        group = proc.createGroupTransform()

        self.assertEqual(len(group), 1)
        self.assertIsInstance(group[0], OCIO.MatrixTransform)
        self.assertEqual("forward", next(group[0].getFormatMetadata().getAttributes())[1])
        self.assertEqual(group[0].getOffset(), self.OFFSET_FWD)

        # Named transform from NamedTransform object and inverse direction.
        proc = cfg.getProcessor(nt, OCIO.TRANSFORM_DIR_INVERSE)
        group = proc.createGroupTransform()

        self.assertEqual(len(group), 1)
        self.assertIsInstance(group[0], OCIO.MatrixTransform)
        self.assertEqual("forward", next(group[0].getFormatMetadata().getAttributes())[1])
        self.assertEqual(group[0].getOffset(), self.OFFSET_FWD_INV)

    def test_processor_from_nt_and_context(self):

        cfg = OCIO.Config().CreateFromStream(SIMPLE_PROFILE)
        context = cfg.getCurrentContext()
        nt = cfg.getNamedTransform("inverse")

        # Named transform from NamedTransform object and forward direction with context.
        proc = cfg.getProcessor(context, nt, OCIO.TRANSFORM_DIR_FORWARD)
        group = proc.createGroupTransform()

        self.assertEqual(len(group), 1)
        self.assertIsInstance(group[0], OCIO.MatrixTransform)
        self.assertEqual("inverse", next(group[0].getFormatMetadata().getAttributes())[1])
        self.assertEqual(group[0].getOffset(), self.OFFSET_INV_INV)

        # Named transform from NamedTransform object and inverse direction with context.
        proc = cfg.getProcessor(context, nt, OCIO.TRANSFORM_DIR_INVERSE)
        group = proc.createGroupTransform()

        self.assertEqual(len(group), 1)
        self.assertIsInstance(group[0], OCIO.MatrixTransform)
        self.assertEqual("inverse", next(group[0].getFormatMetadata().getAttributes())[1])
        self.assertEqual(group[0].getOffset(), self.OFFSET_INV)

    def test_processor_from_name(self):

        cfg = OCIO.Config().CreateFromStream(SIMPLE_PROFILE)

        # Named transform from name and forward direction.
        proc = cfg.getProcessor("inverse", OCIO.TRANSFORM_DIR_FORWARD)
        group = proc.createGroupTransform()

        self.assertEqual(len(group), 1)
        self.assertIsInstance(group[0], OCIO.MatrixTransform)
        self.assertEqual("inverse", next(group[0].getFormatMetadata().getAttributes())[1])
        self.assertEqual(group[0].getOffset(), self.OFFSET_INV_INV)

        # Named transform from name and inverse direction.
        proc = cfg.getProcessor("inverse", OCIO.TRANSFORM_DIR_INVERSE)
        group = proc.createGroupTransform()

        self.assertEqual(len(group), 1)
        self.assertIsInstance(group[0], OCIO.MatrixTransform)
        self.assertEqual("inverse", next(group[0].getFormatMetadata().getAttributes())[1])
        self.assertEqual(group[0].getOffset(), self.OFFSET_INV)

    def test_processor_from_name_and_context(self):

        cfg = OCIO.Config().CreateFromStream(SIMPLE_PROFILE)
        context = cfg.getCurrentContext()

        # Named transform from name and forward direction with context.
        proc = cfg.getProcessor(context, "forward", OCIO.TRANSFORM_DIR_FORWARD)
        group = proc.createGroupTransform()

        self.assertEqual(len(group), 1)
        self.assertIsInstance(group[0], OCIO.MatrixTransform)
        self.assertEqual("forward", next(group[0].getFormatMetadata().getAttributes())[1])
        self.assertEqual(group[0].getOffset(), self.OFFSET_FWD)

        # Named transform from name and inverse direction with context.
        proc = cfg.getProcessor(context, "forward", OCIO.TRANSFORM_DIR_INVERSE)
        group = proc.createGroupTransform()

        self.assertEqual(len(group), 1)
        self.assertIsInstance(group[0], OCIO.MatrixTransform)
        self.assertEqual("forward", next(group[0].getFormatMetadata().getAttributes())[1])
        self.assertEqual(group[0].getOffset(), self.OFFSET_FWD_INV)

    def test_processor_from_alias(self):

        cfg = OCIO.Config().CreateFromStream(SIMPLE_PROFILE)

        # Named transform from alias and forward direction.
        proc = cfg.getProcessor("ntb", OCIO.TRANSFORM_DIR_FORWARD)
        group = proc.createGroupTransform()

        self.assertEqual(len(group), 1)
        self.assertIsInstance(group[0], OCIO.MatrixTransform)
        self.assertEqual("forward_both", next(group[0].getFormatMetadata().getAttributes())[1])
        self.assertEqual(group[0].getOffset(), self.OFFSET_FWD)

        # Named transform from alias and inverse direction.
        proc = cfg.getProcessor("nt3", OCIO.TRANSFORM_DIR_INVERSE)
        group = proc.createGroupTransform()

        self.assertEqual(len(group), 1)
        self.assertIsInstance(group[0], OCIO.MatrixTransform)
        self.assertEqual("inverse_both", next(group[0].getFormatMetadata().getAttributes())[1])
        self.assertEqual(group[0].getOffset(), self.OFFSET_INV)

    def test_processor_from_alias_and_context(self):

        cfg = OCIO.Config().CreateFromStream(SIMPLE_PROFILE)

        # Display color space to forward named transform.
        proc = cfg.getProcessor("dcs", "forward")
        group = proc.createGroupTransform()

        self.assertEqual(len(group), 1)
        self.assertIsInstance(group[0], OCIO.MatrixTransform)
        self.assertEqual("forward", next(group[0].getFormatMetadata().getAttributes())[1])
        self.assertEqual(group[0].getOffset(), self.OFFSET_FWD_INV)

        # Display color space to inverse named transform.
        proc = cfg.getProcessor("dcs", "inverse")
        group = proc.createGroupTransform()

        self.assertEqual(len(group), 1)
        self.assertIsInstance(group[0], OCIO.MatrixTransform)
        self.assertEqual("inverse", next(group[0].getFormatMetadata().getAttributes())[1])
        self.assertEqual(group[0].getOffset(), self.OFFSET_INV)

    def test_processor_from_src_dst_name(self):
        
        cfg = OCIO.Config().CreateFromStream(SIMPLE_PROFILE)

        # Both named transform to display color space.
        proc = cfg.getProcessor("both", "dcs")
        group = proc.createGroupTransform()

        self.assertEqual(len(group), 1)
        self.assertIsInstance(group[0], OCIO.MatrixTransform)
        self.assertEqual("forward_both", next(group[0].getFormatMetadata().getAttributes())[1])
        self.assertEqual(group[0].getOffset(), self.OFFSET_FWD)

        # Forward named transforn to both named transform.
        proc = cfg.getProcessor("forward", "both")
        group = proc.createGroupTransform()
        self.assertEqual(len(group), 2)

        self.assertIsInstance(group[0], OCIO.MatrixTransform)
        self.assertEqual("forward", next(group[0].getFormatMetadata().getAttributes())[1])
        self.assertEqual(group[0].getOffset(), self.OFFSET_FWD)

        self.assertIsInstance(group[1], OCIO.MatrixTransform)
        self.assertEqual("inverse_both", next(group[1].getFormatMetadata().getAttributes())[1])
        self.assertEqual(group[1].getOffset(), self.OFFSET_INV)

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

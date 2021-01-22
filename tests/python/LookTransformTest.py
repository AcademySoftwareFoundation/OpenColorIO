# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest

import PyOpenColorIO as OCIO

class LookTransformTest(unittest.TestCase):

    def setUp(self):
        self.look_tr = OCIO.LookTransform('src', 'dst')

    def tearDown(self):
        self.look_tr = None

    def test_constructor(self):
        """
        Test the constructor.
        """
        lt = OCIO.LookTransform()
        self.assertEqual(lt.getSrc(), '')
        self.assertEqual(lt.getDst(), '')
        lt = OCIO.LookTransform('src', 'dst')
        self.assertEqual(lt.getSrc(), 'src')
        self.assertEqual(lt.getDst(), 'dst')
        lt = OCIO.LookTransform('src', 'dst', 'looks')
        self.assertEqual(lt.getLooks(), 'looks')
        lt = OCIO.LookTransform('src', 'dst', 'looks', True)
        self.assertTrue(lt.getSkipColorSpaceConversion())
        lt = OCIO.LookTransform('src', 'dst', 'looks', True, OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(lt.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)

        lt = OCIO.LookTransform(src='src', dst='dst', direction= OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(lt.getSrc(), 'src')
        self.assertEqual(lt.getDst(), 'dst')
        self.assertEqual(lt.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)

        with self.assertRaises(TypeError):
            OCIO.LookTransform(src='src')
        with self.assertRaises(TypeError):
            OCIO.LookTransform(dst='dst')
        with self.assertRaises(OCIO.Exception):
            OCIO.LookTransform(src='', dst='dst')
        with self.assertRaises(OCIO.Exception):
            OCIO.LookTransform(src='src', dst='')

    def test_src(self):
        """
        Test the setSrc() & getSrc() methods.
        """
        self.assertEqual(self.look_tr.getSrc(), 'src')
        self.look_tr.setSrc('foo')
        self.assertEqual(self.look_tr.getSrc(), 'foo')

        for invalid in (None, 1, [0, 0], [0, 0, 0, 0]):
            with self.assertRaises(TypeError):
                self.look_tr.setSrc(invalid)

    def test_dst(self):
        """
        Test the setDst() & getDst() methods.
        """
        self.assertEqual(self.look_tr.getDst(), 'dst')
        self.look_tr.setDst('bar')
        self.assertEqual(self.look_tr.getDst(), 'bar')

        for invalid in (None, 1, [0, 0], [0, 0, 0, 0]):
            with self.assertRaises(TypeError):
                self.look_tr.setDst(invalid)

    def test_looks(self):
        """
        Test the setLooks() & getLooks() methods.
        """
        self.assertEqual(self.look_tr.getLooks(), '')
        self.look_tr.setLooks('foo|bar')
        self.assertEqual(self.look_tr.getLooks(), 'foo|bar')

        for invalid in (None, 1, [0, 0], [0, 0, 0, 0]):
            with self.assertRaises(TypeError):
                self.look_tr.setLooks(invalid)

    def test_skip_conversion(self):
        """
        Test the setSkipColorSpaceConversion() & getSkipColorSpaceConversion() methods.
        """
        self.assertFalse(self.look_tr.getSkipColorSpaceConversion())
        self.look_tr.setSkipColorSpaceConversion(True)
        self.assertTrue(self.look_tr.getSkipColorSpaceConversion())

    def test_direction(self):
        """
        Test the setDirection() and getDirection() methods.
        """

        # Default initialized direction is forward.
        self.assertEqual(self.look_tr.getDirection(),
                         OCIO.TRANSFORM_DIR_FORWARD)

        for direction in OCIO.TransformDirection.__members__.values():
            self.look_tr.setDirection(direction)
            self.assertEqual(self.look_tr.getDirection(), direction)

        # Wrong type tests.
        for invalid in (None, 1, 'test'):
            with self.assertRaises(TypeError):
                self.look_tr.setDirection(invalid)

    def test_validate(self):
        """
        Test the validate() method.
        """

        self.look_tr.validate()

        self.look_tr.setSrc('')
        with self.assertRaises(OCIO.Exception):
            self.look_tr.validate()
        self.look_tr.setSrc('src')

        self.look_tr.setDst('')
        with self.assertRaises(OCIO.Exception):
            self.look_tr.validate()
        self.look_tr.setDst('dst')

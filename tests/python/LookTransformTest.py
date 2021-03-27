# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest

import PyOpenColorIO as OCIO
from TransformsBaseTest import TransformsBaseTest


class LookTransformTest(unittest.TestCase, TransformsBaseTest):

    def setUp(self):
        self.tr = OCIO.LookTransform('src', 'dst')

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
        lt = OCIO.LookTransform('src', 'dst', 'looks',
                                True, OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(lt.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)

        lt = OCIO.LookTransform(src='src', dst='dst',
                                direction=OCIO.TRANSFORM_DIR_INVERSE)
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
        self.assertEqual(self.tr.getSrc(), 'src')
        self.tr.setSrc('foo')
        self.assertEqual(self.tr.getSrc(), 'foo')

        for invalid in (None, 1, [0, 0], [0, 0, 0, 0]):
            with self.assertRaises(TypeError):
                self.tr.setSrc(invalid)

    def test_dst(self):
        """
        Test the setDst() & getDst() methods.
        """
        self.assertEqual(self.tr.getDst(), 'dst')
        self.tr.setDst('bar')
        self.assertEqual(self.tr.getDst(), 'bar')

        for invalid in (None, 1, [0, 0], [0, 0, 0, 0]):
            with self.assertRaises(TypeError):
                self.tr.setDst(invalid)

    def test_looks(self):
        """
        Test the setLooks() & getLooks() methods.
        """
        self.assertEqual(self.tr.getLooks(), '')
        self.tr.setLooks('foo|bar')
        self.assertEqual(self.tr.getLooks(), 'foo|bar')

        for invalid in (None, 1, [0, 0], [0, 0, 0, 0]):
            with self.assertRaises(TypeError):
                self.tr.setLooks(invalid)

    def test_skip_conversion(self):
        """
        Test the setSkipColorSpaceConversion() & getSkipColorSpaceConversion() methods.
        """
        self.assertFalse(self.tr.getSkipColorSpaceConversion())
        self.tr.setSkipColorSpaceConversion(True)
        self.assertTrue(self.tr.getSkipColorSpaceConversion())

    def test_validate(self):
        """
        Test the validate() method.
        """

        self.tr.validate()

        self.tr.setSrc('')
        with self.assertRaises(OCIO.Exception):
            self.tr.validate()
        self.tr.setSrc('src')

        self.tr.setDst('')
        with self.assertRaises(OCIO.Exception):
            self.tr.validate()
        self.tr.setDst('dst')

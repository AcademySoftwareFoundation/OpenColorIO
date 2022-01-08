# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import copy, unittest, os, sys
import PyOpenColorIO as OCIO

class ViewTransformTest(unittest.TestCase):

    def test_default_contructor(self):
        """
        Test the constructor.
        """
        for ref_type in OCIO.ReferenceSpaceType.__members__.values():
            vt = OCIO.ViewTransform(ref_type)
            self.assertEqual(vt.getReferenceSpaceType(), ref_type)
            self.assertEqual(vt.getName(), '')
            self.assertEqual(vt.getFamily(), '')
            self.assertEqual(vt.getDescription(), '')
            self.assertEqual(len(vt.getCategories()), 0)
            for dir in OCIO.ViewTransformDirection.__members__.values():
                self.assertEqual(vt.getTransform(dir), None)

        vt = OCIO.ViewTransform()
        self.assertEqual(vt.getReferenceSpaceType(), OCIO.REFERENCE_SPACE_SCENE)

        vt = OCIO.ViewTransform(referenceSpace=OCIO.REFERENCE_SPACE_DISPLAY)
        self.assertEqual(vt.getReferenceSpaceType(), OCIO.REFERENCE_SPACE_DISPLAY)

        vt = OCIO.ViewTransform(name='test')
        self.assertEqual(vt.getName(), 'test')

        vt = OCIO.ViewTransform(family='family')
        self.assertEqual(vt.getFamily(), 'family')

        vt = OCIO.ViewTransform(description='description')
        self.assertEqual(vt.getDescription(), 'description')

        vt = OCIO.ViewTransform(categories=['cat1', 'cat2'])
        cats = vt.getCategories()
        self.assertEqual(len(cats), 2)
        self.assertEqual(cats[0], 'cat1')
        self.assertEqual(cats[1], 'cat2')

        mat = OCIO.MatrixTransform()
        vt = OCIO.ViewTransform(toReference=mat)
        self.assertTrue(vt.getTransform(OCIO.VIEWTRANSFORM_DIR_TO_REFERENCE).equals(mat))

        vt = OCIO.ViewTransform(fromReference=mat)
        self.assertTrue(vt.getTransform(OCIO.VIEWTRANSFORM_DIR_FROM_REFERENCE).equals(mat))

    def test_copy(self):
        """
        Test the deepcopy() method.
        """
        vt = OCIO.ViewTransform()
        vt.setName('test name')
        vt.setFamily('test family')
        vt.setDescription('test description')
        mat = OCIO.MatrixTransform()
        vt.setTransform(mat, OCIO.VIEWTRANSFORM_DIR_TO_REFERENCE)
        vt.setTransform(direction=OCIO.VIEWTRANSFORM_DIR_FROM_REFERENCE, transform=mat)
        vt.addCategory('cat1')

        other = copy.deepcopy(vt)
        self.assertFalse(other is vt)

        self.assertEqual(other.getName(), vt.getName())
        self.assertEqual(other.getFamily(), vt.getFamily())
        self.assertEqual(other.getDescription(), vt.getDescription())
        self.assertTrue(other.getTransform(OCIO.VIEWTRANSFORM_DIR_TO_REFERENCE).equals(vt.getTransform(OCIO.VIEWTRANSFORM_DIR_TO_REFERENCE)))
        self.assertTrue(other.getTransform(OCIO.VIEWTRANSFORM_DIR_FROM_REFERENCE).equals(vt.getTransform(OCIO.VIEWTRANSFORM_DIR_FROM_REFERENCE)))
        self.assertEqual(list(other.getCategories()), list(vt.getCategories()))

    def test_name(self):
        """
        Test get/setName.
        """
        vt = OCIO.ViewTransform()
        self.assertEqual(vt.getName(), '')

        vt.setName('test name')
        self.assertEqual(vt.getName(), 'test name')

    def test_family(self):
        """
        Test get/setFamily.
        """
        vt = OCIO.ViewTransform()
        self.assertEqual(vt.getFamily(), '')

        vt.setFamily('test family')
        self.assertEqual(vt.getFamily(), 'test family')

    def test_description(self):
        """
        Test get/setDescription.
        """
        vt = OCIO.ViewTransform()
        self.assertEqual(vt.getDescription(), '')

        vt.setDescription('test description')
        self.assertEqual(vt.getDescription(), 'test description')

    def test_transform(self):
        """
        Test get/setTransform.
        """
        vt = OCIO.ViewTransform()
        self.assertEqual(vt.getTransform(OCIO.VIEWTRANSFORM_DIR_TO_REFERENCE), None)
        self.assertEqual(vt.getTransform(OCIO.VIEWTRANSFORM_DIR_FROM_REFERENCE), None)

        mat = OCIO.MatrixTransform()
        vt.setTransform(mat, OCIO.VIEWTRANSFORM_DIR_TO_REFERENCE)
        self.assertTrue(vt.getTransform(OCIO.VIEWTRANSFORM_DIR_TO_REFERENCE).equals(mat))

        vt.setTransform(None, OCIO.VIEWTRANSFORM_DIR_TO_REFERENCE)
        self.assertEqual(vt.getTransform(OCIO.VIEWTRANSFORM_DIR_TO_REFERENCE), None)

        vt.setTransform(direction=OCIO.VIEWTRANSFORM_DIR_FROM_REFERENCE, transform=mat)
        self.assertTrue(vt.getTransform(OCIO.VIEWTRANSFORM_DIR_FROM_REFERENCE).equals(mat))

        vt.setTransform(transform=None, direction=OCIO.VIEWTRANSFORM_DIR_FROM_REFERENCE)
        self.assertEqual(vt.getTransform(OCIO.VIEWTRANSFORM_DIR_FROM_REFERENCE), None)

    def test_categories(self):
        """
        Test hasCategory, addCategory, removeCategory, getCategories, and clearCategories.
        """
        vt = OCIO.ViewTransform()
        self.assertEqual(len(vt.getCategories()), 0)

        vt.addCategory('cat1')
        self.assertEqual(len(vt.getCategories()), 1)
        self.assertEqual(vt.getCategories()[0], 'cat1')
        self.assertTrue(vt.hasCategory('cat1'))
        # Category is not case sensitive.
        self.assertTrue(vt.hasCategory('cAt1'))
        self.assertFalse(vt.hasCategory('cat2'))
        vt.addCategory('cat1')
        self.assertEqual(len(vt.getCategories()), 1)
        vt.addCategory('CAT1')
        self.assertEqual(len(vt.getCategories()), 1)

        vt.addCategory('CAT2')
        cats = vt.getCategories()
        self.assertEqual(len(cats), 2)
        # Original case is preserved.
        self.assertEqual(cats[0], 'cat1')
        self.assertEqual(cats[1], 'CAT2')

        vt.removeCategory('cat3')
        self.assertEqual(len(vt.getCategories()), 2)
        vt.removeCategory('CAT1')
        self.assertEqual(len(vt.getCategories()), 1)

        vt.addCategory('CAT3')
        vt.addCategory('CAT4')
        self.assertEqual(len(vt.getCategories()), 3)

        vt.clearCategories()
        self.assertEqual(len(vt.getCategories()), 0)

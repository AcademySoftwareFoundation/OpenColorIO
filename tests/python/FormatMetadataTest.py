# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest
import os
import sys

import PyOpenColorIO as OCIO

class FormatMetadataTest(unittest.TestCase):

    def test_default(self):
        """
        Test the default FormatMetadata.
        """

        # FormatMetadata can not be constructed, it has to be retrieved.
        # MatrixTransform has a FormatMetadata.
        mat = OCIO.MatrixTransform()
        fmd = mat.getFormatMetadata()

        # Default name is ROOT and has no value.
        self.assertEqual(fmd.getElementName(), 'ROOT')
        self.assertEqual(fmd.getElementValue(), '')

        # No attributes.
        self.assertEqual(len(list(fmd)), 0)

        # No children.
        children = fmd.getChildElements()
        self.assertEqual(len(list(children)), 0)

    def test_children(self):
        """
        Test the FormatMetadata children.
        """

        # Create a hierarchy:
        # ROOT |- C1-V1 |- C4-V4
        #      |        |- C5-V5 |- C6-V6
        #      |- C2
        #      |- C3-V3
        mat = OCIO.MatrixTransform()
        fmd = mat.getFormatMetadata()
        fmd.addChildElement('C1','V1')
        fmd.addChildElement('C2', '')
        fmd.addChildElement('C3','V3')
        fmdc1 = fmd.getChildElements()[0]
        fmdc1.addChildElement('C4','V4')
        fmdc1.addChildElement('C5','V5')
        fmdc5 = fmdc1.getChildElements()[1]
        fmdc5.addChildElement('C6','V6')

        # Access children.
        self.assertEqual(len(list(fmd.getChildElements())), 3)

        children = fmd.getChildElements()
        child1 = next(children)
        self.assertEqual(child1.getElementName(), 'C1')
        self.assertEqual(child1.getElementValue(), 'V1')
        self.assertEqual(len(list(child1.getChildElements())), 2)
        childrenC1 = child1.getChildElements()
        child4 = next(childrenC1)
        self.assertEqual(child4.getElementName(), 'C4')
        self.assertEqual(child4.getElementValue(), 'V4')
        self.assertEqual(len(list(child4.getChildElements())), 0)
        child5 = next(childrenC1)
        self.assertEqual(child5.getElementName(), 'C5')
        self.assertEqual(child5.getElementValue(), 'V5')
        self.assertEqual(len(list(child5.getChildElements())), 1)
        try:
            next(childrenC1)
        except StopIteration:
            pass
        else:
            raise AssertionError("Child1 should not have other children.")

        childrenC5 = child5.getChildElements()
        child6 = next(childrenC5)
        self.assertEqual(child6.getElementName(), 'C6')
        self.assertEqual(child6.getElementValue(), 'V6')
        self.assertEqual(len(list(child6.getChildElements())), 0)
        child2 = next(children)
        self.assertEqual(child2.getElementName(), 'C2')
        self.assertEqual(child2.getElementValue(), '')
        self.assertEqual(len(list(child2.getChildElements())), 0)
        child3 = next(children)
        self.assertEqual(child3.getElementName(), 'C3')
        self.assertEqual(child3.getElementValue(), 'V3')
        self.assertEqual(len(list(child3.getChildElements())), 0)
        try:
            next(children)
        except StopIteration:
            pass
        else:
            raise AssertionError("FormatMetadata should not have other children.")

        # Test representation.
        self.assertEqual(str(fmd), '<ROOT><C1>V1<C4>V4</C4><C5>V5<C6>V6</C6></C5></C1><C2></C2><C3>V3</C3></ROOT>')

        # Add a new leaf.
        child3.addChildElement('C7','V7')
        self.assertEqual(len(list(child3.getChildElements())), 1)

        # Clear removes children and value, but it preserves the name.
        child3.clear()
        self.assertEqual(len(list(child3.getChildElements())), 0)
        self.assertEqual(child3.getElementName(), 'C3')
        self.assertEqual(child3.getElementValue(), '')

        # FormatMetadata must have a name.
        with self.assertRaises(OCIO.Exception):
            child3.addChildElement('','')

        # Children might have the same name.
        child3.addChildElement('same','')
        child3.addChildElement('same','')
        self.assertEqual(len(list(child3.getChildElements())), 2)

        # Test invalid cases. Name and value have to be strings.
        with self.assertRaises(TypeError):
            child3.addChildElement('string', 42)
        with self.assertRaises(TypeError):
            child3.addChildElement(42, 'string')
        with self.assertRaises(TypeError):
            child3.addChildElement('name', None)

    def test_attributes(self):
        """
        Test the FormatMetadata attributes.
        """

        mat = OCIO.MatrixTransform()
        fmd = mat.getFormatMetadata()
        fmd.addChildElement('C1','V1')
        fmdc1 = fmd.getChildElements()[0]

        fmdc1['att1'] = 'val1'
        fmdc1['att2'] = 'val2'
        fmdc1['att3'] = 'val3'

        # Verify attributes names.
        atts = list(fmdc1)
        self.assertEqual(len(atts), 3)
        self.assertEqual(atts[0], 'att1')
        self.assertEqual(atts[1], 'att2')
        self.assertEqual(atts[2], 'att3')

        # Get attributes.
        attsIt = fmdc1.getAttributes()
        att1 = next(attsIt)
        self.assertEqual(att1[0], 'att1')
        self.assertEqual(att1[1], 'val1')
        att2 = next(attsIt)
        self.assertEqual(att2[0], 'att2')
        self.assertEqual(att2[1], 'val2')
        att3 = next(attsIt)
        self.assertEqual(att3[0], 'att3')
        self.assertEqual(att3[1], 'val3')

        try:
            next(attsIt)
        except StopIteration:
            pass
        else:
            raise AssertionError("FormatMetadata should not have other attributes.")

        # Test representation.
        self.assertEqual(str(fmd), '<ROOT><C1 att1="val1" att2="val2" att3="val3">V1</C1></ROOT>')

        # Attribute names are unique. Setting a value to an existing attribute is replacing the
        # existing attribute value.
        fmdc1['att1'] = 'new val1'
        atts = list(fmdc1)
        self.assertEqual(len(atts), 3)
        attsIt = fmdc1.getAttributes()
        att1 = next(attsIt)
        self.assertEqual(att1[0], 'att1')
        self.assertEqual(att1[1], 'new val1')

        # Clear removes attributes.
        fmdc1.clear()
        atts = list(fmdc1)
        self.assertEqual(len(atts), 0)

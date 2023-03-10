# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.


import PyOpenColorIO as OCIO


class TransformsBaseTest(object):

    def test_direction(self):
        """
        Test the setDirection() and getDirection() methods.
        """

        # Default initialized direction is forward.
        self.assertEqual(self.tr.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)

        for direction in OCIO.TransformDirection.__members__.values():
            self.tr.setDirection(direction)
            self.assertEqual(self.tr.getDirection(), direction)

        # Wrong type tests.
        for invalid in (None, 1):
            with self.assertRaises(TypeError):
                self.tr.setDirection(invalid)

    def tearDown(self):
        self.tr = None

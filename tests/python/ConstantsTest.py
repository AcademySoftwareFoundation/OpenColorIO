# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest, os, sys
import PyOpenColorIO as OCIO

class ConstantsTest(unittest.TestCase):

    def test_interface(self):

        # LoggingLevel
        self.assertEqual(OCIO.LOGGING_LEVEL_NONE, "none")
        self.assertEqual(OCIO.LOGGING_LEVEL_WARNING, "warning")
        self.assertEqual(OCIO.LOGGING_LEVEL_INFO, "info")
        self.assertEqual(OCIO.LOGGING_LEVEL_DEBUG, "debug")
        self.assertEqual(OCIO.LOGGING_LEVEL_UNKNOWN, "unknown")

        # TransformDirection
        self.assertEqual(OCIO.TRANSFORM_DIR_UNKNOWN, "unknown")
        self.assertEqual(OCIO.TRANSFORM_DIR_FORWARD, "forward")
        self.assertEqual(OCIO.TRANSFORM_DIR_INVERSE, "inverse")
        self.assertEqual(OCIO.GetInverseTransformDirection(OCIO.TRANSFORM_DIR_UNKNOWN),
            OCIO.TRANSFORM_DIR_UNKNOWN)
        self.assertEqual(OCIO.GetInverseTransformDirection(OCIO.TRANSFORM_DIR_FORWARD),
            OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(OCIO.GetInverseTransformDirection(OCIO.TRANSFORM_DIR_INVERSE),
            OCIO.TRANSFORM_DIR_FORWARD)

        # ColorSpaceDirection
        self.assertEqual(OCIO.COLORSPACE_DIR_UNKNOWN, "unknown")
        self.assertEqual(OCIO.COLORSPACE_DIR_TO_REFERENCE, "to_reference")
        self.assertEqual(OCIO.COLORSPACE_DIR_FROM_REFERENCE, "from_reference")

        # BitDepth
        self.assertEqual(OCIO.BIT_DEPTH_UNKNOWN, "unknown")
        self.assertEqual(OCIO.BIT_DEPTH_UINT8, "8ui")
        self.assertEqual(OCIO.BIT_DEPTH_UINT10, "10ui")
        self.assertEqual(OCIO.BIT_DEPTH_UINT12, "12ui")
        self.assertEqual(OCIO.BIT_DEPTH_UINT14, "14ui")
        self.assertEqual(OCIO.BIT_DEPTH_UINT16, "16ui")
        self.assertEqual(OCIO.BIT_DEPTH_UINT32, "32ui")
        self.assertEqual(OCIO.BIT_DEPTH_F16, "16f")
        self.assertEqual(OCIO.BIT_DEPTH_F32, "32f")

        # Allocation
        self.assertEqual(OCIO.ALLOCATION_UNKNOWN, "unknown")
        self.assertEqual(OCIO.ALLOCATION_UNIFORM, "uniform")
        self.assertEqual(OCIO.ALLOCATION_LG2, "lg2")

        # Interpolation
        self.assertEqual(OCIO.INTERP_UNKNOWN, "unknown")
        self.assertEqual(OCIO.INTERP_NEAREST, "nearest")
        self.assertEqual(OCIO.INTERP_LINEAR, "linear")
        self.assertEqual(OCIO.INTERP_TETRAHEDRAL, "tetrahedral")
        self.assertEqual(OCIO.INTERP_BEST, "best")

        # GpuLanguage
        self.assertEqual(OCIO.GPU_LANGUAGE_UNKNOWN, "unknown")
        self.assertEqual(OCIO.GPU_LANGUAGE_CG, "cg")
        self.assertEqual(OCIO.GPU_LANGUAGE_GLSL_1_0, "glsl_1.0")
        self.assertEqual(OCIO.GPU_LANGUAGE_GLSL_1_3, "glsl_1.3")

        # EnvironmentMode
        self.assertEqual(OCIO.ENV_ENVIRONMENT_UNKNOWN, "unknown")
        self.assertEqual(OCIO.ENV_ENVIRONMENT_LOAD_PREDEFINED, "loadpredefined")
        self.assertEqual(OCIO.ENV_ENVIRONMENT_LOAD_ALL, "loadall")

        # Roles
        self.assertEqual(OCIO.ROLE_DEFAULT, "default")
        self.assertEqual(OCIO.ROLE_REFERENCE, "reference")
        self.assertEqual(OCIO.ROLE_DATA, "data")
        self.assertEqual(OCIO.ROLE_COLOR_PICKING, "color_picking")
        self.assertEqual(OCIO.ROLE_SCENE_LINEAR, "scene_linear")
        self.assertEqual(OCIO.ROLE_COMPOSITING_LOG, "compositing_log")
        self.assertEqual(OCIO.ROLE_COLOR_TIMING, "color_timing")
        self.assertEqual(OCIO.ROLE_TEXTURE_PAINT, "texture_paint")
        self.assertEqual(OCIO.ROLE_MATTE_PAINT, "matte_paint")

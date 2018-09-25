
import unittest, os, sys
import PyOpenColorIO as OCIO

class ConstantsTest(unittest.TestCase):
    
    def test_interface(self):
        
        # LoggingLevel
        self.assertEqual(OCIO.Constants.LOGGING_LEVEL_NONE, "none")
        self.assertEqual(OCIO.Constants.LOGGING_LEVEL_WARNING, "warning")
        self.assertEqual(OCIO.Constants.LOGGING_LEVEL_INFO, "info")
        self.assertEqual(OCIO.Constants.LOGGING_LEVEL_DEBUG, "debug")
        self.assertEqual(OCIO.Constants.LOGGING_LEVEL_UNKNOWN, "unknown")
        
        # TransformDirection
        self.assertEqual(OCIO.Constants.TRANSFORM_DIR_UNKNOWN, "unknown")
        self.assertEqual(OCIO.Constants.TRANSFORM_DIR_FORWARD, "forward")
        self.assertEqual(OCIO.Constants.TRANSFORM_DIR_INVERSE, "inverse")
        self.assertEqual(OCIO.Constants.GetInverseTransformDirection(OCIO.Constants.TRANSFORM_DIR_UNKNOWN),
            OCIO.Constants.TRANSFORM_DIR_UNKNOWN)
        self.assertEqual(OCIO.Constants.GetInverseTransformDirection(OCIO.Constants.TRANSFORM_DIR_FORWARD),
            OCIO.Constants.TRANSFORM_DIR_INVERSE)
        self.assertEqual(OCIO.Constants.GetInverseTransformDirection(OCIO.Constants.TRANSFORM_DIR_INVERSE),
            OCIO.Constants.TRANSFORM_DIR_FORWARD)
        
        # ColorSpaceDirection
        self.assertEqual(OCIO.Constants.COLORSPACE_DIR_UNKNOWN, "unknown")
        self.assertEqual(OCIO.Constants.COLORSPACE_DIR_TO_REFERENCE, "to_reference")
        self.assertEqual(OCIO.Constants.COLORSPACE_DIR_FROM_REFERENCE, "from_reference")
        
        # BitDepth
        self.assertEqual(OCIO.Constants.BIT_DEPTH_UNKNOWN, "unknown")
        self.assertEqual(OCIO.Constants.BIT_DEPTH_UINT8, "8ui")
        self.assertEqual(OCIO.Constants.BIT_DEPTH_UINT10, "10ui")
        self.assertEqual(OCIO.Constants.BIT_DEPTH_UINT12, "12ui")
        self.assertEqual(OCIO.Constants.BIT_DEPTH_UINT14, "14ui")
        self.assertEqual(OCIO.Constants.BIT_DEPTH_UINT16, "16ui")
        self.assertEqual(OCIO.Constants.BIT_DEPTH_UINT32, "32ui")
        self.assertEqual(OCIO.Constants.BIT_DEPTH_F16, "16f")
        self.assertEqual(OCIO.Constants.BIT_DEPTH_F32, "32f")
        
        # Allocation
        self.assertEqual(OCIO.Constants.ALLOCATION_UNKNOWN, "unknown")
        self.assertEqual(OCIO.Constants.ALLOCATION_UNIFORM, "uniform")
        self.assertEqual(OCIO.Constants.ALLOCATION_LG2, "lg2")
        
        # Interpolation
        self.assertEqual(OCIO.Constants.INTERP_UNKNOWN, "unknown")
        self.assertEqual(OCIO.Constants.INTERP_NEAREST, "nearest")
        self.assertEqual(OCIO.Constants.INTERP_LINEAR, "linear")
        self.assertEqual(OCIO.Constants.INTERP_TETRAHEDRAL, "tetrahedral")
        self.assertEqual(OCIO.Constants.INTERP_BEST, "best")
        
        # GpuLanguage
        self.assertEqual(OCIO.Constants.GPU_LANGUAGE_UNKNOWN, "unknown")
        self.assertEqual(OCIO.Constants.GPU_LANGUAGE_CG, "cg")
        self.assertEqual(OCIO.Constants.GPU_LANGUAGE_GLSL_1_0, "glsl_1.0")
        self.assertEqual(OCIO.Constants.GPU_LANGUAGE_GLSL_1_3, "glsl_1.3")
        
        # EnvironmentMode
        self.assertEqual(OCIO.Constants.ENV_ENVIRONMENT_UNKNOWN, "unknown")
        self.assertEqual(OCIO.Constants.ENV_ENVIRONMENT_LOAD_PREDEFINED, "loadpredefined")
        self.assertEqual(OCIO.Constants.ENV_ENVIRONMENT_LOAD_ALL, "loadall")
        
        # Roles
        self.assertEqual(OCIO.Constants.ROLE_DEFAULT, "default")
        self.assertEqual(OCIO.Constants.ROLE_REFERENCE, "reference")
        self.assertEqual(OCIO.Constants.ROLE_DATA, "data")
        self.assertEqual(OCIO.Constants.ROLE_COLOR_PICKING, "color_picking")
        self.assertEqual(OCIO.Constants.ROLE_SCENE_LINEAR, "scene_linear")
        self.assertEqual(OCIO.Constants.ROLE_COMPOSITING_LOG, "compositing_log")
        self.assertEqual(OCIO.Constants.ROLE_COLOR_TIMING, "color_timing")
        self.assertEqual(OCIO.Constants.ROLE_TEXTURE_PAINT, "texture_paint")
        self.assertEqual(OCIO.Constants.ROLE_MATTE_PAINT, "matte_paint")


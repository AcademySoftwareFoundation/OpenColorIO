
import unittest, os, sys
import PyOpenColorIO as OCIO

class GpuShaderDescTest(unittest.TestCase):
    
    def test_interface(self):
        desc = OCIO.GpuShaderDesc()
        desc.setLanguage(OCIO.Constants.GPU_LANGUAGE_GLSL_1_3)
        self.assertEqual(OCIO.Constants.GPU_LANGUAGE_GLSL_1_3, desc.getLanguage())
        desc.setFunctionName("foo123")
        self.assertEqual("foo123", desc.getFunctionName())
        desc.finalize()
        self.assertEqual("glsl_1.3 foo123 ocio outColor $664fefaf8040c8fae4a3b56f2239872d", 
                         desc.getCacheID())


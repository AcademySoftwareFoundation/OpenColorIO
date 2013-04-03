
import unittest, os, sys
sys.path.append(os.path.join(sys.argv[1], "src", "pyglue"))
import PyOpenColorIO as OCIO

class GpuShaderDescTest(unittest.TestCase):
    
    def test_interface(self):
        desc = OCIO.GpuShaderDesc()
        desc.setLanguage(OCIO.Constants.GPU_LANGUAGE_GLSL_1_3)
        self.assertEqual(OCIO.Constants.GPU_LANGUAGE_GLSL_1_3, desc.getLanguage())
        desc.setFunctionName("foo123")
        self.assertEqual("foo123", desc.getFunctionName())
        desc.setLut3DEdgeLen(32)
        self.assertEqual(32, desc.getLut3DEdgeLen())
        self.assertEqual("glsl_1.3 foo123 32", desc.getCacheID())


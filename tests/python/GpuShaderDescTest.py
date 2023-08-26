# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest
import logging

logger = logging.getLogger(__name__)

try:
    import numpy as np
except ImportError:
    logger.warning(
        "NumPy could not be imported. "
        "Test case will lack significant coverage!"
    )
    np = None

import PyOpenColorIO as OCIO

from UnitTestUtils import STRING_TYPES


class GpuShaderDescTest(unittest.TestCase):

    def test_shader_creator_interface(self):
        desc = OCIO.GpuShaderDesc.CreateShaderDesc()
        desc.setLanguage(OCIO.GPU_LANGUAGE_GLSL_1_3)
        self.assertEqual(OCIO.GPU_LANGUAGE_GLSL_1_3, desc.getLanguage())
        desc.setFunctionName("foo123")
        self.assertEqual("foo123", desc.getFunctionName())
        desc.finalize()
        self.assertEqual("glsl_1.3 foo123 ocio outColor 0 6001c324468d497f99aa06d3014798d8",
                         desc.getCacheID())

    def test_uniform(self):
        # Test dynamic exposure and gamma
        config = OCIO.Config()
        tr = OCIO.ExposureContrastTransform(dynamicExposure=True, 
                                            dynamicGamma=True)
        proc = config.getProcessor(tr)
        desc = OCIO.GpuShaderDesc.CreateShaderDesc()
        gpu_proc = proc.getDefaultGPUProcessor()
        gpu_proc.extractGpuShaderInfo(desc)

        uniforms = desc.getUniforms()
        self.assertEqual(len(uniforms), 2)
        self.assertEqual(uniforms[0][0], "ocio_exposure_contrast_exposureVal")
        self.assertEqual(uniforms[0][1].type, OCIO.UNIFORM_DOUBLE)
        self.assertEqual(uniforms[0][1].getDouble(), 0.0)
        self.assertEqual(uniforms[1][0], "ocio_exposure_contrast_gammaVal")
        self.assertEqual(uniforms[1][1].type, OCIO.UNIFORM_DOUBLE)
        self.assertEqual(uniforms[1][1].getDouble(), 1.0)

        # Can dynamically modify uniforms
        dyn_exposure = desc.getDynamicProperty(OCIO.DYNAMIC_PROPERTY_EXPOSURE)
        dyn_exposure.setDouble(2.0)
        self.assertEqual(uniforms[0][1].getDouble(), 2.0)
        dyn_gamma = desc.getDynamicProperty(OCIO.DYNAMIC_PROPERTY_GAMMA)
        dyn_gamma.setDouble(0.5)
        self.assertEqual(uniforms[1][1].getDouble(), 0.5)

        # Uniforms are present in shader src
        text = desc.getShaderText()
        self.assertEqual(text.count("uniform float"), 2)

        # Iterates uniform name and data
        for name, uniform_data in uniforms:
            self.assertIsInstance(name, STRING_TYPES)
            self.assertIsInstance(uniform_data, OCIO.GpuShaderDesc.UniformData)

    def test_vector_uniform(self):
        if not np:
            logger.warning("NumPy not found. Skipping test!")
            return

        # Test dynamic GradingRGBCurve
        a_curve = OCIO.GradingBSplineCurve([1.0, 2.0, 3.0, 4.0])
        rgb_curve = OCIO.GradingRGBCurve(a_curve, a_curve, a_curve, a_curve)
        tr = OCIO.GradingRGBCurveTransform(values=rgb_curve,
                                           dynamic=True)
        config = OCIO.Config()
        proc = config.getProcessor(tr)
        desc = OCIO.GpuShaderDesc.CreateShaderDesc()
        gpu_proc = proc.getDefaultGPUProcessor()
        gpu_proc.extractGpuShaderInfo(desc)

        uniforms = desc.getUniforms()
        self.assertEqual(len(uniforms), 5)

        self.assertEqual(uniforms[0][0], "ocio_grading_rgbcurve_knotsOffsets")
        self.assertEqual(uniforms[0][1].type, OCIO.UNIFORM_VECTOR_INT)
        vector_int = uniforms[0][1].getVectorInt()
        self.assertTrue(isinstance(vector_int, np.ndarray))
        self.assertEqual(vector_int.dtype, np.intc)
        self.assertTrue(np.array_equal(
            vector_int, 
            np.array([0, 2, 2, 2, 4, 2, 6, 2], 
            dtype=np.intc))
        )

        self.assertEqual(uniforms[1][0], "ocio_grading_rgbcurve_knots")
        self.assertEqual(uniforms[1][1].type, OCIO.UNIFORM_VECTOR_FLOAT)
        vector_float = uniforms[1][1].getVectorFloat()
        self.assertTrue(isinstance(vector_float, np.ndarray))
        self.assertEqual(vector_float.dtype, np.float32)
        self.assertTrue(np.array_equal(
            vector_float, 
            np.array([1.0, 3.0, 1.0, 3.0, 1.0, 3.0, 1.0, 3.0], 
            dtype=np.float32))
        )

        # Can dynamically modify uniforms
        b_curve = OCIO.GradingBSplineCurve([5.0, 6.0, 7.0, 8.0])
        dyn_rgb_curve = desc.getDynamicProperty(
            OCIO.DYNAMIC_PROPERTY_GRADING_RGBCURVE
        )
        dyn_rgb_curve.setGradingRGBCurve(
            OCIO.GradingRGBCurve(b_curve, b_curve, b_curve, b_curve)
        )

        self.assertTrue(np.array_equal(
            uniforms[1][1].getVectorFloat(), 
            np.array([5.0, 7.0, 5.0, 7.0, 5.0, 7.0, 5.0, 7.0], 
            dtype=np.float32))
        )

    def test_texture(self):
        # Test addTexture() & getTextures().
        if not np:
            logger.warning("NumPy not found. Skipping test!")
            return

        desc = OCIO.GpuShaderDesc.CreateShaderDesc()
        buf = np.array([0,0.1,0.2,0.3,0.4,0.5]).astype(np.float32)
        desc.addTexture('tex', 'sampler', 2, 3,
                        OCIO.GpuShaderDesc.TEXTURE_RED_CHANNEL,
                        OCIO.GpuShaderDesc.TEXTURE_2D,
                        OCIO.INTERP_DEFAULT, buf)
        desc.addTexture(textureName='tex2', samplerName='sampler2', width=3, height=2,
                        channel=OCIO.GpuShaderDesc.TEXTURE_RED_CHANNEL,
                        dimensions=OCIO.GpuShaderDesc.TEXTURE_2D,
                        interpolation=OCIO.INTERP_DEFAULT, values=buf)
        textures = desc.getTextures()
        self.assertEqual(len(textures), 2)
        t1 = next(textures)
        self.assertEqual(t1.textureName, 'tex')
        self.assertEqual(t1.samplerName, 'sampler')
        self.assertEqual(t1.width, 2)
        self.assertEqual(t1.height, 3)
        self.assertEqual(t1.channel, OCIO.GpuShaderDesc.TEXTURE_RED_CHANNEL)
        self.assertEqual(t1.interpolation, OCIO.INTERP_DEFAULT)
        v1 = t1.getValues()
        self.assertEqual(len(v1), 6)
        self.assertEqual(v1[0], np.float32(0))
        self.assertEqual(v1[1], np.float32(0.1))
        self.assertEqual(v1[2], np.float32(0.2))
        self.assertEqual(v1[3], np.float32(0.3))
        self.assertEqual(v1[4], np.float32(0.4))
        self.assertEqual(v1[5], np.float32(0.5))

        t2 = next(textures)
        self.assertEqual(t2.textureName, 'tex2')
        self.assertEqual(t2.samplerName, 'sampler2')
        self.assertEqual(t2.width, 3)
        self.assertEqual(t2.height, 2)
        self.assertEqual(t2.channel, OCIO.GpuShaderDesc.TEXTURE_RED_CHANNEL)
        self.assertEqual(t2.interpolation, OCIO.INTERP_DEFAULT)
        v2 = t2.getValues()
        self.assertEqual(len(v2), 6)
        self.assertEqual(v2[0], np.float32(0))
        self.assertEqual(v2[1], np.float32(0.1))
        self.assertEqual(v2[2], np.float32(0.2))
        self.assertEqual(v2[3], np.float32(0.3))
        self.assertEqual(v2[4], np.float32(0.4))
        self.assertEqual(v2[5], np.float32(0.5))

    def test_texture_3d(self):
        # Test add3DTexture() & get3DTextures().
        if not np:
            logger.warning("NumPy not found. Skipping test!")
            return

        desc = OCIO.GpuShaderDesc.CreateShaderDesc()
        buf = np.linspace(0, 1, num=8*3).astype(np.float32)
        bufTest1 = buf[3]
        desc.add3DTexture('tex', 'sampler', 2,
                          OCIO.INTERP_DEFAULT, buf)
        buf = np.linspace(0, 1, num=27*3).astype(np.float32)
        bufTest2 = buf[42]
        desc.add3DTexture('tex2', 'sampler2', 3,
                          OCIO.INTERP_DEFAULT, buf)

        textures = desc.get3DTextures()
        self.assertEqual(len(textures), 2)
        t1 = next(textures)
        self.assertEqual(t1.textureName, 'tex')
        self.assertEqual(t1.samplerName, 'sampler')
        self.assertEqual(t1.edgeLen, 2)
        self.assertEqual(t1.interpolation, OCIO.INTERP_DEFAULT)
        v1 = t1.getValues()
        self.assertEqual(len(v1), 3*8)
        self.assertEqual(v1[3], bufTest1)
        t2 = next(textures)
        self.assertEqual(t2.textureName, 'tex2')
        self.assertEqual(t2.samplerName, 'sampler2')
        self.assertEqual(t2.edgeLen, 3)
        self.assertEqual(t2.interpolation, OCIO.INTERP_DEFAULT)
        v2 = t2.getValues()
        self.assertEqual(len(v2), 3*27)
        self.assertEqual(v2[42], bufTest2)

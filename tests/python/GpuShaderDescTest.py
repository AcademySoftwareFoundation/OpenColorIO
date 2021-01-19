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

class GpuShaderDescTest(unittest.TestCase):

    def test_shader_creator_interface(self):
        desc = OCIO.GpuShaderDesc.CreateShaderDesc()
        desc.setLanguage(OCIO.GPU_LANGUAGE_GLSL_1_3)
        self.assertEqual(OCIO.GPU_LANGUAGE_GLSL_1_3, desc.getLanguage())
        desc.setFunctionName("foo123")
        self.assertEqual("foo123", desc.getFunctionName())
        desc.finalize()
        self.assertEqual("glsl_1.3 foo123 ocio outColor 0 $4dd1c89df8002b409e089089ce8f24e7",
                         desc.getCacheID())

    def test_texture(self):
        # Test addTexture() & getTextures().
        if not np:
            logger.warning("NumPy not found. Skipping test!")
            return

        desc = OCIO.GpuShaderDesc.CreateShaderDesc()
        buf = np.array([0,0.1,0.2,0.3,0.4,0.5]).astype(np.float32)
        desc.addTexture('tex', 'sampler', 2, 3,
                        OCIO.GpuShaderCreator.TEXTURE_RED_CHANNEL,
                        OCIO.INTERP_DEFAULT, buf)
        desc.addTexture(textureName='tex2', samplerName='sampler2', width=3, height=2,
                        channel=OCIO.GpuShaderCreator.TEXTURE_RED_CHANNEL,
                        interpolation=OCIO.INTERP_DEFAULT, values=buf)
        textures = desc.getTextures()
        self.assertEqual(len(textures), 2)
        t1 = next(textures)
        self.assertEqual(t1.textureName, 'tex')
        self.assertEqual(t1.samplerName, 'sampler')
        self.assertEqual(t1.width, 2)
        self.assertEqual(t1.height, 3)
        self.assertEqual(t1.channel, OCIO.GpuShaderCreator.TEXTURE_RED_CHANNEL)
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
        self.assertEqual(t2.channel, OCIO.GpuShaderCreator.TEXTURE_RED_CHANNEL)
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
        desc.add3DTexture('tex', 'sampler', 2,
                          OCIO.INTERP_DEFAULT, buf)
        buf = np.linspace(0, 1, num=27*3).astype(np.float32)
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
        self.assertEqual(v1[3], np.float32(3/(3*8 - 1)))
        t2 = next(textures)
        self.assertEqual(t2.textureName, 'tex2')
        self.assertEqual(t2.samplerName, 'sampler2')
        self.assertEqual(t2.edgeLen, 3)
        self.assertEqual(t2.interpolation, OCIO.INTERP_DEFAULT)
        v2 = t2.getValues()
        self.assertEqual(len(v2), 3*27)
        self.assertEqual(v2[42], np.float32(42/(3*27 - 1)))

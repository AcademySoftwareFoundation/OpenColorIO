
GpuShaderDesc
*************

.. class:: GpuShaderDesc


Initialization
==============

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: GpuShaderDescRcPtr OpenColorIO::GpuShaderDesc::CreateLegacyShaderDesc(unsigned edgelen)

         Create the legacy shader description. 

      .. cpp:function:: GpuShaderDescRcPtr OpenColorIO::GpuShaderDesc::CreateShaderDesc()

         Create the default shader description. 

      .. cpp:function:: GpuShaderCreatorRcPtr OpenColorIO::GpuShaderDesc::clone() const override

   .. group-tab:: Python

      .. py:method:: GpuShaderDesc.CreateLegacyShaderDesc(edgeLen: int, language: PyOpenColorIO.GpuLanguage = GpuLanguage.GPU_LANGUAGE_UNKNOWN, functionName: str = 'OCIOMain', pixelName: str = 'outColor', resourcePrefix: str = 'ocio', uid: str = '') -> PyOpenColorIO.GpuShaderDesc


Getters/Setters
===============

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: unsigned OpenColorIO::GpuShaderDesc::getNumUniforms() const noexcept = 0

      .. cpp:function:: void OpenColorIO::GpuShaderDesc::getUniform(unsigned index, const char *&name, DynamicPropertyRcPtr &value) const = 0

      .. cpp:function:: unsigned OpenColorIO::GpuShaderDesc::getNumTextures() const noexcept = 0

      .. cpp:function:: void OpenColorIO::GpuShaderDesc::getTexture(unsigned index, const char *&textureName, const char *&samplerName, const char *&uid, unsigned &width, unsigned &height, TextureType &channel, Interpolation &interpolation) const = 0

      .. cpp:function:: void OpenColorIO::GpuShaderDesc::getTextureValues(unsigned index, const float *&values) const = 0

      .. cpp:function:: unsigned OpenColorIO::GpuShaderDesc::getNum3DTextures() const noexcept = 0

      .. cpp:function:: void OpenColorIO::GpuShaderDesc::get3DTexture(unsigned index, const char *&textureName, const char *&samplerName, const char *&uid, unsigned &edgelen, Interpolation &interpolation) const = 0

      .. cpp:function:: void OpenColorIO::GpuShaderDesc::get3DTextureValues(unsigned index, const float *&values) const = 0

      .. cpp:function:: const char *OpenColorIO::GpuShaderDesc::getShaderText() const noexcept

         Get the complete OCIO shader program. 

   .. group-tab:: C++

      .. py:method:: GpuShaderDesc.addUniform(self: PyOpenColorIO.GpuShaderDesc, name: str, value: PyOpenColorIO.DynamicProperty) -> bool

      .. py:method:: GpuShaderDesc.getUniforms(self: PyOpenColorIO.GpuShaderDesc) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::GpuShaderDesc>, 0>

      .. py:method:: GpuShaderDesc.getNumTextures(self: PyOpenColorIO.GpuShaderDesc) -> int

      .. py:method:: GpuShaderDesc.addTexture(self: PyOpenColorIO.GpuShaderDesc, textureName: str, samplerName: str, uid: str, width: int, height: int, channel: PyOpenColorIO.GpuShaderCreator.TextureType, interpolation: PyOpenColorIO.Interpolation, values: buffer) -> None

      .. py:method:: GpuShaderDesc.getTextures(self: PyOpenColorIO.GpuShaderDesc) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::GpuShaderDesc>, 1>

      .. py:method:: GpuShaderDesc.getTextureValues(self: PyOpenColorIO.GpuShaderDesc, index: int) -> array

      .. py:method:: GpuShaderDesc.getNum3DTextures(self: PyOpenColorIO.GpuShaderDesc) -> int

      .. py:method:: GpuShaderDesc.add3DTexture(self: PyOpenColorIO.GpuShaderDesc, textureName: str, samplerName: str, uid: str, edgeLen: int, interpolation: PyOpenColorIO.Interpolation, values: buffer) -> None

      .. py:method:: GpuShaderDesc.get3DTextureValues(self: PyOpenColorIO.GpuShaderDesc, index: int) -> array

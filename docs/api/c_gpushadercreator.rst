..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

GpuShaderCreator
****************

.. CAUTION::
   API Docs are a work in progress, expect them to improve over time.

**class GpuShaderCreator**

Inherit from the class to fully customize the implementation of a
GPU shader program from a color transformation.

When no customizations are needed then the
:cpp:class:`GpuShaderDesc` is a better choice.

Subclassed by OpenColorIO::GpuShaderDesc


.. tabs::

   .. group-tab:: C++

      .. cpp:enum:: TextureType 

         *Values:*

         .. cpp:enumerator:: TEXTURE_RED_CHANNEL 

            Only use the red channel of the texture.

         .. cpp:enumerator:: TEXTURE_RGB_CHANNEL 

      **Public Functions**

      .. cpp:function:: GpuShaderCreatorRcPtr clone() const = 0

      .. cpp:function:: const char *getUniqueID() const noexcept

      .. cpp:function:: void setUniqueID(const char *uid) noexcept

      .. cpp:function:: GpuLanguage getLanguage() const noexcept

      .. cpp:function:: void setLanguage(GpuLanguage lang) noexcept

         Set the shader program language.

      .. cpp:function:: const char *getFunctionName() const noexcept

      .. cpp:function:: void setFunctionName(const char *name) noexcept

      .. cpp:function:: const char *getPixelName() const noexcept

      .. cpp:function:: void setPixelName(const char *name) noexcept

         Set the pixel name variable holding the color values.

      .. cpp:function:: const char *getResourcePrefix() const noexcept

         **Note**
         Some applications require that textures, uniforms, and helper
         methods be uniquely named because several processor instances
         could coexist.

      .. cpp:function:: void setResourcePrefix(const char *prefix) noexcept

         Set a prefix to the resource name.

      .. cpp:function:: const char *getCacheID() const noexcept

      .. cpp:function:: void begin(const char *uid)

         Start to collect the shader data.

      .. cpp:function:: void end()

         End to collect the shader data.

      .. cpp:function:: void setTextureMaxWidth(unsigned maxWidth) = 0

         Some graphic cards could have 1D & 2D textures with size
         limitations.

      .. cpp:function:: unsigned getTextureMaxWidth() const noexcept = 0

      .. cpp:function:: unsigned getNextResourceIndex() noexcept

         To avoid texture/unform name clashes always append an increasing
         number to the resource name.

      .. cpp:function:: bool addUniform(const char *name, const DynamicPropertyRcPtr &value) = 0

      .. cpp:function:: void addTexture(const char *textureName, const char *samplerName, const char *uid, unsigned width, unsigned height, TextureType channel, Interpolation interpolation, const float *values) = 0

      .. cpp:function:: void add3DTexture(const char *textureName, const char *samplerName, const char *uid, unsigned edgelen, Interpolation interpolation, const float *values) = 0

      .. cpp:function:: void addToDeclareShaderCode(const char *shaderCode)

      .. cpp:function:: void addToHelperShaderCode(const char *shaderCode)

      .. cpp:function:: void addToFunctionHeaderShaderCode(const char *shaderCode)

      .. cpp:function:: void addToFunctionShaderCode(const char *shaderCode)

      .. cpp:function:: void addToFunctionFooterShaderCode(const char *shaderCode)

      .. cpp:function:: void createShaderText(const char *shaderDeclarations, const char *shaderHelperMethods, const char *shaderFunctionHeader, const char *shaderFunctionBody, const char *shaderFunctionFooter)

         Create the OCIO shader program.

         The OCIO shader program is decomposed to allow a specific
         implementation to change some parts. Some product integrations
         add the color processing within a client shader program,
         imposing constraints requiring this flexibility.

         **Note**

      .. cpp:function:: void finalize()

      .. cpp:function:: ~GpuShaderCreator()


   .. group-tab:: Python

      .. py:class:: TextureType

         Members:

         .. py:const:: TEXTURE_RED_CHANNEL = TextureType.TEXTURE_RED_CHANNEL

         .. py:const:: TEXTURE_RGB_CHANNEL = TextureType.TEXTURE_RGB_CHANNEL

      .. py:method:: add3DTexture(self: PyOpenColorIO.GpuShaderCreator, textureName: str, samplerName: str, uid: str, edgeLen: int, interpolation: PyOpenColorIO.Interpolation, values: float) -> None

      .. py:method:: addTexture(self: PyOpenColorIO.GpuShaderCreator, textureName: str, samplerName: str, uid: str, width: int, height: int, channel: OpenColorIO_v2_0dev::GpuShaderCreator::TextureType, interpolation: PyOpenColorIO.Interpolation, values: float) -> None

      .. py:method:: addToDeclareShaderCode(self: PyOpenColorIO.GpuShaderCreator, shaderCode: str) -> None

      .. py:method:: addToFunctionFooterShaderCode(self: PyOpenColorIO.GpuShaderCreator, shaderCode: str) -> None

      .. py:method:: addToFunctionHeaderShaderCode(self: PyOpenColorIO.GpuShaderCreator, shaderCode: str) -> None

      .. py:method:: addToFunctionShaderCode(self: PyOpenColorIO.GpuShaderCreator, shaderCode: str) -> None

      .. py:method:: addToHelperShaderCode(self: PyOpenColorIO.GpuShaderCreator, shaderCode: str) -> None

      .. py:method:: addUniform(self: PyOpenColorIO.GpuShaderCreator, name: str, value: PyOpenColorIO.DynamicProperty) -> bool

      .. py:method:: begin(self: PyOpenColorIO.GpuShaderCreator, uid: str) -> None

      .. py:method:: clone(self: PyOpenColorIO.GpuShaderCreator) -> `PyOpenColorIO.GpuShaderCreator`_

      .. py:method:: createShaderText(self: PyOpenColorIO.GpuShaderCreator, shaderDeclarations: str, shaderHelperMethods: str, shaderFunctionHeader: str, shaderFunctionBody: str, shaderFunctionFooter: str) -> None

      .. py:method:: end(self: PyOpenColorIO.GpuShaderCreator) -> None

      .. py:method:: finalize(self: PyOpenColorIO.GpuShaderCreator) -> None

      .. py:method:: getCacheID(self: PyOpenColorIO.GpuShaderCreator) -> str

      .. py:method:: getFunctionName(self: PyOpenColorIO.GpuShaderCreator) -> str

      .. py:method:: getLanguage(self: PyOpenColorIO.GpuShaderCreator) -> str

      .. py:method:: getNextResourceIndex(self: PyOpenColorIO.GpuShaderCreator) -> int

      .. py:method:: getPixelName(self: PyOpenColorIO.GpuShaderCreator) -> str

      .. py:method:: getResourcePrefix(self: PyOpenColorIO.GpuShaderCreator) -> str

      .. py:method:: getTextureMaxWidth(self: PyOpenColorIO.GpuShaderCreator) -> int

      .. py:method:: getUniqueID(self: PyOpenColorIO.GpuShaderCreator) -> str

      .. py:method:: setFunctionName(self: PyOpenColorIO.GpuShaderCreator, name: str) -> None

      .. py:method:: setLanguage(self: PyOpenColorIO.GpuShaderCreator, language: str) -> None

      .. py:method:: setPixelName(self: PyOpenColorIO.GpuShaderCreator, name: str) -> None

      .. py:method:: setResourcePrefix(self: PyOpenColorIO.GpuShaderCreator, prefix: str) -> None

      .. py:method:: setTextureMaxWidth(self: PyOpenColorIO.GpuShaderCreator, maxWidth: int) -> None

      .. py:method:: setUniqueID(self: PyOpenColorIO.GpuShaderCreator, uid: str) -> None


GpuShaderCreator
****************

.. class:: GpuShaderCreator

   Inherit from the class to fully customize the implementation of a GPU shader program from a color transformation.

   When no customizations are needed then the :cpp:class:```GpuShaderDesc <c_gpushaderdesc.rst#classOpenColorIO_1_1GpuShaderDesc>`_`` is a better choice. 

   Subclassed by `OpenColorIO::GpuShaderDesc`_


Initialization
==============

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: OpenColorIO::GpuShaderCreator::GpuShaderCreator()

      .. cpp:function:: GpuShaderCreatorRcPtr OpenColorIO::GpuShaderCreator::clone() const = 0



Getters/Setters
===============

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: const char *OpenColorIO::GpuShaderCreator::getUniqueID() const noexcept

      .. cpp:function:: void OpenColorIO::GpuShaderCreator::setUniqueID(const char *uid) noexcept

      .. cpp:function:: GpuLanguage OpenColorIO::GpuShaderCreator::getLanguage() const noexcept

      .. cpp:function:: void OpenColorIO::GpuShaderCreator::setLanguage(GpuLanguage lang) noexcept

         Set the shader program language. 

      .. cpp:function:: const char *OpenColorIO::GpuShaderCreator::getFunctionName() const noexcept

      .. cpp:function:: void OpenColorIO::GpuShaderCreator::setFunctionName(const char *name) noexcept

      .. cpp:function:: const char *OpenColorIO::GpuShaderCreator::getPixelName() const noexcept

      .. cpp:function:: void OpenColorIO::GpuShaderCreator::setPixelName(const char *name) noexcept

         Set the pixel name variable holding the color values. 

      .. cpp:function:: const char *OpenColorIO::GpuShaderCreator::getResourcePrefix() const noexcept

         **Note**
            Some applications require that textures, uniforms, and helper methods be uniquely named because several processor instances could coexist. 

      .. cpp:function:: void OpenColorIO::GpuShaderCreator::setResourcePrefix(const char *prefix) noexcept

         Set a prefix to the resource name. 

      .. cpp:function:: const char *OpenColorIO::GpuShaderCreator::getCacheID() const noexcept

      .. cpp:function:: void OpenColorIO::GpuShaderCreator::setTextureMaxWidth(unsigned maxWidth) = 0

         Some graphic cards could have 1D & 2D textures with size limitations. 

      .. cpp:function:: unsigned OpenColorIO::GpuShaderCreator::getTextureMaxWidth() const noexcept = 0

      Warning: doxygenfunction: Cannot find function “OpenColorIO::GpuShaderCreator::getResourceIndex” in doxygen xml output for project “OpenColorIO” from directory: ./_doxygen/xml

      .. cpp:function:: void OpenColorIO::GpuShaderCreator::begin(const char *uid)

         Start to collect the shader data. 

      .. cpp:function:: void OpenColorIO::GpuShaderCreator::end()

         End to collect the shader data. 



Modify Shader Code
==================

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: bool OpenColorIO::GpuShaderCreator::addUniform(const char *name, const DynamicPropertyRcPtr &value) = 0

      .. cpp:function:: void OpenColorIO::GpuShaderCreator::addTexture(const char *textureName, const char *samplerName, const char *uid, unsigned width, unsigned height, TextureType channel, Interpolation interpolation, const float *values) = 0

      .. cpp:function:: void OpenColorIO::GpuShaderCreator::add3DTexture(const char *textureName, const char *samplerName, const char *uid, unsigned edgelen, Interpolation interpolation, const float *values) = 0

      .. cpp:function:: void OpenColorIO::GpuShaderCreator::addToDeclareShaderCode(const char *shaderCode)

      .. cpp:function:: void OpenColorIO::GpuShaderCreator::addToHelperShaderCode(const char *shaderCode)

      .. cpp:function:: void OpenColorIO::GpuShaderCreator::addToFunctionHeaderShaderCode(const char *shaderCode)

      .. cpp:function:: void OpenColorIO::GpuShaderCreator::addToFunctionHeaderShaderCode(const char *shaderCode)

      .. cpp:function:: void OpenColorIO::GpuShaderCreator::addToFunctionFooterShaderCode(const char *shaderCode)

      .. cpp:enum:: enum OpenColorIO::GpuShaderCreator::TextureType

         *Values:*

         enumerator `TEXTURE_RED_CHANNEL`

            Only use the red channel of the texture. 

         enumerator `TEXTURE_RGB_CHANNEL`

   .. group-tab:: Python

      .. py:method:: GpuShaderCreator.addUniform(self: PyOpenColorIO.GpuShaderCreator, name: str, value: PyOpenColorIO.DynamicProperty) -> bool

      .. py:method:: GpuShaderCreator.addTexture(self: PyOpenColorIO.GpuShaderCreator, textureName: str, samplerName: str, uid: str, width: int, height: int, channel: OpenColorIO_v2_0dev::GpuShaderCreator::TextureType, interpolation: PyOpenColorIO.Interpolation, values: float) -> None

      .. py:method:: GpuShaderCreator.add3DTexture(self: PyOpenColorIO.GpuShaderCreator, textureName: str, samplerName: str, uid: str, edgeLen: int, interpolation: PyOpenColorIO.Interpolation, values: float) -> None


Execute
=======

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: void OpenColorIO::GpuShaderCreator::createShaderText(const char *shaderDeclarations, const char *shaderHelperMethods, const char *shaderFunctionHeader, const char *shaderFunctionBody, const char *shaderFunctionFooter)

         Create the OCIO shader program. 

         The OCIO shader program is decomposed to allow a specific implementation to change some parts. Some product integrations add the color processing within a client shader program, imposing constraints requiring this flexibility. 

         **Note**
      .. cpp:function:: void OpenColorIO::GpuShaderCreator::finalize()

..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

GpuShaderDesc
*************

.. CAUTION::
API Docs are a work in progress, expect them to improve over time.

TODO: Move to .rst
!rst::
GpuShaderDesc
*************
This class holds the GPU-related information needed to build a shader program
from a specific processor.

This class defines the interface and there are two implementations provided.
The "legacy" mode implements the OCIO v1 approach of baking certain ops
in order to have at most one 3D-LUT.  The "generic" mode is the v2 default and
allows all the ops to be processed as-is, without baking, like the CPU renderer.
Custom implementations could be written to accommodate the GPU needs of a
specific client app.


The complete fragment shader program is decomposed in two main parts:
the OCIO shader program for the color processing and the client shader
program which consumes the pixel color processing.

The OCIO shader program is fully described by the GpuShaderDesc
independently from the client shader program. The only critical
point is the agreement on the OCIO function shader name.

To summarize, the complete shader program is:

.. code-block:: cpp

   ////////////////////////////////////////////////////////////////////////
   //                                                                    //
   //               The complete fragment shader program                 //
   //                                                                    //
   ////////////////////////////////////////////////////////////////////////
   //                                                                    //
   //   //////////////////////////////////////////////////////////////   //
   //   //                                                          //   //
   //   //               The OCIO shader program                    //   //
   //   //                                                          //   //
   //   //////////////////////////////////////////////////////////////   //
   //   //                                                          //   //
   //   //   // All global declarations                             //   //
   //   //   uniform sampled3D tex3;                                //   //
   //   //                                                          //   //
   //   //   // All helper methods                                  //   //
   //   //   vec3 computePos(vec3 color)                            //   //
   //   //   {                                                      //   //
   //   //      vec3 coords = color;                                //   //
   //   //      ...                                                 //   //
   //   //      return coords;                                      //   //
   //   //   }                                                      //   //
   //   //                                                          //   //
   //   //   // The OCIO shader function                            //   //
   //   //   vec4 OCIODisplay(in vec4 inColor)                      //   //
   //   //   {                                                      //   //
   //   //      vec4 outColor = inColor;                            //   //
   //   //      ...                                                 //   //
   //   //      outColor.rbg                                        //   //
   //   //         = texture3D(tex3, computePos(inColor.rgb)).rgb;  //   //
   //   //      ...                                                 //   //
   //   //      return outColor;                                    //   //
   //   //   }                                                      //   //
   //   //                                                          //   //
   //   //////////////////////////////////////////////////////////////   //
   //                                                                    //
   //   //////////////////////////////////////////////////////////////   //
   //   //                                                          //   //
   //   //             The client shader program                    //   //
   //   //                                                          //   //
   //   //////////////////////////////////////////////////////////////   //
   //   //                                                          //   //
   //   //   uniform sampler2D image;                               //   //
   //   //                                                          //   //
   //   //   void main()                                            //   //
   //   //   {                                                      //   //
   //   //      vec4 inColor = texture2D(image, gl_TexCoord[0].st); //   //
   //   //      ...                                                 //   //
   //   //      vec4 outColor = OCIODisplay(inColor);               //   //
   //   //      ...                                                 //   //
   //   //      gl_FragColor = outColor;                            //   //
   //   //   }                                                      //   //
   //   //                                                          //   //
   //   //////////////////////////////////////////////////////////////   //
   //                                                                    //
   ////////////////////////////////////////////////////////////////////////


**Usage Example:** *Building a GPU shader*

This example is based on the code in: src/apps/ociodisplay/main.cpp

.. code-block:: cpp

   // Get the processor
   //
   OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromEnv();
   OCIO::ConstProcessorRcPtr processor
      = config->getProcessor("ACES - ACEScg", "Output - sRGB");

   // Step 1: Create a GPU shader description
   //
   // The three potential scenarios are:
   //
   //   1. Instantiate the legacy shader description.  The color processor
   //      is baked down to contain at most one 3D LUT and no 1D LUTs.
   //
   //      This is the v1 behavior and will remain part of OCIO v2
   //      for backward compatibility.
   //
   OCIO::GpuShaderDescRcPtr shaderDesc
         = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);
   //
   //   2. Instantiate the generic shader description.  The color processor
   //      is used as-is (i.e. without any baking step) and could contain
   //      any number of 1D & 3D luts.
   //
   //      This is the default OCIO v2 behavior and allows a much better
   //      match between the CPU and GPU renderers.
   //
   OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::Create();
   //
   //   3. Instantiate a custom shader description.
   //
   //      Writing a custom shader description is a way to tailor the shaders
   //      to the needs of a given client program.  This involves writing a
   //      new class inheriting from the pure virtual class GpuShaderDesc.
   //
   //      Please refer to the GenericGpuShaderDesc class for an example.
   //
   OCIO::GpuShaderDescRcPtr shaderDesc = MyCustomGpuShader::Create();

   shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_3);
   shaderDesc->setFunctionName("OCIODisplay");

   // Step 2: Collect the shader program information for a specific processor
   //
   processor->extractGpuShaderInfo(shaderDesc);

   // Step 3: Create a helper to build the shader. Here we use a helper for
   //         OpenGL but there will also be helpers for other languages.
   //
   OpenGLBuilderRcPtr oglBuilder = OpenGLBuilder::Create(shaderDesc);

   // Step 4: Allocate & upload all the LUTs
   //
   oglBuilder->allocateAllTextures();

   // Step 5: Build the complete fragment shader program using
   //         g_fragShaderText which is the client shader program.
   //
   g_programId = oglBuilder->buildProgram(g_fragShaderText);

   // Step 6: Enable the fragment shader program, and all needed textures
   //
   glUseProgram(g_programId);
   glUniform1i(glGetUniformLocation(g_programId, "tex1"), 1);  // image texture
   oglBuilder->useAllTextures(g_programId);                    // LUT textures


**class GpuShaderDesc : public OpenColorIO::`GpuShaderCreator`_**


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: GpuShaderCreatorRcPtr clone() const override

      .. cpp:function:: unsigned getNumUniforms() const noexcept = 0

      .. cpp:function:: void getUniform(unsigned index, const char *&name, DynamicPropertyRcPtr &value) const = 0

      .. cpp:function:: unsigned getNumTextures() const noexcept = 0

      .. cpp:function:: void getTexture(unsigned index, const char *&textureName, const char *&samplerName, const char *&uid, unsigned &width, unsigned &height, TextureType &channel, Interpolation &interpolation) const = 0

      .. cpp:function:: void getTextureValues(unsigned index, const float *&values) const = 0

      .. cpp:function:: unsigned getNum3DTextures() const noexcept = 0

      .. cpp:function:: void get3DTexture(unsigned index, const char *&textureName, const char *&samplerName, const char *&uid, unsigned &edgelen, Interpolation &interpolation) const = 0

      .. cpp:function:: void get3DTextureValues(unsigned index, const float *&values) const = 0**

      .. cpp:function:: const char *getShaderText() const noexcept

         Get the complete OCIO shader program.

      .. cpp:function:: ~GpuShaderDesc()

      -[ Public Static Functions ]-

      .. cpp:function:: GpuShaderDescRcPtr CreateLegacyShaderDesc(unsigned edgelen)

         Create the legacy shader description.

      .. cpp:function:: GpuShaderDescRcPtr CreateShaderDesc()

         Create the default shader description.

   .. group-tab:: Python

      .. py:class:: PyOpenColorIO.GpuShaderDesc

      .. py:function:: CreateLegacyShaderDesc(edgeLen: int, language: PyOpenColorIO.GpuLanguage = GpuLanguage.GPU_LANGUAGE_UNKNOWN, functionName: str = 'OCIOMain', pixelName: str = 'outColor', resourcePrefix: str = 'ocio', uid: str = '') -> `PyOpenColorIO.GpuShaderDesc`_

      .. py:function:: CreateShaderDesc(language: PyOpenColorIO.GpuLanguage = GpuLanguage.GPU_LANGUAGE_UNKNOWN, functionName: str = 'OCIOMain', pixelName: str = 'outColor', resourcePrefix: str = 'ocio', uid: str = '') -> `PyOpenColorIO.GpuShaderDesc`_

      .. py:class:: Texture

         **property channel**

         **property height**

         **property interpolation**

         **property samplerName**

         **property textureName**

         **property uid**

         **property width**

      .. py:class:: Texture3D

         **property edgeLen**

         **property interpolation**

         **property samplerName**

         **property textureName**

         **property uid**

      .. py:class:: Texture3DIterator

      .. py:class:: TextureIterator

      .. py:class:: UniformIterator

      .. py:method:: add3DTexture(self: PyOpenColorIO.GpuShaderDesc, textureName: str, samplerName: str, uid: str, edgeLen: int, interpolation: PyOpenColorIO.Interpolation, values: buffer) -> None

      .. py:method:: addTexture(self: PyOpenColorIO.GpuShaderDesc, textureName: str, samplerName: str, uid: str, width: int, height: int, channel: PyOpenColorIO.GpuShaderCreator.TextureType, interpolation: PyOpenColorIO.Interpolation, values: buffer) -> None

      .. py:method:: addUniform(self: PyOpenColorIO.GpuShaderDesc, name: str, value: PyOpenColorIO.DynamicProperty) -> bool

      .. py:method:: get3DTextureValues(self: PyOpenColorIO.GpuShaderDesc, index: int) -> array

      .. py:method:: get3DTextures(self: PyOpenColorIO.GpuShaderDesc) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::GpuShaderDesc>, 2>

      .. py:method:: getNum3DTextures(self: PyOpenColorIO.GpuShaderDesc) -> int**

      .. py:method:: getNumTextures(self: PyOpenColorIO.GpuShaderDesc) -> int**

      .. py:method:: getTextureValues(self: PyOpenColorIO.GpuShaderDesc, index: int) -> array

      .. py:method:: getTextures(self: PyOpenColorIO.GpuShaderDesc) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::GpuShaderDesc>, 1>

      .. py:method:: getUniforms(self: PyOpenColorIO.GpuShaderDesc) -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::GpuShaderDesc>, 0>

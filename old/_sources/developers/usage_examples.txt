.. _developers-usageexamples:

Usage Examples
==============

Some examples of using the OCIO API, via both C++ and the Python bindings.

For further examples, see the ``src/apps/`` directory in the git repository

.. _usage_applybasic:

Applying a basic ColorSpace transform, using the CPU
****************************************************
This describes what code is used to convert from a specified source
:cpp:class:`ColorSpace` to a specified destination :cpp:class:`ColorSpace`.
If you are using the OCIO Nuke plugins, the OCIOColorSpace node performs these
steps internally.

#. **Get the Config.**
   This represents the entirety of the current color "universe". It can either
   be initialized by your app at startup or created explicitly. In common
   usage, you can just query :cpp:func:`GetCurrentConfig`, which will auto
   initialize on first use using the :envvar:`OCIO` environment variable.

#. **Get Processor from the Config.**
   A processor corresponds to a 'baked' color transformation. You specify two
   arguments when querying a processor: the :ref:`colorspace_section` you are
   coming from, and the :ref:`colorspace_section` you are going to.
   :ref:`cfgcolorspaces_section` ColorSpaces can be either explicitly named
   strings (defined by the current configuration) or can be
   :ref:`cfgroles_section` (essentially :ref:`colorspace_section` aliases)
   which are consistent across configurations. Constructing a
   :cpp:class:`Processor` object is likely a blocking operation (thread-wise)
   so care should be taken to do this as infrequently as is sensible. Once per
   render 'setup' would be appropriate, once per scanline would be
   inappropriate.

#. **Convert your image, using the Processor.**
   Once you have the processor, you can apply the color transformation using
   the "apply" function.  In :ref:`usage_applybasic_cpp`, you apply the
   processing in-place, by first wrapping your image in an
   :cpp:class:`ImageDesc` class.  This approach is intended to be used in high
   performance applications, and can be used on multiple threads (per scanline,
   per tile, etc).   In :ref:`usage_applybasic_python` you call
   "applyRGB" / "applyRGBA" on your sequence of pixels.   Note that in both
   languages, it is far more efficient to call "apply" on batches of pixels at
   a time.

.. _usage_applybasic_cpp:

C++
+++

.. code-block:: cpp
   
   #include <OpenColorIO/OpenColorIO.h>
   namespace OCIO = OCIO_NAMESPACE;
   
   try
   {
       OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
       ConstProcessorRcPtr processor = config->getProcessor(OCIO::ROLE_COMPOSITING_LOG,
                                                            OCIO::ROLE_SCENE_LINEAR);
       
       OCIO::PackedImageDesc img(imageData, w, h, 4);
       processor->apply(img);
   }
   catch(OCIO::Exception & exception)
   {
       std::cerr << "OpenColorIO Error: " << exception.what() << std::endl;
   }

.. _usage_applybasic_python:

Python
++++++

.. code-block:: py
   
   import PyOpenColorIO as OCIO
   
   try:
       config = OCIO.GetCurrentConfig()
       processor = config.getProcessor(OCIO.Constants.ROLE_COMPOSITING_LOG,
                                       OCIO.Constants.ROLE_SCENE_LINEAR)
       
       # Apply the color transform to the existing RGBA pixel data
       img = processor.applyRGBA(img)
   except Exception, e:
       print "OpenColorIO Error",e

.. _usage_displayimage:

Displaying an image, using the CPU (simple ColorSpace conversion)
*****************************************************************
Converting an image for display is similar to a normal color space conversion.
The only difference is that one has to first determine the name of the display
(destination) ColorSpace by querying the config with the device name and
transform name. 

#. **Get the Config.**
   See :ref:`usage_applybasic` for details.
   
#. **Lookup the display ColorSpace.**
   The display :cpp:class:`ColorSpace` is queried from the configuration using
   :cpp:func:`Config::getDisplayColorSpaceName`.   If the user has specified
   value for the ``device`` or the ``displayTransformName``, use them. If these
   values are unknown default values can be queried (as shown below).

#. **Get the processor from the Config.**
   See :ref:`usage_applybasic` for details.

#. **Convert your image, using the processor.**
   See :ref:`usage_applybasic` for details.

C++
+++

.. code-block:: cpp
   
   #include <OpenColorIO/OpenColorIO.h>
   namespace OCIO = OCIO_NAMESPACE;
   
   OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
   
   // If the user hasn't picked a display, use the defaults...
   const char * device = config->getDefaultDisplayDeviceName();
   const char * transformName = config->getDefaultDisplayTransformName(device);
   const char * displayColorSpace = config->getDisplayColorSpaceName(device, transformName);
   
   ConstProcessorRcPtr processor = config->getProcessor(OCIO::ROLE_SCENE_LINEAR,
                                                        displayColorSpace);
   
   OCIO::PackedImageDesc img(imageData, w, h, 4);
   processor->apply(img);

Python
++++++

.. code-block:: python

    import PyOpenColorIO as OCIO

    config = OCIO.GetCurrentConfig()

    device = config.getDefaultDisplayDeviceName()
    transformName = config.getDefaultDisplayTransformName(device)
    displayColorSpace = config.getDisplayColorSpaceName(device, transformName)

    processor = config.getProcessor(OCIO.Constants.ROLE_SCENE_LINEAR, displayColorSpace)

    processor.applyRGB(imageData)


Displaying an image, using the CPU (Full Display Pipeline)
**********************************************************

This alternative version allows for a more complex displayTransform, allowing
for all of the controls typically added to real-world viewer interfaces. For
example, options are allowed to control which channels (red, green, blue,
alpha, luma) are visible, as well as allowing for optional color corrections
(such as an exposure offset in scene linear). If you are using the OCIO Nuke
plugins, the OCIODisplay node performs these steps internally.

#. **Get the Config.**
   See :ref:`usage_applybasic` for details.
#. **Lookup the display ColorSpace.**
   See :ref:`usage_displayimage` for details
#. **Create a new DisplayTransform.**
   This transform will embody the full 'display' pipeline you wish to control.
   The user is required to call
   :cpp:func:`DisplayTransform::setInputColorSpaceName` to set the input
   ColorSpace, as well as
   :cpp:func:`DisplayTransform::setDisplayColorSpaceName` (with the results of
   :cpp:func:`Config::getDisplayColorSpaceName`).
#. **Set any additional DisplayTransform options.**
   If the user wants to specify a channel swizzle, a scene-linear exposure
   offset, an artistic look, this is the place to add it. See below for an
   example. Note that although we provide recommendations for display, any
   transforms are allowed to be added into any of the slots. So if for your app
   you want to add 3 transforms into a particular slot (chained together), you
   are free to wrap them in a :cpp:class:`GroupTransform` and set it
   accordingly!
#. **Get the processor from the Config.**
   The processor is then queried from the config passing the new
   :cpp:class:`DisplayTransform` as the argument.   Once the processor has been
   returned, the original :cpp:class:`DisplayTransform` is no longer necessary
   to hold onto. (Though if you'd like to for re-use, there is no problem doing
   so).
#. **Convert your image, using the processor.**
   See :ref:`usage_applybasic` for details.

C++
+++

.. code-block:: cpp
   
   // Step 1: Get the config
   OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
   
   // Step 2: Lookup the display ColorSpace
   const char * device = config->getDefaultDisplayDeviceName();
   const char * transformName = config->getDefaultDisplayTransformName(device);
   const char * displayColorSpace = config->getDisplayColorSpaceName(device, transformName);
   
   // Step 3: Create a DisplayTransform, and set the input and display ColorSpaces
   // (This example assumes the input is scene linear. Adapt as needed.)
   
   OCIO::DisplayTransformRcPtr transform = OCIO::DisplayTransform::Create();
   transform->setInputColorSpaceName( OCIO::ROLE_SCENE_LINEAR );
   transform->setDisplayColorSpaceName( displayColorSpace );
   
   // Step 4: Add custom transforms for a 'canonical' Display Pipeline
   
   // Add an fstop exposure control (in SCENE_LINEAR)
   float gain = powf(2.0f, exposure_in_stops);
   const float slope3f[] = { gain, gain, gain };
   OCIO::CDLTransformRcPtr cc =  OCIO::CDLTransform::Create();
   cc->setSlope(slope3f);
   transform->setLinearCC(cc);
   
   // Add a Channel view 'swizzle'
   
   // 'channelHot' controls which channels are viewed.
   int channelHot[4] = { 1, 1, 1, 1 };  // show rgb
   //int channelHot[4] = { 1, 0, 0, 0 };  // show red
   //int channelHot[4] = { 0, 0, 0, 1 };  // show alpha
   //int channelHot[4] = { 1, 1, 1, 0 };  // show luma
   
   float lumacoef[3];
   config.getDefaultLumaCoefs(lumacoef);
   
   float m44[16];
   float offset[4];
   OCIO::MatrixTransform::View(m44, offset, channelHot, lumacoef);
   OCIO::MatrixTransformRcPtr swizzle = OCIO::MatrixTransform::Create();
   swizzle->setValue(m44, offset);
   transform->setChannelView(swizzle);
   
   // And then process the image normally.
   OCIO::ConstProcessorRcPtr processor = config->getProcessor(transform);
   
   OCIO::PackedImageDesc img(imageData, w, h, 4);
   processor->apply(img);

Python
++++++

.. code-block:: python

    import PyOpenColorIO as OCIO

    # Step 1: Get the config
    config = OCIO.GetCurrentConfig()

    # Step 2: Lookup the display ColorSpace
    device = config.getDefaultDisplayDeviceName()
    transformName = config.getDefaultDisplayTransformName(device)
    displayColorSpace = config.getDisplayColorSpaceName(device, transformName)

    # Step 3: Create a DisplayTransform, and set the input and display ColorSpaces
    # (This example assumes the input is scene linear. Adapt as needed.)

    transform = OCIO.DisplayTransform()
    transform.setInputColorSpaceName(OCIO.Constants.ROLE_SCENE_LINEAR)
    transform.setDisplayColorSpaceName(displayColorSpace)

    # Step 4: Add custom transforms for a 'canonical' Display Pipeline

    # Add an fstop exposure control (in SCENE_LINEAR)
    gain = 2**exposure
    slope3f = (gain, gain, gain)

    cc = OCIO.CDLTransform()
    cc.setSlope(slope3f)

    transform.setLinearCC(cc)

    # Add a Channel view 'swizzle'

    channelHot = (1, 1, 1, 1) # show rgb
    # channelHot = (1, 0, 0, 0) # show red
    # channelHot = (0, 0, 0, 1) # show alpha
    # channelHot = (1, 1, 1, 0) # show luma

    lumacoef = config.getDefaultLumaCoefs()

    m44, offset = OCIO.MatrixTransform.View(channelHot, lumacoef)

    swizzle = OCIO.MatrixTransform()
    swizzle.setValue(m44, offset)
    transform.setChannelView(swizzle)

    # And then process the image normally.
    processor = config.getProcessor(transform)

    print processor.applyRGB(imageData)


Displaying an image, using the GPU
**********************************

Applying OpenColorIO's color processing using GPU processing is
straightforward, provided you have the capability to upload custom shader code
and a custom 3D Lookup Table (3DLUT).

#. **Get the Processor.**
   This portion of the pipeline is identical to the CPU approach. Just get the
   processor as you normally would have, see above for details.
#. **Create a GpuShaderDesc.**
#. **Query the GPU Shader Text + 3D LUT.**
#. **Configure the GPU State.**
#. **Draw your image.**

C++
+++

This example is available as a working app in the OCIO source: src/apps/ociodisplay.

.. code-block:: cpp
   
   // Step 0: Get the processor using any of the pipelines mentioned above.
   OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
   const char * device = config->getDefaultDisplayDeviceName();
   const char * transformName = config->getDefaultDisplayTransformName(device);
   const char * displayColorSpace = config->getDisplayColorSpaceName(device, transformName); 
   ConstProcessorRcPtr processor = config->getProcessor(OCIO::ROLE_SCENE_LINEAR,
                                                        displayColorSpace);
   
   // Step 1: Create a GPU Shader Description
   GpuShaderDesc shaderDesc;
   shaderDesc.setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_0);
   shaderDesc.setFunctionName("OCIODisplay");
   const int LUT3D_EDGE_SIZE = 32;
   shaderDesc.setLut3DEdgeLen(LUT3D_EDGE_SIZE);
   
   // Step 2: Compute and the 3D LUT
   // Optional Optimization:
   //     Only do this the 3D LUT's contents
   //     are different from the last drawn frame.
   //     Use getGpuLut3DCacheID to compute the cacheID.
   //     cheaply.
   // 
   // std::string lut3dCacheID = processor->getGpuLut3DCacheID(shaderDesc);
   int num3Dentries = 3*LUT3D_EDGE_SIZE*LUT3D_EDGE_SIZE*LUT3D_EDGE_SIZE;
   std::vector<float> g_lut3d;
   g_lut3d.resize(num3Dentries);
   processor->getGpuLut3D(&g_lut3d[0], shaderDesc);
   
   // Load the data into an OpenGL 3D Texture
   glGenTextures(1, &g_lut3d_textureID);
   glBindTexture(GL_TEXTURE_3D, g_lut3d_textureID);
   glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB,
                LUT3D_EDGE_SIZE, LUT3D_EDGE_SIZE, LUT3D_EDGE_SIZE,
                0, GL_RGB,GL_FLOAT, &g_lut3d[0]);
   
   // Step 3: Query

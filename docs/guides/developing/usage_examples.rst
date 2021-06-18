..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

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

#. **Get a CPU or GPU Processor.**
   Building the processor assembles the operators needed to perform the requested
   transformation, however it is not ready to process pixels. The next step is to
   create a CPU or GPU processor.  This optimizes and finalizes the operators to
   produce something that may be executed efficiently on the target platform.

#. **Convert your image, using the Processor.**
   Once you have a CPU or GPU processor, you can apply the color transformation 
   using the "apply" function.  In :ref:`usage_applybasic_cpp`, you may apply the
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
      OCIO::ConstProcessorRcPtr processor = config->getProcessor(OCIO::ROLE_COMPOSITING_LOG,
                                                                 OCIO::ROLE_SCENE_LINEAR);
       
      OCIO::ConstCPUProcessorRcPtr cpu = processor->getDefaultCPUProcessor();

      // Apply the color transform to an existing RGBA image
      OCIO::PackedImageDesc img(imageData, width, height, 4);
      cpu->apply(img);
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
      processor = config.getProcessor(OCIO.ROLE_COMPOSITING_LOG,
                                      OCIO.ROLE_SCENE_LINEAR)
      cpu = processor.getDefaultCPUProcessor()
       
      # Apply the color transform to the existing RGBA pixel data
      img = [1, 0, 0, 0]
      img = cpu.applyRGBA(img)
   except Exception as e:
      print("OpenColorIO Error: ", e)

.. _usage_displayimage:

Displaying an image, using the CPU using getProcessor
*****************************************************

#. **Get the Config.**
   See :ref:`usage_applybasic` for details.
   
#. **Get the desired display and view.**
   If the user has specified a display and view, use them.  Otherwise 
   :cpp:func:`Config::getDefaultDisplay` and :cpp:func:`Config::getDefaultView`
   may be used to get the defaults.  These may be provided directly in a call
   to getProcessor rather than the source and destination color space.

#. **Get the processor and CPU processor from the Config.**
   See :ref:`usage_applybasic` for details.

#. **Convert your image, using the processor.**
   See :ref:`usage_applybasic` for details.

C++
+++

.. code-block:: cpp
   
   #include <OpenColorIO/OpenColorIO.h>
   namespace OCIO = OCIO_NAMESPACE;
   
   try
   {
      OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
   
      const char * display = config->getDefaultDisplay();
      const char * view = config->getDefaultView(display);
   
      OCIO::ConstProcessorRcPtr processor = config->getProcessor(OCIO::ROLE_SCENE_LINEAR,
                                                                 display, view,
                                                                 OCIO::TRANSFORM_DIR_FORWARD);
      OCIO::ConstCPUProcessorRcPtr cpu = processor->getDefaultCPUProcessor();
   
      OCIO::PackedImageDesc img(imageData, width, height, 4);
      cpu->apply(img);
   }
   catch(OCIO::Exception & exception)
   {
      std::cerr << "OpenColorIO Error: " << exception.what() << std::endl;
   }

Python
++++++

.. code-block:: python

   import PyOpenColorIO as OCIO

   try:
      config = OCIO.GetCurrentConfig()

      display = config.getDefaultDisplay()
      view = config.getDefaultView(display)

      processor = config.getProcessor(OCIO.ROLE_SCENE_LINEAR, display, view, OCIO.TRANSFORM_DIR_FORWARD)
      cpu = processor.getDefaultCPUProcessor()

      img = [1, 0, 0]
      cpu.applyRGB(img)
   except Exception as e:
      print("OpenColorIO Error: ", e)


Displaying an image, using the CPU using DisplayViewTransform
*************************************************************

C++
+++

.. code-block:: cpp
   
   // Step 1: Get the config
   OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
   
   // Step 2: Lookup the display ColorSpace
   const char * display = config->getDefaultDisplay();
   const char * view = config->getDefaultView(display);
   
   // Step 3: Create a DisplayViewTransform, and set the input and display ColorSpaces
   // (This example assumes the input is a role. Adapt as needed.)
   
   OCIO::DisplayTransformRcPtr transform = OCIO::DisplayViewTransform::Create();
   transform->setSrc( OCIO::ROLE_SCENE_LINEAR );
   transform->setDisplay( display );
   transform->setView( view );
   
   // Step 4: Create the processor
   OCIO::ConstProcessorRcPtr processor = config->getProcessor(transform);
   OCIO::ConstCPUProcessorRcPtr cpu = processor->getDefaultCPUProcessor();
   
   // Step 5: Apply the color transform to an existing RGBA pixel
   OCIO::PackedImageDesc img(imageData, width, height, 4);
   cpu->apply(img);

Python
++++++

.. code-block:: python

   import PyOpenColorIO as OCIO

   # Step 1: Get the config
   config = OCIO.GetCurrentConfig()

   # Step 2: Lookup the display ColorSpace
   display = config.getDefaultDisplay()
   view = config.getDefaultView(display)

   # Step 3: Create a DisplayViewTransform, and set the input, display, and view
   # (This example assumes the input is a role. Adapt as needed.)

   transform = OCIO.DisplayViewTransform()
   transform.setSrc(OCIO.ROLE_SCENE_LINEAR)
   transform.setDisplay(display)
   transform.setView(view)

   # Step 4: Create the processor
   processor = config.getProcessor(transform)
   cpu = processor.getDefaultCPUProcessor()

   # Step 5: Apply the color transform to an existing RGB pixel
   imageData = [1, 0, 0]
   print(cpu.applyRGB(imageData))


Displaying an image, using the GPU (Full Display Pipeline)
**********************************************************

This alternative version allows for a more complex viewing pipeline, allowing
for all of the controls typically added to real-world viewer interfaces. For
example, options are allowed to control which channels (red, green, blue,
alpha, luma) are visible, as well as allowing for optional color corrections
(such as an exposure offset in scene linear). 

#. **Get the Config.**
   See :ref:`usage_applybasic` for details.
#. **Lookup the display ColorSpace.**
   See :ref:`usage_displayimage` for details
#. **Create a new DisplayViewTransform.**
   This transform has the basic conversion from the reference space to the 
   display but without the extras such as the channel swizzling and exposure
   control.
   The user is required to call
   :cpp:func:`DisplayViewTransform::setSrc` to set the input
   ColorSpace, as well as
   :cpp:func:`DisplayViewTransform::setDisplay` and.
   :cpp:func:`DisplayViewTransform::setView`
#. **Create a new LegacyViewingPipeline.**
   This transform will embody the full 'display' pipeline you wish to control.
   The user is required to call
   :cpp:func:`LegacyViewingPipeline::setDisplayViewTransform` to set the
   DisplayViewTransform.
#. **Set any additional LegacyViewingPipeline options.**
   If the user wants to specify a channel swizzle, a scene-linear exposure
   offset, an artistic look, this is the place to add it. See ociodisplay for an
   example. Note that although we provide recommendations for display, any
   transforms are allowed to be added into any of the slots. So if for your app
   you want to add 3 transforms into a particular slot (chained together), you
   are free to wrap them in a :cpp:class:`GroupTransform` and set it
   accordingly!
#. **Get the processor from the LegacyViewingPipeline.**
   The processor is then queried from the LegacyViewingPipeline.
#. **Convert your image, using the processor.**
   See :ref:`usage_applybasic` for details for using the CPU.

Applying OpenColorIO's color processing using the GPU is very customizable
and an example helper class is provided for use with OpenGL.

#. **Get the Processor and GPU Processor.**
   This portion of the pipeline is identical to the CPU approach. Just get the
   processor as you normally would have, see above for details.
#. **Create a GpuShaderDesc.**
#. **Extract the GPU Shader.**
#. **Configure the GPU State.**
#. **Draw your image.**

C++
+++

This example is available as a working app in the OCIO source: src/apps/ociodisplay.

.. code-block:: cpp
   
   OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
   
   const char * display = config->getDefaultDisplay();
   const char * view = config->getDefaultView(display);
   const char * look = config->getDisplayViewLooks(display, view);

   OCIO::DisplayViewTransformRcPtr transform = OCIO::DisplayViewTransform::Create();
   transform->setSrc(OCIO::ROLE_SCENE_LINEAR);
   transform->setDisplay(display);
   transform->setView(view);

   OCIO::LegacyViewingPipeline vpt;
   vpt.setDisplayViewTransform(transform);
   vpt.setLooksOverrideEnabled(true);
   vpt.setLooksOverride(look);

   OCIO::ConstProcessorRcPtr processor = vpt.getProcessor(config, config->getCurrentContext());
   OCIO::ConstGPUProcessorRcPtr gpu = processor->getDefaultGPUProcessor();

   OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
   shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_3);
   shaderDesc->setFunctionName("OCIODisplay");
   shaderDesc->setResourcePrefix("ocio_");

   gpu->extractGpuShaderInfo(shaderDesc);

   OCIO::OglAppRcPtr oglApp = std::make_shared<OCIO::ScreenApp>("ociodisplay", 512, 512);
   oglApp->initImage(imgWidth, imgHeight, OCIO::OglApp::COMPONENTS_RGBA, &img[0]);
   oglApp->setShader(shaderDesc);

   oglApp->redisplay();

"""
This script allows the use of OpenColorIO display transforms (3d luts) in
the Mari Viewer. Requires Mari 1.3v2+.

This example is not represntative of the final Mari OCIO workflow, merely
an API demonstration. This code is a work in progress, to demonstrate the
integration of OpenColorIO and Mari. The APIs this code relies on are subject
to change at any time, and as such should not be relied on for production use
(yet).

LINUX testing instructions:

* Build OCIO
mkdir -p dist_mari
mkdir -p build_mari && cd build_mari
cmake -D CMAKE_BUILD_TYPE=Release \
      -D CMAKE_INSTALL_PREFIX=../dist_mari \
      -D PYTHON=/usr/bin/python2.6 \
      -D OCIO_NAMESPACE=OpenColorIO_Mari \
      ../
make install -j8

* Set $OCIO color environment
setenv OCIO setenv OCIO <YOURDIR>/ocio.configs/spi-vfx/config.ocio
(Profiles available for download from opencolorio.org)

* Run Mari with OpenColorIO added to the LD_LIBRARY_PATH, and Python
env LD_LIBRARY_PATH=<YOURDIR>/dist_mari/lib/ PYTHONPATH=<YOURDIR>/dist_mari/lib/python2.6 mari

* Source this script in the python console.
Also - IMPORTANT - you must enable 'Use Color Correction' in the Color Manager.

"""

INPUT_IMAGE_COLORSPACE = "dt8"  # THIS IS Config SPECIFIC. TODO: Replace with OCIO.Constants.ROLE_TEXTURE_PAINT
LUT3D_SIZE = 32
FSTOP_OFFSET = 1.0
VIEW_GAMMA = 1.0
SWIZZLE = (True,  True,  True,  True)

import mari, time

try:
    import PyOpenColorIO as OCIO
    print OCIO.__file__
except Exception,e:
    print "Error: Could not import OpenColorIO python bindings."
    print "Please confirm PYTHONPATH has dir containing PyOpenColorIO.so"

# Viewer controls
# TODO: Expose in an interface (panel)

"""
SWIZZLE_COLOR = (True,  True,  True,  True)
SWIZZLE_RED   = (True,  False, False, False)
SWIZZLE_GREEN = (False, True,  False, False)
SWIZZLE_BLUE  = (False, False, True,  False)
SWIZZLE_ALPHA = (False, False, False, True)
SWIZZLE_LUMA  = (True,  True,  True,  False)
"""
"""
Timings

OCIO Setup: 0.5 ms
OCIO 3D Lut creation: 14.7 ms
Mari Setup: 21.3 ms
Mari Texture Upload: 44.2 ms
"""

def RegisterOCIODisplay():
    if not hasattr(mari.gl_render,"createPostFilter"):
        print "Error: This version of Mari does not support the mari.gl_render.createPostFilter API"
        return
    
    # OpenColorIO SETUP
    # Use OpenColorIO to generate the shader text and the 3dlut
    starttime = time.time()
    
    config = OCIO.GetCurrentConfig()
    display = config.getDefaultDisplay()
    view = config.getDefaultView(display)
    transform = CreateOCIODisplayTransform(config, INPUT_IMAGE_COLORSPACE,
                                           display, view,
                                           SWIZZLE,
                                           FSTOP_OFFSET, VIEW_GAMMA)
    
    processor = config.getProcessor(transform)
    
    shaderDesc = dict( [('language', OCIO.Constants.GPU_LANGUAGE_GLSL_1_3),
                        ('functionName', 'display_ocio_$ID_'),
                        ('lut3DEdgeLen', LUT3D_SIZE)] )
    shaderText = processor.getGpuShaderText(shaderDesc)
    print "OCIO Setup: %0.1f ms" % (1000* (time.time()-starttime))
    
    starttime = time.time()
    lut3d = processor.getGpuLut3D(shaderDesc)
    print "OCIO 3D Lut creation: %0.1f ms" % (1000* (time.time()-starttime))
    
    starttime = time.time()
    # Clear the existing color managment filter stack
    mari.gl_render.clearPostFilterStack()
    
    # Create variable pre-declarations
    desc = "sampler3D lut3d_ocio_$ID_;\n"
    desc += shaderText
    
    # Set the body of the filter
    body = "{ Out = display_ocio_$ID_(Out, lut3d_ocio_$ID_); }"
    
    # HACK: Increment a counter by 1 each time to force a refresh
    if not hasattr(mari, "ocio_counter_hack"):
        mari.ocio_counter_hack = 0
    else:
        mari.ocio_counter_hack += 1
    ocio_counter_hack = mari.ocio_counter_hack
    
    # Create a new filter
    name = "OCIO %s %s %s v%d" % (display, view, INPUT_IMAGE_COLORSPACE, ocio_counter_hack)
    postfilter = mari.gl_render.createPostFilter(name, desc, body)
    
    print "Mari Setup: %0.1f ms" % (1000* (time.time()-starttime))
    
    starttime = time.time()
    # Set the texture to use for the given sampler on this filter
    postfilter.setTexture3D("lut3d_ocio_$ID_",
                            LUT3D_SIZE, LUT3D_SIZE, LUT3D_SIZE,
                            postfilter.FORMAT_RGB, lut3d)
    print "Mari Texture Upload: %0.1f ms" % (1000* (time.time()-starttime))
    
    # Append the filter to the end of the current list of filters
    mari.gl_render.appendPostFilter(postfilter)

def CreateOCIODisplayTransform(config,
                               inputColorSpace,
                               display, view,
                               swizzle,
                               fstopOffset, viewGamma):
    
    displayTransform = OCIO.DisplayTransform()
    displayTransform.setInputColorSpaceName( inputColorSpace )
    
    displayColorSpace = config.getDisplayColorSpaceName(display, view)
    displayTransform.setDisplayColorSpaceName( displayColorSpace )
    
    # Add the channel sizzle
    lumacoef = config.getDefaultLumaCoefs()
    mtx, offset = OCIO.MatrixTransform.View(swizzle, lumacoef)
    
    transform = OCIO.MatrixTransform()
    transform.setValue(mtx, offset)
    displayTransform.setChannelView(transform)
    
    # Add the linear fstop gain
    gain = 2**fstopOffset
    mtx, offset = OCIO.MatrixTransform.Scale((gain,gain,gain,gain))
    transform = OCIO.MatrixTransform()
    transform.setValue(mtx, offset)
    displayTransform.setLinearCC(transform)
    
    # Add the post-display CC
    transform = OCIO.ExponentTransform()
    transform.setValue([1.0 / max(1e-6, v) for v in \
                       (viewGamma, viewGamma, viewGamma, viewGamma)])
    displayTransform.setDisplayCC(transform)
    
    return displayTransform

RegisterOCIODisplay()

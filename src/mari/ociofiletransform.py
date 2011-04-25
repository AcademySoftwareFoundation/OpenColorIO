"""
This script allows the loading of a specied lut (1d/3d/mtx) in
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

* Edit this file to point to use viewer lut you want to use

* Run Mari with OpenColorIO added to the LD_LIBRARY_PATH, and Python
env LD_LIBRARY_PATH=<YOURDIR>/dist_mari/lib/ PYTHONPATH=<YOURDIR>/dist_mari/lib/python2.6 mari

* Source this script in the python console.
Also - IMPORTANT - you must enable 'Use Color Correction' in the Color Manager.

"""

# YOU MUST CHANGE THIS TO MODIFY WHICH LUT TO USE
LUT_FILENAME = "/shots/spi/home/lib/lut/dev/v29/luts/LC3DL_Kodak2383_Sony_GDM.3dl"
LUT3D_SIZE = 32

import mari, time, os

try:
    import PyOpenColorIO as OCIO
    print OCIO.__file__
except Exception,e:
    print "Error: Could not import OpenColorIO python bindings."
    print "Please confirm PYTHONPATH has dir containing PyOpenColorIO.so"

def RegisterOCIOLut():
    if not hasattr(mari.gl_render,"createPostFilter"):
        print "Error: This version of Mari does not support the mari.gl_render.createPostFilter API"
        return
    
    config = OCIO.Config()
    transform = OCIO.FileTransform(src = os.path.realpath(LUT_FILENAME),
                                   interpolation = OCIO.Constants.INTERP_LINEAR,
                                   direction = OCIO.Constants.TRANSFORM_DIR_FORWARD)
    processor = config.getProcessor(transform)
    
    shaderDesc = dict( [('language', OCIO.Constants.GPU_LANGUAGE_GLSL_1_3),
                        ('functionName', 'display_ocio_$ID_'),
                        ('lut3DEdgeLen', LUT3D_SIZE)] )
    shaderText = processor.getGpuShaderText(shaderDesc)
    
    lut3d = processor.getGpuLut3D(shaderDesc)
    
    # Clear the existing color managment filter stack
    mari.gl_render.clearPostFilterStack()
    
    # Create variable pre-declarations
    desc = "sampler3D lut3d_ocio_$ID_;\n"
    desc += shaderText
    
    # Set the body of the filter
    body = "{ Out = display_ocio_$ID_(Out, lut3d_ocio_$ID_); }"
    
    # HACK: Increment a counter by 1 each time to force a refresh
    if not hasattr(mari, "ocio_lut_counter_hack"):
        mari.ocio_lut_counter_hack = 0
    else:
        mari.ocio_lut_counter_hack += 1
    ocio_lut_counter_hack = mari.ocio_lut_counter_hack
    
    # Create a new filter
    name = "OCIO %s v%d" % (os.path.basename(LUT_FILENAME), ocio_lut_counter_hack)
    postfilter = mari.gl_render.createPostFilter(name, desc, body)
    
    # Set the texture to use for the given sampler on this filter
    postfilter.setTexture3D("lut3d_ocio_$ID_",
                            LUT3D_SIZE, LUT3D_SIZE, LUT3D_SIZE,
                            postfilter.FORMAT_RGB, lut3d)
    
    # Append the filter to the end of the current list of filters
    mari.gl_render.appendPostFilter(postfilter)

RegisterOCIOLut()

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

import mari, time, PythonQt
QtGui = PythonQt.QtGui
QtCore = PythonQt.QtCore

try:
    import PyOpenColorIO as OCIO
    mari.app.log("OCIODisplay: %s" % OCIO.__file__)
except Exception,e:
    OCIO = None
    mari.app.log("OCIODisplay: Error: Could not import OpenColorIO python bindings.")
    mari.app.log("OCIODisplay: Please confirm PYTHONPATH has dir containing PyOpenColorIO.so")

__all__ = [ 'OCIO', 'CreateOCIODisplayTransform', 'OCIODisplayUI']

LUT3D_SIZE = 32
WINDOW_NAME = "OpenColorIO Display Manager"
CREATE_FLOATING = True


class OCIODisplayUI(QtGui.QWidget):
    def __init__(self):
        QtGui.QWidget.__init__(self)
        QtGui.QGridLayout(self)
        self.setWindowTitle(WINDOW_NAME)
        self.setMinimumWidth(300)
        
        config = OCIO.GetCurrentConfig()
        
        self.__inputColorSpace = OCIO.Constants.ROLE_DEFAULT
        inputColorSpaces = [ OCIO.Constants.ROLE_TEXTURE_PAINT,
                             'dt8',
                             OCIO.Constants.ROLE_SCENE_LINEAR ]
        for cs in inputColorSpaces:
            if config.getColorSpace(cs) is None: continue
            self.__inputColorSpace = cs
            break
        
        self.__fStopOffset = 0.0
        self.__viewGamma = 1.0
        self.__swizzle = (True,  True,  True,  True)
        
        self.__filter_cacheID = None
        self.__filter = None
        self.__texture3d_cacheID = None
        self.__counter_hack = 0
        
        self.__buildUI()
        self.__rebuildFilter()
    
    def __buildUI(self):
        config = OCIO.GetCurrentConfig()
        
        self.layout().addWidget( QtGui.QLabel("Input Color Space", self), 0, 0)
        csWidget = QtGui.QComboBox(self)
        self.layout().addWidget( csWidget, 0, 1)
        csIndex = None
        for name in (cs.getName() for cs in config.getColorSpaces()):
            csWidget.addItem(name)
            if name == self.__inputColorSpace:
                csIndex = csWidget.count - 1
        if csIndex is not None:
            csWidget.setCurrentIndex(csIndex)
        csWidget.connect( QtCore.SIGNAL('currentIndexChanged(const QString &)'), self.__csChanged)
        
        
        # This doesnt work until the Horizontal enum is exposed.
        """
        self.__fstopWidget_numStops = 3.0
        self.__fstopWidget_ticksPerStop = 4
        
        self.layout().addWidget( QtGui.QLabel("FStop", self), 1, 0)
        fstopWidget = QtGui.QSlider(Horizontal, self)
        self.layout().addWidget( fstopWidget, 1, 1)
        fstopWidget.setMinimum(int(-self.__fstopWidget_numStops*self.__fstopWidget_ticksPerStop))
        fstopWidget.setMaximum(int(self.__fstopWidget_numStops*self.__fstopWidget_ticksPerStop))
        fstopWidget.setTickInterval(self.__fstopWidget_ticksPerStop)
        """
        
    
    def __csChanged(self, text):
        text = str(text)
        if text != self.__inputColorSpace:
            self.__inputColorSpace = text
            self.__rebuildFilter()
    
    def __rebuildFilter(self):
        config = OCIO.GetCurrentConfig()
        display = config.getDefaultDisplay()
        view = config.getDefaultView(display)
        transform = CreateOCIODisplayTransform(config, self.__inputColorSpace,
                                               display, view,
                                               self.__swizzle,
                                               self.__fStopOffset, self.__viewGamma)
        
        processor = config.getProcessor(transform)
        
        shaderDesc = dict( [('language', OCIO.Constants.GPU_LANGUAGE_GLSL_1_3),
                            ('functionName', 'display_ocio_$ID_'),
                            ('lut3DEdgeLen', LUT3D_SIZE)] )
        
        filterCacheID = processor.getGpuShaderTextCacheID(shaderDesc)
        if filterCacheID != self.__filter_cacheID:
            self.__filter_cacheID = filterCacheID
            mari.app.log("OCIODisplay: Creating filter %s" % self.__filter_cacheID)
            
            desc = "sampler3D lut3d_ocio_$ID_;\n"
            desc += processor.getGpuShaderText(shaderDesc)
            body = "{ Out = display_ocio_$ID_(Out, lut3d_ocio_$ID_); }"
            
            # Clear the existing color managment filter stack and create a new filter
            # HACK: Increment a counter by 1 each time to force a refresh
            #self.__counter_hack += 1
            #name = "OCIO %s %s %s v%d" % (display, view, self.__inputColorSpace, self.__counter_hack)
            name = "OCIO %s %s %s" % (display, view, self.__inputColorSpace)
            
            self.__filter = None
            self.__texture3d_cacheID = None
            
            mari.gl_render.clearPostFilterStack()
            self.__filter = mari.gl_render.createPostFilter(name, desc, body)
            mari.gl_render.appendPostFilter(self.__filter)
        else:
            mari.app.log('OCIODisplay: no shader text update required')
        
        texture3d_cacheID = processor.getGpuLut3DCacheID(shaderDesc)
        if texture3d_cacheID != self.__texture3d_cacheID:
            lut3d = processor.getGpuLut3D(shaderDesc)
            
            mari.app.log("OCIODisplay: Updating 3dlut %s" % texture3d_cacheID)
            
            if self.__texture3d_cacheID is None:
                self.__filter.setTexture3D("lut3d_ocio_$ID_",
                                     LUT3D_SIZE, LUT3D_SIZE, LUT3D_SIZE,
                                     self.__filter.FORMAT_RGB, lut3d)
            else:
                self.__filter.updateTexture3D( "lut3d_ocio_$ID_", lut3d)
            
            self.__texture3d_cacheID = texture3d_cacheID
        else:
            mari.app.log("OCIODisplay: No lut3d update required")



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

"""
SWIZZLE_COLOR = (True,  True,  True,  True)
SWIZZLE_RED   = (True,  False, False, False)
SWIZZLE_GREEN = (False, True,  False, False)
SWIZZLE_BLUE  = (False, False, True,  False)
SWIZZLE_ALPHA = (False, False, False, True)
SWIZZLE_LUMA  = (True,  True,  True,  False)

Timings

OCIO Setup: 0.5 ms
OCIO 3D Lut creation: 14.7 ms
Mari Setup: 21.3 ms
Mari Texture Upload: 44.2 ms
"""


if __name__ == '__main__':
    if not hasattr(mari.gl_render,"createPostFilter"):
        mari.app.log("OCIODisplay: Error: This version of Mari does not support the mari.gl_render.createPostFilter API")
    else:
        if OCIO is not None:
            if CREATE_FLOATING:
                w = OCIODisplayUI()
                w.show()
            else:
                if WINDOW_NAME in mari.app.tabNames():
                    mari.app.removeTab(WINDOW_NAME)
                w = OCIODisplayUI()
                mari.app.addTab(WINDOW_NAME, w)

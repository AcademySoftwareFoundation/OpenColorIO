"""
Test the Python bindings.
"""
import PyOpenColorIO as OCIO

print ""
print "PyOCIO:", OCIO.__file__
print "OCIO:",dir(OCIO)
print ""

#import pydoc
#print pydoc.render_doc(OCIO)

#print dir(OCIO.Config)
#c = OCIO.Config.CreateFromEnv()
#print 'c',c
#print 'isEditable', c.isEditable()
#print "resourcePath: '%s' " % (c.getResourcePath(),)
#c = OCIO.GetCurrentConfig()
#print 'isEditable', c.isEditable()
#print "resourcePath: '%s' " % (c.getResourcePath(),)
#
#OCIO.SetCurrentConfig(c)
#c = OCIO.GetCurrentConfig()
#print ''
#print "ColorSpaces"
#for cs in c.getColorSpaces():
#    print "%s %s %s" % (cs.getName(), cs.getFamily(), cs.getBitDepth())


# Create a new config
"""
config = OCIO.Config()

print 'isEditable', config.isEditable()
print "resourcePath: '%s' " % (config.getResourcePath(),)

# Add a colorspace
cs = OCIO.ColorSpace()
cs.setName("lnh")
cs.setFamily("ln")
cs.setBitDepth(OCIO.BIT_DEPTH_F16)
cs.setIsData(False)
cs.setGpuAllocation(OCIO.GPU_ALLOCATION_LG2)
cs.setGpuMin(-16.0)
cs.setGpuMax(6.0)

config.addColorSpace(cs)
config.setColorSpaceForRole(OCIO.ROLE_SCENE_LINEAR, cs.getName())

print config.getDefaultLumaCoefs()
#config.setDefaultLumaCoefs((1/3.0,1/3.0,1/3.0))
#print config.getDefaultLumaCoefs()

xml = config.getXML()
#print xml
"""

config = OCIO.Config.CreateFromEnv()
print 'default display device',config.getDefaultDisplayDeviceName()
for device in config.getDisplayDeviceNames():
    print 'device',device
    print '    default',config.getDefaultDisplayTransformName(device)
    
    for transform in config.getDisplayTransforms(device):
        print '    transform',transform

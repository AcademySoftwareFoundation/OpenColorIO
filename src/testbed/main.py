"""
Test the Python bindings.
"""
import PyOpenColorIO as OCIO

print ""
print "PyOCIO:", OCIO.__file__
print "OCIO:",dir(OCIO)
print "version:",OCIO.version
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
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_F16)
cs.setIsData(False)
cs.setGpuAllocation(OCIO.Constants.GPU_ALLOCATION_LG2)
cs.setGpuMin(-16.0)
cs.setGpuMax(6.0)

config.addColorSpace(cs)
config.setRole(OCIO.Constants.ROLE_SCENE_LINEAR, cs.getName())

print config.getDefaultLumaCoefs()
#config.setDefaultLumaCoefs((1/3.0,1/3.0,1/3.0))
#print config.getDefaultLumaCoefs()

text = config.serialize()
#print text
"""

"""
config = OCIO.Config.CreateFromEnv()
print 'default display device',config.getDefaultDisplayDeviceName()
for device in config.getDisplayDeviceNames():
    print 'device',device
    print '    default',config.getDefaultDisplayTransformName(device)
    
    for transform in config.getDisplayTransformNames(device):
        print '    transform',transform
"""

"""

    OCIO::ConstColorSpaceRcPtr csSrc = config->getColorSpace("dt8");
    OCIO::ConstColorSpaceRcPtr csDst = config->getColorSpace("lnh");
    
    imageVec[0] = 445.0f/1023.0f;
    imageVec[1] = 1023.0/1023.0f;
    imageVec[2] = 0.0/1023.0f;
    std::cout << csSrc->getName() << " ";
    PrintColor(std::cout, &imageVec[0], "input");
    std::cout << std::endl;
    
    """

"""
config = OCIO.Config.CreateFromEnv()
c1 = config.getColorSpace("lnh")
c2 = config.getColorSpace("dt8")
print c1.getName(), c2.getName()
processor = config.getProcessor("dt8","lnh")
print "processor",processor
print "processor isNoOp",processor.isNoOp()

c = ( 445/1023.0, 1.0, 0.0 )
print processor.applyRGB(c)
"""

"""
#print OCIO.Constants.CombineTransformDirections(OCIO.Constants.TRANSFORM_DIR_INVERSE, OCIO.Constants.TRANSFORM_DIR_INVERSE)

#print dir(OCIO.MatrixTransform)
#print OCIO.MatrixTransform.View((False, False, False, True), (1.0, 1.0, 1.0))
#print OCIO.MatrixTransform.Identity()

"""

config = OCIO.Config()

cs = OCIO.ColorSpace()
cs.setName("lgf")
cs.setFamily("lg")
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_F16)
cs.setIsData(False)
cs.setGpuAllocation(OCIO.Constants.GPU_ALLOCATION_LG2)
cs.setGpuMin(-0.5)
cs.setGpuMax(1.5)

g = OCIO.GroupTransform()

t = OCIO.CineonLogToLinTransform()
t.setMaxAimDensity((1.890, 2.046, 2.046))
t.setNegGamma((0.49, 0.57, 0.60))
g.push_back(t)
cs.setTransform(g, OCIO.Constants.COLORSPACE_DIR_TO_REFERENCE)
logOffset = [v * 1023.0 for v in t.getNegGammaAsLogOffset()]
print 'logOffset',logOffset


config.addColorSpace(cs)
config.setRole(OCIO.Constants.ROLE_COMPOSITING_LOG, cs.getName())

text = config.serialize()
print '\n\n'
print text

fname = '/tmp/a.ocio'
f = file(fname,'w')
f.write(text)
f.close()

newconfig = OCIO.Config.CreateFromFile(fname)
print '\n\n'
print newconfig.serialize()

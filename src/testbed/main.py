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

config = OCIO.GetCurrentConfig()
processor = config.getProcessor(OCIO.Constants.ROLE_COMPOSITING_LOG, OCIO.Constants.ROLE_SCENE_LINEAR)
processor.applyRGBA([0,0,0,0]) # works fine
processor.applyRGBA(1)
   
"""
if True:
    config = OCIO.Config()
    
    cs = OCIO.ColorSpace()
    cs.setName("lnh")
    cs.setFamily("ln")
    cs.setBitDepth(OCIO.Constants.BIT_DEPTH_F16)
    cs.setIsData(False)
    cs.setAllocation(OCIO.Constants.ALLOCATION_LG2)
    cs.setAllocationVars((-16.0,6.0))
    
    fileTransform = OCIO.FileTransform(src='foo.lut', interpolation='linear', direction='inverse')
    cs2 = OCIO.ColorSpace(name="lnf",
                          family="ln",
                          bitDepth=OCIO.Constants.BIT_DEPTH_F32,
                          allocation=OCIO.Constants.ALLOCATION_LG2,
                          allocationVars=[-10.0,6.0],
                          to_reference=fileTransform
                          )
    
    config.addColorSpace(cs)
    config.addColorSpace(cs2)
    
    print config.serialize()
"""

"""


print 'isEditable', config.isEditable()
print "resourcePath: '%s' " % (config.getResourcePath(),)

# Add a colorspace
cs = OCIO.ColorSpace()
cs.setName("lnh")
cs.setFamily("ln")
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_F16)
cs.setIsData(False)
cs.setAllocation(OCIO.Constants.ALLOCATION_LG2)
cs.setAllocationVars((-16.0,6.0))

config.addColorSpace(cs)
config.setRole(OCIO.Constants.ROLE_SCENE_LINEAR, cs.getName())

print config.getDefaultLumaCoefs()
#config.setDefaultLumaCoefs((1/3.0,1/3.0,1/3.0))
#print config.getDefaultLumaCoefs()

"""
"""
config = OCIO.Config.CreateFromEnv()
config.sanityCheck()

print 'config',config.getCacheID()
print ''

p = config.getProcessor("lg10", "srgb8")
print p
print 'cpuCacheID',p.getCpuCacheID()

shaderDesc = dict( [('language', OCIO.Constants.GPU_LANGUAGE_GLSL_1_3),
                    ('functionName', 'display_ocio'),
                    ('lut3DEdgeLen', 32)] )

print 'shaderDesc',shaderDesc

shaderTextCacheID = p.getGpuShaderTextCacheID(shaderDesc)
print 'shaderTextCacheID', shaderTextCacheID

shaderText = p.getGpuShaderText(shaderDesc)
print 'shaderText ---'
print shaderText
print '---'

print ''
lut3DCacheID = p.getGpuLut3DCacheID(shaderDesc)
print 'lut3DCacheID', lut3DCacheID

lut3d = p.getGpuLut3D(shaderDesc)
print 'lut3d', len(lut3d)
"""

"""
config = OCIO.Config()


cs = OCIO.ColorSpace()
cs.setName("cheese")
cs.setFamily("cheese")
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_F16)
cs.setIsData(False)
cs.setAllocation(OCIO.Constants.ALLOCATION_LG2)
cs.setAllocationVars((-16.0,6.0))

#t = OCIO.FileTransform(src='foo', interpolation='linear', direction='inverse')
#t = OCIO.ColorSpaceTransform(dst = 'woozle', src = 'foo')
#t = OCIO.ExponentTransform(value=(2.0,0.5, 1.1,1.2))
#t = OCIO.LogTransform(base = 10.0, direction='inverse')

t = OCIO.FileTransform(cccid='foo')

#t = OCIO.GroupTransform(transforms = \
#    [OCIO.LogTransform(base = 10.0, direction='inverse'),
#     OCIO.LogTransform(base = 10.0, direction='inverse')])
#t.getTransforms()
#t.setTransforms([])

cs.setTransform(t, OCIO.Constants.COLORSPACE_DIR_TO_REFERENCE)

config.addColorSpace(cs)
config.setRole(OCIO.Constants.ROLE_SCENE_LINEAR, cs.getName())


text = config.serialize()
print text
"""

"""
print "\n\n\n\n"

print 'Default Display',config.getDefaultDisplay()
print 'getActiveDisplays',config.getActiveDisplays()
print 'getActiveViews',config.getActiveViews()

print ""
for display in config.getDisplays():
    print 'Display:',display
    print '    default',config.getDefaultView(display)
    
    for views in config.getViews(display):
        print '    views',views
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

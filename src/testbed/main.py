"""
Test the Python bindings.
"""
import PyOpenColorSpace as OCS

print ""
print "PyOCS:", OCS.__file__
print "OCS:",dir(OCS)
print ""

#import pydoc
#print pydoc.render_doc(OCS)

#print dir(OCS.Config)
#c = OCS.Config.CreateFromEnv()
#print 'c',c
#print 'isEditable', c.isEditable()
#print "resourcePath: '%s' " % (c.getResourcePath(),)
#c = OCS.GetCurrentConfig()
#print 'isEditable', c.isEditable()
#print "resourcePath: '%s' " % (c.getResourcePath(),)
#
#OCS.SetCurrentConfig(c)
#c = OCS.GetCurrentConfig()
#print ''
#print "ColorSpaces"
#for cs in c.getColorSpaces():
#    print "%s %s %s" % (cs.getName(), cs.getFamily(), cs.getBitDepth())


# Create a new config
config = OCS.Config()

print 'isEditable', config.isEditable()
print "resourcePath: '%s' " % (config.getResourcePath(),)

# Add a colorspace
cs = OCS.ColorSpace()
cs.setName("lnh")
cs.setFamily("ln")
cs.setBitDepth(OCS.BIT_DEPTH_F16)
cs.setIsData(False)
cs.setHWAllocation(OCS.HW_ALLOCATION_LG2)
cs.setHWMin(-16.0)
cs.setHWMax(6.0)

config.addColorSpace(cs)
config.setColorSpaceForRole(OCS.ROLE_SCENE_LINEAR, cs.getName())
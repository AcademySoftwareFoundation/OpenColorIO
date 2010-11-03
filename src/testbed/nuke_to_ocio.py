import shutil, os

"""

 >>> nuke.Lut('REDSpace').toFloat([x/5.0 for x in range(5)])
# Result: [0.0, 0.64340996742248535, 0.86640632152557373, 
0.9471660852432251, 0.98195165395736694]

 >>> nuke.Root()['luts'].toScript()
# Result: 'linear {}\nsRGB {}\nrec709 {}\nCineon {}\nGamma1.8 
{}\nGamma2.2 {}\nPanalog {}\nREDLog {}\nViperLog {}\nREDSpace {}'
"""

def WriteSpiLut1D(filename, xmin, xmax, yvalues):
    f = file(filename,'w')
    f.write("Version 1\n")
    f.write("From %f %f\n" % (xmin, xmax))
    f.write("Length %d\n" % len(yvalues))
    f.write("Components 1\n")
    f.write("{\n")
    for val in yvalues:
        f.write("    %s\n" % (repr(val),))
    f.write("}\n")
    f.close()

def Fit(val, oldmin, oldmax, newmin, newmax):
    return (val - oldmin) / (oldmax - oldmin) * (newmax-newmin) + newmin

config = OCIO.Config()
config.setResourcePath("luts")

OUTPUT_DIR = '/tmp/nuke_export'
try: os.mkdir(OUTPUT_DIR)
except OSError: pass

LUT_DIR = os.path.join(OUTPUT_DIR, 'luts')
try: os.mkdir(LUT_DIR)
except OSError: pass

# Linear
if True:
    cs = OCIO.ColorSpace()
    cs.setBitDepth(OCIO.Constants.BIT_DEPTH_F32)
    cs.setName('linear')
    cs.setFamily('linear')
    cs.setIsData(False)
    cs.setGpuMin(-8.0)
    cs.setGpuMax(8.0)
    cs.setGpuAllocation(OCIO.Constants.GPU_ALLOCATION_LG2)
    config.addColorSpace(cs)

def Create1DSampledColorSpace(name, lutmin, lutmax, lutentries):
    cs = OCIO.ColorSpace()
    cs.setBitDepth(OCIO.Constants.BIT_DEPTH_F32)
    cs.setName(name)
    cs.setFamily(name)
    cs.setIsData(False)
    cs.setGpuMin(0.0)
    cs.setGpuMax(1.0)
    cs.setGpuAllocation(OCIO.Constants.GPU_ALLOCATION_UNIFORM)
    
    lutfilename = name.lower() + '.spi1d'
    xmin = lutmin
    xmax = lutmax
    numentries = lutentries
    xvalues = [ Fit(x, 0.0, float(numentries-1.0), xmin, xmax) for x in xrange(numentries) ]
    yvalues = nuke.Lut(name).fromFloat(xvalues)
    WriteSpiLut1D(os.path.join(LUT_DIR, lutfilename), xmin, xmax, yvalues)
    
    transform = OCIO.FileTransform()
    transform.setSrc(lutfilename)
    transform.setInterpolation( OCIO.Constants.INTERP_LINEAR )
    cs.setTransform(transform, OCIO.Constants.COLORSPACE_DIR_TO_REFERENCE)
    
    return cs

config.addColorSpace( Create1DSampledColorSpace('sRGB', 0.0, 1.0, 2**12) )
config.addColorSpace( Create1DSampledColorSpace('rec709', 0.0, 1.0, 2**12) )
config.addColorSpace( Create1DSampledColorSpace('Cineon', 0.0, 1.0, 2**12) )
config.addColorSpace( Create1DSampledColorSpace('Gamma1.8', 0.0, 1.0, 2**12) )
config.addColorSpace( Create1DSampledColorSpace('Gamma2.2', 0.0, 1.0, 2**12) )
config.addColorSpace( Create1DSampledColorSpace('Panalog', 0.0, 1.0, 2**12) )
config.addColorSpace( Create1DSampledColorSpace('REDLog', 0.0, 1.0, 2**12) )
config.addColorSpace( Create1DSampledColorSpace('ViperLog', 0.0, 1.0, 2**12) )
config.addColorSpace( Create1DSampledColorSpace('REDSpace', 0.0, 1.0, 2**12) )

# data
if True:
    cs = OCIO.ColorSpace()
    cs.setBitDepth(OCIO.Constants.BIT_DEPTH_F32)
    cs.setName('data')
    cs.setFamily('data')
    cs.setIsData(True)
    cs.setGpuMin(0.0)
    cs.setGpuMax(1.0)
    cs.setGpuAllocation(OCIO.Constants.GPU_ALLOCATION_UNIFORM)
    
    config.addColorSpace(cs)

# Set the roles
config.setRole(OCIO.Constants.ROLE_SCENE_LINEAR, "linear")
config.setRole(OCIO.Constants.ROLE_REFERENCE, "linear")
config.setRole(OCIO.Constants.ROLE_COLOR_TIMING, "cineon")
config.setRole(OCIO.Constants.ROLE_COMPOSITING_LOG, "cineon")
config.setRole(OCIO.Constants.ROLE_COLOR_PICKING,"sRGB")
config.setRole(OCIO.Constants.ROLE_DATA,"data")
config.setRole(OCIO.Constants.ROLE_DEFAULT,"data")

# Add display device
for name,colorspace in [ ['None','data'], ['sRGB','sRGB'], ['rec709','rec709']]:
    config.addDisplayDevice('default',name,colorspace)

## Write the result
outputfilename = os.path.join(OUTPUT_DIR, 'config.ocio')
f = file(outputfilename,"w")
f.write(config.serialize())
f.close()
print "Wrote",outputfilename

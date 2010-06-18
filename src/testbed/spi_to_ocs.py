#!/usr/bin/env python

import sys, os, subprocess, shutil
import SpImport

XmlIO = SpImport.SpComp2("PyXmlIO", 1)

import PyOpenColorSpace as OCS

if len(sys.argv) < 3:
    print """
    Usage: spi_to_ocs <color_v7_config.xml> <output_dir>
    
    Convert a proprietary SPI color configuration format (v7 xml) to an OCS config.
    
    env PYTHONPATH=/net/homedirs/jeremys/git/OpenColorSpace/build/ ./src/testbed/spi_to_ocs.py /shots/grn/home/lib/lut/colorspaces.xml /mcp/config
    
    """
    sys.exit(1)

print ""
print "PyOCS:", OCS.__file__
print ""

def GetFileMD5(fname):
    if not os.path.exists(fname):
        raise TypeError("File %s does not exist.", fname)
    process = subprocess.Popen(['md5sum',fname], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    returncode = process.wait()
    if returncode:
        raise TypeError("Could not get cksum on file %s" % fname)
    return process.stdout.read().split()[0]


################################################################################

def BuildConfigFromXMLElement(element, lutDir):
    assert(element.getElementType() == 'color_config')
    
    config = OCS.Config()
    
    # Convert roles
    for oldRole, newRole in [ ("linear", OCS.ROLE_SCENE_LINEAR),
                              ("colortiming", OCS.ROLE_COLOR_TIMING),
                              ("spm", "spi.spm"),
                              ("picking", OCS.ROLE_COLOR_PICKING),
                              ("data", OCS.ROLE_DATA),
                              ("log", OCS.ROLE_COMPOSITING_LOG) ]:
        if element.hasAttr(oldRole):
            config.setColorSpaceForRole(newRole, element.getAttr(oldRole))
    
    for i in xrange(element.getNumberOfChildren()):
        child = element.getChild(i)
        if child.getElementType() == 'colorspace':
            colorspace = BuildColorspaceFromXMLElement(child, lutDir)
            config.addColorSpace(colorspace)
        else:
            print 'TODO: handle config child',child.getElementType()
    
    return config

def BuildColorTransform(elementArray, lutDir):
    group = OCS.GroupTransform()
    
    for element in elementArray:
        attrDict = element.getAttrDict()
        
        if element.getElementType() == 'null':
            continue
        
        elif element.getElementType() in ('lut', 'colormatrix'):
            transform = OCS.FileTransform()
            
            fname = attrDict.pop('file')
            if not os.path.exists(fname):
                raise Exception("File %s does not exist." % (fname))
            
            # TODO: COPY AND RENAME FILE!
            base, extension = os.path.basename(fname).rsplit('.',1)
            if extension == 'lut':
                extension = 'spi1d'
            elif extension == 'lut3d':
                extension = 'spi3d'
            elif extension == 'dat':
                extension = 'spimtx'
            
            newname = os.path.join(lutDir, '%s.%s' % (base, extension))
            if os.path.exists( newname ):
                cksum1 = GetFileMD5(newname) 
                cksum2 = GetFileMD5(fname) 
                if cksum1 != cksum2:
                    raise Exception("Duplicate files with different contents %s %s." % (newname, fname))
                print '    matched %s %s' % (cksum1, fname)
            else:
                shutil.copy(fname, newname)
                print '    copied %s -> %s' % (fname,newname)
            
            transform.setSrc('%s.%s' % (base, extension))
            
            direction = attrDict.pop('direction')
            if direction == 'inverse':
                transform.setDirection(OCS.TRANSFORM_DIR_INVERSE)
            else:
                transform.setDirection(OCS.TRANSFORM_DIR_FORWARD)
            
            interp = attrDict.pop('interpolation', None)
            if interp == 'linear':
                transform.setInterpolation(OCS.INTERP_LINEAR)
            else:
                transform.setInterpolation(OCS.INTERP_NEAREST)
            
            if attrDict:
                print 'TODO: Handle attrs',attrDict, element.getElementType()
            
            group.push_back(transform)
        
        else:
            print 'Unknown color transform',element.getElementType()
    
    return group

def BuildColorspaceFromXMLElement(element, lutDir):
    attrDict = element.getAttrDict()
    
    cs = OCS.ColorSpace()
    cs.setName( attrDict.pop('name') )
    cs.setFamily( attrDict.pop('prefix') )
    
    if attrDict.pop('type', None) == 'data':
        cs.setIsData(True)
    
    bitdepth = attrDict.pop('bitdepth')
    if bitdepth == '8':
        cs.setBitDepth(OCS.BIT_DEPTH_UINT8)
    elif bitdepth == '10':
        cs.setBitDepth(OCS.BIT_DEPTH_UINT10)
    elif bitdepth == '12':
        cs.setBitDepth(OCS.BIT_DEPTH_UINT12)
    elif bitdepth == '16':
        cs.setBitDepth(OCS.BIT_DEPTH_UINT16)
    elif bitdepth == '0':
        cs.setBitDepth(OCS.BIT_DEPTH_F32)
    else:
        #cs.setBitDepth(OCS.BIT_DEPTH_UNKNOWN)
        raise RuntimeError("Unknown bit depth")
    
    gpuallocation = attrDict.pop('gpuallocation', None)
    if gpuallocation is None:
        pass
    elif gpuallocation == 'log2':
        cs.setHWAllocation(OCS.HW_ALLOCATION_LG2)
    elif gpuallocation == 'uniform':
        cs.setHWAllocation(OCS.HW_ALLOCATION_UNIFORM)
    else:
        #cs.setHWAllocation(OCS.HW_ALLOCATION_UNKNOWN)
        raise RuntimeError("Unknown bit allocation")
    
    gpumin = attrDict.pop('gpumin', None)
    if gpumin is not None:
        cs.setHWMin(float(gpumin))
    gpumax = attrDict.pop('gpumax', None)
    if gpumax is not None:
        cs.setHWMax(float(gpumax))
    
    if attrDict:
        print 'TODO: Handle colorspace attrs',attrDict, cs.getName()
    
    element_toref = None
    element_fromref = None
    
    for i in xrange(element.getNumberOfChildren()):
        child = element.getChild(i)
        if child.getElementType() == 'to_linear':
            element_toref = child
        elif child.getElementType() == 'from_linear':
            element_fromref = child
        else:
            print 'TODO: handle colorspace child element',child.getElementType()
    
    toref, fromref = [], []
    
    if element_toref is not None:
        toref = [element_toref.getChild(i) for i in xrange(element_toref.getNumberOfChildren())]
    if element_fromref is not None:
        fromref = [element_fromref.getChild(i) for i in xrange(element_fromref.getNumberOfChildren())]
    
    # This assumes transforms, if both directions are defined, are perfect inverses of each other.
    if len(toref) >= len(fromref):
        transform = BuildColorTransform(toref, lutDir)
        cs.setTransform(transform, OCS.COLORSPACE_DIR_TO_REFERENCE)
    else:
        transform = BuildColorTransform(fromref, lutDir)
        cs.setTransform(transform, OCS.COLORSPACE_DIR_FROM_REFERENCE)
    
    return cs




################################################################################

INPUT_CONFIG = sys.argv[1]
OUTPUT_DIR = sys.argv[2]
LUT_DIR = os.path.join(sys.argv[2] + '/luts')

try: os.makedirs(OUTPUT_DIR)
except: pass
try: os.makedirs(LUT_DIR)
except: pass

p = XmlIO.Parser()
element = p.parse(INPUT_CONFIG)
config = BuildConfigFromXMLElement( element, LUT_DIR)
config.setResourcePath('luts')

OUTPUT_FILE = os.path.join(OUTPUT_DIR, 'config.ocs')
print ''
print 'Writing',OUTPUT_FILE

f = file(OUTPUT_FILE,'w')
f.write(config.getXML())
f.close()

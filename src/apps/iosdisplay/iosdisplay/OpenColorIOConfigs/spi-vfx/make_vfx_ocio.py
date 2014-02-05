#!/usr/bin/env python


import  math, os, sys
import SpImport
OCIO = SpImport.SpComp2("PyOpenColorIO",2)

print "OCIO",OCIO.version

outputfilename = "config.ocio"

config = OCIO.Config()

LUT_SEARCH_PATH = ['luts']
config.setSearchPath(':'.join(LUT_SEARCH_PATH))


# Set roles
config.setRole(OCIO.Constants.ROLE_SCENE_LINEAR, "lnf")
config.setRole(OCIO.Constants.ROLE_REFERENCE, "lnf")
config.setRole(OCIO.Constants.ROLE_COLOR_TIMING, "lg10")
config.setRole(OCIO.Constants.ROLE_COMPOSITING_LOG, "lgf")
config.setRole(OCIO.Constants.ROLE_COLOR_PICKING,"cpf")
config.setRole(OCIO.Constants.ROLE_DATA,"ncf")
config.setRole(OCIO.Constants.ROLE_DEFAULT,"ncf")
config.setRole(OCIO.Constants.ROLE_MATTE_PAINT,"vd8")
config.setRole(OCIO.Constants.ROLE_TEXTURE_PAINT,"dt16")
## Scene Linear ###############################################################

cs = OCIO.ColorSpace(family='ln',name='lnf')
cs.setDescription("lnf :  linear show space")
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_F32)
cs.setAllocationVars([-15.0,6.0])
cs.setAllocation(OCIO.Constants.ALLOCATION_LG2)
config.addColorSpace(cs)

cs = OCIO.ColorSpace(family='ln',name='lnh')
cs.setDescription("lnh :  linear show space")
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_F16)
cs.setAllocationVars([-15.0,6.0])
cs.setAllocation(OCIO.Constants.ALLOCATION_LG2)
config.addColorSpace(cs)

cs = OCIO.ColorSpace(family='ln',name='ln16')
cs.setDescription("ln16 : linear show space")
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_UINT16)
cs.setAllocationVars([-15.0,0.0])
cs.setAllocation(OCIO.Constants.ALLOCATION_LG2)
config.addColorSpace(cs)


## Log ########################################################################

cs = OCIO.ColorSpace(family='lg',name='lg16')
cs.setDescription("lg16 : conversion from film log ")
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_UINT16)
t = OCIO.FileTransform('lg16.spi1d',interpolation=OCIO.Constants.INTERP_NEAREST )
cs.setTransform(t, OCIO.Constants.COLORSPACE_DIR_TO_REFERENCE)
config.addColorSpace(cs)

cs = OCIO.ColorSpace(family='lg',name='lg10')
cs.setDescription("lg10 : conversion from film log")
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_UINT10)
t = OCIO.FileTransform('lg10.spi1d',interpolation=OCIO.Constants.INTERP_NEAREST )
cs.setTransform(t, OCIO.Constants.COLORSPACE_DIR_TO_REFERENCE)
config.addColorSpace(cs)

cs = OCIO.ColorSpace(family='lg',name='lgf')
cs.setDescription("lgf : conversion from film log")
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_F32)
cs.setAllocationVars([-0.25,1.5])
t = OCIO.FileTransform('lgf.spi1d',interpolation=OCIO.Constants.INTERP_LINEAR)
cs.setTransform(t, OCIO.Constants.COLORSPACE_DIR_TO_REFERENCE)
config.addColorSpace(cs)



## PANALOG ########################################################################

cs = OCIO.ColorSpace(family='gn',name='gn10')
cs.setDescription("gn10 :conversion from Panalog")
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_UINT10)
t = OCIO.FileTransform('gn10.spi1d',interpolation=OCIO.Constants.INTERP_NEAREST)
cs.setTransform(t, OCIO.Constants.COLORSPACE_DIR_TO_REFERENCE)
config.addColorSpace(cs)


## VD ########################################################################

cs = OCIO.ColorSpace(family='vd',name='vd16')
cs.setDescription("vd16 :conversion from a gamma 2.2 ")
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_UINT16)
groupTransform = OCIO.GroupTransform()
groupTransform.push_back(OCIO.FileTransform('version_8_whitebalanced.spimtx',direction=OCIO.Constants.TRANSFORM_DIR_INVERSE))
groupTransform.push_back(OCIO.FileTransform('vd16.spi1d',interpolation=OCIO.Constants.INTERP_NEAREST ))
cs.setTransform(groupTransform, OCIO.Constants.COLORSPACE_DIR_TO_REFERENCE)
config.addColorSpace(cs)

cs = OCIO.ColorSpace(family='vd',name='vd10')
cs.setDescription("vd10 :conversion from a gamma 2.2 ")
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_UINT10)
groupTransform = OCIO.GroupTransform()
groupTransform.push_back(OCIO.FileTransform('version_8_whitebalanced.spimtx',direction=OCIO.Constants.TRANSFORM_DIR_INVERSE))
groupTransform.push_back(OCIO.FileTransform('vd10.spi1d',interpolation=OCIO.Constants.INTERP_NEAREST ))
cs.setTransform(groupTransform, OCIO.Constants.COLORSPACE_DIR_TO_REFERENCE)
config.addColorSpace(cs)

cs = OCIO.ColorSpace(family='vd',name='vd8')
cs.setDescription("vd8 :conversion from a gamma 2.2")
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_UINT8)
groupTransform = OCIO.GroupTransform()
groupTransform.push_back(OCIO.FileTransform('version_8_whitebalanced.spimtx',direction=OCIO.Constants.TRANSFORM_DIR_INVERSE))
groupTransform.push_back(OCIO.FileTransform('vd8.spi1d',interpolation=OCIO.Constants.INTERP_NEAREST ))
cs.setTransform(groupTransform, OCIO.Constants.COLORSPACE_DIR_TO_REFERENCE)
config.addColorSpace(cs)

# REC709 CONVERSIONS#############################################################################
cs = OCIO.ColorSpace(family='hd',name='hd10')
cs.setDescription("hd10 : conversion from REC709")
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_UINT10)
groupTransform = OCIO.GroupTransform()
groupTransform.push_back(OCIO.FileTransform('hdOffset.spimtx',interpolation=OCIO.Constants.INTERP_NEAREST ,direction=OCIO.Constants.TRANSFORM_DIR_INVERSE))
groupTransform.push_back(OCIO.ColorSpaceTransform(src='vd16',dst='lnf'))
cs.setTransform(groupTransform, OCIO.Constants.COLORSPACE_DIR_TO_REFERENCE)
config.addColorSpace(cs)


## TEXTURE PUBLISHING ########################################################################
"""
cs = OCIO.ColorSpace(family='dt',name='dt8')
cs.setDescription("dt8 :conversion for diffuse texture")
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_UINT8)
groupTransform = OCIO.GroupTransform()
groupTransform.push_back(OCIO.FileTransform('diffuseTextureMultiplier.spimtx',interpolation=OCIO.Constants.INTERP_NEAREST ,direction=OCIO.Constants.TRANSFORM_DIR_FORWARD))
groupTransform.push_back(OCIO.ColorSpaceTransform(dst='lnf',src='vd16'))
cs.setTransform(groupTransform, OCIO.Constants.COLORSPACE_DIR_TO_REFERENCE)
config.addColorSpace(cs)
"""
cs = OCIO.ColorSpace(family='dt',name='dt16')
cs.setDescription("dt16 :conversion for diffuse texture")
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_UINT16)
groupTransform = OCIO.GroupTransform()
groupTransform.push_back(OCIO.FileTransform('diffuseTextureMultiplier.spimtx',interpolation=OCIO.Constants.INTERP_NEAREST ,direction=OCIO.Constants.TRANSFORM_DIR_FORWARD))
groupTransform.push_back(OCIO.ColorSpaceTransform(dst='lnf',src='vd16'))
cs.setTransform(groupTransform, OCIO.Constants.COLORSPACE_DIR_TO_REFERENCE)
config.addColorSpace(cs)


## COLOR PICKER ########################################################################

cs = OCIO.ColorSpace(family='cp',name='cpf')
cs.setDescription("cpf :video like conversion used for color picking ") 
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_F32)
t=OCIO.FileTransform(src='cpf.spi1d',interpolation=OCIO.Constants.INTERP_LINEAR )
cs.setTransform(t, OCIO.Constants.COLORSPACE_DIR_TO_REFERENCE)
config.addColorSpace(cs)



## DATA ########################################################################


cs = OCIO.ColorSpace(family='nc',name='nc8')
cs.setDescription("nc8 :nc,Non-color used to store non-color data such as depth or surface normals")
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_UINT8)
cs.setIsData(True)
config.addColorSpace(cs)

cs = OCIO.ColorSpace(family='nc',name='nc10')
cs.setDescription("nc10 :nc,Non-color used to store non-color data such as depth or surface normals")
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_UINT10)
cs.setIsData(True)
config.addColorSpace(cs)

cs = OCIO.ColorSpace(family='nc',name='nc16')
cs.setDescription("nc16 :nc,Non-color used to store non-color data such as depth or surface normals")
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_UINT16)
cs.setIsData(True)
config.addColorSpace(cs)

cs = OCIO.ColorSpace(family='nc',name='ncf')
cs.setDescription("ncf :nc,Non-color used to store non-color data such as depth or surface normals")
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_F32)
cs.setIsData(True)
config.addColorSpace(cs)

## DISPLAY SPACES ##################################################################

cs = OCIO.ColorSpace(family='srgb',name='srgb8')
cs.setDescription("srgb8 :rgb display space for the srgb standard.")
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_UINT8)
groupTransform = OCIO.GroupTransform()
groupTransform.push_back(OCIO.ColorSpaceTransform(src='lnf', dst='lg10'))
groupTransform.push_back(OCIO.FileTransform('spi_ocio_srgb_test.spi3d',interpolation=OCIO.Constants.INTERP_LINEAR))
cs.setTransform(groupTransform, OCIO.Constants.COLORSPACE_DIR_FROM_REFERENCE)
config.addColorSpace(cs)

cs = OCIO.ColorSpace(family='p3dci',name='p3dci8')
cs.setDescription("p3dci8 :rgb display space for gamma 2.6 P3 projection.")
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_UINT8)
groupTransform = OCIO.GroupTransform()
groupTransform.push_back(OCIO.ColorSpaceTransform(src='lnf', dst='lg10'))
groupTransform.push_back( OCIO.FileTransform('colorworks_filmlg_to_p3.3dl',interpolation=OCIO.Constants.INTERP_LINEAR))
cs.setTransform(groupTransform, OCIO.Constants.COLORSPACE_DIR_FROM_REFERENCE)
config.addColorSpace(cs)

cs = OCIO.ColorSpace(family='xyz',name='xyz16')
cs.setDescription("xyz16 :Conversion for  DCP creation.")
cs.setBitDepth(OCIO.Constants.BIT_DEPTH_UINT16)
groupTransform = OCIO.GroupTransform()
groupTransform.push_back(OCIO.ColorSpaceTransform(src='lnf', dst='p3dci8'))
groupTransform.push_back(OCIO.ExponentTransform([2.6,2.6,2.6,1.0]))
groupTransform.push_back(OCIO.FileTransform('p3_to_xyz16_corrected_wp.spimtx'))
groupTransform.push_back(OCIO.ExponentTransform([2.6,2.6,2.6,1.0],direction=OCIO.Constants.TRANSFORM_DIR_INVERSE))
cs.setTransform(groupTransform, OCIO.Constants.COLORSPACE_DIR_FROM_REFERENCE)
config.addColorSpace(cs)

## DISPLAY SPACES ##################################################################

for name,colorspace in [ ['Film','srgb8'], ['Log','lg10'], ['Raw','nc10']]:
    config.addDisplay('sRGB',name,colorspace)

for name,colorspace in [ ['Film','p3dci8'], ['Log','lg10'], ['Raw','nc10']]:
    config.addDisplay('DCIP3',name,colorspace)
    

config.setActiveViews(','.join(['Film','Log','Raw']))
config.setActiveDisplays(','.join(['sRGB','DCIP3']))

try:
    config.sanityCheck()
except Exception,e:
    print e
    print "Configuration was not written due to a failed Sanity Check"
    sys.exit()
else:    
    f = file(outputfilename,"w")
    f.write(config.serialize())
    f.close()
    print "Wrote",outputfilename

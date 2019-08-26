# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import SpImport
OCIO = SpImport.SpComp2("PyOpenColorIO",2)
#import PyOpenColorIO as OCIO

c = OCIO.GetCurrentConfig()

processor = c.getProcessor(OCIO.FileTransform('woz_qt_to_mxfva.spi1d', interpolation=OCIO.Constants.INTERP_LINEAR))

node = nuke.createNode("ColorLookup")
node.setName("woz_post_film")
knob = node['lut']
knob.removeKeyAt(0.0, 1)
knob.removeKeyAt(1.0, 1)
knob.removeKeyAt(0.0, 2)
knob.removeKeyAt(1.0, 2)
knob.removeKeyAt(0.0, 3)
knob.removeKeyAt(1.0, 3)


SIZE = 11
for i in xrange(SIZE):
    x = i/(SIZE-1.0)
    
    y = processor.applyRGB((x,x,x))
    
    knob.setValueAt(y[0],x, 1)
    knob.setValueAt(y[1],x, 2)
    knob.setValueAt(y[2],x, 3)


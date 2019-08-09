# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

#import SpImport
#OCIO = SpImport.SpComp2("PyOpenColorIO", 2)

c = OCIO.GetCurrentConfig()

cs_to_lin = c.getProcessor(OCIO.Constants.ROLE_COMPOSITING_LOG, OCIO.Constants.ROLE_SCENE_LINEAR)

node = nuke.createNode("ColorLookup")
node.setName("sm2_slg_to_ln")
knob = node['lut']
knob.removeKeyAt(0.0)
knob.removeKeyAt(1.0)

node2 = nuke.createNode("ColorLookup")
node2.setName("sm2_ln_to_slg")
knob2 = node2['lut']
knob2.removeKeyAt(0.0)
knob2.removeKeyAt(1.0)


def Fit(value, fromMin, fromMax, toMin, toMax):
    if fromMin == fromMax:
        raise ValueError("fromMin == fromMax")
    return (value - fromMin) / (fromMax - fromMin) * (toMax - toMin) + toMin

SIZE = 2**10
for i in xrange(SIZE):
    x = i/(SIZE-1.0)
    
    x = Fit(x, 0.0, 1.0, -0.125, 1.5)
    
    y = cs_to_lin.applyRGB((x,x,x))[1]
    
    knob.setValueAt(y,x)
    knob2.setValueAt(x,y)


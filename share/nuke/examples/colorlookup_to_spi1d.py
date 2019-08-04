# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

def WriteSPI1D(filename, fromMin, fromMax, data):
    f = file(filename,'w')
    f.write("Version 1\n")
    f.write("From %s %s\n" % (fromMin, fromMax))
    f.write("Length %d\n" % len(data))
    f.write("Components 1\n")
    f.write("{\n")
    for value in data:
        f.write("        %s\n" % value)
    f.write("}\n")
    f.close()


knob = nuke.selectedNode()['lut']

SIZE = 2**10

data = []
for i in xrange(SIZE):
    x = i/(SIZE-1.0)
    data.append(knob.getValueAt(x))

WriteSPI1D('/tmp/colorlookup.spi1d', 0.0, 1.0, data)

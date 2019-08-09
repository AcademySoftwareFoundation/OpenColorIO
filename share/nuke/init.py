# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import nuke

EBANNER = "OCIO Error: "
OCIO = None

def load_ocio_plugins():
    """Loads the PyOpenColorIO module and the OCIO-prefixed nodes
    """
    
    global OCIO
    try:
        import PyOpenColorIO as OCIO
    except Exception, e:
        print '%s%s\n%s' % (EBANNER, 'Loading OCIO python module', e)

    allplugs = nuke.plugins(nuke.ALL | nuke.NODIR, "OCIO*.so", "OCIO*.dylib", "OCIO*.dll")

    for p in allplugs:
        try:
            nuke.load(p)
        except Exception, e:
            print '%sLoading OCIO node %s\n%s' % (EBANNER, p, e)


if __name__ == "__main__":
    load_ocio_plugins()

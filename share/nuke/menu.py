import os
import nuke

def ocio_populate_menu():
    """Adds OCIO nodes to a menu in Color
    """

    m_nodes = nuke.toolbar('Nodes')
    m_color = m_nodes.findItem("Color")
    m_ocio = m_color.addMenu("OCIO", icon = "ocio_icon.png")

    allplugs = nuke.plugins(nuke.ALL | nuke.NODIR, "OCIO*.so", "OCIO*.dylib", "OCIO*.dll")

    for fname in allplugs:
        p = os.path.splitext(fname)[0] # strip .so extension
        m_ocio.addCommand(p, lambda p=p: nuke.createNode(p))


def ocio_populate_viewer(remove_nuke_default = True):
    """Registers the a viewer process for each display/transform, and
    sets the default

    Also unregisters the default Nuke viewer processes (sRGB/rec709)
    unless remove_nuke_default is False
    """

    if remove_nuke_default:
        nuke.ViewerProcess.unregister('rec709')
        nuke.ViewerProcess.unregister('sRGB')


    # Formats the display and transform, e.g "Film1D (sRGB"
    DISPLAY_UI_FORMAT = "%(transform)s (%(display)s)"

    import PyOpenColorIO as OCIO
    cfg = OCIO.GetCurrentConfig()

    allDisplays = cfg.getDisplayDeviceNames()

    # For every display, loop over every transform
    for dname in allDisplays:
        allTransforms = cfg.getDisplayTransformNames(dname)

        for xform in allTransforms:
            nuke.ViewerProcess.register(
                name = DISPLAY_UI_FORMAT % {'transform': xform, "display": dname},
                call = nuke.nodes.OCIODisplay,
                args = (),
                kwargs = {"device": dname, "transform": xform})


    defaultDisplay = cfg.getDefaultDisplayDeviceName()
    defaultXform = cfg.getDefaultDisplayTransformName(defaultDisplay)
    
    nuke.knobDefault(
        "Viewer.viewerProcess",
        DISPLAY_UI_FORMAT % {'transform': defaultXform, "display": defaultDisplay})


if __name__ == "__main__":
    ocio_populate_menu()

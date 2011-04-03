import os
import nuke
import ocionuke.cdl


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

    m_utils = m_ocio.addMenu("Utils")
    m_utils.addCommand("Import .ccc to CDL nodes", ocionuke.cdl.import_multiple_from_ccc)
    m_utils.addCommand("Export selected CDL's to .ccc", ocionuke.cdl.export_multiple_to_ccc)


def ocio_populate_viewer(remove_nuke_default = True):
    """Registers the a viewer process for each display/transform, and
    sets the default

    Also unregisters the default Nuke viewer processes (sRGB/rec709)
    unless remove_nuke_default is False
    """
    
    # TODO: How to we unregister all? This assumes no other luts
    # have been registered by other viewer processes
    if remove_nuke_default:
        nuke.ViewerProcess.unregister('rec709')
        nuke.ViewerProcess.unregister('sRGB')


    # Formats the display and transform, e.g "Film1D (sRGB"
    DISPLAY_UI_FORMAT = "%(view)s (%(display)s)"

    import PyOpenColorIO as OCIO
    config = OCIO.GetCurrentConfig()

    # For every display, loop over every transform
    for display in config.getDisplays():
        for view in config.getViews(display):
            nuke.ViewerProcess.register(
                name = DISPLAY_UI_FORMAT % {'view': view, "display": display},
                call = nuke.nodes.OCIODisplay,
                args = (),
                kwargs = {"display": display, "view": view})


    # Get the default display and transform, register it as the
    # default used on Nuke startup
    defaultDisplay = config.getDefaultDisplay()
    defaultView = config.getDefaultView(defaultDisplay)
    
    nuke.knobDefault(
        "Viewer.viewerProcess",
        DISPLAY_UI_FORMAT % {'view': defaultView, "display": defaultDisplay})


if __name__ == "__main__":
    ocio_populate_menu()

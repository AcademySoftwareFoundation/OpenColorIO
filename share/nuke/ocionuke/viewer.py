import nuke


def register_viewers(also_remove = "default"):
    """Registers the a viewer process for each display device/view, and
    sets the default viewer process.

    ``also_remove`` can be set to either:
    
    - "default" to remove the default sRGB/rec709 viewer processes
    - "all" to remove all processes
    - "none" to leave existing viewer processes untouched
    """

    if also_remove not in ("default", "none", "all"):
        raise ValueError("also_remove should be set to 'default', 'none' or 'all'")

    if also_remove == "default":
        nuke.ViewerProcess.unregister('rec709')
        nuke.ViewerProcess.unregister('sRGB')
    elif also_remove == "all":
        # Unregister all processes, but retain the useful "None" option
        for curname in nuke.ViewerProcess.registeredNames():
            nuke.ViewerProcess.unregister(curname)

        nuke.ViewerProcess.register(
            name = "None",
            call = nuke.nodes.ViewerProcess_None,
            args = ())

    # Formats the display and transform, e.g "Film1D (sRGB)"
    DISPLAY_UI_FORMAT = "%(view)s (%(display)s)"

    import PyOpenColorIO as OCIO
    config = OCIO.GetCurrentConfig()

    # For every display, loop over every view
    for display in config.getDisplays():
        for view in config.getViews(display):
            # Register the node
            nuke.ViewerProcess.register(
                name = DISPLAY_UI_FORMAT % {'view': view, "display": display},
                call = nuke.nodes.OCIODisplay,
                args = (),
                kwargs = {"display": display, "view": view})


    # Get the default display and view, set it as the default used on Nuke startup
    defaultDisplay = config.getDefaultDisplay()
    defaultView = config.getDefaultView(defaultDisplay)
    
    nuke.knobDefault(
        "Viewer.viewerProcess",
        DISPLAY_UI_FORMAT % {'view': defaultView, "display": defaultDisplay})

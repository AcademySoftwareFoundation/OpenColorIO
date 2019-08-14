# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

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
        nuke.ViewerProcess.unregister('None')
    elif also_remove == "all":
        # Unregister all processes, including None, which should be defined in config.ocio
        for curname in nuke.ViewerProcess.registeredNames():
            nuke.ViewerProcess.unregister(curname)

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
                kwargs = {"display": display, "view": view, "layer": "all"})


    # Get the default display and view, set it as the default used on Nuke startup
    defaultDisplay = config.getDefaultDisplay()
    defaultView = config.getDefaultView(defaultDisplay)
    
    nuke.knobDefault(
        "Viewer.viewerProcess",
        DISPLAY_UI_FORMAT % {'view': defaultView, "display": defaultDisplay})

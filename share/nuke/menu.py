# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

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
        nodeMenu = nodeClass = os.path.splitext(fname)[0] # strip .so extension
        # Put a space after "OCIO" to match The Foundry's convention (huh?)
        if nodeMenu.startswith("OCIO"):
            nodeMenu = nodeMenu.replace("OCIO", "OCIO ", 1)
        # Only add the item if it doesn't exist
        if not m_ocio.findItem(nodeMenu):
            m_ocio.addCommand(nodeMenu, lambda nodeClass=nodeClass: nuke.createNode(nodeClass))

    m_utils = m_ocio.addMenu("Utils")
    m_utils.addCommand("Import .ccc to CDL nodes", ocionuke.cdl.import_multiple_from_ccc)
    m_utils.addCommand("Export selected CDL's to .ccc", ocionuke.cdl.export_multiple_to_ccc)


if __name__ == "__main__":
    ocio_populate_menu()

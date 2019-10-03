# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

"""Various utilities relating to the OCIOCDLTransform node
"""

import nuke
import PyOpenColorIO as OCIO
import xml.etree.ElementTree as ET


def _node_to_cdltransform(node):
    """From an OCIOCDLTransform node, returns a PyOpenColorIO
    CDLTransform object, which could be used to write XML
    """

    # Color_Knob.value returns single float if control is not
    # expanded, so use value(index=...) to always get three values
    slope = [node['slope'].value(x) for x in range(3)]
    offset = [node['offset'].value(x) for x in range(3)]
    power = [node['power'].value(x) for x in range(3)]
    sat = node['saturation'].value()
    cccid = node['cccid'].value()

    cdl = OCIO.CDLTransform()
    cdl.setSlope(slope)
    cdl.setOffset(offset)
    cdl.setPower(power)
    cdl.setSat(sat)
    cdl.setID(cccid)

    return cdl


def _cdltransform_to_node(cdl, node):
    """From an XML string, populates the parameters on an
    OCIOCDLTransform node
    """

    # Treat "node" as a dictionary of knobs, as the "node" argument could be
    # a the return value of PythonPanel.knob(), as in SelectCCCIDPanel

    node['slope'].setValue(cdl.getSlope())
    node['offset'].setValue(cdl.getOffset())
    node['power'].setValue(cdl.getPower())
    node['saturation'].setValue(cdl.getSat())
    node['cccid'].setValue(cdl.getID())


def _xml_colorcorrection_to_cdltransform(xml, cccposition):
    """From an XML string, return a CDLTransform object, if the cccid
    is absent from the XML the given cccposition will be used instead 
    """
    ccxml = ET.tostring(xml)
    cdl = OCIO.CDLTransform()
    cdl.setXML(ccxml)
    if "cccid" not in xml.getchildren():
	cdl.setID(str(cccposition))

    return cdl

def _xml_to_cdltransforms(xml):
    """Given some XML as a string, returns a list of CDLTransform
    objects for each ColorCorrection (returns a one-item list for a
    .cc file)
    """

    tree = ET.fromstring(xml)

    # Strip away xmlns
    for elem in tree.getiterator():
        if elem.tag.startswith("{"):
            elem.tag = elem.tag.partition("}")[2]

    filetype = tree.tag

    cccposition = 0
    if filetype == "ColorCorrection":
        return [_xml_colorcorrection_to_cdltransform(tree, cccposition)]

    elif filetype in ["ColorCorrectionCollection", "ColorDecisionList"]:
        allcdl = []
        for cc in tree.getchildren():
            if cc.tag == "ColorDecision":
                cc = cc.getchildren()[0] # TODO: something better here
            if cc.tag != "ColorCorrection": continue
            allcdl.append(_xml_colorcorrection_to_cdltransform(cc, cccposition))
            cccposition += 1
        return allcdl

    else:
        raise RuntimeError(
            "The supplied file did not have the correct root element, expected"
            " 'ColorCorrection' or 'ColorCorrectionCollection' or 'ColorDecisionList', got %r" % (filetype))


def _cdltransforms_to_xml(allcc):
    """Given a list of CDLTransform objects, returns an XML string
    """

    root = ET.Element("ColorCorrectionCollection")
    root.attrib['xmlns'] = 'urn:ASC:CDL:v1.2'

    for cc in allcc:
        cur = ET.fromstring(cc.getXML())

        # Strip away xmlns
        for elem in cur.getiterator():
            if elem.tag.startswith("{"):
                elem.tag = elem.tag.partition("}")[2]
        root.append(cur)

    return ET.tostring(root)


def SelectCCCIDPanel(*args, **kwargs):
    # Wrap class definition in a function, so nukescripts.PythonPanel
    # is only accessed when ``SelectCCCIDPanel()`` is called,
    # https://github.com/AcademySoftwareFoundation/OpenColorIO/issues/277

    import nukescripts

    class _SelectCCCIDPanel(nukescripts.PythonPanel):
        """Allows the user to select from a list of CDLTransform
        objects
        """

        def __init__(self, allcdl):
            super(_SelectCCCIDPanel, self).__init__()
            self.available = {}
            for cur in allcdl:
                self.available[cur.getID()] = cur

            self.addKnob(nuke.Enumeration_Knob("cccid", "cccid", self.available.keys()))
            self.addKnob(nuke.Text_Knob("divider"))
            self.addKnob(nuke.Color_Knob("slope"))
            self.addKnob(nuke.Color_Knob("offset"))
            self.addKnob(nuke.Color_Knob("power"))
            self.addKnob(nuke.Double_Knob("saturation"))

        def selected(self):
            return self.available[self.knobs()['cccid'].value()]

        def knobChanged(self, knob):
            """When the user selects a cccid, a grade-preview knobs are set.

            This method is triggered when any knob is changed, which has the
            useful side-effect of preventing changing the preview values, while
            keeping them selectable for copy-and-paste.
            """
            _cdltransform_to_node(self.selected(), self.knobs())

    return _SelectCCCIDPanel(*args, **kwargs)


def export_as_cc(node = None, filename = None):
    """Export a OCIOCDLTransform node as a ColorCorrection XML file
    (.cc)

    If node is None, "nuke.thisNode()" will be used. If filename is
    not specified, the user will be prompted.
    """

    if node is None:
        node = nuke.thisNode()

    cdl = _node_to_cdltransform(node)

    if filename is None:
        ccfilename = nuke.getFilename("Color Correction filename", pattern = "*.cc")
        if ccfilename is None:
            # User clicked cancel
            return

    xml = cdl.getXML()
    print "Writing to %s - contents:\n%s" % (ccfilename, xml)
    open(ccfilename, "w").write(xml)


def import_cc_from_xml(node = None, filename = None):
    """Import a ColorCorrection XML (.cc) into a OCIOCDLTransform node.

    If node is None, "nuke.thisNode()" will be used. If filename is
    not specified, the user will be prompted.
    """

    if node is None:
        node = nuke.thisNode()

    if filename is None:
        ccfilename = nuke.getFilename("Color Correction filename", pattern = "*.cc *.ccc *.cdl")
        if ccfilename is None:
            # User clicked cancel
            return

    xml = open(ccfilename).read()

    allcc = _xml_to_cdltransforms(xml)

    if len(allcc) == 1:
        _cdltransform_to_node(allcc[0], node)
    elif len(allcc) > 1:
        do_selectcccid = nuke.ask(
            "Selected a ColorCorrectionCollection, do you wish to select a ColorCorrection from this file?")
        if do_selectcccid:
            sel = SelectCCCIDPanel(allcc)
            okayed = sel.showModalDialog()
            if okayed:
                cc = sel.selected()
                _cdltransform_to_node(cc, node)
        else:
            return
    else:
        nuke.message("The supplied file (%r) contained no ColorCorrection's" % ccfilename)
        return


def export_multiple_to_ccc(filename = None):
    """Exported all selected OCIOCDLTransform nodes to a
    ColorCorrectionCollection XML file (.ccc)
    """

    if filename is None:
        filename = nuke.getFilename("Color Correction XML file", pattern = "*.cc *.ccc")
        if filename is None:
            # User clicked cancel
            return

    allcc = []
    for node in nuke.selectedNodes("OCIOCDLTransform"):
        allcc.append(_node_to_cdltransform(node))

    xml = _cdltransforms_to_xml(allcc)
    print "Writing %r, contents:\n%s" % (filename, xml)
    open(filename, "w").write(xml)


def import_multiple_from_ccc(filename = None):
    """Import a ColorCorrectionCollection file (.ccc) into multiple
    OCIOCDLTransform nodes. Also creates a single node for a .cc file
    """

    if filename is None:
        filename = nuke.getFilename("Color Correction XML file", pattern = "*.cc *.ccc *.cdl")
        if filename is None:
            # User clicked cancel
            return

    xml = open(filename).read()
    allcc = _xml_to_cdltransforms(xml)

    def _make_node(cdl):
        newnode = nuke.nodes.OCIOCDLTransform(inputs = nuke.selectedNodes()[:1])
        _cdltransform_to_node(cdl, newnode)
        newnode['label'].setValue("id: [value cccid]")

    if len(allcc) > 0:
        for cc in allcc:
            _make_node(cc)
    else:
        nuke.message("The supplied file (%r) contained no ColorCorrection's" % filename)


def select_cccid_for_filetransform(node = None, fileknob = 'file', cccidknob = 'cccid'):
    """Select cccid button for the OCIOFileTransform node, also used
    in OCIOCDLTransform. Presents user with list of cccid's within the
    specified .ccc file, and sets the cccid knob to the selected ID.
    """

    if node is None:
        node = nuke.thisNode()

    filename = node[fileknob].value()

    try:
        xml = open(filename).read()
    except IOError, e:
        nuke.message("Error opening src file: %s" % e)
        raise

    allcc = _xml_to_cdltransforms(xml)

    if len(allcc) == 0:
        nuke.message("The file (%r) contains no ColorCorrection's")
        return

    sel = SelectCCCIDPanel(allcc)
    okayed = sel.showModalDialog()
    if okayed:
        node[cccidknob].setValue(sel.selected().getID())

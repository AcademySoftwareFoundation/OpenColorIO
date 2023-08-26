# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

"""
Convert an ACES Metadata File (AMF) into an OCIO Color Transform File (CTF).

This is a prototype for AMF support in OCIO.  It utilizes a tweaked version of the OCIO v2
ACES Reference config as a database of AMF Transform ID strings and accompanying transforms.

Usage: % python pyocioamf.py <AMF_FILE>

The exported CTF file is written to the same directory as the AMF_FILE and has the same
name, but uses .ctf as the extension.  The CTF file may then be used with any of the
other OCIO tools such as ocioconvert or ociochecklut to apply the AMF color pipeline.
"""

import xml.etree.ElementTree as ET

import PyOpenColorIO as ocio


# Specify the color space name for ACES2065-1 in the ACES config.
ACES = 'ACES - ACES2065-1'
# Setup the namespaces used in the AMF XML file.
NS = {'aces': 'urn:ampas:aces:amf:v1.0', 'cdl': 'urn:ASC:CDL:v1.01'}

def search_colorspaces(config, aces_id):
    """ Search the config for the supplied ACES ID, return the color space name. """
    for cs in config.getColorSpaces():
        desc = cs.getDescription()
        if aces_id in desc:
            return cs.getName()
    return None

def search_viewtransforms(config, aces_id):
    """ Search the config for the supplied ACES ID, return the view transform name. """
    for vt in config.getViewTransforms():
        desc = vt.getDescription()
        if aces_id in desc:
            return vt.getName()
    return None

def search_looktransforms(config, aces_id):
    """ Search the config for the supplied ACES ID, return the look name. """
    for lt in config.getLooks():
        desc = lt.getDescription()
        if aces_id in desc:
            return lt.getName()
    return None

def must_apply(elem, type):
    """ Return True if the 'applied' attribute is not already true. """
    if elem is None:
        return False
    if 'applied' in elem.attrib:
        if elem.attrib['applied'].lower() == 'true': 
            print('Skipping', type, 'Transform that was already applied')
            return False
    return True

def check_lut_path(amf_path, lut_path):
    """ Validate the LUT path and try to fix it if it's a relative path. """
    import os.path
    if not os.path.isfile(lut_path):
        if os.path.isabs(lut_path):
            raise ValueError('File transform refers to path that does not exist: ' + lut_path)
        else:
            # See if the LUT path is relative to where the AMF file is located.
            prefix = os.path.dirname(amf_path)
            abs_path = os.path.join(prefix, lut_path)
            if os.path.isfile(abs_path):
                lut_path = abs_path
            else:
                raise ValueError('File transform refers to path that does not exist: ' + lut_path)

    # TODO: Try some other techniques to find the referenced file.

    # Return the (possibly corrected) path of the LUT.
    return lut_path

def name_ctf_output_file(amf_path):
    """ Return the path to store the resulting CTF file, based on the AMF name. """
    import os.path
    prefix, basename = os.path.split(amf_path)
    fname, ext = os.path.splitext(basename)
    abs_path = os.path.join(prefix, fname + '.ctf')
    return abs_path

def write_ctf(grp, config, amf_path, fname, id):
    """  Take a GroupTransform and some metadata and write a CTF file. """

    fmdg = grp.getFormatMetadata()
    fmdg.setID(id)
    desc = 'Color pipeline for source AMF: \n' + amf_path
    fmdg.addChildElement('Description', desc)

    # Copy entire AMF text into the Info block as documentation.
    # TODO: This doesn't work since it tries to escape all the angle brackets, making it
    #       unreadable, but it would be a nice feature if there's a workaround.
    # try:
    #     f = open(path, 'r')
    #     buf = f.read()
    #     f.close()
    #     fmdg.addChildElement('Info', buf)
    #     #fmdi = fmdg.getChildElements()[0]
    # except:
    #     raise

    grp.write(formatName="Color Transform Format", config=config, fileName=fname)

    print('\nWrote OCIO CTF file:', fname)

def extract_three_floats(elem):
    """ Return three floats extracted from the text of the element. """
    try:
        vals = elem.text.split()
        if len(vals) == 3:
            float_vals = [float(x) for x in vals]
            if len(float_vals) == 3:
                return float_vals
    except:
        print("There was an error parsing CDL values")
        raise
    return None

def parse_cdl(look_elem):
    """ Return the CDL slope, offset, power, and saturation values from a look element. """
    slopes = [1., 1., 1.]
    offsets = [0., 0., 0.]
    powers = [1., 1., 1.]
    sat = 1.
    has_cdl = False

    sop_elem = look_elem.find('./cdl:SOPNode', namespaces=NS)
    if sop_elem is not None:
        slope_elem = sop_elem.find('./cdl:Slope', namespaces=NS)
        if slope_elem is not None:
            slopes = extract_three_floats(slope_elem)
        offset_elem = sop_elem.find('./cdl:Offset', namespaces=NS)
        if offset_elem is not None:
            offsets = extract_three_floats(offset_elem)
        power_elem = sop_elem.find('./cdl:Power', namespaces=NS)
        if power_elem is not None:
            powers = extract_three_floats(power_elem)
        has_cdl = True

    sat_elem = look_elem.find('./cdl:SatNode', namespaces=NS)
    if sat_elem is not None:
        saturation_elem = sat_elem.find('./cdl:Saturation', namespaces=NS)
        if saturation_elem is not None:
            try:
                sat = float(saturation_elem.text)
            except:
                print("There was an error parsing CDL values")
                raise
            has_cdl = True

    return has_cdl, slopes, offsets, powers, sat

def load_cdl_working_space_transform(config, base_path, look_elem, is_to_direc):
    """ Return an OCIO transform for a CDL to/from working space transform. """
    if is_to_direc:
        token = 'aces:toCdlWorkingSpace'
    else:
        token = 'aces:fromCdlWorkingSpace'

    elem = look_elem.find(token, namespaces=NS)
    if elem is not None:
        #
        # ACES transformId case.
        #
        id_elem = elem.find('./aces:transformId', namespaces=NS)
        if id_elem is not None:
            aces_id = id_elem.text

            cs_name = search_colorspaces(config, aces_id)
            if cs_name is not None:
                if is_to_direc:
                    print('  Loading To-CDL-Working-Space Transform from ACES2065-1 to', cs_name)
                    transform = ocio.ColorSpaceTransform(ACES, cs_name, ocio.TRANSFORM_DIR_FORWARD)
                else:
                    print('  Loading From-CDL-Working-Space Transform from', cs_name, 'to ACES2065-1')
                    transform = ocio.ColorSpaceTransform(cs_name, ACES, ocio.TRANSFORM_DIR_FORWARD)
                return transform
            else:
                raise ValueError("Could not find transform for transformId element: " + aces_id)
        #
        # External LUT file case.
        #
        file_elem = elem.find('./aces:file', namespaces=NS)
        if file_elem is not None:
            lut_path = file_elem.text
            if lut_path is not None:
                lut_path = check_lut_path(base_path, lut_path)
                if is_to_direc:
                    print('  Loading To-CDL-Working-Space Transform file:', lut_path)
                else:
                    print('  Loading From-CDL-Working-Space Transform file:', lut_path)
                transform = ocio.FileTransform(lut_path, '', ocio.INTERP_BEST, ocio.TRANSFORM_DIR_FORWARD)
                return transform
    return None

def process_look(config, gt, base_path, look_elem):
    """ Build a look transform and add it to the supplied OCIO group transform. """
    if not must_apply(look_elem, 'Look'):
        return
    #
    # LookTransform ACES transformId case.
    #
    id_elem = look_elem.find('./aces:transformId', namespaces=NS)
    if id_elem is not None:
        aces_id = id_elem.text

        look_name = search_looktransforms(config, aces_id)
        if look_name is not None:
            print('Adding Look Transform:', look_name)
            gt.appendTransform( ocio.LookTransform(ACES, ACES, look_name, False, ocio.TRANSFORM_DIR_FORWARD) )
            return
        else:
            raise ValueError("Could not find transform for transformId element: " + aces_id)
    #
    # LookTransform external LUT file case.
    #
    file_elem = look_elem.find('./aces:file', namespaces=NS)
    if file_elem is not None:
        lut_path = file_elem.text
        if lut_path is not None:
            lut_path = check_lut_path(base_path, lut_path)
            print('Adding Look Transform file:', lut_path)

            # Support CCC ID, if present, to identify which CDL in a CCC file to use.
            ccc_id = ''
            ccref_elem = look_elem.find('./cdl:ColorCorrectionRef', namespaces=NS)
            if ccref_elem is not None:
                ccc_id = ccref_elem.text
                print('Using CCC ID:', ccc_id)

            gt.appendTransform( ocio.FileTransform(lut_path, ccc_id, ocio.INTERP_BEST, ocio.TRANSFORM_DIR_FORWARD) )
            return
    #
    # LookTransform ASC CDL case.
    #
    has_cdl, slopes, offsets, powers, sat = parse_cdl(look_elem)
    if has_cdl:
        ws_elem = look_elem.find('./aces:cdlWorkingSpace', namespaces=NS)
        if ws_elem is None:
            print('Adding CDL Transform')
            gt.appendTransform( ocio.CDLTransform(slopes, offsets, powers, sat) )
            return
        #
        # Attempt to load the working space transforms.
        #
        to_transform = load_cdl_working_space_transform(config, base_path, ws_elem, True)
        from_transform = load_cdl_working_space_transform(config, base_path, ws_elem, False)
        #
        # Handle the four possible scenarios of working space transform availability.
        #
        if (to_transform is None) and (from_transform is None):
            print('Adding CDL Transform')
            gt.appendTransform( ocio.CDLTransform(slopes, offsets, powers, sat) )
        elif (to_transform is not None) and (from_transform is not None):
            print('Adding To-CDL-Working-Space Transform')
            gt.appendTransform(to_transform)
            print('Adding CDL Transform')
            gt.appendTransform( ocio.CDLTransform(slopes, offsets, powers, sat) )
            print('Adding From-CDL-Working-Space Transform')
            gt.appendTransform(from_transform)
        elif to_transform is not None:
            print('Adding To-CDL-Working-Space Transform')
            gt.appendTransform(to_transform)
            print('Adding CDL Transform')
            gt.appendTransform( ocio.CDLTransform(slopes, offsets, powers, sat) )
            print('  Generating From-CDL-Working-Space Transform')
            to_transform.setDirection(ocio.TRANSFORM_DIR_INVERSE)
            print('Adding From-CDL-Working-Space Transform')
            gt.appendTransform(to_transform)
        elif from_transform is not None:
            print('  Generating From-CDL-Working-Space Transform')
            from_transform.setDirection(ocio.TRANSFORM_DIR_INVERSE)
            print('Adding To-CDL-Working-Space Transform')
            gt.appendTransform(from_transform)
            print('Adding CDL Transform')
            gt.appendTransform( ocio.CDLTransform(slopes, offsets, powers, sat) )
            from_transform.setDirection(ocio.TRANSFORM_DIR_FORWARD)
            print('Adding From-CDL-Working-Space Transform')
            gt.appendTransform(from_transform)

def build_output_transform_from_id_elem(config, gt, id_elem, msg, transform_dir):
    """ Add an OCIO transform to the supplied group transform using a transform ID. """
    aces_id = id_elem.text

    dcs_name = search_colorspaces(config, aces_id)
    vt_name = search_viewtransforms(config, aces_id)

    if dcs_name is not None and vt_name is not None:
        print( msg % (dcs_name, vt_name) )
        gt.appendTransform( ocio.DisplayViewTransform(ACES, dcs_name, vt_name, direction=transform_dir) )
    else:
        raise ValueError("Could not process transformId element: " + aces_id)

def load_output_transform(config, gt, base_path, elem, is_inverse):
    """ Add an OCIO transform to the supplied group transform to implement an ACES Output Transform. """
    # Setup some variables based on whether it is an Output or Inverse Output Transform.
    if not is_inverse:
        transform_dir = ocio.TRANSFORM_DIR_FORWARD
        ot_transformId_token = './aces:transformId'
        ot_file_token = './aces:file'
        odt_token = './aces:outputDeviceTransform'
        rrt_token = './aces:referenceRenderingTransform'
        msg = 'Adding Output Transform from ACES2065-1 to display: %s and view: %s'
        file_msg = 'Adding Output Transform LUT file:'
    else:
        transform_dir = ocio.TRANSFORM_DIR_INVERSE
        ot_transformId_token = './aces:inverseOutputTransform/aces:transformId'
        ot_file_token = './aces:inverseOutputTransform/aces:file'
        odt_token = './aces:inverseOutputDeviceTransform'
        rrt_token = './aces:inverseReferenceRenderingTransform'
        msg = 'Adding Inverse Output Transform to ACES2065-1 from display %s and view: %s'
        file_msg = 'Adding Inverse Output Transform LUT file:'
    #
    # OutputTransform ACES transformId case.
    #
    id_elem = elem.find(ot_transformId_token, namespaces=NS)
    if id_elem is not None:
        build_output_transform_from_id_elem(config, gt, id_elem, msg, transform_dir)
        return
    #
    # OutputTransform external LUT file case.
    #
    file_elem = elem.find(ot_file_token, namespaces=NS)
    if file_elem is not None:
        lut_path = file_elem.text
        if lut_path is not None:
            lut_path = check_lut_path(base_path, lut_path)
            print(file_msg, lut_path)
            gt.appendTransform( ocio.FileTransform(lut_path, '', ocio.INTERP_BEST, transform_dir) )
            return
    #
    # Handle referenceRenderingTransform + outputDeviceTransform case.
    #
    odt_elem = elem.find(odt_token, namespaces=NS)
    if odt_elem is not None:
        #
        # ACES transformId case.
        #
        id_elem = odt_elem.find('./aces:transformId', namespaces=NS)
        if id_elem is not None:
            build_output_transform_from_id_elem(config, gt, id_elem, msg, transform_dir)
            return
            # TODO: Could validate that the referenceRenderingTransform element exists and is
            # the expected version (although since there is only one, it is not currently used).
        #
        # External LUT file case.
        #
        file_elem = odt_elem.find('./aces:file', namespaces=NS)
        if file_elem is not None:
            odt_path = file_elem.text
            if odt_path is not None:
                odt_path = check_lut_path(base_path, odt_path)
                odt_transform = ocio.FileTransform(odt_path, '', ocio.INTERP_BEST, transform_dir)

                # NB: Only check for an external file for the RRT if the ODT is also a file.
                rrt_transform = None
                rrt_elem = elem.find(rrt_token, namespaces=NS)
                if rrt_elem is not None:
                    file_elem = rrt_elem.find('./aces:file', namespaces=NS)
                    if file_elem is not None:
                        rrt_path = file_elem.text
                        if rrt_path is not None:
                            rrt_path = check_lut_path(base_path, rrt_path)
                            rrt_transform = ocio.FileTransform(rrt_path, '', ocio.INTERP_BEST, transform_dir)

                if is_inverse:
                    print('Adding Inverse ODT LUT file:', odt_path)
                    gt.appendTransform( odt_transform )
                    if rrt_transform is not None:
                        print('Adding Inverse RRT LUT file:', rrt_path)
                        gt.appendTransform( rrt_transform )
                else:
                    if rrt_transform is not None:
                        print('Adding RRT LUT file:', rrt_path)
                        gt.appendTransform( rrt_transform )
                    print('Adding ODT LUT file:', odt_path)
                    gt.appendTransform( odt_transform )

def load_input_transform(config, gt, base_path, input_elem):
    """ Add an OCIO transform to the supplied group transform to implement an ACES Input Transform. """
    #
    # InputTransform ACES transformId case.
    #
    id_elem = input_elem.find('./aces:transformId', namespaces=NS)
    if id_elem is not None:
        aces_id = id_elem.text

        cs_name = search_colorspaces(config, aces_id)
        if cs_name is not None:
            print('Adding Input Transform from', cs_name, 'to ACES2065-1')
            gt.appendTransform( ocio.ColorSpaceTransform(cs_name, ACES, ocio.TRANSFORM_DIR_FORWARD) )
            return
        else:
            raise ValueError("Could not find transform for transformId element: " + aces_id)
    #
    # InputTransform external LUT file case.
    #
    file_elem = input_elem.find('./aces:file', namespaces=NS)
    if file_elem is not None:
        lut_path = file_elem.text
        if lut_path is not None:
            lut_path = check_lut_path(base_path, lut_path)
            print('Adding Input Transform LUT file:', lut_path)
            gt.appendTransform( ocio.FileTransform(lut_path, '', ocio.INTERP_BEST, ocio.TRANSFORM_DIR_FORWARD) )
            return
    #
    # InputTransform is an inverse OutputTransform case.
    #
    load_output_transform(config, gt, base_path, input_elem, is_inverse=True)

def load_aces_ref_config():
    """ Return an OCIO config object for the ACES reference config required to decode transform IDs. """
    ACES_REF_CONFIG = 'config-aces-reference.yaml'
    import os
    # Get the path to this script, see if the config is in the same directory.
    file_path = os.path.realpath(__file__)
    prefix = os.path.dirname(file_path)
    config_path = os.path.join(prefix, ACES_REF_CONFIG)
    config = ocio.Config().CreateFromFile(config_path)
    # TODO: Try other ways of finding the config.
    return config

def build_ctf(amf_path):
    """ Build a color pipeline that implements the supplied AMF file and write out a CTF file. """
    print('\nProcessing file: ', amf_path, '\n')

    # Use Python to parse the AMF XML file and return an Element Tree object.
    tree = ET.parse(amf_path)
    root = tree.getroot()

    # Clear the OCIO file cache (helpful if someone is editing files in between running this).
    ocio.ClearAllCaches()

    # Load the ACES Reference config that will be used to implement any Transform ID strings
    # encountered in the AMF file.
    config = load_aces_ref_config()

    # Initialize a group transform to hold the results.
    gt = ocio.GroupTransform()

    #
    # Handle the AMF Input Transform.
    #
    input_elem = root.find('./aces:pipeline/aces:inputTransform', namespaces=NS)
    if must_apply(input_elem, 'Input'):
        load_input_transform(config, gt, amf_path, input_elem)
    #
    # Handle all the AMF Look Transforms.
    #
    for look_elem in root.findall('./aces:pipeline/aces:lookTransform', namespaces=NS):
        process_look(config, gt, amf_path, look_elem)
    #
    # Handle the AMF Output Transform.
    #
    output_elem = root.find('./aces:pipeline/aces:outputTransform', namespaces=NS)
    if must_apply(output_elem, 'Output'):
        load_output_transform(config, gt, amf_path, output_elem, is_inverse=False)

    # Print the OCIO transforms in the group transform.
    print('\n', gt)

    # Write the group transform using OCIO's native file format, CTF.
    ctf_path = name_ctf_output_file(amf_path)
    write_ctf(gt, config, amf_path, ctf_path, 'none')


def main():
    import sys
    if len( sys.argv ) != 2:
        raise ValueError( "USAGE: python3 amf_to_ocio.py  <AMF_FILE>" )
    build_ctf( sys.argv[1] )


if __name__=='__main__':
    """ Run the script from the command-line. """
    main()

# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

"""
Convert an ACES Metadata File (AMF) into an OCIO Color Transform File (CTF).

This is a prototype for AMF support in OCIO.  It utilizes a tweaked version of the OCIO v2
ACES Reference config as a database of AMF Transform ID strings and accompanying transforms.

Usage: % python pyocioamf.py <AMF_FILE> [options]

Options:
    --no-idt                      Exclude Input Transform (IDT) from conversion
    --no-lmt                      Exclude Look Transform(s) (LMT) from conversion
    --no-odt                      Exclude Output Transform (ODT) from conversion
    --split-by-working-location   Generate split CTFs based on workingLocation marker (AMF v2.0)

The exported CTF file is written to the same directory as the AMF_FILE and has the same
name, but uses .ctf as the extension.  When using --split-by-working-location, two files
are generated: *_before.ctf (IDT + looks before marker) and *_after.ctf (looks after
marker + ODT).

The CTF file may then be used with any of the other OCIO tools such as ocioconvert or
ociochecklut to apply the AMF color pipeline.
"""

import argparse
import xml.etree.ElementTree as ET

import PyOpenColorIO as ocio


# Default color space name for ACES2065-1 (may vary by config)
# Common names: 'ACES - ACES2065-1' (old configs), 'ACES2065-1' (new configs)
DEFAULT_ACES_NAMES = ['ACES2065-1', 'ACES - ACES2065-1']

# Namespace URIs for AMF versions
AMF_NS_V1 = 'urn:ampas:aces:amf:v1.0'
AMF_NS_V2 = 'urn:ampas:aces:amf:v2.0'
CDL_NS = 'urn:ASC:CDL:v1.01'

# Default namespace dict (will be updated based on detected version)
NS = {'aces': AMF_NS_V1, 'cdl': CDL_NS}


def get_ocio_major_minor_version(config):
    """
    Get the OCIO config profile version as a tuple (major, minor).
    Returns (2, 1) as minimum if version cannot be determined.
    """
    try:
        # getMajorVersion() and getMinorVersion() available in OCIO 2.x
        major = config.getMajorVersion()
        minor = config.getMinorVersion()
        return (major, minor)
    except AttributeError:
        # Fallback for older OCIO versions
        return (2, 1)


def find_aces_colorspace_name(config):
    """
    Find the ACES2065-1 colorspace name in the config.
    Different configs may use different names for the same colorspace.
    """
    # First try the aces_interchange role
    try:
        role_cs = config.getColorSpaceNameByRole('aces_interchange')
        if role_cs:
            return role_cs
    except:
        pass

    # Fall back to searching for common names
    for name in DEFAULT_ACES_NAMES:
        try:
            cs = config.getColorSpace(name)
            if cs is not None:
                return name
        except:
            continue

    # Last resort: search by alias
    for cs in config.getColorSpaces():
        aliases = cs.getAliases() if hasattr(cs, 'getAliases') else []
        for alias in aliases:
            if 'aces2065' in alias.lower() or alias in DEFAULT_ACES_NAMES:
                return cs.getName()

    # Default fallback
    return 'ACES2065-1'


def has_amf_transform_ids_support(config):
    """
    Check if the OCIO config supports the amf_transform_ids attribute (OCIO 2.5+).
    """
    major, minor = get_ocio_major_minor_version(config)
    return (major, minor) >= (2, 5)


def get_amf_transform_ids(element):
    """
    Get AMF Transform IDs from an OCIO config element (colorspace, view transform, look).
    Works with OCIO 2.5+ configs that have the interchange/amf_transform_ids attribute.
    Returns a list of transform ID strings, or empty list if not available.
    """
    try:
        # OCIO 2.5+ API: getInterchangeAttribute("amf_transform_ids")
        # Returns a newline-separated string of transform IDs
        if hasattr(element, 'getInterchangeAttribute'):
            amf_ids_str = element.getInterchangeAttribute("amf_transform_ids")
            if amf_ids_str:
                # Split by newlines and filter out empty strings
                return [id.strip() for id in amf_ids_str.split('\n') if id.strip()]
    except:
        pass
    return []


def detect_amf_version(root):
    """
    Detect AMF version from the root element's namespace or version attribute.
    Returns the version string ('1.0' or '2.0') and updates the global NS dict.
    """
    global NS

    # Check the namespace of the root element
    ns_uri = root.tag.split('}')[0].strip('{') if '}' in root.tag else ''

    # Also check the version attribute
    version_attr = root.attrib.get('version', '')

    if ns_uri == AMF_NS_V2 or version_attr == '2.0':
        NS = {'aces': AMF_NS_V2, 'cdl': CDL_NS}
        return '2.0'
    else:
        NS = {'aces': AMF_NS_V1, 'cdl': CDL_NS}
        return '1.0'

def search_colorspaces(config, aces_id, use_amf_ids=False):
    """
    Search the config for the supplied ACES ID, return the color space name.

    Args:
        config: OCIO config object
        aces_id: ACES Transform ID to search for
        use_amf_ids: If True, search in amf_transform_ids (OCIO 2.5+), else search in description
    """
    for cs in config.getColorSpaces():
        if use_amf_ids:
            # OCIO 2.5+: Search in amf_transform_ids
            amf_ids = get_amf_transform_ids(cs)
            if aces_id in amf_ids:
                return cs.getName()
        else:
            # Legacy: Search in description
            desc = cs.getDescription()
            if aces_id in desc:
                return cs.getName()
    return None


def search_viewtransforms(config, aces_id, use_amf_ids=False):
    """
    Search the config for the supplied ACES ID, return the view transform name.

    Args:
        config: OCIO config object
        aces_id: ACES Transform ID to search for
        use_amf_ids: If True, search in amf_transform_ids (OCIO 2.5+), else search in description
    """
    for vt in config.getViewTransforms():
        if use_amf_ids:
            # OCIO 2.5+: Search in amf_transform_ids
            amf_ids = get_amf_transform_ids(vt)
            if aces_id in amf_ids:
                return vt.getName()
        else:
            # Legacy: Search in description
            desc = vt.getDescription()
            if aces_id in desc:
                return vt.getName()
    return None


def search_looktransforms(config, aces_id, use_amf_ids=False):
    """
    Search the config for the supplied ACES ID, return the look name.

    Args:
        config: OCIO config object
        aces_id: ACES Transform ID to search for
        use_amf_ids: If True, search in amf_transform_ids (OCIO 2.5+), else search in description
    """
    for lt in config.getLooks():
        if use_amf_ids:
            # OCIO 2.5+: Search in amf_transform_ids
            amf_ids = get_amf_transform_ids(lt)
            if aces_id in amf_ids:
                return lt.getName()
        else:
            # Legacy: Search in description
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


def name_ctf_output_file_split(amf_path, suffix):
    """ Return the path for a split CTF file with _before or _after suffix. """
    import os.path
    prefix, basename = os.path.split(amf_path)
    fname, ext = os.path.splitext(basename)
    return os.path.join(prefix, f'{fname}_{suffix}.ctf')


def split_look_transforms_by_working_location(root, ns):
    """
    Split look transforms based on workingLocation marker position.

    Iterates through pipeline children to find the workingLocation marker
    and categorize lookTransform elements as before or after it.

    Args:
        root: XML root element
        ns: Namespace dictionary

    Returns:
        tuple: (before_looks, after_looks, has_working_location)
            - before_looks: List of lookTransform elements before workingLocation
            - after_looks: List of lookTransform elements after workingLocation
            - has_working_location: Boolean indicating if marker was found
    """
    pipeline = root.find('./aces:pipeline', namespaces=ns)
    if pipeline is None:
        return [], [], False

    before_looks = []
    after_looks = []
    found_working_location = False

    for child in pipeline:
        # Get local name by stripping namespace
        local_name = child.tag.split('}')[-1] if '}' in child.tag else child.tag

        if local_name == 'workingLocation':
            found_working_location = True
            continue

        if local_name == 'lookTransform':
            if found_working_location:
                after_looks.append(child)
            else:
                before_looks.append(child)

    return before_looks, after_looks, found_working_location


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

def parse_cdl(look_elem, amf_version):
    """ Return the CDL slope, offset, power, and saturation values from a look element.
        Supports both AMF v1.0 (SOPNode/SatNode) and v2.0 (ASC_SOP/ASC_SAT) element names.
    """
    slopes = [1., 1., 1.]
    offsets = [0., 0., 0.]
    powers = [1., 1., 1.]
    sat = 1.
    has_cdl = False

    # Try v2.0 element names first (ASC_SOP), then fall back to v1.0 (SOPNode)
    sop_elem = look_elem.find('./cdl:ASC_SOP', namespaces=NS)
    if sop_elem is None:
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

    # Try v2.0 element names first (ASC_SAT), then fall back to v1.0 (SatNode)
    sat_elem = look_elem.find('./cdl:ASC_SAT', namespaces=NS)
    if sat_elem is None:
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

def load_cdl_working_space_transform(config, base_path, look_elem, is_to_direc, use_amf_ids=False, aces_cs_name='ACES2065-1'):
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

            cs_name = search_colorspaces(config, aces_id, use_amf_ids)
            if cs_name is not None:
                if is_to_direc:
                    print('  Loading To-CDL-Working-Space Transform from ACES2065-1 to', cs_name)
                    transform = ocio.ColorSpaceTransform(aces_cs_name, cs_name, ocio.TRANSFORM_DIR_FORWARD)
                else:
                    print('  Loading From-CDL-Working-Space Transform from', cs_name, 'to ACES2065-1')
                    transform = ocio.ColorSpaceTransform(cs_name, aces_cs_name, ocio.TRANSFORM_DIR_FORWARD)
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

def process_look(config, gt, base_path, look_elem, amf_version='1.0', use_amf_ids=False, aces_cs_name='ACES2065-1'):
    """ Build a look transform and add it to the supplied OCIO group transform. """
    if not must_apply(look_elem, 'Look'):
        return
    #
    # LookTransform ACES transformId case.
    #
    id_elem = look_elem.find('./aces:transformId', namespaces=NS)
    if id_elem is not None:
        aces_id = id_elem.text

        look_name = search_looktransforms(config, aces_id, use_amf_ids)
        if look_name is not None:
            print('Adding Look Transform:', look_name)
            gt.appendTransform( ocio.LookTransform(aces_cs_name, aces_cs_name, look_name, False, ocio.TRANSFORM_DIR_FORWARD) )
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
    has_cdl, slopes, offsets, powers, sat = parse_cdl(look_elem, amf_version)
    if has_cdl:
        ws_elem = look_elem.find('./aces:cdlWorkingSpace', namespaces=NS)
        if ws_elem is None:
            print('Adding CDL Transform')
            gt.appendTransform( ocio.CDLTransform(slopes, offsets, powers, sat) )
            return
        #
        # Attempt to load the working space transforms.
        #
        to_transform = load_cdl_working_space_transform(config, base_path, ws_elem, True, use_amf_ids, aces_cs_name)
        from_transform = load_cdl_working_space_transform(config, base_path, ws_elem, False, use_amf_ids, aces_cs_name)
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

def build_output_transform_from_id_elem(config, gt, id_elem, msg, transform_dir, use_amf_ids=False, aces_cs_name='ACES2065-1'):
    """ Add an OCIO transform to the supplied group transform using a transform ID. """
    aces_id = id_elem.text

    dcs_name = search_colorspaces(config, aces_id, use_amf_ids)
    vt_name = search_viewtransforms(config, aces_id, use_amf_ids)

    if dcs_name is not None and vt_name is not None:
        print( msg % (dcs_name, vt_name) )
        gt.appendTransform( ocio.DisplayViewTransform(aces_cs_name, dcs_name, vt_name, direction=transform_dir) )
    else:
        raise ValueError("Could not process transformId element: " + aces_id)

def load_output_transform(config, gt, base_path, elem, is_inverse, use_amf_ids=False, aces_cs_name='ACES2065-1'):
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
        build_output_transform_from_id_elem(config, gt, id_elem, msg, transform_dir, use_amf_ids, aces_cs_name)
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
            build_output_transform_from_id_elem(config, gt, id_elem, msg, transform_dir, use_amf_ids, aces_cs_name)
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

def load_input_transform(config, gt, base_path, input_elem, use_amf_ids=False, aces_cs_name='ACES2065-1'):
    """ Add an OCIO transform to the supplied group transform to implement an ACES Input Transform. """
    #
    # InputTransform ACES transformId case.
    #
    id_elem = input_elem.find('./aces:transformId', namespaces=NS)
    if id_elem is not None:
        aces_id = id_elem.text

        cs_name = search_colorspaces(config, aces_id, use_amf_ids)
        if cs_name is not None:
            print('Adding Input Transform from', cs_name, 'to ACES2065-1')
            gt.appendTransform( ocio.ColorSpaceTransform(cs_name, aces_cs_name, ocio.TRANSFORM_DIR_FORWARD) )
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
    load_output_transform(config, gt, base_path, input_elem, is_inverse=True, use_amf_ids=use_amf_ids, aces_cs_name=aces_cs_name)

def load_aces_ref_config(config_path=None):
    """
    Return an OCIO config object for the ACES reference config required to decode transform IDs.

    Args:
        config_path: Optional path to OCIO config file. If None, uses default config.
    """
    import os

    if config_path is None:
        # Default: look for config-aces-reference.yaml in the same directory as this script
        ACES_REF_CONFIG = 'config-aces-reference.yaml'
        file_path = os.path.realpath(__file__)
        prefix = os.path.dirname(file_path)
        config_path = os.path.join(prefix, ACES_REF_CONFIG)

    if not os.path.isfile(config_path):
        raise ValueError(f'OCIO config file not found: {config_path}')

    config = ocio.Config().CreateFromFile(config_path)
    return config

def build_ctf(amf_path, exclude_idt=False, exclude_lmt=False, exclude_odt=False, config_path=None):
    """ Build a color pipeline that implements the supplied AMF file and write out a CTF file.

    Args:
        amf_path: Path to the AMF file
        exclude_idt: If True, exclude Input Transform (IDT) from conversion
        exclude_lmt: If True, exclude Look Transform(s) (LMT) from conversion
        exclude_odt: If True, exclude Output Transform (ODT) from conversion
        config_path: Optional path to OCIO config file. If None, uses default config.
    """
    print('\nProcessing file: ', amf_path, '\n')

    # Use Python to parse the AMF XML file and return an Element Tree object.
    tree = ET.parse(amf_path)
    root = tree.getroot()

    # Detect AMF version and configure namespaces accordingly
    amf_version = detect_amf_version(root)
    print('Detected AMF version:', amf_version)

    # Print exclusion status
    if exclude_idt or exclude_lmt or exclude_odt:
        excluded = []
        if exclude_idt:
            excluded.append('IDT')
        if exclude_lmt:
            excluded.append('LMT')
        if exclude_odt:
            excluded.append('ODT')
        print('Excluding components:', ', '.join(excluded))

    # Clear the OCIO file cache (helpful if someone is editing files in between running this).
    ocio.ClearAllCaches()

    # Load the ACES Reference config that will be used to implement any Transform ID strings
    # encountered in the AMF file.
    config = load_aces_ref_config(config_path)

    # Detect OCIO config version and capabilities
    ocio_version = get_ocio_major_minor_version(config)
    use_amf_ids = has_amf_transform_ids_support(config)
    aces_cs_name = find_aces_colorspace_name(config)

    print(f'OCIO config version: {ocio_version[0]}.{ocio_version[1]}')
    print(f'Using AMF Transform IDs attribute: {use_amf_ids}')
    print(f'ACES2065-1 colorspace name: {aces_cs_name}')

    # Initialize a group transform to hold the results.
    gt = ocio.GroupTransform()

    #
    # Handle the AMF Input Transform.
    #
    if not exclude_idt:
        input_elem = root.find('./aces:pipeline/aces:inputTransform', namespaces=NS)
        if must_apply(input_elem, 'Input'):
            load_input_transform(config, gt, amf_path, input_elem, use_amf_ids, aces_cs_name)
    else:
        print('Skipping Input Transform (IDT) - excluded by user')
    #
    # Handle all the AMF Look Transforms.
    #
    if not exclude_lmt:
        for look_elem in root.findall('./aces:pipeline/aces:lookTransform', namespaces=NS):
            process_look(config, gt, amf_path, look_elem, amf_version, use_amf_ids, aces_cs_name)
    else:
        print('Skipping Look Transform(s) (LMT) - excluded by user')
    #
    # Handle the AMF Output Transform.
    #
    if not exclude_odt:
        output_elem = root.find('./aces:pipeline/aces:outputTransform', namespaces=NS)
        if must_apply(output_elem, 'Output'):
            load_output_transform(config, gt, amf_path, output_elem, is_inverse=False, use_amf_ids=use_amf_ids, aces_cs_name=aces_cs_name)
    else:
        print('Skipping Output Transform (ODT) - excluded by user')

    # Print the OCIO transforms in the group transform.
    print('\n', gt)

    # Write the group transform using OCIO's native file format, CTF.
    ctf_path = name_ctf_output_file(amf_path)
    write_ctf(gt, config, amf_path, ctf_path, 'none')


def build_ctf_split(amf_path, exclude_idt=False, exclude_lmt=False, exclude_odt=False, config_path=None):
    """
    Build two CTF files split by workingLocation marker.

    Generates:
        - *_before.ctf: IDT + lookTransforms before workingLocation
        - *_after.ctf: lookTransforms after workingLocation + ODT

    If no workingLocation is found, falls back to generating a single CTF file.

    Args:
        amf_path: Path to the AMF file
        exclude_idt: If True, exclude Input Transform from before CTF
        exclude_lmt: If True, exclude all Look Transforms from both CTFs
        exclude_odt: If True, exclude Output Transform from after CTF
        config_path: Optional path to OCIO config file
    """
    print('\nProcessing file (split mode): ', amf_path, '\n')

    # Parse AMF XML
    tree = ET.parse(amf_path)
    root = tree.getroot()

    # Detect AMF version
    amf_version = detect_amf_version(root)
    print('Detected AMF version:', amf_version)

    # Check for workingLocation
    before_looks, after_looks, has_working_location = split_look_transforms_by_working_location(root, NS)

    if not has_working_location:
        print('Warning: No workingLocation element found in AMF file.')
        print('Falling back to generating single CTF file.')
        print('')
        build_ctf(amf_path, exclude_idt, exclude_lmt, exclude_odt, config_path)
        return

    print(f'Found workingLocation marker:')
    print(f'  - {len(before_looks)} look transform(s) before workingLocation')
    print(f'  - {len(after_looks)} look transform(s) after workingLocation')

    # Print exclusion status
    if exclude_idt or exclude_lmt or exclude_odt:
        excluded = []
        if exclude_idt:
            excluded.append('IDT')
        if exclude_lmt:
            excluded.append('LMT')
        if exclude_odt:
            excluded.append('ODT')
        print('Excluding components:', ', '.join(excluded))

    # Clear OCIO cache and load config
    ocio.ClearAllCaches()
    config = load_aces_ref_config(config_path)

    # Detect OCIO config capabilities
    ocio_version = get_ocio_major_minor_version(config)
    use_amf_ids = has_amf_transform_ids_support(config)
    aces_cs_name = find_aces_colorspace_name(config)

    print(f'OCIO config version: {ocio_version[0]}.{ocio_version[1]}')
    print(f'Using AMF Transform IDs attribute: {use_amf_ids}')
    print(f'ACES2065-1 colorspace name: {aces_cs_name}')

    # ===== BUILD "BEFORE" CTF =====
    print('\n--- Building BEFORE CTF ---')
    gt_before = ocio.GroupTransform()
    has_before_content = False

    # Add IDT to before CTF
    if not exclude_idt:
        input_elem = root.find('./aces:pipeline/aces:inputTransform', namespaces=NS)
        if must_apply(input_elem, 'Input'):
            load_input_transform(config, gt_before, amf_path, input_elem, use_amf_ids, aces_cs_name)
            has_before_content = True
    else:
        print('Skipping Input Transform (IDT) - excluded by user')

    # Add look transforms BEFORE workingLocation
    if not exclude_lmt:
        for look_elem in before_looks:
            if must_apply(look_elem, 'Look'):
                process_look(config, gt_before, amf_path, look_elem, amf_version, use_amf_ids, aces_cs_name)
                has_before_content = True
    else:
        print('Skipping Look Transform(s) (LMT) - excluded by user')

    # Write before CTF if it has content
    if has_before_content:
        print('\n', gt_before)
        ctf_path_before = name_ctf_output_file_split(amf_path, 'before')
        write_ctf(gt_before, config, amf_path, ctf_path_before, 'before')
    else:
        print('\nNo transforms in BEFORE CTF - skipping file generation')

    # ===== BUILD "AFTER" CTF =====
    print('\n--- Building AFTER CTF ---')
    gt_after = ocio.GroupTransform()
    has_after_content = False

    # Add look transforms AFTER workingLocation
    if not exclude_lmt:
        for look_elem in after_looks:
            if must_apply(look_elem, 'Look'):
                process_look(config, gt_after, amf_path, look_elem, amf_version, use_amf_ids, aces_cs_name)
                has_after_content = True
    else:
        print('Skipping Look Transform(s) (LMT) - excluded by user')

    # Add ODT to after CTF
    if not exclude_odt:
        output_elem = root.find('./aces:pipeline/aces:outputTransform', namespaces=NS)
        if must_apply(output_elem, 'Output'):
            load_output_transform(config, gt_after, amf_path, output_elem, is_inverse=False,
                                  use_amf_ids=use_amf_ids, aces_cs_name=aces_cs_name)
            has_after_content = True
    else:
        print('Skipping Output Transform (ODT) - excluded by user')

    # Write after CTF if it has content
    if has_after_content:
        print('\n', gt_after)
        ctf_path_after = name_ctf_output_file_split(amf_path, 'after')
        write_ctf(gt_after, config, amf_path, ctf_path_after, 'after')
    else:
        print('\nNo transforms in AFTER CTF - skipping file generation')

    # Summary
    print('\n--- Split CTF Generation Complete ---')
    if has_before_content:
        print(f'  Before CTF: {name_ctf_output_file_split(amf_path, "before")}')
    if has_after_content:
        print(f'  After CTF: {name_ctf_output_file_split(amf_path, "after")}')


def main():
    parser = argparse.ArgumentParser(
        description='Convert an ACES Metadata File (AMF) into an OCIO Color Transform File (CTF).',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  python pyocioamf.py example.amf                    # Convert full pipeline
  python pyocioamf.py example.amf --no-idt           # Exclude input transform
  python pyocioamf.py example.amf --no-lmt --no-odt  # Exclude LMT and ODT
  python pyocioamf.py example.amf --config my_config.ocio  # Use custom OCIO config
  python pyocioamf.py example.amf --split-by-working-location  # Generate split CTFs
        '''
    )
    parser.add_argument('amf_file', help='Path to the AMF file to convert')
    parser.add_argument('--config', '-c', dest='config_path',
                        help='Path to OCIO config file (supports OCIO 2.1+, with enhanced support for 2.5+)')
    parser.add_argument('--no-idt', action='store_true',
                        help='Exclude Input Transform (IDT) from conversion')
    parser.add_argument('--no-lmt', action='store_true',
                        help='Exclude Look Transform(s) (LMT) from conversion')
    parser.add_argument('--no-odt', action='store_true',
                        help='Exclude Output Transform (ODT) from conversion')
    parser.add_argument('--split-by-working-location', action='store_true',
                        help='Generate two CTF files split by workingLocation marker (AMF v2.0). '
                             'Creates *_before.ctf (IDT + looks before marker) and '
                             '*_after.ctf (looks after marker + ODT)')

    args = parser.parse_args()

    if args.split_by_working_location:
        build_ctf_split(args.amf_file,
                        exclude_idt=args.no_idt,
                        exclude_lmt=args.no_lmt,
                        exclude_odt=args.no_odt,
                        config_path=args.config_path)
    else:
        build_ctf(args.amf_file,
                  exclude_idt=args.no_idt,
                  exclude_lmt=args.no_lmt,
                  exclude_odt=args.no_odt,
                  config_path=args.config_path)


if __name__=='__main__':
    """ Run the script from the command-line. """
    main()

<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<!-- Copyright Contributors to the OpenColorIO Project. -->

pyocioamf
=========

Script to convert an ACES Metadata File (AMF) into OCIO's native CTF file format.
The CTF may then be used with any of the other OCIO tools to process pixels.

**Supports both AMF v1.0 and v2.0** with automatic version detection.

Example files are provided:
- ``example.amf`` - AMF v1.0 format, references ``example_referenced_lut.clf``
- ``example_v2.amf`` - AMF v2.0 format

The referenced LUT file is in the Academy/ASC Common LUT Format (CLF).

Usage
-----

Basic usage:

```bash
python pyocioamf.py example.amf
```

The CTF file will have the same name as the AMF file but with a .ctf extension.

### Command-Line Options

```bash
# Use a specific OCIO config file
python pyocioamf.py example.amf --config path/to/config.ocio

# Exclude specific components from conversion
python pyocioamf.py example.amf --no-idt           # Exclude input transform
python pyocioamf.py example.amf --no-lmt           # Exclude look transforms
python pyocioamf.py example.amf --no-odt           # Exclude output transform
python pyocioamf.py example.amf --no-lmt --no-odt  # Exclude multiple

# Split CTF generation for AMF v2.0 workingLocation marker
python pyocioamf.py example.amf --split-by-working-location
```

The ``--split-by-working-location`` option generates two CTF files:
- ``*_before.ctf``: IDT + look transforms before the workingLocation marker
- ``*_after.ctf``: Look transforms after the marker + ODT

Dependencies
------------

* PyOpenColorIO

Implementation notes
--------------------

* Automatically detects AMF version (v1.0 or v2.0) from namespace URI or version attribute
* Supports both ASC CDL element naming conventions (``SOPNode``/``SatNode`` and ``ASC_SOP``/``ASC_SAT``)
* Uses a prototype ACES config ``config-aces-reference.yaml`` to interpret the
  ACES Transform IDs encountered in the AMF file. This file should be in the
  same directory as the script.
* Alternatively, use ``--config`` to specify a custom OCIO config file
* For OCIO 2.5+ configs, uses the ``amf_transform_ids`` interchange attribute for transform lookup
* For OCIO 2.1-2.4 configs, searches transform IDs in the description field

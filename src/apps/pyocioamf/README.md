<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<!-- Copyright Contributors to the OpenColorIO Project. -->

pyocioamf
=========

**Work in progress**. Script to convert an ACES Metadata File (AMF) into OCIO's 
native CTF file format.  The CTF may then be used with any of the other OCIO tools 
to process pixels.

An example AMF file named ``example.amf`` is provided.  It references the LUT file
``example_referenced_lut.clf``, which is in the Academy/ASC Common LUT Format (CLF).

Usage
-----

1. Install dependencies on ``PYTHONPATH``
2. Run ``python pyocioamf.py example.amf`` (or any other AMF file).
3. The CTF file will have the same name as the AMF file but with a .ctf extension, in
this case, ``example.ctf``.

Dependencies
------------

* PyOpenColorIO

Implementation notes
--------------------

* Uses a prototype ACES config ``config-aces-reference.yaml`` to interpret the
  ACES Transform IDs encountered in the AMF file.  This file should be in the
  same directory as the script.

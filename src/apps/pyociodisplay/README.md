<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<!-- Copyright Contributors to the OpenColorIO Project. -->

pyociodisplay
=============

**Work in progress**. Minimal image viewer implementation demonstrating use of 
the OCIO GPU renderer in a Python application.

Usage
-----

1. Install dependencies on ``PYTHONPATH``
2. Run ``python pyociodisplay.py``
3. Load image with ``File/Load image...`` menu item

Dependencies
------------

* PyOpenColorIO
* [OpenImageIO (Python bindings)](https://github.com/OpenImageIO/oiio)
* [Imath (Python bindings)](https://github.com/AcademySoftwareFoundation/Imath)
* ``pip install requirements.txt``
  * [numpy](https://pypi.org/project/numpy/)
  * [PyOpenGL](https://pypi.org/project/PyOpenGL/)
  * [PySide2](https://pypi.org/project/PySide2/)

Implementation notes
--------------------

* Assumes the ``OCIO`` environment variable is defined and pointing to a 
  compatible OCIO config file.
* Takes into account file and viewing rules, and may change the input color 
  space and/or view automatically when a new image is loaded.
* Exposure and gamma controls are dynamic parameters, driving uniforms in the 
  generated shader program.
* Specific channel views can be toggled with R, G, B, A keys, and the RGBA view 
  restored with C.

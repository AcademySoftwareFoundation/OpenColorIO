<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<!-- Copyright Contributors to the OpenColorIO Project. -->

ocioview (alpha)
================

**Work in progress**. ``ocioview`` is a visual editor for OCIO configs, written in 
Python.

The app currently consists of three main components; a viewer, a config editor, and a 
transform and config inspector. Multiple viewers can be loaded in different tabs. The 
config editor is a tabbed model/view interface for the current config. Models for 
each config item type interface directly with the config in memory. The inspector 
presents interfaces for inspecting processor curves, serialized config YAML, CTF and 
shader code, and the OCIO log.

The app's scene file is a config. This design allows dynamic interconnectivity between 
config items, reducing risk of errors during config authoring. Undo/redo stack support 
for most features is also implemented.

These components are linked with 10 possible transform subscriptions. Each subscription 
tracks the transform(s) for one config item, and each viewer can subscribe to any of 
these transforms, providing fast visual feedback for transform editing.

``ocioview`` being an alpha release means this app is functional, but still in 
development, so may have some rough edges. Development has mostly been done on Windows. 
Improved support for other platforms is forthcoming. Feedback and bug reports are 
appreciated.

An ``ocioview`` demo was given at the 2023 OCIO Virtual Town Hall meeting, which can be 
viewed on the [ASWF YouTube channel here](https://www.youtube.com/watch?v=y-oq693Wl8g).

Usage
-----

1. Install dependencies on ``PYTHONPATH``
2. Run ``python ocioview.py``

Dependencies
------------

* PyOpenColorIO
* [OpenImageIO (Python bindings)](https://github.com/OpenImageIO/oiio)
* ``pip install -r requirements.txt``
  * [numpy](https://pypi.org/project/numpy/)
  * [Pygments](https://pypi.org/project/Pygments/)
  * [PyOpenGL](https://pypi.org/project/PyOpenGL/)
  * [PySide6](https://pypi.org/project/PySide6/)
  * [QtAwesome](https://pypi.org/project/QtAwesome/)
  * [imageio](https://pypi.org/project/imageio/)

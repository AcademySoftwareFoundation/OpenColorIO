<!-- SPDX-License-Identifier: CC-BY-4.0 -->
<!-- Copyright Contributors to the OpenColorIO Project. -->

OpenColorIO NanoColor
=====================

[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

Introduction
------------

OpenColorIO NanoColor is a light-weight version of OCIO that has no
external dependencies. It is aimed at applications requiring basic
color management in resource-constrained settings, such as running
in a web browser or on a mobile phone.

NanoColor does not support custom config files or LUTs like full OCIO.
Several config files are built into the library itself and these provide
complete implementation of ACES color management. In addition, it's
possible for clients to copy and edit the built-in configs to suit their
needs. This includes the addition of custom color spaces. However, the
color spaces in the ASWF Color Interop Forum rendering core set reserved 
and the behavior of that set may not be modified.

There is a new built-in config that only contains the color spaces in
the ASWF Color Interop Forum core set. This config is available via
the string: "core-renderer-config-v1.0.0-rc1".

NanoColor uses the same underlying transform chain optimization and
CPU and GPU rendering paths as full OCIO. For developers, working with
either NanoColor or full OCIO is very similar. The build process uses
the same CMake set-up, though there are no external dependencies used
in NanoColor mode.

The NanoColor API is virtually the same as the full OCIO API, though
a few functions have been removed in alignment with the feature set. For
example, there are no functions related to uploading texture LUTs to the
GPU, since there is no texture usage in the NanoColor GPU path. All 
supported transforms have a closed-form invertible analytic
representation that does not require LUTs.

One new feature that has been added is a convenience function for 
generating a MatrixTransform based on the chromaticity coordinates of
a set of color primaries. These are methods on the MatrixTransform class:
ConvertTo_XYZ_D65 and ConvertTo_AP0. Usage is demonstrated in unit test:
OpenColorIO/tests/cpu/transforms/BuiltinTransform_tests.cpp:line 304.


Installation instructions
-------------------------

The CMake flag OCIO_FEATURE_SET may be set to "Full" to build full OCIO
and set to "Nano" to build NanoColor. In the NanoColor branch, the default
is "Nano". Here are the basic steps for macOS or Linux (Windows is supported
as well, but the commands are slightly different)::

    $ git clone git@github.com:autodesk-forks/OpenColorIO.git
    $ cd OpenColorIO
    $ git checkout feature/nanocolor
    $ mkdir build && cd build
    $ cmake ..
    $ make -j8
    $ make install


TODO
----

This is currently a prototype and there are a number of tasks remaining:

* Prevent clients from overriding the core color space set.
* A few built-in transforms have been disabled since they required LUTs but
  could be re-enabled by converting to use fixed functions instead.
* Add function to check NanoColor compatibility of a color space
* Clean up the removal of Imath and Pystring.
* Make further optimizations to reduce the size of the library.
* Add JavaScript binding.
* Add documentation.
* Improve the Config cacheID algorithm to not require Yaml.

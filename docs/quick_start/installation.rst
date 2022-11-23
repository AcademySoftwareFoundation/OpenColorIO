..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _installation:

Installation
============

Installation may be done either by installing pre-built binaries from various package 
managers or by building from source.

Please note that the version available through a given package manager may be significantly 
outdated when compared to the current official OCIO release, so please verify the version 
you install is current.  (Unfortunately, even two years after the introduction of 
OpenColorIO v2, several package managers continue to install OCIO 1.1.1.)

Alternatives
************

.. _Python:

Python
++++++

If you only need the Python binding, the simplest solution is to take advantage of the pre-built 
wheels in the Python Package Installer (PyPi) `here <https://pypi.org/project/opencolorio>`_. It 
can be installed as follows, once you have Python installed.

**PyPI**::

    pip install opencolorio

The pre-built wheels are listed `here <https://pypi.org/project/opencolorio/#files>`_.  Note that
source code is provided, so it may be possible for ``pip`` to compile the binding on your machine if
the matrix of Python version and platform version does not have the combination you need.  More
detailed instructions are available for how to use 
`pip. <https://packaging.python.org/en/latest/tutorials/installing-packages/>`_

OpenImageIO
+++++++++++

If you only need to apply color conversions to images, please note that OpenImageIO's oiiotool has 
most of the functionality of the ocioconvert command-line tool (although not everything, such as 
GPU processing). OpenImageIO is available via several package managers (including brew and vcpkg).

**Homebrew**::

    brew install openimageio

**Vcpkg**::

    vcpkg install openimageio[opencolorio,tools]:x64-windows --recurse


Installing OpenColorIO using Package Managers
=============================================

Linux
*****

When it comes to Linux distributions, relatively few of the Linux distribution repositories 
have been updated to OCIO v2. The **latest Fedora** is one good option as it offers a 
recent release of OpenColorIO v2. Information about the package can be found on the
`Fedora project website <https://packages.fedoraproject.org/pkgs/OpenColorIO/OpenColorIO/index.html>`__.

For the other distributions, information about which release of OpenColorIO is available can be 
verified on `pkgs.org <https://pkgs.org/search/?q=OpenColorIO>`__.

**The recommendation is to build OpenColorIO from source**. You may build from source using the 
instructions below. See :ref:`building-from-source`.

Windows using Vcpkg
*******************

Vcpkg can be used to install OpenColorIO on Windows 7 or higher. To do that, Vcpkg must be installed 
by following the `official instructions <https://vcpkg.io/en/getting-started.html>`__. Once Vcpkg 
is installed, OpenColorIO and some of the tools can be installed with the following command::

    vcpkg install opencolorio[tools]:x64-windows

Note that this package **does not** install ocioconvert, ociodisplay, ociolutimage and the Python 
binding.

If you need the extra command-line tools, you'll need to install :ref:`from source. <Windows>` 
However, the Python binding can be installed as described in the :ref:`Python` section.

MacOS using Homebrew
*******************

You can use the Homebrew package manager to install OpenColorIO on macOS.

First install Homebrew as per the instructions on the `Homebrew
homepage <https://brew.sh/>`__ (or see the `Homebrew documentation
<https://docs.brew.sh/>`__ for more detailed instructions)

Then simply run the following command to install::

    brew install opencolorio

Homebrew does not install the Python binding or the command-line tools that depend on either
OpenImageIO or OpenEXR such as ocioconvert, ociodisplay and ociolutimage.

.. _building-from-source:

Building from Source
====================

Dependencies
************

The basic requirements for building OCIO are the following.  Note that, by
default, ``cmake`` will try to install all of the items labelled with * and so
it is not necessary to install those items manually:

- C++ 11-17 compiler (gcc, clang, msvc)
- CMake >= 3.13
- \*Expat >= 2.4.1 (XML parser for CDL/CLF/CTF)
- \*yaml-cpp >= 0.7.0 (YAML parser for Configs)
- \*Imath >= 3.0 (for half domain LUTs)
- \*pystring >= 1.1.3
- \*minizip-ng >= 3.0.6 (for config archiving)
- \*ZLIB (for config archiving)

Some optional components also depend on:

- \*Little CMS >= 2.2 (for ociobakelut ICC profile baking)
- \*OpenGL GLUT & GLEW (for ociodisplay)
- \*OpenEXR >= 3.0 (for apps including ocioconvert)
- OpenImageIO >= 2.1.9 (for apps including ocioconvert)
- \*OpenFX >= 1.4 (for the OpenFX plug-ins)
- \*pybind11 >= 2.9.2 (for the Python binding)
- Python >= 2.7 (for the Python binding)
- Python 3.7 - 3.9 (for the docs, with the following PyPi packages)
    - Sphinx
    - six
    - testresources
    - recommonmark
    - sphinx-press-theme
    - sphinx-tabs
    - breathe
- Doxygen (for the docs)
- NumPy (optionally used in the Python test suite)
- OpenShadingLanguage >= 1.11 (for the OSL unit tests)

Example bash scripts are provided in 
`share/ci/scripts <https://github.com/AcademySoftwareFoundation/OpenColorIO/tree/main/share/ci/scripts>`_ 
for installing some dependencies. These are used by OpenColorIO CI so are 
regularly tested on their noted platforms. The ``install_docs_env.sh``
script will install all dependencies for building OCIO documentation and is 
available for all supported platforms. Use GitBash 
(`provided with Git for Windows <https://gitforwindows.org/>`_) to execute
this script on Windows.

Automated Installation
++++++++++++++++++++++

Dependencies listed above with a preceeding * can be automatically installed at 
build time using the ``OCIO_INSTALL_EXT_PACKAGES`` option in your ``cmake`` 
command (requires an internet connection).  This is the default.  C/C++ 
libraries are pulled from external repositories, built, and are (typically) 
statically-linked into libOpenColorIO. Python packages are installed with ``pip``.
All installs of these components are fully contained within your build directory.

Three ``OCIO_INSTALL_EXT_PACKAGES`` options are available::

    cmake -DOCIO_INSTALL_EXT_PACKAGES=<NONE|MISSING|ALL>

- ``NONE``: Use system installed packages. Fail if any are missing or 
  don't meet minimum version requirements.
- ``MISSING`` (default): Prefer system installed packages. Install any that 
  are not found or don't meet minimum version requirements.
- ``ALL``: Install all required packages, regardless of availability on the 
  current system.

Existing Install Hints
++++++++++++++++++++++

When using libraries already on your system, the CMake variable 
``-D <Package Name>_ROOT=<Path>`` may be used to specify the path to the include and 
library root directory rather than have CMake try to find it.  The package names used 
by OCIO are as follows (note these are case-sensitive):

- ``expat``
- ``yaml-cpp``
- ``Imath``
- ``pystring``
- ``ZLIB``
- ``minizip-ng``
- ``OpenEXR``
- ``OpenImageIO``
- ``lcms2``
- ``pybind11``
- ``openfx``
- ``OSL``
- ``Sphinx``
- ``GLEW``
- ``GLUT``
- ``Python``

This custom variable is also available:

- ``-DPython_EXECUTABLE=<path>`` (Python executable for pybind11)

There are scenarios in which some of the dependencies may not be compiled into the 
OCIO dynamic library.  This is more likely when OCIO does not download the packages
itself.  In these cases, it may be helpful to specify the CMake variable
``-D <Package Name>_STATIC_LIBRARY=ON``, although it is only a hint and is not enforced.
The following package names support this hint:
``expat``, ``yaml-cpp``, ``Imath``, ``lcms2``, and ``minizip-ng``.

The `CMake find modules <https://github.com/AcademySoftwareFoundation/OpenColorIO/tree/main/share/cmake/modules>`_ 
may be consulted for more detail on the handling of a given package.

For the Python packages required for the documentation, ensure their locations
are in your ``PYTHONPATH`` environment variable prior to configuring the build.

.. _enabling-optional-components:

Enabling optional components
****************************

CMake Options
+++++++++++++

There are many options available in `CMake. 
<https://cmake.org/cmake/help/latest/guide/user-interaction/index.html#guide:User%20Interaction%20Guide>`_

Several of the most common ones are:

- ``-DCMAKE_BUILD_TYPE=Release`` (Set to Debug, if necessary)
- ``-DBUILD_SHARED_LIBS=ON`` (Set to OFF to build OCIO as a static library)

Here are the most common OCIO-specific CMake options (the default values are shown):

- ``-DOCIO_BUILD_APPS=ON`` (Set to OFF to not build command-line tools)
- ``-DOCIO_USE_OIIO_FOR_APPS=OFF`` (Set ON to build tools with OpenImageIO rather than OpenEXR)
- ``-DOCIO_BUILD_PYTHON=ON`` (Set to OFF to not build the Python binding)
- ``-DOCIO_BUILD_OPENFX=OFF`` (Set to ON to build the OpenFX plug-ins)
- ``-DOCIO_USE_SSE=ON`` (Set to OFF to turn off SSE CPU performance optimizations)
- ``-DOCIO_BUILD_TESTS=ON`` (Set to OFF to not build the unit tests)
- ``-DOCIO_BUILD_GPU_TESTS=ON`` (Set to OFF to not build the GPU unit tests)
- ``-DOCIO_USE_HEADLESS=OFF`` (Set to ON to do headless GPU reendering)
- ``-DOCIO_WARNING_AS_ERROR=ON`` (Set to OFF to turn off warnings as errors)
- ``-DOCIO_BUILD_DOCS=OFF`` (Set to ON to build the documentation)
- ``-DOCIO_BUILD_FROZEN_DOCS=OFF`` (Set to ON to update the Python documentation)

Several command-line tools (such as ocioconvert) require reading or writing image files.
If ``OCIO_USE_OIIO_FOR_APPS=OFF``, these will be built using OpenEXR rather than OpenImageIO
and therefore you will be limited to using OpenEXR files with these tools rather than the
wider range of image file formats supported by OIIO.  (Using OpenEXR for these tools works
around the issue of a circular dependency between OCIO and OIIO that can complicate some
build chains.)

The CMake output prints information regarding which image library will be used for the
command-line tools (as well as a lot of other info about the build configuration).

Documentation
+++++++++++++

Instructions for installing the documentation pre-requisites and building the docs 
are in the section on :ref:`contributing documentation. <documentation-guidelines>`


.. _osx-and-linux:

MacOS and Linux
***************

While there is a huge range of possible setups, the following steps
should work on macOS and most Linux distros. To keep things simple, this guide 
will use the following example paths - these will almost definitely be 
different for you:

- source code: ``/source/ocio``
- the temporary build location: ``/tmp/ociobuild``
- the final install directory: ``/software/ocio``

First make the build directory and cd to it::

    $ mkdir /tmp/ociobuild
    $ cd /tmp/ociobuild

Next step is to run ``cmake``, which looks for things such as the
compiler's required arguments, optional requirements like Python,
OpenImageIO etc

For this example we will show how to install OCIO to a custom location 
(instead of the default ``/usr/local``), we will thus run ``cmake`` with
``CMAKE_INSTALL_PREFIX``.

Still in ``/tmp/ociobuild``, run::

    $ cmake -DCMAKE_INSTALL_PREFIX=/software/ocio /source/ocio

The last argument is the location of the OCIO source code (containing
the main CMakeLists.txt file). You should see it conclude with something
along the lines of::

    -- Configuring done
    -- Generating done
    -- Build files have been written to: /tmp/ociobuild

Next, build everything (with the ``-j`` flag to build using 8
threads)::

    $ make -j8

Starting with CMake 3.12, you can instead run a portable parallel build::

    $ cmake --build . -j 8

This should complete in a few minutes. Finally, install the files into
the specified location::

    $ make install

If nothing went wrong, ``/software/ocio`` should look something like
this (on Linux or macOS)::

    $ cd /software/ocio
    $ ls
    bin/  include/  lib/  share/
    $ ls bin/
    ociobakelut ociocheck  (and others ...)
    $ ls include/
    OpenColorIO/
    $ ls lib/
    cmake/  libOpenColorIO.dylib  (and some more specific versions ...) 
    libOpenColorIOimageioapphelpers.a  libOpenColorIOoglapphelpers.a
    pkgconfig/  python<version>/
    $ ls lib/pkgconfig
    OpenColorIO.pc
    $ ls lib/python<version>/site-packages
    PyOpenColorIO.so
    $ ls share/ocio
    setup_ocio.sh

.. _Windows:

Windows
*******

While build environments may vary between users, the recommended way to build OCIO from source on 
Windows 7 or newer is to use the scripts provided in the Windows 
`share <https://github.com/AcademySoftwareFoundation/OpenColorIO/tree/main/share/dev/windows>`_ 
section of the OCIO repository. There are two scripts currently available. 

The first script is called 
`ocio_deps.bat <https://github.com/AcademySoftwareFoundation/OpenColorIO/tree/main/share/dev/windows/ocio_deps.bat>`_ 
and it provides some automation to install the most difficult dependencies. Those dependencies are:

- `Vcpkg <https://vcpkg.io/en/index.html>`_
- OpenImageIO
- Freeglut
- Glew
- Python dependencies for documentation

Run this command to execute the ocio_deps.bat script::

    ocio_deps.bat --vcpkg <path to current vcpkg installation or where it should be installed>

The second script is called 
`ocio.bat <https://github.com/AcademySoftwareFoundation/OpenColorIO/tree/main/share/dev/windows/ocio.bat>`_ 
and it provide a way to configure and build OCIO from source. Moreover, this script executes the 
install step of ``cmake`` as well as the unit tests. The main use case is the following::

    ocio.bat --b <path to build folder> --i <path to install folder> 
    --vcpkg <path to vcpkg installation> --ocio <path to ocio repository> --type Release


For more information, please look at each script's documentation::

    ocio.bat --help

    ocio_deps.bat --help

.. _quick-env-config:

Quick environment configuration
===============================

The quickest way to set the required :ref:`environment-setup` is to
source the ``share/ocio/setup_ocio.sh`` script installed with OCIO.
On Windows, use the corresponding setup_ocio.bat file.

For a simple single-user setup, add the following to ``~/.bashrc``
(assuming you are using bash, and the example install directory of
``/software/ocio``)::

    source /software/ocio/share/ocio/setup_ocio.sh

The only environment variable you must configure manually is
:envvar:`OCIO`, which points to the configuration file you wish to
use. For prebuilt config files, see the
:ref:`downloads` section

To do this, you would add a line to ``~/.bashrc`` (or a per-project
configuration script etc), for example::

    export OCIO="/path/to/my/config.ocio"


.. _environment-setup:

Environment variables
=====================

Note: For other user facing environment variables, see :ref:`using_env_vars`.

.. envvar:: OCIO

   This variable needs to point to the global OCIO config file, e.g
   ``config.ocio``

.. envvar:: DYLD_LIBRARY_PATH (macOS)

    The ``lib/`` folder (containing ``libOpenColorIO.dylib``) must be
    on the ``DYLD_LIBRARY_PATH`` search path, or you will get an error
    similar to::

        dlopen(.../OCIOColorSpace.so, 2): Library not loaded: libOpenColorIO.dylib
        Referenced from: .../OCIOColorSpace.so
        Reason: image not found

    This applies to anything that links against OCIO, including the
    ``PyOpenColorIO`` Python binding.

.. envvar:: LD_LIBRARY_PATH (Linux)

    The Linux equivalent of the macOS ``DYLD_LIBRARY_PATH``.

.. envvar:: PYTHONPATH

    Python's module search path. If you are using the PyOpenColorIO
    module, you must add ``lib/python2.x`` to this search path (e.g
    ``python/2.5``), or importing the module will fail::

        >>> import PyOpenColorIO
        Traceback (most recent call last):
          File "<stdin>", line 1, in <module>
        ImportError: No module named PyOpenColorIO

    Note that :envvar:`DYLD_LIBRARY_PATH` or :envvar:`LD_LIBRARY_PATH`
    must be set correctly for the module to work.

.. envvar:: OFX_PLUGIN_PATH

    When building the OCIO OpenFX plugins, include the installed 
    ``OpenColorIO/lib`` directory (where ``OpenColorIO.ofx.bundle`` is located) 
    in this path.

    It is recommended to build OFX plugins in static mode 
    (``BUILD_SHARED_LIBS=OFF``) to avoid any issue loading the OpenColorIO
    library from the plugin once it has been moved. Otherwise, please make sure
    the shared OpenColorIO lib (*.so, *.dll, *.dylib) is visible from the
    plugin by mean of ``PATH``, ``LD_LIBRARY_PATH`` or ``DYLD_LIBRARY_PATH``
    for Windows, Linux and macOS respectively. For systems that supports it,
    it is also possible to edit the RPATH of the plugin to add the location of
    the shared OpenColorIO lib.

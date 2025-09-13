..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _installation:

Installation
============

Installation may be done either by installing pre-built binaries from various package 
managers or by building from source.  (The OCIO project currently does not provide
pre-built libraries as downloadable artifacts other than through package managers.)

Please note that the version available through a given package manager may be significantly 
outdated when compared to the current official OCIO release, so please verify the version 
you install is current.  (Unfortunately, even two years after the introduction of 
OpenColorIO v2, several package managers continue to install OCIO 1.1.1.)

Alternatives
************

.. _Python:

Python
++++++

If you only need the Python binding and command-line tools, the simplest solution is to 
take advantage of the pre-built wheels in the Python Package Index (PyPI) 
`here <https://pypi.org/project/opencolorio>`_. It can be installed as follows, once you 
have Python installed.

**PyPI**::

    pip install opencolorio

The pre-built wheels are listed `here <https://pypi.org/project/opencolorio/#files>`_.  Note that
source code is provided, so it may be possible for ``pip`` to compile the binding on your machine if
the matrix of Python version and platform version does not have the combination you need.  More
detailed instructions are available for how to use 
`pip. <https://packaging.python.org/en/latest/tutorials/installing-packages/>`_

OpenImageIO
+++++++++++

If you only need to apply color conversions to images, please note that OpenImageIO's ``oiiotool`` has 
most of the functionality of the ``ocioconvert`` command-line tool (although not everything, such as 
GPU processing). OpenImageIO is available via several package managers (including Homebrew and Vcpkg).

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

Note that this package **does not** install ``ocioconvert``, ``ociodisplay``, ``ociolutimage``, 
and the Python binding.

If you need the extra command-line tools, you'll need to install :ref:`from source. <Windows>` 
However, the Python binding can be installed as described in the :ref:`Python` section.

MacOS using Homebrew
*******************

You can use the Homebrew package manager to install OpenColorIO on macOS.

First install Homebrew as per the instructions on the `Homebrew
homepage <https://brew.sh/>`__ (or see the `Homebrew documentation
<https://docs.brew.sh/>`__ for more detailed instructions).

Then simply run the following command to install::

    brew install opencolorio

Homebrew does not install the Python binding or the command-line tools that depend on either
OpenImageIO or OpenEXR such as ``ocioconvert``, ``ociodisplay``, and ``ociolutimage``.

.. _building-from-source:

Building from Source
====================

The basic requirements to build OCIO from source on Linux, macOS, and Windows are a
supported C++ compiler, CMake, and an internet connection.  The OCIO Cmake scripts are
able to download, build, and install all other required components.  However, more 
advanced users are also able to have the build use their own version of dependencies.

Dependencies
************

OCIO depends on the following components.  By default, the OCIO CMake scripts will 
download and build the items labelled with a \*, so it is not necessary to install those 
items manually:

Required components:

- C++ 17-23 compiler (gcc, clang, msvc)
- CMake >= 3.14
- \*Expat >= 2.4.1 (XML parser for CDL/CLF/CTF)
- \*yaml-cpp >= 0.8.0 (YAML parser for Configs)
- \*Imath >= 3.1.1 (for half domain LUTs)
- \*pystring >= 1.1.3
- \*minizip-ng >= 3.0.8 (for config archiving)
- \*ZLIB >= 1.2.13 (for config archiving)

Optional OCIO functionality also depends on:

- \*Little CMS >= 2.2 (for ociobakelut ICC profile baking)
- \*OpenGL GLUT & GLEW (for ociodisplay)
- \*OpenEXR >= 3.1.6 (for apps including ocioconvert)
- OpenImageIO >= 2.2.14 (for apps including ocioconvert)
- \*OpenFX >= 1.4 (for the OpenFX plug-ins)
- OpenShadingLanguage >= 1.13 (for the OSL unit tests)
- Doxygen (for the docs)
- NumPy (optionally used in the Python test suite)
- \*pybind11 >= 2.9.2 (for the Python binding)
- Python >= 3.9 (for the Python binding only)
- Python 3.9+ (for building the documentation)

Building the documentation requires the following packages, available via PyPI:

- Sphinx
- six
- testresources
- recommonmark
- sphinx-press-theme
- sphinx-tabs
- breathe

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
command (requires an internet connection).  The C/C++ libraries are pulled from 
external repositories, built, and are (typically) statically-linked into an OCIO
dynamic library.  All installs of these components are fully contained within your 
build directory.

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

If the library is not installed in a typical location where CMake will find it, 
you may specify the location using one of the following methods:

- Set ``-D<package_name>_DIR`` to point to the directory containing the CMake configuration file for the package.

- Set ``-D<package_name>_ROOT`` to point to the directory containing the lib and include directories.

- Set ``-D<package_name>_LIBRARY`` and ``-D<package_name>_INCLUDE_DIR`` to point to the lib and include directories.

Not all packages support all of the above options. Please refer the 
OCIO CMake `find modules <https://github.com/AcademySoftwareFoundation/OpenColorIO/tree/main/share/cmake/modules>`_  for the package that you are having trouble with to see the options it supports.

Usually CMake will use the dynamic library rather than static, if both are present. In this case, 
you may set <package_name>_STATIC_LIBRARY to ON to request use of the static one. If only the 
static library is present (such as when OCIO builds the dependency), then the option is not needed.
The following packages support this option:
``expat``, ``yaml-cpp``, ``Imath``, ``lcms2``, and ``minizip-ng``. Using CMake 3.24+, it is
possible to prefer the static version of ``ZLIB`` with ``-DZLIB_USE_STATIC_LIBS=ON``.

The package names used by OCIO are as follows (note that these are case-sensitive):

Required:

- ``expat``
- ``yaml-cpp``
- ``Imath``
- ``pystring``
- ``ZLIB``
- ``minizip-ng``

Optional:

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


Please note that if you provide your own ``minizip-ng``, rather than having OCIO's CMake
download and build it, you will likely need to set its CMake variables the same way
that OCIO does (e.g., enable ZLib and turn off most other options).  Using a ``minizip-ng``
from various package managers (e.g., Homebrew) probably won't work.  Please see the
settings that begin with ``-DMZ_`` that are used in the OCIO 
`minizip-ng find module. <https://github.com/AcademySoftwareFoundation/OpenColorIO/tree/main/share/cmake/modules/Findminizip-ng.cmake>`_ 

Please note that if you build a static OCIO library, it will not contain the libraries 
for the external packages and so you will need to list those separately when linking your
client application.  If you had OCIO download and build these packages, you will
find them under your build directory in ``ext/dist/lib``.  The libraries that are
needed are: ``expat``, ``yaml-cpp``, ``Imath``, ``pystring``, ``ZLIB``, and ``minizip-ng``.

The OCIO ``make install`` step will install CMake configuration files that may be used
by applications that consume OCIO to find and utilize OCIO during their own build process.

For the Python packages required for the documentation, ensure that their locations
are in your ``PYTHONPATH`` environment variable prior to configuring the build.

This custom variable is also available:

- ``-DPython_EXECUTABLE=<path>`` (Python executable for pybind11)


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
- ``-DOCIO_USE_SIMD=ON`` (Set to OFF to turn off SIMD CPU performance optimizations, such as SSE and NEON)
- ``-DOCIO_USE_SSE2`` (Set to OFF to turn off SSE2 CPU performance optimizations)
- ``-DOCIO_USE_AVX`` (Set to OFF to turn off AVX CPU performance optimizations)
- ``-DOCIO_USE_AVX2`` (Set to OFF to turn off AVX2 CPU performance optimizations)
- ``-DOCIO_USE_F16C`` (Set to OFF to turn off F16C CPU performance optimizations)
- ``-DOCIO_BUILD_TESTS=ON`` (Set to OFF to not build the unit tests)
- ``-DOCIO_BUILD_GPU_TESTS=ON`` (Set to OFF to not build the GPU unit tests)
- ``-DOCIO_USE_HEADLESS=OFF`` (Set to ON to do headless GPU rendering)
- ``-DOCIO_WARNING_AS_ERROR=ON`` (Set to OFF to turn off warnings as errors)
- ``-DOCIO_BUILD_DOCS=OFF`` (Set to ON to build the documentation)

Note that OCIO will turn off any specific SIMD CPU performance optimizations if they are not supported 
by the build target architecture. The default for ``OCIO_USE_SSE2``, ``OCIO_USE_AVX``, ``OCIO_USE_AVX2`` and 
``OCIO_USE_F16C`` depends on the architecture, but will be ON where supported.

On MacOS, the default is to build for the native architecture that CMake is running under.
For example, if a x86_64 version of CMake is running under Rosetta, the native architecture will 
be x86_64, rather then arm64. You can use the ``CMAKE_OSX_ARCHITECTURES`` option to override that.
To build universal binaries, use the following option: ``-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"``. 

When doing a universal build, note that the OCIO dependencies must be built as universal libraries 
too. If you are running in OCIO_INSTALL_EXT_PACKAGES=MISSING or NONE mode, your build will fail if 
any of your installed libraries are not universal. The easiest way to address this is to set 
OCIO_INSTALL_EXT_PACKAGES=ALL in order to let OCIO build everything. Alternatively, you may set 
CMAKE_OSX_ARCHITECTURES to just the platform you are targeting.

Several command-line tools (such as ``ocioconvert``) require reading or writing image files.
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

Run this command to execute the ocio_deps.bat script:

.. code-block:: bash
    
    ocio_deps.bat --vcpkg <path to current vcpkg installation or where it should be installed>

The second script is called 
`ocio.bat <https://github.com/AcademySoftwareFoundation/OpenColorIO/tree/main/share/dev/windows/ocio.bat>`_ 
and it provide a way to configure and build OCIO from source. Moreover, this script executes the 
install step of ``cmake`` as well as the unit tests. The main use case is the following:

.. code-block:: bash

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
On Windows, use the corresponding setup_ocio.bat file. See OCIO's install directory under 
share/ocio.

For a temporary configuration of your terminal, you can run the following script:

.. code-block:: bash

   # Windows - Execute setup_ocio.bat
   [... path to OCIO install directory]/share/ocio/setup_ocio.bat
   # Unix - Execute setup_ocio.sh
   [... path to OCIO install directory]\share\ocio\setup_ocio.sh

For a more permanent option, add the following to ``~/.bashrc``
(assuming you are using bash, and the example install directory of
``/software/ocio``):

.. code-block:: bash

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

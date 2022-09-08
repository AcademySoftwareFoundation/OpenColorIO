..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _installation:

Installation
************

Although OpenColorIO is available through a variety of package managers, please note that the 
version available through a given package manager may be significantly outdated when compared to 
the current official OCIO release. Even two years after the introduction of OpenColorIO v2, several 
package managers continue to install OCIO 1.1.1.

.. _Python:

Python
^^^^^^

If you only need the Python bindings, the simplest solution is to take advantage of the pre-built 
wheels in the Python Package Installer (PyPi) `here <https://pypi.org/project/opencolorio>`__. It 
can be installed by using this command:: 

    pip install opencolorio

OpenImageIO
^^^^^^^^^^^

If you only need to apply color conversions to images, please note that OpenImageIO's oiiotool has 
most of the functionality of the ocioconvert command-line tool (although not everything, such as 
GPU processing). OpenImageIO is available via several package managers (including brew and vcpkg).

**Homebrew**::

    brew install openimageio

**Vcpkg**::

    vcpkg install openimageio[opencolorio,tools]:x64-windows --recurse

Installing OpenColorIO using existing packages
**********************************************

Linux
^^^^^

When it comes to Linux distributions, relatively few of the Linux distribution repositories have been 
updated to OCIO v2. The **latest Fedora** is one good option as it offers the most 
recent release of OpenColorIO v2. Information about the package can be found on 
`fedoraproject website <https://packages.fedoraproject.org/pkgs/OpenColorIO/OpenColorIO/index.html>`__.

For the other distributions, information about which release of OpenColorIO is available can be 
verified on `pkgs.org <https://pkgs.org/search/?q=OpenColorIO>`__.

**The recommendation is to build OpenColorIO from source**. You may build from source using the 
instructions below. See :ref:`building-from-source`.

Windows 7 or newer using vcpkg
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Vcpkg can be used to install OpenColorIO on Windows. In order to do that, Vcpkg must be installed 
by following the `official instructions <https://vcpkg.io/en/getting-started.html>`__. Once Vcpkg 
is installed, OpenColorIO and some of the tools can be installed with the following command::

    vcpkg install opencolorio[tools]:x64-windows

Note that this package **does not** install ocioconvert, ociodisplay, ociolutimage and the Python 
bindings.

The three missing tools can be built from source by following the steps in the :ref:`Windows 7 or newer` 
section while the Python bindings can be install using the pip command in the :ref:`Python` section.

OS X using Homebrew
^^^^^^^^^^^^^^^^^^^

You can use the Homebrew package manager to install OpenColorIO on OS X.

First install Homebrew as per the instructions on the `Homebrew
homepage <http://mxcl.github.com/homebrew/>`__ (or see the `Homebrew wiki
<https://github.com/mxcl/homebrew/wiki/Installation>`__ for more
detailed instructions)

Then simply run the following command to install::

    brew install opencolorio

Homebrew does not install the Python binding or the command-line tools that depend on OpenImageIO 
such as ocioconvert, ociodisplay and ociolutimage.

.. _building-from-source:

Building from source
********************

Dependencies
^^^^^^^^^^^^

The basic requirements for building OCIO are the following.  Note that, by
default, cmake will try to install all of the items labelled with * and so
it is not necessary to install those items manually:

- cmake >= 3.12
- \*Expat >= 2.4.1 (XML parser for CDL/CLF/CTF)
- \*yaml-cpp >= 0.7.0 (YAML parser for Configs)
- \*Imath >= 3.0 (for half domain LUTs)
- \*pystring >= 1.1.3

Some optional components also depend on:

- \*Little CMS >= 2.2 (for ociobakelut ICC profile baking)
- \*pybind11 >= 2.9.2 (for the Python bindings)
- Python >= 2.7 (for the Python bindings)
- Python 3.7 or 3.8 (for the docs, with the following PyPi packages)
    - Sphinx
    - six
    - testresources
    - recommonmark
    - sphinx-press-theme
    - sphinx-tabs
    - breathe
- NumPy (for complete Python test suite)
- Doxygen (for the docs)
- OpenImageIO >= 2.1.9 or OpenEXR >= 3.0 (for apps including ocioconvert)

Example bash scripts are provided in 
`share/ci/scripts <https://github.com/AcademySoftwareFoundation/OpenColorIO/tree/main/share/ci/scripts>`_ 
for installing some dependencies. These are used by OpenColorIO CI so are 
regularly tested on their noted platforms. The ``install_docs_env.sh``
script will install all dependencies for building OCIO documentation and is 
available for all supported platforms. Use GitBash 
(`provided with Git for Windows <https://gitforwindows.org/>`_) to execute
this script on Windows.

Automated Installation
----------------------

Listed dependencies with a preceeding * can be automatically installed at 
build time using the ``OCIO_INSTALL_EXT_PACKAGES`` option in your cmake 
command (requires an internet connection).  This is the default.  C/C++ 
libraries are pulled from external repositories, built, and statically-linked 
into libOpenColorIO. Python packages are installed with ``pip``. All installs 
of these components are fully contained within your build directory.

Three ``OCIO_INSTALL_EXT_PACKAGES`` options are available::

    cmake -DOCIO_INSTALL_EXT_PACKAGES=<NONE|MISSING|ALL>

- ``NONE``: Use system installed packages. Fail if any are missing or 
  don't meet minimum version requirements.
- ``MISSING`` (default): Prefer system installed packages. Install any that 
  are not found or don't meet minimum version requirements.
- ``ALL``: Install all required packages, regardless of availability on the 
  current system.

Existing Install Hints
----------------------

When using existing system libraries, the following CMake variables can be 
defined to hint at non-standard install locations and preference of shared
or static linking:

- ``-Dexpat_ROOT=<path>`` (include and/or library root dir)
- ``-Dexpat_STATIC_LIBRARY=ON`` (prefer static lib)
- ``-Dyaml-cpp_ROOT=<path>`` (include and/or library root dir)
- ``-Dyaml-cpp_STATIC_LIBRARY=ON`` (prefer static lib)
- ``-DImath_ROOT=<path>`` (include and/or library root dir)
- ``-DImath_STATIC_LIBRARY=ON`` (prefer static lib)
- ``-DHalf_ROOT=<path>`` (include and/or library root dir)
- ``-DHalf_STATIC_LIBRARY=ON`` (prefer static lib)
- ``-Dpystring_ROOT=<path>`` (include and/or library root dir)
- ``-Dpystring_STATIC_LIBRARY=ON`` (prefer static lib)
- ``-Dlcms2_ROOT=<path>`` (include and/or library root dir)
- ``-Dlcms2_STATIC_LIBRARY=ON`` (prefer static lib)
- ``-Dpybind11_ROOT=<path>`` (include and/or library root dir)
- ``-DPython_EXECUTABLE=<path>`` (Python executable)

To hint at Python package locations, add paths to the ``PYTHONPATH`` 
environment variable prior to configuring the build.

.. _osx-and-linux:

OS X and Linux
^^^^^^^^^^^^^^

While there is a huge range of possible setups, the following steps
should work on OS X and most Linux distros. To keep things simple, this guide 
will use the following example paths - these will almost definitely be 
different for you:

- source code: ``/source/ocio``
- the temporary build location: ``/tmp/ociobuild``
- the final install directory: ``/software/ocio``

First make the build directory and cd to it::

    $ mkdir /tmp/ociobuild
    $ cd /tmp/ociobuild

Next step is to run cmake, which looks for things such as the
compiler's required arguments, optional requirements like Python,
OpenImageIO etc

For this example we will show how to install OCIO to a custom location 
(instead of the default ``/usr/local``), we will thus run cmake with
``CMAKE_INSTALL_PREFIX``.

Still in ``/tmp/ociobuild``, run::

    $ cmake -DCMAKE_INSTALL_PREFIX=/software/ocio /source/ocio

The last argument is the location of the OCIO source code (containing
the main CMakeLists.txt file). You should see something along the
lines of::

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
this::

    $ cd /software/ocio
    $ ls
    bin/     include/ lib/
    $ ls bin/
    ociobakelut ociocheck  (and others ...)
    $ ls include/
    OpenColorIO/   PyOpenColorIO/ pkgconfig/
    $ ls lib/
    libOpenColorIO.a      libOpenColorIO.dylib

.. _Windows 7 or newer:

Windows 7 or newer
^^^^^^^^^^^^^^^^^^

While build environments may vary between users, the recommended way to build OCIO from source on 
Windows is to use the scripts provided in the Windows 
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
install step of cmake as well as the unit tests. The main use case is the following::

    ocio.bat --b <path to build folder> --i <path to install folder> 
    --vcpkg <path to vcpkg installation> --ocio <path to ocio repository> --type Release


For more information, please look at each script's documentation::

    ocio.bat --help

    ocio_deps.bat --help

.. _enabling-optional-components:

Enabling optional components
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The OpenColorIO library is probably not all you want - the Python
libraries bindings, the Nuke nodes and several applications are only
built if their dependencies are found.

In the case of the Python bindings, the dependencies are the Python
headers for the version you wish to use. These may be picked up by
default - if so, when you run cmake you would see::

    -- Python 2.6 okay, will build the Python bindings against .../include/python2.6

If not, you can point cmake to correct Python executable using the
``-D PYTHON=...`` cmake flag::

    $ cmake -D PYTHON=/my/custom/python2.6 /source/ocio

The applications included with OCIO have various dependencies - to
determine these, look at the CMake output when first run::

    -- Not building ocioconvert. Requirement(s) found: OIIO:FALSE


.. _quick-env-config:

Quick environment configuration
*******************************

The quickest way to set the required :ref:`environment-setup` is to
source the ``share/ocio/setup_ocio.sh`` script installed with OCIO.

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
*********************

Note: For other user facing environment variables, see :ref:`using_env_vars`.

.. envvar:: OCIO

   This variable needs to point to the global OCIO config file, e.g
   ``config.ocio``

.. envvar:: DYLD_LIBRARY_PATH

    The ``lib/`` folder (containing ``libOpenColorIO.dylib``) must be
    on the ``DYLD_LIBRARY_PATH`` search path, or you will get an error
    similar to::

        dlopen(.../OCIOColorSpace.so, 2): Library not loaded: libOpenColorIO.dylib
        Referenced from: .../OCIOColorSpace.so
        Reason: image not found

    This applies to anything that links against OCIO, including the
    ``PyOpenColorIO`` Python bindings.

.. envvar:: LD_LIBRARY_PATH

    Equivalent to the ``DYLD_LIBRARY_PATH`` on Linux

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

..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _installation:

Installation
============

The easy way
************

While prebuilt binaries are not yet available for all platforms, OCIO
is available via several platform's package managers.

Please note that the package managers will install the current stable
release.  If you want OCIO v2, you currently must build from source.
See :ref:`building-from-source`.


Fedora and RHEL
^^^^^^^^^^^^^^^

In Fedora Core 15 and above, the following command will install OpenColorIO::

    yum install OpenColorIO

Providing you are using the `Fedora EPEL repository
<http://fedoraproject.org/wiki/EPEL>`__ (see the `FAQ for instructions
<http://fedoraproject.org/wiki/EPEL/FAQ#Using_EPEL>`__), this same
command will work for RedHat Enterprise Linux 6 and higher (including
RHEL derivatives such as CentOS 6 and Scientific Linux 6)

OS X using Homebrew
^^^^^^^^^^^^^^^^^^^

You can use the Homebrew package manager to install OpenColorIO on OS X.

First install Homebrew as per the instructions on the `Homebrew
homepage <http://mxcl.github.com/homebrew/>`__ (or see the `Homebrew wiki
<https://github.com/mxcl/homebrew/wiki/Installation>`__ for more
detailed instructions)

Then simply run the following command to install::

    brew install opencolorio

To build with the Python library use this command::

    brew install opencolorio --with-python


.. _building-from-source:

Building from source
********************

Dependencies
************

The basic requirements for building OCIO are the following.  Note that, by
default, cmake will try to install all of the items labelled with * and so
it is not necessary to install those items manually:

- cmake >= 3.12
- \*Expat >= 2.2.5 (XML parser for CDL/CLF/CTF)
- \*yaml-cpp >= 0.6.3 (YAML parser for Configs)
- \*IlmBase (Half only) >= 2.3.0 (for half domain LUTs)
- \*pystring >= 1.1.3

Some optional components also depend on:

- \*Little CMS >= 2.2 (for ociobakelut ICC profile baking)
- \*pybind11 >= 2.4.3 (for the Python bindings)
- Python >= 2.7 (for the Python bindings)
- Python >= 3.7 (for the docs, with the following PyPi packages)
    - Sphinx
    - six
    - testresources
    - recommonmark
    - sphinx-press-theme
    - sphinx-tabs
    - breathe
- NumPy (for complete Python test suite)
- Doxygen (for the docs)
- OpenImageIO >= 2.1.9 (for apps including ocioconvert)

Example bash scripts are provided in 
`share/ci/scripts <https://github.com/AcademySoftwareFoundation/OpenColorIO/tree/master/share/ci/scripts>`_ 
for installing some dependencies. These are used by OpenColorIO CI so are 
regularly tested on their noted platforms. The ``install_docs_env.sh``
script will install all dependencies for building OCIO documentation and is 
available for all supported platforms. Use GitBash 
(`provided with Git for Windows <https://gitforwindows.org/>`_) to execute
this script on Windows.

Automated Installation
^^^^^^^^^^^^^^^^^^^^^^

Listed dependencies with a preceeding * can be automatically installed at 
build time using the ``OCIO_INSTALL_EXT_PACKAGES`` option in your cmake 
command (requires an internet connection).  This is the default.  C/C++ 
libraries are pulled from external repositories, built, and statically-linked 
into libOpenColorIO. Python packages are installed with ``pip``. All installs 
of these components are fully contained within your build directory.

Three ``OCIO_INSTALL_EXT_PACKAGES`` options are available::

    cmake -DOCIO_INSTALL_EXT_PACKAGES=<NONE|MISSING|ALL>

- ``NONE``: Use system installed packages. Fail if any are missing or 
  don't meet minimum version requireements.
- ``MISSING`` (default): Prefer system installed packages. Install any that 
  are not found or don't meet minimum version requireements.
- ``ALL``: Install all required packages, regardless of availability on the 
  current system.

Existing Install Hints
^^^^^^^^^^^^^^^^^^^^^^

When using existing system libraries, the following CMake variables can be 
defined to hint at non-standard install locations and preference of shared
or static linking:

- ``-DExpat_ROOT=<path>`` (include and/or library root dir)
- ``-DExpat_STATIC_LIBRARY=ON`` (prefer static lib)
- ``-Dyaml-cpp_ROOT=<path>`` (include and/or library root dir)
- ``-Dyaml-cpp_STATIC_LIBRARY=ON`` (prefer static lib)
- ``-DHalf_ROOT=<path>`` (include and/or library root dir)
- ``-DHalf_STATIC_LIBRARY=ON`` (prefer static lib)
- ``-Dpystring_ROOT=<path>`` (include and/or library root dir)
- ``-Dpystring_STATIC_LIBRARY=ON`` (prefer static lib)
- ``-Dlcms2_ROOT=<path>`` (include and/or library root dir)
- ``-Dlcms2_STATIC_LIBRARY=ON`` (prefer static lib)
- ``-pybind11_ROOT=<path>`` (include and/or library root dir)
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

.. _windows-build:

Windows Build
^^^^^^^^^^^^^

While build environments may vary between user, here is an example batch file
for compiling on Windows as provided by `@hodoulp <https://github.com/hodoulp>`__::

    @echo off


    REM Grab the repo name, default is ocio
    set repo_name=ocio
    if not %1.==. set repo_name=%1


    REM Using cygwin to have Linux cool command line tools
    set CYGWIN=nodosfilewarning

    set CMAKE_PATH=D:\OpenSource\3rdParty\cmake-3.12.2
    set GLUT_PATH=D:\OpenSource\3rdParty\freeglut-3.0.0-2
    set GLEW_PATH=D:\OpenSource\3rdParty\glew-1.9.0
    set PYTHON_PATH=C:\Python27

    REM Add glut & glew dependencies to have GPU unit tests
    set PATH=%GLEW_PATH%\bin;%GLUT_PATH%\bin;D:\Tools\cygwin64\bin;%CMAKE_PATH%\bin;%PATH%

    REM Add Ninja & jom to speed-up command line build i.e. one is enough
    set PATH=D:\OpenSource\3rdParty\ninja;D:\OpenSource\3rdParty\jom;%PYTHONPATH%;%PATH%

    call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
    REM call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvarsall.bat" x64

    set OCIO_PATH=D:\OpenSource\%repo_name%

    D:

    IF NOT EXIST %OCIO_PATH% ( 
    echo %OCIO_PATH% does not exist
    exit /b
    )
    cd %OCIO_PATH%


    set CMAKE_BUILD_TYPE=Release

    echo *******
    echo *********************************************
    echo ******* Building %OCIO_PATH%
    echo **
    echo **
    set are_you_sure = Y
    set /P are_you_sure=Build in %CMAKE_BUILD_TYPE% ([Y]/N)?  
    if not %are_you_sure%==Y set CMAKE_BUILD_TYPE=Debug


    set BUILD_PATH=%OCIO_PATH%\build_rls
    set COMPILED_THIRD_PARTY_HOME=D:/OpenSource/3rdParty/compiled-dist_rls
    set OCIO_BUILD_PYTHON=1

    if not %CMAKE_BUILD_TYPE%==Release (
    set BUILD_PATH=%OCIO_PATH%\build_dbg
    set COMPILED_THIRD_PARTY_HOME=D:/OpenSource/3rdParty/compiled-dist_dbg
    set OCIO_BUILD_PYTHON=0
    )

    set INSTALL_PATH=%COMPILED_THIRD_PARTY_HOME%/OpenColorIO-2.0.0

    IF NOT EXIST %BUILD_PATH% ( mkdir %BUILD_PATH% )
    cd %BUILD_PATH%

    echo **
    echo **

    REM cmake -G "Visual Studio 14 2015 Win64"
    REM cmake -G "Visual Studio 15 2017 Win64"
    REM cmake -G "Ninja"
    cmake -G "NMake Makefiles JOM" ^
        -DCMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE% ^
        -DCMAKE_INSTALL_PREFIX=%INSTALL_PATH% ^
        -DBUILD_SHARED_LIBS=ON ^
        -DOCIO_BUILD_APPS=ON ^
        -DOCIO_BUILD_TESTS=ON ^
        -DOCIO_BUILD_GPU_TESTS=ON ^
        -DOCIO_BUILD_DOCS=OFF ^
        -DOCIO_USE_SSE=ON ^
        -DOCIO_WARNING_AS_ERROR=ON ^
        -DOCIO_BUILD_PYTHON=%OCIO_BUILD_PYTHON% ^
        -DPython_LIBRARY=%PYTHONPATH%\libs\python27.lib ^
        -DPython_INCLUDE_DIR=%PYTHONPATH%\include ^
        -DPython_EXECUTABLE=%PYTHONPATH%\python.exe ^
        -DOCIO_BUILD_JAVA=OFF ^
        -DCMAKE_PREFIX_PATH=%COMPILED_THIRD_PARTY_HOME%\OpenImageIO-1.9.0;%COMPILED_THIRD_PARTY_HOME%/ilmbase-2.2.0 ^
        %OCIO_PATH%

    REM Add OCIO & OIIO
    set PATH=%BUILD_PATH%\src\OpenColorIO;%INSTALL_PATH%\bin;%COMPILED_THIRD_PARTY_HOME%\OpenImageIO-1.9.0\bin;%PATH%


    REM Find the current branch
    set GITBRANCH=
    for /f %%I in ('git.exe rev-parse --abbrev-ref HEAD 2^> NUL') do set GITBRANCH=%%I

    if not "%GITBRANCH%" == ""  prompt $C%GITBRANCH%$F $P$G

    TITLE %repo_name% (%GITBRANCH%)

    echo *******
    echo *********************************************
    if not "%GITBRANCH%" == "" echo branch  = %GITBRANCH%
    echo *
    echo Mode         = %CMAKE_BUILD_TYPE%
    echo Build path   = %BUILD_PATH%
    echo Install path = %INSTALL_PATH%
    echo *
    echo compile = jom all
    echo test    = ctest -V
    echo doc     = jom doc
    echo install = jom install
    echo *********************************************
    echo *******

You could create a desktop shortcut with the following command:
    ``%comspec% /k "C:\Users\hodoulp\ocio.bat" ocio``

Also look to the Appveyor config script at the root of repository for an example
build sequence.

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

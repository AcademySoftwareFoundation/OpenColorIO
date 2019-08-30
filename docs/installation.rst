.. _installation:

Installation
============

The easy way
************

While prebuilt binaries are not yet available for all platforms, OCIO
is available via several platform's package managers.

Fedora and RHEL
+++++++++++++++

In Fedora Core 15 and above, the following command will install OpenColorIO::

    yum install OpenColorIO

Providing you are using the `Fedora EPEL repository
<http://fedoraproject.org/wiki/EPEL>`__ (see the `FAQ for instructions
<http://fedoraproject.org/wiki/EPEL/FAQ#Using_EPEL>`__), this same
command will work for RedHat Enterprise Linux 6 and higher (including
RHEL derivatives such as CentOS 6 and Scientific Linux 6)

macOS using Homebrew
++++++++++++++++++++

You can use the Homebrew package manager to install OpenColorIO on macOS.

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

.. _macos-and-linux

macOS and Linux
+++++++++++++++

While there is a huge range of possible setups, the following steps
should work on macOS and most Linux distros.

The basic requirements are:

- cmake >= 3.11
- (optional) Python 2.x (for the Python bindings)
- (optional) Nuke 6.x or newer (for the Nuke nodes)
- (optional) OpenImageIO (for apps including ocioconvert)
- (optional) Truelight SDK (for TruelightTransform)

To keep things simple, this guide will use the following example
paths - these will almost definitely be different for you:

- source code: ``/source/ocio``
- the temporary build location: ``/tmp/ociobuild``
- the final install directory: ``/software/ocio``

First make the build directory and cd to it::

    $ mkdir /tmp/ociobuild
    $ cd /tmp/ociobuild

Next step is to run cmake, which looks for things such as the
compiler's required arguments, optional requirements like Python,
Nuke, OpenImageIO etc

As we want to install OCIO to a custom location (instead of the
default ``/usr/local``), we will run cmake with
``CMAKE_INSTALL_PREFIX``

Still in ``/tmp/ociobuild``, run::

    $ cmake -D CMAKE_INSTALL_PREFIX=/software/ocio /source/ocio

The last argument is the location of the OCIO source code (containing
the main CMakeLists.txt file). You should see something along the
lines of::

    -- Configuring done
    -- Generating done
    -- Build files have been written to: /tmp/ociobuild

Next, build everything (with the ``-j`` flag to build using 8
threads)::

    $ make -j8

This should complete in a few minutes. Finally, install the files into
the specified location::

    $ make install

If nothing went wrong, ``/software/ocio`` should look something like
this::

    $ cd /software/ocio
    $ ls
    bin/     include/ lib/
    $ ls bin/
    ocio2icc    ociobakelut ociocheck
    $ ls include/
    OpenColorIO/   PyOpenColorIO/ pkgconfig/
    $ ls lib/
    libOpenColorIO.a      libOpenColorIO.dylib

.. _windows-build

Windows Build
+++++++++++++

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
        -DPYTHON_LIBRARY=%PYTHONPATH%\libs\python27.lib ^
        -DPYTHON_INCLUDE_DIR=%PYTHONPATH%\include ^
        -DPYTHON_EXECUTABLE=%PYTHONPATH%\python.exe ^
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
++++++++++++++++++++++++++++

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

Same process with Nuke (although it less likely to be picked up
automatically). Point cmake to your Nuke install directory by adding
``-D NUKE_INSTALL_PATH``::

    $ cmake -D PYTHON=/my/custom/python2.6 -D NUKE_INSTALL_PATH=/Applications/Nuke6.2v1/Nuke6.2v1.app/Contents/MacOS/ /source/ocio

The ``NUKE_INSTALL_PATH`` directory should contain the Nuke executable
(e.g Nuke6.2v1), and a ``include/`` directory containing ``DDImage/``
and others.

If set correctly, you will see something similar to::

    -- Found Nuke: /Applications/Nuke6.2v1/Nuke6.2v1.app/Contents/MacOS/include
    -- Nuke_API_VERSION: --6.2--

The Nuke plugins are installed into ``lib/nuke$MAJOR.$MINOR/``, e.g
``lib/nuke6.2/OCIODisdplay.so``


.. note::

    If you are using the Nuke plugins, you should compile the Python
    bindings for the same version of Python that Nuke uses
    internally. For Nuke 6.0 and 6.1 this is Python 2.5, and for 6.2
    it is Python 2.6

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


.. _nuke-configuration:

Nuke Configuration
******************

If you specified the ``NUKE_INSTALL_PATH`` option when running cmake,
you should have a ``/software/ocio/lib/nuke6.2`` directory containing
various files.

If you have followed :ref:`quick-env-config`, the plugins should be
functional. However, one common additional configuration step is to
register an OCIODisplay node for each display device/view specified in
the config.

To do this, in a menu.py on :envvar:`NUKE_PATH` (e.g
``~/.nuke/menu.py`` for a single user setup), add the following:

.. code-block:: python

    import ocionuke.viewer
    ocionuke.viewer.populate_viewer(also_remove = "default")

The ``also_remove`` argument can be set to either "default" to remove
the default sRGB/rec709 options, "all" to remove everything, or "none"
to leave existing viewer processes untouched.

Alternatively, if your workflow has different requirements, you can
copy the function and modify it as required, or use it as reference to
write your own, better viewer setup function!

.. literalinclude:: viewer.py
   :language: python


.. _environment-setup:

Environment variables
*********************

.. envvar:: OCIO

   This variable needs to point to the global OCIO config file, e.g
   ``config.ocio``

.. envvar:: OCIO_LOGGING_LEVEL

    Configures OCIO's internal logging level. Valid values are
    ``none``, ``warning``, ``info``, or ``debug`` (or their respective
    numeric values ``0``, ``1``, ``2``, or ``3`` can be used)

    Logging output is sent to STDERR output.

.. envvar:: OCIO_ACTIVE_DISPLAYS

   Overrides the :ref:`active_displays` configuration value.
   Colon-separated list of displays, e.g ``sRGB:P3``

.. envvar:: OCIO_ACTIVE_VIEWS

   Overrides the :ref:`active_views` configuration
   item. Colon-separated list of view names, e.g
   ``internal:client:DI``

.. envvar:: DYLD_LIBRARY_PATH

    The ``lib/`` folder (containing ``libOpenColorIO.dylib``) must be
    on the ``DYLD_LIBRARY_PATH`` search path, or you will get an error
    similar to::

        dlopen(.../OCIOColorSpace.so, 2): Library not loaded: libOpenColorIO.dylib
        Referenced from: .../OCIOColorSpace.so
        Reason: image not found

    This applies to anything that links against OCIO, including the
    Nuke nodes, and the ``PyOpenColorIO`` Python bindings.

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

.. envvar:: NUKE_PATH

    Nuke's customization search path, where it will look for plugins,
    gizmos, init.py and menu.py scripts and other customizations.

    This should point to both ``lib/nuke6.2/`` (or whatever version
    the plugins are built against), and ``share/nuke/``

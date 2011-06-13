Installation
============

.. _building-from-source:

Building from source
********************

While there is a huge range of possible setups, the following steps
should work on OS X and most Linux distros.

The basic requirements are:

- cmake >= 2.8
- (optional) Python 2.x (for the Python bindings)
- (optional) Nuke 6.x (for the Nuke nodes)
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

Next step is to run cmake, which looks for things such as the compiler
required arguments, optional requirements like Python, Nuke,
OpenImageIO etc

As we want to install OCIO to a custom location (instead of the
default ``/usr/local``), we will run cmake with
``CMAKE_INSTALL_PREFIX``

Still in ``/tmp/ociobuild``, run::

    $ cmake -D CMAKE_INSTALL_PREFIX=/software/ocio /source/ocio

The last argument is the location of the OCIO source code (containng
the main CMakeLists.txt file). You should see something along the
lines of::

    -- Configuring done
    -- Generating done
    -- Build files have been written to: /tmp/ociobuild

Next, build everything (with the ``-j`` flag to build using 8 threads)::

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


.. _enabling-optional-components:

Enabling optional components
----------------------------

The OpenColourIO library is probably not all you want - the Python
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

.. _nuke-configuration:

Nuke Configuration
******************

If you specified the NUKE_INSTALL_PATH when running cmake, you should
have a ``/software/ocio/lib/nuke6.2`` containing various files.

The bear-minimum required to use the plugins is to point the
:envvar:`NUKE_PATH` environment variable to this directory. It is also
recommended you add the ``share/nuke/`` directory to this path

To run Nuke, in a terminal execute the following::

    # Set Nuke location
    export NUKE_PATH="/software/ocio/lib/nuke6.2:${NUKE_PATH}"
    export NUKE_PATH="/software/ocio/share/nuke:${NUKE_PATH}"

    # Point to libOpenColorIO.dylib (change "DYLD" to "LD" on Linux)
    export DYLD_LIBRARY_PATH="/software/ocio/lib:${DYLD_LIBRARY_PATH}"

    # Launch Nuke
    nuke myscript.nk

The export lines can be point in ``~/.bashrc`` for a single user, or a
show-specific startup shell script etc.

One common configuration step is to register an OCIODisplay node for
each display device/view specified in the config.

In a menu.py on :envvar:`NUKE_PATH` (e.g ``~/.nuke/menu.py`` for a single
user setup), you can call the following:

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

   This variable needs to point to the global OCIO config file, e.g ``config.ocio``

.. envvar:: DYLD_LIBRARY_PATH

    The ``lib/`` folder (containing ``libOpenColorIO.dylib``) must be
    on the ``DYLD_LIBRARY_PATH`` search path, or you will get an error
    similar to::

        dlopen(.../OCIOColorSpace.so, 2): Library not loaded: libOpenColorIO.dylib
        Referenced from: .../OCIOColorSpace.so
        Reason: image not found

    This applies to anything that links against OCIO, including the Nuke
    nodes, and the ``PyOpenColorIO`` Python bindings.

.. envvar:: LD_LIBRARY_PATH

    Equivalent to the ``DYLD_LIBRARY_PATH`` on Linux

.. envvar:: PYTHONPATH

    Python's module search path. If you are using the PyOpenColorIO module,
    you must add ``lib/python2.x`` to this search path (e.g ``python/2.5``),
    or importing the module will fail::

        >>> import PyOpenColorIO
        Traceback (most recent call last):
          File "<stdin>", line 1, in <module>
        ImportError: No module named PyOpenColorIO

    Note that :envvar:`DYLD_LIBRARY_PATH` or :envvar:`LD_LIBRARY_PATH` must
    be set correctly for the module to work.

.. envvar:: NUKE_PATH

    Nuke's customisation search path, where it will look for plugins,
    gizmos, init.py and menu.py scripts and other customisations.

    For information on setting this for OCIO, see :ref:`nuke-configuration`

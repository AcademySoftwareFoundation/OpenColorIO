Setup
=====

Building from source
********************

While there are an almost infinite number of setups, the following
steps should cover many common setups.

The basic requirements are:

- cmake >2.8
- (optional) Python 2.x (for the Python bindings)
- (optional) Nuke 6.x (for the Nuke nodes)
- (optional) Boost's unit_test_framework

In the OCIO source directory, make a build directory (optionally, but
highly recommended)::

    $ mkdir build && cd build

Then run cmake::

    $ cmake -D CMAKE_INSTALL_PREFIX=install ../

You should see something along the lines of::

    -- Configuring done
    -- Generating done
    -- Build files have been written to: .../build

Next, to build (with the ``-j`` flag to build using 8 CPU cores)::

    make -j8

Finally, to install into the CMAKE_INSTALL_PREFIX (the install
directory)::

    make install

If nothing went wrong, you should now have an ``include/`` directory
containing the OpenColorIO headers, and a ``lib/`` directory
containing the libOpenColorIO library (a .dylib, .so or .dll file)

Enabling optional components
----------------------------

The OpenColourIO library is probably not all you want - the Python
libraries bindings and the Nuke nodes are only built if the
dependencies are found.

In the case of the Python bindings, the dependencies are the Python
headers for the version you wish to use. These may be picked up by
default - if so, when you run cmake, you would see::

    -- Python 2.6 okay, will build the Python bindings against .../include/python2.6

If not, you can point cmake to correct Python executable using the
``-D PYTHON=...`` cmake flag::

    $ cmake -D PYTHON=/my/custom/python2.6

Same process with Nuke (although it less likely to be picked up
automatically). Point cmake to your Nuke install directory using ``-D
NUKE_INSTALL_PATH``::

    $ cmake -D NUKE_INSTALL_PATH=/Applications/Nuke6.2v1/Nuke6.2v1.app/Contents/MacOS/

The ``NUKE_INSTALL_PATH`` directory should contain the Nuke executable
(e.g Nuke6.2v1), and a ``include/`` directory containing ``DDImage/``
and others.

If set correctly, you will see something similar to:

    -- Found Nuke: /Applications/Nuke6.2v1/Nuke6.2v1.app/Contents/MacOS/include
    -- Nuke_DDImageVersion: --6.2v1--

The Nuke plugins are installed into ``lib/$NUKE_VERSION/``, e.g
``lib/nuke6.2v1/OCIODisdplay.so``

Note that if you are using the Nuke plugins, you should compile the
Python bindings for the same version of Python that Nuke uses
internally. For Nuke 6.0 and 6.1 this is Python 2.5, and for 6.2 it is
Python 2.6

Environment setup
*****************

.. envvar:: OCIO
   
   This env needs to point to the global profile.yaml

.. envvar:: DYLD_LIBRARY_PATH

    The ``lib/`` folder (containing ``libOpenColorIO.dylib``) must be
    on the ``DYLD_LIBRARY_PATH`` search path, or you will get an error
    similar to::

        dlopen(.../OCIOColorSpace.so, 2): Library not loaded: libOpenColorIO.dylib
        Referenced from: .../OCIOColorSpace.so
        Reason: image not found

    This also applies to the Python and the ``PyOpenColorIO`` module,
    where the correct ``lib/python2.x`` path must be included in this
    search path.

.. envvar:: LD_LIBRARY_PATH

    Equivelant to the ``DYLD_LIBRARY_PATH`` on Linux

Nuke Configuration
******************

If you build the OCIO Nuke plugins, they will be installed into
``lib/$NUKE_VERSION`` (for example: `lib/nuke6.2v1`). The bear-minimum
required to use the plugins is to point the NUKE_PATH environment
variable to this directory.

However, you probably want to load the plugins, and add them to the
menu - so also add ``$BUILD_DIR/share/nuke`` to ``NUKE_PATH``, and
when Nuke is run it will execute two scripts, ``init.py`` which
automatically loads all OCIO plugins, and ``menu.py`` which adds them
to a OCIO menu under the Color node menu.

The OCIO workflow within Nuke is a different topic, but one common
setup step is to register an OCIODisplay node as a viewer process (to
apply a viewer LUT)

To do this, we you use the OCIO Python bindings to find all configured
display devices (e.g sRGB device, DCIP3 device) and transforms (e.g
Film emulation, raw and log), then register a viewer processor each
combination.

The following function is defined in the OCIO-supplied menu.py file,
so you can simply call ``ocio_populate_viewer()`` in a custom menu.py
file (e.g in ``~/.nuke/menu``)

Alternativly, if your workflow has different requirements, you can
copy the code and modify it as required, or use it as reference to
write your own, better viewer setup function!

.. TODO: Would be nice to ".. include" this rather than duplicating,
.. but menu.py contains other functions

.. code-block:: python

    def ocio_populate_viewer(remove_nuke_default = True):
        """Registers the a viewer process for each display/transform, and
        sets the default

        Also unregisters the default Nuke viewer processes (sRGB/rec709)
        unless remove_nuke_default is False
        """

        if remove_nuke_default:
            nuke.ViewerProcess.unregister('rec709')
            nuke.ViewerProcess.unregister('sRGB')


        # Formats the display and transform, e.g "Film1D (sRGB"
        DISPLAY_UI_FORMAT = "%(transform)s (%(display)s)"

        import PyOpenColorIO as OCIO
        cfg = OCIO.GetCurrentConfig()

        allDisplays = cfg.getDisplayDeviceNames()

        # For every display, loop over every transform
        for dname in allDisplays:
            allTransforms = cfg.getDisplayTransformNames(dname)

            for xform in allTransforms:
                nuke.ViewerProcess.register(
                    name = DISPLAY_UI_FORMAT % {'transform': xform, "display": dname},
                    call = nuke.nodes.OCIODisplay,
                    args = (),
                    kwargs = {"device": dname, "transform": xform})


        # Get the default display and transform, register it as the
        # default used on Nuke startup
        defaultDisplay = cfg.getDefaultDisplayDeviceName()
        defaultXform = cfg.getDefaultDisplayTransformName(defaultDisplay)

        nuke.knobDefault(
            "Viewer.viewerProcess",
            DISPLAY_UI_FORMAT % {'transform': defaultXform, "display": defaultDisplay})

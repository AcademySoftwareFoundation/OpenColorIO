Python API
==========

Description
***********

A color configuration (:py:class:`PyOpenColorIO.Config`) defines all the color spaces to be
available at runtime.

(:py:class:`PyOpenColorIO.Config`)  is the main object for interacting with this library. 
It encapsulates all the information necessary to use customized
:py:class:`PyOpenColorIO.ColorSpaceTransform` and
:py:class:`PyOpenColorIO.DisplayTransform` operations.

See the :ref:`user-guide` for more information on selecting, creating, 
and working with custom color configurations.

For applications interested in using only one color configuration at
a time (this is the vast majority of apps), their API would
traditionally get the global configuration and use that, as
opposed to creating a new one. This simplifies the use case
for plugins and bindings, as it alleviates the need to pass
around configuration handles.

An example of an application where this would not be
sufficient would be a multi-threaded image proxy server
(daemon) that wants to handle multiple show configurations
concurrently in a single process. This app would need to keep
multiple configurations alive, and manage them appropriately.

Roughly speaking, a novice user should select a default
configuration that most closely approximates the use case
(animation, visual effects, etc.), and set the :envvar:`OCIO`
environment variable to point at the root of that configuration.

.. note::
   Initialization using environment variables is typically preferable
   in a multi-app ecosystem, as it allows all applications to be consistently configured.

.. note::
   Paths to LUTs can be relative. The search paths are defined in :py:class:`PyOpenColorIO.Config`.

See :ref:`developers-usageexamples`

Examples of Use
+++++++++++++++

.. code-block:: python

    import PyOpenColorIO as OCIO
    
    # Load an existing configuration from the environment.
    # The resulting configuration is read-only. If $OCIO is set, it will use that.
    # Otherwise it will use an internal default.
    config = OCIO.GetCurrentConfig()
    
    # What color spaces exist?
    colorSpaceNames = [ cs.getName() for cs in config.getColorSpaces() ]
    
    # Given a string, can we parse a color space name from it?
    inputString = 'myname_linear.exr'
    colorSpaceName = config.parseColorSpaceFromString(inputString)
    if colorSpaceName:
        print 'Found color space', colorSpaceName
    else:
        print 'Could not get color space from string', inputString
    
    # What is the name of scene-linear in the configuration?
    colorSpace = config.getColorSpace(OCIO.Constants.ROLE_SCENE_LINEAR)
    if colorSpace:
        print colorSpace.getName()
    else:
        print 'The role of scene-linear is not defined in the configuration'
    
    # For examples of how to actually perform the color transform math,
    # see 'Python: Processor' docs.
    
    # Create a new, empty, editable configuration
    config = OCIO.Config()
    
    # Create a new color space, and add it
    cs = OCIO.ColorSpace(...)
    # (See ColorSpace for details)
    config.addColorSpace(cs)
    
    # For additional examples of config manipulation, see
    # https://github.com/imageworks/OpenColorIO-Configs/blob/master/nuke-default/make.py

Exceptions
**********

.. autoclass:: PyOpenColorIO.Exception
    :members:
    :undoc-members:

.. autoclass:: PyOpenColorIO.ExceptionMissingFile
    :members:
    :undoc-members:

Global
******

.. autofunction:: PyOpenColorIO.ClearAllCaches

.. autofunction:: PyOpenColorIO.GetCurrentConfig

.. autofunction:: PyOpenColorIO.GetLoggingLevel

.. autofunction:: PyOpenColorIO.SetCurrentConfig

.. autofunction:: PyOpenColorIO.SetLoggingLevel

Config
******

.. autoclass:: PyOpenColorIO.Config
    :members:
    :undoc-members:

ColorSpace
**********

.. autoclass:: PyOpenColorIO.ColorSpace
    :members:
    :undoc-members:

Look
****

.. autoclass:: PyOpenColorIO.Look
    :members:
    :undoc-members:

Processor
*********

.. autoclass:: PyOpenColorIO.Processor
    :members:
    :undoc-members:

Context
*******

.. autoclass:: PyOpenColorIO.Context
    :members:
    :undoc-members:

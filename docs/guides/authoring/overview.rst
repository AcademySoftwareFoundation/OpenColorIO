..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _config_overview:



Config syntax
=============

OpenColorIO is primarily controlled by a central configuration file,
usually named ``config.ocio``. This page will only describe how to
*syntactically* write this OCIO config - e.g. what transforms are
available, or what sections are optional.

This page alone will not help you to write a useful config file! See
the :ref:`configurations` section for examples of complete, practical
configs, and discussion of how they fit within a facilities workflow.

Please note that you should use the OCIO Python or C++ API to generate
the config.ocio file rather than writing the YAML by hand in a text editor.
However, if you do ever modify YAML by hand rather than via the API, you
should run :ref:`overview-ociocheck` on it to ensure that the syntax is
correct.

YAML basics
***********

This config file is a YAML document, so it is important to have some
basic knowledge of this format.

The `Wikipedia article on YAML <http://en.wikipedia.org/wiki/YAML>`__
has a good overview.

OCIO configs typically use a small subset of YAML, so looking at
existing configs is probably the quickest way to familiarise yourself
(just remember the indentation is important!).

Checking for errors
*******************

Use the ``ociocheck`` command line tool to validate your config. It
will inform you of YAML-syntax errors, but more importantly it
performs various OCIO-specific validations.  There are also validate
methods on the Config class in the C++ and Python APIs, (although
they do not do the role checking that ociocheck does).

.. TODO: Insert API reference :py:meth:`Config.validate`.

For more information, see the overview of :ref:`overview-ociocheck`

Config sections
***************

An OCIO config has the following sections:

* :ref:`config-header` -- The header contains the version and LUT search path.
* :ref:`config-environment` -- The environment defines the context variables used 
  in the config.
* :ref:`config-roles` -- The roles define which color spaces should be used for common 
  tasks.
* :ref:`config-rules` -- These rules define sensible defaults that help
  applications provide a better user experience.
* :ref:`config-displays-views` -- This section defines how color spaces should be viewed.
* :ref:`config-looks` -- Looks are transforms used to adjust colors, such as to apply a
  creative effect.
* :ref:`config-colorspaces` -- This section defines the scene-referred color space encodings
  available within the config.
* :ref:`config-display-colorspaces` -- This section defines the display-referred color space
  encodings available within the config.
* :ref:`config-named-transforms` -- Named Transforms are a way to provide transforms that
  do not have a fixed relationship to a specific reference space, such as a utility curve.

A collection of :ref:`config-transforms` is provided for use in the various sections
of the config file.

.. _config-header:

Config header
*************

``ocio_profile_version``
^^^^^^^^^^^^^^^^^^^^^^^^

Required.

By convention, the profile starts with ``ocio_profile_version``.

This is a string, specifying which version of the OCIO config syntax is used.

The currently supported version strings are ``1`` and ``2``.

.. code-block:: yaml

    ocio_profile_version: 2


``description``
^^^^^^^^^^^^^^^

Optional. A brief description of the configuration.

.. code-block:: yaml

    description: This is the OCIO config for show: foo.


``name``
^^^^^^^^

Optional. A unique name for the config.  Future versions of OCIO might use this as a
sort of "namespace" for the color spaces defined in the rest of the config.

.. code-block:: yaml

    name: studio-config-v1.0.0_aces-v1.3_ocio-v2.1


``search_path``
^^^^^^^^^^^^^^^

Optional. Default is an empty search path.

``search_path`` is a colon-separated list of directories. Each
directory is checked in order to locate a file (e.g. a LUT).

This works is very similar to how the UNIX ``$PATH`` env-var works for
executables.

A common directory structure for a config is::

    config.ocio
    luts/
      lg10_to_lnf.spi1d
      lg10_to_p3.3dl

For this, we would set ``search_path`` as follows:

.. code-block:: yaml

    search_path: "luts"

In a color space definition, we might have a FileTransform which refers
to the LUT ``lg10_to_lnf.spi1d``. It will look in the ``luts``
directory, relative to the ``config.ocio`` file's location.

Paths can be relative (to the directory containing ``config.ocio``),
or absolute (e.g. ``/mnt/path/to/my/luts``)

Multiple paths can be specified, including a mix of relative and
absolute paths. Each path is separated with a colon ``:``

.. code-block:: yaml

    search_path: "/mnt/path/to/my/luts:luts"

Paths may also be written on separate lines (this is more Windows friendly):

.. code-block:: yaml

    search_path: 
      - luts1
      - luts2

Finally, paths can reference OCIO's context variables:

.. code-block:: yaml

    search_path: "/shots/show/$SHOT/cc/data:luts"

This allows for some clever setups, for example per-shot LUT's with
fallbacks to a default. For more information, see the examples in
:ref:`userguide-looks`


``family_separator``
^^^^^^^^^^^^^^^^^^^^

Optional.  Defines the character used to split color space family strings
into hierarchical menus.  It may only be a single character.  If no separator
is defined, the Menu Helpers API will not generate hierarchical menus.

.. code-block:: yaml

    family_separator: /


``inactive_colorspaces``
^^^^^^^^^^^^^^^^^^^^^^^^

Optional.  Identify a list of color spaces that should not be used.  These spaces
may stay in the config and will still work in ColorSpaceTransforms, but they will
not be added to application menus.  This will be overridden by the environment
variable :envvar:`OCIO_INACTIVE_COLORSPACES`.

.. code-block:: yaml

    inactive_colorspaces: [ do_not_use_this_colorspace, prev_version_colorspace ]


``luma``
^^^^^^^^

Deprecated. Optional. Default is the Rec.709 primaries specified by the ASC:

.. code-block:: yaml

    luma: [0.2126, 0.7152, 0.0722]

These are the luminance coefficients, which can be used by
OCIO-supporting applications when adjusting saturation (e.g. in an
image-viewer when displaying a single channel)

.. note::

    While the API method is not yet officially deprecated, ``luma`` is
    a legacy option from Imageworks' internal, closed-source
    predecessor to OCIO.

    The ``luma`` value is not respected anywhere within the OCIO
    library. Also very few (if any) applications supporting OCIO will
    respect the value either.


.. _config-environment:

Environment
***********

``environment``
^^^^^^^^^^^^^^^

Optional. The envrionment section declares all of the context variables used
in the configuration.

.. code-block:: yaml

    environment:
      SEQ: default_sequence
      SHOT: $SHOT

It is highly recommended that config authors using context variables include 
the environment section for the following reasons:

* It provides performance benefits to applications
* It will make the config easier to read and maintain
* It allows defining default values
* It improves the validation that may be performed on a config

This config uses two context variables: SEQ and SHOT.  SEQ has a default value
of default_sequence.  This is the value that will be used if the environment
does not contain the SEQ variable and the context variable is not otherwise
defined.  The SHOT variable does not have a default and hence the use of the
syntax shown.

The environment must be self-contained and may not refer to any other variables.
For instance, in the example above it would not be legal to have ``SHOT: $FOO``
since FOO is not one of the declared variables.

Every context variable used in the config must be declared since no other
environment variables will be loaded into the context.  In studios that use
a large number of environment variables, this may provide a performance 
benefit for applications.


.. _config-roles:

Roles
*****

``roles``
^^^^^^^^^

Required.

A "role" is an alternate name to a color space, which can be used by
applications to perform task-specific color transforms without
requiring the user to select a color space by name.

For example, the Nuke node OCIOLogConvert: instead of requiring the
user to select the appropriate log color space, the node performs a
transform between ``scene_linear`` and ``compositing_log``, and the
OCIO config specifies the project-appropriate color spaces. This
simplifies life for artists, as they don't have to remember which is
the correct log color space for the current project - the
OCIOLogConvert always does the correct thing.


A typical role definition looks like this, taken from the
:ref:`config-spivfx` example configuration:

.. code-block:: yaml

    roles:
      color_picking: cpf
      color_timing: lg10
      compositing_log: lgf
      data: ncf
      default: ncf
      matte_paint: vd8
      reference: lnf
      scene_linear: lnf
      texture_paint: dt16


All values in this example (such as ``cpf``, ``lg10`` and ``ncf``)
refer to color spaces defined later the config, in the ``colorspaces``
section.


Here is a description of the roles defined within OpenColorIO. Note
that application developers may also define roles for config authors
to use to control other types of tasks not listed below.

.. warning::
   Unfortunately there is a fair amount of variation in how
   applications interpret OCIO roles.  This section should be
   expanded to try and clarify the intended usage.

* ``aces_interchange`` - defines the color space in the config that
  implements the ACES2065-1 color space defined in SMPTE ST2065-1.
  This role is used to convert scene-referred color spaces between
  different configs that both define this role.

* ``cie_xyz_d65_interchange`` - defines the color space in the config
  that implements standard CIE XYZ colorimetry, adapted to a D65 white.
  This role is used to convert display-referred color spaces between
  different configs that both define this role.

* ``color_picking`` - colors in a color-selection UI can be displayed
  in this space, while selecting colors in a different working space
  (e.g. ``scene_linear`` or ``texture_paint``).

* ``color_timing`` - color space used for applying color corrections,
  e.g. user-specified grade within an image viewer (if the application
  uses the ``DisplayTransform::setDisplayCC`` API method)

* ``compositing_log`` - a log color space used for certain processing
  operations (plate resizing, pulling keys, degrain, etc). Used by the
  OCIOLogConvert Nuke node.

* ``data`` - used when writing data outputs such as normals, depth
  data, and other "non color" data. The color space in this role should
  typically have ``data: true`` specified, so no color transforms are
  applied.

* ``default`` - when ``strictparsing: false``, this color space is used
  as a fallback.

* ``matte_paint`` - color space which matte-paintings are created in
  (for more information, :ref:`see the guide on baking ICC profiles
  for Photoshop <userguide-bakelut-photoshop>`, and
  :ref:`config-spivfx`).

* ``reference`` - the color space against which the other color spaces
  are defined.

.. note::
   The reference role has sometimes been misinterpreted as being the
   space in which "reference art" is stored in.

* ``scene_linear`` - the scene-referred linear-to-light color space,
  often the same as the reference space (see:ref:`faq-terminology`).

* ``texture_paint`` - similar to ``matte_paint`` but for painting
  textures for 3D objects (see the description of texture painting in
  :ref:`SPI's pipeline <config-spipipeline-texture>`).


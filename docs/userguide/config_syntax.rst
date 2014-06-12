.. _userguide-configsyntax:

Config syntax
=============

OpenColorIO is primarily controlled by a central configuration file,
usually named ``config.ocio``. This page will only describe how to
*syntactically* write this OCIO config - e.g what transforms are
available, or what sections are optional.

This page alone will not help you to write a useful config file! See
the :ref:`configurations` section for examples of complete, practical
configs, and discussion of how they fit within a facitilies workflow.

YAML basics
***********

This config file is a YAML document, so it is important to have some
basic knowledge of this format.

The `Wikipedia article on YAML <http://en.wikipedia.org/wiki/YAML>`__
has a good overview.

OCIO configs typically use a small subset of YAML, so looking at
existing configs is probably the quickest way to familiarise yourself
(just remember the indentation is important!)


Checking for errors
*******************

Use the ``ociocheck`` command line tool to validate your config. It
will inform you of YAML-syntax errors, but more importantly it
performs various OCIO-specific "sanity checks".

For more information, see the overview of :ref:`overview-ociocheck`

Config sections
***************

``ocio_profile_version``
++++++++++++++++++++++++

Required.

By convention, the profile starts with ``ocio_profile_version``.

This is an integer, specifying which version of the OCIO config syntax
is used.

Currently there is only one OCIO profile version, so the value is
always ``1`` (one)

.. code-block:: yaml

    ocio_profile_version: 1


``search_path``
+++++++++++++++

Optional. Default is an empty search path.

``search_path`` is a colon-seperated list of directories. Each
directory is checked in order to locate a file (e.g a LUT).

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

In a colorspace definition, we might have a FileTransform which refers
to the LUT ``lg10_to_lnf.spi1d``. It will look in the ``luts``
directory, relative to the ``config.ocio`` file's location.

Paths can be relative (to the directory containing ``config.ocio``),
or absolute (e.g ``/mnt/path/to/my/luts``)

Multiple paths can be specified, including a mix of relative and
absolute paths. Each path is separated with a colon ``:``

.. code-block:: yaml

    search_path: "/mnt/path/to/my/luts:luts"

Finally, paths can reference OCIO's context variables:

.. code-block:: yaml

    search_path: "/shots/show/$SHOT/cc/data:luts"

This allows for some clever setups, for example per-shot LUT's with
fallbacks to a default. For more information, see the examples in
:ref:`userguide-looks`


``strictparsing``
+++++++++++++++++

Optional. Valid values are ``true`` and ``false``. Default is ``true``
(assuming a config is present):

.. code-block:: yaml

    strictparsing: true


OCIO provides a mechanism for applications to extract the colorspace
from a filename (the ``parseColorSpaceFromString`` API method)

So for a file like ``example_render_v001_lnf.0001.exr`` it will
determine the colorspace ``lnf`` (it being the right-most substring
containing a colorspace name)

However, if the colorspace cannot be determined and ``strictparsing:
true``, it will produce an error.

If the colorspace cannot be determined and ``strictparsing: false``,
the default role will be used. This allows unhandled images to operate
in "non-color managed" mode.

Application authors should note: when no config is present (e.g via
``$OCIO``), the default internal profile specifies
``strictparsing=false``, and the default color space role is
``raw``. This means that ANY string passed to OCIO will be parsed as
the default ``raw``. This is nice because in the absence of a config,
the behavior from your application perspective is that the library
essentially falls back to "non-color managed".


``luma``
++++++++

Deprecated. Optional. Default is the Rec.709 primaries specified by the ASC:

.. code-block:: yaml

    luma: [0.2126, 0.7152, 0.0722]

These are the luminance coeficients, which can be used by
OCIO-supporting applications when adjusting saturation (e.g in an
image-viewer when displaying a single channel)

.. note::

    While the API method is not yet officially deprecated, ``luma`` is
    a legacy option from Imageworks' internal, closed-source
    predecessor to OCIO.

    The ``luma`` value is not respected anywhere within the OCIO
    library. Also very few (if any) applications supporting OCIO will
    respect the value either.


``roles``
+++++++++

Required.

A "role" is an alias to a colorspaces, which can be used by
applications to perform task-specific color transforms without
requiring the user to select a colorspace by name.

For example, the Nuke node OCIOLogConvert: instead of requiring the
user to select the appropriate log colorspace, the node performs a
transform between ``scene_linear`` and ``compositing_log``, and the
OCIO config specifies the project-appropriate colorspaces. This
simplifies life for artists, as they don't have to remember which is
the correct log colorspace for the current project - the
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
refer to colorspaces defined later the config, in the ``colorspaces``
section.


A description of all roles. Note that applications may interpret or
use these differently.

* ``color_picking`` - Colors in a color-selection UI can be displayed
  in this space, while selecting colors in a different working space
  (e.g ``scene_linear`` or ``texture_paint``)

* ``color_timing`` - colorspace used for applying color corrections,
  e.g user-specified grade within an image viewer (if the application
  uses the ``DisplayTransform::setDisplayCC`` API method)

* ``compositing_log`` - a log colorspace used for certain processing
  operations (plate resizing, pulling keys, degrain, etc). Used by the
  OCIOLogConvert Nuke node.

* ``data`` - used when writing data outputs such as normals, depth
  data, and other "non color" data. The colorspace in this role should
  typically have ``data: true`` specified, so no color transforms are
  applied.

* ``default`` - when ``strictparsing: false``, this colorspace is used
  as a fallback. If not defined, the ``scene_linear`` role is used

* ``matte_paint`` - Colorspace which matte-paintings are created in
  (for more information, :ref:`see the guide on baking ICC profiles
  for Photoshop <userguide-bakelut-photoshop>`, and
  :ref:`config-spivfx`)

* ``reference`` - Colorspace used for reference imagery (e.g sRGB
  images from the internet)

* ``scene_linear`` - The scene-referred linear-to-light colorspace,
  typically used as reference space (see :ref:`faq-terminology`)

* ``texture_paint`` - Similar to ``matte_paint`` but for painting
  textures for 3D objects (see the description of texture painting in
  :ref:`SPI's pipeline <config-spipipeline-texture>`)


``displays``
++++++++++++

Required.

This section defines all the display devices which will be used. For
example you might have a sRGB display device for artist workstations,
a DCIP3 display device for the screening room projector.

Each display device has a number of "views". These views provide
different ways to display the image on the selected display
device. Examples of common views are:

* "Film" to emulate the final projected result on the current display
* "Log" to send log-space pixel values directly to the display,
  resulting in a "lifted" image useful for checking black-levels.
* "Raw" when assigned a colorspace with ``raw: yes`` set will show the
  unaltered image, useful for tech-checking images

An example of the ``displays`` section from the :ref:`config-spivfx` config:

.. code-block:: yaml

    displays:
      DCIP3:
        - !<View> {name: Film, colorspace: p3dci8}
        - !<View> {name: Log, colorspace: lg10}
        - !<View> {name: Raw, colorspace: nc10}
      sRGB:
        - !<View> {name: Film, colorspace: srgb8}
        - !<View> {name: Log, colorspace: lg10}
        - !<View> {name: Raw, colorspace: nc10}
        - !<View> {name: Film, colorspace: srgb8}


All the colorspaces (``p3dci8``, ``srgb8`` etc) refer to colorspaces
defined later in the config.

Unless the ``active_displays`` and ``active_views`` sections are
defined, the first display and first view will be the default.


``active_displays``
+++++++++++++++++++

Optional. Default is for all displays to be visible, and to respect
order of items in ``displays`` section.

You can choose what display devices to make visible in UI's, and
change the order in which they appear.

Given the example ``displays`` block in the previous section - to make
the sRGB device appear first:

.. code-block:: yaml

    active_displays: [sRGB, DCIP3]

To display only the ``DCIP3`` device, simply remove ``sRGB``:

.. code-block:: yaml

    active_displays: [DCIP3]


The value can be overridden by the `OCIO_ACTIVE_DISPLAYS`
env-var. This allows you to make the ``sRGB`` the only active display,
like so:

.. code-block:: yaml

    active_displays: [sRGB]

Then on a review machine with a DCI P3 projector, set the following
environment variable, making ``DCIP3`` the only visible display
device::

    export OCIO_ACTIVE_DISPLAYS="DCIP3"

Or specify multiple active displays, by separating each with a colon::

    export OCIO_ACTIVE_DISPLAYS="DCIP3:sRGB"


``active_views``
++++++++++++++++

Optional. Default is for all views to be visible, and to respect order
of the views under the display.

Works identically to ``active_displays``, but controls which *views* are
visible.

Overridden by the ``OCIO_ACTIVE_VIEWS`` env-var::

    export OCIO_ACTIVE_DISPLAYS="Film:Log:Raw"


``looks``
+++++++++

Optional.

This section defines a list of "looks". A look is a color transform
defined similarly to a colorspace, with a few important differences.

For example, a look could be defined for a "first pass DI beauty
grade", which is used to view shots with a rough approximation of the
final grade.

When the look is defined in the config, you must specify a name, the
color transform, and the colorspace in which the grade is performed
(the "process space"). You can optionally specify an inverse transform
for when the look transform is not trivially invertable (e.g it
applies a 3D LUT)

When an application applies a look, OCIO ensures the grade is applied
in the correct colorspace (by converting from the input colorspace to
the process space, applies the look's transform, and converts the
image to the output colorspace)

Here is a simple ``looks:`` section, which defines two looks:

.. code-block:: yaml

    looks:
      - !<Look>
        name: beauty
        process_space: lnf
        transform: !<CDLTransform> {slope: [1, 2, 1]}

      - !<Look>
        name: neutral
        process_space: lg10
        transform: !<FileTransform> {src: 'neutral-${SHOT}-${SEQ}.csp', interpolation: linear }
        inverse_transform: !<FileTransform> {src: 'neutral-${SHOT}-${SEQ}-reverse.csp', interpolation: linear }


Here, the "beauty" look applies a simple, static ASC CDL grade, making
the image very green (for some artistic reason!). The beauty look is
appied in the scene-linear ``lnf`` colorspace (this colorspace is
defined elsewhere in the config.

Next is a definition for a "neutral" look, which applies a
shot-specific CSP LUT, dynamically finding the correct LUT based on
the SEQ and SHOT :ref:`context variables <userguide-looks>`.

For example if ``SEQ=ab`` and ``SHOT=1234``, this look will search for
a LUT named ``neutral-ab-1234.csp`` in locations specified in
``search_path``.

The ``process_space`` here is ``lg10``. This means when the look is
applied, OCIO will perform the following steps:

* Transform the image from it's current colorspace to the ``lg10`` process space
* Apply apply the FileTransform (which applies the grade LUT)
* Transform the graded image from the process space to the output colorspace

The "beauty" look specifies the optional ``inverse_transform``,
because in this example the neutral CSP files contain a 3D LUT. For
many transforms, OCIO will automatically calculate the inverse
transform (as with the "beauty" look), however with a 3D LUT the
inverse transform needs to be defined.

If the look was applied in reverse, and ``inverse_transform`` as not
specified, then OCIO would give a helpful error message. This is
commonly done for non-invertable looks


As in colorspace definitions, the transform can be specified as a
series of transforms using the ``GroupTransform``, for example:

.. code-block:: yaml

    looks:
      - !<Look>
        name: beauty
        process_space: lnf
        transform: !<GroupTransform>
          children:
            - !<CDLTransform> {slope: [1, 2, 1]}
            - !<FileTransform> {src: beauty.spi1d, interpolation: nearest}


``colorspaces``
+++++++++++++++

Required.

This section is a list of all the colorspaces known to OCIO. A
colorspace can be referred to elsewhere within the config (including
other colorspace definitions), and are used within OCIO-supporting
applications.




``to_reference`` and ``from_reference``
---------------------------------------

Here is a example of a very simple ``colorspaces`` section, modified
from the :ref:`config-spivfx` example config:

.. code-block:: yaml

    colorspaces:
      - !<ColorSpace>
        name: lnf
        bitdepth: 32f
        description: |
          lnf : linear show space

      - !<ColorSpace>
        name: lg16
        bitdepth: 16ui
        description: |
          lg16 : conversion from film log
        to_reference: !<FileTransform> {src: lg16_to_lnf.spi1d, interpolation: nearest}


First the ``lnf`` colorspace (short for linear float) is used as our
reference colorspace. The name can be anything, but the idea of a
reference colorspace is an important convention within OCIO: **all
other colorspaces are defined as transforms either to or from this
colorspace.**

The ``lg16`` colorspace is a 16-bit log colorspace (see
:ref:`config-spivfx` for an explaination of this colorspace). It has a
name, a bitdepth and a description.

The ``lg16`` colorspace is defined as a transform from ``lg16`` to the
reference colorspace (``lnf``). That transform is to apply the LUT
``lg16_to_lnf.spi1d``. This LUT has an input of ``lg16`` integers and
outputs linear 32-bit float values

Since the 1D LUT is automatically invertable by OCIO, we can use this
colorspace both to convert ``lg16`` images to ``lnf``, and ``lnf``
images to ``lg16``

Importantly, because of the reference colorspace concept, we can
convert images from ``lg16`` to the reference colorspace, and then on
to any other colorspace.


Here is another example colorspace, which is defined using
``from_reference``.

.. code-block:: yaml

      - !<ColorSpace>
        name: srgb8
        bitdepth: 8ui
        description: |
          srgb8 :rgb display space for the srgb standard.
        from_reference: !<FileTransform> {src: srgb8.spi3d, interpolation: linear}

We use ``from_reference`` here because we have a LUT which transforms
from the reference colorspace (``lnf`` in this example) to sRGB.

In this case ``srgb8.spi3d`` is a complex 3D LUT which cannot be
inverted, so it is considered a "display only" colorspace. If we did have a second 3D LUT to apply the inverse transform, we can specify both ``to_reference`` and ``from_reference``


.. code-block:: yaml

      - !<ColorSpace>
        name: srgb8
        bitdepth: 8ui
        description: |
          srgb8 :rgb display space for the srgb standard.
        from_reference: !<FileTransform> {src: lnf_to_srgb8.spi3d, interpolation: linear}
        to_reference: !<FileTransform> {src: srgb8_to_lnf.spi3d, interpolation: linear}

Using multiple transforms
-------------------------

The previous example colorspaces all used a single transform each,
however it is often useful to use multiple transforms to define a
colorspace.

.. code-block:: yaml

      - !<ColorSpace>
        name: srgb8
        bitdepth: 8ui
        description: |
          srgb8 :rgb display space for the srgb standard.
        from_reference: !<GroupTransform>
          children:
            - !<ColorSpaceTransform> {src: lnf, dst: lg16}
            - !<FileTransform> {src: lg16_to_srgb8.spi3d, interpolation: linear}

Here to get from the reference colorspace, we first use the
``ColorSpaceTransform`` to convert from ``lnf`` to ``lg16``, then
apply our 3D LUT on the log-encoded images.

This primarily demonstrates the meta-transform ``GroupTransform``: a
transform which simply composes two or more transforms together into
one. Anything that acceptsa transform like ``FileTransform`` or
``CDLTransform`` will also accept a ``GroupTransform``

It is also worth noting the ``ColorSpaceTransform``, which transforms
between ``lnf`` and ``lg16`` colorspaces (which are defined within the
current config).


Example transform steps
-----------------------

This section explains how OCIO internally applies all the
transforms. It can be skipped over if you understand how the reference
colorspace works.

.. code-block:: yaml

    colorspaces:
      - !<ColorSpace>
        name: lnf
        bitdepth: 32f
        description: |
          lnf : linear show space

      - !<ColorSpace>
        name: lg16
        bitdepth: 16ui
        description: |
          lg16 : conversion from film log
        to_reference: !<FileTransform> {src: lg16.spi1d, interpolation: nearest}

      - !<ColorSpace>
        name: srgb8
        bitdepth: 8ui
        description: |
          srgb8 :rgb display space for the srgb standard.
        from_reference: !<GroupTransform>
          children:
            - !<ColorSpaceTransform> {src: lnf, dst: lg16}
            - !<FileTransform> {src: lg16_to_srgb8.spi3d, interpolation: linear}


To explain how this all ties together to display an image, say we have
an image in the ``lnf`` colorspace (e.g a linear EXR) and wish to
convert it to ``srgb8`` - the transform steps are:

* ``ColorSpaceTransform`` is applied, converting from lnf to lg16
* The ``FileTransform`` is applied, converting from lg16 to srgb8.

A more complex example: we have an image in the ``lg16`` colorspace,
and convert to ``srgb8`` (using the lg16 definition from earlier, or
the :ref:`config-spivfx` config):

First OCIO converts from lg16 to the reference space, using the transform defined in lg16's to_reference:

* ``FileTransform`` applies the lg16.spi1d

With the image now in the reference space, srgb8's transform is applied:

* ColorSpaceTransform to transform from lnf to lg16
* FileTransform applies the ``lg16_to_srgb8.spi3d`` LUT.

.. note::

    OCIO has an transform optimiser which removes redunant steps, and
    combines similar transforms into one operation.

    In the previous example, the complete transform chain would be
    "lg16 -> lnf, lnf -> lg16, lg16 -> srgb8". However the optimiser
    will reduce this to "lg16 -> srgb".


``bitdepth``
------------

Optional. Default: ``32f``


Specify an appropriate bitdepth for the colorspace, and applications
can use this to automatically output images in the correct bit-depth.

Valid options are:

  * ``8ui``
  * ``10ui``
  * ``12ui``
  * ``14ui``
  * ``16ui``
  * ``32ui``
  * ``16f``
  * ``32f``

The number is in bits. ``ui`` stands for unsigned integer. ``f``
stands for floating point.

Example:

.. code-block:: yaml

    - !<ColorSpace>
      name: srgb8
      bitdepth: 8ui

      from_reference: [...]


``isdata:``
-----------

Optional. Default: false. Boolean.

The ``isdata`` key on a colorspace informs OCIO that this colorspace
is used for non-color data channels, such as the "normals" output of a
a multipass 3D render.

Here is example "non-color" colorspace from the :ref:`config-spivfx`
config:

.. code-block:: yaml

    - !<ColorSpace>
      name: ncf
      family: nc
      equalitygroup:
      bitdepth: 32f
      description: |
        ncf :nc,Non-color used to store non-color data such as depth or surface normals
      isdata: true
      allocation: uniform


``equalitygroup:``
------------------

Optional.

If two colorspaces are in the "equality group", transforms between
them are considered non-operations.

You might have multiple colorspaces which are identical, but operate
at different bit-depths.

For example, see the ``lg10`` and ``lg16`` colorspaces in the
:ref:`config-spivfx` config. If loading a ``lg10`` image and
converting to ``lg16``, no transform is required. This is of course
faster, but may cause an unexpected increase in precision (e.g it skip
potential clamping caused by a LUT)

.. code-block:: yaml

    - !<ColorSpace>
      name: lg16
      equalitygroup: lg
      bitdepth: 16ui
      to_reference: !<FileTransform> {src: lg16.spi1d, interpolation: nearest}

    - !<ColorSpace>
      name: lg10
      equalitygroup: lg
      bitdepth: 10ui
      to_reference: !<FileTransform> {src: lg10.spi1d, interpolation: nearest}

**Do not** put different colorspaces in the same equality group. For
  logical grouping of "similar" colorspaces, use the ``family``
  option.


``family:``
-----------

Optional.

Allows for logical grouping of colorspaces within a UI.

For example, a series of "log" colorspaces could be put in one
"family". Within a UI like the Nuke ``OCIOColorSpace`` node, these
will be grouped together.


.. code-block:: yaml

  - !<ColorSpace>
    name: kodaklog
    family: log
    equalitygroup: kodaklog
    [...]

  - !<ColorSpace>
    name: si2klog
    family: log
    equalitygroup: si2klog
    [...]

  - !<ColorSpace>
    name: rec709
    family: display
    equalitygroup: rec709
    [...]


Unlike ``equalitygroup``, the ``family`` has no impact on image
processing.


``allocation`` and ``allocationvars``
-------------------------------------

Optional.

These two options are used when OCIO transforms are applied on the
GPU.

It is also used to automatically generate a "shaper LUT" when
:ref:`baking LUT's <userguide-bakelut>` unless one is explicitly
specified (not all output formats utilise this)

For a detailed description, see :ref:`allocationvars`

Example of a "0-1" colorspace

.. code-block:: yaml

    allocation: uniform
    allocationvars: [0.0, 1.0]

.. code-block:: yaml

    allocation: lg2
    allocationvars: [-15, 6]


``description``
---------------

Optional.

A human-readable description of the colorspace.

The YAML syntax allows for either single-line descriptions:

.. code-block:: yaml

    - !<ColorSpace>
      name: kodaklog
      [...]
      description: A concise description of the kodaklog colorspace.

Or multiple-lines:

.. code-block:: yaml

    - !<ColorSpace>
      name: kodaklog
      [...]
      description:
        This is a multi-line description of the kodaklog colorspace,
        to demonstrate the YAML syntax for doing so.

        Here is the second line. The first one will be unwrapped into
        a single line, as will this one.


It's common to use literal ``|`` block syntax to preserve all newlines:

.. code-block:: yaml

    - !<ColorSpace>
      name: kodaklog
      [...]
      description: |
        This is one line.
        This is the second.


Available transforms
********************

``AllocationTransform``
+++++++++++++++++++++++

Transforms from reference space to the range specified by the
``vars:``

Keys:

* ``allocation``
* ``vars``
* ``direction``


``CDLTransform``
++++++++++++++++

Applies an ASC CDL compliant grade

Keys:

* ``slope``
* ``offset``
* ``power``
* ``sat``
* ``direction``


``ClampTransform``
++++++++++++++++

Constrain a value to lie between min and max.  If min is
greater than max no clamp is performed.

Keys:

* ``min``
* ``max``
* ``direction``


``ColorSpaceTransform``
+++++++++++++++++++++++

Transforms from ``src`` colorspace to ``dst`` colorspace.

Keys:

* ``src``
* ``dst``
* ``direction``


``ExponentTransform``
+++++++++++++++++++++

Raises pixel values to a given power (often referred to as "gamma")

.. code-block:: yaml

    !<ExponentTransform> {value: [1.8, 1.8, 1.8, 1]}

Keys:

* ``value``
* ``direction``


``FileTransform``
+++++++++++++++++

Applies a lookup table (LUT)

Keys:

* ``src``
* ``cccid``
* ``interpolation``
* ``direction``


``GroupTransform``
++++++++++++++++++

Combines multiple transforms into one.

.. code-block:: yaml

    colorspaces:
    
      - !<ColorSpace>
        name: adx10

        [...]

        to_reference: !<GroupTransform>
          children:
            - !<FileTransform> {src: adx_adx10_to_cdd.spimtx}
            - !<FileTransform> {src: adx_cdd_to_cid.spimtx}

A group transform is accepted anywhere a "regular" transform is.


``LogTransform``
++++++++++++++++

Applies a mathematical logarithm with a given base to the pixel values.

Keys:

* ``base``


``LookTransform``
+++++++++++++++++

Applies a named look


``MatrixTransform``
+++++++++++++++++++

Applies a matrix transform to the pixel values

Keys:

* ``matrix``
* ``offset``
* ``direction``


``TruelightTransform``
++++++++++++++++++++++

Applies a transform from a Truelight profile.

Keys:

* ``config_root``
* ``profile``
* ``camera``
* ``input_display``
* ``recorder``
* ``print``
* ``lamp``
* ``output_camera``
* ``display``
* ``cube_input``
* ``direction``

.. note::

    This transform requires OCIO to be compiled with the Truelight
    SDK present.

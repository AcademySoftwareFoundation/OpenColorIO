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

Optional. Valid values are ``true`` and ``false``. Default is ``true``:

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
the default role will be used.


``luma``
++++++++

Optional. Default is the Rec.709 primaries specified by the ASC:

.. code-block:: yaml

    luma: [0.2126, 0.7152, 0.0722]

These are the luminance coeficients, which can be used by
OCIO-supporting applications when adjusting saturation (e.g in an
image-viewer when displaying a single channel)


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

An example of the ``displays`` section from the :ref:`spi-vfx` config:

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

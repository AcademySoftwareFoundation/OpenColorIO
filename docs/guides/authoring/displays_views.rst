..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _displays_views:

.. _config-displays-views:

Displays & Views
****************

``displays``
^^^^^^^^^^^^

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

A view may be defined as a transform directly from the scene-referred
reference space to the display, as illustrated above.  Alternatively
a view may be defined using the combination of a View Transform and a
Display Color Space.  In this case, the View Transform converts from 
the scene-referred reference space to the display-referred reference
space and then the Display Color Space is used to convert from the
display-referred reference space to the display.  Here is an example:

.. code-block:: yaml

    displays:
      DCIP3:
        - !<View> {name: Film, view_transform: FilmView, display_colorspace: DCIP3}

    view_transforms:
      - !<ViewTransform>
        name: FilmView
        from_reference: <omitted for brevity>

    display_colorspaces:
      - !<ColorSpace> 
        name: DCIP3
        from_display_reference: <omitted for brevity>

The keys allowed with a View are:

* ``name``: A name for the View.
* ``colorspace``: The color space whose transform will be used to
  convert from the reference space to the display.
* ``view_transform``: The transform used to convert from either the
  scene-referred reference to the display-referred reference space 
  or as a transform to be applied in the display-referred reference.
* ``display_colorspace``: The display color space that is used to convert
  from the display-referred reference space to the display.
* ``looks``: One or more Look Transforms to be applied.  The string
  may use the '+' character to apply a look in the forward direction
  and the '-' character to apply in reverse. See :ref:`config-looks`
* ``rule``: The viewing rule to be used with this View. See :ref:`config-viewing-rules`
* ``description``: A description string for this View.

Note that a View may use either the colorspace key or it may use both
the view_transform and dispay_colorspace keys.  No other combinations
are allowed.


.. _view_transforms:

``view_transforms``
^^^^^^^^^^^^^^^^^^^

Optional.  Defines transforms to convert between the OCIO reference spaces.

An OCIO config may contain two Reference spaces: one scene-referred and the other
display-referred.  The View Transforms are mappings between these reference
spaces.  There are a variety of terms used in the industry for these mappings.
In ISO 22028-1 and in ACES, they are called "color-rendering transforms" and
in the ITU standards for HDR television (such as ITU-R BT.2100 and BT.2390)
they are called an "Opto-optical Transfer Function" or OOTF.

It is also possible to specify a View Transform for adjusting display-referred
reference space values. In other words, it may convert from the display-referred
reference space back to itself.  This is useful for describing transforms used
for HDR to SDR video conversion, or vice versa.

A View Transform may use the following keys:

* ``name``: A name for the ViewTransform.
* ``description``: A description of the ViewTransform.
* ``family``: A family string (similar to ColorSpace).
* ``categories``: The categories used for menu filtering (similar to ColorSpace).
* ``from_scene_reference``: The transform from the scene-referred reference space
  to the display-referred reference space.
* ``to_scene_reference``: The transform from the display-referred reference space
  to the scene-referred reference space.
* ``from_display_reference``: The transform from the display-referred reference 
  space to the display-referred reference space.
* ``to_display_reference``: The inverse of the from_display_reference transform.

.. TODO: Good spot for an example in a future revision.

``default_view_transform``
^^^^^^^^^^^^^^^^^^^^^^^^^^

Optional.  Defines the default view transform.

The default view transform is the view transform that is used if a ColorSpaceTransform
needs to convert between a scene-referred and display-referred colorspace.  If this
key is missing, the first view transform in the config is used.

.. code-block:: yaml

  default_view_transform: un-tone-mapped


.. _shared_views:

``shared_views``
^^^^^^^^^^^^^^^^

Optional. Allows a view to be defined once for use in multiple displays.

For example, this:

.. code-block:: yaml

  shared_views:
    - !<View> {name: Log, colorspace: lg10}
    - !<View> {name: Raw, colorspace: nc10}

  displays:
    DCIP3:
      - !<View> {name: Film, colorspace: p3dci8}
      - !<Views> [ Log, Raw ]
    sRGB:
      - !<View> {name: Film, colorspace: srgb8}
      - !<Views> [ Log, Raw ]

Is equivalent to this:

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

A shared view may use the special token <USE_DISPLAY_NAME> to request
that the display_colorspace name be set equal to the display name that
the shared view is used with.  In the following example, when the ACES
shared view is used with the sRGB display, its display_colorspace is
set to "sRGB" and when it is used with the Rec709 display, its 
display_colorspace is set to "Rec709".

.. code-block:: yaml

  shared_views:
    - !<View> {name: ACES, view_transform: ACES-sdr-video, 
               display_colorspace: <USE_DISPLAY_NAME>}
    - !<View> {name: Log, colorspace: lg10}
    - !<View> {name: Raw, colorspace: nc10}

  displays:
    sRGB:
      - !<Views> [ ACES, Log, Raw ]
    Rec709:
      - !<Views> [ ACES, Log, Raw ]

  display_colorspaces:
    - !<ColorSpace> 
      name: sRGB
      from_display_reference: <omitted for brevity>
    - !<ColorSpace> 
      name: Rec709
      from_display_reference: <omitted for brevity>

As with any other view, a shared view may appear (or not) in the list of 
active_views.

Application developers do not need to worry about shared views since the
API presents them as if they were a typical View.


.. _active-displays:

``active_displays``
^^^^^^^^^^^^^^^^^^^

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


The value can be overridden by the ``OCIO_ACTIVE_DISPLAYS``
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


.. _active-views:

``active_views``
^^^^^^^^^^^^^^^^

Optional. Default is for all views to be visible, and to respect order
of the views under the display.

Works identically to ``active_displays``, but controls which *views* are
visible.

Overridden by the ``OCIO_ACTIVE_VIEWS`` env-var::

    export OCIO_ACTIVE_VIEWS="Film:Log:Raw"


.. _virtual-display:

``virtual_display``
^^^^^^^^^^^^^^^^^^^

Optional. A virtual display may be defined to allow OCIO to instantiate new 
displays from ICC profiles.

The syntax is similar to a conventional display.  It may incorporate both 
views and shared views.  There may only be one virtual display in a config.

When OCIO instantiates a display from an ICC monitor profile it will create
a display colorspace which is used with any views that have the display_colorspace
set to ``<USE_DISPLAY_NAME>``.

So in this example, if the application asks OCIO to instantiate a display
from an ICC profile, the user would then see a second display, in addition
to "sRGB", named after the ICC profile.  The views available for that new
display are taken from the virtual display.

.. code-block:: yaml

  displays:
    sRGB:
      - !<View> {name: ACES, view_transform: ACES-sdr-video, 
                 display_colorspace: <USE_DISPLAY_NAME>}
      - !<View> {name: Log, colorspace: lg10}
      - !<Views> [ Raw ]

  virtual_display:
    - !<View> {name: ACES, view_transform: ACES-sdr-video, 
               display_colorspace: <USE_DISPLAY_NAME>}
    - !<View> {name: Log, colorspace: lg10}
    - !<Views> [ Raw ]

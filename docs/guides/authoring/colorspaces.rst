..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _colorspaces:


.. _config-colorspaces:

Colorspaces
***********

``colorspaces``
^^^^^^^^^^^^^^^

Required.

This section is a list of the scene-referred colorspaces in the config.
A colorspace may be referred to elsewhere within the config (including
other colorspace definitions), and are used within OCIO-supporting
applications.

A colorspace may use the following keys:


``to_scene_reference`` and ``from_scene_reference``
---------------------------------------------------

These keys specify the transforms that define the relationship between
the colorspace and the scene-referred reference space.

Note: In OCIO v1, the keys ``to_reference`` and ``from_reference`` were
used (since there was only one reference space).  These are still supported
as synonyms.

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
        to_scene_reference: !<FileTransform> {src: lg16_to_lnf.spi1d, interpolation: nearest}


First the ``lnf`` colorspace (short for linear float) is used as our
reference colorspace. The name can be anything, but the idea of a
reference colorspace is an important convention within OCIO: **all
other colorspaces are defined as transforms either to or from this
colorspace.**

The ``lg16`` colorspace is a 16-bit log colorspace (see
:ref:`config-spivfx` for an explanation of this colorspace). It has a
name, a bit-depth and a description.

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
``from_scene_reference``.

.. code-block:: yaml

      - !<ColorSpace>
        name: srgb8
        bitdepth: 8ui
        description: |
          srgb8 :rgb display space for the srgb standard.
        from_scene_reference: !<FileTransform> {src: srgb8.spi3d, interpolation: linear}

We use ``from_scene_reference`` here because we have a LUT which transforms
from the reference colorspace (``lnf`` in this example) to sRGB.

In this case ``srgb8.spi3d`` is a complex 3D LUT which cannot be
inverted, so it is considered a "display only" colorspace. If we did have a second 3D LUT to apply the inverse transform, we can specify both ``to_scene_reference`` and ``from_scene_reference``


.. code-block:: yaml

      - !<ColorSpace>
        name: srgb8
        bitdepth: 8ui
        description: |
          srgb8 :rgb display space for the srgb standard.
        from_scene_reference: !<FileTransform> {src: lnf_to_srgb8.spi3d, interpolation: linear}
        to_scene_reference: !<FileTransform> {src: srgb8_to_lnf.spi3d, interpolation: linear}

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
        from_scene_reference: !<GroupTransform>
          children:
            - !<ColorSpaceTransform> {src: lnf, dst: lg16}
            - !<FileTransform> {src: lg16_to_srgb8.spi3d, interpolation: linear}

Here to get from the reference colorspace, we first use the
``ColorSpaceTransform`` to convert from ``lnf`` to ``lg16``, then
apply our 3D LUT on the log-encoded images.

.. TODO: Eventually, we could :cpp:class: these class references to the API doc sections:
.. https://www.sphinx-doc.org/en/1.5.1/domains.html

This primarily demonstrates the meta-transform ``GroupTransform``: a
transform which simply composes two or more transforms together into
one. Anything that accepts a transform like ``FileTransform`` or
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
        to_scene_reference: !<FileTransform> {src: lg16.spi1d, interpolation: nearest}

      - !<ColorSpace>
        name: srgb8
        bitdepth: 8ui
        description: |
          srgb8 :rgb display space for the srgb standard.
        from_scene_reference: !<GroupTransform>
          children:
            - !<ColorSpaceTransform> {src: lnf, dst: lg16}
            - !<FileTransform> {src: lg16_to_srgb8.spi3d, interpolation: linear}


To explain how this all ties together to display an image, say we have
an image in the ``lnf`` colorspace (e.g. a linear EXR) and wish to
convert it to ``srgb8`` - the transform steps are:

* ``ColorSpaceTransform`` is applied, converting from lnf to lg16
* The ``FileTransform`` is applied, converting from lg16 to srgb8.

A more complex example: we have an image in the ``lg16`` colorspace,
and convert to ``srgb8`` (using the lg16 definition from earlier, or
the :ref:`config-spivfx` config):

First OCIO converts from lg16 to the reference space, using the transform defined in lg16's to_scene_reference:

* ``FileTransform`` applies the lg16.spi1d

With the image now in the reference space, srgb8's transform is applied:

* ColorSpaceTransform to transform from lnf to lg16
* FileTransform applies the ``lg16_to_srgb8.spi3d`` LUT.

.. note::

    OCIO has an transform optimizer which removes redundant steps, and
    combines similar transforms into one operation.

    In the previous example, the complete transform chain would be
    "lg16 -> lnf, lnf -> lg16, lg16 -> srgb8". However the optimizer
    will reduce this to "lg16 -> srgb".


``encoding``
------------

Optional.  Specify how color space values are numerically encoded.

It is very helpful for applications to be able to know the basic type 
of encoding of a color space. For example, it is well known that the 
performance of various types of image processing algorithms varies based 
on the type of encoding. Applying a spatial filter to a scene-linear 
image gives a different subjective result than if applied to the same 
image encoded in a log color space. Likewise certain algorithms such as 
keying or tracking may assume that the color encoding is roughly 
perceptually uniform and thus may have difficulties with scene-linear 
images.

The allowed values and definitions are:

``scene-linear`` -- A scene-referred encoding where the numeric 
representation is proportional to scene luminance. Examples: ACES2065-1, ACEScg.

``display-linear`` -- A display-referred encoding where the numeric 
representation is proportional to display luminance. Example: CIE XYZ values 
measured off of a display or projection screen.

``log`` -- A scene-referred encoding where the numeric representation is roughly 
proportional to the logarithm of scene-luminance (often with some divergence 
in the shadows as with most camera log encodings). Examples: ACEScct, ACEScc, 
ARRI LogC, Sony S-Log3/S-Gamut3.

``sdr-video`` -- A display-referred encoding where the numeric representation is 
proportional to an SDR video signal. Examples: Rec.709/Rec.1886 video, sRGB.

``hdr-video`` -- A display-referred encoding where the numeric representation is 
proportional to an HDR video signal. Examples: Rec.2100/PQ or Rec.2100/HLG.

``data`` -- A non-color channel. Note that typically such a color space would 
also have the isdata attribute set to true. Examples: alpha, normals, Z-depth.


``bitdepth``
------------

Optional. Default: ``32f``


Specify an appropriate bit-depth for the colorspace, and applications
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

      from_scene_reference: [...]


``isdata``
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


``equalitygroup``
------------------

Optional.

If two colorspaces are in the "equality group", transforms between
them are considered non-operations.

You might have multiple colorspaces which are identical, but operate
at different bit-depths.

For example, see the ``lg10`` and ``lg16`` colorspaces in the
:ref:`config-spivfx` config. If loading a ``lg10`` image and
converting to ``lg16``, no transform is required. This is of course
faster, but may cause an unexpected increase in precision (e.g. it skip
potential clamping caused by a LUT)

.. code-block:: yaml

    - !<ColorSpace>
      name: lg16
      equalitygroup: lg
      bitdepth: 16ui
      to_scene_reference: !<FileTransform> {src: lg16.spi1d, interpolation: nearest}

    - !<ColorSpace>
      name: lg10
      equalitygroup: lg
      bitdepth: 10ui
      to_scene_reference: !<FileTransform> {src: lg10.spi1d, interpolation: nearest}

**Do not** put different colorspaces in the same equality group. For
  logical grouping of "similar" colorspaces, use the ``family``
  option.


``family``
-----------

Optional.

Allows for logical grouping of colorspaces within a UI.

For example, a series of "log" colorspaces could be put in one
"family". Within a UI like the Nuke ``OCIOColorSpace`` node, these
will be grouped together.

The Menu Helpers API allows applications to build hierarchical menus 
for color spaces based on the ``family`` key.  The ``family_separator``
key of the config is used to define the character used to separate the
family string into tokens.

.. code-block:: yaml

  family_separator: /

  color_spaces:
    - !<ColorSpace>
      name: ACME_log4
      family: Log/Cameras/ACME
      equalitygroup: ACME_log4
      [...]

    - !<ColorSpace>
      name: ACEScct
      family: Log/ACES
      equalitygroup: ACEScct
      [...]

    - !<ColorSpace>
      name: Rec.709
      family: Video/Broadcast/SDR
      equalitygroup: Rec.709
      [...]

Unlike ``equalitygroup``, the ``family`` has no impact on image
processing.


``aliases``
-----------

Optional.

The aliases key is used to define alternate names for the colorspace.
For example, it may be useful to define a shorter version of the name
that is easier to include in texture path names.  Or it may be necessary
to define an older version of the name for the color space for backwards
compatibility.

.. code-block:: yaml

    aliases: [shortName, obsoleteName]


``allocation`` and ``allocationvars``
-------------------------------------

Optional.

These two options were used in OCIO v1 when transforms were applied on the
GPU.  However, the new GPU renderer in OCIO v2 does not need these.

However, they may still be used to automatically generate a "shaper LUT" when
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


.. _config-display-colorspaces:

Display Colorspaces
*******************

``display_colorspaces``
^^^^^^^^^^^^^^^^^^^^^^^

Optional.

This section is a list of all the display-referred colorspaces in the config. 
A display colorspace is very similar to a colorspace except its transforms
go from or to the display-referred reference space rather than the
scene-referred reference space.

A display colorspace may use all the same keys as a colorspace except
it uses ``to_display_reference`` and ``from_display_reference`` rather
than ``to_scene_reference`` and ``from_scene_reference`` to specify
its transforms.

.. code-block:: yaml

    display_colorspaces:

      - !<ColorSpace>
        name: sRGB
        family: 
        description: |
          sRGB monitor (piecewise EOTF)
        isdata: false
        categories: [ file-io ]
        encoding: sdr-video
        from_display_reference: !<GroupTransform>
          children:
            - !<BuiltinTransform> {style: "DISPLAY - CIE-XYZ-D65_to_sRGB"}
            - !<RangeTransform> {min_in_value: 0., min_out_value: 0., max_in_value: 1., max_out_value: 1.}

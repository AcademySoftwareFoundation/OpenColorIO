.. _userguide-tooloverview:

Tool overview
=============

OCIO is comprised of many parts. At the lowest level there is the C++ API.
This API can be used in applications and plugins

Note that all these plugins use the same config file to define color spaces,
roles and so on. For information on setting up configurations, see 
:ref:`configurations`

The API
*******

Most users will never directly interact with the C++ API. However the API is
used by all the supplied applications (e.g :ref:`overview-ocio2icc`) and plugins
(e.g the :ref:`overview-nukeplugins`)

To get started with the API, see the :ref:`developer-guide`

.. _overview-ociocheck:

ociocheck
*********

This is a command line tool which shows an overview of an OCIO config
file, and check for obvious errors

For example, the following shows the output of a config with a typo -
the colorspace used for ``compositing_log`` is not incorrect::

    $ ociocheck --iconfig example.ocio

    OpenColorIO Library Version: 0.8.3
    OpenColorIO Library VersionHex: 525056
    Loading example.ocio

    ** General **
    Search Path: luts
    Working Dir: /tmp

    Default Display: sRGB
    Default View: Film

    ** Roles **
    ncf (default)
    lnf (scene_linear)
    NOT DEFINED (compositing_log)

    ** ColorSpaces **
    lnf
    lgf
    ncf
    srgb8 -- output only

    ERROR: Config failed sanitycheck. The role 'compositing_log' refers to a colorspace, 'lgff', which is not defined.

    Tests complete.

It cannot verify the defined color transforms are "correct", only that
the config file can be loaded by OCIO without error. Some of the
problems it will detect are:

* Duplicate colorspace names
* References to undefined colorspaces
* Required roles being undefined
* At least one display device is defined


As with all the OCIO command line tools, you can use the `--help` argument to
read a description and see the other arguments accepted::

    $ ociocheck --help
    ociocheck -- validate an OpenColorIO configuration

    usage:  ociocheck [options]

        --help        Print help message
        --iconfig %s  Input .ocio configuration file (default: $OCIO)
        --oconfig %s  Output .ocio file


.. _overview-ociobakelut:

ociobakelut
***********

A command line tool which bakes a color transform into various color
lookup file formats ("a LUT")

This is intended for applications that have not directly integrated
OCIO, but can load LUT files

If we want to create a ``lnf`` to ``srgb8`` viewer LUT for Houdini's
MPlay::

    $ ociobakelut --inputspace scene_linear --shaperspace lg10 --outputspace srgb8 --format houdini houdini__lnf_to_lg10_to_srgb8.lut

The ``--inputspace`` and ``-outputspace`` options specify the
colorspace of the input image, and the displayed image.

Since a 3D LUT can only practically operate on 0-1 (e.g a Log image),
the ``--shaperspace`` option is specified. This uses the Houdini LUT's
1D "pretransform" LUT to do "lnf" to "lg10", then the 3D LUT part to
go from "lg10" to "srgb8" (basically creating a single file containing
a 1D linear-to-log LUT, and a 3D log-to-sRGB LUT)

To make a log to sRGB LUT for Flame, the usage is similar, except the
shaperspace option is omitted, as the input colorspace does not have
values outside 0.0-1.0 (being a Log space)::

    $ ociobakelut --inputspace lg10 --outputspace srgb8 --format flame flame__lg10_to_srgb.3dl

See the :ref:`faq-supportedlut` section for a list of formats that
support baking

.. TODO: For more information on baking LUT's, see :ref:`userguide-bakelut`


.. _overview-ocio2icc:

ocio2icc
********

A command line tool to generate an ICC "proofing" profile from a color space
transform, which can be used in applications such as Photoshop.

A common workflow is for matte-painters to work on sRGB files in Photoshop. An
ICC profile is used to view the work with the same film emulation transform as
used in other departments.

.. TODO: Link to more elaborate description


.. _overview-ocioconvert:

ocioconvert
***********

Loads an image, applies a color transform, and saves it to a new file.

OpenImageIO is used to open and save the file, so a wide range of formats are
supported.

.. TODO: Link to more elaborate description


.. _overview-ociodisplay:

ociodisplay
***********

A basic image viewer. Uses OpenImageIO to load images, and displays them using
OCIO and typical viewer controls (scene-linear exposure control and a
post-display gamma control)

May be useful to users to quickly check colorspace configuration, but
primarily a demonstration of the OCIO API

.. TODO: Link to more elaborate description


.. _overview-nukeplugins:

Nuke plugins
************

A set of OCIO nodes for The Foundry's Nuke, including:

* OCIOColorSpace, transforms between two color spaces (similar to the built-in
  "ColorSpace" node, but the colorspaces are described in the OCIO config file)

* OCIODisplay to be used as viewer processes

* OCIOFileTransform loads a transform from a file (e.g a 1D or 3D LUT), and
  applies it

* OCIOCDLTransform applies CDL-compliant grades, and includes utilities to
  create/load ASC CDL files

.. TODO - Link to more elaborate description

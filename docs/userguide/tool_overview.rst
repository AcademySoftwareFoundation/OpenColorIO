.. _userguide-tooloverview:

Tool overview
=============

OCIO is comprised of many parts. At the lowest level there is the C++ API.
This API can be used in applications and plugins

Noet that all these plugins use the same config file to define colour spaces,
roles and so on. For information on setting up configurations, see 
:ref:`userguide-configuration`

The API
*******

Most users will never directly interact with the C++ API. However the API is used by all the supplied applications (e.g :ref:`overview-ocio2icc`) and plugins (e.g 
the :ref:`overview-nukeplugins`)

To get started with the API, see the :ref:`developer-guide`

.. _overview-ociocheck:

ociocheck
*********

This is a command line tool to validate an OCIO config file.

It performs various sanity checks, such as that the config does not refer to
an undefined colour space.

It cannot verify the defined color transforms are "correct", only that the
config file can be loaded by OCIO without error.

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
************

A command line tool which bakes a color transform into a lookup table.

.. TODO: Link to more elaborate description


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

OpenImageIO is used to open and save the file, so a wide range of formats are supported.

.. TODO: Link to more elaborate description


.. _overview-ociodisplay:

ociodisplay
***********

A basic image viewer. Uses OpenImageIO to load images, and displays them using OCIO and typical viewer controls (scene-linear exposure control and a post-display gamma control)

May be useful to users to quickly check colorspace configuration, but
primarily a demonstration of the OCIO API

.. TODO: Link to more elaborate description


.. _overview-nukeplugins:

Nuke plugins
************

A set of OCIO nodes for The Foundry's Nuke, including:

* OCIOColorSpace, transforms between two color spaces (similar to the built-in "ColorSpace" node, but the colorspaces are described in the OCIO config file)

* OCIODisplay to be used as viewer processes

* OCIOFileTransform loads a transform from a file (e.g a 1D or 3D LUT), and applies it

* OCIOCDLTransform applies CDL-compliant grades, and includes utilities to create/load ASC CDL files

.. TODO - Link to more elaborate description
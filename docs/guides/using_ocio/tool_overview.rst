..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _userguide-tooloverview:

Tool overview
=============

The following command-line tools are provided with OpenColorIO.  Note that
compilation of the tools requires additional components such as OpenImageIO
and OpenGL.

The --help argument may be provided for info about the arguments and most
tools use the -v argument for more verbose output.

Many of the tools require you to first set the OCIO environment variable to
point to the config file you want to use.

.. _overview-ociocheck:

ociocheck
*********

This is a command-line tool which shows an overview of an OCIO config
file, and checks for obvious errors.

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

    ERROR: Config failed validation. The role 'compositing_log' refers to a colorspace, 'lgff', which is not defined.

    Tests complete.

It cannot verify the defined color transforms are "correct", only that
the config file can be loaded by OCIO without error. Some of the
problems it will detect are:

* Duplicate colorspace names
* References to undefined colorspaces
* Required roles being undefined
* At least one display device is defined
* No v2 features are used in a v1 config


As with all the OCIO command-line tools, you can use the `--help` argument to
read a description and see the other arguments accepted::

    $ ociocheck --help
    ociocheck -- validate an OpenColorIO configuration

    usage:  ociocheck [options]

        --help        Print help message
        --iconfig %s  Input .ocio configuration file (default: $OCIO)
        --oconfig %s  Output .ocio file


.. _overview-ociochecklut:

ociochecklut
************

The ociochecklut tool will load any LUT type supported by OCIO and print 
any errors or warnings encountered.  An RGB or RGBA value may be provided
and will be evaluated through the LUT and the result printed to the console.
Alternatively, the -t option will send a predefined set of test RGB values
through the LUT::

    $  ociochecklut -v acescct_to_rec709.clf  0.18 0.18 0.18

    OCIO Version: 2.0.0dev

    Input  [R G B]: [      0.18      0.18       0.18]
    Output [R G B]: [0.05868321 0.0586832 0.05868318]

The --gpu argument may be used to evaluate using the GPU renderer rather
than the CPU renderer.


.. _overview-ociobakelut:

ociobakelut
***********

A command-line tool which bakes a color transform into various color
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

To make a legacy log to sRGB LUT in 3dl format, the usage is similar, except
the shaperspace option is omitted, as the input colorspace does not have
values outside 0.0-1.0 (being a Log space)::

    $ ociobakelut --inputspace lg10 --outputspace srgb8 --format flame flame__lg10_to_srgb.3dl

See the :ref:`faq-supportedlut` section for a list of formats that
support baking

The ociobakelut command supports many arguments, use the --help argument for
a summary. For more information on baking LUT's, see :ref:`userguide-bakelut`


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


.. _overview-ociolutimage:

ociolutimage
************

Convert a 3D LUT to or from an image.

.. TODO: Link to more elaborate description


.. _overview-ociomakeclf:

ociomakeclf
***********

The ociomakeclf tool converts a LUT into Academy/ASC Common LUT Format (CLF)::

    $ ociomakeclf lut_file.cube lut_file.clf

The --csc argument may be used to convert the LUT into an ACES compliant Look
Modification Transform (LMT) that may be referenced from an ACES Metadata File.
An ACES LMT requires ACES2065-1 color space values on input and output.  The
--csc argument is used to specify the standard color space that the Look LUT
expects on input and output and the tool will prepend a transform from ACES2065-1
to the LUT color space and postpend a transform from that color space back to
ACES2065-1::

    $ ociomakeclf my_ACEScct_look.cube my_LMT.clf --csc ACEScct

The --list argument will print out all of the standard ACES color spaces that are 
supported as --csc arguments.

.. _overview-ocioperf:

ocioperf
********

The ocioperf tool allows you to benchmark the performance of a given color
transformation on your hardware.  Please use the --help argument for a 
description of the options.

.. TODO: Link to more elaborate description

.. _overview-ociowrite:

ociowrite
*********

The ociowrite tool allows you to serialize a color transformation to an XML file.
This is useful for troubleshooting and also to be able to send a complete OCIO
color conversion as a single file.

Note that this command does not do any baking of the transform into another format
and so should give identical results to the original.

The --colorspaces argument specifies the source and destination color spaces for
a ColorSpaceTransform and the --file argument specifies that name of the output file.
The OCIO environment variable is used to specify the config file to be used.

The two file formats supported are CTF and CLF and this is selected by the extension
you provide to the --file argument.  The CTF format is recommended because it is able
to represent all OCIO transforms and operators.  The CLF format is also allowed since
it has wider support in non-OCIO applications but the tool will not write the file if
the transformation would require an operator that is not supported by CLF.

Here is an example::

    $ export OCIO=/path/to/the/config.ocio
    $ ociowrite --colorspaces acescct aces2065-1 --file mytransform.ctf

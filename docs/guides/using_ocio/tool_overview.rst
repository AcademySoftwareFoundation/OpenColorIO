..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

.. _userguide-tooloverview:

Tool overview
=============

The following command-line tools are provided with OpenColorIO.

The --help argument may be provided for info about the arguments and most
tools use the -v argument for more verbose output.

Many of the tools require you to first set the OCIO environment variable to
point to the config file you want to use.

Note that some tools depend on OpenEXR or OpenImageIO and other libraries:
 * ociolutimage: (OpenEXR or OpenImageIO)
 * ociodisplay: (OpenEXR or OpenImageIO), OpenGL, GLEW, GLUT
 * ocioconvert: (OpenEXR or OpenImageIO)

.. TODO: link to build instructions
.. TODO: check app lib dependencies
.. TODO: make a pretty table in RST.

.. _overview-ocioarchive:

ocioarchive
***********

This command-line tool allows you to convert a config and its external LUT files
into an OCIOZ archive file.  A .ocioz file may be supplied to any command that
takes the path to a config or set as the OCIO environment variable.

This example creates a file called myarchive.ocioz::

    $ ocioarchive myarchive --iconfig myconfig/config.ocio

This command will expand it back out::

    $ ocioarchive --extract myarchive.ocioz

The --list option may be used to see the contents of a .ocioz file.


.. _overview-ociocheck:

ociocheck
*********

This is a command-line tool which shows an overview of an OCIO config
file, and checks for obvious errors.

For example, the following shows the output of a config with a typo -
the colorspace used for ``compositing_log`` is incorrect::

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
support baking, and see the :ref:`userguide-bakelut` for more information
on baking LUTs.

The ociobakelut command supports many arguments, use the --help argument for
a summary. 


.. _overview-ocioconvert:

ocioconvert
***********

Loads an image, applies a color transform, and saves it to a new file.

The ocioconvert tool applies either an aribtrary LUT, or a complex OpenColorIO 
transform. OCIO transforms can be from an input color space to either an
output color space or a (display,view) pair. 

Both CPU (default) and GPU renderers are supported. The --gpuinfo argument 
may be used to output the shader program used by the GPU renderer.

Uses OpenImageIO or OpenEXR for opening and saving files and modifying
metadata. Supported formats will vary depending on the use of OpenImageIO.
Use the --help argument for more information on to the available options.

.. TODO: Examples


.. _overview-ociodisplay:

ociodisplay
***********

An example image viewer demonstrating the OCIO C++ API. 

Uses OpenImageIO or OpenEXR to load images, and displays them using OCIO and
typical viewer controls (scene-linear exposure control and a
post-display gamma control).

May be useful to users to quickly check a color space configuration.

NOTE: This program is not a very good example of how to build a UI.
For example, it assumes each display has the same views, which is often
not the case.  Also, it does not leverage any of the new OCIO v2 features.

.. TODO: Link to discussion of OpenImageIO source?


.. _overview-pyociodisplay:

pyociodisplay
*************

The pyociodisplay tool is a minimal image viewer implementation demonstrating use of 
the OCIO GPU renderer in a Python application.  It requires downloading a few dependencies
before use.  For more information, please see the 
`README. <https://github.com/AcademySoftwareFoundation/OpenColorIO/tree/main/src/apps/pyociodisplay>`_ 


.. _overview-ociolutimage:

ociolutimage
************

The ociolutimage tool converts a 3D LUT to or from an image.

Image containers are occasionally used for encoding and exchanging simple color 
lookup data where standard LUT formats are less feasible. The ociolutimage tool
offers an arguably "artist-friendly", WYSIWYG workflow for creating LUTs 
representing arbitrary color transforms. 

The workflow is a three step process::

    1. Generate an identity lattice image with ociolutimage --generate
    2. Apply color transforms to the generated image (e.g., in a DCC application)
    3. Extract LUT data from the modified RGB lattice values with 
       ociolutimage --extract

.. TODO: Rephrase. (This feels a little awkward)
.. TODO: Caveats -- permissible types of transforms
.. TODO: Discussion -- preserving extended range
.. TODO: Tutorial? -- shapers + 'pre-baked' inverse shapers

.. seealso:: 
    Nuke's `CMSTestPattern <https://learn.foundry.com/nuke/content/reference_guide/color_nodes/cmstestpattern.html>`_ and `GenerateLUT <https://learn.foundry.com/nuke/content/reference_guide/color_nodes/generatelut.html>`_ nodes are analogous to the
    ociolutimage --generate and --extract options, respectively. Applications such as `Lattice <https://videovillage.co/lattice>`_ provide similar functionality.


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

The metric used for assessing performance is the time taken to apply a 
transform to an image with respect to each pixel, to each line, or to the 
entire image plane (or all three). By default, each test is run ten times. 

Transforms are either provided as an external file or specified in the active 
config (i.e., the config pointed to by the OCIO environment variable).

Examples::

    $ ocioperf —displayview ACEScg sRGB ‘Show LUT’ —iter 20 —image test.exr 
    # Measures an ACEScg —> sRGB / ‘Show LUT’ DisplayViewTransform applied to each 
    # pixel of ‘test.exr’ twenty times.

    $ ocioperf —transform my_transform.ctf —out f32 —image meow.jpg
    # Measures ‘my_transform.ctf’ applied to the whole ‘meow.jpg’ image and output 
    # as 32-bit float data ten times.

    $ ocioperf —colorspaces ‘LogC AWG’ ACEScg —test 1 —image marcie.dpx
    # Measures a ‘LogC AWG’ —> ACEScg ColorSpaceTransform applied to each line of 
    # ‘marcie.dpx’ ten times.

.. TODO: examples formatting


.. _overview-ocioview:

ocioview
********

This is a new GUI tool for inspecting and editing config files. It is currently an
alpha release and we are looking for contributors to extend it or provide tutorials.
Please see the README in apps/ocioview for details about installation.


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


.. _overview-pyocioamf:

pyocioamf
*********

The pyocioamf tool is an initial attempt to support the ACES Metadata File (AMF)
`format. <https://docs.acescentral.com/guides/amf/>`_
This Python script will take an AMF file and produce an OCIO CTF file that implements its color
pipeline.  The CTF file may be applied to images using tools such as :ref:`overview-ocioconvert`.
For more information, please see the 
`README. <https://github.com/AcademySoftwareFoundation/OpenColorIO/tree/main/src/apps/pyocioamf>`_ 

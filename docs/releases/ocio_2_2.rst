..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.


OCIO 2.2 Release
================

Timeline
********

OpenColorIO 2.2 was delivered in October 2022 and is in the VFX Reference Platform for
calendar year 2023.


New Feature Guide
=================

Built-in Configs
****************

The OCIO Configs Working Group has put a lot of effort into building new ACES configs that
take advantage of the features of OCIO v2 and that are informed by the learnings from the 
previous ACES configs.  The new configs are available from the project's GitHub repo: 
`OpenColorIO-Config-ACES 
<https://github.com/AcademySoftwareFoundation/OpenColorIO-Config-ACES/releases/tag/v1.0.0>`_

To make them easily accessible, they are built in to the library itself and may be
accessed directly from within applications that incorporate the OCIO 2.2 library. 

For Users
+++++++++

Wherever you are able to provide a file path to a config, you may now provide a URI-type 
string to instead use one of the built-in configs. For example, you may use these strings 
for the OCIO environment variable.

To use the :ref:`aces_cg`, use this string for the config path::
    ocio://cg-config-v1.0.0_aces-v1.3_ocio-v2.1

To use the :ref:`aces_studio`, use this string for the config path::
    ocio://studio-config-v1.0.0_aces-v1.3_ocio-v2.1

This string will give you the current default config, which is currently the ACES CG Config::
    ocio://default

In future releases, it is expected that updated versions of these configs will be added, 
each with a unique name string. However, the previous strings will continue to be 
supported for backwards compatibility.

For Developers
++++++++++++++

The new URI-type strings may be passed into the existing APIs such as ``CreateFromEnv`` 
and ``CreateFromFile``.  There is also a new ``CreateFromBuiltinConfig``.  No changes 
should be needed, but please test that users of your application may use a path containing 
a URI-type string rather than a path to a config.ocio file.

If you'd like to offer the built-in configs from your user interface, there is an API to 
access them through the new ``BuiltinConfigRegistry``.  Here is basic Python code to 
access the registry and print the strings to present in a UI::

    registry = ocio.BuiltinConfigRegistry().getBuiltinConfigs()
    for item in registry:
        # The short_name is the URI-style name.
        # The ui_name is the name to use in a user interface.
        short_name, ui_name, isRecommended, isDefault = item

        # Don't present built-in configs to users if they are no longer recommended.
        if isRecommended:
            print(ui_name)

Prints:

    Academy Color Encoding System - CG Config [COLORSPACES v1.0.0] [ACES v1.3] [OCIO v2.1]

    Academy Color Encoding System - Studio Config [COLORSPACES v1.0.0] [ACES v1.3] [OCIO v2.1]

If your application saves the path to a config and the user enters ``ocio://default``, it 
is recommended that you don't save that as is.  Instead, call 
``getDefaultBuiltinConfigName`` to get the name of the current default.  This is 
guaranteed to give the same results, whereas the default config could change somewhat 
between releases.  Prepend ``ocio://`` in order to save it as a valid config path.  Here's 
the Python code::

    "ocio://" + ocio.BuiltinConfigRegistry().getDefaultBuiltinConfigName()


Config Archiving
****************

An OCIO config, including its external LUT files, may now be packaged into a single file.  
This makes it easier to distribute configs and may slightly discourage tampering with the 
contents.  An archived config may be created with the new ``ocioarchive`` command-line 
tool and is stored in a file with the extension ``.ocioz``.  It is basically a compressed 
ZIP archive of the config and its LUTs.

To be archivable, the external LUT files must be contained within (or below) the config's 
working directory. (In OCIO terminology, the "working directory" is the directory that 
contains the config.ocio file.)

A config is not archivable if any of the following are true:

* It contains FileTransforms with a src outside the working directory
* The search path contains paths outside the working directory
* The search path contains paths that start with a context variable
* The working directory is not set (if archiving a Config object)

Context variables are allowed but the intent is that they may only resolve to paths that
are within or below the working directory.  This is because the archiving function will
only archive files that are within the working directory in order to ensure that if it is
later expanded, that it will not create any files outside this directory.

For example, a context variable on the search path intended to contain the name of a 
sub-directory under the working directory must have the form ``./$DIR_NAME`` rather than 
just ``$DIR_NAME`` to be considered archivable. This is imperfect since there is no way to
prevent the context variable from creating a path outside the working dir, but it should
at least draw attention to the fact that the archive would fail if used with context vars
that try to abuse the intended functionality.

All files within or below the working directory that end with an extension corresponding 
to a LUT type supported by OCIO are added to the archive.  This approach was taken since 
the goal was for the archive to work with any combination of context variables that the 
original config works with.  Files with other extensions or no extension are not archived.


For Users
+++++++++

Wherever you are able to provide a file path to a config, you may now provide a path to a 
.ocioz file. For example, you may use this for the OCIO environment variable.

The ``ocioarchive`` command-line tool will allow you to archive a config or to expand an 
existing archive.  It will also allow you to list the contents of an archived config.  Run 
``ocioarchive --help`` to get a print-out of the various options.

The print out from the ``ociocheck`` command-line tool now includes a new line-item 
indicating whether the config is archivable.

For Developers
++++++++++++++

The new .ocioz files may be passed into the existing APIs such as ``CreateFromEnv`` and 
``CreateFromFile``.  No changes should be needed, but please test that users of your 
application may use a path containing a .ocioz file rather than a path to a config.ocio 
file.

The Config class has new ``isArchivable`` and ``archive`` methods.  There is also an 
``ExtractOCIOZArchive`` function.


Abstracting the Source of External LUT Files
********************************************

The new ConfigIOProxy class allows the calling program to supply the config and any 
associated LUT files directly, without relying on the standard file system.  This opens 
the door to expanded ways in which OCIO may be used.

The new config archiving feature was implemented using this mechanism.

For Developers
++++++++++++++

Please refer to the ``ConfigIOProxy`` class.  By implementing the ``getLutData``, 
``getConfigData``, and ``getFastLutFileHash`` methods, you have control over how the 
config is provided to OCIO.  No file system access to a config is required.

The ``CreateFromConfigIOProxy`` factory allows for the creation of a Config object from a 
ConfigIOProxy object.


Converting To or From a Known Color Space
*****************************************

An OCIO config defines its own self-contained universe of color spaces.  But there are not 
any requirements for color spaces which must always be included or how they must be named.  
This poses difficulties for many applications which need to convert to or from certain 
known standard color spaces.  For example, a renderer might have a physical sun and sky 
model which produces colors in a CIE space and it needs to convert those into the 
rendering space defined by a user's custom OCIO config.  Or an application may use an SDK 
to debayer images from a digital cinema camera.  The SDK produces images in a specific 
color space which then needs to be processed into something viewable through a user's 
custom OCIO config.

For Developers
++++++++++++++

OCIO v2 introduced the Interchange Roles to help address this problem but these had 
previously been optional and are unlikely to be included in OCIO v1 configs (although it 
would be perfectly legal to add them).

OCIO 2.2 introduces the new functions ``GetProcessorToBuiltinColorSpace`` and 
``GetProcessorFromBuiltinColorSpace`` that will allow you to convert to or from any of the 
color spaces in the built-in Default config (this is currently the ACES CG config 
described above).  This built-in config includes common spaces such as "Linear Rec.709 
(sRGB)", "sRGB - Texture", "ACEScg", and "ACES2065-1".

If the source config defines the necessary Interchange Role (typically 
``aces_interchange``), then the conversion will be well-defined and equivalent to calling 
``GetProcessorFromConfigs`` with the source config and the Built-in config

However, if the Interchange Roles are not present, heuristics will be used to try and 
identify a common color space in the source config that may be used to allow the 
conversion to proceed. If the heuristics fail to find a suitable space, an exception is 
thrown. The heuristics may evolve, so the results returned by this function for a given 
source config and color space may change in future releases of the library. However, the 
Interchange Roles are required in config versions 2.2 and higher, so it is hoped that the 
need for the heuristics will decrease over time.

The current heuristics should work on any config (including an OCIO v1 config) that was 
generated by editing one of the ACES configs or any config that uses one of the following 
as its reference space:

* ACES2065-1
* ACEScg
* Scene-linear Rec.709 (sRGB)
* Scene-linear Rec.2020
* Scene-linear P3-D65

And that has a color space either for any of the above spaces or for an sRGB texture space 
that has "sRGB" (case-insensitive) in its color space name or one of its aliases.

Note that the heuristics create a Processor and evaluate color values that must match 
within a certain tolerance.  No color space is selected purely based on its name alone.  
If the heuristics fail to find a recognized color space, an exception is thrown.


Making the interchange roles required for config versions 2.2 or higher
***********************************************************************

For Users
+++++++++

Users were surveyed during the OCIO 2.2 development process as to whether the Interchange 
Roles should become mandatory.  The response was overwhelmingly in favor of doing this, 
largely because it allows robust interchange of color spaces between configs or to 
external known standard color spaces.

Therefore, as described in the previous section, for config files of version 2.2 or 
higher, it is mandatory to define the ``aces_interchange`` role.  If the config includes 
display color spaces, the ``cie_xyz_d65_interchange`` role is also required.  

Note that the ``cie_xyz_d65_interchange`` is only used in connection with display color 
spaces (that is, with the display-referred connection space).  It is not used for 
scene-referred color spaces, and indeed it is an error if a scene-referred space is 
assigned to that role.

The ``ociocheck`` command-line tool has been updated to make these checks.  In addition, 
its reporting on other roles has been modified to be more lenient regarding roles which 
are no longer considered essential.

For Developers
++++++++++++++

The Config::validate method will log an error if the Config object does not meet these 
requirements.  Note that an exception is not thrown since it was felt that the Config's 
``upgradeToLatestVersion`` method must always produce a valid config.


Determining if a Color Space is Linear
**************************************

There have been many requests from developers that would like a standard way to determine 
if a color space is linear, since this impacts what sort of processing is suitable.  OCIO 
v2 introduced a new ``encoding`` attribute for color spaces which contains this 
information.  However, this is optional and may not be set by all config authors.  And it 
won't be present in OCIO v1 configs, which are still widely used.

For Developers
++++++++++++++

OCIO 2.2 adds a new ``isColorSpaceLinear`` method to the Config class which may be used 
for this purpose.  

Note that since OCIO has both a scene-referred and a display-referred reference space, the 
method also takes a ReferenceSpaceType enum to indicate which reference space the 
linearity determination is with respect to.  Typically developers will set this to 
``REFERENCE_SPACE_SCENE``.

The following algorithm is used to make the determination:

* If the color space ``isdata`` attribute is true, return false.
* If the reference space type of the color space differs from the requested reference 
  space type, return false.
* If the color space's encoding attribute is present, return true if it matches the 
  expected reference space type (i.e., "scene-linear" for ``REFERENCE_SPACE_SCENE`` or 
  "display-linear" for ``REFERENCE_SPACE_DISPLAY``) and false otherwise.
* If the color space has no ``to_reference`` or ``from_reference`` transform, return true.
* Evaluate several points through the color space's transform and check if the output only 
  differs by a scale factor (which may be different per channel, e.g. allowing an arbitrary 
  matrix transform, with no offset).

Note that the last step is a heuristic that may or may not be accurate.  However, note 
that the ``encoding`` attribute takes precedence and so config authors have the ultimate 
control over the linearity determination.


Getting a Processor for a NamedTransform
****************************************

For Developers
++++++++++++++

A new config object was introduced in OCIO v2 called Named Transforms.  These are used 
when there is a need to apply a mathematical function which is not a conversion between 
two specific color spaces.  The most common example is applying a transfer function curve 
to convert linear data to non-linear, or vice-versa.

The new ACES configs include Named Transforms, so it is important for application 
developers to start supporting this type of config object.  The preferred method for doing 
so is to add a new tool, similar to FileTransform that applies a Named Transform.  

What is new in OCIO 2.2 is that the code for applying these is now simpler with the 
introduction of several new getProcessor methods on the Config class that will return a 
Processor directly from a NamedTransform object.  

In addition, the NamedTransform class has a GetTransform method that returns a (regular) 
Transform object for a given direction.  It will create the transform from the inverse 
direction if the transform for the requested direction is missing.


Circular OCIO / OIIO Build Dependency Solution
**********************************************

A long-standing complaint has been regarding the circular build dependency between OCIO 
and OpenImageIO.  This is due to the fact that OIIO wants to use OCIO for color management 
and OCIO wants to use OIIO in its command-line tools ``ocioconvert``, ``ociolutimage``, 
and ``ociodisplay`` for reading and writing image files.  These tools will not be built if
OIIO is not available when configuring the build.

Furthermore, some package installers will not install these command-line tools due to the
dependency on OIIO.

By default, OCIO will now build these tools with OpenEXR rather than relying on OIIO.

For Users
+++++++++

If you have a version of OCIO that was not compiled with tools such as ``ocioconvert`` and 
you want to use OCIO to process images, you could try using OpenImageIO's ``oiiotool``.  
(Although note that ``ocioconvert`` has a few features that are not in ``oiiotool``, such 
as GPU processing support.)  Similarly if you have ``ocioconvert``, but it is compiled 
with OpenEXR rather than OpenImageIO, you may use ``oiiotool`` to convert other image file 
formats to/from OpenEXR.

If you want to use ``oiiotool`` but it does not support a particular type of conversion, 
you may be able to use ``ociowrite`` to export a CTF file and then use that with the 
``--ociofiletransform`` option in ``oiiotool``.

For Developers
++++++++++++++

In OCIO 2.2, by default, the build will now use OpenEXR rather than OpenImageIO for the 
command-line tools that read or write images.  This will limit the functionality of the 
aforementioned command-line tools to only working with OpenEXR files.  If you want support 
for more file formats in these tools, you will still need to have OIIO available when 
building OCIO and set the CMake variable ``-D OCIO_USE_OIIO_FOR_APPS=ON``.


Miscellaneous Improvements
**************************

Here are some other improvements in OCIO 2.2:

* Support for more types of ICC Monitor Profiles -- All of the parametric curve types are 
  now supported.

* New hash function for calculating cache IDs -- The md5 algorithm has been replaced with 
  xxhash, which provides a considerable speed-up for various operations.  The APIs that 
  return cache ID strings will obviously return different strings now, but please note that 
  these are not guaranteed to be unchanged across releases.  (The 128-bit version of xxhash 
  was used, which is the same length as for md5.)

* The command-line tools ``ocioconvert``, ``ociowrite``, and ``ocioperf`` now support 
  using an inverse DisplayViewTransform.

* Add DisplayViewTransform and NamedTransform support to Baker.

* Several new Built-in Transforms are available for version 2.2 config files, including 
  ARRI LogC4.

* Preliminary support for ACES Metadata File (AMF) -- A prototype Python tool has been 
  added named ``pyocioamf`` that converts an AMF file into the OCIO native transform format 
  CTF. It uses a prototype ACES Reference config file that is serving as a database of ACES 
  Transform IDs for interpreting the AMF file. 

* Support for PyPI installation from source rather than pre-built binaries.


Release Notes
=============

For more detail, please see the GitHub release pages:

`OCIO 2.2.0 <https://github.com/AcademySoftwareFoundation/OpenColorIO/releases/tag/v2.2.0>`_

`OCIO 2.2.1 <https://github.com/AcademySoftwareFoundation/OpenColorIO/releases/tag/v2.2.1>`_

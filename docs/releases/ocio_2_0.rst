..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.


OCIO 2.0 Release
================

OpenColorIO v2 is a major update, three years in development, and contains a
large number of new features.  This section describes the new features and 
their relevance for both config authors and application developers.

Timeline
********

OpenColorIO v2 was proposed to the community at SIGGRAPH 2017 and reached 
"feature complete" at SIGGRAPH 2020.  The official 2.0.0 release was made in 
January 2021 after a period of stabilization and refinement.  OCIO v2 is in 
the VFX Reference Platform for calendar year 2021.

Demo Config
***********
There is a config file that illustrates many of the new OCIO v2 features
described below.  This is available in the configs section of the documentation
as :ref:`ocio-v2_demo`.


New Feature Guide
=================

New GPU Renderer
****************

The GPU renderer in OCIO v1 baked the operators into a form that was easier to
use on the GPUs of the day.  It did not fully match the CPU renderer and was not
suitable for rendering final frames.  The GPU renderer has been completely 
replaced for OCIO v2 and now computes pixels using the same operators as the CPU.

**Config authors**: You should experience a faithful match between CPU and GPU
results.

**Developers**: The GPU API needed to be completely redone in order to support
uploading the necessary textures to the GPU.  The new API is more powerful and
allows more options for customizing the GPU renderer to meet the needs of a 
wider variety of applications.


Enhanced CPU Renderer
*********************

The CPU renderer has been improved and uses more vectorized (SSE) instructions, support
for integer pixel buffers, and improved optimization.

**Config authors**: You should notice improved performance (up to 20x) in some cases.

**Developers**: A compliation setting allows you to turn on/off SSE usage.  There is
a new command-line tool ``ocioperf`` to test performance on a given system with 
a given color space conversion.


Support for Integer Pixel Buffers
*********************************

OCIO v1 only supported floating-point pixel buffers and only supported in-place
processing.  OCIO v2 supports integer pixels and does not require in-place
processing.  There is also support for a variety of pixel layouts (e.g. RGB and 
BGR). Using the integer buffers where possible not only allows optimization of
memory traffic, it also allows OCIO to do additional optimizations.

**Config authors**: No impact.

**Developers**: Update your apply calls to use integer buffers where possible.  This 
is one of the biggest performance wins for CPU processing.


Improved Optimization
*********************

OCIO v2 is able to do a lot more optimization of a Processor in preparation for
the pixel apply call.  There is also now an environment variable that may be used
to provide very fine-grained control over the optimization settings.

**Config authors**: You may now override optimization settings to troubleshoot, if 
needed via the ``OCIO_OPTIMIZATION_FLAGS`` environment variable.

**Developers**: There is now an extra step between getting a Processor and calling 
apply().  You need to get a CPUProcessor or GPUProcessor and this is where you
may specify custom optimization settings.  Note that the environment variable
allows users to override these settings.


Inversion of all Operators
**************************

In OCIO v1, 3D-LUTs could not be inverted and this caused some conversions to
fail.  In OCIO v2, all transforms may be inverted.

**Config authors**: If you were building separate 3D-LUTs for both the from- and 
to_reference directions, you may not need to do that.  However, if you are 
applying sophisticated gamut mapping, the separate transform is probably still
worth making.

**Developers**: You don't need to worry about some color conversions failing.


New Operators to Support ACES
*****************************

Users have sometimes run into issues with the ACES config when processing 
extremely bright values and finding the results don't match the official
Academy CTL results.  In OCIO v2, new operators have been added to facilitate
an exact match to ACES Output Transforms.  Note that the new v2 ACES OCIO Config
is still under development.

**Config authors**: If you use ACES Output Transforms, please keep an eye out for
the v2 ACES Config that will leverage built-in ACES transforms.

**Developers**: The upcoming v2 ACES Config will allow your application to have
very accurate ACES performance on both CPU and GPU.


Built-in Color Transforms
*************************

OCIO v2 has the ability to generate a set of commonly used transforms
on demand in memory, avoiding the need for external LUT files.  This means
that some configs may be built with no need for any external files.
The current set of BuiltInTransforms includes the ACES Color Space Conversion
transforms as well as the components needed for the ACES Output Transforms.
(In OCIO v2, the ACES Output Transforms are factored into a View Transform
and a Display Color Space.)

**Config authors**: You will be able to simply use a built-in transform rather
than creating your own versions of common color spaces.

**Developers**: It is possible to create configs that don't require external
LUT files and are thus more robust (e.g. render farms, cloud processing).


Support for Academy/ASC CLF
***************************

OCIO v2 has a very thorough implementation of the Common LUT Format (CLF) 
released with ACES 1.2.  This LUT format offers many advantages over
existing formats.  A new command-line tool ``ociomakeclf`` will convert any
of the LUT formats supported by OCIO into CLF format.  Also, it is able to
insert conversions from and to ACES2065-1 before and after the LUT in order
to make an ACES-compliant LMT that may be used in an ACES Metadata File.

**Config authors**: The features of CLF may allow you to write smaller and/or
more accurate LUT files for your configs.  If you were saving transform
components in separate files, you may combine them into a single file, if
desired.

**Developers**: You may now add CLF support to your application.  Note that this
is necessary in order to obtain ACES Logo Certification.  Note that CLF has
much better metadata support compared to previous LUT formats and so apps
should expose these fields to users.


Serialization of all OCIO transforms as CTF
*******************************************

The Autodesk Color Transform Format (CTF) is able to serialize all OCIO
transforms into an XML format that is a superset of Academy/ASC CLF.
This is very useful for troubleshooting.  It also opens up new workflow
possibilities.  The new ``ociowrite`` command-line tool will serialize
an OCIO Processor object to a CTF file.  Similarly there is a write method
on the GroupTransform class.

**Config authors**: You may use CTF to store a chain of arbitrary OCIO transforms
to an XML file for use in a config or to send as a self-contained file.

**Developers**: A given OCIO Processor may be easily serialized and restored.


Access to all transforms from the public API
********************************************

The public API now allows access to everything OCIO is able to load/read,
including LUT entries.  Also, an OCIO Processor may now be converted into
a GroupTransform.  This opens up new ways of using OCIO.

**Config authors**: No impact.

**Developers**: Even if you have your own color rendering engine, you may now
use OCIO to read and write the many LUT formats it supports, including the
Common LUT Format (CLF).


Display-referred Connection Space
*********************************

There is now a second reference space in OCIO.  The original reference space
is typically a scene-referred color space and the new space is intended to be 
for a display-referred color space.  This means that the conversion from a
scene-referred space to a display space may be broken down into a view transform
plus a display color space.  There are new config sections for ``view_transforms``
and ``display_colorspaces``.

**Config authors**: Break down your Views into a view transform and display
color space.  Having a separate display color space faciitates direct conversion
from one display to another without needing to convert back to the scene-referred
reference space.

**Developers**: No impact.  The new display color spaces will appear in menus
with other color spaces by default.


Shared Views
************

It is now possible to define a View and reuse it for multiple displays.  There is
a new ``shared_views`` section of the config file.

**Config authors**: Make your configs easier to read and maintain by using
shared views.

**Developers**: No impact.


Support for ICC Monitor Profiles
********************************

OCIO v2 is able to read basic ICC monitor profiles.  Also a new virtual display
object in the config allows a config author to define how OCIO may instantiate
a new display and views from a user's ICC monitor profile.

**Config authors**: Add a ``virtual_display`` to your config to enable a user to
use the ICC profile for their monitor.  Note that if you do not add the virtual
display, applications will not be able to instantiate an ICC profile with your config.

**Developers**: There is new SystemMonitor code to use to ask OCIO to instantiate 
a new display and views from an ICC profile.  This is also able to query the operating
system on Mac and Windows to obtain the user's current profile.


A Categories Attribute for Color Spaces
***************************************

A new attribute called ``categories`` has been added to color spaces.  The goal is
to allow applications to filter the complete list of color spaces down to only
show users the ones needed for the task at hand.  For example, when choosing a
working space, it may not be ideal to show all the color spaces in the config.
The list of color spaces is filtered by both a "user-defined set" and an
"application-defined" set of categories.  For example, the user-defined set
could include categories for which department (lighting, comp, etc.) and the
artist experience level (e.g. basic or advanced).  The application-defined set
could include categories for different types of menus, for example working
spaces vs. file IO.

**Config authors**: Add the categories attribute to help applications shorten 
their menus to only include the appropriate color spaces for various tasks.
Configure your app configuration scripts to set the ``OCIO_USER_CATEGORIES`` env
var with the appropriate categories.

**Developers**: Use the Menu Helpers classes to build your application color 
space menus to take advantage of this feature.  Publish the category string
you use for each of your color space menus so config authors may use them in
their configs.


Encoding Attribute
******************

There is a new attribute called ``encoding`` that may be used to indicate the
type of encoding used for a color space.  The encoding options are ``scene-linear``,
``display-linear``, ``log``, ``sdr-video``, ``hdr-video``, and ``data``. This is useful
to applications since image processing algorithms often need to know the encoding
for optimium results.  The encoding may also be used in the viewing rules to
filter views based on the color space.  The encoding attribute may be used
in combination with the categories.  For example, an application may ask for
the list of all color spaces with a category "working-space" and an encoding
"scene-linear".

**Config authors**: Set the encoding attribute on your color spaces to help
applications know how to process images in that space better.  Also, use the
encoding in viewing rules to allow applications to filter the views to be
appropriate for a given color space.

**Developers**: Knowing the encoding of a color space may allow you to 
optimize your image processing algorithms.


Inactive color space list
*************************

There is now an ``inactive_colorspaces`` list in the config and a corresponding
environment variable.  This allows config authors to keep color spaces in a
config but prevent them from appearing in application menus.

**Config authors**: This allows you to remove color spaces you don't want
users to have access to.  This may also be set via the ``OCIO_INACTIVE_COLORSPACES``
environment variable.

**Developers**: These color spaces will not show up in the normal list of
color spaces, however you may still use them as arguments to getProcessor.
(For example if your application has assets that use an earlier version
of the config where those spaces were active.)  The Menu Helpers classes 
show how to deal with temporarily adding an inactive color space to menus 
when it is necessary.


Color space Aliases
*******************

There is a new ``aliases`` attribute that allows the config author to define
a set of alternate names for a color space.  For example, the official name
may be a user-friendly name for the UI and a shorter name that is intended
for embedding in file paths may be declared as an alias. 

**Config authors**: This avoids the need to add duplicates of color spaces 
simply to have an alternate name.  It may also be used to handle deprecated
color space names.

**Developers**: There is also a new getCanonicalName method on the Config 
that may be used to resolve aliases to the official name.  Before saving a
color space name, it may be desirable to convert to the canonical name.


Hierarchical menus
******************

The config has a new family_separator attribute that specifies a character to
be used in the ``family`` attribute to break strings down into a hierarchy.
The Menu Helpers is able to generate hierarchical menus based on this.

**Config authors**: Use the family attribute to help applications organize
long color space lists better.

**Developers**: Use the Menu Helpers classes to build your application color 
space menus to take advantage of this feature.


Color Picker Helper
*******************

There are Mixing Helpers classes that show how to implement a color picker
that works well with scene-linear data.  This facilitates making UI sliders
for linear values and also doing sensible RGB to HSV conversions with float
values outside [0,1].

**Config authors**: No impact.

**Developers**: This simplifies making scene-linear friendly color pickers.


File Rules
**********

The File Rules allow a config to specify how to assign a default color space
to a file based on the path using glob or regex pattern matching.  This opens
up new workflows since it is no longer necessary to embed a color space name
into the path.

**Config authors**: You may not need to embed a color space name into your
paths anymore.  You may be able to rely on better default file handling among
various applications.

**Developers**: Implement support for the new file rules.  Also, if your
application honored "strictparsing: true" mode in OCIO v1, the code for doing
this has changed in v2.  It is now always possible to obtain from OCIO a valid
default color space for a file.


Viewing Rules
*************

The Viewing Rules allows a config author to specify which Views in a display
are appropriate for a given color space.  It also makes it possible to have
the default view be a function of the color space.

**Config authors**: Set the viewing rules to enable friendlier application
behavior.

**Developers**: When asking for the list of views for a display, use the new
API that allows passing in the color space being viewed.  The first view in
the list is the most appropriate one for that color space.  This is useful
as the default view for the first time an image or asset is viewed or for
generating sensible proxy or thumbnail images.


Dynamic Properties
******************

Certain transforms now support dynamic properties which are parameters that
may be adjusted even after a transform has been converted into a Processor.
This is useful especially when users are making live updates, for example
when adjusting the exposure or gamma of an image in a viewport.  On the GPU,
these are mapped to uniforms.

**Config authors**: No impact.  Dynamic properties are not exposed in configs,
which are locked representations of a color pipeline.

**Developers**: Expose dynamic properties in situations where the user is
creating a transform with editable values or when editing exposure or gamma
controls on a viewing pipeline for improved performance.  See the 
DisplayViewHelpers application helpers code or the unit tests for examples.
Note however that transforms included in a config should be considered 
locked.


New Transforms for Building Looks
*********************************

ASC CDL transforms are easy to edit but are not very powerful, whereas a
Lut3D is very powerful, but difficult to edit and understand what it does.
OCIO v2 introduces some new transforms that fall in a middle ground -- they
are more powerful than a CDL but are also parametrically adjustable and
easy to read.  The new transforms are for Primary adjustments, fine
adjustments to Tone reproduction, and spline-based RGB curves.  The new
transforms make use of dynamic properties to facilitate live interactive
adjustments on the CPU and GPU.

**Config authors**: You may find these new transforms useful when building
Look Transforms.  When used in a config, the parameters may be considered
locked (same as a CDL).

**Developers**: You may want to expose editing functionality for these
transforms and support their dynamic properties.  Note however that once
a transform is included in a config, it should be considered locked.


Providing an Interchange Mechanism Between Configs
**************************************************

In OCIO v1, there was no way to convert an image or asset in a color space
from one config into a color space from a different config.  This presented
a serious challenge for some workflows.  In OCIO v2 there are new APIs that
enable this conversion.  However, it requires the config author to implement
new roles called aces_interchange and cie_xyz_d65_interchange.

**Config authors**: Please implement these roles in your configs.

**Developers**: This feature may open up long awaited workflows for you.


Named Color Transforms
**********************

There is a new ``named_transforms`` section of the config that may be
used to define transforms that the config author wants to make available
but which are not actually a color space, look, or view.  For example, a
gamut mapping transform or a utility curve transform.

**Config authors**: These may provide a useful alternative to color spaces.

**Developers**: These new transforms do not appear in color space menus
by default.  So action is needed to make them available to users.  This
could be done by adding a new Named Transform tool alongside an existing
Color Space Transform tool, for example.


Processor Caching
*****************

In OCIO v2 there is now a cache for Processors.  The caching system is aware of
context variables and will detect if a variable has been changed.

**Config authors**: No impact.

**Developers**: This may facilitate various options such as realtime playback
of timelines that leverage context variables.


New Description and Name Attributes
***********************************

A new ``name`` attribute has been added to many of the OCIO transforms to provide
some additional labeling options in a config file.  The ``description`` attribute
has been added to Views to allow similar description strings as are used in color
spaces.  There is also a ``name`` attribute at the top level of the config itself.

**Config authors**: These may prove useful.  Consider updating the name attribute of
the config as the config file is versioned.

**Developers**: Consider exposing the description string for both color spaces
and views.



Changes from v1
===============

DisplayTransform
****************

The decision was made to refactor DisplayTransform to make it easier to use
and easier to invert.  The functionality of the DisplayTransform is now in
the LegacyViewingPipeline class in src/OpenColorIO/apphelpers.  The original
DisplayTransform class has been removed.  There is a new DisplayViewTransform
available that now supports inversion.

**Config authors**: No impact.

**Developers**: The DisplayViewTransform, along with ColorSpaceTransform are
the two key pieces of functionality to expose to users.  If you were using
the original DisplayTransform, update to LegacyViewingPipeline for viewports.
But you should also consider exposing DisplayViewTransform to users as a
tool for baking in (or inverting) a display + view.


Clamping
********

In OCIO v1, the exponent transform was used to implement the ASC CDL and it
had unusual clamping behavior where it would clamp negative values *except*
if the power was 1. The decision was made to add new transforms that provide
more clamping options.  A style attribute has been added to the ASC CDL and
Exponent Transforms that allow a variety of negative-handling options to be
selected.  For v1 configs, the original exponent behavior is used, but in v2
configs, the new operators are used.  Also, in OCIO v1, the optimization 
process sometimes changed whether a transformation clamped or not.  In v2,
the optimized transforms more closely follow the clamping behavior of the
original.

**Config authors**: Verify the clamping behavior in your configs and adjust the
style arguments as desired when upgrading configs to v2.

**Developers**: There should be fewer differences in behavior due to changes in
clamping based on parameter value changes or optimization changes.


Strict Parsing Mode
*******************

The code for implementing "strictparsing: true" mode has changed from v1.

**Config authors**: If you use this mode, verify that your applications
support it as you expect.

**Developers**: See the File Rules API for more info.


Default Role
************

In OCIO v1, the default role was sometimes used as a fallback in general cases
where a color space could not be found.  This is no longer the case.

**Config authors**: If you relied on this behavior, please verify your configs.
Note that this behavior may have hid errors that existed in your configs.

**Developers**: No impact.


Context Variable Changes
************************

In a v2 config, it is now illegal to use the context variable tokens '$' and '%'
in color space names (in other words, it is illegal to use them if they are not
actually context variables).

**Config authors**: Please do not use these characters except for context variables.

**Developers**: No impact.


Allocation Variables
********************

The color space allocation variables are not used by the new GPU renderer.
However, they are still used by the ociobakelut utility and if an application
requests the legacy GPU shader.

**Config authors**: If you don't care about baking and you are using applications
that use the new GPU renderer, you don't need to set the allocation variables on
color spaces anymore.

**Developers**: Update your applications to use the new GPU renderer.


Nearest Interpolation Method
****************************

OCIO v1 implemented a "nearest neighbor" intepolation method for Lut1D and Lut3D
which is faster than the default linear interpolation but not as accurate.  Since
processing of integer pixel buffers was not supported in v1, the "nearest" method
may have been used by some to speed up applying a Lut1D to integer source material.
OCIO v2 does not implement a separate "nearest" method, and FileTransforms that
request this style will receive "linear" instead.  However, OCIO v2 does support
processing of integer pixel buffers directly and this provides a big speed-up
for this use-case without requiring "nearest" mode (i.e., a look-up is done
without interpolation for Lut1D ops when the inputs are integers).

**Config authors**: Please be aware of this change.

**Developers**: To benefit from this performance improvement, please call OCIO 
directly on integer pixel buffers rather than converting them to float buffers first.


Color Space Conversion No-ops
*****************************

In OCIO v1, a ColorSpaceTransform from color space A to color space A (i.e.,
using the same color space for source and destination) was not necessarily a
no-op.  In OCIO v2, this is a no-op.  In other words, all color spaces may
be considered to be in an equality group with themselves.

**Config authors**: Please be aware of this change.

**Developers**: No impact.


CDLTransform XML read/write
***************************

In OCIO v1, the CDLTransform had getXML and setXML methods.  In OCIO v2, the
read/write/metadata handling has been refactored to be more like other formats.
CreateFromFile is the replacement for setXML and the write method on the
GroupTransform class is the replacement for getXML.  The FormatMetadata class
provides extensive metadata access.

**Config authors**: No impact.

**Developers**: Please update your code to use the new methods.


Version Handling
****************

In OCIO v1, the YAML parser would warn if the config version was higher than
the library version but continue to try to read it.  In OCIO v2, if the config
version is higher than the library it will immediately throw an exception.

**Config authors**: Please be aware of the change.

**Developers**: No impact.


Bypassing Data Spaces
*********************

In OCIO v1, if a color space set its isdata attribute to true, a ColorSpaceTransform
would be a no-op.  The DisplayTransform also bypassed most of its processing.
In OCIO v2, data spaces are still bypassed by default.  However, the application 
developer may now use a new argument to specify that data spaces should be
processed.  

**Config authors**: Please be aware of the change.

**Developers**: This may be useful, for example, when displaying data color spaces on
an HDR monitor.


Release Notes
=============

For more detail, please see the GitHub release pages:

`OCIO 2.0.0 <https://github.com/AcademySoftwareFoundation/OpenColorIO/releases/tag/v2.0.0>`_

`OCIO 2.0.1 <https://github.com/AcademySoftwareFoundation/OpenColorIO/releases/tag/v2.0.1>`_

`OCIO 2.0.2 <https://github.com/AcademySoftwareFoundation/OpenColorIO/releases/tag/v2.0.2>`_

`OCIO 2.0.3 <https://github.com/AcademySoftwareFoundation/OpenColorIO/releases/tag/v2.0.3>`_

`OCIO 2.0.4 <https://github.com/AcademySoftwareFoundation/OpenColorIO/releases/tag/v2.0.4>`_

`OCIO 2.0.5 <https://github.com/AcademySoftwareFoundation/OpenColorIO/releases/tag/v2.0.5>`_

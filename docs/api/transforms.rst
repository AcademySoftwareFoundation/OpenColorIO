..
  SPDX-License-Identifier: CC-BY-4.0
  Copyright Contributors to the OpenColorIO Project.

Transforms
**********

.. CAUTION::
   API Docs are a work in progress, expect them to improve over time.


FormatMetadata
==============

**class FormatMetadata**

The FormatMetadata class is intended to be a generic container to
hold metadata from various file formats.

This class provides a hierarchical metadata container. A metadata
object is similar to an element in XML. It contains:

* A name string (e.g. “Description”).

* A value string (e.g. “updated viewing LUT”).

* A list of attributes (name, value) string pairs (e.g. “version”, “1.5”).

* And a list of child sub-elements, which are also objects implementing FormatMetadata.


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: const char *getName() const = 0

      .. cpp:function:: void setName(const char*) = 0

      .. cpp:function:: const char *getValue() const = 0

      .. cpp:function:: void setValue(const char*) = 0

      .. cpp:function:: int getNumAttributes() const = 0

      .. cpp:function:: const char *getAttributeName(int i) const = 0

      .. cpp:function:: const char *getAttributeValue(int i) const = 0

      .. cpp:function:: void addAttribute(const char *name, const char *value) = 0

         Add an attribute with a given name and value. If an attribute
         with the same name already exists, the value is replaced.

      .. cpp:function:: int getNumChildrenElements() const = 0

      .. cpp:function:: const FormatMetadata &getChildElement(int i) const = 0

      .. cpp:function:: FormatMetadata &getChildElement(int i) = 0

      .. cpp:function:: FormatMetadata &addChildElement(const char *name, const char *value) = 0

         Add a child element with a given name and value. Name has to be
         non-empty. Value may be empty, particularly if this element will
         have children. Return a reference to the added element.

      .. cpp:function:: void clear() = 0

      .. cpp:function:: FormatMetadata &operator=(const FormatMetadata &rhs) = 0

      .. cpp:function:: FormatMetadata(const FormatMetadata &rhs) = delete

      .. cpp:function:: ~FormatMetadata()


   .. group-tab:: Python

      .. py:class:: AttributeIterator

      .. py:class:: AttributeNameIterator

      .. py:class:: ChildElementIterator

      .. py:class:: ConstChildElementIterator

      .. py:method:: addChildElement(self: PyOpenColorIO.FormatMetadata, name: str, value: str) -> `PyOpenColorIO.FormatMetadata`_

      .. py:method:: clear(self: PyOpenColorIO.FormatMetadata) -> None

      .. py:method:: getAttributes(self: PyOpenColorIO.FormatMetadata) -> OpenColorIO_v2_0dev::PyIterator<OpenColorIO_v2_0dev::FormatMetadata const&, 1>

      .. py:method:: getChildElements(*args, **kwargs)

         Overloaded function.

         1. .. py:method:: getChildElements(self: PyOpenColorIO.FormatMetadata) -> OpenColorIO_v2_0dev::PyIterator<OpenColorIO_v2_0dev::FormatMetadata const&, 2>

         2. .. py:method:: getChildElements(self: PyOpenColorIO.FormatMetadata) -> OpenColorIO_v2_0dev::PyIterator<OpenColorIO_v2_0dev::FormatMetadata&, 3>

      .. py:method:: getName(self: PyOpenColorIO.FormatMetadata) -> str

      .. py:method:: getValue(self: PyOpenColorIO.FormatMetadata) -> str

      .. py:method:: setName(self: PyOpenColorIO.FormatMetadata, name: str) -> None

      .. py:method:: setValue(self: PyOpenColorIO.FormatMetadata, value: str) -> None


Transform
=========

**class Transform**

Base class for all the transform classes.

Subclassed by OpenColorIO::AllocationTransform,
OpenColorIO::BuiltinTransform, OpenColorIO::CDLTransform,
OpenColorIO::ColorSpaceTransform,
OpenColorIO::DisplayViewTransform, OpenColorIO::ExponentTransform,
OpenColorIO::ExponentWithLinearTransform,
OpenColorIO::ExposureContrastTransform, OpenColorIO::FileTransform,
OpenColorIO::FixedFunctionTransform, OpenColorIO::GroupTransform,
OpenColorIO::LogAffineTransform, OpenColorIO::LogCameraTransform,
OpenColorIO::LogTransform, OpenColorIO::LookTransform,
OpenColorIO::Lut1DTransform, OpenColorIO::Lut3DTransform,
OpenColorIO::MatrixTransform, OpenColorIO::RangeTransform


C++
---

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: TransformRcPtr createEditableCopy() const = 0

      .. cpp:function:: TransformDirection getDirection() const noexcept = 0

      .. cpp:function:: void setDirection(TransformDirection dir) noexcept = 0

         Note that this only affects the evaluation and not the values
         stored in the object.

      .. cpp:function:: void validate() const

         Will throw if data is not valid.

      .. cpp:function:: Transform(const Transform&) = delete

      .. cpp:function:: Transform &operator=(const Transform&) = delete

      .. cpp:function:: ~Transform() = default


   .. group-tab:: Python

      .. py:method:: getDirection(self: PyOpenColorIO.Transform) -> PyOpenColorIO.TransformDirection

      .. py:method:: setDirection(self: PyOpenColorIO.Transform, direction: PyOpenColorIO.TransformDirection) -> None

      .. py:method:: validate(self: PyOpenColorIO.Transform) -> None


AllocationTransform
===================

**class AllocationTransform : public OpenColorIO::Transform**

Forward direction wraps the ‘expanded’ range into the specified,
often compressed, range.


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: TransformRcPtr createEditableCopy() const override

      .. cpp:function:: TransformDirection getDirection() const noexcept override

      .. cpp:function:: void setDirection(TransformDirection dir) noexcept override

      Note that this only affects the evaluation and not the values
      stored in the object.

      .. cpp:function:: void validate() const override

      Will throw if data is not valid.

      .. cpp:function:: Allocation getAllocation() const

      .. cpp:function:: void setAllocation(Allocation allocation)

      .. cpp:function:: int getNumVars() const

      .. cpp:function:: void getVars(float *vars) const

      .. cpp:function:: void setVars(int numvars, const float *vars)

      .. cpp:function:: `AllocationTransform`_ &operator=(const AllocationTransform&) = delete

      .. cpp:function:: ~AllocationTransform()

      -[ Public Static Functions ]-

      .. cpp:function:: AllocationTransformRcPtr Create()


   .. group-tab:: Python

      .. py:method:: getAllocation(self: PyOpenColorIO.AllocationTransform) -> PyOpenColorIO.Allocation

      .. py:method:: getVars(self: PyOpenColorIO.AllocationTransform) -> List[float]

      .. py:method:: setAllocation(self: PyOpenColorIO.AllocationTransform, allocation: PyOpenColorIO.Allocation) -> None

      .. py:method:: setVars(self: PyOpenColorIO.AllocationTransform, vars: List[float]) -> None


BuiltinTransform
================

**class BuiltinTransform : public OpenColorIO::Transform**

A built-in transform is similar to a FileTransform, but without the
file. OCIO knows how to build a set of commonly used transforms
on-demand, thus avoiding the need for external files and
simplifying config authoring.


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: const char *getStyle() const noexcept = 0

      .. cpp:function:: void setStyle(const char *style) = 0

         Select an existing built-in transform style from the list
         accessible through :cpp:class:``BuiltinTransformRegistry``. The
         style is the ID string that identifies which transform to apply.

      .. cpp:function:: const char *getDescription() const noexcept = 0

      .. cpp:function:: ~BuiltinTransform() = default

      -[ Public Static Functions ]-

      .. cpp:function:: BuiltinTransformRcPtr Create()

   .. group-tab:: Python

      .. py:method:: getDescription(self: PyOpenColorIO.BuiltinTransform) -> str

      .. py:method:: getStyle(self: PyOpenColorIO.BuiltinTransform) -> str

      .. py:method:: setStyle(self: PyOpenColorIO.BuiltinTransform, style: str) -> None


CDLTransform
============

**class CDLTransform : public OpenColorIO::Transform**

An implementation of the ASC Color Decision List (CDL), based on
the ASC v1.2 specification.

**Note**
​ If the config version is 1, negative values are clamped if the
power is not 1.0. For config version 2 and higher, the negative
handling is controlled by the CDL style.


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: FormatMetadata &getFormatMetadata() noexcept = 0

      .. cpp:function:: const FormatMetadata &getFormatMetadata() const noexcept = 0

      .. cpp:function:: bool equals(const CDLTransform &other) const noexcept = 0

      .. cpp:function:: CDLStyle getStyle() const = 0

      .. cpp:function:: void setStyle(CDLStyle style) = 0

         Use CDL_ASC to clamp values to [0,1] per the ASC spec. Use
         NO_CLAMP to never clamp values (regardless of whether power is
         1.0). The NO_CLAMP option passes negatives through unchanged
         (like the NEGATIVE_PASS_THRU style of ExponentTransform). The
         default style is CDL_NO_CLAMP.

      .. cpp:function:: const char *getXML() const = 0

      .. cpp:function:: void setXML(const char *xml) = 0

         The default style is CDL_NO_CLAMP.

      .. cpp:function:: void getSlope(double *rgb) const = 0

      .. cpp:function:: void setSlope(const double *rgb) = 0

      .. cpp:function:: void getOffset(double *rgb) const = 0

      .. cpp:function:: void setOffset(const double *rgb) = 0

      .. cpp:function:: void getPower(double *rgb) const = 0

      .. cpp:function:: void setPower(const double *rgb) = 0

      .. cpp:function:: void getSOP(double *vec9) const = 0

      .. cpp:function:: void setSOP(const double *vec9) = 0

      .. cpp:function:: double getSat() const = 0

      .. cpp:function:: void setSat(double sat) = 0

      .. cpp:function:: void getSatLumaCoefs(double *rgb) const = 0

         These are hard-coded, by spec, to r709.

      .. cpp:function:: const char *getID() const = 0

         Unique Identifier for this correction.

      .. cpp:function:: void setID(const char *id) = 0

      .. cpp:function:: const char *getDescription() const = 0

         Deprecated. Use ``getFormatMetadata``. First textual description
         of color correction (stored on the SOP). If there is already a
         description, the setter will replace it with the supplied text.

      .. cpp:function:: void setDescription(const char *desc) = 0

         Deprecated. Use ``getFormatMetadata``.

      .. cpp:function:: CDLTransform(const CDLTransform&) = delete

      .. cpp:function:: `CDLTransform`_ &operator=(const CDLTransform&) = delete

      .. cpp:function:: ~CDLTransform() = default

      -[ Public Static Functions ]-

      .. cpp:function:: CDLTransformRcPtr Create()

      .. cpp:function:: CDLTransformRcPtr CreateFromFile(const char *src, const char
      *cccid)

         Load the CDL from the src .cc or .ccc file. If a .ccc is used,
         the cccid must also be specified src must be an absolute path
         reference, no relative directory or envvar resolution is
         performed.


   .. group-tab:: Python

      .. py:method:: static CreateFromFile(src: str, id: str) -> `PyOpenColorIO.CDLTransform`_

      .. py:method:: equals(self: PyOpenColorIO.CDLTransform, other: PyOpenColorIO.CDLTransform) -> bool

      .. py:method:: getDescription(self: PyOpenColorIO.CDLTransform) -> str

      .. py:method:: getFormatMetadata(*args, **kwargs)

         Overloaded function.

         1. ..py:method:: getFormatMetadata(self: PyOpenColorIO.CDLTransform) -> PyOpenColorIO.FormatMetadata

         2. ..py:method:: getFormatMetadata(self: PyOpenColorIO.CDLTransform) -> PyOpenColorIO.FormatMetadata

      .. py:method:: getID(self: PyOpenColorIO.CDLTransform) -> str

      .. py:method:: getOffset(self: PyOpenColorIO.CDLTransform) -> List[float[3]]

      .. py:method:: getPower(self: PyOpenColorIO.CDLTransform) -> List[float[3]]

      .. py:method:: getSOP(self: PyOpenColorIO.CDLTransform) -> List[float[9]]

      .. py:method:: getSat(self: PyOpenColorIO.CDLTransform) -> float

      .. py:method:: getSatLumaCoefs(self: PyOpenColorIO.CDLTransform) -> List[float[3]]

      .. py:method:: getSlope(self: PyOpenColorIO.CDLTransform) -> List[float[3]]

      .. py:method:: getStyle(self: PyOpenColorIO.CDLTransform) -> PyOpenColorIO.CDLStyle

      .. py:method:: getXML(self: PyOpenColorIO.CDLTransform) -> str

      .. py:method:: setDescription(self: PyOpenColorIO.CDLTransform, description: str) -> None

      .. py:method:: setID(self: PyOpenColorIO.CDLTransform, id: str) -> None

      .. py:method:: setOffset(self: PyOpenColorIO.CDLTransform, rgb: List[float[3]]) -> None

      .. py:method:: setPower(self: PyOpenColorIO.CDLTransform, rgb: List[float[3]]) -> None

      .. py:method:: setSOP(self: PyOpenColorIO.CDLTransform, vec9: List[float[9]]) -> None

      .. py:method:: setSat(self: PyOpenColorIO.CDLTransform, sat: float) -> None

      .. py:method:: setSlope(self: PyOpenColorIO.CDLTransform, rgb: List[float[3]]) -> None

      .. py:method:: setStyle(*args, **kwargs)

         Overloaded function.

         1. ..py:method:: setStyle(self: PyOpenColorIO.CDLTransform, style: PyOpenColorIO.CDLStyle) -> None

         2. ..py:method:: setStyle(self: PyOpenColorIO.CDLTransform, style: PyOpenColorIO.CDLStyle) -> None

      .. py:method:: setXML(self: PyOpenColorIO.CDLTransform, xml: str) -> None


ColorSpaceTransform
===================

**class ColorSpaceTransform : public OpenColorIO::Transform**


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: TransformRcPtr createEditableCopy() const override

      .. cpp:function:: TransformDirection getDirection() const noexcept override

      .. cpp:function:: void setDirection(TransformDirection dir) noexcept override

         Note that this only affects the evaluation and not the values
         stored in the object.

      .. cpp:function:: void validate() const override

         Will throw if data is not valid.

      .. cpp:function:: const char *getSrc() const

      .. cpp:function:: void setSrc(const char *src)

      .. cpp:function:: const char *getDst() const

      .. cpp:function:: void setDst(const char *dst)

      .. cpp:function:: bool getDataBypass() const noexcept

         Data color spaces do not get processed when true (which is the
         default).

      .. cpp:function:: void setDataBypass(bool enabled) noexcept

      .. cpp:function:: `ColorSpaceTransform`_ &operator=(const ColorSpaceTransform&) = delete

      .. cpp:function:: ~ColorSpaceTransform()

      -[ Public Static Functions ]-

      .. cpp:function:: ColorSpaceTransformRcPtr Create()


   .. group-tab:: Python

      .. py:method:: getDataBypass(self: PyOpenColorIO.ColorSpaceTransform) -> bool

      .. py:method:: getDst(self: PyOpenColorIO.ColorSpaceTransform) -> str

      .. py:method:: getSrc(self: PyOpenColorIO.ColorSpaceTransform) -> str

      .. py:method:: setDataBypass(self: PyOpenColorIO.ColorSpaceTransform, dataBypass: bool) -> None

      .. py:method:: setDst(self: PyOpenColorIO.ColorSpaceTransform, dst: str) -> None

      .. py:method:: setSrc(self: PyOpenColorIO.ColorSpaceTransform, src: str) -> None


DisplayViewTransform
====================

**class DisplayViewTransform : public OpenColorIO::Transform**


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: TransformRcPtr createEditableCopy() const override

      .. cpp:function:: TransformDirection getDirection() const noexcept override

      .. cpp:function:: void setDirection(TransformDirection dir) noexcept override

         Note that this only affects the evaluation and not the values
         stored in the object.

      .. cpp:function:: void validate() const override

         Will throw if data is not valid.

      .. cpp:function:: const char *getSrc() const

      .. cpp:function:: void setSrc(const char *name)

         Specify the incoming color space.

      .. cpp:function:: const char *getDisplay() const

      .. cpp:function:: void setDisplay(const char *display)

         Specify which display to use.

      .. cpp:function:: const char *getView() const

      .. cpp:function:: void setView(const char *view)

         Specify which view transform to use.

      .. cpp:function:: bool getLooksBypass() const

      .. cpp:function:: void setLooksBypass(bool bypass)

         Looks will be bypassed when true (the default is false).

      .. cpp:function:: bool getDataBypass() const noexcept

      .. cpp:function:: void setDataBypass(bool bypass) noexcept

         Data color spaces do not get processed when true (which is the
         default).

      .. cpp:function:: ~DisplayViewTransform()

      -[ Public Static Functions ]-

      .. cpp:function:: DisplayViewTransformRcPtr Create()


   .. group-tab:: Python


      .. py:method:: getDataBypass(self: PyOpenColorIO.DisplayViewTransform) -> bool

      .. py:method:: getDisplay(self: PyOpenColorIO.DisplayViewTransform) -> str

      .. py:method:: getLooksBypass(self: PyOpenColorIO.DisplayViewTransform) -> bool

      .. py:method:: getSrc(self: PyOpenColorIO.DisplayViewTransform) -> str

      .. py:method:: getView(self: PyOpenColorIO.DisplayViewTransform) -> str

      .. py:method:: setDataBypass(self: PyOpenColorIO.DisplayViewTransform, dataBypass: bool) -> None

      .. py:method:: setDisplay(self: PyOpenColorIO.DisplayViewTransform, display: str) -> None

      .. py:method:: setLooksBypass(self: PyOpenColorIO.DisplayViewTransform, looksBypass: bool) -> None

      .. py:method:: setSrc(self: PyOpenColorIO.DisplayViewTransform, src: str) -> None

      .. py:method:: setView(self: PyOpenColorIO.DisplayViewTransform, view: str) -> None


DynamicProperty
===============

**class DynamicProperty**

Allows transform parameter values to be set on-the-fly (after
finalization). For example, to modify the exposure in a viewport.

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: DynamicPropertyType getType() const = 0

      .. cpp:function:: DynamicPropertyValueType getValueType() const = 0

      .. cpp:function:: double getDoubleValue() const = 0

      .. cpp:function:: void setValue(double value) = 0

      .. cpp:function:: bool isDynamic() const = 0

      .. cpp:function:: `DynamicProperty`_ &operator=(const DynamicProperty&) = delete

      .. cpp:function:: ~DynamicProperty()

   .. group-tab:: Python

      .. py:method:: getDoubleValue(self: PyOpenColorIO.DynamicProperty) -> float

      .. py:method:: getType(self: PyOpenColorIO.DynamicProperty) -> PyOpenColorIO.DynamicPropertyType

      .. py:method:: getValueType(self: PyOpenColorIO.DynamicProperty) -> PyOpenColorIO.DynamicPropertyValueType

      .. py:method:: isDynamic(self: PyOpenColorIO.DynamicProperty) -> bool

      .. py:method:: setValue(self: PyOpenColorIO.DynamicProperty, value: float) -> None


ExponentTransform
=================

**class ExponentTransform : public OpenColorIO::Transform**

Represents exponent transform: pow( clamp(color), value ).

​.. note:: For configs with version == 1: Negative style is ignored
and if the exponent is 1.0, this will not clamp. Otherwise, the
input color will be clamped between [0.0, inf]. For configs with
version > 1: Negative value handling may be specified via
setNegativeStyle.


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: const FormatMetadata &getFormatMetadata() const noexcept = 0
   
      .. cpp:function:: FormatMetadata &getFormatMetadata() noexcept = 0

      .. cpp:function:: bool equals(const ExponentTransform &other) const noexcept = 0

         Checks if this exactly equals other.

      .. cpp:function:: void getValue(double (&vec4)[4]) const noexcept = 0

      .. cpp:function:: void setValue(const double (&vec4)[4]) noexcept = 0

      .. cpp:function:: NegativeStyle getNegativeStyle() const = 0

         Specifies how negative values are handled. Legal values:

         * **NEGATIVE_CLAMP**  Clamp negative values (default).

         * **NEGATIVE_MIRROR**  Positive curve is rotated 180 degrees around
         the origin to handle negatives.

         * **NEGATIVE_PASS_THRU**  Negative values are passed through
         unchanged.

      .. cpp:function:: void setNegativeStyle(NegativeStyle style) = 0

      .. cpp:function:: ExponentTransform(const ExponentTransform&) = delete

      .. cpp:function:: `ExponentTransform`_ &operator=(const ExponentTransform&) = delete

      .. cpp:function:: ~ExponentTransform() = default

      -[ Public Static Functions ]-

      .. cpp:function:: ExponentTransformRcPtr Create()


   .. group-tab:: Python

      .. py:method:: equals(self: PyOpenColorIO.ExponentTransform, other: PyOpenColorIO.ExponentTransform) -> bool

      .. py:method:: getFormatMetadata(*args, **kwargs)

         Overloaded function.

         1. .. py:method:: getFormatMetadata(self: PyOpenColorIO.ExponentTransform) -> PyOpenColorIO.FormatMetadata

         2. .. py:method:: getFormatMetadata(self: PyOpenColorIO.ExponentTransform) -> PyOpenColorIO.FormatMetadata

      .. py:method:: getNegativeStyle(self: PyOpenColorIO.ExponentTransform) -> PyOpenColorIO.NegativeStyle

      .. py:method:: getValue(self: PyOpenColorIO.ExponentTransform) -> List[float[4]]

      .. py:method:: setNegativeStyle(self: PyOpenColorIO.ExponentTransform, style: PyOpenColorIO.NegativeStyle) -> None

      .. py:method:: setValue(self: PyOpenColorIO.ExponentTransform, value: List[float[4]]) -> None


ExponentWithLinearTransform
===========================

**class ExponentWithLinearTransform : public OpenColorIO::Transform
**

Represents power functions with a linear section in the shadows
such as sRGB and L*.

The basic formula is::

pow( (x + offset)/(1 + offset), gamma ) with the breakpoint at
offset/(gamma - 1).

Negative values are never clamped.


C++
---

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: const FormatMetadata &getFormatMetadata() const noexcept = 0
      

      .. cpp:function:: FormatMetadata &getFormatMetadata() noexcept = 0

      .. cpp:function:: bool equals(const ExponentWithLinearTransform &other) const noexcept = 0**

         Checks if this exactly equals other.

      .. cpp:function:: void getGamma(double (&values)[4]) const noexcept = 0

      .. cpp:function:: void setGamma(const double (&values)[4]) noexcept = 0

         Set the exponent value for the power function for R, G, B, A.

         **Note**
         The gamma values must be in the range of [1, 10]. Set the
         transform direction to inverse to obtain the effect of values
         less than 1.

      .. cpp:function:: void getOffset(double (&values)[4]) const noexcept = 0

      .. cpp:function:: void setOffset(const double (&values)[4]) noexcept = 0

         Set the offset value for the power function for R, G, B, A.

         **Note**
         The offset values must be in the range [0, 0.9].

      .. cpp:function:: NegativeStyle getNegativeStyle() const = 0

         Specifies how negative values are handled. Legal values:

         * **NEGATIVE_LINEAR**  Linear segment continues into negatives
         (default).

         * **NEGATIVE_MIRROR**  Positive curve is rotated 180 degrees around
         the origin to handle negatives.

      .. cpp:function:: void setNegativeStyle(NegativeStyle style) = 0

      .. cpp:function:: ExponentWithLinearTransform(const ExponentWithLinearTransform&) = delete

      .. cpp:function:: `ExponentWithLinearTransform`_ &operator=(const ExponentWithLinearTransform&) = delete

      .. cpp:function:: ~ExponentWithLinearTransform() = default

      -[ Public Static Functions ]-

      .. cpp:function:: ExponentWithLinearTransformRcPtr Create()

   .. group-tab:: Python

      .. py:method:: equals(self: PyOpenColorIO.ExponentWithLinearTransform, other: PyOpenColorIO.ExponentWithLinearTransform) -> bool

      .. py:method:: getFormatMetadata(*args, **kwargs)

         Overloaded function.

         1. .. py:method:: getFormatMetadata(self: PyOpenColorIO.ExponentWithLinearTransform) -> PyOpenColorIO.FormatMetadata

         2. .. py:method:: getFormatMetadata(self: PyOpenColorIO.ExponentWithLinearTransform) -> PyOpenColorIO.FormatMetadata

      .. py:method:: getGamma(self: PyOpenColorIO.ExponentWithLinearTransform) -> List[float[4]]

      .. py:method:: getNegativeStyle(self: PyOpenColorIO.ExponentWithLinearTransform) -> PyOpenColorIO.NegativeStyle

      .. py:method:: getOffset(self: PyOpenColorIO.ExponentWithLinearTransform) -> List[float[4]]

      .. py:method:: setGamma(self: PyOpenColorIO.ExponentWithLinearTransform, values: List[float[4]]) -> None

      .. py:method:: setNegativeStyle(self: PyOpenColorIO.ExponentWithLinearTransform, style: PyOpenColorIO.NegativeStyle) -> None

      .. py:method:: setOffset(self: PyOpenColorIO.ExponentWithLinearTransform, values: List[float[4]]) -> None


ExposureContrastTransform
=========================

**class ExposureContrastTransform : public OpenColorIO::Transform
**

Applies exposure, gamma, and pivoted contrast adjustments. Adjusts
the math to be appropriate for linear, logarithmic, or video color
spaces.


C++
---

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: const FormatMetadata &getFormatMetadata() const noexcept = 0

      .. cpp:function:: FormatMetadata &getFormatMetadata() noexcept = 0**

      .. cpp:function:: bool equals(const ExposureContrastTransform &other) const noexcept = 0**

         Checks if this exactly equals other.

      .. cpp:function:: ExposureContrastStyle getStyle() const = 0

      .. cpp:function:: void setStyle(ExposureContrastStyle style) = 0

         Select the algorithm for linear, video or log color spaces.

      .. cpp:function:: double getExposure() const = 0

      .. cpp:function:: void setExposure(double exposure) = 0

         Applies an exposure adjustment. The value is in units of stops
         (regardless of style), for example, a value of -1 would be
         equivalent to reducing the lighting by one half.

      .. cpp:function:: bool isExposureDynamic() const = 0

      .. cpp:function:: void makeExposureDynamic() = 0

      .. cpp:function:: double getContrast() const = 0

      .. cpp:function:: void setContrast(double contrast) = 0

         Applies a contrast/gamma adjustment around a pivot point. The
         contrast and gamma are mathematically the same, but two controls
         are provided to enable the use of separate dynamic parameters.
         Contrast is usually a scene-referred adjustment that pivots
         around gray whereas gamma is usually a display-referred
         adjustment that pivots around white.

      .. cpp:function:: bool isContrastDynamic() const = 0

      .. cpp:function:: void makeContrastDynamic() = 0

      .. cpp:function:: double getGamma() const = 0

      .. cpp:function:: void setGamma(double gamma) = 0

      .. cpp:function:: bool isGammaDynamic() const = 0

      .. cpp:function:: void makeGammaDynamic() = 0

      .. cpp:function:: double getPivot() const = 0

      .. cpp:function:: void setPivot(double pivot) = 0

         Set the pivot point around which the contrast and gamma controls
         will work. Regardless of whether linear/video/log-style is being
         used, the pivot is always expressed in linear. In other words, a
         pivot of 0.18 is always mid-gray.

      .. cpp:function:: double getLogExposureStep() const = 0

      .. cpp:function:: void setLogExposureStep(double logExposureStep) = 0

         Set the increment needed to move one stop for the log-style
         algorithm. For example, ACEScct is 0.057, LogC is roughly 0.074,
         and Cineon is roughly 90/1023 = 0.088. The default value is
         0.088.

      .. cpp:function:: double getLogMidGray() const = 0

      .. cpp:function:: void setLogMidGray(double logMidGray) = 0

         Set the position of 18% gray for use by the log-style algorithm.
         For example, ACEScct is about 0.41, LogC is about 0.39, and
         ADX10 is 445/1023 = 0.435. The default value is 0.435.

      .. cpp:function:: ~ExposureContrastTransform() = default

      .. cpp:function:: ExposureContrastTransformRcPtr Create()

   .. group-tab:: Python

      .. py:method:: equals(self: PyOpenColorIO.ExposureContrastTransform, other: PyOpenColorIO.ExposureContrastTransform) -> bool

      .. py:method:: getContrast(self: PyOpenColorIO.ExposureContrastTransform) -> float

      .. py:method:: getExposure(self: PyOpenColorIO.ExposureContrastTransform) -> float

      .. py:method:: getFormatMetadata(*args, **kwargs)

         Overloaded function.

         1. .. py:method:: getFormatMetadata(self: PyOpenColorIO.ExposureContrastTransform) -> PyOpenColorIO.FormatMetadata

         2. .. py:method:: getFormatMetadata(self: PyOpenColorIO.ExposureContrastTransform) -> PyOpenColorIO.FormatMetadata

      .. py:method:: getGamma(self: PyOpenColorIO.ExposureContrastTransform) -> float

      .. py:method:: getLogExposureStep(self: PyOpenColorIO.ExposureContrastTransform) -> float

      .. py:method:: getLogMidGray(self: PyOpenColorIO.ExposureContrastTransform) -> float

      .. py:method:: getPivot(self: PyOpenColorIO.ExposureContrastTransform) -> float

      .. py:method:: getStyle(self: PyOpenColorIO.ExposureContrastTransform) -> PyOpenColorIO.ExposureContrastStyle

      .. py:method:: isContrastDynamic(self: PyOpenColorIO.ExposureContrastTransform) -> bool

      .. py:method:: isExposureDynamic(self: PyOpenColorIO.ExposureContrastTransform) -> bool

      .. py:method:: isGammaDynamic(self: PyOpenColorIO.ExposureContrastTransform) -> bool

      .. py:method:: makeContrastDynamic(self: PyOpenColorIO.ExposureContrastTransform) -> None

      .. py:method:: makeExposureDynamic(self: PyOpenColorIO.ExposureContrastTransform) -> None

      .. py:method:: makeGammaDynamic(self: PyOpenColorIO.ExposureContrastTransform) -> None

      .. py:method:: setContrast(self: PyOpenColorIO.ExposureContrastTransform, contrast: float) -> None

      .. py:method:: setExposure(self: PyOpenColorIO.ExposureContrastTransform, exposure: float) -> None

      .. py:method:: setGamma(self: PyOpenColorIO.ExposureContrastTransform, gamma: float) -> None

      .. py:method:: setLogExposureStep(self: PyOpenColorIO.ExposureContrastTransform, logExposureStep: float) -> None

      .. py:method:: setLogMidGray(self: PyOpenColorIO.ExposureContrastTransform, logMidGray: float) -> None

      .. py:method:: setPivot(self: PyOpenColorIO.ExposureContrastTransform, pivot: float) -> None

      .. py:method:: setStyle(self: PyOpenColorIO.ExposureContrastTransform, style: PyOpenColorIO.ExposureContrastStyle) -> None


FileTransform
=============

**class FileTransform : public OpenColorIO::Transform**


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: TransformRcPtr createEditableCopy() const override

      .. cpp:function:: TransformDirection getDirection() const noexcept override

      .. cpp:function:: void setDirection(TransformDirection dir) noexcept override

         Note that this only affects the evaluation and not the values
         stored in the object.

      .. cpp:function:: void validate() const override

         Will throw if data is not valid.

      .. cpp:function:: const char *getSrc() const

      .. cpp:function:: void setSrc(const char *src)

      .. cpp:function:: const char *getCCCId() const

      .. cpp:function:: void setCCCId(const char *id)

      .. cpp:function:: CDLStyle getCDLStyle() const

      .. cpp:function:: void setCDLStyle(CDLStyle)

         Can be used with CDL, CC & CCC formats to specify the clamping
         behavior of the CDLTransform. Default is CDL_NO_CLAMP.

      .. cpp:function:: Interpolation getInterpolation() const

      .. cpp:function:: void setInterpolation(Interpolation interp)

      .. cpp:function:: `FileTransform`_ &operator=(const FileTransform&) = delete

      .. cpp:function:: ~FileTransform()

      -[ Public Static Functions ]-

      .. cpp:function:: FileTransformRcPtr Create()

      .. cpp:function:: int getNumFormats()

         Get the number of LUT readers.

      .. cpp:function:: const char *getFormatNameByIndex(int index)

         Get the LUT readers at index, return empty string if an invalid
         index is specified.

      .. cpp:function:: const char *getFormatExtensionByIndex(int index)

         Get the LUT reader extension at index, return empty string if an
         invalid index is specified.

   .. group-tab:: Python

      .. py:class:: FormatIterator

      .. py:method:: getCCCId(self: PyOpenColorIO.FileTransform) -> str

      .. py:method:: getCDLStyle(self: PyOpenColorIO.FileTransform) -> PyOpenColorIO.CDLStyle

      .. py:method:: getFormats() -> OpenColorIO_v2_0dev::PyIterator<std::shared_ptr<OpenColorIO_v2_0dev::FileTransform>, 0>

      .. py:method:: getInterpolation(self: PyOpenColorIO.FileTransform) -> PyOpenColorIO.Interpolation

      .. py:method:: getSrc(self: PyOpenColorIO.FileTransform) -> str

      .. py:method:: setCCCId(self: PyOpenColorIO.FileTransform, cccId: str) -> None

      .. py:method:: setCDLStyle(self: PyOpenColorIO.FileTransform, style: PyOpenColorIO.CDLStyle) -> None

      .. py:method:: setInterpolation(self: PyOpenColorIO.FileTransform, interpolation: PyOpenColorIO.Interpolation) -> None

      .. py:method:: setSrc(self: PyOpenColorIO.FileTransform, src: str) -> None


FixedFunctionTransform
======================

**class FixedFunctionTransform : public OpenColorIO::Transform**

Provides a set of hard-coded algorithmic building blocks that are
needed to accurately implement various common color
transformations.

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: const FormatMetadata &getFormatMetadata() const noexcept = 0
      

      .. cpp:function:: FormatMetadata &getFormatMetadata() noexcept = 0

      .. cpp:function:: bool equals(const FixedFunctionTransform &other) const noexcept = 0

      Checks if this exactly equals other.

      .. cpp:function:: FixedFunctionStyle getStyle() const = 0

      .. cpp:function:: void setStyle(FixedFunctionStyle style) = 0

         Select which algorithm to use.

      .. cpp:function:: size_t getNumParams() const = 0

      .. cpp:function:: void getParams(double *params) const = 0

      .. cpp:function:: void setParams(const double *params, size_t num) = 0

         Set the parameters (for functions that require them).

      .. cpp:function:: FixedFunctionTransform(const FixedFunctionTransform&) = delete

      .. cpp:function:: `FixedFunctionTransform`_ &operator=(const FixedFunctionTransform&) = delete

      .. cpp:function:: ~FixedFunctionTransform() = default

      -[ Public Static Functions ]-

      .. cpp:function:: FixedFunctionTransformRcPtr Create()

.. tabs::

   .. group-tab:: Python

      .. py:method:: equals(self: PyOpenColorIO.FixedFunctionTransform, other: PyOpenColorIO.FixedFunctionTransform) -> bool

      .. py:method:: getFormatMetadata(*args, **kwargs)

         Overloaded function.

         1. .. py:method:: getFormatMetadata(self: PyOpenColorIO.FixedFunctionTransform) -> PyOpenColorIO.FormatMetadata

         2. .. py:method:: getFormatMetadata(self: PyOpenColorIO.FixedFunctionTransform) -> PyOpenColorIO.FormatMetadata

      .. py:method:: getParams(self: PyOpenColorIO.FixedFunctionTransform) List[float]

      .. py:method:: getStyle(self: PyOpenColorIO.FixedFunctionTransform) PyOpenColorIO.FixedFunctionStyle

      .. py:method:: setParams(self: PyOpenColorIO.FixedFunctionTransform, para List[float]) -> None

      .. py:method:: setStyle(self: PyOpenColorIO.FixedFunctionTransform, sty PyOpenColorIO.FixedFunctionStyle) -> None


GroupTransform
==============

**class GroupTransform : public OpenColorIO::Transform


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: TransformRcPtr createEditableCopy() const override

      .. cpp:function:: TransformDirection getDirection() const noexcept override

      .. cpp:function:: void setDirection(TransformDirection dir) noexcept override

         Note that this only affects the evaluation and not the values
         stored in the object.

      .. cpp:function:: void validate() const override

         Will throw if data is not valid.

      .. cpp:function:: const FormatMetadata &getFormatMetadata() const noexcept

      .. cpp:function:: FormatMetadata &getFormatMetadata() noexcept

      .. cpp:function:: ConstTransformRcPtr getTransform(int index) const

      .. cpp:function:: TransformRcPtr &getTransform(int index)

      .. cpp:function:: int getNumTransforms() const

      .. cpp:function:: void appendTransform(TransformRcPtr transform)

         Adds a transform to the end of the group.

      .. cpp:function:: void prependTransform(TransformRcPtr transform)

         Add a transform at the beginning of the group.

      .. cpp:function:: `GroupTransform`_ &operator=(const GroupTransform&) = delete

      .. cpp:function:: ~GroupTransform()

      -[ Public Static Functions ]-

      .. cpp:function:: GroupTransformRcPtr Create()

   .. group-tab:: Python

      .. py:class:: TransformIterator

      .. py:method:: appendTransform(self: PyOpenColorIO.GroupTransform, transform: PyOpenColorIO.Transform) -> None

      .. py:method:: getFormatMetadata(*args, **kwargs)

         Overloaded function.

         1. .. py:method:: getFormatMetadata(self: PyOpenColorIO.GroupTransform) -> PyOpenColorIO.FormatMetadata

         2. .. py:method:: getFormatMetadata(self: PyOpenColorIO.GroupTransform) -> PyOpenColorIO.FormatMetadata

      .. py:method:: prependTransform(self: PyOpenColorIO.GroupTransform, transform: PyOpenColorIO.Transform) -> None


LogAffineTransform
==================

**class LogAffineTransform : public OpenColorIO::Transform**

Applies a logarithm with an affine transform before and after.
Represents the Cineon lin-to-log type transforms::

logSideSlope * log( linSideSlope * color + linSideOffset, base) +
logSideOffset

* Default values are: 1. * log( 1. * color + 0., 2.) + 0.

* The alpha channel is not affected.


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: const FormatMetadata &getFormatMetadata() const noexcept = 0
      

      .. cpp:function:: FormatMetadata &getFormatMetadata() noexcept = 0

      .. cpp:function:: bool equals(const LogAffineTransform &other) const noexcept = 0
      

         Checks if this exactly equals other.

      .. cpp:function:: double getBase() const noexcept = 0

      .. cpp:function:: void setBase(double base) noexcept = 0

      .. cpp:function:: void getLogSideSlopeValue(double (&values)[3]) const noexcept = 0
      

      .. cpp:function:: void setLogSideSlopeValue(const double (&values)[3]) noexcept = 0
      

      .. cpp:function:: void getLogSideOffsetValue(double (&values)[3]) const noexcept = 0

      .. cpp:function:: void setLogSideOffsetValue(const double (&values)[3]) noexcept = 0

      .. cpp:function:: void getLinSideSlopeValue(double (&values)[3]) const noexcept = 0
      

      .. cpp:function:: void setLinSideSlopeValue(const double (&values)[3]) noexcept = 0
      

      .. cpp:function:: void getLinSideOffsetValue(double (&values)[3]) const noexcept = 0

      .. cpp:function:: void setLinSideOffsetValue(const double (&values)[3]) noexcept = 0

      .. cpp:function:: LogAffineTransform(const LogAffineTransform&) = delete

      .. cpp:function:: `LogAffineTransform`_ &operator=(const LogAffineTransform&) = delete

      .. cpp:function:: ~LogAffineTransform() = default

      -[ Public Static Functions ]-

      .. cpp:function:: LogAffineTransformRcPtr Create()

   .. group-tab:: Python

      .. py:method:: equals(self: PyOpenColorIO.LogAffineTransform, other: PyOpenColorIO.LogAffineTransform) -> bool

      .. py:method:: getBase(self: PyOpenColorIO.LogAffineTransform) -> float

      .. py:method:: getFormatMetadata(*args, **kwargs)

         Overloaded function.

         1. .. py:method:: getFormatMetadata(self: PyOpenColorIO.LogAffineTransform) -> PyOpenColorIO.FormatMetadata

         2. .. py:method:: getFormatMetadata(self: PyOpenColorIO.LogAffineTransform) -> PyOpenColorIO.FormatMetadata

      .. py:method:: getLinSideOffsetValue(self: PyOpenColorIO.LogAffineTransform) -> List[float[3]]

      .. py:method:: getLinSideSlopeValue(self: PyOpenColorIO.LogAffineTransform) -> List[float[3]]

      .. py:method:: getLogSideOffsetValue(self: PyOpenColorIO.LogAffineTransform) -> List[float[3]]

      .. py:method:: getLogSideSlopeValue(self: PyOpenColorIO.LogAffineTransform) -> List[float[3]]

      .. py:method:: setBase(self: PyOpenColorIO.LogAffineTransform, base: float) -> None

      .. py:method:: setLinSideOffsetValue(self: PyOpenColorIO.LogAffineTransform, values: List[float[3]]) -> None

      .. py:method:: setLinSideSlopeValue(self: PyOpenColorIO.LogAffineTransform, values: List[float[3]]) -> None

      .. py:method:: setLogSideOffsetValue(self: PyOpenColorIO.LogAffineTransform, values: List[float[3]]) -> None

      .. py:method:: setLogSideSlopeValue(self: PyOpenColorIO.LogAffineTransform, values: List[float[3]]) -> None


LogCameraTransform
==================

**class LogCameraTransform : public OpenColorIO::Transform**

Same as :cpp:class:``LogAffineTransform`` but with the addition of
a linear segment near black. This formula is used for many camera
logs (e.g., LogC) as well as ACEScct.

* The linSideBreak specifies the point on the linear axis where
the log and linear segments meet. It must be set (there is no
default).

* The linearSlope specifies the slope of the linear segment of the
forward (linToLog) transform. By default it is set equal to the
slope of the log curve at the break point.

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: const FormatMetadata &getFormatMetadata() const noexcept = 0
      

      .. cpp:function:: FormatMetadata &getFormatMetadata() noexcept = 0

      .. cpp:function:: bool equals(const LogCameraTransform &other) const noexcept = 0
      

         Checks if this exactly equals other.

      .. cpp:function:: double getBase() const noexcept = 0

      .. cpp:function:: void setBase(double base) noexcept = 0

      .. cpp:function:: void getLogSideSlopeValue(double (&values)[3]) const noexcept = 0
      

      .. cpp:function:: void setLogSideSlopeValue(const double (&values)[3]) noexcept = 0
      

      .. cpp:function:: void getLogSideOffsetValue(double (&values)[3]) const noexcept = 0

      .. cpp:function:: void setLogSideOffsetValue(const double (&values)[3]) noexcept = 0

      .. cpp:function:: void getLinSideSlopeValue(double (&values)[3]) const noexcept = 0
      

      .. cpp:function:: void setLinSideSlopeValue(const double (&values)[3]) noexcept = 0
      

      .. cpp:function:: void getLinSideOffsetValue(double (&values)[3]) const noexcept = 0

      .. cpp:function:: void setLinSideOffsetValue(const double (&values)[3]) noexcept = 0

      .. cpp:function:: bool getLinSideBreakValue(double (&values)[3]) const noexcept = 0
      

         Return true if LinSideBreak values were set, false if they were
         not.

      .. cpp:function:: void setLinSideBreakValue(const double (&values)[3]) noexcept = 0
      

      .. cpp:function:: bool getLinearSlopeValue(double (&values)[3]) const = 0

         Return true if LinearSlope values were set, false if they were
         not.

      .. cpp:function:: void setLinearSlopeValue(const double (&values)[3]) = 0

         Set LinearSlope value.

         **Note**
         You must call setLinSideBreakValue before calling this.

      .. cpp:function:: void unsetLinearSlopeValue() = 0

         Remove LinearSlope values so that default values are used.

      .. cpp:function:: LogCameraTransform(const LogCameraTransform&) = delete

      .. cpp:function:: `LogCameraTransform`_ &operator=(const LogCameraTransform&) = delete

      .. cpp:function:: ~LogCameraTransform() = default

      -[ Public Static Functions ]-

      .. cpp:function:: LogCameraTransformRcPtr Create()

   .. group-tab:: Python

      .. py:method:: equals(self: PyOpenColorIO.LogCameraTransform, other:
      PyOpenColorIO.LogCameraTransform) -> bool

      .. py:method:: getBase(self: PyOpenColorIO.LogCameraTransform) -> float

      .. py:method:: getFormatMetadata(*args, **kwargs)

         Overloaded function.

         1. .. py:method:: getFormatMetadata(self: PyOpenColorIO.LogCameraTransform) -> PyOpenColorIO.FormatMetadata

         2. .. py:method:: getFormatMetadata(self: PyOpenColorIO.LogCameraTransform) -> PyOpenColorIO.FormatMetadata

      .. py:method:: getLinSideBreakValue(self: PyOpenColorIO.LogCameraTransform) -> List[float[3]]

      .. py:method:: getLinSideOffsetValue(self: PyOpenColorIO.LogCameraTransform) -> List[float[3]]

      .. py:method:: getLinSideSlopeValue(self: PyOpenColorIO.LogCameraTransform) -> List[float[3]]

      .. py:method:: getLinearSlopeValue(self: PyOpenColorIO.LogCameraTransform) -> List[float[3]]

      .. py:method:: getLogSideOffsetValue(self: PyOpenColorIO.LogCameraTransform) -> List[float[3]]

      .. py:method:: getLogSideSlopeValue(self: PyOpenColorIO.LogCameraTransform) -> List[float[3]]

      .. py:method:: setBase(self: PyOpenColorIO.LogCameraTransform, base: float) -> None

      .. py:method:: setLinSideBreakValue(self: PyOpenColorIO.LogCameraTransform, values: List[float[3]]) -> None

      .. py:method:: setLinSideOffsetValue(self: PyOpenColorIO.LogCameraTransform, values: List[float[3]]) -> None

      .. py:method:: setLinSideSlopeValue(self: PyOpenColorIO.LogCameraTransform, values: List[float[3]]) -> None

      .. py:method:: setLinearSlopeValue(self: PyOpenColorIO.LogCameraTransform, values: List[float[3]]) -> None

      .. py:method:: setLogSideOffsetValue(self: PyOpenColorIO.LogCameraTransform, values: List[float[3]]) -> None

      .. py:method:: setLogSideSlopeValue(self: PyOpenColorIO.LogCameraTransform, values: List[float[3]]) -> None

      .. py:method:: unsetLinearSlopeValue(self: PyOpenColorIO.LogCameraTransform) -> None


LogTransform
============

**class LogTransform : public OpenColorIO::Transform**

Represents log transform: log(color, base)

* The input will be clamped for negative numbers.

* Default base is 2.0.

* The alpha channel is not affected.

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: const FormatMetadata &getFormatMetadata() const noexcept = 0
      

      .. cpp:function:: FormatMetadata &getFormatMetadata() noexcept = 0

      .. cpp:function:: bool equals(const LogTransform &other) const noexcept = 0

      Checks if this exactly equals other.

      .. cpp:function:: double getBase() const noexcept = 0

      .. cpp:function:: void setBase(double val) noexcept = 0

      .. cpp:function:: LogTransform(const LogTransform&) = delete

      .. cpp:function:: `LogTransform`_ &operator=(const LogTransform&) = delete

      .. cpp:function:: ~LogTransform() = default

      -[ Public Static Functions ]-

      .. cpp:function:: LogTransformRcPtr Create()

   .. group-tab:: Python

      .. py:method:: equals(self: PyOpenColorIO.LogTransform, other: PyOpenColorIO.LogTransform) -> bool

      .. py:method:: getBase(self: PyOpenColorIO.LogTransform) -> float

      .. py:method:: getFormatMetadata(*args, **kwargs)

         Overloaded function.

         1. .. py:method:: getFormatMetadata(self: PyOpenColorIO.LogTransform) -> PyOpenColorIO.FormatMetadata

         2. .. py:method:: getFormatMetadata(self: PyOpenColorIO.LogTransform) -> PyOpenColorIO.FormatMetadata

      .. py:method:: setBase(self: PyOpenColorIO.LogTransform, base: float) -> None


LookTransform
=============

**class LookTransform : public OpenColorIO::Transform**


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: TransformRcPtr createEditableCopy() const override**

      .. cpp:function:: TransformDirection getDirection() const noexcept override**

      .. cpp:function:: void setDirection(TransformDirection dir) noexcept override**

         Note that this only affects the evaluation and not the values
         stored in the object.

      .. cpp:function:: void validate() const override**

         Will throw if data is not valid.

      .. cpp:function:: const char *getSrc() const**

      .. cpp:function:: void setSrc(const char *src)**

      .. cpp:function:: const char *getDst() const**

      .. cpp:function:: void setDst(const char *dst)**

      .. cpp:function:: const char *getLooks() const**

      .. cpp:function:: void setLooks(const char *looks)**

         Specify looks to apply. Looks is a potentially comma (or colon)
         delimited list of look names, Where +/- prefixes are optionally
         allowed to denote forward/inverse look specification. (And
         forward is assumed in the absence of either)

      .. cpp:function:: bool getSkipColorSpaceConversion() const**

      .. cpp:function:: void setSkipColorSpaceConversion(bool skip)**

      .. cpp:function:: `LookTransform`_ &operator=(const LookTransform&) = delete**

      .. cpp:function:: ~LookTransform()**

         Do not use (needed only for pybind11).

      -[ Public Static Functions ]-

      .. cpp:function:: LookTransformRcPtr Create()**

      .. cpp:function:: const char *GetLooksResultColorSpace(const ConstConfigRcPtr &config, const ConstContextRcPtr &context, const char *looks)**

         Return the name of the color space after applying looks in the
         forward direction but without converting to the destination
         color space. This is equivalent to the process space of the last
         look in the look sequence (and takes into account that a look
         fall-back may be used).

   .. group-tab:: Python

      .. py:method:: getDst(self: PyOpenColorIO.LookTransform) -> str

      .. py:method:: getLooks(self: PyOpenColorIO.LookTransform) -> str

      .. py:method:: getSkipColorSpaceConversion(self: PyOpenColorIO.LookTransform) -> bool

      .. py:method:: getSrc(self: PyOpenColorIO.LookTransform) -> str

      .. py:method:: setDst(self: PyOpenColorIO.LookTransform, dst: str) -> None

      .. py:method:: setLooks(self: PyOpenColorIO.LookTransform, looks: str) -> None

      .. py:method:: setSkipColorSpaceConversion(self: PyOpenColorIO.LookTransform, skipColorSpaceConversion: bool) -> None

      .. py:method:: setSrc(self: PyOpenColorIO.LookTransform, src: str) -> None


Lut1DTransform
==============

**class Lut1DTransform : public OpenColorIO::Transform**

Represents a 1D-LUT transform.


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: BitDepth getFileOutputBitDepth() const noexcept = 0

      .. cpp:function:: void setFileOutputBitDepth(BitDepth bitDepth) noexcept = 0

         Get the bit-depth associated with the LUT values read from a
         file or set the bit-depth of values to be written to a file (for
         file formats such as CLF that support multiple bit-depths).
         However, note that the values stored in the object are always
         normalized.

      .. cpp:function:: const FormatMetadata &getFormatMetadata() const noexcept = 0
      

      .. cpp:function:: FormatMetadata &getFormatMetadata() noexcept = 0

      .. cpp:function:: bool equals(const Lut1DTransform &other) const noexcept = 0

         Checks if this exactly equals other.

      .. cpp:function:: unsigned long getLength() const = 0

      .. cpp:function:: void setLength(unsigned long length) = 0

         Changing the length will reset the LUT to identity. Will throw
         for lengths longer than 1024x1024.

      .. cpp:function:: void getValue(unsigned long index, float &r, float &g, float &b) const = 0

      .. cpp:function:: void setValue(unsigned long index, float r, float g, float b) = 0
      

         Set the values of a LUT1D. Will throw if the index is outside of
         the range from 0 to (length-1).

         The LUT values are always for the “forward” LUT, regardless of
         how the transform direction is set.

         These values are normalized relative to what may be stored in
         any given LUT files. For example in a CLF file using a “10i”
         output depth, a value of 1023 in the file is normalized to 1.0.
         The values here are unclamped and may extend outside [0,1].

         LUTs in various file formats may only provide values for one
         channel where R, G, B are the same. Even in that case, you
         should provide three equal values to the setter.

      .. cpp:function:: bool getInputHalfDomain() const noexcept = 0

      .. cpp:function:: void setInputHalfDomain(bool isHalfDomain) noexcept = 0

         In a half-domain LUT, the contents of the LUT specify the
         desired value of the function for each half-float value.
         Therefore, the length of the LUT must be 65536 entries or else
         `validate()`_ will throw.

      .. cpp:function:: bool getOutputRawHalfs() const noexcept = 0

      .. cpp:function:: void setOutputRawHalfs(bool isRawHalfs) noexcept = 0

         Set OutputRawHalfs to true if you want to output the LUT
         contents as 16-bit floating point values expressed as unsigned
         16-bit integers representing the equivalent bit pattern. For
         example, the value 1.0 would be written as the integer 15360
         because it has the same bit-pattern. Note that this implies the
         values will be quantized to a 16-bit float. Note that this
         setting only controls the output formatting (where supported)
         and not the values for getValue/setValue. The only file formats
         that currently support this are CLF and CTF.

      .. cpp:function:: Lut1DHueAdjust getHueAdjust() const noexcept = 0

      .. cpp:function:: void setHueAdjust(Lut1DHueAdjust algo) noexcept = 0

         The 1D-LUT transform optionally supports a hue adjustment
         feature that was used in some versions of ACES. This adjusts the
         hue of the result to approximately match the input.

      .. cpp:function:: Interpolation getInterpolation() const = 0

      .. cpp:function:: void setInterpolation(Interpolation algo) = 0

      .. cpp:function:: Lut1DTransform(const Lut1DTransform&) = delete

      .. cpp:function:: `Lut1DTransform`_ &operator=(const Lut1DTransform&) = delete

      .. cpp:function:: ~Lut1DTransform() = default

      -[ Public Static Functions ]-

      .. cpp:function:: Lut1DTransformRcPtr Create()

         Create an identity 1D-LUT of length two.

      .. cpp:function:: Lut1DTransformRcPtr Create(unsigned long length, bool isHalfDomain)

         Create an identity 1D-LUT with specific length and half-domain
         setting. Will throw for lengths longer than 1024x1024.

.. tabs::

   .. group-tab:: Python

      .. py:method:: equals(self: PyOpenColorIO.Lut1DTransform, other: PyOpenColorIO.Lut1DTransform) -> bool

      .. py:method:: getData(self: PyOpenColorIO.Lut1DTransform) -> array

      .. py:method:: getFileOutputBitDepth(self: PyOpenColorIO.Lut1DTransform) -> PyOpenColorIO.BitDepth

      .. py:method:: getFormatMetadata(*args, **kwargs)**

         Overloaded function.

         1. .. py:method:: getFormatMetadata(self: PyOpenColorIO.Lut1DTransform) -> PyOpenColorIO.FormatMetadata

         2. .. py:method:: getFormatMetadata(self: PyOpenColorIO.Lut1DTransform) -> PyOpenColorIO.FormatMetadata

      .. py:method:: getHueAdjust(self: PyOpenColorIO.Lut1DTransform) -> PyOpenColorIO.Lut1DHueAdjust

      .. py:method:: getInputHalfDomain(self: PyOpenColorIO.Lut1DTransform) -> bool

      .. py:method:: getInterpolation(self: PyOpenColorIO.Lut1DTransform) -> PyOpenColorIO.Interpolation

      .. py:method:: getLength(self: PyOpenColorIO.Lut1DTransform) -> int

      .. py:method:: getOutputRawHalfs(self: PyOpenColorIO.Lut1DTransform) -> bool

      .. py:method:: getValue(self: PyOpenColorIO.Lut1DTransform, index: int) -> tuple

      .. py:method:: setData(self: PyOpenColorIO.Lut1DTransform, data: buffer) -> None

      .. py:method:: setFileOutputBitDepth(self: PyOpenColorIO.Lut1DTransform, bitDepth: PyOpenColorIO.BitDepth) -> None

      .. py:method:: setHueAdjust(self: PyOpenColorIO.Lut1DTransform, hueAdjust: PyOpenColorIO.Lut1DHueAdjust) -> None

      .. py:method:: setInputHalfDomain(self: PyOpenColorIO.Lut1DTransform, isHalfDomain: bool) -> None

      .. py:method:: setInterpolation(self: PyOpenColorIO.Lut1DTransform, interpolation: PyOpenColorIO.Interpolation) -> None

      .. py:method:: setLength(self: PyOpenColorIO.Lut1DTransform, length: int) -> None

      .. py:method:: setOutputRawHalfs(self: PyOpenColorIO.Lut1DTransform, isRawHalfs: bool) -> None

      .. py:method:: setValue(self: PyOpenColorIO.Lut1DTransform, index: int, r: float, g: float, b: float) -> None


Lut3DTransform
==============

**class Lut3DTransform : public OpenColorIO::Transform**

Represents a 3D-LUT transform.

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: BitDepth getFileOutputBitDepth() const noexcept = 0

      .. cpp:function:: void setFileOutputBitDepth(BitDepth bitDepth) noexcept = 0

         Get the bit-depth associated with the LUT values read from a
         file or set the bit-depth of values to be written to a file (for
         file formats such as CLF that support multiple bit-depths).
         However, note that the values stored in the object are always
         normalized.

      .. cpp:function:: const FormatMetadata &getFormatMetadata() const noexcept = 0
      

      .. cpp:function:: FormatMetadata &getFormatMetadata() noexcept = 0

      .. cpp:function:: bool equals(const Lut3DTransform &other) const noexcept = 0

         Checks if this exactly equals other.

      .. cpp:function:: unsigned long getGridSize() const = 0

      .. cpp:function:: void setGridSize(unsigned long gridSize) = 0

         Changing the grid size will reset the LUT to identity. Will
         throw for grid sizes larger than 129.

      .. cpp:function:: void getValue(unsigned long indexR, unsigned long indexG, unsigned long indexB, float &r, float &g, float &b) const = 0

      .. cpp:function:: void setValue(unsigned long indexR, unsigned long indexG, unsigned long indexB, float r, float g, float b) = 0

         Set the values of a 3D-LUT. Will throw if an index is outside of
         the range from 0 to (gridSize-1).

         The LUT values are always for the “forward” LUT, regardless of
         how the transform direction is set.

         These values are normalized relative to what may be stored in
         any given LUT files. For example in a CLF file using a “10i”
         output depth, a value of 1023 in the file is normalized to 1.0.
         The values here are unclamped and may extend outside [0,1].

         .. cpp:function:: Interpolation getInterpolation() const = 0

      .. cpp:function:: void setInterpolation(Interpolation algo) = 0

      .. cpp:function:: Lut3DTransform(const Lut3DTransform&) = delete

      .. cpp:function:: `Lut3DTransform`_ &operator=(const Lut3DTransform&) = delete

      .. cpp:function:: ~Lut3DTransform() = default

      -[ Public Static Functions ]-

      .. cpp:function:: Lut3DTransformRcPtr Create()

         Create an identity 3D-LUT of size 2x2x2.

      .. cpp:function:: Lut3DTransformRcPtr Create(unsigned long gridSize)

         Create an identity 3D-LUT with specific grid size. Will throw
         for grid size larger than 129.

   .. group-tab:: Python

      .. py:method:: equals(self: PyOpenColorIO.Lut3DTransform, other: PyOpenColorIO.Lut3DTransform) -> bool

      .. py:method:: getData(self: PyOpenColorIO.Lut3DTransform) -> array

      .. py:method:: getFileOutputBitDepth(self: PyOpenColorIO.Lut3DTransform) -> PyOpenColorIO.BitDepth

      .. py:method:: getFormatMetadata(*args, **kwargs)

         Overloaded function.

         1. .. py:method:: getFormatMetadata(self: PyOpenColorIO.Lut3DTransform) -> PyOpenColorIO.FormatMetadata

         2. .. py:method:: getFormatMetadata(self: PyOpenColorIO.Lut3DTransform) -> PyOpenColorIO.FormatMetadata

      .. py:method:: getGridSize(self: PyOpenColorIO.Lut3DTransform) -> int

      .. py:method:: getInterpolation(self: PyOpenColorIO.Lut3DTransform) -> PyOpenColorIO.Interpolation

      .. py:method:: getValue(self: PyOpenColorIO.Lut3DTransform, indexR: int, indexG: int, indexB: int) -> tuple

      .. py:method:: setData(self: PyOpenColorIO.Lut3DTransform, data: buffer) -> None

      .. py:method:: setFileOutputBitDepth(self: PyOpenColorIO.Lut3DTransform, bitDepth: PyOpenColorIO.BitDepth) -> None

      .. py:method:: setGridSize(self: PyOpenColorIO.Lut3DTransform, gridSize: int) -> None

      .. py:method:: setInterpolation(self: PyOpenColorIO.Lut3DTransform, interpolation: PyOpenColorIO.Interpolation) -> None

      .. py:method:: setValue(self: PyOpenColorIO.Lut3DTransform, indexR: int, indexG: int, indexB: int, r: float, g: float, b: float) -> None


MatrixTransform
===============

**class MatrixTransform : public OpenColorIO::Transform**

Represents an MX+B Matrix transform.

**Note**
For singular matrices, an inverse direction will throw an
exception during finalization.


.. tabs::

   .. group-tab:: C++

      .. cpp:function:: const FormatMetadata &getFormatMetadata() const noexcept = 0

      .. cpp:function:: FormatMetadata &getFormatMetadata() noexcept = 0

      .. cpp:function:: bool equals(const MatrixTransform &other) const noexcept = 0

         Checks if this exactly equals other.

      .. cpp:function:: void getMatrix(double *m44) const = 0

      .. cpp:function:: void setMatrix(const double *m44) = 0

         Get or set the values of a Matrix. Expects 16 values, where the
         first four are the coefficients to generate the R output channel
         from R, G, B, A input channels.

         The Matrix values are always for the “forward” Matrix,
         regardless of how the transform direction is set.

         These values are normalized relative to what may be stored in
         file formats such as CLF. For example in a CLF file using a
         “32f” input depth and “10i” output depth, a value of 1023 in the
         file is normalized to 1.0. The values here are unclamped and may
         extend outside [0,1].

      .. cpp:function:: void getOffset(double *offset4) const = 0

      .. cpp:function:: void setOffset(const double *offset4) = 0

         Get or set the R, G, B, A offsets to be applied after the
         matrix.

         These values are normalized relative to what may be stored in
         file formats such as CLF. For example, in a CLF file using a
         “10i” output depth, a value of 1023 in the file is normalized to
         1.0. The values here are unclamped and may extend outside [0,1].

      .. cpp:function:: BitDepth getFileInputBitDepth() const noexcept = 0

      .. cpp:function:: void setFileInputBitDepth(BitDepth bitDepth) noexcept = 0

      .. cpp:function:: BitDepth getFileOutputBitDepth() const noexcept = 0

      .. cpp:function:: void setFileOutputBitDepth(BitDepth bitDepth) noexcept = 0

      .. cpp:function:: MatrixTransform(const MatrixTransform&) = delete

      .. cpp:function:: `MatrixTransform`_ &operator=(const MatrixTransform&) = delete

      .. cpp:function:: ~MatrixTransform() = default

      -[ Public Static Functions ]-

      .. cpp:function:: MatrixTransformRcPtr Create()

      .. cpp:function:: void Fit(double *m44, double *offset4, const double *oldmin4, const double *oldmax4, const double *newmin4, const double *newmax4)

      .. cpp:function:: void Identity(double *m44, double *offset4)

      .. cpp:function:: void Sat(double *m44, double *offset4, double sat, const double *lumaCoef3)

      .. cpp:function:: void Scale(double *m44, double *offset4, const double *scale4)

      .. cpp:function:: void View(double *m44, double *offset4, int *channelHot4, const double *lumaCoef3)

   .. group-tab:: Python

      .. py:function:: Fit(oldMin: List[float[4]] = [0.0, 0.0, 0.0, 0.0], oldMax: List[float[4]] = [1.0, 1.0, 1.0, 1.0], newMin: List[float[4]] = [0.0, 0.0, 0.0, 0.0], newMax: List[float[4]] = [1.0, 1.0, 1.0, 1.0]) -> `PyOpenColorIO.MatrixTransform`_

      .. py:function:: Identity() -> `PyOpenColorIO.MatrixTransform`_

      .. py:function:: Sat(sat: float, lumaCoef: List[float[3]]) -> `PyOpenColorIO.MatrixTransform`_

      .. py:function:: Scale(scale: List[float[4]]) -> `PyOpenColorIO.MatrixTransform`_

      .. py:function:: View(channelHot: List[int[4]], lumaCoef: List[float[3]]) -> `PyOpenColorIO.MatrixTransform`_

      .. py:method:: equals(self: PyOpenColorIO.MatrixTransform, other: PyOpenColorIO.MatrixTransform) -> bool

      .. py:method:: getFileInputBitDepth(self: PyOpenColorIO.MatrixTransform) -> PyOpenColorIO.BitDepth

      .. py:method:: getFileOutputBitDepth(self: PyOpenColorIO.MatrixTransform) -> PyOpenColorIO.BitDepth

      .. py:method:: getFormatMetadata(*args, **kwargs)

         Overloaded function.

         1. .. py:method:: getFormatMetadata(self: PyOpenColorIO.MatrixTransform) -> PyOpenColorIO.FormatMetadata

         2. .. py:method:: getFormatMetadata(self: PyOpenColorIO.MatrixTransform) -> PyOpenColorIO.FormatMetadata

      .. py:method:: getMatrix(self: PyOpenColorIO.MatrixTransform) -> List[float[16]]

      .. py:method:: getOffset(self: PyOpenColorIO.MatrixTransform) -> List[float[4]]

      .. py:method:: setFileInputBitDepth(self: PyOpenColorIO.MatrixTransform, bitDepth: PyOpenColorIO.BitDepth) -> None

      .. py:method:: setFileOutputBitDepth(self: PyOpenColorIO.MatrixTransform, bitDepth: PyOpenColorIO.BitDepth) -> None

      .. py:method:: setMatrix(self: PyOpenColorIO.MatrixTransform, matrix: List[float[16]]) -> None

      .. py:method:: setOffset(self: PyOpenColorIO.MatrixTransform, offset: List[float[4]]) -> None


RangeTransform
==============

**class RangeTransform : public OpenColorIO::Transform**

Represents a range transform

The Range is used to apply an affine transform (scale & offset) and
clamps values to min/max bounds on all color components except the
alpha. The scale and offset values are computed from the input and
output bounds.

Refer to section 7.2.4 in specification S-2014-006 “A Common File
Format

for Look-Up Tables” from the Academy of Motion Picture Arts and
Sciences and the American Society of Cinematographers.

The “noClamp” style described in the specification S-2014-006
becomes a MatrixOp at the processor level.

.. tabs::

   .. group-tab:: C++

      .. cpp:function:: RangeStyle getStyle() const noexcept = 0

      .. cpp:function:: void setStyle(RangeStyle style) noexcept = 0

         Set the Range style to clamp or not input values.

      .. cpp:function:: const FormatMetadata &getFormatMetadata() const noexcept = 0
      

      .. cpp:function:: FormatMetadata &getFormatMetadata() noexcept = 0

      .. cpp:function:: bool equals(const RangeTransform &other) const noexcept = 0

         Checks if this equals other.

      .. cpp:function:: BitDepth getFileInputBitDepth() const noexcept = 0

      .. cpp:function:: void setFileInputBitDepth(BitDepth bitDepth) noexcept = 0

      .. cpp:function:: BitDepth getFileOutputBitDepth() const noexcept = 0

      .. cpp:function:: void setFileOutputBitDepth(BitDepth bitDepth) noexcept = 0

      .. cpp:function:: double getMinInValue() const noexcept = 0

         Get the minimum value for the input.

      .. cpp:function:: void setMinInValue(double val) noexcept = 0

         Set the minimum value for the input.

      .. cpp:function:: bool hasMinInValue() const noexcept = 0

         Is the minimum value for the input set?

      .. cpp:function:: void unsetMinInValue() noexcept = 0

         Unset the minimum value for the input.

      .. cpp:function:: void setMaxInValue(double val) noexcept = 0

         Set the maximum value for the input.

      .. cpp:function:: double getMaxInValue() const noexcept = 0

         Get the maximum value for the input.

      .. cpp:function:: bool hasMaxInValue() const noexcept = 0

         Is the maximum value for the input set?

      .. cpp:function:: void unsetMaxInValue() noexcept = 0

         Unset the maximum value for the input.

      .. cpp:function:: void setMinOutValue(double val) noexcept = 0

         Set the minimum value for the output.

      .. cpp:function:: double getMinOutValue() const noexcept = 0

         Get the minimum value for the output.

      .. cpp:function:: bool hasMinOutValue() const noexcept = 0

         Is the minimum value for the output set?

      .. cpp:function:: void unsetMinOutValue() noexcept = 0

         Unset the minimum value for the output.

      .. cpp:function:: void setMaxOutValue(double val) noexcept = 0

         Set the maximum value for the output.

      .. cpp:function:: double getMaxOutValue() const noexcept = 0

         Get the maximum value for the output.

      .. cpp:function:: bool hasMaxOutValue() const noexcept = 0

         Is the maximum value for the output set?

      .. cpp:function:: void unsetMaxOutValue() noexcept = 0

         Unset the maximum value for the output.

      .. cpp:function:: RangeTransform(const RangeTransform&) = delete

      .. cpp:function:: `RangeTransform`_ &operator=(const RangeTransform&) = delete

      .. cpp:function:: ~RangeTransform() = default

      -[ Public Static Functions ]-

      .. cpp:function:: RangeTransformRcPtr Create()

      Creates an instance of RangeTransform.

   .. group-tab:: Python

      .. py:method:: equals(self: PyOpenColorIO.RangeTransform, other: PyOpenColorIO.RangeTransform) -> bool

      .. py:method:: getFileInputBitDepth(self: PyOpenColorIO.RangeTransform) -> PyOpenColorIO.BitDepth

      .. py:method:: getFileOutputBitDepth(self: PyOpenColorIO.RangeTransform) -> PyOpenColorIO.BitDepth

      .. py:method:: getFormatMetadata(*args, **kwargs)**

         Overloaded function.

         1. .. py:method:: getFormatMetadata(self: PyOpenColorIO.RangeTransform) -> PyOpenColorIO.FormatMetadata

         2. .. py:method:: getFormatMetadata(self: PyOpenColorIO.RangeTransform) -> PyOpenColorIO.FormatMetadata

      .. py:method:: getMaxInValue(self: PyOpenColorIO.RangeTransform) -> float

      .. py:method:: getMaxOutValue(self: PyOpenColorIO.RangeTransform) -> float

      .. py:method:: getMinInValue(self: PyOpenColorIO.RangeTransform) -> float

      .. py:method:: getMinOutValue(self: PyOpenColorIO.RangeTransform) -> float

      .. py:method:: getStyle(self: PyOpenColorIO.RangeTransform) -> PyOpenColorIO.RangeStyle

      .. py:method:: hasMaxInValue(self: PyOpenColorIO.RangeTransform) -> bool

      .. py:method:: hasMaxOutValue(self: PyOpenColorIO.RangeTransform) -> bool

      .. py:method:: hasMinInValue(self: PyOpenColorIO.RangeTransform) -> bool

      .. py:method:: hasMinOutValue(self: PyOpenColorIO.RangeTransform) -> bool

      .. py:method:: setFileInputBitDepth(self: PyOpenColorIO.RangeTransform, bitDepth: PyOpenColorIO.BitDepth) -> None

      .. py:method:: setFileOutputBitDepth(self: PyOpenColorIO.RangeTransform, bitDepth: PyOpenColorIO.BitDepth) -> None

      .. py:method:: setMaxInValue(self: PyOpenColorIO.RangeTransform, value: float) -> None

      .. py:method:: setMaxOutValue(self: PyOpenColorIO.RangeTransform, value: float) -> None

      .. py:method:: setMinInValue(self: PyOpenColorIO.RangeTransform, value: float) -> None

      .. py:method:: setMinOutValue(self: PyOpenColorIO.RangeTransform, value: float) -> None

      .. py:method:: setStyle(self: PyOpenColorIO.RangeTransform, style: PyOpenColorIO.RangeStyle) -> None

      .. py:method:: unsetMaxOutValue(*args, **kwargs)

         Overloaded function.

         1. .. py:method:: unsetMaxOutValue(self: PyOpenColorIO.RangeTransform) -> None

         2. .. py:method:: unsetMaxOutValue(self: PyOpenColorIO.RangeTransform) -> None

      .. py:method:: unsetMinInValue(self: PyOpenColorIO.RangeTransform) -> None

      .. py:method:: unsetMinOutValue(self: PyOpenColorIO.RangeTransform) -> None

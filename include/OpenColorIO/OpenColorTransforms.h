// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_OPENCOLORTRANSFORMS_H
#define INCLUDED_OCIO_OPENCOLORTRANSFORMS_H

#include "OpenColorTypes.h"

#ifndef OCIO_NAMESPACE
#error This header cannot be used directly. Use <OpenColorIO/OpenColorIO.h> instead.
#endif

/*!rst::
C++ Transforms
==============

Typically only needed when creating and/or manipulating configurations
*/

namespace OCIO_NAMESPACE
{


//!rst:: //////////////////////////////////////////////////////////////////

//!cpp:class:: The FormatMetadata class is intended to be a generic
// container to hold metadata from various file formats.
//
// This class provides a hierarchical metadata container.
// A metadata object is similar to an element in XML.
// It contains:
//
// * A name string (e.g. "Description").
// * A value string (e.g. "updated viewing LUT").
// * A list of attributes (name, value) string pairs (e.g. "version", "1.5").
// * And a list of child sub-elements, which are also objects implementing
//   FormatMetadata.
//
class OCIOEXPORT FormatMetadata
{
public:
    //!cpp:function::
    virtual const char * getName() const = 0;
    //!cpp:function::
    virtual void setName(const char *) = 0;

    //!cpp:function::
    virtual const char * getValue() const = 0;
    //!cpp:function::
    virtual void setValue(const char *) = 0;

    //!cpp:function::
    virtual int getNumAttributes() const = 0;
    //!cpp:function::
    virtual const char * getAttributeName(int i) const = 0;
    //!cpp:function::
    virtual const char * getAttributeValue(int i) const = 0;
    //!cpp:function:: Add an attribute with a given name and value. If an
    // attribute with the same name already exists, the value is replaced.
    virtual void addAttribute(const char * name, const char * value) = 0;

    //!cpp:function::
    virtual int getNumChildrenElements() const = 0;
    //!cpp:function::
    virtual const FormatMetadata & getChildElement(int i) const = 0;
    //!cpp:function::
    virtual FormatMetadata & getChildElement(int i) = 0;

    //!cpp:function:: Add a child element with a given name and value. Name has to be
    // non-empty. Value may be empty, particularly if this element will have children.
    // Return a reference to the added element.
    virtual FormatMetadata & addChildElement(const char * name, const char * value) = 0;

    //!cpp:function::
    virtual void clear() = 0;
    //!cpp:function::
    virtual FormatMetadata & operator=(const FormatMetadata & rhs) = 0;

    //!cpp:function::
    FormatMetadata(const FormatMetadata & rhs) = delete;
    //!cpp:function::
    virtual ~FormatMetadata();

protected:
    FormatMetadata();
};

//!rst:: //////////////////////////////////////////////////////////////////

//!cpp:class:: Base class for all the transform classes
class OCIOEXPORT Transform
{
public:
    //!cpp:function::
    virtual TransformRcPtr createEditableCopy() const = 0;

    //!cpp:function::
    virtual TransformDirection getDirection() const noexcept = 0;
    //!cpp:function:: Note that this only affects the evaluation and not the values
    // stored in the object.
    virtual void setDirection(TransformDirection dir) noexcept = 0;

    //!cpp:function:: Will throw if data is not valid.
    virtual void validate() const;

    //!cpp:function::
    Transform(const Transform &) = delete;
    //!cpp:function::
    Transform & operator= (const Transform &) = delete;
    //!cpp:function::
    virtual ~Transform() = default;

protected:
    Transform() = default;
};

//!cpp:function::
extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const Transform&);


//!rst:: //////////////////////////////////////////////////////////////////

//!cpp:class:: Forward direction wraps the 'expanded' range into the
// specified, often compressed, range.
class OCIOEXPORT AllocationTransform : public Transform
{
public:
    //!cpp:function::
    static AllocationTransformRcPtr Create();

    //!cpp:function::
    TransformRcPtr createEditableCopy() const override;

    //!cpp:function::
    TransformDirection getDirection() const noexcept override;
    //!cpp:function::
    void setDirection(TransformDirection dir) noexcept override;

    //!cpp:function:: Will throw if data is not valid.
    void validate() const override;

    //!cpp:function::
    Allocation getAllocation() const;
    //!cpp:function::
    void setAllocation(Allocation allocation);

    //!cpp:function::
    int getNumVars() const;
    //!cpp:function::
    void getVars(float * vars) const;
    //!cpp:function::
    void setVars(int numvars, const float * vars);

    //!cpp:function::
    AllocationTransform & operator= (const AllocationTransform &) = delete;
    //!cpp:function::
    virtual ~AllocationTransform();

private:
    AllocationTransform();
    AllocationTransform(const AllocationTransform &);

    static void deleter(AllocationTransform * t);

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};

//!cpp:function::
extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const AllocationTransform&);


//!rst:: //////////////////////////////////////////////////////////////////

//!cpp:class:: An implementation of the ASC Color Decision List (CDL), based on the ASC v1.2
// specification.
//
// ​.. note::
//    If the config version is 1, negative values are clamped if the power is not 1.0.
//    For config version 2 and higher, the negative handling is controlled by the CDL style.
class OCIOEXPORT CDLTransform : public Transform
{
public:
    //!cpp:function::
    static CDLTransformRcPtr Create();

    //!cpp:function:: Load the CDL from the src .cc or .ccc file.
    // If a .ccc is used, the cccid must also be specified
    // src must be an absolute path reference, no relative directory
    // or envvar resolution is performed.
    static CDLTransformRcPtr CreateFromFile(const char * src, const char * cccid);

    //!cpp:function::
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;
    //!cpp:function::
    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;

    //!cpp:function::
    virtual bool equals(const CDLTransform & other) const noexcept = 0;

    //!cpp:function::
    virtual CDLStyle getStyle() const = 0;
    //!cpp:function:: Use CDL_ASC to clamp values to [0,1] per the ASC spec.  Use NO_CLAMP to
    // never clamp values (regardless of whether power is 1.0).  The NO_CLAMP option passes
    // negatives through unchanged (like the NEGATIVE_PASS_THRU style of ExponentTransform).
    // The default style is CDL_NO_CLAMP.
    virtual void setStyle(CDLStyle style) = 0;

    //!cpp:function::
    virtual const char * getXML() const = 0;
    //!cpp:function:: The default style is CDL_NO_CLAMP.
    virtual void setXML(const char * xml) = 0;

    //!rst:: **ASC_SOP**
    //
    // Slope, offset, power::
    //
    //    out = clamp( (in * slope) + offset ) ^ power

    //!cpp:function::
    virtual void getSlope(double * rgb) const = 0;
    //!cpp:function::
    virtual void setSlope(const double * rgb) = 0;

    //!cpp:function::
    virtual void getOffset(double * rgb) const = 0;
    //!cpp:function::
    virtual void setOffset(const double * rgb) = 0;

    //!cpp:function::
    virtual void getPower(double * rgb) const = 0;
    //!cpp:function::
    virtual void setPower(const double * rgb) = 0;

    //!cpp:function::
    virtual void getSOP(double * vec9) const = 0;
    //!cpp:function::
    virtual void setSOP(const double * vec9) = 0;

    //!rst:: **ASC_SAT**
    //

    //!cpp:function::
    virtual double getSat() const = 0;
    //!cpp:function::
    virtual void setSat(double sat) = 0;

    //!cpp:function:: These are hard-coded, by spec, to r709.
    virtual void getSatLumaCoefs(double * rgb) const = 0;

    //!rst:: **Metadata**
    //
    // These do not affect the image processing, but
    // are often useful for pipeline purposes and are
    // included in the serialization.

    //!cpp:function:: Unique Identifier for this correction.
    virtual const char * getID() const = 0;
    //!cpp:function::
    virtual void setID(const char * id) = 0;

    //!cpp:function:: Deprecated. Use `getFormatMetadata`.
    // First textual description of color correction (stored
    // on the SOP). If there is already a description, the setter will
    // replace it with the supplied text.
    virtual const char * getDescription() const = 0;
    //!cpp:function:: Deprecated. Use `getFormatMetadata`.
    virtual void setDescription(const char * desc) = 0;

    //!cpp:function::
    CDLTransform(const CDLTransform &) = delete;
    //!cpp:function::
    CDLTransform & operator= (const CDLTransform &) = delete;
    //!cpp:function::
    virtual ~CDLTransform() = default;

protected:
    CDLTransform() = default;
};

//!cpp:function::
extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const CDLTransform &);


//!rst:: //////////////////////////////////////////////////////////////////

//!cpp:class::
class OCIOEXPORT ColorSpaceTransform : public Transform
{
public:
    //!cpp:function::
    static ColorSpaceTransformRcPtr Create();

    //!cpp:function::
    TransformRcPtr createEditableCopy() const override;

    //!cpp:function::
    TransformDirection getDirection() const noexcept override;
    //!cpp:function::
    void setDirection(TransformDirection dir) noexcept override;

    //!cpp:function:: Will throw if data is not valid.
    void validate() const override;

    //!cpp:function::
    const char * getSrc() const;
    //!cpp:function::
    void setSrc(const char * src);

    //!cpp:function::
    const char * getDst() const;
    //!cpp:function::
    void setDst(const char * dst);

    //!cpp:function::
    ColorSpaceTransform & operator=(const ColorSpaceTransform &) = delete;
    //!cpp:function::
    virtual ~ColorSpaceTransform();

private:
    ColorSpaceTransform();
    ColorSpaceTransform(const ColorSpaceTransform &);

    static void deleter(ColorSpaceTransform * t);

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};

//!cpp:function::
extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const ColorSpaceTransform &);


//!rst:: //////////////////////////////////////////////////////////////////

//!cpp:class::
class OCIOEXPORT DisplayTransform : public Transform
{
public:
    //!cpp:function::
    static DisplayTransformRcPtr Create();

    //!cpp:function::
    TransformRcPtr createEditableCopy() const override;

    //!cpp:function::
    TransformDirection getDirection() const noexcept override;
    //!cpp:function::
    void setDirection(TransformDirection dir) noexcept override;

    //!cpp:function:: Will throw if data is not valid.
    void validate() const override;

    //!cpp:function::
    const char * getInputColorSpaceName() const;
    //!cpp:function:: Step 0. Specify the incoming color space.
    void setInputColorSpaceName(const char * name);

    //!cpp:function::
    ConstTransformRcPtr getLinearCC() const;
    //!cpp:function:: Step 1: Apply a Color Correction, in ROLE_SCENE_LINEAR.
    void setLinearCC(const ConstTransformRcPtr & cc);

    //!cpp:function::
    ConstTransformRcPtr getColorTimingCC() const;
    //!cpp:function:: Step 2: Apply a color correction, in ROLE_COLOR_TIMING.
    void setColorTimingCC(const ConstTransformRcPtr & cc);

    //!cpp:function::
    ConstTransformRcPtr getChannelView() const;
    //!cpp:function:: Step 3: Apply the Channel Viewing Swizzle (mtx).
    void setChannelView(const ConstTransformRcPtr & transform);

    //!cpp:function::
    const char * getDisplay() const;
    //!cpp:function:: Step 4: Apply the output display transform
    // This is controlled by the specification of (display, view)
    void setDisplay(const char * display);

    //!cpp:function::
    const char * getView() const;
    //!cpp:function:: Specify which view transform to use
    void setView(const char * view);

    //!cpp:function::
    ConstTransformRcPtr getDisplayCC() const;
    //!cpp:function:: Step 5: Apply a post display transform color correction
    void setDisplayCC(const ConstTransformRcPtr & cc);



    //!cpp:function::
    const char * getLooksOverride() const;
    //!cpp:function:: A user can optionally override the looks that are,
    // by default, used with the expected display / view combination.
    // A common use case for this functionality is in an image viewing
    // app, where per-shot looks are supported.  If for some reason
    // a per-shot look is not defined for the current Context, the
    // Config::getProcessor fcn will not succeed by default.  Thus,
    // with this mechanism the viewing app could override to looks = "",
    // and this will allow image display to continue (though hopefully)
    // the interface would reflect this fallback option.)
    //
    // Looks is a potentially comma (or colon) delimited list of lookNames,
    // Where +/- prefixes are optionally allowed to denote forward/inverse
    // look specification. (And forward is assumed in the absence of either)
    void setLooksOverride(const char * looks);

    //!cpp:function::
    bool getLooksOverrideEnabled() const;
    //!cpp:function:: Specify whether the lookOverride should be used,
    // or not. This is a separate flag, as it's often useful to override
    // "looks" to an empty string.
    void setLooksOverrideEnabled(bool enabled);

    //!cpp:function::
    DisplayTransform & operator=(const DisplayTransform &);
    //!cpp:function::
    virtual ~DisplayTransform();

private:
    DisplayTransform();
    DisplayTransform(const DisplayTransform &);

    static void deleter(DisplayTransform * t);

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};

//!cpp:function::
extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const DisplayTransform &);


//!rst:: //////////////////////////////////////////////////////////////////

//!cpp:class:: Allows transform parameter values to be set on-the-fly
// (after finalization).  For example, to modify the exposure in a viewport.
class OCIOEXPORT DynamicProperty
{
public:
    //!cpp:function::
    virtual DynamicPropertyType getType() const = 0;

    //!cpp:function::
    virtual DynamicPropertyValueType getValueType() const = 0;

    //!cpp:function::
    virtual double getDoubleValue() const = 0;
    //!cpp:function::
    virtual void setValue(double value) = 0;

    //!cpp:function::
    virtual bool isDynamic() const = 0;

    //!cpp:function::
    DynamicProperty & operator=(const DynamicProperty &) = delete;
    //!cpp:function::
    virtual ~DynamicProperty();

protected:
    DynamicProperty();
    DynamicProperty(const DynamicProperty &);
};

//!rst:: //////////////////////////////////////////////////////////////////

//!cpp:class:: Represents exponent transform: pow( clamp(color), value ).
//
// ​.. note::
//    For configs with version == 1: Negative style is ignored and if the exponent is 1.0,
//    this will not clamp. Otherwise, the input color will be clamped between [0.0, inf].
//    For configs with version > 1: Negative value handling may be specified via setNegativeStyle.
class OCIOEXPORT ExponentTransform : public Transform
{
public:
    //!cpp:function::
    static ExponentTransformRcPtr Create();

    //!cpp:function::
    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    //!cpp:function::
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    //!cpp:function:: Checks if this exactly equals other.
    virtual bool equals(const ExponentTransform & other) const noexcept = 0;

    //!cpp:function::
    virtual void getValue(double(&vec4)[4]) const noexcept = 0;
    //!cpp:function::
    virtual void setValue(const double(&vec4)[4]) noexcept = 0;

    //!cpp:function::  Specifies how negative values are handled. Legal values:
    //
    // * NEGATIVE_CLAMP -- Clamp negative values (default).
    // * NEGATIVE_MIRROR -- Positive curve is rotated 180 degrees around the origin to
    //                      handle negatives.
    // * NEGATIVE_PASS_THRU -- Negative values are passed through unchanged.
    virtual NegativeStyle getNegativeStyle() const = 0;
    //!cpp:function::
    virtual void setNegativeStyle(NegativeStyle style) = 0;
    
    //!cpp:function::
    ExponentTransform(const ExponentTransform &) = delete;
    //!cpp:function::
    ExponentTransform & operator= (const ExponentTransform &) = delete;
    //!cpp:function::
    virtual ~ExponentTransform() = default;

protected:
    ExponentTransform() = default;
};

//!cpp:function::
extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const ExponentTransform &);


//!rst:: //////////////////////////////////////////////////////////////////

//!cpp:class:: Represents power functions with a linear section in the shadows
// such as sRGB and L*.
//
// The basic formula is::
//
//   pow( (x + offset)/(1 + offset), gamma )
//   with the breakpoint at offset/(gamma - 1).
//
// Negative values are never clamped.
class OCIOEXPORT ExponentWithLinearTransform : public Transform
{
public:
    //!cpp:function::
    static ExponentWithLinearTransformRcPtr Create();

    //!cpp:function::
    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    //!cpp:function::
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    //!cpp:function:: Checks if this exactly equals other.
    virtual bool equals(const ExponentWithLinearTransform & other) const noexcept = 0;

    //!cpp:function::
    virtual void getGamma(double(&values)[4]) const noexcept = 0;
    //!cpp:function:: Set the exponent value for the power function for R, G, B, A.
    //
    // .. note::
    //    The gamma values must be in the range of [1, 10]. Set the transform direction
    //    to inverse to obtain the effect of values less than 1.
    virtual void setGamma(const double(&values)[4]) noexcept = 0;

    //!cpp:function::
    virtual void getOffset(double(&values)[4]) const noexcept = 0;
    //!cpp:function:: Set the offset value for the power function for R, G, B, A.
    //
    // .. note::
    //    The offset values must be in the range [0, 0.9].
    virtual void setOffset(const double(&values)[4]) noexcept = 0;

    //!cpp:function::  Specifies how negative values are handled. Legal values:
    //
    // * NEGATIVE_LINEAR -- Linear segment continues into negatives (default).
    // * NEGATIVE_MIRROR -- Positive curve is rotated 180 degrees around the origin to
    //                      handle negatives.
    virtual NegativeStyle getNegativeStyle() const = 0;
    //!cpp:function::
    virtual void setNegativeStyle(NegativeStyle style) = 0;
    
    //!cpp:function::
    ExponentWithLinearTransform(const ExponentWithLinearTransform &) = delete;
    //!cpp:function::
    ExponentWithLinearTransform & operator= (const ExponentWithLinearTransform &) = delete;
    //!cpp:function::
    virtual ~ExponentWithLinearTransform() = default;

protected:
    ExponentWithLinearTransform() = default;
};

//!cpp:function::
extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const ExponentWithLinearTransform &);

//!rst:: //////////////////////////////////////////////////////////////////

//!cpp:class:: Applies exposure, gamma, and pivoted contrast adjustments.
// Adjusts the math to be appropriate for linear, logarithmic, or video
// color spaces.
class OCIOEXPORT ExposureContrastTransform : public Transform
{
public:
    //!cpp:function::
    static ExposureContrastTransformRcPtr Create();

    //!cpp:function::
    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    //!cpp:function::
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    //!cpp:function:: Checks if this exactly equals other.
    virtual bool equals(const ExposureContrastTransform & other) const noexcept = 0;

    //!cpp:function::
    virtual ExposureContrastStyle getStyle() const = 0;
    //!cpp:function:: Select the algorithm for linear, video
    // or log color spaces.
    virtual void setStyle(ExposureContrastStyle style) = 0;

    //!cpp:function::
    virtual double getExposure() const = 0;
    //!cpp:function:: Applies an exposure adjustment.  The value is in
    // units of stops (regardless of style), for example, a value of -1
    // would be equivalent to reducing the lighting by one half.
    virtual void setExposure(double exposure) = 0;
    //!cpp:function::
    virtual bool isExposureDynamic() const = 0;
    //!cpp:function::
    virtual void makeExposureDynamic() = 0;

    //!cpp:function::
    virtual double getContrast() const = 0;
    //!cpp:function:: Applies a contrast/gamma adjustment around a pivot
    // point.  The contrast and gamma are mathematically the same, but two
    // controls are provided to enable the use of separate dynamic
    // parameters.  Contrast is usually a scene-referred adjustment that
    // pivots around gray whereas gamma is usually a display-referred
    // adjustment that pivots around white.
    virtual void setContrast(double contrast) = 0;
    //!cpp:function::
    virtual bool isContrastDynamic() const = 0;
    //!cpp:function::
    virtual void makeContrastDynamic() = 0;
    //!cpp:function::
    virtual double getGamma() const = 0;
    //!cpp:function::
    virtual void setGamma(double gamma) = 0;
    //!cpp:function::
    virtual bool isGammaDynamic() const = 0;
    //!cpp:function::
    virtual void makeGammaDynamic() = 0;

    //!cpp:function::
    virtual double getPivot() const = 0;
    //!cpp:function:: Set the pivot point around which the contrast
    // and gamma controls will work. Regardless of whether
    // linear/video/log-style is being used, the pivot is always expressed
    // in linear. In other words, a pivot of 0.18 is always mid-gray.
    virtual void setPivot(double pivot) = 0;

    //!cpp:function:: 
    virtual double getLogExposureStep() const = 0;
    //!cpp:function:: Set the increment needed to move one stop for
    // the log-style algorithm. For example, ACEScct is 0.057, LogC is
    // roughly 0.074, and Cineon is roughly 90/1023 = 0.088.
    // The default value is 0.088.
    virtual void setLogExposureStep(double logExposureStep) = 0;

    //!cpp:function:: 
    virtual double getLogMidGray() const = 0;
    //!cpp:function:: Set the position of 18% gray for use by the
    // log-style algorithm. For example, ACEScct is about 0.41, LogC is
    // about 0.39, and ADX10 is 445/1023 = 0.435.
    // The default value is 0.435.
    virtual void setLogMidGray(double logMidGray) = 0;

    virtual ~ExposureContrastTransform() = default;

protected:
    ExposureContrastTransform() = default;

private:
    ExposureContrastTransform(const ExposureContrastTransform &) = delete;
    ExposureContrastTransform & operator= (const ExposureContrastTransform &) = delete;
};

//!cpp:function::
extern OCIOEXPORT std::ostream & operator<<(std::ostream &,
                                            const ExposureContrastTransform &);

//!rst:: //////////////////////////////////////////////////////////////////

//!cpp:class::
class OCIOEXPORT FileTransform : public Transform
{
public:
    //!cpp:function::
    static FileTransformRcPtr Create();

    //!cpp:function::
    TransformRcPtr createEditableCopy() const override;

    //!cpp:function::
    TransformDirection getDirection() const noexcept override;
    //!cpp:function::
    void setDirection(TransformDirection dir) noexcept override;

    //!cpp:function:: Will throw if data is not valid.
    void validate() const override;

    //!cpp:function::
    const char * getSrc() const;
    //!cpp:function::
    void setSrc(const char * src);

    //!cpp:function::
    const char * getCCCId() const;
    //!cpp:function::
    void setCCCId(const char * id);

    //!cpp:function::
    CDLStyle getCDLStyle() const;
    //!cpp:function:: Can be used with CDL, CC & CCC formats to specify the clamping behavior of
    // the CDLTransform. Default is CDL_NO_CLAMP.
    void setCDLStyle(CDLStyle);

    //!cpp:function::
    Interpolation getInterpolation() const;
    //!cpp:function::
    void setInterpolation(Interpolation interp);

    //!cpp:function:: Get the number of LUT readers.
    static int getNumFormats();
    //!cpp:function:: Get the LUT readers at index, return empty string if
    // an invalid index is specified.
    static const char * getFormatNameByIndex(int index);

    //!cpp:function:: Get the LUT reader extension at index, return empty string if
    // an invalid index is specified.
    static const char * getFormatExtensionByIndex(int index);

    //!cpp:function::
    FileTransform & operator=(const FileTransform &) = delete;
    //!cpp:function::
    virtual ~FileTransform();

private:
    FileTransform();
    FileTransform(const FileTransform &);

    static void deleter(FileTransform * t);

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};

//!cpp:function::
extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const FileTransform &);


//!rst:: //////////////////////////////////////////////////////////////////

//!cpp:class:: Provides a set of hard-coded algorithmic building blocks
// that are needed to accurately implement various common color transformations.
class OCIOEXPORT FixedFunctionTransform : public Transform
{
public:
    //!cpp:function::
    static FixedFunctionTransformRcPtr Create();

    //!cpp:function::
    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    //!cpp:function::
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    //!cpp:function:: Checks if this exactly equals other.
    virtual bool equals(const FixedFunctionTransform & other) const noexcept = 0;

    //!cpp:function::
    virtual FixedFunctionStyle getStyle() const = 0;
    //!cpp:function:: Select which algorithm to use.
    virtual void setStyle(FixedFunctionStyle style) = 0;

    //!cpp:function::
    virtual size_t getNumParams() const = 0;
    //!cpp:function::
    virtual void getParams(double * params) const = 0;
    //!cpp:function:: Set the parameters (for functions that require them).
    virtual void setParams(const double * params, size_t num) = 0;

    //!cpp:function::
    FixedFunctionTransform(const FixedFunctionTransform &) = delete;
    //!cpp:function::
    FixedFunctionTransform & operator= (const FixedFunctionTransform &) = delete;
    //!cpp:function::
    virtual ~FixedFunctionTransform() = default;

protected:
    FixedFunctionTransform() = default;
};

//!cpp:function::
extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const FixedFunctionTransform &);


//!rst:: //////////////////////////////////////////////////////////////////

//!cpp:class::
class OCIOEXPORT GroupTransform : public Transform
{
public:
    //!cpp:function::
    static GroupTransformRcPtr Create();

    //!cpp:function::
    TransformRcPtr createEditableCopy() const override;

    //!cpp:function::
    TransformDirection getDirection() const noexcept override;
    //!cpp:function::
    void setDirection(TransformDirection dir) noexcept override;

    //!cpp:function:: Will throw if data is not valid.
    void validate() const override;

    //!cpp:function::
    virtual const FormatMetadata & getFormatMetadata() const noexcept;
    //!cpp:function::
    virtual FormatMetadata & getFormatMetadata() noexcept;

    //!cpp:function::
    ConstTransformRcPtr getTransform(int index) const;

    //!cpp:function::
    TransformRcPtr & getTransform(int index);

    //!cpp:function::
    int getNumTransforms() const;
    //!cpp:function:: Adds a transform to the end of the group.
    void appendTransform(TransformRcPtr transform);
    //!cpp:function:: Add a transform at the beginning of the group.
    void prependTransform(TransformRcPtr transform);

    //!cpp:function::
    GroupTransform & operator=(const GroupTransform &) = delete;
    //!cpp:function::
    virtual ~GroupTransform();

private:
    GroupTransform();
    GroupTransform(const GroupTransform &);

    static void deleter(GroupTransform * t);

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};

//!cpp:function::
extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const GroupTransform &);


//!rst:: //////////////////////////////////////////////////////////////////

//!cpp:class::  Applies a logarithm with an affine transform before and after.
// Represents the Cineon lin-to-log type transforms::
//
//   logSideSlope * log( linSideSlope * color + linSideOffset, base) + logSideOffset
//
// * Default values are: 1. * log( 1. * color + 0., 2.) + 0.
// * The alpha channel is not affected.
//
class OCIOEXPORT LogAffineTransform : public Transform
{
public:
    //!cpp:function::
    static LogAffineTransformRcPtr Create();

    //!cpp:function::
    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    //!cpp:function::
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    //!cpp:function:: Checks if this exactly equals other.
    virtual bool equals(const LogAffineTransform & other) const noexcept = 0;

    //!cpp:function::
    virtual double getBase() const noexcept = 0;
    //!cpp:function::
    virtual void setBase(double base) noexcept = 0;

    //!rst:: **Get/Set values for the R, G, B components**
    //

    //!cpp:function::
    virtual void getLogSideSlopeValue(double(&values)[3]) const noexcept = 0;
    //!cpp:function::
    virtual void setLogSideSlopeValue(const double(&values)[3]) noexcept = 0;
    //!cpp:function::
    virtual void getLogSideOffsetValue(double(&values)[3]) const noexcept = 0;
    //!cpp:function::
    virtual void setLogSideOffsetValue(const double(&values)[3]) noexcept = 0;
    //!cpp:function::
    virtual void getLinSideSlopeValue(double(&values)[3]) const noexcept = 0;
    //!cpp:function::
    virtual void setLinSideSlopeValue(const double(&values)[3]) noexcept = 0;
    //!cpp:function::
    virtual void getLinSideOffsetValue(double(&values)[3]) const noexcept = 0;
    //!cpp:function::
    virtual void setLinSideOffsetValue(const double(&values)[3]) noexcept = 0;

    //!cpp:function::
    LogAffineTransform(const LogAffineTransform &) = delete;
    //!cpp:function::
    LogAffineTransform & operator= (const LogAffineTransform &) = delete;
    //!cpp:function::
    virtual ~LogAffineTransform() = default;

protected:
    LogAffineTransform() = default;
};

//!cpp:function::
extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const LogAffineTransform &);


//!rst:: //////////////////////////////////////////////////////////////////

//!cpp:class::  Same as :cpp:class:`LogAffineTransform` but with the addition of a linear segment
// near black. This formula is used for many camera logs (e.g., LogC) as well as ACEScct.
//
// * The linSideBreak specifies the point on the linear axis where the log and linear
//   segments meet.  It must be set (there is no default).  
// * The linearSlope specifies the slope of the linear segment of the forward (linToLog)
//   transform.  By default it is set equal to the slope of the log curve at the break point.
//
class OCIOEXPORT LogCameraTransform : public Transform
{
public:
    //!cpp:function::
    static LogCameraTransformRcPtr Create();

    //!cpp:function::
    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    //!cpp:function::
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    //!cpp:function:: Checks if this exactly equals other.
    virtual bool equals(const LogCameraTransform & other) const noexcept = 0;

    //!cpp:function::
    virtual double getBase() const noexcept = 0;
    //!cpp:function::
    virtual void setBase(double base) noexcept = 0;

    //!rst:: **Get/Set values for the R, G, B components**
    //

    //!cpp:function::
    virtual void getLogSideSlopeValue(double(&values)[3]) const noexcept = 0;
    //!cpp:function::
    virtual void setLogSideSlopeValue(const double(&values)[3]) noexcept = 0;
    //!cpp:function::
    virtual void getLogSideOffsetValue(double(&values)[3]) const noexcept = 0;
    //!cpp:function::
    virtual void setLogSideOffsetValue(const double(&values)[3]) noexcept = 0;
    //!cpp:function::
    virtual void getLinSideSlopeValue(double(&values)[3]) const noexcept = 0;
    //!cpp:function::
    virtual void setLinSideSlopeValue(const double(&values)[3]) noexcept = 0;
    //!cpp:function::
    virtual void getLinSideOffsetValue(double(&values)[3]) const noexcept = 0;
    //!cpp:function::
    virtual void setLinSideOffsetValue(const double(&values)[3]) noexcept = 0;

    //!cpp:function:: Return true if LinSideBreak values were set, false if they were not.
    virtual bool getLinSideBreakValue(double(&values)[3]) const noexcept = 0;
    //!cpp:function::
    virtual void setLinSideBreakValue(const double(&values)[3]) noexcept = 0;

    //!cpp:function:: Return true if LinearSlope values were set, false if they were not.
    virtual bool getLinearSlopeValue(double(&values)[3]) const = 0;
    //!cpp:function:: Set LinearSlope value.
    // Note: You must call setLinSideBreakValue before calling this.
    virtual void setLinearSlopeValue(const double(&values)[3]) = 0;
    //!cpp:function:: Remove LinearSlope values so that default values are used.
    virtual void unsetLinearSlopeValue() = 0;
    
    //!cpp:function::
    LogCameraTransform(const LogCameraTransform &) = delete;
    //!cpp:function::
    LogCameraTransform & operator= (const LogCameraTransform &) = delete;
    //!cpp:function::
    virtual ~LogCameraTransform() = default;

protected:
    LogCameraTransform() = default;
};

//!cpp:function::
extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const LogCameraTransform &);


//!rst:: //////////////////////////////////////////////////////////////////

//!cpp:class:: Represents log transform: log(color, base)
//
// * The input will be clamped for negative numbers.
// * Default base is 2.0.
// * The alpha channel is not affected.
class OCIOEXPORT LogTransform : public Transform
{
public:
    //!cpp:function::
    static LogTransformRcPtr Create();

    //!cpp:function::
    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    //!cpp:function::
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    //!cpp:function:: Checks if this exactly equals other.
    virtual bool equals(const LogTransform & other) const noexcept = 0;

    //!cpp:function::
    virtual double getBase() const noexcept = 0;
    //!cpp:function::
    virtual void setBase(double val) noexcept = 0;

    //!cpp:function::
    LogTransform(const LogTransform &) = delete;
    //!cpp:function::
    LogTransform & operator= (const LogTransform &) = delete;
    //!cpp:function::
    virtual ~LogTransform() = default;

protected:
    LogTransform() = default;
};

//!cpp:function::
extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const LogTransform &);


//!rst:: //////////////////////////////////////////////////////////////////

//!cpp:class::
class OCIOEXPORT LookTransform : public Transform
{
public:
    //!cpp:function::
    static LookTransformRcPtr Create();

    //!cpp:function::
    TransformRcPtr createEditableCopy() const override;

    //!cpp:function::
    TransformDirection getDirection() const noexcept override;
    //!cpp:function::
    void setDirection(TransformDirection dir) noexcept override;

    //!cpp:function:: Will throw if data is not valid.
    void validate() const override;

    //!cpp:function::
    const char * getSrc() const;
    //!cpp:function::
    void setSrc(const char * src);

    //!cpp:function::
    const char * getDst() const;
    //!cpp:function::
    void setDst(const char * dst);

    //!cpp:function::
    const char * getLooks() const;
    //!cpp:function:: Specify looks to apply.
    // Looks is a potentially comma (or colon) delimited list of look names,
    // Where +/- prefixes are optionally allowed to denote forward/inverse
    // look specification. (And forward is assumed in the absence of either)
    void setLooks(const char * looks);

    //!cpp:function::
    LookTransform & operator=(const LookTransform &) = delete;
    //!cpp:function::
    virtual ~LookTransform();

private:
    LookTransform();
    LookTransform(const LookTransform &);

    static void deleter(LookTransform * t);

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};

//!cpp:function::
extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const LookTransform &);


//!rst:: //////////////////////////////////////////////////////////////////

//!cpp:class:: Represents a 1D-LUT transform.
class OCIOEXPORT Lut1DTransform : public Transform
{
public:
    //!cpp:function:: Create an identity 1D-LUT of length two.
    static Lut1DTransformRcPtr Create();

    //!cpp:function:: Create an identity 1D-LUT with specific length and
    // half-domain setting. Will throw for lengths longer than 1024x1024.
    static Lut1DTransformRcPtr Create(unsigned long length,
                                      bool isHalfDomain);

    //!cpp:function::
    virtual BitDepth getFileOutputBitDepth() const noexcept = 0;
    //!cpp:function:: Get the bit-depth associated with the LUT values read
    // from a file or set the bit-depth of values to be written to a file
    // (for file formats such as CLF that support multiple bit-depths).
    // However, note that the values stored in the object are always
    // normalized.
    virtual void setFileOutputBitDepth(BitDepth bitDepth) noexcept = 0;

    //!cpp:function::
    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    //!cpp:function::
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    //!cpp:function:: Checks if this exactly equals other.
    virtual bool equals(const Lut1DTransform & other) const noexcept = 0;

    //!cpp:function::
    virtual unsigned long getLength() const = 0;
    //!cpp:function:: Changing the length will reset the LUT to identity.
    // Will throw for lengths longer than 1024x1024.
    virtual void setLength(unsigned long length) = 0;

    //!cpp:function::
    virtual void getValue(unsigned long index, float & r, float & g, float & b) const = 0;
    //!cpp:function:: Set the values of a LUT1D.  Will throw if the index
    // is outside of the range from 0 to (length-1).
    //
    // The LUT values are always for the "forward" LUT, regardless of how
    // the transform direction is set.
    //
    // These values are normalized relative to what may be stored in any
    // given LUT files. For example in a CLF file using a "10i" output
    // depth, a value of 1023 in the file is normalized to 1.0. The
    // values here are unclamped and may extend outside [0,1].
    //
    // LUTs in various file formats may only provide values for one
    // channel where R, G, B are the same. Even in that case, you should
    // provide three equal values to the setter.
    virtual void setValue(unsigned long index, float r, float g, float b) = 0;

    //!cpp:function::
    virtual bool getInputHalfDomain() const noexcept = 0;
    //!cpp:function:: In a half-domain LUT, the contents of the LUT specify
    // the desired value of the function for each half-float value.
    // Therefore, the length of the LUT must be 65536 entries or else
    // validate() will throw.
    virtual void setInputHalfDomain(bool isHalfDomain) noexcept = 0;

    //!cpp:function::
    virtual bool getOutputRawHalfs() const noexcept = 0;
    //!cpp:function:: Set OutputRawHalfs to true if you want to output the
    // LUT contents as 16-bit floating point values expressed as unsigned
    // 16-bit integers representing the equivalent bit pattern.
    // For example, the value 1.0 would be written as the integer 15360
    // because it has the same bit-pattern.  Note that this implies the
    // values will be quantized to a 16-bit float.  Note that this setting
    // only controls the output formatting (where supported) and not the 
    // values for getValue/setValue.  The only file formats that currently
    // support this are CLF and CTF.
    virtual void setOutputRawHalfs(bool isRawHalfs) noexcept = 0;

    //!cpp:function::
    virtual Lut1DHueAdjust getHueAdjust() const noexcept = 0;
    //!cpp:function:: The 1D-LUT transform optionally supports a hue adjustment
    // feature that was used in some versions of ACES.  This adjusts the hue
    // of the result to approximately match the input.
    virtual void setHueAdjust(Lut1DHueAdjust algo) noexcept = 0;

    //!cpp:function::
    virtual Interpolation getInterpolation() const = 0;
    //!cpp:function::
    virtual void setInterpolation(Interpolation algo) = 0;

    //!cpp:function::
    Lut1DTransform(const Lut1DTransform &) = delete;
    //!cpp:function::
    Lut1DTransform & operator= (const Lut1DTransform &) = delete;
    //!cpp:function::
    virtual ~Lut1DTransform() = default;

protected:
    Lut1DTransform() = default;
};

//!cpp:function::
extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const Lut1DTransform&);

//!rst:: //////////////////////////////////////////////////////////////////

//!cpp:class:: Represents a 3D-LUT transform.
class OCIOEXPORT Lut3DTransform : public Transform
{
public:
    //!cpp:function:: Create an identity 3D-LUT of size 2x2x2.
    static Lut3DTransformRcPtr Create();

    //!cpp:function:: Create an identity 3D-LUT with specific grid size.
    // Will throw for grid size larger than 129.
    static Lut3DTransformRcPtr Create(unsigned long gridSize);

    //!cpp:function::
    virtual BitDepth getFileOutputBitDepth() const noexcept = 0;
    //!cpp:function:: Get the bit-depth associated with the LUT values read
    // from a file or set the bit-depth of values to be written to a file
    // (for file formats such as CLF that support multiple bit-depths).
    // However, note that the values stored in the object are always
    // normalized.
    virtual void setFileOutputBitDepth(BitDepth bitDepth) noexcept = 0;

    //!cpp:function::
    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    //!cpp:function::
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    //!cpp:function:: Checks if this exactly equals other.
    virtual bool equals(const Lut3DTransform & other) const noexcept = 0;

    //!cpp:function::
    virtual unsigned long getGridSize() const = 0;
    //!cpp:function:: Changing the grid size will reset the LUT to identity.
    // Will throw for grid sizes larger than 129.
    virtual void setGridSize(unsigned long gridSize) = 0;

    //!cpp:function::
    virtual void getValue(unsigned long indexR,
                          unsigned long indexG,
                          unsigned long indexB,
                          float & r, float & g, float & b) const = 0;
    //!cpp:function:: Set the values of a 3D-LUT. Will throw if an index is
    // outside of the range from 0 to (gridSize-1).
    //
    // The LUT values are always for the "forward" LUT, regardless of how the
    // transform direction is set.
    //
    // These values are normalized relative to what may be stored in any
    // given LUT files. For example in a CLF file using a "10i" output
    // depth, a value of 1023 in the file is normalized to 1.0. The values
    // here are unclamped and may extend outside [0,1].
    virtual void setValue(unsigned long indexR,
                          unsigned long indexG,
                          unsigned long indexB,
                          float r, float g, float b) = 0;

    //!cpp:function::
    virtual Interpolation getInterpolation() const = 0;
    //!cpp:function::
    virtual void setInterpolation(Interpolation algo) = 0;

    //!cpp:function::
    Lut3DTransform(const Lut3DTransform &) = delete;
    //!cpp:function::
    Lut3DTransform & operator= (const Lut3DTransform &) = delete;
    //!cpp:function::
    virtual ~Lut3DTransform() = default;

protected:
    Lut3DTransform() = default;
};

//!cpp:function::
extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const Lut3DTransform&);

//!rst:: //////////////////////////////////////////////////////////////////

//!cpp:class:: Represents an MX+B Matrix transform.
//
// .. note::
//    For singular matrices, an inverse direction will throw an exception during finalization.
class OCIOEXPORT MatrixTransform : public Transform
{
public:
    //!cpp:function::
    static MatrixTransformRcPtr Create();

    //!cpp:function::
    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    //!cpp:function::
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    //!cpp:function:: Checks if this exactly equals other.
    virtual bool equals(const MatrixTransform & other) const noexcept = 0;

    //!cpp:function::
    virtual void getMatrix(double * m44) const = 0;
    //!cpp:function:: Get or set the values of a Matrix. Expects 16 values,
    // where the first four are the coefficients to generate the R output
    // channel from R, G, B, A input channels.
    //
    // The Matrix values are always for the "forward" Matrix, regardless of
    // how the transform direction is set.
    //
    // These values are normalized relative to what may be stored in
    // file formats such as CLF. For example in a CLF file using a "32f"
    // input depth and "10i" output depth, a value of 1023 in the file
    // is normalized to 1.0. The values here are unclamped and may
    // extend outside [0,1].
    virtual void setMatrix(const double * m44) = 0;

    //!cpp:function::
    virtual void getOffset(double * offset4) const = 0;
    //!cpp:function:: Get or set the R, G, B, A offsets to be applied
    // after the matrix.
    //
    // These values are normalized relative to what may be stored in
    // file formats such as CLF. For example, in a CLF file using a
    // "10i" output depth, a value of 1023 in the file is normalized
    // to 1.0. The values here are unclamped and may extend
    // outside [0,1].
    virtual void setOffset(const double * offset4) = 0;

    //!rst:: **File bit-depth**
    //
    // Get the bit-depths associated with the matrix values read from a
    // file or set the bit-depths of values to be written to a file
    // (for file formats such as CLF that support multiple bit-depths).
    //
    // In a format such as CLF, the matrix values are scaled to take
    // pixels at the specified inBitDepth to pixels at the specified
    // outBitDepth.  This complicates the interpretation of the matrix
    // values and so this object always holds normalized values and
    // scaling is done on the way from or to file formats such as CLF.

    //!cpp:function::
    virtual BitDepth getFileInputBitDepth() const noexcept = 0;
    //!cpp:function::
    virtual void setFileInputBitDepth(BitDepth bitDepth) noexcept = 0;
    //!cpp:function::
    virtual BitDepth getFileOutputBitDepth() const noexcept = 0;
    //!cpp:function::
    virtual void setFileOutputBitDepth(BitDepth bitDepth) noexcept = 0;

    //!rst:: **Convenience functions**
    //
    // Build the matrix and offset corresponding to higher-level concepts.
    // 
    // .. note::
    //    These can throw an exception if for any component
    //    ``oldmin == oldmax. (divide by 0)``

    //!cpp:function::
    static void Fit(double * m44, double* offset4,
                    const double * oldmin4, const double * oldmax4,
                    const double * newmin4, const double * newmax4);

    //!cpp:function::
    static void Identity(double * m44, double * offset4);

    //!cpp:function::
    static void Sat(double * m44, double * offset4,
                    double sat, const double * lumaCoef3);

    //!cpp:function::
    static void Scale(double * m44, double * offset4,
                      const double * scale4);

    //!cpp:function::
    static void View(double * m44, double * offset4,
                     int * channelHot4,
                     const double * lumaCoef3);

    //!cpp:function::
    MatrixTransform(const MatrixTransform &) = delete;
    //!cpp:function::
    MatrixTransform & operator= (const MatrixTransform &) = delete;
    //!cpp:function::
    virtual ~MatrixTransform() = default;

protected:
    MatrixTransform() = default;
};

//!cpp:function::
extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const MatrixTransform &) noexcept;

//!rst:: //////////////////////////////////////////////////////////////////

//!cpp:class:: Represents a range transform
//
// The Range is used to apply an affine transform (scale & offset) and
// clamps values to min/max bounds on all color components except the alpha.
// The scale and offset values are computed from the input and output bounds.
// 
// Refer to section 7.2.4 in specification S-2014-006 "A Common File Format
// for Look-Up Tables" from the Academy of Motion Picture Arts and Sciences
// and the American Society of Cinematographers.
//
// The "noClamp" style described in the specification S-2014-006 becomes a
// MatrixOp at the processor level.
class OCIOEXPORT RangeTransform : public Transform
{
public:
    //!cpp:function:: Creates an instance of RangeTransform.
    static RangeTransformRcPtr Create();

    //!cpp:function::
    virtual RangeStyle getStyle() const noexcept = 0;
    //!cpp:function:: Set the Range style to clamp or not input values.
    virtual void setStyle(RangeStyle style) noexcept = 0;

    //!cpp:function::
    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    //!cpp:function::
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    //!cpp:function:: Checks if this equals other.
    virtual bool equals(const RangeTransform & other) const noexcept = 0;

    //!rst:: **File bit-depth**
    //
    // Get the bit-depths associated with the range values read from a file
    // or set the bit-depths of values to be written to a file (for file
    // formats such as CLF that support multiple bit-depths).
    //
    // In a format such as CLF, the range values are scaled to take
    // pixels at the specified inBitDepth to pixels at the specified
    // outBitDepth. This complicates the interpretation of the range
    // values and so this object always holds normalized values and
    // scaling is done on the way from or to file formats such as CLF.

    //!cpp:function::
    virtual BitDepth getFileInputBitDepth() const noexcept = 0;
    //!cpp:function::
    virtual void setFileInputBitDepth(BitDepth bitDepth) noexcept = 0;
    //!cpp:function::
    virtual BitDepth getFileOutputBitDepth() const noexcept = 0;
    //!cpp:function::
    virtual void setFileOutputBitDepth(BitDepth bitDepth) noexcept = 0;

    //!rst:: **Range values**
    //
    // Note that changing the transform direction does not modify the
    // in/out values, they are always for the "forward" direction.
    //
    // These values are normalized relative to what may be stored in file
    // formats such as CLF. For example in a CLF file using a "10i" input
    // depth, a MaxInValue of 1023 in the file is normalized to 1.0.
    // Likewise, for an output depth of "12i", a MaxOutValue of 4095 in the
    // file is normalized to 1.0. The values here are unclamped and may
    // extend outside [0,1].

    //!cpp:function:: Get the minimum value for the input.
    virtual double getMinInValue() const noexcept = 0;
    //!cpp:function:: Set the minimum value for the input.
    virtual void setMinInValue(double val) noexcept = 0;
    //!cpp:function:: Is the minimum value for the input set?
    virtual bool hasMinInValue() const noexcept = 0;
    //!cpp:function:: Unset the minimum value for the input
    virtual void unsetMinInValue() noexcept = 0;

    //!cpp:function:: Set the maximum value for the input.
    virtual void setMaxInValue(double val) noexcept = 0;
    //!cpp:function:: Get the maximum value for the input.
    virtual double getMaxInValue() const noexcept = 0;
    //!cpp:function:: Is the maximum value for the input set?
    virtual bool hasMaxInValue() const noexcept = 0;
    //!cpp:function:: Unset the maximum value for the input.
    virtual void unsetMaxInValue() noexcept = 0;

    //!cpp:function:: Set the minimum value for the output.
    virtual void setMinOutValue(double val) noexcept = 0;
    //!cpp:function:: Get the minimum value for the output.
    virtual double getMinOutValue() const noexcept = 0;
    //!cpp:function:: Is the minimum value for the output set?
    virtual bool hasMinOutValue() const noexcept = 0;
    //!cpp:function:: Unset the minimum value for the output.
    virtual void unsetMinOutValue() noexcept = 0;

    //!cpp:function:: Set the maximum value for the output.
    virtual void setMaxOutValue(double val) noexcept = 0;
    //!cpp:function:: Get the maximum value for the output.
    virtual double getMaxOutValue() const noexcept = 0;
    //!cpp:function:: Is the maximum value for the output set?
    virtual bool hasMaxOutValue() const noexcept = 0;
    //!cpp:function:: Unset the maximum value for the output.
    virtual void unsetMaxOutValue() noexcept = 0;

    //!cpp:function::
    RangeTransform(const RangeTransform &) = delete;
    //!cpp:function::
    RangeTransform & operator= (const RangeTransform &) = delete;
    //!cpp:function::
    virtual ~RangeTransform() = default;

protected:
    RangeTransform() = default;
};

//!cpp:function::
extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const RangeTransform &) noexcept;

} // namespace OCIO_NAMESPACE

#endif

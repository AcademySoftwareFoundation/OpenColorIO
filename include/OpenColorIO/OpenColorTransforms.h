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



/**
 * The FormatMetadata class is intended to be a generic
 * container to hold metadata from various file formats.
 *
 * This class provides a hierarchical metadata container.
 * A metadata object is similar to an element in XML.
 * It contains:
 *
 * * A name string (e.g. "Description").
 * * A value string (e.g. "updated viewing LUT").
 * * A list of attributes (name, value) string pairs (e.g. "version", "1.5").
 * * And a list of child sub-elements, which are also objects implementing
 *   FormatMetadata.
 */
class OCIOEXPORT FormatMetadata
{
public:
    virtual const char * getName() const = 0;
    virtual void setName(const char *) = 0;

    virtual const char * getValue() const = 0;
    virtual void setValue(const char *) = 0;

    virtual int getNumAttributes() const = 0;
    virtual const char * getAttributeName(int i) const = 0;
    virtual const char * getAttributeValue(int i) const = 0;
    /**
     * Add an attribute with a given name and value. If an
     * attribute with the same name already exists, the value is replaced.
     */
    virtual void addAttribute(const char * name, const char * value) = 0;

    virtual int getNumChildrenElements() const = 0;
    virtual const FormatMetadata & getChildElement(int i) const = 0;
    virtual FormatMetadata & getChildElement(int i) = 0;

    /**
     * Add a child element with a given name and value. Name has to be
     * non-empty. Value may be empty, particularly if this element will have children.
     * Return a reference to the added element.
     */
    virtual FormatMetadata & addChildElement(const char * name, const char * value) = 0;

    virtual void clear() = 0;
    virtual FormatMetadata & operator=(const FormatMetadata & rhs) = 0;

    FormatMetadata(const FormatMetadata & rhs) = delete;
    virtual ~FormatMetadata();

protected:
    FormatMetadata();
};


/// Base class for all the transform classes
class OCIOEXPORT Transform
{
public:
    virtual TransformRcPtr createEditableCopy() const = 0;

    virtual TransformDirection getDirection() const noexcept = 0;
    /**
     * Note that this only affects the evaluation and not the values
     * stored in the object.
     */
    virtual void setDirection(TransformDirection dir) noexcept = 0;

    /// Will throw if data is not valid.
    virtual void validate() const;

    Transform(const Transform &) = delete;
    Transform & operator= (const Transform &) = delete;
    virtual ~Transform() = default;

protected:
    Transform() = default;
};

extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const Transform&);



/**
 * Forward direction wraps the 'expanded' range into the
 * specified, often compressed, range.
 */
class OCIOEXPORT AllocationTransform : public Transform
{
public:
    static AllocationTransformRcPtr Create();

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    /// Will throw if data is not valid.
    void validate() const override;

    Allocation getAllocation() const;
    void setAllocation(Allocation allocation);

    int getNumVars() const;
    void getVars(float * vars) const;
    void setVars(int numvars, const float * vars);

    AllocationTransform & operator= (const AllocationTransform &) = delete;
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

extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const AllocationTransform&);


/**
 * A built-in transform is similar to a FileTransform, but without the file.
 * OCIO knows how to build a set of commonly used transforms on-demand, thus avoiding the need
 * for external files and simplifying config authoring.
 */
class OCIOEXPORT BuiltinTransform : public Transform
{
public:
    static BuiltinTransformRcPtr Create();

    virtual const char * getStyle() const noexcept = 0;
    /**
     * Select an existing built-in transform style from the list accessible
     * through :cpp:class:`BuiltinTransformRegistry`. The style is the ID string that identifies
     * which transform to apply.
     */
    virtual void setStyle(const char * style) = 0;

    virtual const char * getDescription() const noexcept = 0;

    // Do not use (needed only for pybind11).
    virtual ~BuiltinTransform() = default;

protected:
    BuiltinTransform() = default;

private:
    BuiltinTransform(const BuiltinTransform &) = delete;
    BuiltinTransform & operator= (const BuiltinTransform &) = delete;
};

//
extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const BuiltinTransform &) noexcept;


/**
 * An implementation of the ASC Color Decision List (CDL), based on the ASC v1.2
 * specification.
 *
 * \note​
 *    If the config version is 1, negative values are clamped if the power is not 1.0.
 *    For config version 2 and higher, the negative handling is controlled by the CDL style.
 */
class OCIOEXPORT CDLTransform : public Transform
{
public:
    static CDLTransformRcPtr Create();

    /**
     * Load the CDL from the src .cc or .ccc file.
     * If a .ccc is used, the cccid must also be specified
     * src must be an absolute path reference, no relative directory
     * or envvar resolution is performed.
     */
    static CDLTransformRcPtr CreateFromFile(const char * src, const char * cccid);

    virtual FormatMetadata & getFormatMetadata() noexcept = 0;
    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;

    virtual bool equals(const CDLTransform & other) const noexcept = 0;

    virtual CDLStyle getStyle() const = 0;
    /**
     * Use CDL_ASC to clamp values to [0,1] per the ASC spec.  Use NO_CLAMP to
     * never clamp values (regardless of whether power is 1.0).  The NO_CLAMP option passes
     * negatives through unchanged (like the NEGATIVE_PASS_THRU style of ExponentTransform).
     * The default style is CDL_NO_CLAMP.
     */
    virtual void setStyle(CDLStyle style) = 0;

    virtual const char * getXML() const = 0;
    /// The default style is CDL_NO_CLAMP.
    virtual void setXML(const char * xml) = 0;

    // TODO: Move to .rst
    // !rst:: **ASC_SOP**
    //
    // Slope, offset, power::
    //
    //    out = clamp( (in * slope) + offset ) ^ power

    virtual void getSlope(double * rgb) const = 0;
    virtual void setSlope(const double * rgb) = 0;

    virtual void getOffset(double * rgb) const = 0;
    virtual void setOffset(const double * rgb) = 0;

    virtual void getPower(double * rgb) const = 0;
    virtual void setPower(const double * rgb) = 0;

    virtual void getSOP(double * vec9) const = 0;
    virtual void setSOP(const double * vec9) = 0;

    virtual double getSat() const = 0;
    virtual void setSat(double sat) = 0;

    /// These are hard-coded, by spec, to r709.
    virtual void getSatLumaCoefs(double * rgb) const = 0;

    // TODO: Move to .rst
    //!rst:: **Metadata**
    //
    // These do not affect the image processing, but
    // are often useful for pipeline purposes and are
    // included in the serialization.

    /// Unique Identifier for this correction.
    virtual const char * getID() const = 0;
    virtual void setID(const char * id) = 0;

    /**
     * Deprecated. Use `getFormatMetadata`.
     * First textual description of color correction (stored
     * on the SOP). If there is already a description, the setter will
     * replace it with the supplied text.
     */
    virtual const char * getDescription() const = 0;
    /// Deprecated. Use `getFormatMetadata`.
    virtual void setDescription(const char * desc) = 0;

    CDLTransform(const CDLTransform &) = delete;
    CDLTransform & operator= (const CDLTransform &) = delete;
    virtual ~CDLTransform() = default;

protected:
    CDLTransform() = default;
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const CDLTransform &);


class OCIOEXPORT ColorSpaceTransform : public Transform
{
public:
    static ColorSpaceTransformRcPtr Create();

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    void validate() const override;

    const char * getSrc() const;
    void setSrc(const char * src);

    const char * getDst() const;
    void setDst(const char * dst);

    /// Data color spaces do not get processed when true (which is the default).
    bool getDataBypass() const noexcept;
    void setDataBypass(bool enabled) noexcept;

    ColorSpaceTransform & operator=(const ColorSpaceTransform &) = delete;
    // Do not use (needed only for pybind11).
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

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const ColorSpaceTransform &);


class OCIOEXPORT DisplayViewTransform : public Transform
{
public:
    static DisplayViewTransformRcPtr Create();

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    /// Will throw if data is not valid.
    void validate() const override;

    const char * getSrc() const;
    /// Specify the incoming color space.
    void setSrc(const char * name);

    const char * getDisplay() const;
    /// Specify which display to use.
    void setDisplay(const char * display);

    const char * getView() const;
    /// Specify which view transform to use.
    void setView(const char * view);

    bool getLooksBypass() const;
    /// Looks will be bypassed when true (the default is false).
    void setLooksBypass(bool bypass);

    bool getDataBypass() const noexcept;
    /// Data color spaces do not get processed when true (which is the default).
    void setDataBypass(bool bypass) noexcept;

    // Do not use (needed only for pybind11).
    virtual ~DisplayViewTransform();

private:
    DisplayViewTransform();
    DisplayViewTransform(const DisplayViewTransform &) = delete;
    DisplayViewTransform & operator=(const DisplayViewTransform &) = delete;

    static void deleter(DisplayViewTransform * t);

    class Impl;
    Impl * m_impl;
    Impl * getImpl() { return m_impl; }
    const Impl * getImpl() const { return m_impl; }
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const DisplayViewTransform &);


/**
 * Allows transform parameter values to be set on-the-fly
 * (after finalization).  For example, to modify the exposure in a viewport.
 */
class OCIOEXPORT DynamicProperty
{
public:
    virtual DynamicPropertyType getType() const = 0;

    virtual DynamicPropertyValueType getValueType() const = 0;

    virtual double getDoubleValue() const = 0;
    virtual void setValue(double value) = 0;

    virtual bool isDynamic() const = 0;

    DynamicProperty & operator=(const DynamicProperty &) = delete;
    virtual ~DynamicProperty();

protected:
    DynamicProperty();
    DynamicProperty(const DynamicProperty &);
};


/**
 * \brief Represents exponent transform: pow( clamp(color), value ).
 *
 * ​.. note::
 *    For configs with version == 1: Negative style is ignored and if the exponent is 1.0,
 *    this will not clamp. Otherwise, the input color will be clamped between [0.0, inf].
 *    For configs with version > 1: Negative value handling may be specified via setNegativeStyle.
 */
class OCIOEXPORT ExponentTransform : public Transform
{
public:
    static ExponentTransformRcPtr Create();

    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    /// Checks if this exactly equals other.
    virtual bool equals(const ExponentTransform & other) const noexcept = 0;

    virtual void getValue(double(&vec4)[4]) const noexcept = 0;
    virtual void setValue(const double(&vec4)[4]) noexcept = 0;

    /**
     *  Specifies how negative values are handled. Legal values:
     *
     * * NEGATIVE_CLAMP -- Clamp negative values (default).
     * * NEGATIVE_MIRROR -- Positive curve is rotated 180 degrees around the origin to
     *                      handle negatives.
     * * NEGATIVE_PASS_THRU -- Negative values are passed through unchanged.
     */
    virtual NegativeStyle getNegativeStyle() const = 0;
    virtual void setNegativeStyle(NegativeStyle style) = 0;
    
    ExponentTransform(const ExponentTransform &) = delete;
    ExponentTransform & operator= (const ExponentTransform &) = delete;
    virtual ~ExponentTransform() = default;

protected:
    ExponentTransform() = default;
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const ExponentTransform &);


/**
 * Represents power functions with a linear section in the shadows
 * such as sRGB and L*.
 *
 * The basic formula is::
 *
 *   pow( (x + offset)/(1 + offset), gamma )
 *   with the breakpoint at offset/(gamma - 1).
 *
 * Negative values are never clamped.
 */
class OCIOEXPORT ExponentWithLinearTransform : public Transform
{
public:
    static ExponentWithLinearTransformRcPtr Create();

    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    /// Checks if this exactly equals other.
    virtual bool equals(const ExponentWithLinearTransform & other) const noexcept = 0;

    virtual void getGamma(double(&values)[4]) const noexcept = 0;
    /**
     * Set the exponent value for the power function for R, G, B, A.
     *
     * \note 
     *    The gamma values must be in the range of [1, 10]. Set the transform direction
     *    to inverse to obtain the effect of values less than 1.
     */
    virtual void setGamma(const double(&values)[4]) noexcept = 0;

    virtual void getOffset(double(&values)[4]) const noexcept = 0;
    /**
     * Set the offset value for the power function for R, G, B, A.
     *
     * \note 
     *    The offset values must be in the range [0, 0.9].
     */
    virtual void setOffset(const double(&values)[4]) noexcept = 0;

    /**
     *  Specifies how negative values are handled. Legal values:
     *
     * * NEGATIVE_LINEAR -- Linear segment continues into negatives (default).
     * * NEGATIVE_MIRROR -- Positive curve is rotated 180 degrees around the origin to
     *                      handle negatives.
     */
    virtual NegativeStyle getNegativeStyle() const = 0;
    virtual void setNegativeStyle(NegativeStyle style) = 0;
    
    ExponentWithLinearTransform(const ExponentWithLinearTransform &) = delete;
    ExponentWithLinearTransform & operator= (const ExponentWithLinearTransform &) = delete;
    virtual ~ExponentWithLinearTransform() = default;

protected:
    ExponentWithLinearTransform() = default;
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const ExponentWithLinearTransform &);

/**
 * Applies exposure, gamma, and pivoted contrast adjustments.
 * Adjusts the math to be appropriate for linear, logarithmic, or video
 * color spaces.
 */
class OCIOEXPORT ExposureContrastTransform : public Transform
{
public:
    static ExposureContrastTransformRcPtr Create();

    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    /// Checks if this exactly equals other.
    virtual bool equals(const ExposureContrastTransform & other) const noexcept = 0;

    virtual ExposureContrastStyle getStyle() const = 0;
    /**
     * Select the algorithm for linear, video
     * or log color spaces.
     */
    virtual void setStyle(ExposureContrastStyle style) = 0;

    virtual double getExposure() const = 0;
    /**
     * Applies an exposure adjustment.  The value is in
     * units of stops (regardless of style), for example, a value of -1
     * would be equivalent to reducing the lighting by one half.
     */
    virtual void setExposure(double exposure) = 0;
    virtual bool isExposureDynamic() const = 0;
    virtual void makeExposureDynamic() = 0;

    virtual double getContrast() const = 0;
    /**
     * Applies a contrast/gamma adjustment around a pivot
     * point.  The contrast and gamma are mathematically the same, but two
     * controls are provided to enable the use of separate dynamic
     * parameters.  Contrast is usually a scene-referred adjustment that
     * pivots around gray whereas gamma is usually a display-referred
     * adjustment that pivots around white.
     */
    virtual void setContrast(double contrast) = 0;
    virtual bool isContrastDynamic() const = 0;
    virtual void makeContrastDynamic() = 0;
    virtual double getGamma() const = 0;
    virtual void setGamma(double gamma) = 0;
    virtual bool isGammaDynamic() const = 0;
    virtual void makeGammaDynamic() = 0;

    virtual double getPivot() const = 0;
    /**
     * Set the pivot point around which the contrast
     * and gamma controls will work. Regardless of whether
     * linear/video/log-style is being used, the pivot is always expressed
     * in linear. In other words, a pivot of 0.18 is always mid-gray.
     */
    virtual void setPivot(double pivot) = 0;

    virtual double getLogExposureStep() const = 0;
    /**
     * Set the increment needed to move one stop for
     * the log-style algorithm. For example, ACEScct is 0.057, LogC is
     * roughly 0.074, and Cineon is roughly 90/1023 = 0.088.
     * The default value is 0.088.
     */
    virtual void setLogExposureStep(double logExposureStep) = 0;

    virtual double getLogMidGray() const = 0;
    /**
     * Set the position of 18% gray for use by the
     * log-style algorithm. For example, ACEScct is about 0.41, LogC is
     * about 0.39, and ADX10 is 445/1023 = 0.435.
     * The default value is 0.435.
     */
    virtual void setLogMidGray(double logMidGray) = 0;

    virtual ~ExposureContrastTransform() = default;

protected:
    ExposureContrastTransform() = default;

private:
    ExposureContrastTransform(const ExposureContrastTransform &) = delete;
    ExposureContrastTransform & operator= (const ExposureContrastTransform &) = delete;
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &,
                                            const ExposureContrastTransform &);


class OCIOEXPORT FileTransform : public Transform
{
public:
    static FileTransformRcPtr Create();

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    /// Will throw if data is not valid.
    void validate() const override;

    const char * getSrc() const;
    void setSrc(const char * src);

    const char * getCCCId() const;
    void setCCCId(const char * id);

    CDLStyle getCDLStyle() const;
    /**
     * Can be used with CDL, CC & CCC formats to specify the clamping behavior of
     * the CDLTransform. Default is CDL_NO_CLAMP.
     */
    void setCDLStyle(CDLStyle);

    Interpolation getInterpolation() const;
    void setInterpolation(Interpolation interp);

    /// Get the number of LUT readers.
    static int getNumFormats();
    /**
     * Get the LUT readers at index, return empty string if
     * an invalid index is specified.
     */
    static const char * getFormatNameByIndex(int index);

    /**
     * Get the LUT reader extension at index, return empty string if
     * an invalid index is specified.
     */
    static const char * getFormatExtensionByIndex(int index);

    FileTransform & operator=(const FileTransform &) = delete;
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

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const FileTransform &);


/**
 * Provides a set of hard-coded algorithmic building blocks
 * that are needed to accurately implement various common color transformations.
 */
class OCIOEXPORT FixedFunctionTransform : public Transform
{
public:
    static FixedFunctionTransformRcPtr Create();

    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    /// Checks if this exactly equals other.
    virtual bool equals(const FixedFunctionTransform & other) const noexcept = 0;

    virtual FixedFunctionStyle getStyle() const = 0;
    /// Select which algorithm to use.
    virtual void setStyle(FixedFunctionStyle style) = 0;

    virtual size_t getNumParams() const = 0;
    virtual void getParams(double * params) const = 0;
    /// Set the parameters (for functions that require them).
    virtual void setParams(const double * params, size_t num) = 0;

    FixedFunctionTransform(const FixedFunctionTransform &) = delete;
    FixedFunctionTransform & operator= (const FixedFunctionTransform &) = delete;
    virtual ~FixedFunctionTransform() = default;

protected:
    FixedFunctionTransform() = default;
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const FixedFunctionTransform &);


class OCIOEXPORT GroupTransform : public Transform
{
public:
    static GroupTransformRcPtr Create();

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    /// Will throw if data is not valid.
    void validate() const override;

    virtual const FormatMetadata & getFormatMetadata() const noexcept;
    virtual FormatMetadata & getFormatMetadata() noexcept;

    ConstTransformRcPtr getTransform(int index) const;

    TransformRcPtr & getTransform(int index);

    int getNumTransforms() const;
    /// Adds a transform to the end of the group.
    void appendTransform(TransformRcPtr transform);
    /// Add a transform at the beginning of the group.
    void prependTransform(TransformRcPtr transform);

    GroupTransform & operator=(const GroupTransform &) = delete;
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

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const GroupTransform &);



/**
 *  Applies a logarithm with an affine transform before and after.
 * Represents the Cineon lin-to-log type transforms::
 *
 *   logSideSlope * log( linSideSlope * color + linSideOffset, base) + logSideOffset
 *
 * * Default values are: 1. * log( 1. * color + 0., 2.) + 0.
 * * The alpha channel is not affected.
 */
class OCIOEXPORT LogAffineTransform : public Transform
{
public:
    static LogAffineTransformRcPtr Create();

    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    /// Checks if this exactly equals other.
    virtual bool equals(const LogAffineTransform & other) const noexcept = 0;

    virtual double getBase() const noexcept = 0;
    virtual void setBase(double base) noexcept = 0;

    // !rst:: **Get/Set values for the R, G, B components**

    virtual void getLogSideSlopeValue(double(&values)[3]) const noexcept = 0;
    virtual void setLogSideSlopeValue(const double(&values)[3]) noexcept = 0;
    virtual void getLogSideOffsetValue(double(&values)[3]) const noexcept = 0;
    virtual void setLogSideOffsetValue(const double(&values)[3]) noexcept = 0;
    virtual void getLinSideSlopeValue(double(&values)[3]) const noexcept = 0;
    virtual void setLinSideSlopeValue(const double(&values)[3]) noexcept = 0;
    virtual void getLinSideOffsetValue(double(&values)[3]) const noexcept = 0;
    virtual void setLinSideOffsetValue(const double(&values)[3]) noexcept = 0;

    LogAffineTransform(const LogAffineTransform &) = delete;
    LogAffineTransform & operator= (const LogAffineTransform &) = delete;
    virtual ~LogAffineTransform() = default;

protected:
    LogAffineTransform() = default;
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const LogAffineTransform &);


/**
 *  Same as :cpp:class:`LogAffineTransform` but with the addition of a linear segment
 * near black. This formula is used for many camera logs (e.g., LogC) as well as ACEScct.
 *
 * * The linSideBreak specifies the point on the linear axis where the log and linear
 *   segments meet.  It must be set (there is no default).  
 * * The linearSlope specifies the slope of the linear segment of the forward (linToLog)
 *   transform.  By default it is set equal to the slope of the log curve at the break point.
 */
class OCIOEXPORT LogCameraTransform : public Transform
{
public:
    static LogCameraTransformRcPtr Create();

    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    /// Checks if this exactly equals other.
    virtual bool equals(const LogCameraTransform & other) const noexcept = 0;

    virtual double getBase() const noexcept = 0;
    virtual void setBase(double base) noexcept = 0;

    // !rst:: **Get/Set values for the R, G, B components**

    virtual void getLogSideSlopeValue(double(&values)[3]) const noexcept = 0;
    virtual void setLogSideSlopeValue(const double(&values)[3]) noexcept = 0;
    virtual void getLogSideOffsetValue(double(&values)[3]) const noexcept = 0;
    virtual void setLogSideOffsetValue(const double(&values)[3]) noexcept = 0;
    virtual void getLinSideSlopeValue(double(&values)[3]) const noexcept = 0;
    virtual void setLinSideSlopeValue(const double(&values)[3]) noexcept = 0;
    virtual void getLinSideOffsetValue(double(&values)[3]) const noexcept = 0;
    virtual void setLinSideOffsetValue(const double(&values)[3]) noexcept = 0;

    /// Return true if LinSideBreak values were set, false if they were not.
    virtual bool getLinSideBreakValue(double(&values)[3]) const noexcept = 0;
    virtual void setLinSideBreakValue(const double(&values)[3]) noexcept = 0;

    /// Return true if LinearSlope values were set, false if they were not.
    virtual bool getLinearSlopeValue(double(&values)[3]) const = 0;
    /**
     * \brief Set LinearSlope value.
     * 
     * \note
     *      You must call setLinSideBreakValue before calling this.
     */
    virtual void setLinearSlopeValue(const double(&values)[3]) = 0;
    /// Remove LinearSlope values so that default values are used.
    virtual void unsetLinearSlopeValue() = 0;
    
    LogCameraTransform(const LogCameraTransform &) = delete;
    LogCameraTransform & operator= (const LogCameraTransform &) = delete;
    virtual ~LogCameraTransform() = default;

protected:
    LogCameraTransform() = default;
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const LogCameraTransform &);


/**
 * Represents log transform: log(color, base)
 *
 * * The input will be clamped for negative numbers.
 * * Default base is 2.0.
 * * The alpha channel is not affected.
 */
class OCIOEXPORT LogTransform : public Transform
{
public:
    static LogTransformRcPtr Create();

    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    /// Checks if this exactly equals other.
    virtual bool equals(const LogTransform & other) const noexcept = 0;

    virtual double getBase() const noexcept = 0;
    virtual void setBase(double val) noexcept = 0;

    LogTransform(const LogTransform &) = delete;
    LogTransform & operator= (const LogTransform &) = delete;
    virtual ~LogTransform() = default;

protected:
    LogTransform() = default;
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const LogTransform &);


class OCIOEXPORT LookTransform : public Transform
{
public:
    static LookTransformRcPtr Create();

    TransformRcPtr createEditableCopy() const override;

    TransformDirection getDirection() const noexcept override;
    void setDirection(TransformDirection dir) noexcept override;

    /// Will throw if data is not valid.
    void validate() const override;

    const char * getSrc() const;
    void setSrc(const char * src);

    const char * getDst() const;
    void setDst(const char * dst);

    const char * getLooks() const;
    /**
     * Specify looks to apply.
     * Looks is a potentially comma (or colon) delimited list of look names,
     * Where +/- prefixes are optionally allowed to denote forward/inverse
     * look specification. (And forward is assumed in the absence of either)
     */
    void setLooks(const char * looks);

    bool getSkipColorSpaceConversion() const;
    void setSkipColorSpaceConversion(bool skip);

    /**
     * Return the name of the color space after applying looks in the forward
     * direction but without converting to the destination color space.  This is equivalent
     * to the process space of the last look in the look sequence (and takes into account that
     * a look fall-back may be used).
     */
    static const char * GetLooksResultColorSpace(const ConstConfigRcPtr & config,
                                                 const ConstContextRcPtr & context,
                                                 const char * looks);

    LookTransform & operator=(const LookTransform &) = delete;
    /// Do not use (needed only for pybind11).
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

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const LookTransform &);


/// Represents a 1D-LUT transform.
class OCIOEXPORT Lut1DTransform : public Transform
{
public:
    /// Create an identity 1D-LUT of length two.
    static Lut1DTransformRcPtr Create();

    /**
     * Create an identity 1D-LUT with specific length and
     * half-domain setting. Will throw for lengths longer than 1024x1024.
     */
    static Lut1DTransformRcPtr Create(unsigned long length,
                                      bool isHalfDomain);

    virtual BitDepth getFileOutputBitDepth() const noexcept = 0;
    /**
     * Get the bit-depth associated with the LUT values read
     * from a file or set the bit-depth of values to be written to a file
     * (for file formats such as CLF that support multiple bit-depths).
     * However, note that the values stored in the object are always
     * normalized.
     */
    virtual void setFileOutputBitDepth(BitDepth bitDepth) noexcept = 0;

    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    /// Checks if this exactly equals other.
    virtual bool equals(const Lut1DTransform & other) const noexcept = 0;

    virtual unsigned long getLength() const = 0;
    /**
     * Changing the length will reset the LUT to identity.
     * Will throw for lengths longer than 1024x1024.
     */
    virtual void setLength(unsigned long length) = 0;

    virtual void getValue(unsigned long index, float & r, float & g, float & b) const = 0;
    /**
     * Set the values of a LUT1D.  Will throw if the index
     * is outside of the range from 0 to (length-1).
     *
     * The LUT values are always for the "forward" LUT, regardless of how
     * the transform direction is set.
     *
     * These values are normalized relative to what may be stored in any
     * given LUT files. For example in a CLF file using a "10i" output
     * depth, a value of 1023 in the file is normalized to 1.0. The
     * values here are unclamped and may extend outside [0,1].
     *
     * LUTs in various file formats may only provide values for one
     * channel where R, G, B are the same. Even in that case, you should
     * provide three equal values to the setter.
     */
    virtual void setValue(unsigned long index, float r, float g, float b) = 0;

    virtual bool getInputHalfDomain() const noexcept = 0;
    /**
     * In a half-domain LUT, the contents of the LUT specify
     * the desired value of the function for each half-float value.
     * Therefore, the length of the LUT must be 65536 entries or else
     * validate() will throw.
     */
    virtual void setInputHalfDomain(bool isHalfDomain) noexcept = 0;

    virtual bool getOutputRawHalfs() const noexcept = 0;
    /**
     * Set OutputRawHalfs to true if you want to output the
     * LUT contents as 16-bit floating point values expressed as unsigned
     * 16-bit integers representing the equivalent bit pattern.
     * For example, the value 1.0 would be written as the integer 15360
     * because it has the same bit-pattern.  Note that this implies the
     * values will be quantized to a 16-bit float.  Note that this setting
     * only controls the output formatting (where supported) and not the 
     * values for getValue/setValue.  The only file formats that currently
     * support this are CLF and CTF.
     */
    virtual void setOutputRawHalfs(bool isRawHalfs) noexcept = 0;

    virtual Lut1DHueAdjust getHueAdjust() const noexcept = 0;
    /**
     * The 1D-LUT transform optionally supports a hue adjustment
     * feature that was used in some versions of ACES.  This adjusts the hue
     * of the result to approximately match the input.
     */
    virtual void setHueAdjust(Lut1DHueAdjust algo) noexcept = 0;

    virtual Interpolation getInterpolation() const = 0;
    virtual void setInterpolation(Interpolation algo) = 0;

    Lut1DTransform(const Lut1DTransform &) = delete;
    Lut1DTransform & operator= (const Lut1DTransform &) = delete;
    virtual ~Lut1DTransform() = default;

protected:
    Lut1DTransform() = default;
};

extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const Lut1DTransform&);


/// Represents a 3D-LUT transform.
class OCIOEXPORT Lut3DTransform : public Transform
{
public:
    /// Create an identity 3D-LUT of size 2x2x2.
    static Lut3DTransformRcPtr Create();

    /**
     * Create an identity 3D-LUT with specific grid size.
     * Will throw for grid size larger than 129.
     */
    static Lut3DTransformRcPtr Create(unsigned long gridSize);

    virtual BitDepth getFileOutputBitDepth() const noexcept = 0;
    /**
     * Get the bit-depth associated with the LUT values read
     * from a file or set the bit-depth of values to be written to a file
     * (for file formats such as CLF that support multiple bit-depths).
     * However, note that the values stored in the object are always
     * normalized.
     */
    virtual void setFileOutputBitDepth(BitDepth bitDepth) noexcept = 0;

    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    /// Checks if this exactly equals other.
    virtual bool equals(const Lut3DTransform & other) const noexcept = 0;

    virtual unsigned long getGridSize() const = 0;
    /**
     * Changing the grid size will reset the LUT to identity.
     * Will throw for grid sizes larger than 129.
     */
    virtual void setGridSize(unsigned long gridSize) = 0;

    virtual void getValue(unsigned long indexR,
                          unsigned long indexG,
                          unsigned long indexB,
                          float & r, float & g, float & b) const = 0;
    /**
     * Set the values of a 3D-LUT. Will throw if an index is
     * outside of the range from 0 to (gridSize-1).
     *
     * The LUT values are always for the "forward" LUT, regardless of how the
     * transform direction is set.
     *
     * These values are normalized relative to what may be stored in any
     * given LUT files. For example in a CLF file using a "10i" output
     * depth, a value of 1023 in the file is normalized to 1.0. The values
     * here are unclamped and may extend outside [0,1].
     */
    virtual void setValue(unsigned long indexR,
                          unsigned long indexG,
                          unsigned long indexB,
                          float r, float g, float b) = 0;

    virtual Interpolation getInterpolation() const = 0;
    virtual void setInterpolation(Interpolation algo) = 0;

    Lut3DTransform(const Lut3DTransform &) = delete;
    Lut3DTransform & operator= (const Lut3DTransform &) = delete;
    virtual ~Lut3DTransform() = default;

protected:
    Lut3DTransform() = default;
};

extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const Lut3DTransform&);

/**
 * Represents an MX+B Matrix transform.
 *
 * \note 
 *    For singular matrices, an inverse direction will throw an exception during finalization.
 */
class OCIOEXPORT MatrixTransform : public Transform
{
public:
    static MatrixTransformRcPtr Create();

    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    /// Checks if this exactly equals other.
    virtual bool equals(const MatrixTransform & other) const noexcept = 0;

    virtual void getMatrix(double * m44) const = 0;
    /**
     * Get or set the values of a Matrix. Expects 16 values,
     * where the first four are the coefficients to generate the R output
     * channel from R, G, B, A input channels.
     *
     * The Matrix values are always for the "forward" Matrix, regardless of
     * how the transform direction is set.
     *
     * These values are normalized relative to what may be stored in
     * file formats such as CLF. For example in a CLF file using a "32f"
     * input depth and "10i" output depth, a value of 1023 in the file
     * is normalized to 1.0. The values here are unclamped and may
     * extend outside [0,1].
     */
    virtual void setMatrix(const double * m44) = 0;

    virtual void getOffset(double * offset4) const = 0;
    /**
     * Get or set the R, G, B, A offsets to be applied
     * after the matrix.
     *
     * These values are normalized relative to what may be stored in
     * file formats such as CLF. For example, in a CLF file using a
     * "10i" output depth, a value of 1023 in the file is normalized
     * to 1.0. The values here are unclamped and may extend
     * outside [0,1].
     */
    virtual void setOffset(const double * offset4) = 0;

    // TODO: Move to .rst
    // !rst:: **File bit-depth**
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

    virtual BitDepth getFileInputBitDepth() const noexcept = 0;
    virtual void setFileInputBitDepth(BitDepth bitDepth) noexcept = 0;
    virtual BitDepth getFileOutputBitDepth() const noexcept = 0;
    virtual void setFileOutputBitDepth(BitDepth bitDepth) noexcept = 0;

    // TODO: Move to .rst
    // !rst:: **Convenience functions**
    //
    // Build the matrix and offset corresponding to higher-level concepts.
    // 
    // .. note::
    //    These can throw an exception if for any component
    //    ``oldmin == oldmax. (divide by 0)``

    static void Fit(double * m44, double* offset4,
                    const double * oldmin4, const double * oldmax4,
                    const double * newmin4, const double * newmax4);

    static void Identity(double * m44, double * offset4);

    static void Sat(double * m44, double * offset4,
                    double sat, const double * lumaCoef3);

    static void Scale(double * m44, double * offset4,
                      const double * scale4);

    static void View(double * m44, double * offset4,
                     int * channelHot4,
                     const double * lumaCoef3);

    MatrixTransform(const MatrixTransform &) = delete;
    MatrixTransform & operator= (const MatrixTransform &) = delete;
    virtual ~MatrixTransform() = default;

protected:
    MatrixTransform() = default;
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const MatrixTransform &) noexcept;


/**
 * Represents a range transform
 *
 * The Range is used to apply an affine transform (scale & offset) and
 * clamps values to min/max bounds on all color components except the alpha.
 * The scale and offset values are computed from the input and output bounds.
 * 
 * Refer to section 7.2.4 in specification S-2014-006 "A Common File Format
 * for Look-Up Tables" from the Academy of Motion Picture Arts and Sciences
 * and the American Society of Cinematographers.
 *
 * The "noClamp" style described in the specification S-2014-006 becomes a
 * MatrixOp at the processor level.
 */
class OCIOEXPORT RangeTransform : public Transform
{
public:
    /// Creates an instance of RangeTransform.
    static RangeTransformRcPtr Create();

    virtual RangeStyle getStyle() const noexcept = 0;
    /// Set the Range style to clamp or not input values.
    virtual void setStyle(RangeStyle style) noexcept = 0;

    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    /// Checks if this equals other.
    virtual bool equals(const RangeTransform & other) const noexcept = 0;

    // TODO: Move to .rst
    // !rst:: **File bit-depth**
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

    virtual BitDepth getFileInputBitDepth() const noexcept = 0;
    virtual void setFileInputBitDepth(BitDepth bitDepth) noexcept = 0;
    virtual BitDepth getFileOutputBitDepth() const noexcept = 0;
    virtual void setFileOutputBitDepth(BitDepth bitDepth) noexcept = 0;

    // TODO: Move to .rst
    // !rst:: **Range values**
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

    /// Get the minimum value for the input.
    virtual double getMinInValue() const noexcept = 0;
    /// Set the minimum value for the input.
    virtual void setMinInValue(double val) noexcept = 0;
    /// Is the minimum value for the input set?
    virtual bool hasMinInValue() const noexcept = 0;
    /// Unset the minimum value for the input
    virtual void unsetMinInValue() noexcept = 0;

    /// Set the maximum value for the input.
    virtual void setMaxInValue(double val) noexcept = 0;
    /// Get the maximum value for the input.
    virtual double getMaxInValue() const noexcept = 0;
    /// Is the maximum value for the input set?
    virtual bool hasMaxInValue() const noexcept = 0;
    /// Unset the maximum value for the input.
    virtual void unsetMaxInValue() noexcept = 0;

    /// Set the minimum value for the output.
    virtual void setMinOutValue(double val) noexcept = 0;
    /// Get the minimum value for the output.
    virtual double getMinOutValue() const noexcept = 0;
    /// Is the minimum value for the output set?
    virtual bool hasMinOutValue() const noexcept = 0;
    /// Unset the minimum value for the output.
    virtual void unsetMinOutValue() noexcept = 0;

    /// Set the maximum value for the output.
    virtual void setMaxOutValue(double val) noexcept = 0;
    /// Get the maximum value for the output.
    virtual double getMaxOutValue() const noexcept = 0;
    /// Is the maximum value for the output set?
    virtual bool hasMaxOutValue() const noexcept = 0;
    /// Unset the maximum value for the output.
    virtual void unsetMaxOutValue() noexcept = 0;

    RangeTransform(const RangeTransform &) = delete;
    RangeTransform & operator= (const RangeTransform &) = delete;
    virtual ~RangeTransform() = default;

protected:
    RangeTransform() = default;
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const RangeTransform &) noexcept;

} // namespace OCIO_NAMESPACE

#endif

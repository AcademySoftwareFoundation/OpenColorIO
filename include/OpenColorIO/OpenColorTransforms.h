// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_OPENCOLORTRANSFORMS_H
#define INCLUDED_OCIO_OPENCOLORTRANSFORMS_H

#include <initializer_list>
#include <limits>

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
 * The FormatMetadata class is intended to be a generic container to hold metadata from various
 * file formats.
 *
 * This class provides a hierarchical metadata container. A metadata object is similar to an
 * element in XML. The top level element is named "ROOT" and can't be renamed. Several transforms
 * have a FormatMetadata.
 * The root element and all of the sub-elements may contain:
 * * A name string (e.g. "ROOT", "Description"...). Name can't be empty.
 * * A value string (e.g. "updated viewing LUT"). Value can be empty.
 * * A list of attributes (name, value) string pairs (e.g. "version", "1.5"). There are helper
 *   functions to get and set "id" and "name" attributes. Attribute names are unique.
 * * And a list of child sub-elements, which are also objects implementing FormatMetadata. There
 *   can be several sub-elements with the same name.
 */
class OCIOEXPORT FormatMetadata
{
public:
    virtual const char * getElementName() const noexcept = 0;
    /// Name has to be a non-empty string. Top-level element can't be renamed. 'ROOT' is reserved.
    virtual void setElementName(const char *) = 0;

    virtual const char * getElementValue() const noexcept = 0;
    virtual void setElementValue(const char *) = 0;

    virtual int getNumAttributes() const noexcept = 0;
    /// Get the name of a attribute ("" if attribute does not exist).
    virtual const char * getAttributeName(int i) const noexcept = 0;
    /// Get the value of a attribute ("" if attribute does not exist).
    virtual const char * getAttributeValue(int i) const noexcept = 0;
    /// Get the value of a attribute of a given name ("" if attribute does not exist).
    virtual const char * getAttributeValue(const char * name) const noexcept = 0;
    /**
     * Add an attribute with a given name and value. If an attribute with the same name already
     * exists, its value is replaced. Throw if name is NULL or empty.
     */
    virtual void addAttribute(const char * name, const char * value) = 0;

    virtual int getNumChildrenElements() const noexcept = 0;
    /**
     * Access a child element.
     *
     * \note
     *    Adding siblings might cause a reallocation of the container and thus might make the
     *    reference unusable.
     *    Index i has to be positive and less than getNumChildrenElements() or the function will
     *    throw.
     */
    virtual const FormatMetadata & getChildElement(int i) const = 0;
    virtual FormatMetadata & getChildElement(int i) = 0;

    /**
     * Add a child element with a given name and value.
     *
     * Name has to be non-empty. Value may be empty, particularly if this element will have
     * children. Element is added after all existing children. Use
     * getChildElement(getNumChildrenElements()-1) to access the added element.
     */
    virtual void addChildElement(const char * name, const char * value) = 0;

    /// Remove all children, all attributes and the value.
    virtual void clear() noexcept = 0;

    virtual FormatMetadata & operator=(const FormatMetadata & rhs) = 0;

    /**
     * Convenience method to easily get/set the 'name' attribute.  This corresponds to the
     * ProcessNode name attribute from a CLF / CTF file or the name key of a transform in the
     * config YAML.
     */
    virtual const char * getName() const noexcept = 0;
    virtual void setName(const char * name) noexcept = 0;
    /**
     * Convenience method to easily get/set the 'id' attribute.  This corresponds to the
     * ProcessNode id attribute from a CLF/CTF file or the ColorCorrection id attribute from a
     * CC/CCC/CDL file.
     */
    virtual const char * getID() const noexcept = 0;
    virtual void setID(const char * id) noexcept = 0;

    FormatMetadata(const FormatMetadata & rhs) = delete;
    /// Do not use (needed only for pybind11).
    virtual ~FormatMetadata() = default;

protected:
    FormatMetadata() = default;
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const FormatMetadata &);


/// Base class for all the transform classes
class OCIOEXPORT Transform
{
public:
    virtual TransformRcPtr createEditableCopy() const = 0;

    virtual TransformDirection getDirection() const noexcept = 0;
    /// Note that this only affects the evaluation and not the values stored in the object.
    virtual void setDirection(TransformDirection dir) noexcept = 0;

    virtual TransformType getTransformType() const noexcept = 0;

    /// Will throw if data is not valid.
    virtual void validate() const;

    Transform(const Transform &) = delete;
    Transform & operator= (const Transform &) = delete;
    /// Do not use (needed only for pybind11).
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

    TransformType getTransformType() const noexcept override { return TRANSFORM_TYPE_ALLOCATION; }

    /// Will throw if data is not valid.
    void validate() const override;

    Allocation getAllocation() const;
    void setAllocation(Allocation allocation);

    int getNumVars() const;
    void getVars(float * vars) const;
    void setVars(int numvars, const float * vars);

    AllocationTransform & operator= (const AllocationTransform &) = delete;
    /// Do not use (needed only for pybind11).
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

    TransformType getTransformType() const noexcept override { return TRANSFORM_TYPE_BUILTIN; }

    virtual const char * getStyle() const noexcept = 0;
    /**
     * Select an existing built-in transform style from the list accessible
     * through :cpp:class:`BuiltinTransformRegistry`. The style is the ID string that identifies
     * which transform to apply.
     */
    virtual void setStyle(const char * style) = 0;

    virtual const char * getDescription() const noexcept = 0;

    /// Do not use (needed only for pybind11).
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
     * \brief Load the CDL from the src .cdl, .cc, or .ccc file.
     *
     * \note
     *    The cccid can be the ID of a CDL or the index of the CDL (as string). If cccid is NULL or
     *    empty the first CDL is returned.  The cccid is case-sensitive. The src must be an
     *    absolute path reference, no relative directory or envvar resolution is performed. Throws
     *    if file does not contain any CDL or if the specified cccid is not found.
     */
    static CDLTransformRcPtr CreateFromFile(const char * src, const char * cccid);
    
    /**
     * \brief Load all of the CDLs in a .cdl or .ccc file into a single GroupTransform.
     *
     * \note
     *    This may be useful as a quicker way for applications to check the contents of each of
     *    the CDLs. The src must be an absolute path reference, no relative directory or envvar
     *    resolution is performed.
     */
    static GroupTransformRcPtr CreateGroupFromFile(const char * src);

    TransformType getTransformType() const noexcept override { return TRANSFORM_TYPE_CDL; }

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

    /**
     * The get/setID methods are now deprecated. The preferred way of interacting with the ID is
     * now via the transform's formatMetadata.
     */
    virtual const char * getID() const = 0;
    virtual void setID(const char * id) = 0;

    /* Get/Set the first Description element under the SOPNode.
     * Note: These emulate the get/setDescription methods from OCIO v1.
     *
     * Use the FormatMetadata interface for access to other Description elements in the CDL.
     * The Description children of the SOPNode element in the CDL XML are named 'SOPDescription'
     * in the FormatMetadata. NULL or empty string removes the first SOPDescription element.
     */
    virtual const char * getFirstSOPDescription() const = 0;
    virtual void setFirstSOPDescription(const char * description) = 0;

    CDLTransform(const CDLTransform &) = delete;
    CDLTransform & operator= (const CDLTransform &) = delete;
    /// Do not use (needed only for pybind11).
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

    TransformType getTransformType() const noexcept override { return TRANSFORM_TYPE_COLORSPACE; }

    void validate() const override;

    const char * getSrc() const;
    void setSrc(const char * src);

    const char * getDst() const;
    void setDst(const char * dst);

    /// Data color spaces do not get processed when true (which is the default).
    bool getDataBypass() const noexcept;
    void setDataBypass(bool enabled) noexcept;

    ColorSpaceTransform & operator=(const ColorSpaceTransform &) = delete;
    /// Do not use (needed only for pybind11).
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

    TransformType getTransformType() const noexcept override { return TRANSFORM_TYPE_DISPLAY_VIEW; }

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

    /// Do not use (needed only for pybind11).
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
 * Used by the grading transforms to hold the red, green, blue, and master components
 * of a single parameter.  The master component affects all three channels (RGB).
 */
struct OCIOEXPORT GradingRGBM
{
    GradingRGBM() = default;
    GradingRGBM(const GradingRGBM &) = default;
    GradingRGBM(double red, double green, double blue, double master)
        : m_red(red)
        , m_green(green)
        , m_blue(blue)
        , m_master(master)
    {
    }
    GradingRGBM(const double(&rgbm)[4])
        : m_red(rgbm[0])
        , m_green(rgbm[1])
        , m_blue(rgbm[2])
        , m_master(rgbm[3])
    {
    }
    double m_red{ 0. };
    double m_green{ 0. };
    double m_blue{ 0. };
    double m_master{ 0. };
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const GradingRGBM &);

/// Grading primary values.
struct OCIOEXPORT GradingPrimary
{
    GradingPrimary() = delete;
    GradingPrimary(const GradingPrimary &) = default;
    explicit GradingPrimary(GradingStyle style)
        : m_pivot(style == GRADING_LOG ? -0.2 : 0.18)
        , m_clampBlack(NoClampBlack())
        , m_clampWhite(NoClampWhite())
    {
    }

    GradingRGBM m_brightness{ 0.0, 0.0, 0.0, 0.0 };
    GradingRGBM m_contrast  { 1.0, 1.0, 1.0, 1.0 };
    GradingRGBM m_gamma     { 1.0, 1.0, 1.0, 1.0 };
    GradingRGBM m_offset    { 0.0, 0.0, 0.0, 0.0 };
    GradingRGBM m_exposure  { 0.0, 0.0, 0.0, 0.0 };
    GradingRGBM m_lift      { 0.0, 0.0, 0.0, 0.0 };
    GradingRGBM m_gain      { 1.0, 1.0, 1.0, 1.0 };

    double m_saturation{ 1.0 };
    double m_pivot; // For LOG default is -0.2. LIN default is 0.18.
    double m_pivotBlack{ 0.0 };
    double m_pivotWhite{ 1.0 };
    double m_clampBlack;
    double m_clampWhite;

    /// The valid range for each parameter varies.
    void validate(GradingStyle style) const;

    static double NoClampBlack();
    static double NoClampWhite();
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const GradingPrimary &);

/// 2D control point used by \ref GradingBSplineCurve.
struct OCIOEXPORT GradingControlPoint
{
    GradingControlPoint() = default;
    GradingControlPoint(const GradingControlPoint &) = default;
    GradingControlPoint(float x, float y) : m_x(x), m_y(y) {}
    float m_x{ 0.f };
    float m_y{ 0.f };
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const GradingControlPoint &);

/// A BSpline curve defined with \ref GradingControlPoint.
class OCIOEXPORT GradingBSplineCurve
{
public:
    /// Create a BSpline curve with a specified number of control points.
    static GradingBSplineCurveRcPtr Create(size_t size);
    /// Create a BSpline curve with a list of control points.
    static GradingBSplineCurveRcPtr Create(std::initializer_list<GradingControlPoint> values);

    virtual GradingBSplineCurveRcPtr createEditableCopy() const = 0;
    virtual size_t getNumControlPoints() const noexcept = 0;
    virtual void setNumControlPoints(size_t size) = 0;
    virtual const GradingControlPoint & getControlPoint(size_t index) const = 0;
    virtual GradingControlPoint & getControlPoint(size_t index) = 0;
    virtual float getSlope(size_t index) const = 0;
    virtual void setSlope(size_t index, float slope) = 0;
    virtual bool slopesAreDefault() const = 0;
    virtual void validate() const = 0;

    GradingBSplineCurve(const GradingBSplineCurve &) = delete;
    GradingBSplineCurve & operator= (const GradingBSplineCurve &) = delete;

    /// Do not use (needed only for pybind11).
    virtual ~GradingBSplineCurve() = default;

protected:
    GradingBSplineCurve() = default;
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const GradingBSplineCurve &);

/**
 * A set of red, green, blue and master curves. It is used by RGBCurveTransform and can be used as
 * a dynamic property (see \ref DynamicPropertyGradingRGBCurve).
 */
class OCIOEXPORT GradingRGBCurve
{
public:
    static GradingRGBCurveRcPtr Create(GradingStyle style);
    static GradingRGBCurveRcPtr Create(const ConstGradingRGBCurveRcPtr & rhs);
    static GradingRGBCurveRcPtr Create(const ConstGradingBSplineCurveRcPtr & red,
                                       const ConstGradingBSplineCurveRcPtr & green,
                                       const ConstGradingBSplineCurveRcPtr & blue,
                                       const ConstGradingBSplineCurveRcPtr & master);

    virtual GradingRGBCurveRcPtr createEditableCopy() const = 0;
    virtual void validate() const = 0;
    virtual bool isIdentity() const = 0;
    virtual ConstGradingBSplineCurveRcPtr getCurve(RGBCurveType c) const = 0;
    virtual GradingBSplineCurveRcPtr getCurve(RGBCurveType c) = 0;

    /// Do not use (needed only for pybind11).
    virtual ~GradingRGBCurve() = default;

protected:
    GradingRGBCurve() = default;
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const GradingRGBCurve &);

/**
 * Used by the grading tone transforms to hold the red, green, blue, master, start,
 * and width components of a single parameter.  The master component affects all three channels
 * (RGB).  The start and width components control the range of tones affected. Although this
 * struct simply uses "start" and "width" for all the range values, the actual user-facing name
 * changes based on the parameter.
 */
struct OCIOEXPORT GradingRGBMSW
{
    GradingRGBMSW() = default;
    GradingRGBMSW(const GradingRGBMSW &) = default;
    GradingRGBMSW(double red, double green, double blue, double master, double start, double width)
        : m_red   (red)
        , m_green (green)
        , m_blue  (blue)
        , m_master(master)
        , m_start (start)
        , m_width (width)
    {
    }
    GradingRGBMSW(const double(&rgbmsw)[6])
        : m_red   (rgbmsw[0])
        , m_green (rgbmsw[1])
        , m_blue  (rgbmsw[2])
        , m_master(rgbmsw[3])
        , m_start (rgbmsw[4])
        , m_width (rgbmsw[5])
    {
    }
    GradingRGBMSW(double start, double width)
        : m_start(start)
        , m_width(width)
    {
    }
    double m_red   { 1. };
    double m_green { 1. };
    double m_blue  { 1. };
    double m_master{ 1. };
    double m_start { 0. }; // Or center for midtones.
    double m_width { 1. }; // Or pivot for shadows and highlights.
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const GradingRGBMSW &);

/// Grading tone values.
struct OCIOEXPORT GradingTone
{
    GradingTone() = delete;
    GradingTone(const GradingTone &) = default;
    explicit GradingTone(GradingStyle style)
        : m_blacks(style == GRADING_LIN ? GradingRGBMSW(0., 4.) :
                  (style == GRADING_LOG ? GradingRGBMSW(0.4, 0.4) :
                                          GradingRGBMSW(0.4, 0.4)))
        , m_shadows(style == GRADING_LIN ? GradingRGBMSW(2., -7.) :
                   (style == GRADING_LOG ? GradingRGBMSW(0.5, 0.) :
                                           GradingRGBMSW(0.6, 0.)))
        , m_midtones(style == GRADING_LIN ? GradingRGBMSW(0., 8.) :
                    (style == GRADING_LOG ? GradingRGBMSW(0.4, 0.6) :
                                            GradingRGBMSW(0.4, 0.7)))
        , m_highlights(style == GRADING_LIN ? GradingRGBMSW(-2., 9.) :
                      (style == GRADING_LOG ? GradingRGBMSW(0.3, 1.) :
                                              GradingRGBMSW(0.2, 1.)))
        , m_whites(style == GRADING_LIN ? GradingRGBMSW(0., 8.) :
                  (style == GRADING_LOG ? GradingRGBMSW(0.4, 0.5) :
                                          GradingRGBMSW(0.5, 0.5)))
    {
    }

    /**
     * The valid range for each parameter varies. The client is expected to enforce
     * these bounds in the UI.
     */
    void validate() const;

    GradingRGBMSW m_blacks;
    GradingRGBMSW m_shadows;
    GradingRGBMSW m_midtones;
    GradingRGBMSW m_highlights;
    GradingRGBMSW m_whites;
    double m_scontrast{ 1.0 };
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const GradingTone &);

/**
 * Allows transform parameter values to be set on-the-fly (after finalization).  For
 * example, to modify the exposure in a viewport.  Dynamic properties can be accessed from the
 * :cpp:class:`CPUProcessor` or :cpp:class:`GpuShaderCreator` to change values between processing.
 *
 * .. code-block:: cpp
 *
 *    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
 *    OCIO::ConstProcessorRcPtr processor = config->getProcessor(colorSpace1, colorSpace2);
 *    OCIO::ConstCPUProcessorRcPtr cpuProcessor = processor->getDefaultCPUProcessor();
 *
 *    if (cpuProcessor->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE))
 *    {
 *        // Get the in-memory implementation of the dynamic property.
 *        OCIO::DynamicPropertyRcPtr dynProp =
 *            cpuProcessor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE);
 *        // Get the interface used to change the double value.
 *        OCIO::DynamicPropertyDoubleRcPtr exposure =
 *            OCIO::DynamicPropertyValue::AsDouble(dynProp);
 *        // Update of the dynamic property instance with the new value.
 *        exposure->setValue(1.1f);
 *    }
 *    if (cpuProcessor->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_PRIMARY))
 *    {
 *        OCIO::DynamicPropertyRcPtr dynProp =
 *            cpuProcessor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_PRIMARY);
 *        OCIO::DynamicPropertyGradingPrimaryRcPtr primaryProp =
 *            OCIO::DynamicPropertyValue::AsGradingPrimary(dynProp);
 *        OCIO::GradingPrimary primary = primaryProp->getValue();
 *        primary.m_saturation += 0.1f;
 *        rgbCurveProp->setValue(primary);
 *    }
 *    if (cpuProcessor->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_RGBCURVE))
 *    {
 *        OCIO::DynamicPropertyRcPtr dynProp =
 *            cpuProcessor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_RGBCURVE);
 *        OCIO::DynamicPropertyGradingRGBCurveRcPtr rgbCurveProp =
 *            OCIO::DynamicPropertyValue::AsGradingRGBCurve(dynProp);
 *        OCIO::ConstGradingRGBCurveRcPtr rgbCurve = rgbCurveProp->getValue()->createEditableCopy();
 *        OCIO::GradingBSplineCurveRcPtr rCurve = rgbCurve->getCurve(OCIO::RGB_RED);
 *        rCurve->getControlPoint(1).m_y += 0.1f;
 *        rgbCurveProp->setValue(rgbCurve);
 *    }
 */
class OCIOEXPORT DynamicProperty
{
public:
    virtual DynamicPropertyType getType() const noexcept = 0;

    DynamicProperty & operator=(const DynamicProperty &) = delete;
    DynamicProperty(const DynamicProperty &) = delete;

    /// Do not use (needed only for pybind11).
    virtual ~DynamicProperty() = default;

protected:
    DynamicProperty() = default;
};

namespace DynamicPropertyValue
{
/**
 * Get the property as DynamicPropertyDoubleRcPtr to access the double value. Will throw if
 * property type is not a type that holds a double such as DYNAMIC_PROPERTY_EXPOSURE.
 */
extern OCIOEXPORT DynamicPropertyDoubleRcPtr AsDouble(DynamicPropertyRcPtr & prop);
/**
 * Get the property as DynamicPropertyGradingPrimaryRcPtr to access the GradingPrimary value. Will
 * throw if property type is not DYNAMIC_PROPERTY_GRADING_PRIMARY.
 */
extern OCIOEXPORT DynamicPropertyGradingPrimaryRcPtr AsGradingPrimary(DynamicPropertyRcPtr & prop);
/**
 * Get the property as DynamicPropertyGradingRGBCurveRcPtr to access the GradingRGBCurveRcPtr
 * value. Will throw if property type is not DYNAMIC_PROPERTY_GRADING_RGBCURVE.
 */
extern OCIOEXPORT DynamicPropertyGradingRGBCurveRcPtr AsGradingRGBCurve(DynamicPropertyRcPtr & prop);
/**
 * Get the property as DynamicPropertyGradingToneRcPtr to access the GradingTone value. Will throw
 * if property type is not DYNAMIC_PROPERTY_GRADING_TONE.
 */
extern OCIOEXPORT DynamicPropertyGradingToneRcPtr AsGradingTone(DynamicPropertyRcPtr & prop);
}

/// Interface used to access dynamic property double value.
class OCIOEXPORT DynamicPropertyDouble
{
public:
    virtual double getValue() const = 0;
    virtual void setValue(double value) = 0;

    DynamicPropertyDouble(const DynamicPropertyDouble &) = delete;
    DynamicPropertyDouble & operator=(const DynamicPropertyDouble &) = delete;
    /// Do not use (needed only for pybind11).
    virtual ~DynamicPropertyDouble() = default;

protected:
    DynamicPropertyDouble() = default;
};

/// Interface used to access dynamic property GradingPrimary value.
class OCIOEXPORT DynamicPropertyGradingPrimary
{
public:
    virtual const GradingPrimary & getValue() const = 0;
    /// Will throw if value is not valid.
    virtual void setValue(const GradingPrimary & value) = 0;

    DynamicPropertyGradingPrimary(const DynamicPropertyGradingPrimary &) = delete;
    DynamicPropertyGradingPrimary & operator=(const DynamicPropertyGradingPrimary &) = delete;
    /// Do not use (needed only for pybind11).
    virtual ~DynamicPropertyGradingPrimary() = default;

protected:
    DynamicPropertyGradingPrimary() = default;
};

/// Interface used to access dynamic property ConstGradingRGBCurveRcPtr value.
class OCIOEXPORT DynamicPropertyGradingRGBCurve
{
public:
    virtual const ConstGradingRGBCurveRcPtr & getValue() const = 0;
    /// Will throw if value is not valid.
    virtual void setValue(const ConstGradingRGBCurveRcPtr & value) = 0;

    DynamicPropertyGradingRGBCurve(const DynamicPropertyGradingRGBCurve &) = delete;
    DynamicPropertyGradingRGBCurve & operator=(const DynamicPropertyGradingRGBCurve &) = delete;
    /// Do not use (needed only for pybind11).
    virtual ~DynamicPropertyGradingRGBCurve() = default;

protected:
    DynamicPropertyGradingRGBCurve() = default;
};

/// Interface used to access dynamic property GradingTone value.
class OCIOEXPORT DynamicPropertyGradingTone
{
public:
    virtual const GradingTone & getValue() const = 0;
    /// Will throw if value is not valid.
    virtual void setValue(const GradingTone & value) = 0;

    DynamicPropertyGradingTone(const DynamicPropertyGradingTone &) = delete;
    DynamicPropertyGradingTone & operator=(const DynamicPropertyGradingTone &) = delete;
    /// Do not use (needed only for pybind11).
    virtual ~DynamicPropertyGradingTone() = default;

protected:
    DynamicPropertyGradingTone() = default;
};


/**
 * \brief Represents exponent transform: pow( clamp(color), value ).
 *
 * \note For configs with version == 1: Negative style is ignored and if the exponent is 1.0,
 * this will not clamp. Otherwise, the input color will be clamped between [0.0, inf].
 * For configs with version > 1: Negative value handling may be specified via setNegativeStyle.
 */
class OCIOEXPORT ExponentTransform : public Transform
{
public:
    static ExponentTransformRcPtr Create();

    TransformType getTransformType() const noexcept override { return TRANSFORM_TYPE_EXPONENT; }

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
    /// Do not use (needed only for pybind11).
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

    TransformType getTransformType() const noexcept override { return TRANSFORM_TYPE_EXPONENT_WITH_LINEAR; }

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
    /// Do not use (needed only for pybind11).
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

    TransformType getTransformType() const noexcept override { return TRANSFORM_TYPE_EXPOSURE_CONTRAST; }

    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    /// Checks if this exactly equals other.
    virtual bool equals(const ExposureContrastTransform & other) const noexcept = 0;

    virtual ExposureContrastStyle getStyle() const = 0;
    /// Select the algorithm for linear, video or log color spaces.
    virtual void setStyle(ExposureContrastStyle style) = 0;

    virtual double getExposure() const = 0;
    /**
     * Applies an exposure adjustment.  The value is in units of stops (regardless of style), for
     * example, a value of -1  would be equivalent to reducing the lighting by one half.
     */
    virtual void setExposure(double exposure) = 0;
    /**
     * Exposure can be made dynamic so the value can be changed through the CPU or GPU processor,
     * but if there are several ExposureContrastTransform only one can have a dynamic exposure.
     */
    virtual bool isExposureDynamic() const = 0;
    virtual void makeExposureDynamic() = 0;
    virtual void makeExposureNonDynamic() = 0;

    virtual double getContrast() const = 0;
    /**
     * Applies a contrast/gamma adjustment around a pivot point.  The contrast and gamma are
     * mathematically the same, but two controls are provided to enable the use of separate
     * dynamic parameters.  Contrast is usually a scene-referred adjustment that pivots around
     * gray whereas gamma is usually a display-referred adjustment that pivots around white.
     */
    virtual void setContrast(double contrast) = 0;
    /**
     * Contrast can be made dynamic so the value can be changed through the CPU or GPU processor,
     * but if there are several ExposureContrastTransform only one can have a dynamic contrast.
     */
    virtual bool isContrastDynamic() const = 0;
    virtual void makeContrastDynamic() = 0;
    virtual void makeContrastNonDynamic() = 0;

    virtual double getGamma() const = 0;
    virtual void setGamma(double gamma) = 0;
    /**
     * Gamma can be made dynamic so the value can be changed through the CPU or GPU processor,
     * but if there are several ExposureContrastTransform only one can have a dynamic gamma.
     */
    virtual bool isGammaDynamic() const = 0;
    virtual void makeGammaDynamic() = 0;
    virtual void makeGammaNonDynamic() = 0;

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

    /// Do not use (needed only for pybind11).
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

    TransformType getTransformType() const noexcept override { return TRANSFORM_TYPE_FILE; }

    /// Will throw if data is not valid.
    void validate() const override;

    const char * getSrc() const;
    void setSrc(const char * src);

    /**
     * The cccid can be the ID of a CDL or the index of the CDL (as string). If cccid is NULL or
     * empty the first CDL is returned.  The cccid is case-sensitive.
     */
    const char * getCCCId() const;
    void setCCCId(const char * id);

    CDLStyle getCDLStyle() const;
    /**
     * Can be used with CDL, CC & CCC formats to specify the clamping behavior of
     * the CDLTransform. Default is CDL_NO_CLAMP.
     */
    void setCDLStyle(CDLStyle);

    /**
     * The file parsers that care about interpolation (LUTs) will try to make use of the requested
     * interpolation method when loading the file.  In these cases, if the requested method could
     * not be used, a warning is logged.  If no method is provided, or a method cannot be used,
     * INTERP_DEFAULT is used.
     */
    Interpolation getInterpolation() const;
    void setInterpolation(Interpolation interp);

    /// Get the number of LUT readers.
    static int GetNumFormats();
    /// Get the LUT readers at index, return empty string if an invalid index is specified.
    static const char * GetFormatNameByIndex(int index);
    /// Get the LUT reader extension at index, return empty string if an invalid index is specified.
    static const char * GetFormatExtensionByIndex(int index);

    FileTransform & operator=(const FileTransform &) = delete;
    /// Do not use (needed only for pybind11).
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
    static FixedFunctionTransformRcPtr Create(FixedFunctionStyle style);
    static FixedFunctionTransformRcPtr Create(FixedFunctionStyle style,
                                              const double * params,
                                              size_t num);

    TransformType getTransformType() const noexcept override { return TRANSFORM_TYPE_FIXED_FUNCTION; }

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
    /// Do not use (needed only for pybind11).
    virtual ~FixedFunctionTransform() = default;

protected:
    FixedFunctionTransform() = default;
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const FixedFunctionTransform &);


/**
 * Primary color correction controls.
 *
 * This transform is for making basic color correction adjustments to an image such as brightness,
 * contrast, or saturation.
 *
 * The controls are customized for linear, logarithmic, and video color encodings.
 * * Linear controls: Exposure, Contrast, Pivot, Offset, Saturation, Black Clip, White Clip.
 * * Log controls: Brightness, Contrast, Pivot, Log Gamma, Saturation, Black Clip, White Clip,
 *                 Black Pivot White Pivot.
 * * Video controls : Lift, Gamma, Gain, Offset, Saturation, Black Clip, White Clip,
 *                    Black Pivot White Pivot.
 *
 * The controls are dynamic, so they may be adjusted even after the Transform has been included
 * in a Processor.
 */
class OCIOEXPORT GradingPrimaryTransform : public Transform
{
public:
    /// Creates an instance of GradingPrimaryTransform.
    static GradingPrimaryTransformRcPtr Create(GradingStyle style);

    TransformType getTransformType() const noexcept override { return TRANSFORM_TYPE_GRADING_PRIMARY; }

    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    /// Checks if this equals other.
    virtual bool equals(const GradingPrimaryTransform & other) const noexcept = 0;

    /// Adjusts the behavior of the transform for log, linear, or video color space encodings.
    virtual GradingStyle getStyle() const noexcept = 0;
    /// Will reset value to style's defaults if style is not the current style.
    virtual void setStyle(GradingStyle style) noexcept = 0;

    virtual const GradingPrimary & getValue() const = 0;
    /// Throws if value is not valid.
    virtual void setValue(const GradingPrimary & values) = 0;

    /**
     * Parameters can be made dynamic so the values can be changed through the CPU or GPU processor,
     * but if there are several GradingPrimaryTransform only one can have dynamic parameters.
     */
    virtual bool isDynamic() const noexcept = 0;
    virtual void makeDynamic() noexcept = 0;
    virtual void makeNonDynamic() noexcept = 0;

    GradingPrimaryTransform(const GradingPrimaryTransform &) = delete;
    GradingPrimaryTransform & operator= (const GradingPrimaryTransform &) = delete;
    /// Do not use (needed only for pybind11).
    virtual ~GradingPrimaryTransform() = default;

protected:
    GradingPrimaryTransform() = default;
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const GradingPrimaryTransform &) noexcept;


/**
 * RGB curve color correction controls.
 *
 * This transform allows for modifying tone reproduction via B-spline curves.
 *
 * There is an R, G, and B curve along with a Master curve (that applies to R, G, and B).  Each
 * curve is specified via the x and y coordinates of its control points.  A monotonic spline is
 * fit to the control points.  The x coordinates must be non-decreasing. When the grading style
 * is linear, the units for the control points are photographic stops relative to 0.18.
 *
 * The control points are dynamic, so they may be adjusted even after the Transform is included
 * in a Processor.
 */
class OCIOEXPORT GradingRGBCurveTransform : public Transform
{
public:
    /// Creates an instance of GradingPrimaryTransform.
    static GradingRGBCurveTransformRcPtr Create(GradingStyle style);

    TransformType getTransformType() const noexcept override { return TRANSFORM_TYPE_GRADING_RGB_CURVE; }

    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    /// Checks if this equals other.
    virtual bool equals(const GradingRGBCurveTransform & other) const noexcept = 0;

    /// Adjusts the behavior of the transform for log, linear, or video color space encodings.
    virtual GradingStyle getStyle() const noexcept = 0;
    /// Will reset value to style's defaults if style is not the current style.
    virtual void setStyle(GradingStyle style) noexcept = 0;

    virtual const ConstGradingRGBCurveRcPtr getValue() const = 0;
    /// Throws if value is not valid.
    virtual void setValue(const ConstGradingRGBCurveRcPtr & values) = 0;

    /**
     * It is possible to provide a desired slope value for each control point.  The number of slopes is 
     * always the same as the number of control points and so the control points must be set before 
     * setting the slopes.  The slopes are primarily intended for use by config authors looking to match
     * a specific shape with as few control points as possible, they are not intended to be exposed to
     * a user interface for direct manipulation.  When a curve is being generated for creative purposes
     * it is better to let OCIO calculate the slopes automatically.
     */
    virtual float getSlope(RGBCurveType c, size_t index) const = 0;
    virtual void setSlope(RGBCurveType c, size_t index, float slope) = 0;
    virtual bool slopesAreDefault(RGBCurveType c) const = 0;

    /**
     * The scene-linear grading style applies a lin-to-log transform to the pixel
     * values before going through the curve.  However, in some cases (e.g. drawing curves in a UI)
     * it may be useful to bypass the lin-to-log. Default value is false.
     */
    virtual bool getBypassLinToLog() const = 0;
    virtual void setBypassLinToLog(bool bypass) = 0;

    /**
     * Parameters can be made dynamic so the values can be changed through the CPU or GPU processor,
     * but if there are several GradingRGBCurveTransform only one can have dynamic parameters.
     */
    virtual bool isDynamic() const noexcept = 0;
    virtual void makeDynamic() noexcept = 0;
    virtual void makeNonDynamic() noexcept = 0;

    GradingRGBCurveTransform(const GradingRGBCurveTransform &) = delete;
    GradingRGBCurveTransform & operator= (const GradingRGBCurveTransform &) = delete;
    /// Do not use (needed only for pybind11).
    virtual ~GradingRGBCurveTransform() = default;

protected:
    GradingRGBCurveTransform() = default;
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const GradingRGBCurveTransform &) noexcept;


/**
 * Tonal color correction controls.
 *
 * This transform is for making fine adjustments to tone reproduction in specific tonal ranges.
 *
 * There are five tonal controls and each one has two parameters to control its range:
 * * Blacks (start, width)
 * * Shadows(start, pivot)
 * * Midtones(center, width)
 * * Highlights(start, pivot)
 * * Whites(start, width)
 *
 * The transform has three styles that adjust the response and default ranges for linear,
 * logarithimic, and video color encodings. The defaults vary based on the style.  When the
 * style is linear, the units for start/width/etc. are photographic stops relative to 0.18.
 *
 * Each control allows R, G, B adjustments and a Master adjustment.
 *
 * There is also an S-contrast control for imparting an S-shape curve.
 * 
 * The controls are dynamic, so they may be adjusted even after the Transform has been included
 * in a Processor.
 */
class OCIOEXPORT GradingToneTransform : public Transform
{
public:
    /// Creates an instance of GradingToneTransform.
    static GradingToneTransformRcPtr Create(GradingStyle style);

    TransformType getTransformType() const noexcept override { return TRANSFORM_TYPE_GRADING_TONE; }

    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    virtual bool equals(const GradingToneTransform & other) const noexcept = 0;

    /// Adjusts the behavior of the transform for log, linear, or video color space encodings.
    virtual GradingStyle getStyle() const noexcept = 0;
    /// Will reset value to style's defaults if style is not the current style.
    virtual void setStyle(GradingStyle style) noexcept = 0;

    virtual const GradingTone & getValue() const = 0;
    virtual void setValue(const GradingTone & values) = 0;

    /**
     * Parameters can be made dynamic so the values can be changed through the CPU or GPU processor,
     * but if there are several GradingToneTransform only one can have dynamic parameters.
     */
    virtual bool isDynamic() const noexcept = 0;
    virtual void makeDynamic() noexcept = 0;
    virtual void makeNonDynamic() noexcept = 0;

    GradingToneTransform(const GradingToneTransform &) = delete;
    GradingToneTransform & operator= (const GradingToneTransform &) = delete;
    /// Do not use (needed only for pybind11).
    virtual ~GradingToneTransform() = default;

protected:
    GradingToneTransform() = default;
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const GradingToneTransform &) noexcept;


class OCIOEXPORT GroupTransform : public Transform
{
public:
    static GroupTransformRcPtr Create();

    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    /// Throws if index is not allowed.
    virtual ConstTransformRcPtr getTransform(int index) const = 0;

    /// Throws if index is not allowed.
    virtual TransformRcPtr & getTransform(int index) = 0;

    /// Return number of transforms.
    virtual int getNumTransforms() const noexcept = 0;
    /// Adds a transform to the end of the group.
    virtual void appendTransform(TransformRcPtr transform) noexcept = 0;
    /// Add a transform at the beginning of the group.
    virtual void prependTransform(TransformRcPtr transform) noexcept = 0;

    /**
     * \brief Write the transforms comprising the group to the stream.
     *
     * Writing (as opposed to Baking) is a lossless process. An exception is thrown if the
     * processor cannot be losslessly written to the specified file format. Transforms such as
     * FileTransform or ColorSpaceTransform are resolved into write-able simple transforms using
     * the config and context.  Supported formats include CTF, CLF, and CDL. All available formats
     * can be listed with the following:
     * @code
     * // What are the allowed writing output formats?
     * std::ostringstream formats;
     * formats << "Formats to write to: ";
     * for (int i = 0; i < GroupTransform::GetNumWriteFormats(); ++i)
     * {
     *    if (i != 0) formats << ", ";
     *    formats << GroupTransform::GetFormatNameByIndex(i);
     *    formats << " (." << GroupTransform::GetFormatExtensionByIndex(i) << ")";
     * }
     * @endcode
     */
    virtual void write(const ConstConfigRcPtr & config,
                       const char * formatName,
                       std::ostream & os) const = 0;

    /// Get the number of writers.
    static int GetNumWriteFormats() noexcept;

    /// Get the writer at index, return empty string if an invalid index is specified.
    static const char * GetFormatNameByIndex(int index) noexcept;
    static const char * GetFormatExtensionByIndex(int index) noexcept;

    GroupTransform(const GroupTransform &) = delete;
    GroupTransform & operator=(const GroupTransform &) = delete;
    /// Do not use (needed only for pybind11).
    virtual ~GroupTransform() = default;

protected:
    GroupTransform() = default;
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const GroupTransform &);



/**
 * Applies a logarithm with an affine transform before and after.
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

    TransformType getTransformType() const noexcept override { return TRANSFORM_TYPE_LOG_AFFINE; }

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
    /// Do not use (needed only for pybind11).
    virtual ~LogAffineTransform() = default;

protected:
    LogAffineTransform() = default;
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const LogAffineTransform &);


/**
 * Same as LogAffineTransform but with the addition of a linear segment near black. This formula
 * is used for many camera logs (e.g., LogC) as well as ACEScct.
 *
 * * The linSideBreak specifies the point on the linear axis where the log and linear
 *   segments meet.  It must be set (there is no default).  
 * * The linearSlope specifies the slope of the linear segment of the forward (linToLog)
 *   transform.  By default it is set equal to the slope of the log curve at the break point.
 */
class OCIOEXPORT LogCameraTransform : public Transform
{
public:
    /// LinSideBreak must be set for the transform to be valid (there is no default).
    static LogCameraTransformRcPtr Create(const double(&linSideBreakValues)[3]);

    TransformType getTransformType() const noexcept override { return TRANSFORM_TYPE_LOG_CAMERA; }

    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    /// Checks if this exactly equals other.
    virtual bool equals(const LogCameraTransform & other) const noexcept = 0;

    virtual double getBase() const noexcept = 0;
    virtual void setBase(double base) noexcept = 0;

    /// Get/Set values for the R, G, B components.
    virtual void getLogSideSlopeValue(double(&values)[3]) const noexcept = 0;
    virtual void setLogSideSlopeValue(const double(&values)[3]) noexcept = 0;
    virtual void getLogSideOffsetValue(double(&values)[3]) const noexcept = 0;
    virtual void setLogSideOffsetValue(const double(&values)[3]) noexcept = 0;
    virtual void getLinSideSlopeValue(double(&values)[3]) const noexcept = 0;
    virtual void setLinSideSlopeValue(const double(&values)[3]) noexcept = 0;
    virtual void getLinSideOffsetValue(double(&values)[3]) const noexcept = 0;
    virtual void setLinSideOffsetValue(const double(&values)[3]) noexcept = 0;
    virtual void getLinSideBreakValue(double(&values)[3]) const noexcept = 0;
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
    /// Do not use (needed only for pybind11).
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

    TransformType getTransformType() const noexcept override { return TRANSFORM_TYPE_LOG; }

    virtual const FormatMetadata & getFormatMetadata() const noexcept = 0;
    virtual FormatMetadata & getFormatMetadata() noexcept = 0;

    /// Checks if this exactly equals other.
    virtual bool equals(const LogTransform & other) const noexcept = 0;

    virtual double getBase() const noexcept = 0;
    virtual void setBase(double val) noexcept = 0;

    LogTransform(const LogTransform &) = delete;
    LogTransform & operator= (const LogTransform &) = delete;
    /// Do not use (needed only for pybind11).
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

    TransformType getTransformType() const noexcept override { return TRANSFORM_TYPE_LOOK; }

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

    TransformType getTransformType() const noexcept override { return TRANSFORM_TYPE_LUT1D; }

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
    virtual void setHueAdjust(Lut1DHueAdjust algo) = 0;

    virtual Interpolation getInterpolation() const = 0;
    virtual void setInterpolation(Interpolation algo) = 0;

    Lut1DTransform(const Lut1DTransform &) = delete;
    Lut1DTransform & operator= (const Lut1DTransform &) = delete;
    /// Do not use (needed only for pybind11).
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

    TransformType getTransformType() const noexcept override { return TRANSFORM_TYPE_LUT3D; }

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
    /// Do not use (needed only for pybind11).
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

    TransformType getTransformType() const noexcept override { return TRANSFORM_TYPE_MATRIX; }

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
    /// Do not use (needed only for pybind11).
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

    TransformType getTransformType() const noexcept override { return TRANSFORM_TYPE_RANGE; }

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
    /// Do not use (needed only for pybind11).
    virtual ~RangeTransform() = default;

protected:
    RangeTransform() = default;
};

extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const RangeTransform &) noexcept;

} // namespace OCIO_NAMESPACE

#endif

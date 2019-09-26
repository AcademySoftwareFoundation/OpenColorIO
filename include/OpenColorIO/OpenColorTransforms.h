// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_OPENCOLORTRANSFORMS_H
#define INCLUDED_OCIO_OPENCOLORTRANSFORMS_H

#include "OpenColorTypes.h"

#ifndef OCIO_NAMESPACE_ENTER
#error This header cannot be used directly. Use <OpenColorIO/OpenColorIO.h> instead.
#endif

/*!rst::
C++ Transforms
==============

Typically only needed when creating and/or manipulating configurations
*/

OCIO_NAMESPACE_ENTER
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
    class FormatMetadata
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

        //!cpp:function:: Add a child element with a given name. Name has to be
        // non-empty. Return a reference to the added element.
        virtual FormatMetadata & addChildElement(const char * name) = 0;

        //!cpp:function:: Add a child element with a given name and value. Name
        // has to be non-empty. Return a reference to the added element.
        virtual FormatMetadata & addChildElement(const char * name, const char * value) = 0;

        //!cpp:function::
        virtual void clear() = 0;
        //!cpp:function::
        virtual FormatMetadata & operator=(const FormatMetadata & rhs) = 0;

    protected:
        FormatMetadata();
        virtual ~FormatMetadata();
};

    //!rst:: //////////////////////////////////////////////////////////////////

    //!cpp:class:: Base class for all the transform classes
    class OCIOEXPORT Transform
    {
    public:
        virtual TransformRcPtr createEditableCopy() const = 0;

        virtual TransformDirection getDirection() const = 0;
        //!cpp:function::
        virtual void setDirection(TransformDirection dir) = 0;

        //!cpp:function:: Will throw if data is not valid.
        virtual void validate() const;

    protected:
        Transform() = default;
        virtual ~Transform() = default;

    private:
        Transform(const Transform &) = delete;
        Transform & operator= (const Transform &) = delete;
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
        virtual TransformRcPtr createEditableCopy() const;

        //!cpp:function::
        virtual TransformDirection getDirection() const;
        //!cpp:function::
        virtual void setDirection(TransformDirection dir);

        //!cpp:function:: Will throw if data is not valid.
        virtual void validate() const;

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

    private:
        AllocationTransform();
        AllocationTransform(const AllocationTransform &);
        virtual ~AllocationTransform();

        AllocationTransform & operator= (const AllocationTransform &);

        static void deleter(AllocationTransform * t);

        class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };

    //!cpp:function::
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const AllocationTransform&);


    //!rst:: //////////////////////////////////////////////////////////////////

    //!cpp:class:: An implementation of the ASC CDL Transfer Functions and
    // Interchange - Syntax (Based on the version 1.2 document)
    //
    // .. note::
    //    the clamping portion of the CDL is only applied if a non-identity
    //    power is specified.
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
        virtual TransformRcPtr createEditableCopy() const;

        //!cpp:function::
        virtual TransformDirection getDirection() const;
        //!cpp:function::
        virtual void setDirection(TransformDirection dir);

        //!cpp:function:: Will throw if data is not valid.
        virtual void validate() const;

        //!cpp:function::
        FormatMetadata & getFormatMetadata();
        //!cpp:function::
        const FormatMetadata & getFormatMetadata() const;

        //!cpp:function::
        bool equals(const ConstCDLTransformRcPtr & other) const;

        //!cpp:function::
        const char * getXML() const;
        //!cpp:function::
        void setXML(const char * xml);

        //!rst:: **ASC_SOP**
        //
        // Slope, offset, power::
        //
        //    out = clamp( (in * slope) + offset ) ^ power

        //!cpp:function::
        void getSlope(double * rgb) const;
        //!cpp:function::
        void setSlope(const double * rgb);

        void getSlope(float * rgb) const;
        void setSlope(const float * rgb);

        //!cpp:function::
        void getOffset(double * rgb) const;
        //!cpp:function::
        void setOffset(const double * rgb);

        void getOffset(float * rgb) const;
        void setOffset(const float * rgb);

        //!cpp:function::
        void getPower(double * rgb) const;
        //!cpp:function::
        void setPower(const double * rgb);

        void getPower(float * rgb) const;
        void setPower(const float * rgb);

        //!cpp:function::
        void getSOP(double * vec9) const;
        //!cpp:function::
        void setSOP(const double * vec9);

        void getSOP(float * vec9) const;
        void setSOP(const float * vec9);

        //!rst:: **ASC_SAT**
        //
        
        //!cpp:function::
        double getSat() const;
        //!cpp:function::
        void setSat(double sat);
        
        //!cpp:function:: These are hard-coded, by spec, to r709.
        void getSatLumaCoefs(double * rgb) const;
        void getSatLumaCoefs(float * rgb) const;

        //!rst:: **Metadata**
        //
        // These do not affect the image processing, but
        // are often useful for pipeline purposes and are
        // included in the serialization.
        
        //!cpp:function:: Unique Identifier for this correction.
        const char * getID() const;
        //!cpp:function::
        void setID(const char * id);
        
        //!cpp:function:: Deprecated. Use `getFormatMetadata`.
        // First textual description of color correction (stored
        // on the SOP). If there is already a description, the setter will
        // replace it with the supplied text.
        const char * getDescription() const;
        //!cpp:function:: Deprecated. Use `getFormatMetadata`.
        void setDescription(const char * desc);
    
    private:
        CDLTransform();
        CDLTransform(const CDLTransform &);
        virtual ~CDLTransform();
        
        CDLTransform & operator=(const CDLTransform &);
        
        static void deleter(CDLTransform * t);
        
        class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
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
        virtual TransformRcPtr createEditableCopy() const;

        //!cpp:function::
        virtual TransformDirection getDirection() const;
        //!cpp:function::
        virtual void setDirection(TransformDirection dir);

        //!cpp:function:: Will throw if data is not valid.
        virtual void validate() const;

        //!cpp:function::
        const char * getSrc() const;
        //!cpp:function::
        void setSrc(const char * src);

        //!cpp:function::
        const char * getDst() const;
        //!cpp:function::
        void setDst(const char * dst);

    private:
        ColorSpaceTransform();
        ColorSpaceTransform(const ColorSpaceTransform &);
        virtual ~ColorSpaceTransform();
        
        ColorSpaceTransform & operator=(const ColorSpaceTransform &);
        
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
        virtual TransformRcPtr createEditableCopy() const;

        //!cpp:function::
        virtual TransformDirection getDirection() const;
        //!cpp:function::
        virtual void setDirection(TransformDirection dir);

        //!cpp:function:: Will throw if data is not valid.
        virtual void validate() const;
       
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
        // look specification. (And forward is assumed in the absense of either) 
        void setLooksOverride(const char * looks);

        //!cpp:function::
        bool getLooksOverrideEnabled() const;
        //!cpp:function:: Specifiy whether the lookOverride should be used,
        // or not. This is a speparate flag, as it's often useful to override
        // "looks" to an empty string. 
        void setLooksOverrideEnabled(bool enabled);
        
    private:
        DisplayTransform();
        DisplayTransform(const DisplayTransform &);
        virtual ~DisplayTransform();
        
        DisplayTransform & operator=(const DisplayTransform &);
        
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

    protected:

        DynamicProperty();
        DynamicProperty(const DynamicProperty &);
        virtual ~DynamicProperty();

        DynamicProperty & operator=(const DynamicProperty &);
    };

    //!rst:: //////////////////////////////////////////////////////////////////

    //!cpp:class:: Represents exponent transform: pow( clamp(color), value)
    //
    // For configs with version == 1: If the exponent is 1.0, this will not clamp.
    // Otherwise, the input color will be clamped between [0.0, inf].
    // For configs with version > 1: Negative values are always clamped.
    class OCIOEXPORT ExponentTransform : public Transform
    {
    public:
        //!cpp:function::
        static ExponentTransformRcPtr Create();

        //!cpp:function::
        virtual TransformRcPtr createEditableCopy() const;

        //!cpp:function::
        virtual TransformDirection getDirection() const;
        //!cpp:function::
        virtual void setDirection(TransformDirection dir);

        //!cpp:function:: Will throw if data is not valid.
        virtual void validate() const;

        //!cpp:function::
        const FormatMetadata & getFormatMetadata() const;
        //!cpp:function::
        FormatMetadata & getFormatMetadata();

        //!cpp:function::
        void getValue(double(&vec4)[4]) const;
        //!cpp:function::
        void setValue(const double(&vec4)[4]);

        void getValue(float * vec4) const;
        void setValue(const float * vec4);

    private:
        ExponentTransform();
        ExponentTransform(const ExponentTransform &);
        virtual ~ExponentTransform();
        
        ExponentTransform & operator=(const ExponentTransform &);
        
        static void deleter(ExponentTransform * t);
        
        class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
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
        virtual TransformRcPtr createEditableCopy() const;

        //!cpp:function::
        virtual TransformDirection getDirection() const;
        //!cpp:function::
        virtual void setDirection(TransformDirection dir);

        //!cpp:function:: Validate the transform and throw if invalid.
        virtual void validate() const;

        //!cpp:function::
        const FormatMetadata & getFormatMetadata() const;
        //!cpp:function::
        FormatMetadata & getFormatMetadata();

        //!cpp:function::
        void getGamma(double(&values)[4]) const;
        //!cpp:function:: Set the exponent value for the power function for R, G, B, A.
        //
        // .. note::
        //    The gamma values must be in the range of [1, 10]. Set the transform direction 
        //    to inverse to obtain the effect of values less than 1.
        void setGamma(const double(&values)[4]);

        //!cpp:function::
        void getOffset(double(&values)[4]) const;
        //!cpp:function:: Set the offset value for the power function for R, G, B, A.
        //
        // .. note::
        //    The offset values must be in the range [0, 0.9].
        void setOffset(const double(&values)[4]);

    private:
        ExponentWithLinearTransform();
        ExponentWithLinearTransform(const ExponentWithLinearTransform &);
        virtual ~ExponentWithLinearTransform();
        
        ExponentWithLinearTransform & operator=(const ExponentWithLinearTransform &);
        
        static void deleter(ExponentWithLinearTransform * t);
        
        class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
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
        virtual TransformRcPtr createEditableCopy() const;

        //!cpp:function::
        virtual TransformDirection getDirection() const;
        //!cpp:function::
        virtual void setDirection(TransformDirection dir);

        //!cpp:function:: Will throw if data is not valid.
        virtual void validate() const;

        //!cpp:function::
        const FormatMetadata & getFormatMetadata() const;
        //!cpp:function::
        FormatMetadata & getFormatMetadata();

        //!cpp:function::
        ExposureContrastStyle getStyle() const;
        //!cpp:function:: Select the algorithm for linear, video
        // or log color spaces.
        void setStyle(ExposureContrastStyle style);

        //!cpp:function::
        double getExposure() const;
        //!cpp:function:: Applies an exposure adjustment.  The value is in
        // units of stops (regardless of style), for example, a value of -1
        // would be equivalent to reducing the lighting by one half.
        void setExposure(double exposure);
        //!cpp:function::
        bool isExposureDynamic() const;
        //!cpp:function::
        void makeExposureDynamic();

        //!cpp:function::
        double getContrast() const;
        //!cpp:function:: Applies a contrast/gamma adjustment around a pivot
        // point.  The contrast and gamma are mathematically the same, but two
        // controls are provided to enable the use of separate dynamic
        // parameters.  Contrast is usually a scene-referred adjustment that
        // pivots around gray whereas gamma is usually a display-referred
        // adjustment that pivots around white.
        void setContrast(double contrast);
        //!cpp:function::
        bool isContrastDynamic() const;
        //!cpp:function::
        void makeContrastDynamic();
        //!cpp:function::
        double getGamma() const;
        //!cpp:function::
        void setGamma(double gamma);
        //!cpp:function::
        bool isGammaDynamic() const;
        //!cpp:function::
        void makeGammaDynamic();

        //!cpp:function::
        double getPivot() const;
        //!cpp:function:: Set the pivot point around which the contrast
        // and gamma controls will work. Regardless of whether
        // linear/video/log-style is being used, the pivot is always expressed
        // in linear. In other words, a pivot of 0.18 is always mid-gray.
        void setPivot(double pivot);

        //!cpp:function:: 
        double getLogExposureStep() const;
        //!cpp:function:: Set the increment needed to move one stop for
        // the log-style algorithm. For example, ACEScct is 0.057, LogC is
        // roughly 0.074, and Cineon is roughly 90/1023 = 0.088.
        // The default value is 0.088.
        void setLogExposureStep(double logExposureStep);

        //!cpp:function:: 
        double getLogMidGray() const;
        //!cpp:function:: Set the position of 18% gray for use by the
        // log-style algorithm. For example, ACEScct is about 0.41, LogC is
        // about 0.39, and ADX10 is 445/1023 = 0.435.
        // The default value is 0.435.
        void setLogMidGray(double logMidGray);

    private:
        ExposureContrastTransform();
        ExposureContrastTransform(const ExposureContrastTransform &);
        virtual ~ExposureContrastTransform();

        ExposureContrastTransform & operator=(const ExposureContrastTransform &);

        static void deleter(ExposureContrastTransform * t);

        class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
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
        virtual TransformRcPtr createEditableCopy() const;

        //!cpp:function::
        virtual TransformDirection getDirection() const;
        //!cpp:function::
        virtual void setDirection(TransformDirection dir);

        //!cpp:function:: Will throw if data is not valid.
        virtual void validate() const;

        //!cpp:function::
        const char * getSrc() const;
        //!cpp:function::
        void setSrc(const char * src);

        //!cpp:function::
        const char * getCCCId() const;
        //!cpp:function::
        void setCCCId(const char * id);

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

    private:
        FileTransform();
        FileTransform(const FileTransform &);
        virtual ~FileTransform();
        
        FileTransform & operator=(const FileTransform &);
        
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
        virtual TransformRcPtr createEditableCopy() const;

        //!cpp:function::
        virtual TransformDirection getDirection() const;
        //!cpp:function::
        virtual void setDirection(TransformDirection dir);

        //!cpp:function:: Will throw if data is not valid.
        virtual void validate() const;

        //!cpp:function::
        const FormatMetadata & getFormatMetadata() const;
        //!cpp:function::
        FormatMetadata & getFormatMetadata();

        //!cpp:function::
        FixedFunctionStyle getStyle() const;
        //!cpp:function:: Select which algorithm to use.
        void setStyle(FixedFunctionStyle style);

        //!cpp:function::
        size_t getNumParams() const;
        //!cpp:function::
        void getParams(double * params) const;
        //!cpp:function:: Set the parameters (for functions that require them).
        void setParams(const double * params, size_t num);

    private:
        FixedFunctionTransform();
        FixedFunctionTransform(const FixedFunctionTransform &);
        virtual ~FixedFunctionTransform();
        
        FixedFunctionTransform & operator=(const FixedFunctionTransform &);
        
        static void deleter(FixedFunctionTransform * t);

        class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
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
        virtual TransformRcPtr createEditableCopy() const;

        //!cpp:function::
        virtual TransformDirection getDirection() const;
        //!cpp:function::
        virtual void setDirection(TransformDirection dir);

        //!cpp:function:: Will throw if data is not valid.
        virtual void validate() const;

        //!cpp:function::
        virtual const FormatMetadata & getFormatMetadata() const;
        //!cpp:function::
        virtual FormatMetadata & getFormatMetadata();

        //!cpp:function::
        ConstTransformRcPtr getTransform(int index) const;

        //!cpp:function::
        TransformRcPtr & getTransform(int index);

        //!cpp:function::
        int size() const;
        //!cpp:function:: Adds a copy of transform to the group.
        void push_back(const ConstTransformRcPtr & transform);
        //!cpp:function:: Adds transform to the group.
        void push_back(TransformRcPtr & transform);
        //!cpp:function:: Clears transforms (not the FormatMetadata).
        void clear();
        //!cpp:function::
        bool empty() const;

    private:
        GroupTransform();
        GroupTransform(const GroupTransform &);
        virtual ~GroupTransform();
        
        GroupTransform & operator=(const GroupTransform &);
        
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
    class OCIOEXPORT LogAffineTransform : public Transform
    {
    public:
        //!cpp:function::
        static LogAffineTransformRcPtr Create();

        //!cpp:function::
        virtual TransformRcPtr createEditableCopy() const;

        //!cpp:function::
        virtual TransformDirection getDirection() const;
        //!cpp:function::
        virtual void setDirection(TransformDirection dir);

        //!cpp:function:: Will throw if data is not valid.
        virtual void validate() const;

        //!cpp:function::
        const FormatMetadata & getFormatMetadata() const;
        //!cpp:function::
        FormatMetadata & getFormatMetadata();

        //!cpp:function::
        double getBase() const;
        //!cpp:function::
        void setBase(double base);

        //!rst:: **Get/Set values for the R, G, B components**
        //

        //!cpp:function::
        void getLogSideSlopeValue(double(&values)[3]) const;
        //!cpp:function::
        void setLogSideSlopeValue(const double(&values)[3]);
        //!cpp:function::
        void getLogSideOffsetValue(double(&values)[3]) const;
        //!cpp:function::
        void setLogSideOffsetValue(const double(&values)[3]);
        //!cpp:function::
        void getLinSideSlopeValue(double(&values)[3]) const;
        //!cpp:function::
        void setLinSideSlopeValue(const double(&values)[3]);
        //!cpp:function::
        void getLinSideOffsetValue(double(&values)[3]) const;
        //!cpp:function::
        void setLinSideOffsetValue(const double(&values)[3]);

    private:
        LogAffineTransform();
        LogAffineTransform(const LogAffineTransform &);
        virtual ~LogAffineTransform();

        LogAffineTransform & operator=(const LogAffineTransform &);

        static void deleter(LogAffineTransform * t);

        class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };

    //!cpp:function::
    extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const LogAffineTransform &);


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
        virtual TransformRcPtr createEditableCopy() const;

        //!cpp:function::
        virtual TransformDirection getDirection() const;
        //!cpp:function::
        virtual void setDirection(TransformDirection dir);

        //!cpp:function:: Will throw if data is not valid.
        virtual void validate() const;

        //!cpp:function::
        const FormatMetadata & getFormatMetadata() const;
        //!cpp:function::
        FormatMetadata & getFormatMetadata();

        //!cpp:function::
        double getBase() const;
        //!cpp:function::
        void setBase(double val);

    private:
        LogTransform();
        LogTransform(const LogTransform &);
        virtual ~LogTransform();
        
        LogTransform & operator=(const LogTransform &);
        
        static void deleter(LogTransform * t);
        
        class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
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
        virtual TransformRcPtr createEditableCopy() const;

        //!cpp:function::
        virtual TransformDirection getDirection() const;
        //!cpp:function::
        virtual void setDirection(TransformDirection dir);

        //!cpp:function:: Will throw if data is not valid.
        virtual void validate() const;

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
        // look specification. (And forward is assumed in the absense of either)
        void setLooks(const char * looks);
        
    private:
        LookTransform();
        LookTransform(const LookTransform &);
        virtual ~LookTransform();
        
        LookTransform & operator=(const LookTransform &);
        
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
    class OCIOEXPORT LUT1DTransform : public Transform
    {
    public:
        //!cpp:function:: Create an identity 1D-LUT of length two.
        static LUT1DTransformRcPtr Create();

        //!cpp:function:: Create an identity 1D-LUT with specific length and
        // half-domain setting. Will throw for lengths longer than 1024x1024.
        static LUT1DTransformRcPtr Create(unsigned long length,
                                          bool isHalfDomain);

        //!cpp:function::
        virtual TransformRcPtr createEditableCopy() const;

        //!cpp:function::
        virtual TransformDirection getDirection() const;
        //!cpp:function:: Set the direction the 1D-LUT should be evaluated in.
        // Note that this only affects the evaluation and not the values
        // stored in the object.
        virtual void setDirection(TransformDirection dir);

        //!cpp:function:: Will throw if data is not valid.
        virtual void validate() const;

        //!cpp:function::
        BitDepth getFileOutputBitDepth() const;
        //!cpp:function:: Get the bit-depth associated with the LUT values read
        // from a file or set the bit-depth of values to be written to a file
        // (for file formats such as CLF that support multiple bit-depths).
        // However, note that the values stored in the object are always
        // normalized.
        void setFileOutputBitDepth(BitDepth bitDepth);

        //!cpp:function::
        const FormatMetadata & getFormatMetadata() const;
        //!cpp:function::
        FormatMetadata & getFormatMetadata();

        //!cpp:function::
        unsigned long getLength() const;
        //!cpp:function:: Changing the length will reset the LUT to identity.
        // Will throw for lengths longer than 1024x1024.
        void setLength(unsigned long length);

        //!cpp:function::
        void getValue(unsigned long index, float & r, float & g, float & b) const;
        //!cpp:function:: Set the values of a LUT1D.  Will throw if the index
        // is outside of the range from 0 to (length-1).
        //
        // These values are normalized relative to what may be stored in any
        // given LUT files. For example in a CLF file using a "10i" output
        // depth, a value of 1023 in the file is normalized to 1.0. The
        // values here are unclamped and may extend outside [0,1].
        //
        // LUTs in various file formats may only provide values for one
        // channel where R, G, B are the same. Even in that case, you should
        // provide three equal values to the setter.
        void setValue(unsigned long index, float r, float g, float b);

        //!cpp:function::
        bool getInputHalfDomain() const;
        //!cpp:function:: In a half-domain LUT, the contents of the LUT specify
        // the desired value of the function for each half-float value.
        // Therefore, the length of the LUT must be 65536 entries or else
        // validate() will throw.
        void setInputHalfDomain(bool isHalfDomain);

        //!cpp:function::
        bool getOutputRawHalfs() const;
        //!cpp:function:: Set OutputRawHalfs to true if you want to output the
        // LUT contents as 16-bit floating point values expressed as unsigned
        // 16-bit integers representing the equivalent bit pattern.
        // For example, the value 1.0 would be written as the integer 15360 
        // because it has the same bit-pattern.  Note that this implies the 
        // values will be quantized to a 16-bit float.  Note that this setting
        // only controls the output formatting (where supported) and not the 
        // values for getValue/setValue.  The only file formats that currently
        // support this are CLF and CTF.
        void setOutputRawHalfs(bool isRawHalfs);

        //!cpp:function::
        LUT1DHueAdjust getHueAdjust() const;
        //!cpp:function:: The 1D-LUT transform optionally supports a hue adjustment
        // feature that was used in some versions of ACES.  This adjusts the hue
        // of the result to approximately match the input.
        void setHueAdjust(LUT1DHueAdjust algo);

        //!cpp:function::
        Interpolation getInterpolation() const;
        //!cpp:function::
        void setInterpolation(Interpolation algo);

    private:
        LUT1DTransform();
        LUT1DTransform(unsigned long length,
                       bool isHalfDomain);
        LUT1DTransform(const LUT1DTransform &);
        virtual ~LUT1DTransform();

        LUT1DTransform& operator= (const LUT1DTransform &);

        static void deleter(LUT1DTransform* t);

        class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };

    //!cpp:function::
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const LUT1DTransform&);

    //!rst:: //////////////////////////////////////////////////////////////////

    //!cpp:class:: Represents a 3D-LUT transform.
    class OCIOEXPORT LUT3DTransform : public Transform
    {
    public:
        //!cpp:function:: Create an identity 3D-LUT of size 2x2x2.
        static LUT3DTransformRcPtr Create();

        //!cpp:function:: Create an identity 3D-LUT with specific grid size.
        // Will throw for grid size larger than 129.
        static LUT3DTransformRcPtr Create(unsigned long gridSize);

        //!cpp:function::
        virtual TransformRcPtr createEditableCopy() const;

        //!cpp:function::
        virtual TransformDirection getDirection() const;
        //!cpp:function:: Set the direction the 3D-LUT should be evaluated in.
        // Note that this only affects the evaluation and not the values
        // stored in the object.
        virtual void setDirection(TransformDirection dir);

        //!cpp:function:: Will throw if data is not valid.
        virtual void validate() const;

        //!cpp:function::
        BitDepth getFileOutputBitDepth() const;
        //!cpp:function:: Get the bit-depth associated with the LUT values read
        // from a file or set the bit-depth of values to be written to a file
        // (for file formats such as CLF that support multiple bit-depths).
        // However, note that the values stored in the object are always
        // normalized.
        void setFileOutputBitDepth(BitDepth bitDepth);

        //!cpp:function::
        const FormatMetadata & getFormatMetadata() const;
        //!cpp:function::
        FormatMetadata & getFormatMetadata();

        //!cpp:function::
        unsigned long getGridSize() const;
        //!cpp:function:: Changing the grid size will reset the LUT to identity.
        // Will throw for grid sizes larger than 129.
        void setGridSize(unsigned long gridSize);

        //!cpp:function::
        void getValue(unsigned long indexR,
                      unsigned long indexG,
                      unsigned long indexB,
                      float & r, float & g, float & b) const;
        //!cpp:function:: Set the values of a 3D-LUT. Will throw if an index is
        // outside of the range from 0 to (gridSize-1).
        //
        // These values are normalized relative to what may be stored in any
        // given LUT files. For example in a CLF file using a "10i" output
        // depth, a value of 1023 in the file is normalized to 1.0. The values
        // here are unclamped and may extend outside [0,1].
        void setValue(unsigned long indexR,
                      unsigned long indexG,
                      unsigned long indexB,
                      float r, float g, float b);

        //!cpp:function::
        Interpolation getInterpolation() const;
        //!cpp:function::
        void setInterpolation(Interpolation algo);

    private:
        LUT3DTransform();
        explicit LUT3DTransform(unsigned long gridSize);

        LUT3DTransform(const LUT3DTransform &);
        virtual ~LUT3DTransform();

        LUT3DTransform& operator= (const LUT3DTransform &);

        static void deleter(LUT3DTransform* t);

        class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };

    //!cpp:function::
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const LUT3DTransform&);

    //!rst:: //////////////////////////////////////////////////////////////////
    
    //!cpp:class:: Represents an MX+B Matrix transform
    class OCIOEXPORT MatrixTransform : public Transform
    {
    public:
        //!cpp:function::
        static MatrixTransformRcPtr Create();

        //!cpp:function::
        virtual TransformRcPtr createEditableCopy() const;

        //!cpp:function::
        virtual TransformDirection getDirection() const;
        //!cpp:function:: Set the direction the matrix should be evaluated in.
        // Note that this only affects the evaluation and not the values
        // stored in the object.
        //
        // For singular matrices, an inverse direction will throw an
        // exception during finalization
        virtual void setDirection(TransformDirection dir);

        //!cpp:function:: Will throw if data is not valid.
        virtual void validate() const;

        //!cpp:function::
        const FormatMetadata & getFormatMetadata() const;
        //!cpp:function::
        FormatMetadata & getFormatMetadata();

        //!cpp:function:: Checks if this exactly equals other.
        bool equals(const MatrixTransform & other) const;

        //!cpp:function::
        void getValue(float * m44, float * offset4) const;
        //!cpp:function::
        void setValue(const float * m44, const float * offset4);
        
        //!cpp:function::
        void getMatrix(double * m44) const;
        //!cpp:function:: Get or set the values of a Matrix. Expects 16 values,
        // where the first four are the coefficients to generate the R output
        // channel from R, G, B, A input channels.
        //
        // These values are normalized relative to what may be stored in
        // file formats such as CLF. For example in a CLF file using a "32f"
        // input depth and "10i" output depth, a value of 1023 in the file
        // is normalized to 1.0. The values here are unclamped and may
        // extend outside [0,1].
        void setMatrix(const double * m44);

        void getMatrix(float * m44) const;
        void setMatrix(const float * m44);
        
        //!cpp:function::
        void getOffset(double * offset4) const;
        //!cpp:function:: Get or set the R, G, B, A offsets to be applied
        // after the matrix.
        //
        // These values are normalized relative to what may be stored in
        // file formats such as CLF. For example, in a CLF file using a
        // "10i" output depth, a value of 1023 in the file is normalized
        // to 1.0. The values here are unclamped and may extend
        // outside [0,1].
        void setOffset(const double * offset4);
        
        void getOffset(float * offset4) const;
        void setOffset(const float * offset4);

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
        BitDepth getFileInputBitDepth() const;
        //!cpp:function::
        void setFileInputBitDepth(BitDepth bitDepth);
        //!cpp:function::
        BitDepth getFileOutputBitDepth() const;
        //!cpp:function::
        void setFileOutputBitDepth(BitDepth bitDepth);

        //!rst:: **Convenience functions**
        //
        // Build the matrix and offset corresponding to higher-level concepts.
        // 
        // .. note::
        //    These can throw an exception if for any component
        //    ``oldmin == oldmax. (divide by 0)``

        //!cpp:function::
        static void Fit(float * m44, float * offset4,
                        const float * oldmin4, const float * oldmax4,
                        const float * newmin4, const float * newmax4);
        static void Fit(double * m44, double* offset4,
                        const double * oldmin4, const double * oldmax4,
                        const double * newmin4, const double * newmax4);
        
        //!cpp:function::
        static void Identity(double * m44, double * offset4);
        static void Identity(float * m44, float * offset4);

        //!cpp:function::
        static void Sat(double * m44, double * offset4,
                        double sat, const double * lumaCoef3);
        static void Sat(float * m44, float * offset4,
                        float sat, const float * lumaCoef3);

        //!cpp:function::
        static void Scale(double * m44, double * offset4,
                          const double * scale4);
        static void Scale(float * m44, float * offset4,
                          const float * scale4);

        //!cpp:function::
        static void View(double * m44, double * offset4,
                         int * channelHot4,
                         const double * lumaCoef3);
        static void View(float * m44, float * offset4,
                         int * channelHot4,
                         const float * lumaCoef3);

    private:
        MatrixTransform();
        MatrixTransform(const MatrixTransform &);
        virtual ~MatrixTransform();
        
        MatrixTransform & operator=(const MatrixTransform &);
        
        static void deleter(MatrixTransform * t);
        
        class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };

    //!cpp:function::
    extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const MatrixTransform &);

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

        //!cpp:function:: Creates a copy of this.
        virtual TransformRcPtr createEditableCopy() const = 0;
        
        //!cpp:function::
        virtual TransformDirection getDirection() const = 0;
        //!cpp:function:: Set the direction the Range should be evaluated in.
        // Note that this only affects the evaluation and not the values stored
        // in the object.
        virtual void setDirection(TransformDirection dir) = 0;

        //!cpp:function::
        virtual RangeStyle getStyle() const = 0;
        //!cpp:function:: Set the Range style to clamp or not input values.
        virtual void setStyle(RangeStyle style) = 0;

        //!cpp:function:: Will throw if data is not valid.
        virtual void validate() const = 0;

        //!cpp:function::
        virtual const FormatMetadata & getFormatMetadata() const = 0;
        //!cpp:function::
        virtual FormatMetadata & getFormatMetadata() = 0;

        //!cpp:function:: Checks if this equals other.
        virtual bool equals(const RangeTransform & other) const = 0;

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
        virtual BitDepth getFileInputBitDepth() const = 0;
        //!cpp:function::
        virtual void setFileInputBitDepth(BitDepth bitDepth) = 0;
        //!cpp:function::
        virtual BitDepth getFileOutputBitDepth() const = 0;
        //!cpp:function::
        virtual void setFileOutputBitDepth(BitDepth bitDepth) = 0;

        //!rst:: **Range values**
        //
        // These values are normalized relative to what may be stored in file
        // formats such as CLF. For example in a CLF file using a "10i" input
        // depth, a MaxInValue of 1023 in the file is normalized to 1.0.
        // Likewise, for an output depth of "12i", a MaxOutValue of 4095 in the
        // file is normalized to 1.0. The values here are unclamped and may
        // extend outside [0,1].

        //!cpp:function:: Get the minimum value for the input.
        virtual double getMinInValue() const = 0;
        //!cpp:function:: Set the minimum value for the input.
        virtual void setMinInValue(double val) = 0;
        //!cpp:function:: Is the minimum value for the input set?
        virtual bool hasMinInValue() const = 0;
        //!cpp:function:: Unset the minimum value for the input
        virtual void unsetMinInValue() = 0;

        //!cpp:function:: Set the maximum value for the input.
        virtual void setMaxInValue(double val) = 0;
        //!cpp:function:: Get the maximum value for the input.
        virtual double getMaxInValue() const = 0;
        //!cpp:function:: Is the maximum value for the input set?
        virtual bool hasMaxInValue() const = 0;
        //!cpp:function:: Unset the maximum value for the input.
        virtual void unsetMaxInValue() = 0;

        //!cpp:function:: Set the minimum value for the output.
        virtual void setMinOutValue(double val) = 0;
        //!cpp:function:: Get the minimum value for the output.
        virtual double getMinOutValue() const = 0;
        //!cpp:function:: Is the minimum value for the output set?
        virtual bool hasMinOutValue() const = 0;
        //!cpp:function:: Unset the minimum value for the output.
        virtual void unsetMinOutValue() = 0;

        //!cpp:function:: Set the maximum value for the output.
        virtual void setMaxOutValue(double val) = 0;
        //!cpp:function:: Get the maximum value for the output.
        virtual double getMaxOutValue() const = 0;
        //!cpp:function:: Is the maximum value for the output set?
        virtual bool hasMaxOutValue() const = 0;
        //!cpp:function:: Unset the maximum value for the output.
        virtual void unsetMaxOutValue() = 0;

    protected:
        RangeTransform() = default;
        virtual ~RangeTransform() = default;

    private:
        RangeTransform(const RangeTransform &) = delete;
        RangeTransform & operator= (const RangeTransform &) = delete;
    };

    //!cpp:function::
    extern OCIOEXPORT std::ostream & operator<<(std::ostream &, const RangeTransform &);

}
OCIO_NAMESPACE_EXIT

#endif

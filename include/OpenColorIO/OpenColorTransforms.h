/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


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
    
    //!cpp:class:: Base class for all the transform classes
    class OCIOEXPORT Transform
    {
    public:
        virtual ~Transform();
        virtual TransformRcPtr createEditableCopy() const = 0;
        
        virtual TransformDirection getDirection() const = 0;
        virtual void setDirection(TransformDirection dir) = 0;

        //!cpp:function:: Will throw if data is not valid.
        virtual void validate() const;

    private:
        Transform& operator= (const Transform &);
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
        
        AllocationTransform& operator= (const AllocationTransform &);
        
        static void deleter(AllocationTransform* t);
        
        class Impl;
        friend class Impl;
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
        
        //!cpp:function::
        // Load the CDL from the src .cc or .ccc file.
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
        void setSlope(const float * rgb);
        //!cpp:function::
        void getSlope(float * rgb) const;
        
        //!cpp:function::
        void setOffset(const float * rgb);
        //!cpp:function::
        void getOffset(float * rgb) const;
        
        //!cpp:function::
        void setPower(const float * rgb);
        //!cpp:function::
        void getPower(float * rgb) const;
        
        //!cpp:function::
        void setSOP(const float * vec9);
        //!cpp:function::
        void getSOP(float * vec9) const;
        
        //!rst:: **ASC_SAT**
        
        //!cpp:function::
        void setSat(float sat);
        //!cpp:function::
        float getSat() const;
        
        //!cpp:function:: These are hard-coded, by spec, to r709
        void getSatLumaCoefs(float * rgb) const;
        
        //!rst:: **Metadata**
        // 
        // These do not affect the image processing, but
        // are often useful for pipeline purposes and are
        // included in the serialization.
        
        //!cpp:function:: Unique Identifier for this correction
        void setID(const char * id);
        //!cpp:function::
        const char * getID() const;
        
        //!cpp:function:: Textual description of color correction
        // (stored on the SOP)
        void setDescription(const char * desc);
        //!cpp:function::
        const char * getDescription() const;
    
    private:
        CDLTransform();
        CDLTransform(const CDLTransform &);
        virtual ~CDLTransform();
        
        CDLTransform& operator= (const CDLTransform &);
        
        static void deleter(CDLTransform* t);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };
    
    //!cpp:function::
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const CDLTransform&);
    
    
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
        
        ColorSpaceTransform& operator= (const ColorSpaceTransform &);
        
        static void deleter(ColorSpaceTransform* t);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };
    
    //!cpp:function::
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const ColorSpaceTransform&);
    
    
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
       
        //!cpp:function:: Step 0. Specify the incoming color space
        void setInputColorSpaceName(const char * name);
        //!cpp:function::
        const char * getInputColorSpaceName() const;
        
        //!cpp:function:: Step 1: Apply a Color Correction, in ROLE_SCENE_LINEAR
        void setLinearCC(const ConstTransformRcPtr & cc);
        //!cpp:function::
        ConstTransformRcPtr getLinearCC() const;
        
        //!cpp:function:: Step 2: Apply a color correction, in ROLE_COLOR_TIMING
        void setColorTimingCC(const ConstTransformRcPtr & cc);
        //!cpp:function::
        ConstTransformRcPtr getColorTimingCC() const;
        
        //!cpp:function:: Step 3: Apply the Channel Viewing Swizzle (mtx)
        void setChannelView(const ConstTransformRcPtr & transform);
        //!cpp:function::
        ConstTransformRcPtr getChannelView() const;
        
        //!cpp:function:: Step 4: Apply the output display transform
        // This is controlled by the specification of (display, view)
        void setDisplay(const char * display);
        //!cpp:function::
        const char * getDisplay() const;
        
        //!cpp:function::Specify which view transform to use
        void setView(const char * view);
        //!cpp:function::
        const char * getView() const;
        
        //!cpp:function:: Step 5: Apply a post display transform color correction
        void setDisplayCC(const ConstTransformRcPtr & cc);
        //!cpp:function::
        ConstTransformRcPtr getDisplayCC() const;
        
        
        
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
        const char * getLooksOverride() const;
        
        //!cpp:function:: Specifiy whether the lookOverride should be used,
        // or not. This is a speparate flag, as it's often useful to override
        // "looks" to an empty string
        void setLooksOverrideEnabled(bool enabled);
        //!cpp:function:: 
        bool getLooksOverrideEnabled() const;
        
    private:
        DisplayTransform();
        DisplayTransform(const DisplayTransform &);
        virtual ~DisplayTransform();
        
        DisplayTransform& operator= (const DisplayTransform &);
        
        static void deleter(DisplayTransform* t);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };
    
    //!cpp:function::
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const DisplayTransform&);
    
    
    //!rst:: //////////////////////////////////////////////////////////////////

    //!cpp:class:: Allows transform parameter values to be set on-the-fly
    //             (after finalization).  For example, to modify the exposure
    //             in a viewport.
    class OCIOEXPORT DynamicProperty
    {
    public:

        //!cpp:function:: 
        virtual DynamicPropertyValueType getValueType() const = 0;
        //!cpp:function:: 
        virtual double getDoubleValue() const = 0;
        //!cpp:function:: 
        virtual void setValue(double value) = 0;

    protected:

        DynamicProperty() {}
        virtual ~DynamicProperty() {}

        DynamicProperty& operator= (const DynamicProperty &);
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
        void setValue(const float * vec4);
        void setValue(const double(&vec4)[4]);
        //!cpp:function::
        void getValue(float * vec4) const;
        void getValue(double(&vec4)[4]) const;
    
    private:
        ExponentTransform();
        ExponentTransform(const ExponentTransform &);
        virtual ~ExponentTransform();
        
        ExponentTransform& operator= (const ExponentTransform &);
        
        static void deleter(ExponentTransform* t);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };
    
    //!cpp:function::
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const ExponentTransform&);
    
    
    //!rst:: //////////////////////////////////////////////////////////////////
    
    //!cpp:class:: Represents power functions with a linear section in the shadows 
    //             such as sRGB and L*.
    //
    // The basic formula is:
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

        //!cpp:function:: Set the exponent value for the power function for R, G, B, A.
        // .. note::
        //     The gamma values must be in the range of [1, 10]. Set the transform direction 
        //     to inverse to obtain the effect of values less than 1.
        void setGamma(const double(&values)[4]);
        //!cpp:function::
        void getGamma(double(&values)[4]) const;

        //!cpp:function::
        // .. note:: The offset values must be in the range [0, 0.9].
        void setOffset(const double(&values)[4]);
        //!cpp:function::
        void getOffset(double(&values)[4]) const;

    private:
        ExponentWithLinearTransform();
        ExponentWithLinearTransform(const ExponentWithLinearTransform &);
        virtual ~ExponentWithLinearTransform();
        
        ExponentWithLinearTransform& operator= (const ExponentWithLinearTransform &);
        
        static void deleter(ExponentWithLinearTransform* t);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };

    //!cpp:function::
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const ExponentWithLinearTransform&);

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
        virtual void setDirection(TransformDirection dir);
        //!cpp:function::
        virtual TransformDirection getDirection() const;

        //!cpp:function:: Will throw if data is not valid.
        virtual void validate() const;

        //!cpp:function:: Select the algorithm for linear, video
        // or log color spaces.
        ExposureContrastStyle getStyle() const;
        //!cpp:function::
        void setStyle(ExposureContrastStyle style);

        //!cpp:function:: Applies an exposure adjustment.  The value is in
        //                units of stops (regardless of style), for example,
        //                a value of -1 would be equivalent to reducing the
        //                lighting by one half.
        double getExposure() const;
        //!cpp:function::
        void setExposure(double exposure);
        //!cpp:function::
        bool isExposureDynamic() const;
        //!cpp:function::
        void makeExposureDynamic();

        //!cpp:function:: Applies a contrast/gamma adjustment around a pivot
        //                point.  The contrast and gamma are mathematically the
        //                same, but two controls are provided to enable the use
        //                of separate dynamic parameters.  Contrast is usually
        //                a scene-referred adjustment that pivots around gray
        //                whereas gamma is usually a display-referred adjustment
        //                that pivots around white.
        double getContrast() const;
        //!cpp:function::
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

        //!cpp:function:: Get/set the pivot point around which the contrast
        //                and gamma controls will work. Regardless of whether
        //                linear/video/log-style is being used, the pivot is
        //                always expressed in linear. In other words, a pivot
        //                of 0.18 is always mid-gray.
        double getPivot() const;
        //!cpp:function::
        void setPivot(double pivot);

        //!cpp:function:: Get/set the increment needed to move one stop for
        //                the log-style algorithm. For example, ACEScct is
        //                0.057, LogC is roughly 0.074, and Cineon is roughly
        //                90/1023 = 0.088. The default value is 0.088.
        double getLogExposureStep() const;
        //!cpp:function::
        void setLogExposureStep(double logExposureStep);

        //!cpp:function:: Get/set the position of 18% gray for use by the
        //                log-style algorithm. For example, ACEScct is about
        //                0.41, LogC is about 0.39, and ADX10 is 
        //                445/1023 = 0.435.  The default value is 0.435.
        double getLogMidGray() const;
        //!cpp:function::
        void setLogMidGray(double logMidGray);

    private:
        ExposureContrastTransform();
        ExposureContrastTransform(const ExposureContrastTransform &);
        virtual ~ExposureContrastTransform();

        ExposureContrastTransform & operator= (const ExposureContrastTransform &);

        static void deleter(ExposureContrastTransform * t);

        class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };

    //!cpp:function::
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&,
                                                const ExposureContrastTransform&);

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
        
        //!cpp:function:: get the number of lut readers
        static int getNumFormats();
        //!cpp:function:: get the lut readers at index, return empty string if
        // an invalid index is specified
        static const char * getFormatNameByIndex(int index);
        
        //!cpp:function:: get the lut reader extension at index, return empty string if
        // an invalid index is specified
        static const char * getFormatExtensionByIndex(int index);
        
    private:
        FileTransform();
        FileTransform(const FileTransform &);
        virtual ~FileTransform();
        
        FileTransform& operator= (const FileTransform &);
        
        static void deleter(FileTransform* t);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };
    
    //!cpp:function::
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const FileTransform&);


    //!rst:: //////////////////////////////////////////////////////////////////

    //!cpp:class:: Provides a set of hard-coded algorithmic building blocks 
    // that are needed to accurately implement various common color transformations.
    //
    //
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
        FixedFunctionStyle getStyle() const;
        //!cpp:function:: Select which algorithm to use.
        void setStyle(FixedFunctionStyle style);

        //!cpp:function::
        size_t getNumParams() const;
        //!cpp:function:: Set the parameters (for functions that require them).
        void setParams(const double * params, size_t num);
        //!cpp:function::
        void getParams(double * params) const;
    
    private:
        FixedFunctionTransform();
        FixedFunctionTransform(const FixedFunctionTransform &);
        virtual ~FixedFunctionTransform();
        
        FixedFunctionTransform & operator= (const FixedFunctionTransform &);
        
        static void deleter(FixedFunctionTransform * t);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };
    
    //!cpp:function::
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const FixedFunctionTransform&);

    
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
        ConstTransformRcPtr getTransform(int index) const;
        
        //!cpp:function::
        int size() const;
        //!cpp:function::
        void push_back(const ConstTransformRcPtr& transform);
        //!cpp:function::
        void clear();
        //!cpp:function::
        bool empty() const;
    
    private:
        GroupTransform();
        GroupTransform(const GroupTransform &);
        virtual ~GroupTransform();
        
        GroupTransform& operator= (const GroupTransform &);
        
        static void deleter(GroupTransform* t);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };
    
    //!cpp:function::
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const GroupTransform&);
    

    //!rst:: //////////////////////////////////////////////////////////////////

    //!cpp:class::  Applies a logarithm with an affine transform before and after.
    // Represents the Cineon lin-to-log type transforms.
    //
    // logSideSlope * log( linSideSlope * color + linSideOffset, base) + logSideOffset
    //
    // * Default values are: 1. * log( 1. * color + 0., 2.) + 0.
    // * Only the rgb channels are affected.
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
        void setBase(double base);
        //!cpp:function::
        double getBase() const;

        //!rst:: Set/Get values for each R, G, and B components.

        //!cpp:function::
        void setLogSideSlopeValue(const double(&values)[3]);
        //!cpp:function::
        void setLogSideOffsetValue(const double(&values)[3]);
        //!cpp:function::
        void setLinSideSlopeValue(const double(&values)[3]);
        //!cpp:function::
        void setLinSideOffsetValue(const double(&values)[3]);
        //!cpp:function::
        void getLogSideSlopeValue(double(&values)[3]) const;
        //!cpp:function::
        void getLogSideOffsetValue(double(&values)[3]) const;
        //!cpp:function::
        void getLinSideSlopeValue(double(&values)[3]) const;
        //!cpp:function::
        void getLinSideOffsetValue(double(&values)[3]) const;

    private:
        LogAffineTransform();
        LogAffineTransform(const LogAffineTransform &);
        virtual ~LogAffineTransform();

        LogAffineTransform& operator= (const LogAffineTransform &);

        static void deleter(LogAffineTransform* t);

        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };

    //!cpp:function::
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const LogAffineTransform&);


    //!rst:: //////////////////////////////////////////////////////////////////
    
    //!cpp:class:: Represents log transform: log(color, base)
    // 
    // * The input will be clamped for negative numbers.
    // * Default base is 2.0
    // * Only the rgb channels are affected
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
        void setBase(double val);
        //!cpp:function::
        double getBase() const;

    private:
        LogTransform();
        LogTransform(const LogTransform &);
        virtual ~LogTransform();
        
        LogTransform& operator= (const LogTransform &);
        
        static void deleter(LogTransform* t);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };
    
    //!cpp:function::
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const LogTransform&);


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
        
        //!cpp:function:: Specify looks to apply.
        // Looks is a potentially comma (or colon) delimited list of look names,
        // Where +/- prefixes are optionally allowed to denote forward/inverse
        // look specification. (And forward is assumed in the absense of either)
        void setLooks(const char * looks);
        //!cpp:function::
        const char * getLooks() const;
        
    private:
        LookTransform();
        LookTransform(const LookTransform &);
        virtual ~LookTransform();
        
        LookTransform& operator= (const LookTransform &);
        
        static void deleter(LookTransform* t);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };
    
    //!cpp:function::
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const LookTransform&);
    
    
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
        //!cpp:function::
        virtual void setDirection(TransformDirection dir);

        //!cpp:function:: Will throw if data is not valid.
        virtual void validate() const;

        //!cpp:function:: Checks if this exactly equals other.
        bool equals(const MatrixTransform & other) const;
        
        //!cpp:function::
        void setValue(const float * m44, const float * offset4);
        //!cpp:function::
        void getValue(float * m44, float * offset4) const;
        
        //!cpp:function::
        void setMatrix(const float * m44);
        void setMatrix(const double * m44);
        //!cpp:function::
        void getMatrix(float * m44) const;
        void getMatrix(double * m44) const;
        
        //!cpp:function::
        void setOffset(const float * offset4);
        void setOffset(const double * offset4);
        //!cpp:function::
        void getOffset(float * offset4) const;
        void getOffset(double * offset4) const;
        
        //!rst:: **Convenience functions**
        //
        // to get the mtx and offset corresponding to higher-level concepts
        // 
        // .. note::
        //    These can throw an exception if for any component
        //    ``oldmin == oldmax. (divide by 0)``
        
        //!cpp:function::
        static void Fit(float * m44, float * offset4,
                        const float * oldmin4, const float * oldmax4,
                        const float * newmin4, const float * newmax4);
        
        //!cpp:function::
        static void Identity(float * m44, float * offset4);
        
        //!cpp:function::
        static void Sat(float * m44, float * offset4,
                        float sat, const float * lumaCoef3);
        
        //!cpp:function::
        static void Scale(float * m44, float * offset4,
                          const float * scale4);
        
        //!cpp:function::
        static void View(float * m44, float * offset4,
                         int * channelHot4,
                         const float * lumaCoef3);
    
    private:
        MatrixTransform();
        MatrixTransform(const MatrixTransform &);
        virtual ~MatrixTransform();
        
        MatrixTransform& operator= (const MatrixTransform &);
        
        static void deleter(MatrixTransform* t);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };
    
    //!cpp:function::
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const MatrixTransform&);

    //!rst:: //////////////////////////////////////////////////////////////////

    //!cpp:class:: Represents a range transform
    //
    // The Range is used to apply an affine transform (scale & offset) and
    // clamps values to min/max bounds on all color components except the alpha.
    // The scale and offset values are computed from the input and output bounds.
    // 
    // .. note::
    //    Refer to section 7.2.4 in specification S-2014-006
    //    "A Common File Format for Look-Up Tables" from the
    //    Academy of Motion Picture Arts and Sciences and 
    //    the American Society of Cinematographers.
    // 
    // .. note::
    //    The "noClamp" style described in the specification (S-2014-006.pdf)
    //    becomes a MatrixOp at the processor level.
    //
    class OCIOEXPORT RangeTransform : public Transform
    {
    public:
        //!cpp:function:: Creates an instance of RangeTransform.
        static RangeTransformRcPtr Create();
        
        //!cpp:function:: Creates a copy of this.
        virtual TransformRcPtr createEditableCopy() const;
        
        //!cpp:function:: Get Transform direction.
        virtual TransformDirection getDirection() const;
        //!cpp:function:: Set Transform direction.
        virtual void setDirection(TransformDirection dir);

        //!cpp:function:: Get the style (i.e. clamping or not).
        virtual RangeStyle getStyle() const;
        //!cpp:function:: Set the Range style to clamp or not input values.
        virtual void setStyle(RangeStyle style);

        //!cpp:function:: Will throw if data is not valid.
        virtual void validate() const;

        //!cpp:function:: Checks if this equals other.
        bool equals(const RangeTransform & other) const;

        //!cpp:function:: Set the minimum value for the input.
        void setMinInValue(double val);
        //!cpp:function:: Get the minimum value for the input.
        double getMinInValue() const;
        //!cpp:function:: Is the minimum value for the input set?
        bool hasMinInValue() const;
        //!cpp:function:: Unset the minimum value for the input
        void unsetMinInValue();

        //!cpp:function:: Set the maximum value for the input.
        void setMaxInValue(double val);
        //!cpp:function:: Get the maximum value for the input.
        double getMaxInValue() const;
        //!cpp:function:: Is the maximum value for the input set?
        bool hasMaxInValue() const;
        //!cpp:function:: Unset the maximum value for the input.
        void unsetMaxInValue();

        //!cpp:function:: Set the minimum value for the output.
        void setMinOutValue(double val);
        //!cpp:function:: Get the minimum value for the output.
        double getMinOutValue() const;
        //!cpp:function:: Is the minimum value for the output set?
        bool hasMinOutValue() const;
        //!cpp:function:: Unset the minimum value for the output.
        void unsetMinOutValue();

        //!cpp:function:: Set the maximum value for the output.
        void setMaxOutValue(double val);
        //!cpp:function:: Get the maximum value for the output.
        double getMaxOutValue() const;
        //!cpp:function:: Is the maximum value for the output set?
        bool hasMaxOutValue() const;
        //!cpp:function:: Unset the maximum value for the output.
        void unsetMaxOutValue();

    private:
        RangeTransform();
        RangeTransform(const RangeTransform &);
        virtual ~RangeTransform();
        
        RangeTransform& operator= (const RangeTransform &);
        
        static void deleter(RangeTransform* t);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };
    
    //!cpp:function::
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const RangeTransform&);

    //!rst:: //////////////////////////////////////////////////////////////////
    
    //!cpp:class:: Truelight transform using its API
    class OCIOEXPORT TruelightTransform : public Transform
    {
    public:
        //!cpp:function::
        static TruelightTransformRcPtr Create();
        
        //!cpp:function::
        virtual TransformRcPtr createEditableCopy() const;
        
        //!cpp:function::
        virtual TransformDirection getDirection() const;
        //!cpp:function::
        virtual void setDirection(TransformDirection dir);

        //!cpp:function:: Will throw if data is not valid.
        virtual void validate() const;

        //!cpp:function::
        void setConfigRoot(const char * configroot);
        //!cpp:function::
        const char * getConfigRoot() const;
        
        //!cpp:function::
        void setProfile(const char * profile);
        //!cpp:function::
        const char * getProfile() const;
        
        //!cpp:function::
        void setCamera(const char * camera);
        //!cpp:function::
        const char * getCamera() const;
        
        //!cpp:function::
        void setInputDisplay(const char * display);
        //!cpp:function::
        const char * getInputDisplay() const;
        
        //!cpp:function::
        void setRecorder(const char * recorder);
        //!cpp:function::
        const char * getRecorder() const;
        
        //!cpp:function::
        void setPrint(const char * print);
        //!cpp:function::
        const char * getPrint() const;
        
        //!cpp:function::
        void setLamp(const char * lamp);
        //!cpp:function::
        const char * getLamp() const;
        
        //!cpp:function::
        void setOutputCamera(const char * camera);
        //!cpp:function::
        const char * getOutputCamera() const;
        
        //!cpp:function::
        void setDisplay(const char * display);
        //!cpp:function::
        const char * getDisplay() const;
        
        //!cpp:function::
        void setCubeInput(const char * type);
        //!cpp:function::
        const char * getCubeInput() const;
        
    private:
        TruelightTransform();
        TruelightTransform(const TruelightTransform &);
        virtual ~TruelightTransform();
        
        TruelightTransform& operator= (const TruelightTransform &);
        
        static void deleter(TruelightTransform* t);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };
    
    //!cpp:function::
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const TruelightTransform &);
    
}
OCIO_NAMESPACE_EXIT

#endif

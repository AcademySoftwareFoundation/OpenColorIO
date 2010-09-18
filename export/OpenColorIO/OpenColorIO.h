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


#ifndef INCLUDED_OCIO_OCIO_H
#define INCLUDED_OCIO_OCIO_H

#define OCIO_VERSION "0.5.16"

// Namespace mojo
#ifndef OCIO_NAMESPACE
#define OCIO_NAMESPACE OpenColorIO
#endif

#define OCIO_VERSION_NS v0
#define OCIO_NAMESPACE_ENTER namespace OCIO_NAMESPACE { namespace OCIO_VERSION_NS
#define OCIO_NAMESPACE_EXIT using namespace OCIO_VERSION_NS; }
#define OCIO_NAMESPACE_USING using namespace OCIO_NAMESPACE;


///////////////////////////////////////////////////////////////////////////////
//
// OpenColorIO

/*
#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

// Example: Compositing plugin, which converts from "log" to "lin"
try
{
    // Get the global OpenColorIO config
    // This will auto-initialize (using $OCIO) on first use
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    
    // Get the processor corresponding to this transform.
    ConstProcessorRcPtr processor = config->getProcessor(OCIO::ROLE_COMPOSITING_LOG,
                                                         OCIO::ROLE_SCENE_LINEAR);
    
    // Wrap the image in a light-weight ImageDescription
    OCIO::PackedImageDesc img(imageData, w, h, 4);
    
    // Apply the color transformation (in place)
    processor->apply(img);
}
catch(OCIO::Exception & exception)
{
    std::cerr << "OpenColorIO Error: " << exception.what() << std::endl;
}


// Example: Apply the default display transform (to a scene-linear image)
try
{
    // Get the global OpenColorIO config
    // This will auto-initialize (using $OCIO) on first use
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    
    const char * device = config->getDefaultDisplayDeviceName();
    const char * transformName = config->getDefaultDisplayTransformName(device);
    const char * displayColorSpace = config->getDisplayColorSpaceName(device, transformName);
    
    OCIO::DisplayTransformRcPtr transform = OCIO::DisplayTransform::Create();
    transform->setInputColorSpaceName( OCIO::ROLE_SCENE_LINEAR );
    transform->setDisplayColorSpaceName( displayColorSpace );
    
    // Get the processor corresponding to this transform
    OCIO::ConstProcessorRcPtr processor = config->getProcessor(transform);
    
    OCIO::PackedImageDesc img(imageData, w, h, 4);
    processor->apply(img);
}
catch(OCIO::Exception & exception)
{
    std::cerr << "OpenColorIO Error: " << exception.what() << std::endl;
}
*/

#include <cstdlib>
#include <exception>
#include <iosfwd>
#include <limits>
#include <memory>
#include <string>


#ifdef __APPLE__
#include <tr1/memory>
#define OCIO_SHARED_PTR std::tr1::shared_ptr
#define OCIO_DYNAMIC_POINTER_CAST std::tr1::dynamic_pointer_cast
#else
#include <boost/shared_ptr.hpp>
#define OCIO_SHARED_PTR boost::shared_ptr
#define OCIO_DYNAMIC_POINTER_CAST boost::dynamic_pointer_cast
#endif

OCIO_NAMESPACE_ENTER
{
    ///////////////////////////////////////////////////////////////////////////
    //
    // EXCEPTION / ENUMS / PREDECLARATIONS
    //
    //
    
    class Config;
    typedef OCIO_SHARED_PTR<const Config> ConstConfigRcPtr;
    typedef OCIO_SHARED_PTR<Config> ConfigRcPtr;
    
    class ColorSpace;
    typedef OCIO_SHARED_PTR<const ColorSpace> ConstColorSpaceRcPtr;
    typedef OCIO_SHARED_PTR<ColorSpace> ColorSpaceRcPtr;
    
    class Processor;
    typedef OCIO_SHARED_PTR<const Processor> ConstProcessorRcPtr;
    typedef OCIO_SHARED_PTR<Processor> ProcessorRcPtr;
    
    class ImageDesc;
    class GpuShaderDesc;
    class Exception;
    
    class Transform;
    typedef OCIO_SHARED_PTR<const Transform> ConstTransformRcPtr;
    typedef OCIO_SHARED_PTR<Transform> TransformRcPtr;
    
    class GroupTransform;
    typedef OCIO_SHARED_PTR<const GroupTransform> ConstGroupTransformRcPtr;
    typedef OCIO_SHARED_PTR<GroupTransform> GroupTransformRcPtr;
    
    class FileTransform;
    typedef OCIO_SHARED_PTR<const FileTransform> ConstFileTransformRcPtr;
    typedef OCIO_SHARED_PTR<FileTransform> FileTransformRcPtr;
    
    class ColorSpaceTransform;
    typedef OCIO_SHARED_PTR<const ColorSpaceTransform> ConstColorSpaceTransformRcPtr;
    typedef OCIO_SHARED_PTR<ColorSpaceTransform> ColorSpaceTransformRcPtr;
    
    class DisplayTransform;
    typedef OCIO_SHARED_PTR<const DisplayTransform> ConstDisplayTransformRcPtr;
    typedef OCIO_SHARED_PTR<DisplayTransform> DisplayTransformRcPtr;
    
    class CDLTransform;
    typedef OCIO_SHARED_PTR<const CDLTransform> ConstCDLTransformRcPtr;
    typedef OCIO_SHARED_PTR<CDLTransform> CDLTransformRcPtr;
    
    class MatrixTransform;
    typedef OCIO_SHARED_PTR<const MatrixTransform> ConstMatrixTransformRcPtr;
    typedef OCIO_SHARED_PTR<MatrixTransform> MatrixTransformRcPtr;
    
    template <class T, class U>
    inline OCIO_SHARED_PTR<T> DynamicPtrCast(OCIO_SHARED_PTR<U> const & ptr)
    {
        return OCIO_DYNAMIC_POINTER_CAST<T,U>(ptr);
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    enum ColorSpaceDirection
    {
        COLORSPACE_DIR_UNKNOWN = 0,
        COLORSPACE_DIR_TO_REFERENCE,
        COLORSPACE_DIR_FROM_REFERENCE
    };
    
    enum TransformDirection
    {
        TRANSFORM_DIR_UNKNOWN = 0,
        TRANSFORM_DIR_FORWARD,
        TRANSFORM_DIR_INVERSE
    };
    
    enum Interpolation
    {
        INTERP_UNKNOWN = 0,
        INTERP_NEAREST, //! nearest neighbor in all dimensions
        INTERP_LINEAR   //! linear interpolation in all dimensions
    };
    
    enum BitDepth {
        BIT_DEPTH_UNKNOWN = 0,
        BIT_DEPTH_UINT8,
        BIT_DEPTH_UINT10,
        BIT_DEPTH_UINT12,
        BIT_DEPTH_UINT14,
        BIT_DEPTH_UINT16,
        BIT_DEPTH_UINT32,
        BIT_DEPTH_F16,
        BIT_DEPTH_F32
    };
    
    enum GpuAllocation {
        GPU_ALLOCATION_UNKNOWN = 0,
        GPU_ALLOCATION_UNIFORM,
        GPU_ALLOCATION_LG2
    };
    
    //! Used when there is a choice of hardware shader language.
    
    enum GpuLanguage
    {
        GPU_LANGUAGE_UNKNOWN = 0,
        GPU_LANGUAGE_CG,  ///< Nvidia Cg shader
        GPU_LANGUAGE_GLSL_1_0,     ///< OpenGL Shading Language
        GPU_LANGUAGE_GLSL_1_3,     ///< OpenGL Shading Language
    };
    
    
    
    
    /*!
    ///////////////////////////////////////////////////////////////////////////
    //
    // Config
    //
    //
    
    For applications which are interested in using a single color config
    at a time (this is the vast majority of apps), their API would
    traditionally get the global configuration, and use that, as opposed to
    creating a new one.  This simplifies the use case for plugins / bindings,
    as it alleviates the need to pass configuration handles around.
    
    An example of an application where this would not be sufficient would be
    a multi-threaded image proxy server (daemon), which wished to handle
    multiple show configurations in a single process concurrently. This app
    would need to keep multiple configurations alive, and to manage them
    appropriately.
    
    The color configuration (Config) is the main object for interacting
    with this libray.  It encapsulates all of the information necessary to
    utilized customized ColorSpace transformations and DisplayTransform
    operations.
    
    See the included FAQ for more detailed information on selecting / creating
    / working with custom color configurations.
    
    Roughly speaking, if you're a novice user you will want to select a
    default configuration that most closely approximates your use case
    (animation, visual effects, etc), and set $OCIO_CONFIG to point at the
    root of that configuration.
    
    Use cases
    1) Just want it to work, assume single config.
        it can change out from under me, dont care
     ConstConfigRcPtr config = GetCurrentConfig();
    
    2) actively editing config
     A: does not exist.  You can never actively edit the global one.
        If you are processing pixels off of a local non-const config,
        it's the clients responsibility to make sure you're not changing
        it while pixels are in transit.
    
    3) creating a bunch different configs, etiting all
    A: User is responsible for editibility in this case.
    
    4) intializing from speicifc config, changing which is global
    A: CreateConfig, config->loadFromFile, SetCurrentConfig(config)
    
        Get the current config. If it has not been previously set,
        a new config will be created by reading the $OCIO environment
        variable.
        
        'Auto' initialization using environment variables is typically
        preferable in a multi-app ecosystem, as it allows all applications
        to be consistently configured.
    */
    
    ConstConfigRcPtr GetCurrentConfig();
    
    //! Set the current configuration;
    //  this will store a copy of the specified config
    
    void SetCurrentConfig(const ConstConfigRcPtr & config);
    
    
    class Config
    {
    public:
        
        // INITIALIZATION /////////////////////////////////////////////////////
        
        static ConfigRcPtr Create();
        static ConstConfigRcPtr CreateFromEnv();
        static ConstConfigRcPtr CreateFromFile(const char * filename);
        
        ConfigRcPtr createEditableCopy() const;
        
        const char * getResourcePath() const;
        void setResourcePath(const char * path);
        
        const char * getResolvedResourcePath() const;
        
        const char * getDescription() const;
        void setDescription(const char * description);
        
        void writeXML(std::ostream& os) const;
        
        
        // COLORSPACES ////////////////////////////////////////////////////////
        
        /*!
        The ColorSpace is the state of an image with respect to colorimetry and
        color encoding. Transforming images between different ColorSpaces is
        the primary motivation for this libray.
        
        While a completely discussion of ColorSpaces is beyond the scope of
        header documentation, traditional uses would be to have ColorSpaces
        corresponding to: physical capture devices (known cameras, scanners),
        and internal 'convenience' spaces (such as scene linear, logaraithmic.
        
        ColorSpaces are specific to a particular image precision (float32,
        uint8,  etc), and the set of ColorSpaces that provide equivalent
        mappings (at different precisions) are referred to as a 'family'.
        */
        
        int getNumColorSpaces() const;
        
        // This will null if an invalid index is specified
        const char * getColorSpaceNameByIndex(int index) const;
        
        
        //! These fcns all accept either a colorspace OR role name.
        
        // (Colorspace names take precedence over roles)
        // This will return null if the specified name is not found.
        
        ConstColorSpaceRcPtr getColorSpace(const char * name) const;
        ColorSpaceRcPtr getEditableColorSpace(const char * name);
        int getIndexForColorSpace(const char * name) const;
        
        
        //! If another colorspace was already registered with the
        // same name, this will overwrite it. This stores a
        // copy of the specified color space
        
        void addColorSpace(const ConstColorSpaceRcPtr & cs);
        void clearColorSpaces();
        
        //! Given the specified string, get the longest, right-most,
        //  ColorSpace substring that appears.
        //  If strict parsing is enabled, and no colorspace are found
        //  return a null pointer. If strict parsing is disabled,
        //  return ROLE_DEFAULT (if defined). If the default role is
        //  not defined, return a null pointer.
        
        const char * parseColorSpaceFromString(const char * str) const;
        
        bool isStrictParsingEnabled() const;
        void setStrictParsingEnabled(bool enabled);
        
        // Roles (like an alias for a color space)
        // You query the colorSpace corresponding to a role
        // using the normal getColorSpace fcn.
        
        //! Setting the csname name to a null string unsets it
        void setRole(const char * role, const char * colorSpaceName);
        
        int getNumRoles() const;
        const char * getRoleNameByIndex(int index) const;
        
        // Display Transforms
        int getNumDisplayDeviceNames() const;
        const char * getDisplayDeviceName(int index) const;
        const char * getDefaultDisplayDeviceName() const;
        int getNumDisplayTransformNames(const char * device) const;
        const char * getDisplayTransformName(const char * device, int index) const;
        const char * getDefaultDisplayTransformName(const char * device) const;
        const char * getDisplayColorSpaceName(const char * device, const char * displayTransformName) const;
        
        void addDisplayDevice(const char * device,
                              const char * transformName,
                              const char * colorSpaceName);
        
        
        
        
        // Get the default coefficients for computing luma.
        //
        // There is no "1 size fits all" set of luma coefficients.
        // (The values are typically different for each colorspace,
        // and the application of them may be non-sensical depending on
        // the intenisty coding anyways).  Thus, the 'right' answer is
        // to make these functions on the ColorSpace class.  However,
        // it's often useful to have a config-wide default so here it is.
        // We will add the colorspace specific luma call if/when another
        // client is interesting in using it.
        
        void getDefaultLumaCoefs(float * rgb) const;
        
        // These should be normalized. (sum to 1.0 exactly)
        void setDefaultLumaCoefs(const float * rgb);
        
        
        //! Convert from inputColorSpace to outputColorSpace
        //
        //  Note: This may provide higher fidelity than anticipated due to
        //        internal optimizations. For example, if the inputcolorspace
        //        and the outputColorSpace are members of the same family, no
        //        conversion will be applied, even though strictly speaking
        //        quantization should be added.
        // 
        //  If you wish to test these calls for quantization characteristics,
        //  apply in two steps, the image must contain RGB triples (though
        //  arbitrary numbers of additional channels can be supported (ignored)
        //  using the pixelStrideBytes arg.
        
        ConstProcessorRcPtr getProcessor(const ConstColorSpaceRcPtr & srcColorSpace,
                                         const ConstColorSpaceRcPtr & dstColorSpace) const;
        
        //! Names can be colorspace name, role name, or a combination of both
        ConstProcessorRcPtr getProcessor(const char * srcName,
                                         const char * dstName) const;
        
        // Get the processor for the specified transform.
        // Not often needed, but will allow for the re-use of atomic OCIO functionality
        // (Such as to apply an individual LUT file)
        
        ConstProcessorRcPtr getProcessor(const ConstTransformRcPtr& transform,
                                         TransformDirection direction = TRANSFORM_DIR_FORWARD) const;
    private:
        Config();
        ~Config();
        
        Config(const Config &);
        Config& operator= (const Config &);
        
        static void deleter(Config* c);
        
        class Impl;
        friend class Impl;
        std::auto_ptr<Impl> m_impl;
    };
    
    std::ostream& operator<< (std::ostream&, const Config&);
    
    
    ///////////////////////////////////////////////////////////////////////////
    //
    // ColorSpace
    //
    
    class ColorSpace
    {
    public:
        static ColorSpaceRcPtr Create();
        
        ColorSpaceRcPtr createEditableCopy() const;
        
        const char * getName() const;
        void setName(const char * name);
        
        const char * getFamily() const;
        void setFamily(const char * family);
        
        const char * getDescription() const;
        void setDescription(const char * description);
        
        BitDepth getBitDepth() const;
        void setBitDepth(BitDepth bitDepth);
        
        
        
        // ColorSpaces that are data are treated a bit special. Basically, any
        // colorspace transforms you try to apply to them are ignored.  (Think
        // of applying a gamut mapping transform to an ID pass). Also, the
        // DisplayTransform process obeys special 'data min' and 'data max' args.
        //
        // This is traditionally used for pixel data that represents non-color
        // pixel data, such as normals, point positions, ID information, etc.
        
        bool isData() const;
        void setIsData(bool isData);
        
        // Gpu allocation information
        GpuAllocation getGpuAllocation() const;
        void setGpuAllocation(GpuAllocation allocation);
        
        float getGpuMin() const;
        void setGpuMin(float min);
        
        float getGpuMax() const;
        void setGpuMax(float max);
        
        
        
        
        ConstGroupTransformRcPtr getTransform(ColorSpaceDirection dir) const;
        GroupTransformRcPtr getEditableTransform(ColorSpaceDirection dir);
        
        void setTransform(const ConstGroupTransformRcPtr & groupTransform,
                          ColorSpaceDirection dir);
        
        // Setting a transform to a non-empty group makes it specified
        bool isTransformSpecified(ColorSpaceDirection dir) const;
    
    private:
        ColorSpace();
        ~ColorSpace();
        
        ColorSpace(const ColorSpace &);
        ColorSpace& operator= (const ColorSpace &);
        
        static void deleter(ColorSpace* c);
        
        class Impl;
        friend class Impl;
        std::auto_ptr<Impl> m_impl;
    };
    
    std::ostream& operator<< (std::ostream&, const ColorSpace&);
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    //
    // Processor
    //
    //
    
    
    class Processor
    {
    public:
        virtual ~Processor();
        
        virtual bool isNoOp() const = 0;
        
        ///////////////
        //
        // CPU Path
        
        //! Apply to an image
        virtual void apply(ImageDesc& img) const = 0;
        
        //! Apply to a single pixel.
        // This is not as efficicent as applying to an entire image at once.
        // If you are processing multiple pixels, and have the flexiblity,
        // use the above function instead.
        
        virtual void applyRGB(float * pixel) const = 0;
        virtual void applyRGBA(float * pixel) const = 0;
        
        ///////////////
        //
        // GPU Path
        
        virtual const char * getGpuShaderText(const GpuShaderDesc & shaderDesc) const = 0;
        
        virtual void getGpuLut3D(float* lut3d, const GpuShaderDesc & shaderDesc) const = 0;
        virtual const char * getGpuLut3DCacheID(const GpuShaderDesc & shaderDesc) const = 0;
        
        /*
        virtual int getGpuLut3DEdgeLen() const = 0;
        */
        
        //! Get the 3d lut + cg shader for the specified DisplayTransform
        //
        // cg signature will be:
        //        shaderFcnName(in half4 inPixel,
        //                      const uniform sampler3D   lut3d)
        
        // lut3d should be size: 3*edgeLen*edgeLen*edgeLen
        
        // return 0 if unknown
        
    private:
        Processor& operator= (const Processor &);
    };
    
    
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    //
    // ImageDesc
    //
    //
    
    const ptrdiff_t AutoStride = std::numeric_limits<ptrdiff_t>::min();
    
    // This is a light-weight wrapper around an image, that provides
    // a context for pixel access.   This does NOT claim ownership
    // of the pixels, or do any internal allocations or copying of
    // image data
    
    class ImageDesc
    {
    public:
        virtual ~ImageDesc();
        
        virtual long getWidth() const = 0;
        virtual long getHeight() const = 0;
        
        virtual ptrdiff_t getXStrideBytes() const = 0;
        virtual ptrdiff_t getYStrideBytes() const = 0;
        
        virtual float* getRData() const = 0;
        virtual float* getGData() const = 0;
        virtual float* getBData() const = 0;
    
    private:
        ImageDesc& operator= (const ImageDesc &);
    };
    
    std::ostream& operator<< (std::ostream&, const ImageDesc&);
    
    
    class PackedImageDesc : public ImageDesc
    {
    public:
        PackedImageDesc(float * data,
                        long width, long height,
                        long numChannels,
                        ptrdiff_t chanStrideBytes = AutoStride,
                        ptrdiff_t xStrideBytes = AutoStride,
                        ptrdiff_t yStrideBytes = AutoStride);
        
        virtual ~PackedImageDesc();
        
        virtual long getWidth() const;
        virtual long getHeight() const;
        
        virtual ptrdiff_t getXStrideBytes() const;
        virtual ptrdiff_t getYStrideBytes() const;
        
        virtual float* getRData() const;
        virtual float* getGData() const;
        virtual float* getBData() const;
        
    private:
        class Impl;
        friend class Impl;
        std::auto_ptr<Impl> m_impl;
        
        PackedImageDesc(const PackedImageDesc &);
        PackedImageDesc& operator= (const PackedImageDesc &);
    };
    
    
    class PlanarImageDesc : public ImageDesc
    {
    public:
        PlanarImageDesc(float * rData, float * gData, float * bData,
                        long width, long height,
                        ptrdiff_t yStrideBytes = AutoStride);
        
        virtual ~PlanarImageDesc();
        
        virtual long getWidth() const;
        virtual long getHeight() const;
        
        virtual ptrdiff_t getXStrideBytes() const;
        virtual ptrdiff_t getYStrideBytes() const;
        
        virtual float* getRData() const;
        virtual float* getGData() const;
        virtual float* getBData() const;
        
    private:
        class Impl;
        friend class Impl;
        std::auto_ptr<Impl> m_impl;
        
        PlanarImageDesc(const PlanarImageDesc &);
        PlanarImageDesc& operator= (const PlanarImageDesc &);
    };
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    class GpuShaderDesc
    {
    public:
        GpuShaderDesc();
        ~GpuShaderDesc();
        
        void setLanguage(GpuLanguage lang);
        GpuLanguage getLanguage() const;
        
        void setFunctionName(const char * name);
        const char * getFunctionName() const;
        
        void setLut3DEdgeLen(int len);
        int getLut3DEdgeLen() const;
        
    private:
        class Impl;
        friend class Impl;
        std::auto_ptr<Impl> m_impl;
        
        GpuShaderDesc(const GpuShaderDesc &);
        GpuShaderDesc& operator= (const GpuShaderDesc &);
    };
    
    
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    //
    // Transforms
    //
    // Typically only needed when creating and/or manipulating configurations
    
    class Transform
    {
    public:
        virtual ~Transform();
        virtual TransformRcPtr createEditableCopy() const = 0;
        
        virtual TransformDirection getDirection() const = 0;
        virtual void setDirection(TransformDirection dir) = 0;
        
    
    private:
        Transform& operator= (const Transform &);
    };
    
    std::ostream& operator<< (std::ostream&, const Transform&);
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    class GroupTransform : public Transform
    {
    public:
        static GroupTransformRcPtr Create();
        
        virtual TransformRcPtr createEditableCopy() const;
        
        virtual TransformDirection getDirection() const;
        virtual void setDirection(TransformDirection dir);
        
        ConstTransformRcPtr getTransform(int index) const;
        TransformRcPtr getEditableTransform(int index);
        
        int size() const;
        void push_back(const ConstTransformRcPtr& transform);
        void clear();
        bool empty() const;
    
    private:
        GroupTransform();
        GroupTransform(const GroupTransform &);
        virtual ~GroupTransform();
        
        GroupTransform& operator= (const GroupTransform &);
        
        static void deleter(GroupTransform* t);
        
        class Impl;
        friend class Impl;
        std::auto_ptr<Impl> m_impl;
    };
    
    std::ostream& operator<< (std::ostream&, const GroupTransform&);
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    class FileTransform : public Transform
    {
    public:
        static FileTransformRcPtr Create();
        
        virtual TransformRcPtr createEditableCopy() const;
        
        virtual TransformDirection getDirection() const;
        virtual void setDirection(TransformDirection dir);
        
        const char * getSrc() const;
        void setSrc(const char * src);
        
        Interpolation getInterpolation() const;
        void setInterpolation(Interpolation interp);
    
    private:
        FileTransform();
        FileTransform(const FileTransform &);
        virtual ~FileTransform();
        
        FileTransform& operator= (const FileTransform &);
        
        static void deleter(FileTransform* t);
        
        class Impl;
        friend class Impl;
        std::auto_ptr<Impl> m_impl;
    };
    
    std::ostream& operator<< (std::ostream&, const FileTransform&);
    
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    class ColorSpaceTransform : public Transform
    {
    public:
        static ColorSpaceTransformRcPtr Create();
        
        virtual TransformRcPtr createEditableCopy() const;
        
        virtual TransformDirection getDirection() const;
        virtual void setDirection(TransformDirection dir);
        
        const char * getSrc() const;
        void setSrc(const char * src);
        
        const char * getDst() const;
        void setDst(const char * dst);
    
    private:
        ColorSpaceTransform();
        ColorSpaceTransform(const ColorSpaceTransform &);
        virtual ~ColorSpaceTransform();
        
        ColorSpaceTransform& operator= (const ColorSpaceTransform &);
        
        static void deleter(ColorSpaceTransform* t);
        
        class Impl;
        friend class Impl;
        std::auto_ptr<Impl> m_impl;
    };
    
    std::ostream& operator<< (std::ostream&, const ColorSpaceTransform&);
    
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    
    class DisplayTransform : public Transform
    {
    public:
        static DisplayTransformRcPtr Create();
        
        virtual TransformRcPtr createEditableCopy() const;
        
        virtual TransformDirection getDirection() const;
        virtual void setDirection(TransformDirection dir);
        
        
        // Step 0. Specify the incoming color space
        void setInputColorSpaceName(const char * name);
        const char * getInputColorSpaceName() const;
        
        // Step 1: Apply a Color Correction, in ROLE_SCENE_LINEAR
        void setLinearCC(const ConstTransformRcPtr & cc);
        ConstTransformRcPtr getLinearCC() const;
        
        // Step 2: Apply a color correction, in ROLE_COLOR_TIMING
        void setColorTimingCC(const ConstTransformRcPtr & cc);
        ConstTransformRcPtr getColorTimingCC() const;
        
        // Step 3: Apply the View Matrix
        
        // Step 4: Apply the output display transform
        void setDisplayColorSpaceName(const char * name);
        const char * getDisplayColorSpaceName() const;
    
    private:
        DisplayTransform();
        DisplayTransform(const DisplayTransform &);
        virtual ~DisplayTransform();
        
        DisplayTransform& operator= (const DisplayTransform &);
        
        static void deleter(DisplayTransform* t);
        
        class Impl;
        friend class Impl;
        std::auto_ptr<Impl> m_impl;
    };
    
    std::ostream& operator<< (std::ostream&, const DisplayTransform&);
    
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    // An implementation of the ASC CDL Transfer Functions and Interchange
    // Syntax (Based on the version 1.2 document)
    //
    // Note: the clamping portion of the CDL is only applied if 
    // a non-identity power is specified.
    
    class CDLTransform : public Transform
    {
    public:
        static CDLTransformRcPtr Create();
        
        virtual TransformRcPtr createEditableCopy() const;
        
        virtual TransformDirection getDirection() const;
        virtual void setDirection(TransformDirection dir);
        
        bool equals(const ConstCDLTransformRcPtr & other) const;
        
        const char * getXML() const;
        void setXML(const char * xml);
        
        // ASC_SOP
        // Slope, offset, power
        // out = clamp( (in * slope) + offset ) ^ power
        //
        
        void setSlope(const float * rgb);
        void getSlope(float * rgb) const;
        
        void setOffset(const float * rgb);
        void getOffset(float * rgb) const;
        
        void setPower(const float * rgb);
        void getPower(float * rgb) const;
        
        void setSOP(const float * vec9);
        void getSOP(float * vec9) const;
        
        // ASC_SAT
        
        void setSat(float sat);
        float getSat() const;
        
        // These are hard-coded, by spec, to r709
        void getSatLumaCoefs(float * rgb) const;
        
        // Metadata
        // These do not affect the image processing, but
        // are often useful for pipeline purposes and are
        // included in the serialization.
        
        // Unique Identifier for this correction
        void setID(const char * id);
        const char * getID() const;
        
        // Textual description of color correction
        // (stored on the SOP)
        void setDescription(const char * desc);
        const char * getDescription() const;
    
    private:
        CDLTransform();
        CDLTransform(const CDLTransform &);
        virtual ~CDLTransform();
        
        CDLTransform& operator= (const CDLTransform &);
        
        static void deleter(CDLTransform* t);
        
        class Impl;
        friend class Impl;
        std::auto_ptr<Impl> m_impl;
    };
    
    std::ostream& operator<< (std::ostream&, const CDLTransform&);
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    //
    // Represents an MX+B Matrix transform
    
    class MatrixTransform : public Transform
    {
    public:
        static MatrixTransformRcPtr Create();
        
        virtual TransformRcPtr createEditableCopy() const;
        
        virtual TransformDirection getDirection() const;
        virtual void setDirection(TransformDirection dir);
        
        bool equals(const MatrixTransform & other) const;
        
        void setValue(const float * m44, const float * offset4);
        void getValue(float * m44, float * offset4) const;
        
        
        // Convenience functions to get the mtx and offset
        // corresponding to higher-level concepts
        
        // This can throw an exception if for any component
        // oldmin == oldmax. (divide by 0)
        
        static void Fit(float * m44, float * offset4,
                        const float * oldmin4, const float * oldmax4,
                        const float * newmin4, const float * newmax4);
        
        static void Identity(float * m44, float * offset4);
        
        static void Sat(float * m44, float * offset4,
                        float sat, const float * lumaCoef3);
        
        static void Scale(float * m44, float * offset4,
                          const float * scale4);
        
        static void View(float * m44, float * offset4,
                         bool * channelHot4,
                         const float * lumaCoef3);
    
    private:
        MatrixTransform();
        MatrixTransform(const MatrixTransform &);
        virtual ~MatrixTransform();
        
        MatrixTransform& operator= (const MatrixTransform &);
        
        static void deleter(MatrixTransform* t);
        
        class Impl;
        friend class Impl;
        std::auto_ptr<Impl> m_impl;
    };
    
    std::ostream& operator<< (std::ostream&, const MatrixTransform&);
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    //
    // Utils
    
    //! An exception class to throw for an errors detected at runtime
    //  Warning: ALL fcns on the Config class can potentially throw
    //  this exception.
    
    class Exception : public std::exception
    {
    public:
        Exception(const char *) throw();
        Exception(const Exception&) throw();
        Exception& operator=(const Exception&) throw();
        virtual ~Exception() throw();
        virtual const char* what() const throw();
        
    private:
        std::string msg_;
    };
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    const char * BoolToString(bool val);
    bool BoolFromString(const char * s);
    
    const char * TransformDirectionToString(TransformDirection dir);
    TransformDirection TransformDirectionFromString(const char * s);
    
    TransformDirection GetInverseTransformDirection(TransformDirection dir);
    TransformDirection CombineTransformDirections(TransformDirection d1,
                                                  TransformDirection d2);
    
    const char * ColorSpaceDirectionToString(ColorSpaceDirection dir);
    ColorSpaceDirection ColorSpaceDirectionFromString(const char * s);
    
    const char * BitDepthToString(BitDepth bitDepth);
    BitDepth BitDepthFromString(const char * s);
    bool BitDepthIsFloat(BitDepth bitDepth);
    int BitDepthToInt(BitDepth bitDepth);
    
    const char * GpuAllocationToString(GpuAllocation allocation);
    GpuAllocation GpuAllocationFromString(const char * s);
    
    const char * InterpolationToString(Interpolation interp);
    Interpolation InterpolationFromString(const char * s);
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    /*!
    ColorSpace Roles are used so that plugins, in addition to this API can have
    abstract ways of asking for common colorspaces, without referring to them
    by hardcoded names.
    
    Internal:
        GetGPUDisplayTransform - (ROLE_SCENE_LINEAR (fstop exposure))
                        (ROLE_COLOR_TIMING (ASCColorCorrection))
    
    External Plugins (currently known):
        Colorpicker UIs - (ROLE_COLOR_PICKING)
        Compositor LogConvert (ROLE_SCENE_LINEAR, ROLE_COMPOSITING_LOG)
    
    */
    
    extern const char * ROLE_DEFAULT;
    extern const char * ROLE_REFERENCE;
    extern const char * ROLE_DATA;
    extern const char * ROLE_COLOR_PICKING;
    extern const char * ROLE_SCENE_LINEAR;
    extern const char * ROLE_COMPOSITING_LOG;
    extern const char * ROLE_COLOR_TIMING;
}
OCIO_NAMESPACE_EXIT

#endif

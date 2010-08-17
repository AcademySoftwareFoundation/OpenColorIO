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


///////////////////////////////////////////////////////////////////////////////
//
// OpenColorIO
// Version 0.5.11
//

// TODO: Add ColorSpaceTransform xml serialization / deserialization
// TODO: Turn the lutpath into a search path mechanism
// TODO: Unify all fcns that return colorspace classes to return colorspace name string instead?
//       This may assist with dynamic color spaces
// TODO: Colorspace limit functions, GetLinearColorspaceMax
// TODO: Filmlook
// TODO: Compute statistics on image?
// TODO: Compute Histogram on image?
// TODO: ASCColorCorrection example.
// TODO: highlight compression coefficients?
// TODO: ColorspaceFromFilename
// TODO: Histogram / Statistics FCN
// TODO: rgb_to_hsv?  Efficient for per-pixel application?
// TODO: Add processor.getCacheID
// TODO: per-shot looks?

// TODO: get simple display transform working. can it be expressed as an op?

// TODO: add op optimizations.  op collapsing.  cache op tree.
// TODO: add gamma ops
// TODO: add analytical log ops
// TODO: test 1d atomic ops
// TODO: test full colorspace conversions

// TODO: Figure out for each transform class what is required, move into constructor
// TODO: Add enums to namespace / submodule (both in C++ / python)
// TODO: provide way to tag colorspace operations as explicitly not allowed? what about hdbty<->qt?
// TODO: provide xml defaults mechanism for cleaner xml code
// TODO: such as int vectors, double from str, float vectors, etc.
// TODO: add ocio package (.gz?) file, and ability to convert between representations.
// TODO: add all nuke plugins, also get official Foundry code review
// TODO: add additional lut formats
// TODO: add internal namespace for all implementation objects Ops, etc.
// TODO: Opt build as default?
// TODO: Add prettier xml output (newlines between colorspaces?)

// TODO: Cross-platform

/*
// Example: Compositing plugin, which converts from "log" to "lin"
try
{
    // Get the global config, which will auto-initialize
    // from the environment on first use.
    
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    
    OCIO::ConstColorSpaceRcPtr csSrc = \
        config->getColorSpaceForRole(OCIO::ROLE_COMPOSITING_LOG);
    
    OCIO::ConstColorSpaceRcPtr csDst = \
        config->getColorSpaceForRole(OCIO::ROLE_SCENE_LINEAR);
    
    ConstProcessorRcPtr processor = config->getProcessor(csSrc, csDst);
    
    // Wrap the image in a light-weight ImageDescription,
    // and convert it in place
    
    OCIO::PackedImageDesc img(imageData, w, h, 4);
    processor->apply(img);
}
catch(OCIO::Exception & exception)
{
    std::cerr << "OpenColorIO Error: " << exception.what() << std::endl;
}


// Example: Apply the default display transform (to a scene-linear image)
try
{
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    
    const char * device = config->getDefaultDisplayDevice();
    const char * transformName = config->getDefaultDisplayTransformName(device);
    const char * displayColorSpace = config->getDisplayColorspace(device, transformName);
    
    OCIO::DisplayTransformRcPtr transform = OCIO::DisplayTransform::Create();
    transform->setInputColorSpace( config->getColorSpaceForRole(OCIO::ROLE_SCENE_LINEAR) );
    transform->setDisplayColorSpace( config->getColorSpaceByName(displayColorSpace) );
    
    OCIO::ConstProcessorRcPtr processor = config->getProcessor(transform);
    
    OCIO::PackedImageDesc img(imageData, w, h, 4);
    processor->apply(img);
}
catch(OCIO::Exception & exception)
{
    std::cerr << "OpenColorIO Error: " << exception.what() << std::endl;
}

*/















// Namespace mojo
#define OCIO_VERSION_NS v1
#define OCIO_NAMESPACE_ENTER namespace SPI \
{ namespace OCIO \
{ namespace OCIO_VERSION_NS
#define OCIO_NAMESPACE_EXIT using namespace OCIO_VERSION_NS; \
} /*namespace OCIO*/ \
} /*namespace SPI*/
#define OCIO_NAMESPACE SPI::OCIO
#define OCIO_NAMESPACE_USING using namespace SPI::OCIO;

#include <exception>
#include <memory>
#include <iosfwd>
#include <string>
#include <limits>
#include <cstdlib>


#ifdef __APPLE__
#include <tr1/memory>
#else
#include <boost/shared_ptr.hpp>
#endif

OCIO_NAMESPACE_ENTER
{
    // TODO: Clean up the SharedPtr definition
    #ifdef __APPLE__
    #define SharedPtr std::tr1::shared_ptr
    #define DynamicPtrCast std::tr1::dynamic_pointer_cast
    #else
    #define SharedPtr boost::shared_ptr
    #define DynamicPtrCast boost::dynamic_pointer_cast
    #endif
    
    ///////////////////////////////////////////////////////////////////////////
    //
    // EXCEPTION / ENUMS / PREDECLARATIONS
    //
    //
    
    class Config;
    typedef SharedPtr<const Config> ConstConfigRcPtr;
    typedef SharedPtr<Config> ConfigRcPtr;
    
    class ColorSpace;
    typedef SharedPtr<const ColorSpace> ConstColorSpaceRcPtr;
    typedef SharedPtr<ColorSpace> ColorSpaceRcPtr;
    
    class Processor;
    typedef SharedPtr<const Processor> ConstProcessorRcPtr;
    typedef SharedPtr<Processor> ProcessorRcPtr;
    
    class ImageDesc;
    class GpuShaderDesc;
    class Exception;
    
    class Transform;
    typedef SharedPtr<const Transform> ConstTransformRcPtr;
    typedef SharedPtr<Transform> TransformRcPtr;
    
    class GroupTransform;
    typedef SharedPtr<const GroupTransform> ConstGroupTransformRcPtr;
    typedef SharedPtr<GroupTransform> GroupTransformRcPtr;
    
    class FileTransform;
    typedef SharedPtr<const FileTransform> ConstFileTransformRcPtr;
    typedef SharedPtr<FileTransform> FileTransformRcPtr;
    
    class ColorSpaceTransform;
    typedef SharedPtr<const ColorSpaceTransform> ConstColorSpaceTransformRcPtr;
    typedef SharedPtr<ColorSpaceTransform> ColorSpaceTransformRcPtr;
    
    class DisplayTransform;
    typedef SharedPtr<const DisplayTransform> ConstDisplayTransformRcPtr;
    typedef SharedPtr<DisplayTransform> DisplayTransformRcPtr;
    
    class CDLTransform;
    typedef SharedPtr<const CDLTransform> ConstCDLTransformRcPtr;
    typedef SharedPtr<CDLTransform> CDLTransformRcPtr;
    
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
    // TODO: Specify language spec in each enum?
    
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
        
        
        // TODO: add sanityCheck circa exr
        // confirm all colorspace roles exist
        // confirm there arent duplicate colorspaces
        // confirm all files exist with read permissions?
        
        
        const char * getResourcePath() const;
        void setResourcePath(const char * path);
        
        // TODO: replace with mechanism that supports bundles
        const char * getResolvedResourcePath() const;
        
        const char * getDescription() const;
        void setDescription(const char * description);
        
        // TODO: allow migration to binary file format
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
        
        // TODO: Make this API clean and simple
        int getNumColorSpaces() const;
        ConstColorSpaceRcPtr getColorSpaceByIndex(int index) const;
        ColorSpaceRcPtr getEditableColorSpaceByIndex(int index);
        
        ConstColorSpaceRcPtr getColorSpaceByName(const char * name) const;
        ColorSpaceRcPtr getEditableColorSpaceByName(const char * name);
        int getIndexForColorSpace(const char * name) const;
        
        // if another colorspace was already registered with the
        // same name, this will overwrite it.
        // Stores the live reference to this colorspace
        void addColorSpace(ColorSpaceRcPtr cs);
        void addColorSpace(const ConstColorSpaceRcPtr & cs);
        
        void clearColorSpaces();
        
        //! Given the specified string, get the longest, right-most,
        //  ColorSpace substring that appears.  Return an empty string
        //  if none are found.
        
        const char * parseColorSpaceFromString(const char * str) const;
        
        // Families
        
        
        
        // Roles
        ConstColorSpaceRcPtr getColorSpaceForRole(const char * role) const;
        void setColorSpaceForRole(const char * role, const char * csname);
        void unsetRole(const char * role);
        int getNumRoles() const;
        const char * getRole(int index) const;
        
        
        
        // Display Transforms
        // TODO: add default display device + default display transform
        int getNumDisplayDevices() const;
        const char * getDisplayDevice(int index) const;
        const char * getDefaultDisplayDevice() const;
        
        int getNumDisplayTransformNames(const char * device) const;
        const char * getDisplayTransformName(const char * device, int index) const;
        const char * getDefaultDisplayTransformName(const char * device) const;
        
        const char * getDisplayColorspace(const char * device, const char * displayTransformName) const;
        
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
        
        // Individual lut application functions
        // Can be used to apply a .lut, .dat, .lut3d, or .3dl file.
        // Not generally needed, but useful in testing.
        
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
        
        // ColorSpaces are equal if their names are equal. That is all.
        bool equals(const ConstColorSpaceRcPtr &) const;
        
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
        
        // GPU allocation information
        GpuAllocation getGPUAllocation() const;
        void setGPUAllocation(GpuAllocation allocation);
        
        float getGPUMin() const;
        void setGPUMin(float min);
        
        float getGPUMax() const;
        void setGPUMax(float max);
        
        
        
        
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
        
        virtual const char * getGPUShaderText(const GpuShaderDesc & shaderDesc) const = 0;
        
        virtual void getGPULut3D(float* lut3d, const GpuShaderDesc & shaderDesc) const = 0;
        virtual const char * getGPULut3DCacheID(const GpuShaderDesc & shaderDesc) const = 0;
        
        /*
        virtual int getGPULut3DEdgeLen() const = 0;
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
        
        // static int GetMaxGpuCacheSizeMB();
        // static void SetMaxGpuCacheSizeMB(int maxCacheEntries);
        
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
        
        // TODO: these mimic vector, but not our naming. Fix? ex: isEmpty, getSize
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
        
        // TODO: how is this used with multiple luts in a single file (1d+3d)
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
        
        
        // STAGE I: What is the colorspace for the image coming in?
        void setInputColorSpace(const ConstColorSpaceRcPtr & cs);
        ConstColorSpaceRcPtr getInputColorSpace() const;
        
        
        
        // STAGE II: Apply a Color Correction, in linear, if desired.
        
        // By default, this will convert the incoming image into the ROLE_SCENE_LINEAR
        // colorspace
        
        void setLinearCC(const ConstCDLTransformRcPtr & cc);
        
        //! As a convenience, set this in stops
        void setLinearExposure(const float* v4);
        
        ConstCDLTransformRcPtr getLinearCC() const;
        
        
        
        
        // STAGE III: Apply an arbitrary color correction, in the color timing colorspace
        // if needed
        
        // If a look is needed...
        //void setLookColorspace(const ConstColorSpaceRcPtr & cs);
        //void setLookColorCorrection(const ConstTransformRcPtr & transform);
        
        
        
        // STAGE IV: Apply the View Matrix, if needed
        
        
        
        // STAGE V: Specify which Colorspace is appropriate for viewing
        
        void setDisplayColorSpace(const ConstColorSpaceRcPtr & cs);
        ConstColorSpaceRcPtr getDisplayColorSpace() const;
        
        // STAGE VI: Apply Post-processing
    
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
        
        const char * getXML() const;
        void setXML(const char * xml);
        
        // Throw an exception if the CDL is in any way invalid
        void sanityCheck() const;
        
        bool isNoOp() const;
        
        // TODO: Reset?
        
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
    
    extern const char * ROLE_REFERENCE;
    extern const char * ROLE_DATA;
    extern const char * ROLE_COLOR_PICKING;
    extern const char * ROLE_SCENE_LINEAR;
    extern const char * ROLE_COMPOSITING_LOG;
    extern const char * ROLE_COLOR_TIMING;
}
OCIO_NAMESPACE_EXIT

#endif

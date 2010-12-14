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


#ifndef INCLUDED_OCIO_OPENCOLORIO_H
#define INCLUDED_OCIO_OPENCOLORIO_H

#include <exception>
#include <iosfwd>
#include <string>

#include "OpenColorVersion.h"
#include "OpenColorABI.h"
#include "OpenColorTypes.h"
#include "OpenColorTransforms.h"

/*
// Usage Example:
// (Compositing plugin, which converts from "log" to "lin")

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

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
*/

///////////////////////////////////////////////////////////////////////////////
//
// OpenColorIO

OCIO_NAMESPACE_ENTER
{
    //! An exception class to throw for an errors detected at runtime
    //  Warning: ALL fcns on the Config class can potentially throw
    //  this exception.
    
    class OCIOEXPORT Exception : public std::exception
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
    
    extern OCIOEXPORT ConstConfigRcPtr GetCurrentConfig();
    
    //! Set the current configuration;
    //  this will store a copy of the specified config
    
    extern OCIOEXPORT void SetCurrentConfig(const ConstConfigRcPtr & config);
    
    
    class OCIOEXPORT Config
    {
    public:
        
        // INITIALIZATION /////////////////////////////////////////////////////
        
        static ConfigRcPtr Create();
        static ConstConfigRcPtr CreateFromEnv();
        static ConstConfigRcPtr CreateFromFile(const char * filename);
        static ConstConfigRcPtr CreateFromStream(std::istream & istream);
        
        ConfigRcPtr createEditableCopy() const;
        
        const char * getResourcePath() const;
        void setResourcePath(const char * path);
        const char * getResolvedResourcePath() const;
        
        void setSearchPath(const char * path);
        const char * getSearchPath(bool expand = false) const;
        const char * findFile(const char * filename) const;
        
        const char * getDescription() const;
        void setDescription(const char * description);
        
        void serialize(std::ostream & os) const;
        
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
        Impl * m_impl;
    };
    
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const Config&);
    
    
    ///////////////////////////////////////////////////////////////////////////
    //
    // ColorSpace
    //
    
    class OCIOEXPORT ColorSpace
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
        
        
        // Allocation
        // If this color space needs to be transferred to
        // a limited dynamic range coding space (such as during display
        // with a GPU path, use this allocation to maximize bit efficiency.
        
        Allocation getAllocation() const;
        void setAllocation(Allocation allocation);
        
        // Specify the optional variable values to configure
        // the allocation.  If no variables are specified,
        // the defaults are used.
        //
        // ALLOCATION_UNIFORM
        // 2 vars: [min, max]
        //
        // ALLOCATION_LG2
        // 2 vars: [lg2min, lg2max]
        // 3 vars: [lg2min, lg2max, linear_offset]
        
        int getAllocationNumVars() const;
        void getAllocationVars(float * vars) const;
        void setAllocationVars(int numvars, const float * vars);
        
        
        
        ConstTransformRcPtr getTransform(ColorSpaceDirection dir) const;
        TransformRcPtr getEditableTransform(ColorSpaceDirection dir);
        
        void setTransform(const ConstTransformRcPtr & transform,
                          ColorSpaceDirection dir);
        
        // Setting a transform to a non-null call makes it specified
        bool isTransformSpecified(ColorSpaceDirection dir) const;
    
    private:
        ColorSpace();
        ~ColorSpace();
        
        ColorSpace(const ColorSpace &);
        ColorSpace& operator= (const ColorSpace &);
        
        static void deleter(ColorSpace* c);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
    };
    
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const ColorSpace&);
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    //
    // Processor
    //
    //
    
    
    class OCIOEXPORT Processor
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
    
    class OCIOEXPORT ImageDesc
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
    
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const ImageDesc&);
    
    
    class OCIOEXPORT PackedImageDesc : public ImageDesc
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
        Impl * m_impl;
        
        PackedImageDesc(const PackedImageDesc &);
        PackedImageDesc& operator= (const PackedImageDesc &);
    };
    
    
    class OCIOEXPORT PlanarImageDesc : public ImageDesc
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
        Impl * m_impl;
        
        PlanarImageDesc(const PlanarImageDesc &);
        PlanarImageDesc& operator= (const PlanarImageDesc &);
    };
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    class OCIOEXPORT GpuShaderDesc
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
        Impl * m_impl;
        
        GpuShaderDesc(const GpuShaderDesc &);
        GpuShaderDesc& operator= (const GpuShaderDesc &);
    };
    
}
OCIO_NAMESPACE_EXIT

#endif

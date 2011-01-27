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

/*!rst::
C++ API
=======

**Usage Example:** *Compositing plugin, which converts from "log" to "lin"*

.. code-block:: cpp
   
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

OCIO_NAMESPACE_ENTER
{
    
    
    ///////////////////////////////////////////////////////////////////////////
    //!rst::
    // Exception
    // *********
    
    //!cpp:class:: An exception class to throw for an errors detected at
    // runtime
    //
    // .. warning:: 
    //    ALL fcns on the Config class can potentially throw this exception.
    class OCIOEXPORT Exception : public std::exception
    {
    public:
        //!cpp:function::
        Exception(const char *) throw();
        //!cpp:function::
        Exception(const Exception&) throw();
        //!cpp:function::
        Exception& operator=(const Exception&) throw();
        //!cpp:function::
        virtual ~Exception() throw();
        //!cpp:function::
        virtual const char* what() const throw();
        
    private:
        std::string msg_;
    };
    
    
    ///////////////////////////////////////////////////////////////////////////
    //!rst::
    // Config
    // ******
    //
    // For applications which are interested in using a single color config at
    // a time (this is the vast majority of apps), their API would
    // traditionally get the global configuration, and use that, as opposed to
    // creating a new one.   This simplifies the use case for
    // plugins / bindings, as it alleviates the need to pass configuration
    // handles around.
    // 
    // An example of an application where this would not be sufficient would be
    // a multi-threaded image proxy server (daemon), which wished to handle
    // multiple show configurations in a single process concurrently.   This
    // app would need to keep multiple configurations alive, and to manage them
    // appropriately.
    // 
    // The color configuration (:cpp:class:`Config`) is the main object for
    // interacting with this libray.   It encapsulates all of the information
    // necessary to utilized customized :cpp:class:`ColorSpaceTransform` and
    // :cpp:class:`DisplayTransform` operations.
    // 
    // See the included :doc:`FAQ` for more detailed information on
    // selecting / creating / working with custom color configurations.
    // 
    // Roughly speaking, if you're a novice user you will want to select a
    // default configuration that most closely approximates your use case
    // (animation, visual effects, etc), and set :envvar:`OCIO` environment
    // variable to point at the root of that configuration.
    // 
    // .. note::
    //    Initialization using environment variables is typically preferable in
    //    a multi-app ecosystem, as it allows all applications to be
    //    consistently configured.
    //
    // See :doc:`UsageExamples`
    
    //!cpp:function::
    extern OCIOEXPORT ConstConfigRcPtr GetCurrentConfig();
    
    //!cpp:function:: Set the current configuration.
    //
    // this will store a copy of the specified config
    extern OCIOEXPORT void SetCurrentConfig(const ConstConfigRcPtr & config);
    
    //!cpp:class::
    class OCIOEXPORT Config
    {
    public:
        
        ///////////////////////////////////////////////////////////////////////////
        //!rst:: .. _cfginit_section:
        // 
        // Initialization
        // ^^^^^^^^^^^^^^
        
        //!cpp:function::
        static ConfigRcPtr Create();
        //!cpp:function::
        static ConstConfigRcPtr CreateFromEnv();
        //!cpp:function::
        static ConstConfigRcPtr CreateFromFile(const char * filename);
        //!cpp:function::
        static ConstConfigRcPtr CreateFromStream(std::istream & istream);
        
        //!cpp:function::
        ConfigRcPtr createEditableCopy() const;
        
        //!cpp:function::
        // This will throw an exception if the config is malformed. The most
        // common error situation are references to colorspaces that do not
        // exist.
        void sanityCheck() const;
        
        //!cpp:function::
        const char * getDescription() const;
        //!cpp:function::
        void setDescription(const char * description);
        
        //!cpp:function::
        void serialize(std::ostream & os) const;
        
        ///////////////////////////////////////////////////////////////////////////
        //!rst:: .. _cfgresource_section:
        // 
        // Resources
        // ^^^^^^^^^
        // Given a lut src name, where should we find it?
        
        //!cpp:function::
        ConstContextRcPtr getCurrentContext() const;
        
        //!cpp:function::
        const char * getSearchPath() const;
        void setSearchPath(const char * path);
        
        //!cpp:function::
        const char * getWorkingDir() const;
        void setWorkingDir(const char * dirname);
        
        ///////////////////////////////////////////////////////////////////////////
        //!rst:: .. _cfgcolorspaces_section:
        // 
        // ColorSpaces
        // ^^^^^^^^^^^
        
        //!cpp:function::
        int getNumColorSpaces() const;
        //!cpp:function:: This will null if an invalid index is specified
        const char * getColorSpaceNameByIndex(int index) const;
        
        //!rst::
        // .. note::
        //    These fcns all accept either a colorspace OR role name.
        //    (Colorspace names take precedence over roles)
        
        //!cpp:function:: This will return null if the specified name is not
        // found.
        ConstColorSpaceRcPtr getColorSpace(const char * name) const;
        //!cpp:function::
        int getIndexForColorSpace(const char * name) const;
        
        //!cpp:function::
        // .. note::
        //    If another colorspace was already registered with the same name,
        //    this will overwrite it. This stores a copy of the specified
        //    colorspace
        void addColorSpace(const ConstColorSpaceRcPtr & cs);
        //!cpp:function::
        void clearColorSpaces();
        
        //!cpp:function:: Given the specified string, get the longest,
        // right-most, ColorSpace substring that appears.
        //
        // * If strict parsing is enabled, and no colorspace are found return a
        //   null pointer.
        // * If strict parsing is disabled, return ROLE_DEFAULT (if defined).
        // * If the default role is not defined, return a null pointer.
        const char * parseColorSpaceFromString(const char * str) const;
        
        //!cpp:function::
        bool isStrictParsingEnabled() const;
        //!cpp:function::
        void setStrictParsingEnabled(bool enabled);
        
        ///////////////////////////////////////////////////////////////////////////
        //!rst:: .. _cfgroles_section:
        // 
        // Roles
        // ^^^^^
        // Are like an alias for a colorspace. You can query the colorSpace
        // corresponding to a role using the normal getColorSpace fcn.
        
        //!cpp:function::
        // .. note::
        //    Setting the ``colorSpaceName`` name to a null string unsets it
        void setRole(const char * role, const char * colorSpaceName);
        //!cpp:function::
        int getNumRoles() const;
        //!cpp:function:: returns true if the role has been defined
        bool hasRole(const char * role) const;
        //!cpp:function:: get the role name at index, this will return values
        // like 'scene_linear', 'compositing_log'.
        // Returns NULL if index is out of range
        const char * getRoleName(int index) const;
        
        ///////////////////////////////////////////////////////////////////////////
        //!rst:: .. _cfgdisplaytransforms_section:
        // 
        // DisplayTransforms
        // ^^^^^^^^^^^^^^^^^
        
        //!cpp:function::
        int getNumDisplayDeviceNames() const;
        //!cpp:function::
        const char * getDisplayDeviceName(int index) const;
        //!cpp:function::
        const char * getDefaultDisplayDeviceName() const;
        //!cpp:function::
        int getNumDisplayTransformNames(const char * device) const;
        //!cpp:function::
        const char * getDisplayTransformName(const char * device, int index) const;
        //!cpp:function::
        const char * getDefaultDisplayTransformName(const char * device) const;
        //!cpp:function::
        const char * getDisplayColorSpaceName(const char * device, const char * displayTransformName) const;
        //!cpp:function::
        void addDisplayDevice(const char * device,
                              const char * transformName,
                              const char * colorSpaceName);
        
        ///////////////////////////////////////////////////////////////////////////
        //!rst:: .. _cfgluma_section:
        // 
        // Luma
        // ^^^^
        //
        // Get the default coefficients for computing luma.
        //
        // .. note::
        //    There is no "1 size fits all" set of luma coefficients. (The
        //    values are typically different for each colorspace, and the
        //    application of them may be non-sensical depending on the
        //    intenisty coding anyways).  Thus, the 'right' answer is to make
        //    these functions on the :cpp:class:`Config` class.  However, it's
        //    often useful to have a config-wide default so here it is. We will
        //    add the colorspace specific luma call if/when another client is
        //    interesting in using it.
        
        //!cpp:function::
        void getDefaultLumaCoefs(float * rgb) const;
        //!cpp:function:: These should be normalized. (sum to 1.0 exactly)
        void setDefaultLumaCoefs(const float * rgb);
        
        ///////////////////////////////////////////////////////////////////////////
        //!rst:: .. _cfgprocessors_section:
        // 
        // Processors
        // ^^^^^^^^^^
        //
        // Convert from inputColorSpace to outputColorSpace
        // 
        // .. note::
        //    This may provide higher fidelity than anticipated due to internal
        //    optimizations. For example, if the inputcolorspace and the
        //    outputColorSpace are members of the same family, no conversion
        //    will be applied, even though strictly speaking quantization
        //    should be added.
        // 
        // If you wish to test these calls for quantization characteristics,
        // apply in two steps, the image must contain RGB triples (though
        // arbitrary numbers of additional channels can be supported (ignored)
        // using the pixelStrideBytes arg.
        
        //!cpp:function::
        ConstProcessorRcPtr getProcessor(const ConstContextRcPtr & context,
                                         const ConstColorSpaceRcPtr & srcColorSpace,
                                         const ConstColorSpaceRcPtr & dstColorSpace) const;
        //!cpp:function::
        ConstProcessorRcPtr getProcessor(const ConstColorSpaceRcPtr & srcColorSpace,
                                         const ConstColorSpaceRcPtr & dstColorSpace) const;
        
            //!cpp:function::
        // .. note::
        //    Names can be colorspace name, role name, or a combination of both
        ConstProcessorRcPtr getProcessor(const char * srcName,
                                         const char * dstName) const;
        //!cpp:function::
        ConstProcessorRcPtr getProcessor(const ConstContextRcPtr & context,
                                         const char * srcName,
                                         const char * dstName) const;
        
        //!rst:: Get the processor for the specified transform.
        //
        // Not often needed, but will allow for the re-use of atomic OCIO
        // functionality (Such as to apply an individual LUT file)
        
        //!cpp:function::
        ConstProcessorRcPtr getProcessor(const ConstTransformRcPtr& transform) const;
        //!cpp:function::
        ConstProcessorRcPtr getProcessor(const ConstTransformRcPtr& transform,
                                         TransformDirection direction) const;
        //!cpp:function::
        ConstProcessorRcPtr getProcessor(const ConstContextRcPtr & context,
                                         const ConstTransformRcPtr& transform,
                                         TransformDirection direction) const;
        
    private:
        Config();
        ~Config();
        
        Config(const Config &);
        Config& operator= (const Config &);
        
        static void deleter(Config* c);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };
    
    //!cpp:function::
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const Config&);
    
    
    ///////////////////////////////////////////////////////////////////////////
    //!rst:: .. _colorspace_section:
    // 
    // ColorSpace
    // **********
    // The *ColorSpace* is the state of an image with respect to colorimetry
    // and color encoding.   Transforming images between different
    // *ColorSpaces* is the primary motivation for this libray.
    //
    // While a completely discussion of color spaces is beyond the scope of
    // header documentation, traditional uses would be to have *ColorSpaces*
    // corresponding to: physical capture devices (known cameras, scanners),
    // and internal 'convenience' spaces (such as scene linear, logaraithmic).
    //
    // *ColorSpaces* are specific to a particular image precision (float32,
    // uint8, etc), and the set of ColorSpaces that provide equivalent mappings
    // (at different precisions) are referred to as a 'family'.
    
    //!cpp:class::
    class OCIOEXPORT ColorSpace
    {
    public:
        //!cpp:function::
        static ColorSpaceRcPtr Create();
        
        //!cpp:function::
        ColorSpaceRcPtr createEditableCopy() const;
        
        //!cpp:function::
        const char * getName() const;
        //!cpp:function::
        void setName(const char * name);
        
        //!cpp:function::
        const char * getFamily() const;
        //!cpp:function::
        void setFamily(const char * family);
        
        //!cpp:function::
        const char * getDescription() const;
        //!cpp:function::
        void setDescription(const char * description);
        
        //!cpp:function::
        BitDepth getBitDepth() const;
        //!cpp:function::
        void setBitDepth(BitDepth bitDepth);
        
        ///////////////////////////////////////////////////////////////////////////
        //!rst::
        // Data
        // ^^^^
        // ColorSpaces that are data are treated a bit special. Basically, any
        // colorspace transforms you try to apply to them are ignored.  (Think
        // of applying a gamut mapping transform to an ID pass). Also, the
        // :cpp:class:`DisplayTransform` process obeys special 'data min' and
        // 'data max' args.
        //
        // This is traditionally used for pixel data that represents non-color
        // pixel data, such as normals, point positions, ID information, etc.
        
        //!cpp:function::
        bool isData() const;
        //!cpp:function::
        void setIsData(bool isData);
        
        ///////////////////////////////////////////////////////////////////////////
        //!rst::
        // Allocation
        // ^^^^^^^^^^
        // If this color space needs to be transferred to a limited dynamic
        // range coding space (such as during display with a GPU path, use this
        // allocation to maximize bit efficiency.
        
        //!cpp:function::
        Allocation getAllocation() const;
        //!cpp:function::
        void setAllocation(Allocation allocation);
        
        //!rst::
        // Specify the optional variable values to configure the allocation.
        // If no variables are specified, the defaults are used.
        //
        // ALLOCATION_UNIFORM::
        //    
        //    2 vars: [min, max]
        //
        // ALLOCATION_LG2::
        //    
        //    2 vars: [lg2min, lg2max]
        //    3 vars: [lg2min, lg2max, linear_offset]
        
        //!cpp:function::
        int getAllocationNumVars() const;
        //!cpp:function::
        void getAllocationVars(float * vars) const;
        //!cpp:function::
        void setAllocationVars(int numvars, const float * vars);
        
        ///////////////////////////////////////////////////////////////////////////
        //!rst::
        // Transform
        // ^^^^^^^^^
        
        //!cpp:function::
        ConstTransformRcPtr getTransform(ColorSpaceDirection dir) const;
        //!cpp:function::
        TransformRcPtr getEditableTransform(ColorSpaceDirection dir);
        //!cpp:function::
        void setTransform(const ConstTransformRcPtr & transform,
                          ColorSpaceDirection dir);
        //!cpp:function:: Setting a transform to a non-null call makes it specified
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
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };
    
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const ColorSpace&);
    
    
    ///////////////////////////////////////////////////////////////////////////
    //!rst::
    // Processor
    // *********
    
    //!cpp:class::
    class OCIOEXPORT Processor
    {
    public:
        //!cpp:function::
        virtual ~Processor();
        
        //!cpp:function::
        virtual bool isNoOp() const = 0;
        
        ///////////////////////////////////////////////////////////////////////////
        //!rst::
        // CPU Path
        // ^^^^^^^^
        
        //!cpp:function:: Apply to an image
        virtual void apply(ImageDesc& img) const = 0;
        
        //!rst::
        // Apply to a single pixel
        // 
        // .. note::
        //    This is not as efficicent as applying to an entire image at once.
        //    If you are processing multiple pixels, and have the flexiblity,
        //    use the above function instead.
        
        //!cpp:function:: 
        virtual void applyRGB(float * pixel) const = 0;
        //!cpp:function:: 
        virtual void applyRGBA(float * pixel) const = 0;
        
        ///////////////////////////////////////////////////////////////////////////
        //!rst::
        // GPU Path
        // ^^^^^^^^
        // Get the 3d lut + cg shader for the specified
        // :cpp:class:`DisplayTransform`
        //
        // cg signature will be::
        //    
        //    shaderFcnName(in half4 inPixel, const uniform sampler3D lut3d)
        //
        // lut3d should be size: 3 * edgeLen * edgeLen * edgeLen
        // return 0 if unknown
        
        //!cpp:function::
        virtual const char * getGpuShaderText(const GpuShaderDesc & shaderDesc) const = 0;
        //!cpp:function::
        virtual const char * getGpuShaderTextCacheID(const GpuShaderDesc & shaderDesc) const = 0;
        
        //!cpp:function::
        virtual void getGpuLut3D(float* lut3d, const GpuShaderDesc & shaderDesc) const = 0;
        //!cpp:function::
        virtual const char * getGpuLut3DCacheID(const GpuShaderDesc & shaderDesc) const = 0;
        
    private:
        Processor& operator= (const Processor &);
    };
    
    
    ///////////////////////////////////////////////////////////////////////////
    //!rst::
    // ImageDesc
    // *********
    
    //!rst::
    // .. c:var:: const ptrdiff_t AutoStride
    //    
    //    AutoStride
    const ptrdiff_t AutoStride = std::numeric_limits<ptrdiff_t>::min();
    
    //!cpp:class::
    // This is a light-weight wrapper around an image, that provides a context
    // for pixel access.   This does NOT claim ownership of the pixels, or do
    // any internal allocations or copying of image data
    class OCIOEXPORT ImageDesc
    {
    public:
        //!cpp:function::
        virtual ~ImageDesc();
        
        //!cpp:function::
        virtual long getWidth() const = 0;
        //!cpp:function::
        virtual long getHeight() const = 0;
        
        //!cpp:function::
        virtual ptrdiff_t getXStrideBytes() const = 0;
        //!cpp:function::
        virtual ptrdiff_t getYStrideBytes() const = 0;
        
        //!cpp:function::
        virtual float* getRData() const = 0;
        //!cpp:function::
        virtual float* getGData() const = 0;
        //!cpp:function::
        virtual float* getBData() const = 0;
    
    private:
        ImageDesc& operator= (const ImageDesc &);
    };
    
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const ImageDesc&);
    
    
    ///////////////////////////////////////////////////////////////////////////
    //!rst::
    // PackedImageDesc
    // ^^^^^^^^^^^^^^^
    
    //!cpp:class::
    class OCIOEXPORT PackedImageDesc : public ImageDesc
    {
    public:
        //!cpp:function::
        PackedImageDesc(float * data,
                        long width, long height,
                        long numChannels,
                        ptrdiff_t chanStrideBytes = AutoStride,
                        ptrdiff_t xStrideBytes = AutoStride,
                        ptrdiff_t yStrideBytes = AutoStride);
        //!cpp:function::
        virtual ~PackedImageDesc();
        
        //!cpp:function::
        virtual long getWidth() const;
        //!cpp:function::
        virtual long getHeight() const;
        
        //!cpp:function::
        virtual ptrdiff_t getXStrideBytes() const;
        //!cpp:function::
        virtual ptrdiff_t getYStrideBytes() const;
        
        //!cpp:function::
        virtual float* getRData() const;
        //!cpp:function::
        virtual float* getGData() const;
        //!cpp:function::
        virtual float* getBData() const;
        
    private:
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
        
        PackedImageDesc(const PackedImageDesc &);
        PackedImageDesc& operator= (const PackedImageDesc &);
    };
    
    
    ///////////////////////////////////////////////////////////////////////////
    //!rst::
    // PlanarImageDesc
    // ^^^^^^^^^^^^^^^
    
    //!cpp:class::
    class OCIOEXPORT PlanarImageDesc : public ImageDesc
    {
    public:
        //!cpp:function::
        PlanarImageDesc(float * rData, float * gData, float * bData,
                        long width, long height,
                        ptrdiff_t yStrideBytes = AutoStride);
        //!cpp:function::
        virtual ~PlanarImageDesc();
        
        //!cpp:function::
        virtual long getWidth() const;
        //!cpp:function::
        virtual long getHeight() const;
        
        //!cpp:function::
        virtual ptrdiff_t getXStrideBytes() const;
        //!cpp:function::
        virtual ptrdiff_t getYStrideBytes() const;
        
        //!cpp:function::
        virtual float* getRData() const;
        //!cpp:function::
        virtual float* getGData() const;
        //!cpp:function::
        virtual float* getBData() const;
        
    private:
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
        
        PlanarImageDesc(const PlanarImageDesc &);
        PlanarImageDesc& operator= (const PlanarImageDesc &);
    };
    
    
    ///////////////////////////////////////////////////////////////////////////
    //!rst::
    // GpuShaderDesc
    // *************
    
    //!cpp:class::
    class OCIOEXPORT GpuShaderDesc
    {
    public:
        //!cpp:function::
        GpuShaderDesc();
        //!cpp:function::
        ~GpuShaderDesc();
        
        //!cpp:function::
        void setLanguage(GpuLanguage lang);
        //!cpp:function::
        GpuLanguage getLanguage() const;
        
        //!cpp:function::
        void setFunctionName(const char * name);
        //!cpp:function::
        const char * getFunctionName() const;
        
        //!cpp:function::
        void setLut3DEdgeLen(int len);
        //!cpp:function::
        int getLut3DEdgeLen() const;
        
    private:
        
        GpuShaderDesc(const GpuShaderDesc &);
        GpuShaderDesc& operator= (const GpuShaderDesc &);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };
    
    
    ///////////////////////////////////////////////////////////////////////////
    //!rst::
    // Context
    // *******
    
    //!cpp:class::
    class OCIOEXPORT Context
    {
    public:
        //!cpp:function::
        static ContextRcPtr Create();
        
        //!cpp:function::
        ContextRcPtr createEditableCopy() const;
        
        //!cpp:function::
        void setSearchPath(const char * path);
        //!cpp:function::
        const char * getSearchPath() const;
        
        //!cpp:function::
        void setWorkingDir(const char * dirname);
        //!cpp:function::
        const char * getWorkingDir() const;
        
        //!cpp:function::
        void setStringVar(const char * name, const char * value);
        //!cpp:function::
        const char * getStringVar(const char * name) const;
        
        //!cpp:function:: Seed all string vars with the current environment
        void loadEnvironment();
        
        //! Do a string lookup.
        //!cpp:function:: Do a file lookup.
        // 
        // Evaluate the specified variable (as needed). Will not throw exceptions.
        const char * resolveStringVar(const char * val) const;
        
        //! Do a file lookup.
        //!cpp:function:: Do a file lookup.
        // 
        // Evaluate all variables (as needed).
        // Also, walk the full search path until the file is found.
        // If the filename cannot be found, an exception will be thrown.
        const char * resolveFileLocation(const char * filename) const;
    
    private:
        Context();
        ~Context();
        
        Context(const Context &);
        Context& operator= (const Context &);
        
        static void deleter(Context* c);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };
}
OCIO_NAMESPACE_EXIT

#endif // INCLUDED_OCIO_OPENCOLORIO_H

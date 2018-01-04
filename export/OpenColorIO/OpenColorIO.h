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
#include <cstddef>

#include "OpenColorABI.h"
#include "OpenColorTypes.h"
#include "OpenColorTransforms.h"

/*!rst::
C++ API
=======

**Usage Example:** *Compositing plugin that converts from "log" to "lin"*

.. code-block:: cpp
   
   #include <OpenColorIO/OpenColorIO.h>
   namespace OCIO = OCIO_NAMESPACE;
   
   try
   {
       // Get the global OpenColorIO config
       // This will auto-initialize (using $OCIO) on first use
       OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
       
       // Get the processor corresponding to this transform.
       OCIO::ConstProcessorRcPtr processor = config->getProcessor(OCIO::ROLE_COMPOSITING_LOG,
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
    // Exceptions
    // **********
    
    //!cpp:class:: An exception class to throw for errors detected at
    // runtime.
    //
    // .. warning:: 
    //    All functions in the Config class can potentially throw this exception.
    class OCIOEXPORT Exception : public std::exception
    {
    public:
        //!cpp:function:: Constructor that takes a string as the exception message.
        Exception(const char *) throw();
        //!cpp:function:: Constructor that takes an exception pointer.
        Exception(const Exception&) throw();
        //!cpp:function:: Constructor that takes an exception pointer and returns an exception pointer (???).
        Exception& operator=(const Exception&) throw();
        //!cpp:function::
        virtual ~Exception() throw();
        //!cpp:function::
        virtual const char* what() const throw();
        
    private:
        //Add pragma warnings, STL member is private and not consumed by client of DLL
        #ifdef _WIN32
        #pragma warning(push)
        #pragma warning(disable:4251)
        #endif // _WIN32

        std::string msg_;

        #ifdef _WIN32
        #pragma warning(pop)
        #endif // _WIN32
    };
    
    //!cpp:class:: An exception class for errors detected at
    // runtime, thrown when OCIO cannot find a file that is expected to
    // exist. This is provided as a custom type to
    // distinguish cases where one wants to continue looking for
    // missing files, but wants to properly fail
    // for other error conditions.
    
    class OCIOEXPORT ExceptionMissingFile : public Exception
    {
    public:
        //!cpp:function:: Constructor that takes a string as the exception message.
        ExceptionMissingFile(const char *) throw();
        //!cpp:function:: Constructor that takes an existing exception.
        ExceptionMissingFile(const ExceptionMissingFile&) throw();
    };
    
    ///////////////////////////////////////////////////////////////////////////
    //!rst::
    // Global
    // ******
    
    //!cpp:function::
    // OpenColorIO, during normal usage, tends to cache certain information
    // (such as the contents of LUTs on disk, intermediate results, etc.).
    // Calling this function will flush all such information.
    // Under normal usage, this is not necessary, but it can be helpful in particular instances,
    // such as designing OCIO profiles, and wanting to re-read luts without
    // restarting.
    
    extern OCIOEXPORT void ClearAllCaches();
    
    //!cpp:function:: Get the version number for the library, as a
    // dot-delimited string (e.g., "1.0.0"). This is also available
    // at compile time as OCIO_VERSION.
    
    extern OCIOEXPORT const char * GetVersion();
    
    //!cpp:function:: Get the version number for the library, as a
    // single 4-byte hex number (e.g., 0x01050200 for "1.5.2"), to be used
    // for numeric comparisons. This is also available
    // at compile time as OCIO_VERSION_HEX.
    
    extern OCIOEXPORT int GetVersionHex();
    
    //!cpp:function:: Get the global logging level.
    // You can override this at runtime using the :envvar:`OCIO_LOGGING_LEVEL`
    // environment variable. The client application that sets this should use
    // :cpp:func:`SetLoggingLevel`, and not the environment variable. The default value is INFO.
    
    extern OCIOEXPORT LoggingLevel GetLoggingLevel();
    
    //!cpp:function:: Set the global logging level.
    extern OCIOEXPORT void SetLoggingLevel(LoggingLevel level);
    
    
    ///////////////////////////////////////////////////////////////////////////
    //!rst::
    // Config
    // ******
    //
    // A config defines all the color spaces to be available at runtime.
    // 
    // The color configuration (:cpp:class:`Config`) is the main object for
    // interacting with this library. It encapsulates all of the information
    // necessary to use customized :cpp:class:`ColorSpaceTransform` and
    // :cpp:class:`DisplayTransform` operations.
    // 
    // See the :ref:`user-guide` for more information on
    // selecting, creating, and working with custom color configurations.
    // 
    // For applications interested in using only one color config at
    // a time (this is the vast majority of apps), their API would
    // traditionally get the global configuration and use that, as opposed to
    // creating a new one. This simplifies the use case for
    // plugins and bindings, as it alleviates the need to pass around configuration
    // handles.
    // 
    // An example of an application where this would not be sufficient would be
    // a multi-threaded image proxy server (daemon), which wished to handle
    // multiple show configurations in a single process concurrently. This
    // app would need to keep multiple configurations alive, and to manage them
    // appropriately.
    // 
    // Roughly speaking, a novice user should select a
    // default configuration that most closely approximates the use case
    // (animation, visual effects, etc.), and set the :envvar:`OCIO` environment
    // variable to point at the root of that configuration.
    // 
    // .. note::
    //    Initialization using environment variables is typically preferable in
    //    a multi-app ecosystem, as it allows all applications to be
    //    consistently configured.
    //
    // See :ref:`developers-usageexamples`
    
    //!cpp:function:: Get the current configuration.
    
    extern OCIOEXPORT ConstConfigRcPtr GetCurrentConfig();
    
    //!cpp:function:: Set the current configuration. This will then store a copy of the specified config.
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
        
        //!cpp:function:: Constructor a default empty configuration.
        static ConfigRcPtr Create();
        //!cpp:function::  Constructor a configuration using the OCIO environmnet variable.
        static ConstConfigRcPtr CreateFromEnv();
        //!cpp:function:: Constructor a configuration using a specific config file.
        static ConstConfigRcPtr CreateFromFile(const char * filename);
        //!cpp:function::
        static ConstConfigRcPtr CreateFromStream(std::istream & istream);
        
        //!cpp:function::
        ConfigRcPtr createEditableCopy() const;
        
        //!cpp:function::
        // This will throw an exception if the config is malformed. The most
        // common error occurs when references are made to colorspaces that do not
        // exist.
        void sanityCheck() const;
        
        //!cpp:function::
        const char * getDescription() const;
        //!cpp:function::
        void setDescription(const char * description);
        
        //!cpp:function::
        // Returns the string representation of the Config in YAML text form.
        // This is typically stored on disk in a file with the extension .ocio.
        void serialize(std::ostream & os) const;
        
        //!cpp:function::
        // This will produce a hash of the all colorspace definitions, etc.
        // All external references, such as files used in FileTransforms, etc.,
        // will be incorporated into the cacheID. While the contents of
        // the files are not read, the file system is queried for relavent
        // information (mtime, inode) so that the config's cacheID will
        // change when the underlying luts are updated.
        // If a context is not provided, the current Context will be used.
        // If a null context is provided, file references will not be taken into
        // account (this is essentially a hash of Config::serialize).
        const char * getCacheID() const;
        //!cpp:function::
        const char * getCacheID(const ConstContextRcPtr & context) const;
        
        ///////////////////////////////////////////////////////////////////////////
        //!rst:: .. _cfgresource_section:
        // 
        // Resources
        // ^^^^^^^^^
        // Given a lut src name, where should we find it?
        
        //!cpp:function::
        ConstContextRcPtr getCurrentContext() const;
        
        //!cpp:function::
        void addEnvironmentVar(const char * name, const char * defaultValue);
        //!cpp:function::
        int getNumEnvironmentVars() const;
        //!cpp:function::
        const char * getEnvironmentVarNameByIndex(int index) const;
        //!cpp:function::
        const char * getEnvironmentVarDefault(const char * name) const;
        //!cpp:function::
        void clearEnvironmentVars();
        //!cpp:function::
        void setEnvironmentMode(EnvironmentMode mode);
        //!cpp:function::
        EnvironmentMode getEnvironmentMode() const;
        //!cpp:function::
        void loadEnvironment();
        
        //!cpp:function::
        const char * getSearchPath() const;
        //!cpp:function::
        void setSearchPath(const char * path);
        
        //!cpp:function::
        const char * getWorkingDir() const;
        //!cpp:function::
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
        //    These fcns all accept either a color space OR role name.
        //    (Colorspace names take precedence over roles.)
        
        //!cpp:function:: This will return null if the specified name is not
        // found.
        ConstColorSpaceRcPtr getColorSpace(const char * name) const;
        //!cpp:function::
        int getIndexForColorSpace(const char * name) const;
        
        //!cpp:function::
        // .. note::
        //    If another color space is already registered with the same name,
        //    this will overwrite it. This stores a copy of the specified
        //    color space.
        void addColorSpace(const ConstColorSpaceRcPtr & cs);
        //!cpp:function::
        void clearColorSpaces();
        
        //!cpp:function:: Given the specified string, get the longest,
        // right-most, colorspace substring that appears.
        //
        // * If strict parsing is enabled, and no color space is found, return
        //   an empty string.
        // * If strict parsing is disabled, return ROLE_DEFAULT (if defined).
        // * If the default role is not defined, return an empty string.
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
        // A role is like an alias for a colorspace. You can query the colorspace
        // corresponding to a role using the normal getColorSpace fcn.
        
        //!cpp:function::
        // .. note::
        //    Setting the ``colorSpaceName`` name to a null string unsets it.
        void setRole(const char * role, const char * colorSpaceName);
        //!cpp:function::
        int getNumRoles() const;
        //!cpp:function:: Return true if the role has been defined.
        bool hasRole(const char * role) const;
        //!cpp:function:: Get the role name at index, this will return values
        // like 'scene_linear', 'compositing_log'.
        // Return empty string if index is out of range.
        const char * getRoleName(int index) const;
        
        
        
        ///////////////////////////////////////////////////////////////////////////
        //!rst:: .. _cfgdisplayview_section:
        // 
        // Display/View Registration
        // ^^^^^^^^^^^^^^^^^^^^^^^^^
        //
        // Looks is a potentially comma (or colon) delimited list of lookNames,
        // Where +/- prefixes are optionally allowed to denote forward/inverse
        // look specification. (And forward is assumed in the absence of either)
        
        //!cpp:function::
        const char * getDefaultDisplay() const;
        //!cpp:function::
        int getNumDisplays() const;
        //!cpp:function::
        const char * getDisplay(int index) const;
        
        //!cpp:function::
        const char * getDefaultView(const char * display) const;
        //!cpp:function::
        int getNumViews(const char * display) const;
        //!cpp:function::
        const char * getView(const char * display, int index) const;
        
        //!cpp:function::
        const char * getDisplayColorSpaceName(const char * display, const char * view) const;
        //!cpp:function::
        const char * getDisplayLooks(const char * display, const char * view) const;
        
        //!cpp:function:: For the (display,view) combination,
        // specify which colorSpace and look to use.
        // If a look is not desired, then just pass an empty string
        
        void addDisplay(const char * display, const char * view,
                        const char * colorSpaceName, const char * looks);
        
        //!cpp:function::
        void clearDisplays();
        
        // $OCIO_ACTIVE_DISPLAYS envvar can, at runtime, optionally override the allowed displays.
        // It is a comma or colon delimited list.
        // Active displays that are not in the specified profile will be ignored, and the
        // left-most defined display will be the default.
        
        //!cpp:function:: Comma-delimited list of display names.
        void setActiveDisplays(const char * displays);
        //!cpp:function::
        const char * getActiveDisplays() const;
        
        // $OCIO_ACTIVE_VIEWS envvar can, at runtime, optionally override the allowed views.
        // It is a comma or colon delimited list.
        // Active views that are not in the specified profile will be ignored, and the
        // left-most defined view will be the default.
        
        //!cpp:function:: Comma-delimited list of view names.
        void setActiveViews(const char * views);
        //!cpp:function::
        const char * getActiveViews() const;
        
        
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
        //    application of them may be nonsensical depending on the
        //    intensity coding anyways). Thus, the 'right' answer is to make
        //    these functions on the :cpp:class:`Config` class. However, it's
        //    often useful to have a config-wide default so here it is. We will
        //    add the colorspace specific luma call if/when another client is
        //    interesting in using it.
        
        //!cpp:function::
        void getDefaultLumaCoefs(float * rgb) const;
        //!cpp:function:: These should be normalized (sum to 1.0 exactly).
        void setDefaultLumaCoefs(const float * rgb);
        
        
        ///////////////////////////////////////////////////////////////////////////
        //!rst:: .. _cflooka_section:
        // 
        // Look
        // ^^^^
        //
        // Manager per-shot look settings.
        //
        
        //!cpp:function::
        ConstLookRcPtr getLook(const char * name) const;
        
        //!cpp:function::
        int getNumLooks() const;
        
        //!cpp:function::
        const char * getLookNameByIndex(int index) const;
        
        //!cpp:function::
        void addLook(const ConstLookRcPtr & look);
        
        //!cpp:function::
        void clearLooks();
        
        
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
        // apply in two steps; the image must contain RGB triples (though
        // arbitrary numbers of additional channels can be supported (ignored)
        // using the pixelStrideBytes arg).
        
        //!cpp:function::
        ConstProcessorRcPtr getProcessor(const ConstContextRcPtr & context,
                                         const ConstColorSpaceRcPtr & srcColorSpace,
                                         const ConstColorSpaceRcPtr & dstColorSpace) const;
        //!cpp:function::
        ConstProcessorRcPtr getProcessor(const ConstColorSpaceRcPtr & srcColorSpace,
                                         const ConstColorSpaceRcPtr & dstColorSpace) const;
        
            //!cpp:function::
        // .. note::
        //    Names can be colorspace name, role name, or a combination of both.
        ConstProcessorRcPtr getProcessor(const char * srcName,
                                         const char * dstName) const;
        //!cpp:function::
        ConstProcessorRcPtr getProcessor(const ConstContextRcPtr & context,
                                         const char * srcName,
                                         const char * dstName) const;
        
        //!rst:: Get the processor for the specified transform.
        //
        // Not often needed, but will allow for the re-use of atomic OCIO
        // functionality (such as to apply an individual LUT file).
        
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
    
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const Config&);
    
    
    ///////////////////////////////////////////////////////////////////////////
    //!rst:: .. _colorspace_section:
    // 
    // ColorSpace
    // **********
    // The *ColorSpace* is the state of an image with respect to colorimetry
    // and color encoding. Transforming images between different
    // *ColorSpaces* is the primary motivation for this library.
    //
    // While a complete discussion of color spaces is beyond the scope of
    // header documentation, traditional uses would be to have *ColorSpaces*
    // corresponding to: physical capture devices (known cameras, scanners),
    // and internal 'convenience' spaces (such as scene linear, logarithmic).
    //
    // *ColorSpaces* are specific to a particular image precision (float32,
    // uint8, etc.), and the set of ColorSpaces that provide equivalent mappings
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
        
        //!cpp:function::Get the family, for use in user interfaces (optional)
        const char * getFamily() const;
        //!cpp:function::Set the family, for use in user interfaces (optional)
        void setFamily(const char * family);
        
        //!cpp:function::Get the ColorSpace group name (used for equality comparisons)
        // This allows no-op transforms between different colorspaces.
        // If an equalityGroup is not defined (an empty string), it will be considered
        // unique (i.e., it will not compare as equal to other ColorSpaces with an
        // empty equality group).  This is often, though not always, set to the
        // same value as 'family'.
        const char * getEqualityGroup() const;
        //!cpp:function::
        void setEqualityGroup(const char * equalityGroup);
        
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
        // colorspace transforms you try to apply to them are ignored. (Think
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
        // If this colorspace needs to be transferred to a limited dynamic
        // range coding space (such as during display with a GPU path), use this
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
        // If a transform in the specified direction has been specified,
        // return it. Otherwise return a null ConstTransformRcPtr
        ConstTransformRcPtr getTransform(ColorSpaceDirection dir) const;
        //!cpp:function::
        // Specify the transform for the appropriate direction.
        // Setting the transform to null will clear it.
        void setTransform(const ConstTransformRcPtr & transform,
                          ColorSpaceDirection dir);
    
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
    //!rst:: .. _look_section:
    // 
    // Look
    // ****
    // The *Look* is an 'artistic' image modification, in a specified image
    // state.
    // The processSpace defines the ColorSpace the image is required to be
    // in, for the math to apply correctly.
    
    //!cpp:class::
    class OCIOEXPORT Look
    {
    public:
        //!cpp:function::
        static LookRcPtr Create();
        
        //!cpp:function::
        LookRcPtr createEditableCopy() const;
        
        //!cpp:function::
        const char * getName() const;
        //!cpp:function::
        void setName(const char * name);
        
        //!cpp:function::
        const char * getProcessSpace() const;
        //!cpp:function::
        void setProcessSpace(const char * processSpace);
        
        //!cpp:function::
        ConstTransformRcPtr getTransform() const;
        //!cpp:function:: Setting a transform to a non-null call makes it allowed.
        void setTransform(const ConstTransformRcPtr & transform);
        
        //!cpp:function::
        ConstTransformRcPtr getInverseTransform() const;
        //!cpp:function:: Setting a transform to a non-null call makes it allowed.
        void setInverseTransform(const ConstTransformRcPtr & transform);

        //!cpp:function::
        const char * getDescription() const;
        //!cpp:function::
        void setDescription(const char * description);
    private:
        Look();
        ~Look();
        
        Look(const Look &);
        Look& operator= (const Look &);
        
        static void deleter(Look* c);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };
    
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const Look&);
    
    
    ///////////////////////////////////////////////////////////////////////////
    //!rst::
    // Processor
    // *********
    
    //!cpp:class::
    class OCIOEXPORT Processor
    {
    public:
        //!cpp:function::
        static ProcessorRcPtr Create();
        
        //!cpp:function::
        bool isNoOp() const;
        
        //!cpp:function:: does the processor represent an image transformation that
        //                introduces crosstalk between the image channels
        bool hasChannelCrosstalk() const;
        
        //!cpp:function::
        ConstProcessorMetadataRcPtr getMetadata() const;
        
        ///////////////////////////////////////////////////////////////////////////
        //!rst::
        // CPU Path
        // ^^^^^^^^
        
        //!cpp:function:: Apply to an image.
        void apply(ImageDesc& img) const;
        
        //!rst::
        // Apply to a single pixel.
        // 
        // .. note::
        //    This is not as efficient as applying to an entire image at once.
        //    If you are processing multiple pixels, and have the flexibility,
        //    use the above function instead.
        
        //!cpp:function:: 
        void applyRGB(float * pixel) const;
        //!cpp:function:: 
        void applyRGBA(float * pixel) const;
        
        //!cpp:function:: 
        const char * getCpuCacheID() const;
        
        ///////////////////////////////////////////////////////////////////////////
        //!rst::
        // GPU Path
        // ^^^^^^^^
        // Get the 3d lut + cg shader for the specified
        // :cpp:class:`DisplayTransform`.
        //
        // cg signature will be::
        //    
        //    shaderFcnName(in half4 inPixel, const uniform sampler3D lut3d)
        //
        // lut3d should be size: 3 * edgeLen * edgeLen * edgeLen
        // return 0 if unknown
        
        //!cpp:function::
        const char * getGpuShaderText(const GpuShaderDesc & shaderDesc) const;
        //!cpp:function::
        const char * getGpuShaderTextCacheID(const GpuShaderDesc & shaderDesc) const;
        
        //!cpp:function::
        void getGpuLut3D(float* lut3d, const GpuShaderDesc & shaderDesc) const;
        //!cpp:function::
        const char * getGpuLut3DCacheID(const GpuShaderDesc & shaderDesc) const;
        
    private:
        Processor();
        ~Processor();
        
        Processor(const Processor &);
        Processor& operator= (const Processor &);
        
        static void deleter(Processor* c);
        
        friend class Config;
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };
    
    
    //!cpp:class::
    // This class contains meta information about the process that generated
    // this processor.  The results of these functions do not
    // impact the pixel processing.
    
    class OCIOEXPORT ProcessorMetadata
    {
    public:
        //!cpp:function::
        static ProcessorMetadataRcPtr Create();
        
        //!cpp:function::
        int getNumFiles() const;
        //!cpp:function::
        const char * getFile(int index) const;
        
        //!cpp:function::
        int getNumLooks() const;
        //!cpp:function::
        const char * getLook(int index) const;
        
        //!cpp:function::
        void addFile(const char * fname);
        //!cpp:function::
        void addLook(const char * look);
    private:
        ProcessorMetadata();
        ~ProcessorMetadata();
        ProcessorMetadata(const ProcessorMetadata &);
        ProcessorMetadata& operator= (const ProcessorMetadata &);
        
        static void deleter(ProcessorMetadata* c);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
    };
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    //!rst::
    // Baker
    // *****
    // 
    // In certain situations it is necessary to serialize transforms into a variety
    // of application specific lut formats. The Baker can be used to create lut
    // formats that ocio supports for writing.
    // 
    // **Usage Example:** *Bake a houdini sRGB viewer lut*
    // 
    // .. code-block:: cpp
    //    
    //    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromEnv();
    //    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    //    baker->setConfig(config);
    //    baker->setFormat("houdini"); // set the houdini type
    //    baker->setType("3D"); // we want a 3D lut
    //    baker->setInputSpace("lnf");
    //    baker->setShaperSpace("log");
    //    baker->setTargetSpace("sRGB");
    //    std::ostringstream out;
    //    baker->bake(out); // fresh bread anyone!
    //    std::cout << out.str();
    
    class OCIOEXPORT Baker
    {
    public:
        //!cpp:function:: create a new Baker
        static BakerRcPtr Create();
        
        //!cpp:function:: create a copy of this Baker
        BakerRcPtr createEditableCopy() const;
        
        //!cpp:function:: set the config to use
        void setConfig(const ConstConfigRcPtr & config);
        //!cpp:function:: get the config to use
        ConstConfigRcPtr getConfig() const;
        
        //!cpp:function:: set the lut output format
        void setFormat(const char * formatName);
        //!cpp:function:: get the lut output format
        const char * getFormat() const;
        
        // TODO: Change this to an enum
        //!cpp:function:: set the lut output type (1D or 3D)
        void setType(const char * type);
        //!cpp:function:: get the lut output type
        const char * getType() const;
        
        //!cpp:function:: set *optional* meta data for luts that support it
        void setMetadata(const char * metadata);
        //!cpp:function:: get the meta data that has been set
        const char * getMetadata() const;
        
        //!cpp:function:: set the input ColorSpace that the lut will be
        // applied to
        void setInputSpace(const char * inputSpace);
        //!cpp:function:: get the input ColorSpace that has been set
        const char * getInputSpace() const;
        
        //!cpp:function:: set an *optional* ColorSpace to be used to shape /
        // transfer the input colorspace. This is mostly used to allocate
        // an HDR luminance range into an LDR one. If a shaper space
        // is not explicitly specified, and the file format supports one,
        // the ColorSpace Allocation will be used
        
        void setShaperSpace(const char * shaperSpace);
        //!cpp:function:: get the shaper colorspace that has been set
        const char * getShaperSpace() const;

        //!cpp:function:: set the looks to be applied during baking
        // Looks is a potentially comma (or colon) delimited list of lookNames,
        // Where +/- prefixes are optionally allowed to denote forward/inverse
        // look specification. (And forward is assumed in the absence of either)
        void setLooks(const char * looks);
        //!cpp:function:: get the looks to be applied during baking
        const char * getLooks() const;
        
        //!cpp:function:: set the target device colorspace for the lut
        void setTargetSpace(const char * targetSpace);
        //!cpp:function:: get the target colorspace that has been set
        const char * getTargetSpace() const;
        
        //!cpp:function:: override the default the shaper sample size,
        // default: <format specific>
        void setShaperSize(int shapersize);
        //!cpp:function:: get the shaper sample size
        int getShaperSize() const;
        
        //!cpp:function:: override the default cube sample size
        // default: <format specific>
        void setCubeSize(int cubesize);
        //!cpp:function:: get the cube sample size
        int getCubeSize() const;
        
        //!cpp:function:: bake the lut into the output stream
        void bake(std::ostream & os) const;
        
        //!cpp:function:: get the number of lut writers
        static int getNumFormats();
        
        //!cpp:function:: get the lut writer at index, return empty string if
        // an invalid index is specified
        static const char * getFormatNameByIndex(int index);
        static const char * getFormatExtensionByIndex(int index);
        
    private:
        Baker();
        ~Baker();
        
        Baker(const Baker &);
        Baker& operator= (const Baker &);
        
        static void deleter(Baker* o);
        
        class Impl;
        friend class Impl;
        Impl * m_impl;
        Impl * getImpl() { return m_impl; }
        const Impl * getImpl() const { return m_impl; }
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
    // for pixel access. This does NOT claim ownership of the pixels or copy
    // image data
    
    class OCIOEXPORT ImageDesc
    {
    public:
        //!cpp:function::
        virtual ~ImageDesc();
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
        // Pass the pointer to packed image data: rgbrgbrgb, etc.
        // The number of channels must be greater than or equal to 3
        // If a 4th channel is specified, it is assumed to be alpha
        // information.  Channels > 4 will be ignored.
        
        PackedImageDesc(float * data,
                        long width, long height,
                        long numChannels,
                        ptrdiff_t chanStrideBytes = AutoStride,
                        ptrdiff_t xStrideBytes = AutoStride,
                        ptrdiff_t yStrideBytes = AutoStride);
        //!cpp:function::
        virtual ~PackedImageDesc();
        
        //!cpp:function::
        float * getData() const;
        
        //!cpp:function::
        long getWidth() const;
        //!cpp:function::
        long getHeight() const;
        //!cpp:function::
        long getNumChannels() const;
        
        //!cpp:function::
        ptrdiff_t getChanStrideBytes() const;
        //!cpp:function::
        ptrdiff_t getXStrideBytes() const;
        //!cpp:function::
        ptrdiff_t getYStrideBytes() const;
        
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
        // Pass the pointer to the specified image planes: rrrr gggg bbbb, etc.
        // aData is optional, pass NULL if no alpha exists.
        // {r,g,b} Data must be specified
        
        PlanarImageDesc(float * rData, float * gData, float * bData, float * aData,
                        long width, long height,
                        ptrdiff_t yStrideBytes = AutoStride);
        //!cpp:function::
        virtual ~PlanarImageDesc();
        
        //!cpp:function::
        float* getRData() const;
        //!cpp:function::
        float* getGData() const;
        //!cpp:function::
        float* getBData() const;
        //!cpp:function::
        float* getAData() const;
        
        //!cpp:function::
        long getWidth() const;
        //!cpp:function::
        long getHeight() const;
        
        //!cpp:function::
        ptrdiff_t getYStrideBytes() const;
        
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
        
        //!cpp:function:: Set the shader program language
        void setLanguage(GpuLanguage lang);
        //!cpp:function::
        GpuLanguage getLanguage() const;
        
        //!cpp:function:: Set the function name of the shader program
        void setFunctionName(const char * name);
        //!cpp:function::
        const char * getFunctionName() const;
        
        //!cpp:function::
        void setLut3DEdgeLen(int len);
        //!cpp:function::
        int getLut3DEdgeLen() const;
        
        //!cpp:function:: 
        const char * getCacheID() const;
        
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
        const char * getCacheID() const;
        
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
        
        //!cpp:function::
        int getNumStringVars() const;
        //!cpp:function::
        const char * getStringVarNameByIndex(int index) const;
        
        //!cpp:function::
        void clearStringVars();
        
        //!cpp:function::
        void setEnvironmentMode(EnvironmentMode mode);
        
        //!cpp:function::
        EnvironmentMode getEnvironmentMode() const;
        
        //!cpp:function:: Seed all string vars with the current environment.
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
    
    extern OCIOEXPORT std::ostream& operator<< (std::ostream&, const Context&);
}
OCIO_NAMESPACE_EXIT

#endif // INCLUDED_OCIO_OPENCOLORIO_H

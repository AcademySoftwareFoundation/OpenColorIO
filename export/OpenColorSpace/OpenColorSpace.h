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


#ifndef INCLUDED_OCS_OCS_H
#define INCLUDED_OCS_OCS_H


///////////////////////////////////////////////////////////////////////////////
//
// OCS
// OpenColorSpace
// Version 0.5.7
//

// TODO: get simple display transform working. can it be expressed as an op?
// TODO: can you also generate hw transform for ops as well?

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
// TODO: add ocs package (.gz?) file, and ability to convert between representations.
// TODO: add all nuke plugins, also get official Foundry code review
// TODO: add additional lut formats
// TODO: add internal namespace for all implementation objects Ops, etc.
// TODO: Opt build as default?
// TODO: Add prettier xml output (newlines between colorspaces?)

// TODO: Cross-platform

/*
// Example use case for a compositing plugin, which converts from "log" to "lin"
try
{
    // Get the global config, which will auto-initialize
    // from the environment on first use.
    
    OCS::ConstConfigRcPtr config = OCS::GetCurrentConfig();
    
    OCS::ConstColorSpaceRcPtr csSrc = \
        config->getColorSpaceForRole(OCS::ROLE_COMPOSITING_LOG);
    
    OCS::ConstColorSpaceRcPtr csDst = \
        config->getColorSpaceForRole(OCS::ROLE_SCENE_LINEAR);
    
    // Wrap the image in a light-weight ImageDescription,
    // and convert it in place
    
    OCS::PackedImageDesc imgDesc(imageData, w, h, 4);
    config->applyTransform(imgDesc, csSrc, csDst);
}
catch(OCS::OCSException& exception)
{
    std::cerr << "OpenColorSpace Error: " << exception.what() << std::endl;
}
*/

// Namespace mojo
#define OCS_VERSION_NS v1
#define OCS_NAMESPACE_ENTER namespace SPI \
{ namespace OCS \
{ namespace OCS_VERSION_NS
#define OCS_NAMESPACE_EXIT using namespace OCS_VERSION_NS; \
} /*namespace OCS*/ \
} /*namespace SPI*/
#define OCS_NAMESPACE SPI::OCS
#define OCS_NAMESPACE_USING using namespace SPI::OCS;

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

OCS_NAMESPACE_ENTER
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
    
    class Transform;
    typedef SharedPtr<const Transform> ConstTransformRcPtr;
    typedef SharedPtr<Transform> TransformRcPtr;
    
    class GroupTransform;
    typedef SharedPtr<const GroupTransform> ConstGroupTransformRcPtr;
    typedef SharedPtr<GroupTransform> GroupTransformRcPtr;
    
    class FileTransform;
    typedef SharedPtr<const FileTransform> ConstFileTransformRcPtr;
    typedef SharedPtr<FileTransform> FileTransformRcPtr;
    
    class ImageDesc;
    class OCSException;
    
    enum TransformDirection
    {
        TRANSFORM_DIR_UNKNOWN = 0,
        TRANSFORM_DIR_FORWARD,
        TRANSFORM_DIR_INVERSE
    };
    
    
    enum ColorSpaceDirection
    {
        COLORSPACE_DIR_UNKNOWN = 0,
        COLORSPACE_DIR_TO_REFERENCE,
        COLORSPACE_DIR_FROM_REFERENCE
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
    
    enum HwAllocation {
        HW_ALLOCATION_UNKNOWN = 0,
        HW_ALLOCATION_UNIFORM,
        HW_ALLOCATION_LG2
    };
    
    enum Interpolation
    {
        INTERP_UNKNOWN = 0,
        INTERP_NEAREST, //! nearest neighbor in all dimensions
        INTERP_LINEAR   //! linear interpolation in all dimensions
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
    (animation, visual effects, etc), and set $OCS_CONFIG to point at the
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
        a new config will be created by reading the $OCS environment
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
        
        
        // Families
        
        
        
        // Roles
        ConstColorSpaceRcPtr getColorSpaceForRole(const char * role) const;
        void setColorSpaceForRole(const char * role, const char * csname);
        void unsetRole(const char * role);
        
        int getNumRoles() const;
        const char * getRole(int index) const;
        
        
        
        
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
        
        bool isTransformNoOp(const ConstColorSpaceRcPtr & srcColorSpace,
                             const ConstColorSpaceRcPtr & dstColorSpace) const;
        
        void applyTransform(ImageDesc& img,
                            const ConstColorSpaceRcPtr & srcColorSpace,
                            const ConstColorSpaceRcPtr & dstColorSpace) const;
        
        
        
        
        
        
        // Individual lut application functions
        // Can be used to apply a .lut, .dat, .lut3d, or .3dl file.
        // Not generally needed, but useful in testing.
        
        bool isTransformNoOp(const Transform& transform) const;
        
        void applyTransform(ImageDesc& imageDesc,
                            const ConstTransformRcPtr& transform,
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
        
        // HW allocation information
        HwAllocation getHWAllocation() const;
        void setHWAllocation(HwAllocation allocation);
        
        float getHWMin() const;
        void setHWMin(float min);
        
        float getHWMax() const;
        void setHWMax(float max);
        
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
    
    
    
    /////////////////////////////////////////////////////////
    //
    // Utils
    
    //! An exception class to throw for an errors detected at runtime
    //  Warning: ALL fcns on the Config class can potentially throw
    //  this exception.
    
    class OCSException : public std::exception
    {
    public:
        OCSException(const char *) throw();
        OCSException(const OCSException&) throw();
        OCSException& operator=(const OCSException&) throw();
        virtual ~OCSException() throw();
        virtual const char* what() const throw();
        
    private:
        std::string msg_;
    };
    
    
    
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
    
    const char * HwAllocationToString(HwAllocation allocation);
    HwAllocation HwAllocationFromString(const char * s);
    
    const char * InterpolationToString(Interpolation interp);
    Interpolation InterpolationFromString(const char * s);
    
    /*!
    ColorSpace Roles are used so that plugins, in addition to this API can have
    abstract ways of asking for common colorspaces, without referring to them
    by hardcoded names.
    
    Internal:
        GetHWDisplayTransform - (ROLE_SCENE_LINEAR (fstop exposure))
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
OCS_NAMESPACE_EXIT

#endif

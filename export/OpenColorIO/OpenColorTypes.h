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


#ifndef INCLUDED_OCIO_OPENCOLORTYPES_H
#define INCLUDED_OCIO_OPENCOLORTYPES_H

#include "OpenColorABI.h"

#ifndef OCIO_NAMESPACE_ENTER
#error This header cannot be used directly. Use <OpenColorIO/OpenColorIO.h> instead.
#endif

#include <limits>
#include <string>

/*!rst::
C++ Types
=========
*/

OCIO_NAMESPACE_ENTER
{
    // Predeclare all class ptr definitions
    
    //!rst::
    // Core
    // ****
    
    class OCIOEXPORT Config;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<const Config> ConstConfigRcPtr;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<Config> ConfigRcPtr;
    
    class OCIOEXPORT ColorSpace;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<const ColorSpace> ConstColorSpaceRcPtr;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<ColorSpace> ColorSpaceRcPtr;
    
    class OCIOEXPORT Look;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<const Look> ConstLookRcPtr;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<Look> LookRcPtr;
    
    class OCIOEXPORT Context;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<const Context> ConstContextRcPtr;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<Context> ContextRcPtr;
    
    class OCIOEXPORT Processor;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<const Processor> ConstProcessorRcPtr;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<Processor> ProcessorRcPtr;
    
    class OCIOEXPORT ProcessorMetadata;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<const ProcessorMetadata> ConstProcessorMetadataRcPtr;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<ProcessorMetadata> ProcessorMetadataRcPtr;
    
    class OCIOEXPORT Baker;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<const Baker> ConstBakerRcPtr;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<Baker> BakerRcPtr;
    
    class OCIOEXPORT ImageDesc;
    class OCIOEXPORT GpuShaderDesc;
    class OCIOEXPORT Exception;
    
    
    //!rst::
    // Transforms
    // **********
    
    class OCIOEXPORT Transform;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<const Transform> ConstTransformRcPtr;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<Transform> TransformRcPtr;
    
    class OCIOEXPORT AllocationTransform;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<const AllocationTransform> ConstAllocationTransformRcPtr;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<AllocationTransform> AllocationTransformRcPtr;
    
    class OCIOEXPORT CDLTransform;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<const CDLTransform> ConstCDLTransformRcPtr;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<CDLTransform> CDLTransformRcPtr;
    
    class OCIOEXPORT ColorSpaceTransform;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<const ColorSpaceTransform> ConstColorSpaceTransformRcPtr;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<ColorSpaceTransform> ColorSpaceTransformRcPtr;
    
    class OCIOEXPORT DisplayTransform;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<const DisplayTransform> ConstDisplayTransformRcPtr;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<DisplayTransform> DisplayTransformRcPtr;
    
    class OCIOEXPORT ExponentTransform;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<const ExponentTransform> ConstExponentTransformRcPtr;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<ExponentTransform> ExponentTransformRcPtr;
    
    class OCIOEXPORT FileTransform;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<const FileTransform> ConstFileTransformRcPtr;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<FileTransform> FileTransformRcPtr;
    
    class OCIOEXPORT GroupTransform;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<const GroupTransform> ConstGroupTransformRcPtr;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<GroupTransform> GroupTransformRcPtr;
    
    class OCIOEXPORT LogTransform;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<const LogTransform> ConstLogTransformRcPtr;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<LogTransform> LogTransformRcPtr;
    
    class OCIOEXPORT LookTransform;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<const LookTransform> ConstLookTransformRcPtr;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<LookTransform> LookTransformRcPtr;
    
    class OCIOEXPORT MatrixTransform;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<const MatrixTransform> ConstMatrixTransformRcPtr;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<MatrixTransform> MatrixTransformRcPtr;
    
    class OCIOEXPORT TruelightTransform;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<const TruelightTransform> ConstTruelightTransformRcPtr;
    //!cpp:type::
    typedef OCIO_SHARED_PTR<TruelightTransform> TruelightTransformRcPtr;
    
    template <class T, class U>
    inline OCIO_SHARED_PTR<T> DynamicPtrCast(OCIO_SHARED_PTR<U> const & ptr)
    {
        return OCIO_DYNAMIC_POINTER_CAST<T,U>(ptr);
    }
    
    
    //!rst::
    // Enums
    // *****
    
    enum LoggingLevel
    {
        LOGGING_LEVEL_NONE = 0,
        LOGGING_LEVEL_WARNING = 1,
        LOGGING_LEVEL_INFO = 2,
        LOGGING_LEVEL_DEBUG = 3,
        LOGGING_LEVEL_UNKNOWN = 255
    };
    
    //!cpp:type::
    enum ColorSpaceDirection
    {
        COLORSPACE_DIR_UNKNOWN = 0,
        COLORSPACE_DIR_TO_REFERENCE,
        COLORSPACE_DIR_FROM_REFERENCE
    };
    
    //!cpp:type::
    enum TransformDirection
    {
        TRANSFORM_DIR_UNKNOWN = 0,
        TRANSFORM_DIR_FORWARD,
        TRANSFORM_DIR_INVERSE
    };
    
    //!cpp:type::
    //
    // Specify the interpolation type to use
    // If the specified interpolation type is not supported in the requested
    // context (for example, using tetrahedral interpolationon 1D luts)
    // an exception will be throw.
    //
    // INTERP_BEST will choose the best interpolation type for the requested
    // context:
    //
    // Lut1D INTERP_BEST: LINEAR
    // Lut3D INTERP_BEST: LINEAR
    //
    // Note: INTERP_BEST is subject to change in minor releases, so if you
    // care about locking off on a specific interpolation type, we'd recommend
    // directly specifying it.
    
    enum Interpolation
    {
        INTERP_UNKNOWN = 0,
        INTERP_NEAREST = 1,     //! nearest neighbor in all dimensions
        INTERP_LINEAR = 2,      //! linear interpolation in all dimensions
        INTERP_TETRAHEDRAL = 3, //! tetrahedral interpolation in all directions
        INTERP_BEST = 255       //! the 'best' suitable interpolation type
    };
    
    //!cpp:type::
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
    
    //!cpp:type::
    enum Allocation {
        ALLOCATION_UNKNOWN = 0,
        ALLOCATION_UNIFORM,
        ALLOCATION_LG2
    };
    
    //!cpp:type:: Used when there is a choice of hardware shader language.
    enum GpuLanguage
    {
        GPU_LANGUAGE_UNKNOWN = 0,
        GPU_LANGUAGE_CG,           ///< Nvidia Cg shader
        GPU_LANGUAGE_GLSL_1_0,     ///< OpenGL Shading Language
        GPU_LANGUAGE_GLSL_1_3      ///< OpenGL Shading Language
    };
    
    //!cpp:type::
    enum EnvironmentMode
    {
        ENV_ENVIRONMENT_UNKNOWN = 0,
        ENV_ENVIRONMENT_LOAD_PREDEFINED,
        ENV_ENVIRONMENT_LOAD_ALL
    };
    
    //!rst::
    // Conversion
    // **********
    
    //!cpp:function::
    extern OCIOEXPORT const char * BoolToString(bool val);
    //!cpp:function::
    extern OCIOEXPORT bool BoolFromString(const char * s);
    
    //!cpp:function::
    extern OCIOEXPORT const char * LoggingLevelToString(LoggingLevel level);
    //!cpp:function::
    extern OCIOEXPORT LoggingLevel LoggingLevelFromString(const char * s);
    
    //!cpp:function::
    extern OCIOEXPORT const char * TransformDirectionToString(TransformDirection dir);
    //!cpp:function::
    extern OCIOEXPORT TransformDirection TransformDirectionFromString(const char * s);
    
    //!cpp:function::
    extern OCIOEXPORT TransformDirection GetInverseTransformDirection(TransformDirection dir);
    //!cpp:function::
    extern OCIOEXPORT TransformDirection CombineTransformDirections(TransformDirection d1,
                                                                    TransformDirection d2);
    
    //!cpp:function::
    extern OCIOEXPORT const char * ColorSpaceDirectionToString(ColorSpaceDirection dir);
    //!cpp:function::
    extern OCIOEXPORT ColorSpaceDirection ColorSpaceDirectionFromString(const char * s);
    
    //!cpp:function::
    extern OCIOEXPORT const char * BitDepthToString(BitDepth bitDepth);
    //!cpp:function::
    extern OCIOEXPORT BitDepth BitDepthFromString(const char * s);
    //!cpp:function::
    extern OCIOEXPORT bool BitDepthIsFloat(BitDepth bitDepth);
    //!cpp:function::
    extern OCIOEXPORT int BitDepthToInt(BitDepth bitDepth);
    
    //!cpp:function::
    extern OCIOEXPORT const char * AllocationToString(Allocation allocation);
    //!cpp:function::
    extern OCIOEXPORT Allocation AllocationFromString(const char * s);
    
    //!cpp:function::
    extern OCIOEXPORT const char * InterpolationToString(Interpolation interp);
    //!cpp:function::
    extern OCIOEXPORT Interpolation InterpolationFromString(const char * s);
    
    //!cpp:function::
    extern OCIOEXPORT const char * GpuLanguageToString(GpuLanguage language);
    //!cpp:function::
    extern OCIOEXPORT GpuLanguage GpuLanguageFromString(const char * s);
    
    //!cpp:function::
    extern OCIOEXPORT const char * EnvironmentModeToString(EnvironmentMode mode);
    //!cpp:function::
    extern OCIOEXPORT EnvironmentMode EnvironmentModeFromString(const char * s);
    
    
    /*!rst::
    Roles
    *****
    
    ColorSpace Roles are used so that plugins, in addition to this API can have
    abstract ways of asking for common colorspaces, without referring to them
    by hardcoded names.
    
    Internal::
       
       GetGPUDisplayTransform - (ROLE_SCENE_LINEAR (fstop exposure))
                                (ROLE_COLOR_TIMING (ASCColorCorrection))
    
    External Plugins (currently known)::
       
       Colorpicker UIs       - (ROLE_COLOR_PICKING)
       Compositor LogConvert - (ROLE_SCENE_LINEAR, ROLE_COMPOSITING_LOG)
    
    */
    
    //!rst::
    // .. c:var:: const char* ROLE_DEFAULT
    //    
    //    "default"
    extern OCIOEXPORT const char * ROLE_DEFAULT;
    //!rst::
    // .. c:var:: const char* ROLE_REFERENCE
    //    
    //    "reference"
    extern OCIOEXPORT const char * ROLE_REFERENCE;
    //!rst::
    // .. c:var:: const char* ROLE_DATA
    //    
    //    "data"
    extern OCIOEXPORT const char * ROLE_DATA;
    //!rst::
    // .. c:var:: const char* ROLE_COLOR_PICKING
    //    
    //    "color_picking"
    extern OCIOEXPORT const char * ROLE_COLOR_PICKING;
    //!rst::
    // .. c:var:: const char* ROLE_SCENE_LINEAR
    //    
    //    "scene_linear"
    extern OCIOEXPORT const char * ROLE_SCENE_LINEAR;
    //!rst::
    // .. c:var:: const char* ROLE_COMPOSITING_LOG
    //    
    //    "compositing_log"
    extern OCIOEXPORT const char * ROLE_COMPOSITING_LOG;
    //!rst::
    // .. c:var:: const char* ROLE_COLOR_TIMING
    //    
    //    "color_timing"
    extern OCIOEXPORT const char * ROLE_COLOR_TIMING;
    //!rst::
    // .. c:var:: const char* ROLE_TEXTURE_PAINT
    //    
    //    This role defines the transform for painting textures. In some
    //    workflows this is just a inverse display gamma with some limits
    extern OCIOEXPORT const char * ROLE_TEXTURE_PAINT;
    //!rst::
    // .. c:var:: const char* ROLE_MATTE_PAINT
    //    
    //    This role defines the transform for matte painting. In some workflows
    //    this is a 1D HDR to LDR allocation. It is normally combined with
    //    another display transform in the host app for preview.
    extern OCIOEXPORT const char * ROLE_MATTE_PAINT;
    
}
OCIO_NAMESPACE_EXIT

#endif

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

#include <limits>
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

#ifndef OCIO_NAMESPACE_ENTER
#error This header cannot be used directly. Use <OpenColorIO/OpenColorIO.h> instead.
#endif

OCIO_NAMESPACE_ENTER
{
    // Predeclare all class ptr definitions
    
    class OCIOEXPORT Config;
    typedef OCIO_SHARED_PTR<const Config> ConstConfigRcPtr;
    typedef OCIO_SHARED_PTR<Config> ConfigRcPtr;
    
    class OCIOEXPORT ColorSpace;
    typedef OCIO_SHARED_PTR<const ColorSpace> ConstColorSpaceRcPtr;
    typedef OCIO_SHARED_PTR<ColorSpace> ColorSpaceRcPtr;
    
    class OCIOEXPORT Processor;
    typedef OCIO_SHARED_PTR<const Processor> ConstProcessorRcPtr;
    typedef OCIO_SHARED_PTR<Processor> ProcessorRcPtr;
    
    class OCIOEXPORT ImageDesc;
    class OCIOEXPORT GpuShaderDesc;
    class OCIOEXPORT Exception;
    
    
    
    
    class OCIOEXPORT Transform;
    typedef OCIO_SHARED_PTR<const Transform> ConstTransformRcPtr;
    typedef OCIO_SHARED_PTR<Transform> TransformRcPtr;
    
    class OCIOEXPORT AllocationTransform;
    typedef OCIO_SHARED_PTR<const AllocationTransform> ConstAllocationTransformRcPtr;
    typedef OCIO_SHARED_PTR<AllocationTransform> AllocationTransformRcPtr;
    
    class OCIOEXPORT CDLTransform;
    typedef OCIO_SHARED_PTR<const CDLTransform> ConstCDLTransformRcPtr;
    typedef OCIO_SHARED_PTR<CDLTransform> CDLTransformRcPtr;
    
    class OCIOEXPORT ColorSpaceTransform;
    typedef OCIO_SHARED_PTR<const ColorSpaceTransform> ConstColorSpaceTransformRcPtr;
    typedef OCIO_SHARED_PTR<ColorSpaceTransform> ColorSpaceTransformRcPtr;
    
    class OCIOEXPORT DisplayTransform;
    typedef OCIO_SHARED_PTR<const DisplayTransform> ConstDisplayTransformRcPtr;
    typedef OCIO_SHARED_PTR<DisplayTransform> DisplayTransformRcPtr;
    
    class OCIOEXPORT ExponentTransform;
    typedef OCIO_SHARED_PTR<const ExponentTransform> ConstExponentTransformRcPtr;
    typedef OCIO_SHARED_PTR<ExponentTransform> ExponentTransformRcPtr;
    
    class OCIOEXPORT FileTransform;
    typedef OCIO_SHARED_PTR<const FileTransform> ConstFileTransformRcPtr;
    typedef OCIO_SHARED_PTR<FileTransform> FileTransformRcPtr;
    
    class OCIOEXPORT GroupTransform;
    typedef OCIO_SHARED_PTR<const GroupTransform> ConstGroupTransformRcPtr;
    typedef OCIO_SHARED_PTR<GroupTransform> GroupTransformRcPtr;
    
    class OCIOEXPORT MatrixTransform;
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
    
    enum Allocation {
        ALLOCATION_UNKNOWN = 0,
        ALLOCATION_UNIFORM,
        ALLOCATION_LG2
    };
    
    //! Used when there is a choice of hardware shader language.
    
    enum GpuLanguage
    {
        GPU_LANGUAGE_UNKNOWN = 0,
        GPU_LANGUAGE_CG,  ///< Nvidia Cg shader
        GPU_LANGUAGE_GLSL_1_0,     ///< OpenGL Shading Language
        GPU_LANGUAGE_GLSL_1_3,     ///< OpenGL Shading Language
    };
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    extern OCIOEXPORT const char * BoolToString(bool val);
    extern OCIOEXPORT bool BoolFromString(const char * s);
    
    extern OCIOEXPORT const char * TransformDirectionToString(TransformDirection dir);
    extern OCIOEXPORT TransformDirection TransformDirectionFromString(const char * s);
    
    extern OCIOEXPORT TransformDirection GetInverseTransformDirection(TransformDirection dir);
    extern OCIOEXPORT TransformDirection CombineTransformDirections(TransformDirection d1,
                                                                         TransformDirection d2);
    
    extern OCIOEXPORT const char * ColorSpaceDirectionToString(ColorSpaceDirection dir);
    extern OCIOEXPORT ColorSpaceDirection ColorSpaceDirectionFromString(const char * s);
    
    extern OCIOEXPORT const char * BitDepthToString(BitDepth bitDepth);
    extern OCIOEXPORT BitDepth BitDepthFromString(const char * s);
    extern OCIOEXPORT bool BitDepthIsFloat(BitDepth bitDepth);
    extern OCIOEXPORT int BitDepthToInt(BitDepth bitDepth);
    
    extern OCIOEXPORT const char * AllocationToString(Allocation allocation);
    extern OCIOEXPORT Allocation AllocationFromString(const char * s);
    
    extern OCIOEXPORT const char * InterpolationToString(Interpolation interp);
    extern OCIOEXPORT Interpolation InterpolationFromString(const char * s);
    
    
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
    
    extern OCIOEXPORT const char * ROLE_DEFAULT;
    extern OCIOEXPORT const char * ROLE_REFERENCE;
    extern OCIOEXPORT const char * ROLE_DATA;
    extern OCIOEXPORT const char * ROLE_COLOR_PICKING;
    extern OCIOEXPORT const char * ROLE_SCENE_LINEAR;
    extern OCIOEXPORT const char * ROLE_COMPOSITING_LOG;
    extern OCIOEXPORT const char * ROLE_COLOR_TIMING;
}
OCIO_NAMESPACE_EXIT

#endif

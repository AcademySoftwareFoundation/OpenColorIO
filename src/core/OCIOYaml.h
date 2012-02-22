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

#include <OpenColorIO/OpenColorIO.h>

#include "Platform.h"

#ifndef WINDOWS

// fwd declare yaml-cpp visibility
#pragma GCC visibility push(hidden)
namespace YAML {
    class Exception;
    class BadDereference;
    class RepresentationException;
    class EmitterException;
    class ParserException;
    class InvalidScalar;
    class KeyNotFound;
    template <typename T> class TypedKeyNotFound;
    template <> class TypedKeyNotFound<OCIO_NAMESPACE::ColorSpace>;
    template <> class TypedKeyNotFound<OCIO_NAMESPACE::Config>;
    template <> class TypedKeyNotFound<OCIO_NAMESPACE::Exception>;
    template <> class TypedKeyNotFound<OCIO_NAMESPACE::GpuShaderDesc>;
    template <> class TypedKeyNotFound<OCIO_NAMESPACE::ImageDesc>;
    template <> class TypedKeyNotFound<OCIO_NAMESPACE::Look>;
    template <> class TypedKeyNotFound<OCIO_NAMESPACE::Processor>;
    
    template <> class TypedKeyNotFound<OCIO_NAMESPACE::Transform>;
    template <> class TypedKeyNotFound<OCIO_NAMESPACE::AllocationTransform>;
    template <> class TypedKeyNotFound<OCIO_NAMESPACE::CDLTransform>;
    template <> class TypedKeyNotFound<OCIO_NAMESPACE::ColorSpaceTransform>;
    template <> class TypedKeyNotFound<OCIO_NAMESPACE::DisplayTransform>;
    template <> class TypedKeyNotFound<OCIO_NAMESPACE::ExponentTransform>;
    template <> class TypedKeyNotFound<OCIO_NAMESPACE::FileTransform>;
    template <> class TypedKeyNotFound<OCIO_NAMESPACE::GroupTransform>;
    template <> class TypedKeyNotFound<OCIO_NAMESPACE::LogTransform>;
    template <> class TypedKeyNotFound<OCIO_NAMESPACE::LookTransform>;
    template <> class TypedKeyNotFound<OCIO_NAMESPACE::MatrixTransform>;
    template <> class TypedKeyNotFound<OCIO_NAMESPACE::TruelightTransform>;
}
#pragma GCC visibility pop

#endif 

#include <yaml-cpp/yaml.h>

#ifndef INCLUDED_OCIO_YAML_H
#define INCLUDED_OCIO_YAML_H

OCIO_NAMESPACE_ENTER
{
    
    // Core
    OCIOHIDDEN void operator >> (const YAML::Node& node, ColorSpaceRcPtr& cs);
    OCIOHIDDEN YAML::Emitter& operator << (YAML::Emitter& out, ColorSpaceRcPtr cs);
    OCIOHIDDEN void operator >> (const YAML::Node& node, GroupTransformRcPtr& t);
    OCIOHIDDEN YAML::Emitter& operator << (YAML::Emitter& out, ConstGroupTransformRcPtr t);
    OCIOHIDDEN void operator >> (const YAML::Node& node, TransformRcPtr& t);
    OCIOHIDDEN YAML::Emitter& operator << (YAML::Emitter& out, ConstTransformRcPtr t);
    OCIOHIDDEN void operator >> (const YAML::Node& node, LookRcPtr& cs);
    OCIOHIDDEN YAML::Emitter& operator << (YAML::Emitter& out, LookRcPtr cs);
    
    // Transforms
    OCIOHIDDEN void operator >> (const YAML::Node& node, AllocationTransformRcPtr& t);
    OCIOHIDDEN YAML::Emitter& operator << (YAML::Emitter& out, ConstAllocationTransformRcPtr t);
    OCIOHIDDEN void operator >> (const YAML::Node& node, CDLTransformRcPtr& t);
    OCIOHIDDEN YAML::Emitter& operator << (YAML::Emitter& out, ConstCDLTransformRcPtr t);
    OCIOHIDDEN void operator >> (const YAML::Node& node, ColorSpaceTransformRcPtr& t);
    OCIOHIDDEN YAML::Emitter& operator << (YAML::Emitter& out, ConstColorSpaceTransformRcPtr t);
    OCIOHIDDEN void operator >> (const YAML::Node& node, ExponentTransformRcPtr& t);
    OCIOHIDDEN YAML::Emitter& operator << (YAML::Emitter& out, ConstExponentTransformRcPtr t);
    OCIOHIDDEN void operator >> (const YAML::Node& node, FileTransformRcPtr& t);
    OCIOHIDDEN YAML::Emitter& operator << (YAML::Emitter& out, ConstFileTransformRcPtr t);
    OCIOHIDDEN void operator >> (const YAML::Node& node, LogTransformRcPtr& t);
    OCIOHIDDEN YAML::Emitter& operator << (YAML::Emitter& out, ConstLogTransformRcPtr t);
    OCIOHIDDEN void operator >> (const YAML::Node& node, LookTransformRcPtr& t);
    OCIOHIDDEN YAML::Emitter& operator << (YAML::Emitter& out, ConstLookTransformRcPtr t);
    OCIOHIDDEN void operator >> (const YAML::Node& node, MatrixTransformRcPtr& t);
    OCIOHIDDEN YAML::Emitter& operator << (YAML::Emitter& out, ConstMatrixTransformRcPtr t);
    OCIOHIDDEN void operator >> (const YAML::Node& node, TruelightTransformRcPtr& t);
    OCIOHIDDEN YAML::Emitter& operator << (YAML::Emitter& out, ConstTruelightTransformRcPtr t);
    
    // Enums
    OCIOHIDDEN YAML::Emitter& operator << (YAML::Emitter& out, BitDepth depth);
    OCIOHIDDEN void operator >> (const YAML::Node& node, BitDepth& depth);
    OCIOHIDDEN YAML::Emitter& operator << (YAML::Emitter& out, Allocation alloc);
    OCIOHIDDEN void operator >> (const YAML::Node& node, Allocation& alloc);
    OCIOHIDDEN YAML::Emitter& operator << (YAML::Emitter& out, ColorSpaceDirection dir);
    OCIOHIDDEN void operator >> (const YAML::Node& node, ColorSpaceDirection& dir);
    OCIOHIDDEN YAML::Emitter& operator << (YAML::Emitter& out, TransformDirection dir);
    OCIOHIDDEN void operator >> (const YAML::Node& node, TransformDirection& dir);
    OCIOHIDDEN YAML::Emitter& operator << (YAML::Emitter& out, Interpolation iterp);
    OCIOHIDDEN void operator >> (const YAML::Node& node, Interpolation& iterp);
    
    void LogUnknownKeyWarning(const std::string & name, const YAML::Node& tag);
}
OCIO_NAMESPACE_EXIT

#endif // INCLUDED_OCIO_YAML_H

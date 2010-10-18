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

// TODO: Add the OCIO_NAMESPACE to yaml-cpp
#include <yaml.h>

#ifndef INCLUDED_OCIO_YAML_H
#define INCLUDED_OCIO_YAML_H

OCIO_NAMESPACE_ENTER
{
    
    // Core
    void operator >> (const YAML::Node& node, ColorSpaceRcPtr& cs);
    YAML::Emitter& operator << (YAML::Emitter& out, ColorSpaceRcPtr cs);
    void operator >> (const YAML::Node& node, GroupTransformRcPtr& t);
    YAML::Emitter& operator << (YAML::Emitter& out, ConstGroupTransformRcPtr t);
    void operator >> (const YAML::Node& node, TransformRcPtr& t);
    YAML::Emitter& operator << (YAML::Emitter& out, ConstTransformRcPtr t);
    
    // Transforms
    void operator >> (const YAML::Node& node, FileTransformRcPtr& t);
    YAML::Emitter& operator << (YAML::Emitter& out, ConstFileTransformRcPtr t);
    void operator >> (const YAML::Node& node, ColorSpaceTransformRcPtr& t);
    YAML::Emitter& operator << (YAML::Emitter& out, ConstColorSpaceTransformRcPtr t);
    void operator >> (const YAML::Node& node, ExponentTransformRcPtr& t);
    YAML::Emitter& operator << (YAML::Emitter& out, ConstExponentTransformRcPtr t);
    void operator >> (const YAML::Node& node, CineonLogToLinTransformRcPtr& t);
    YAML::Emitter& operator << (YAML::Emitter& out, ConstCineonLogToLinTransformRcPtr t);
    void operator >> (const YAML::Node& node, MatrixTransformRcPtr& t);
    YAML::Emitter& operator << (YAML::Emitter& out, ConstMatrixTransformRcPtr t);
    void operator >> (const YAML::Node& node, CDLTransformRcPtr& t);
    YAML::Emitter& operator << (YAML::Emitter& out, ConstCDLTransformRcPtr t);
    
    // Enums
    YAML::Emitter& operator << (YAML::Emitter& out, BitDepth depth);
    void operator >> (const YAML::Node& node, BitDepth& depth);
    YAML::Emitter& operator << (YAML::Emitter& out, GpuAllocation alloc);
    void operator >> (const YAML::Node& node, GpuAllocation& alloc);
    YAML::Emitter& operator << (YAML::Emitter& out, ColorSpaceDirection dir);
    void operator >> (const YAML::Node& node, ColorSpaceDirection& dir);
    YAML::Emitter& operator << (YAML::Emitter& out, TransformDirection dir);
    void operator >> (const YAML::Node& node, TransformDirection& dir);
    YAML::Emitter& operator << (YAML::Emitter& out, Interpolation iterp);
    void operator >> (const YAML::Node& node, Interpolation& iterp);
    
}
OCIO_NAMESPACE_EXIT

#endif // INCLUDED_OCIO_YAML_H

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

#include <cstring>

#include <OpenColorIO/OpenColorIO.h>

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

#include "Logging.h"
#include "MathUtils.h"
#include "pystring/pystring.h"
#include "PathUtils.h"
#include "ParseUtils.h"
#include "Display.h"
#include "OCIOYaml.h"

OCIO_NAMESPACE_ENTER
{
    
    namespace
    {
    
#ifdef OLDYAML
        typedef YAML::Iterator Iterator;
#else
        typedef YAML::const_iterator Iterator;
#endif
        
        // Iterator access
        
        inline const YAML::Node& get_first(const Iterator it)
        {
#ifdef OLDYAML
            return it.first();
#else
            return it->first;
#endif
        }
        
        inline const YAML::Node& get_second(const Iterator it)
        {
#ifdef OLDYAML
            return it.second();
#else
            return it->second;
#endif
        }
        
        // Basic types
        
        inline void load(const YAML::Node& node, bool& x)
        {
#ifdef OLDYAML
            node.Read<bool>(x);
#else
            x = node.as<bool>();
#endif
        }
        
        inline void load(const YAML::Node& node, int& x)
        {
#ifdef OLDYAML
            node.Read<int>(x);
#else
            x = node.as<int>();
#endif
        }
        
        inline void load(const YAML::Node& node, float& x)
        {
#ifdef OLDYAML
            node.Read<float>(x);
#else
            x = node.as<float>();
#endif
        }
        
        inline void load(const YAML::Node& node, std::string& x)
        {
#ifdef OLDYAML
            node.Read<std::string>(x);
#else
            x = node.as<std::string>();
#endif
        }
        
        inline void load(const YAML::Node& node, std::vector<std::string>& x)
        {
#ifdef OLDYAML
            node >> x;
#else
            x = node.as<std::vector<std::string> >();
#endif
        }
        
        inline void load(const YAML::Node& node, std::vector<float>& x)
        {
#ifdef OLDYAML
            node >> x;
#else
            x = node.as<std::vector<float> >();
#endif
        }
        
        // Enums
        
        inline void load(const YAML::Node& node, BitDepth& depth)
        {
            std::string str;
            load(node, str);
            depth = BitDepthFromString(str.c_str());
        }
        
        inline void save(YAML::Emitter& out, BitDepth depth)
        {
            out << BitDepthToString(depth);
        }
        
        inline void load(const YAML::Node& node, Allocation& alloc)
        {
            std::string str;
            load(node, str);
            alloc = AllocationFromString(str.c_str());
        }
        
        inline void save(YAML::Emitter& out, Allocation alloc)
        {
            out << AllocationToString(alloc);
        }
        
        inline void load(const YAML::Node& node, ColorSpaceDirection& dir)
        {
            std::string str;
            load(node, str);
            dir = ColorSpaceDirectionFromString(str.c_str());
        }
        
        inline void save(YAML::Emitter& out, ColorSpaceDirection dir)
        {
            out << ColorSpaceDirectionToString(dir);
        }
        
        inline void load(const YAML::Node& node, TransformDirection& dir)
        {
            std::string str;
            load(node, str);
            dir = TransformDirectionFromString(str.c_str());
        }
        
        inline void save(YAML::Emitter& out, TransformDirection dir)
        {
            out << TransformDirectionToString(dir);
        }
        
        inline void load(const YAML::Node& node, Interpolation& interp)
        {
            std::string str;
            load(node, str);
            interp = InterpolationFromString(str.c_str());
        }
        
        inline void save(YAML::Emitter& out, Interpolation interp)
        {
            out << InterpolationToString(interp);
        }
        
        //
        
        inline void LogUnknownKeyWarning(const std::string & name,
                                         const YAML::Node& tag)
        {
            std::string key;
            load(tag, key);
        
            std::ostringstream os;
            os << "Unknown key in " << name << ": '" << key << "'.";
            LogWarning(os.str());
        }
        
        // View
        
        inline void load(const YAML::Node& node, View& v)
        {
            if(node.Tag() != "View")
                return;
            
            std::string key, stringval;
            
            for (Iterator iter = node.begin();
                 iter != node.end();
                 ++iter)
            {
                const YAML::Node& first = get_first(iter);
                const YAML::Node& second = get_second(iter);
                
                load(first, key);
                
                if (second.Type() == YAML::NodeType::Null) continue;
                
                if(key == "name")
                {
                    load(second, stringval);
                    v.name = stringval;
                }
                else if(key == "colorspace")
                {
                    load(second, stringval);
                    v.colorspace = stringval;
                }
                else if(key == "looks" || key == "look")
                {
                    load(second, stringval);
                    v.looks = stringval;
                }
                else
                {
                    LogUnknownKeyWarning(node.Tag(), first);
                }
            }
            
            if(v.name.empty())
            {
                throw Exception("View does not specify 'name'.");
            }
            if(v.colorspace.empty())
            {
                std::ostringstream os;
                os << "View '" << v.name << "' ";
                os << "does not specify colorspace.";
                throw Exception(os.str().c_str());
            }
        }
        
        inline void save(YAML::Emitter& out, View view)
        {
            out << YAML::VerbatimTag("View");
            out << YAML::Flow;
            out << YAML::BeginMap;
            out << YAML::Key << "name" << YAML::Value << view.name;
            out << YAML::Key << "colorspace" << YAML::Value << view.colorspace;
            if(!view.looks.empty()) out << YAML::Key << "looks" << YAML::Value << view.looks;
            out << YAML::EndMap;
        }
        
        // Common Transform
        
        inline void EmitBaseTransformKeyValues(YAML::Emitter & out,
                                               const ConstTransformRcPtr & t)
        {
            if(t->getDirection() != TRANSFORM_DIR_FORWARD)
            {
                out << YAML::Key << "direction";
                out << YAML::Value << YAML::Flow;
                save(out, t->getDirection());
            }
        }
        
        // AllocationTransform
        
        inline void load(const YAML::Node& node, AllocationTransformRcPtr& t)
        {
            t = AllocationTransform::Create();
            
            std::string key;
            
            for (Iterator iter = node.begin();
                 iter != node.end();
                 ++iter)
            {
                const YAML::Node& first = get_first(iter);
                const YAML::Node& second = get_second(iter);
                
                load(first, key);
                
                if (second.Type() == YAML::NodeType::Null) continue;
                
                if(key == "allocation")
                {
                    Allocation val;
                    load(second, val);
                    t->setAllocation(val);
                }
                else if(key == "vars")
                {
                    std::vector<float> val;
                    load(second, val);
                    if(!val.empty())
                    {
                        t->setVars(static_cast<int>(val.size()), &val[0]);
                    }
                }
                else if(key == "direction")
                {
                    TransformDirection val;
                    load(second, val);
                    t->setDirection(val);
                }
                else
                {
                    LogUnknownKeyWarning(node.Tag(), first);
                }
            }
        }
        
        inline void save(YAML::Emitter& out, ConstAllocationTransformRcPtr t)
        {
            out << YAML::VerbatimTag("AllocationTransform");
            out << YAML::Flow << YAML::BeginMap;
            
            out << YAML::Key << "allocation";
            out << YAML::Value << YAML::Flow;
            save(out, t->getAllocation());
            
            if(t->getNumVars() > 0)
            {
                std::vector<float> vars(t->getNumVars());
                t->getVars(&vars[0]);
                out << YAML::Key << "vars";
                out << YAML::Flow << YAML::Value << vars;
            }
            
            EmitBaseTransformKeyValues(out, t);
            out << YAML::EndMap;
        }
        
        // CDLTransform
        
        inline void load(const YAML::Node& node, CDLTransformRcPtr& t)
        {
            t = CDLTransform::Create();
            
            std::string key;
            std::vector<float> floatvecval;
            
            for (Iterator iter = node.begin();
                 iter != node.end();
                 ++iter)
            {
                const YAML::Node& first = get_first(iter);
                const YAML::Node& second = get_second(iter);
                
                load(first, key);
                
                if (second.Type() == YAML::NodeType::Null) continue;
                
                if(key == "slope")
                {
                    load(second, floatvecval);
                    if(floatvecval.size() != 3)
                    {
                        std::ostringstream os;
                        os << "CDLTransform parse error, 'slope' field must be 3 ";
                        os << "floats. Found '" << floatvecval.size() << "'.";
                        throw Exception(os.str().c_str());
                    }
                    t->setSlope(&floatvecval[0]);
                }
                else if(key == "offset")
                {
                    load(second, floatvecval);
                    if(floatvecval.size() != 3)
                    {
                        std::ostringstream os;
                        os << "CDLTransform parse error, 'offset' field must be 3 ";
                        os << "floats. Found '" << floatvecval.size() << "'.";
                        throw Exception(os.str().c_str());
                    }
                    t->setOffset(&floatvecval[0]);
                }
                else if(key == "power")
                {
                    load(second, floatvecval);
                    if(floatvecval.size() != 3)
                    {
                        std::ostringstream os;
                        os << "CDLTransform parse error, 'power' field must be 3 ";
                        os << "floats. Found '" << floatvecval.size() << "'.";
                        throw Exception(os.str().c_str());
                    }
                    t->setPower(&floatvecval[0]);
                }
                else if(key == "saturation" || key == "sat")
                {
                    float val = 0.0f;
                    load(second, val);
                    t->setSat(val);
                }
                else if(key == "direction")
                {
                    TransformDirection val;
                    load(second, val);
                    t->setDirection(val);
                }
                else
                {
                    LogUnknownKeyWarning(node.Tag(), first);
                }
            }
        }
        
        inline void save(YAML::Emitter& out, ConstCDLTransformRcPtr t)
        {
            out << YAML::VerbatimTag("CDLTransform");
            out << YAML::Flow << YAML::BeginMap;
            
            std::vector<float> slope(3);
            t->getSlope(&slope[0]);
            if(!IsVecEqualToOne(&slope[0], 3))
            {
                out << YAML::Key << "slope";
                out << YAML::Value << YAML::Flow << slope;
            }
            
            std::vector<float> offset(3);
            t->getOffset(&offset[0]);
            if(!IsVecEqualToZero(&offset[0], 3))
            {
                out << YAML::Key << "offset";
                out << YAML::Value << YAML::Flow << offset;
            }
            
            std::vector<float> power(3);
            t->getPower(&power[0]);
            if(!IsVecEqualToOne(&power[0], 3))
            {
                out << YAML::Key << "power";
                out << YAML::Value << YAML::Flow << power;
            }
            
            if(!IsScalarEqualToOne(t->getSat()))
            {
                out << YAML::Key << "sat" << YAML::Value << t->getSat();
            }
            
            EmitBaseTransformKeyValues(out, t);
            out << YAML::EndMap;
        }
        
        // ColorSpaceTransform
        
        inline void load(const YAML::Node& node, ColorSpaceTransformRcPtr& t)
        {
            t = ColorSpaceTransform::Create();
            
            std::string key, stringval;
            
            for (Iterator iter = node.begin();
                 iter != node.end();
                 ++iter)
            {
                const YAML::Node& first = get_first(iter);
                const YAML::Node& second = get_second(iter);
                
                load(first, key);
                
                if (second.Type() == YAML::NodeType::Null) continue;
                
                if(key == "src")
                {
                    load(second, stringval);
                    t->setSrc(stringval.c_str());
                }
                else if(key == "dst")
                {
                    load(second, stringval);
                    t->setDst(stringval.c_str());
                }
                else if(key == "direction")
                {
                    TransformDirection val;
                    load(second, val);
                    t->setDirection(val);
                }
                else
                {
                    LogUnknownKeyWarning(node.Tag(), first);
                }
            }
        }
        
        inline void save(YAML::Emitter& out, ConstColorSpaceTransformRcPtr t)
        {
            out << YAML::VerbatimTag("ColorSpaceTransform");
            out << YAML::Flow << YAML::BeginMap;
            out << YAML::Key << "src" << YAML::Value << t->getSrc();
            out << YAML::Key << "dst" << YAML::Value << t->getDst();
            EmitBaseTransformKeyValues(out, t);
            out << YAML::EndMap;
        }
        
        // ExponentTransform
        
        inline void load(const YAML::Node& node, ExponentTransformRcPtr& t)
        {
            t = ExponentTransform::Create();
            
            std::string key;
            
            for (Iterator iter = node.begin();
                 iter != node.end();
                 ++iter)
            {
                const YAML::Node& first = get_first(iter);
                const YAML::Node& second = get_second(iter);
                
                load(first, key);
                
                if (second.Type() == YAML::NodeType::Null) continue;
                
                if(key == "value")
                {
                    std::vector<float> val;
                    load(second, val);
                    if(val.size() != 4)
                    {
                        std::ostringstream os;
                        os << "ExponentTransform parse error, value field must be 4 ";
                        os << "floats. Found '" << val.size() << "'.";
                        throw Exception(os.str().c_str());
                    }
                    t->setValue(&val[0]);
                }
                else if(key == "direction")
                {
                    TransformDirection val;
                    load(second, val);
                    t->setDirection(val);
                }
                else
                {
                    LogUnknownKeyWarning(node.Tag(), first);
                }
            }
        }
        
        inline void save(YAML::Emitter& out, ConstExponentTransformRcPtr t)
        {
            out << YAML::VerbatimTag("ExponentTransform");
            out << YAML::Flow << YAML::BeginMap;
            
            std::vector<float> value(4, 0.0);
            t->getValue(&value[0]);
            out << YAML::Key << "value";
            out << YAML::Value << YAML::Flow << value;
            EmitBaseTransformKeyValues(out, t);
            out << YAML::EndMap;
        }
        
        // FileTransform
        
        inline void load(const YAML::Node& node, FileTransformRcPtr& t)
        {
            t = FileTransform::Create();
            
            std::string key, stringval;
            
            for (Iterator iter = node.begin();
                 iter != node.end();
                 ++iter)
            {
                const YAML::Node& first = get_first(iter);
                const YAML::Node& second = get_second(iter);
                
                load(first, key);
                
                if (second.Type() == YAML::NodeType::Null) continue;
                
                if(key == "src")
                {
                    load(second, stringval);
                    t->setSrc(stringval.c_str());
                }
                else if(key == "cccid")
                {
                    load(second, stringval);
                    t->setCCCId(stringval.c_str());
                }
                else if(key == "interpolation")
                {
                    Interpolation val;
                    load(second, val);
                    t->setInterpolation(val);
                }
                else if(key == "direction")
                {
                    TransformDirection val;
                    load(second, val);
                    t->setDirection(val);
                }
                else
                {
                    LogUnknownKeyWarning(node.Tag(), first);
                }
            }
        }
        
        inline void save(YAML::Emitter& out, ConstFileTransformRcPtr t)
        {
            out << YAML::VerbatimTag("FileTransform");
            out << YAML::Flow << YAML::BeginMap;
            out << YAML::Key << "src" << YAML::Value << t->getSrc();
            const char * cccid = t->getCCCId();
            if(cccid && *cccid)
            {
                out << YAML::Key << "cccid" << YAML::Value << t->getCCCId();
            }
            out << YAML::Key << "interpolation";
            out << YAML::Value;
            save(out, t->getInterpolation());
            
            EmitBaseTransformKeyValues(out, t);
            out << YAML::EndMap;
        }
        
        // GroupTransform
        
        void load(const YAML::Node& node, TransformRcPtr& t);
        void save(YAML::Emitter& out, ConstTransformRcPtr t);
        
        inline void load(const YAML::Node& node, GroupTransformRcPtr& t)
        {
            t = GroupTransform::Create();
            
            std::string key;
            
            for (Iterator iter = node.begin();
                 iter != node.end();
                 ++iter)
            {
                const YAML::Node& first = get_first(iter);
                const YAML::Node& second = get_second(iter);
                
                load(first, key);
                
                if (second.Type() == YAML::NodeType::Null) continue;
                
                if(key == "children")
                {
                    for(unsigned i = 0; i < second.size(); ++i)
                    {
                        TransformRcPtr childTransform;
                        load(second[i], childTransform);
                        
                        // TODO: consider the forwards-compatibility implication of
                        // throwing an exception.  Should this be a warning, instead?
                        if(!childTransform)
                        {
                            throw Exception("Child transform could not be parsed.");
                        }
                        
                        t->push_back(childTransform);
                    }
                }
                else if(key == "direction")
                {
                    TransformDirection val;
                    load(second, val);
                    t->setDirection(val);
                }
                else
                {
                    LogUnknownKeyWarning(node.Tag(), first);
                }
            }
        }
        
        inline void save(YAML::Emitter& out, ConstGroupTransformRcPtr t)
        {
            out << YAML::VerbatimTag("GroupTransform");
            out << YAML::BeginMap;
            EmitBaseTransformKeyValues(out, t);
            
            out << YAML::Key << "children";
            out << YAML::Value;
            
            out << YAML::BeginSeq;
            for(int i = 0; i < t->size(); ++i)
            {
                save(out, t->getTransform(i));
            }
            out << YAML::EndSeq;
            
            out << YAML::EndMap;
        }
        
        // LogTransform
        
        inline void load(const YAML::Node& node, LogTransformRcPtr& t)
        {
            t = LogTransform::Create();
            
            std::string key;
            
            for (Iterator iter = node.begin();
                 iter != node.end();
                 ++iter)
            {
                const YAML::Node& first = get_first(iter);
                const YAML::Node& second = get_second(iter);
                
                load(first, key);
                
                if (second.Type() == YAML::NodeType::Null) continue;
                
                if(key == "base")
                {
                    float val = 0.0f;
                    load(second, val);
                    t->setBase(val);
                }
                else if(key == "direction")
                {
                    TransformDirection val;
                    load(second, val);
                    t->setDirection(val);
                }
                else
                {
                    LogUnknownKeyWarning(node.Tag(), first);
                }
            }
        }
        
        inline void save(YAML::Emitter& out, ConstLogTransformRcPtr t)
        {
            out << YAML::VerbatimTag("LogTransform");
            out << YAML::Flow << YAML::BeginMap;
            out << YAML::Key << "base" << YAML::Value << t->getBase();
            EmitBaseTransformKeyValues(out, t);
            out << YAML::EndMap;
        }
        
        // LookTransform
        
        inline void load(const YAML::Node& node, LookTransformRcPtr& t)
        {
            t = LookTransform::Create();
            
            std::string key, stringval;
            
            for (Iterator iter = node.begin();
                 iter != node.end();
                 ++iter)
            {
                const YAML::Node& first = get_first(iter);
                const YAML::Node& second = get_second(iter);
                
                load(first, key);
                
                if (second.Type() == YAML::NodeType::Null) continue;
                
                if(key == "src")
                {
                    load(second, stringval);
                    t->setSrc(stringval.c_str());
                }
                else if(key == "dst")
                {
                    load(second, stringval);
                    t->setDst(stringval.c_str());
                }
                else if(key == "looks")
                {
                    load(second, stringval);
                    t->setLooks(stringval.c_str());
                }
                else if(key == "direction")
                {
                    TransformDirection val;
                    load(second, val);
                    t->setDirection(val);
                }
                else
                {
                    LogUnknownKeyWarning(node.Tag(), first);
                }
            }
        }
        
        inline void save(YAML::Emitter& out, ConstLookTransformRcPtr t)
        {
            out << YAML::VerbatimTag("LookTransform");
            out << YAML::Flow << YAML::BeginMap;
            out << YAML::Key << "src" << YAML::Value << t->getSrc();
            out << YAML::Key << "dst" << YAML::Value << t->getDst();
            out << YAML::Key << "looks" << YAML::Value << t->getLooks();
            EmitBaseTransformKeyValues(out, t);
            out << YAML::EndMap;
        }
        
        // MatrixTransform
        
        inline void load(const YAML::Node& node, MatrixTransformRcPtr& t)
        {
            t = MatrixTransform::Create();
            
            std::string key;
            
            for (Iterator iter = node.begin();
                 iter != node.end();
                 ++iter)
            {
                const YAML::Node& first = get_first(iter);
                const YAML::Node& second = get_second(iter);
                
                load(first, key);
                
                if (second.Type() == YAML::NodeType::Null) continue;
                
                if(key == "matrix")
                {
                    std::vector<float> val;
                    load(second, val);
                    if(val.size() != 16)
                    {
                        std::ostringstream os;
                        os << "MatrixTransform parse error, matrix field must be 16 ";
                        os << "floats. Found '" << val.size() << "'.";
                        throw Exception(os.str().c_str());
                    }
                    t->setMatrix(&val[0]);
                }
                else if(key == "offset")
                {
                    std::vector<float> val;
                    load(second, val);
                    if(val.size() != 4)
                    {
                        std::ostringstream os;
                        os << "MatrixTransform parse error, offset field must be 4 ";
                        os << "floats. Found '" << val.size() << "'.";
                        throw Exception(os.str().c_str());
                    }
                    t->setOffset(&val[0]);
                }
                else if(key == "direction")
                {
                    TransformDirection val;
                    load(second, val);
                    t->setDirection(val);
                }
                else
                {
                    LogUnknownKeyWarning(node.Tag(), first);
                }
            }
        }
        
        inline void save(YAML::Emitter& out, ConstMatrixTransformRcPtr t)
        {
            out << YAML::VerbatimTag("MatrixTransform");
            out << YAML::Flow << YAML::BeginMap;
            
            std::vector<float> matrix(16, 0.0);
            t->getMatrix(&matrix[0]);
            if(!IsM44Identity(&matrix[0]))
            {
                out << YAML::Key << "matrix";
                out << YAML::Value << YAML::Flow << matrix;
            }
            
            std::vector<float> offset(4, 0.0);
            t->getOffset(&offset[0]);
            if(!IsVecEqualToZero(&offset[0],4))
            {
                out << YAML::Key << "offset";
                out << YAML::Value << YAML::Flow << offset;
            }
            
            EmitBaseTransformKeyValues(out, t);
            out << YAML::EndMap;
        }
        
        // TruelightTransform
        
        inline void load(const YAML::Node& node, TruelightTransformRcPtr& t)
        {
            t = TruelightTransform::Create();
            
            std::string key, stringval;
            
            for (Iterator iter = node.begin();
                 iter != node.end();
                 ++iter)
            {
                const YAML::Node& first = get_first(iter);
                const YAML::Node& second = get_second(iter);
                
                load(first, key);
                
                if (second.Type() == YAML::NodeType::Null) continue;
                
                if(key == "config_root")
                {
                    load(second, stringval);
                    t->setConfigRoot(stringval.c_str());
                }
                else if(key == "profile")
                {
                    load(second, stringval);
                    t->setProfile(stringval.c_str());
                }
                else if(key == "camera")
                {
                    load(second, stringval);
                    t->setCamera(stringval.c_str());
                }
                else if(key == "input_display")
                {
                    load(second, stringval);
                    t->setInputDisplay(stringval.c_str());
                }
                else if(key == "recorder")
                {
                    load(second, stringval);
                    t->setRecorder(stringval.c_str());
                }
                else if(key == "print")
                {
                    load(second, stringval);
                    t->setPrint(stringval.c_str());
                }
                else if(key == "lamp")
                {
                    load(second, stringval);
                    t->setLamp(stringval.c_str());
                }
                else if(key == "output_camera")
                {
                    load(second, stringval);
                    t->setOutputCamera(stringval.c_str());
                }
                else if(key == "display")
                {
                    load(second, stringval);
                    t->setDisplay(stringval.c_str());
                }
                else if(key == "cube_input")
                {
                    load(second, stringval);
                     t->setCubeInput(stringval.c_str());
                }
                else if(key == "direction")
                {
                    TransformDirection val;
                    load(second, val);
                    t->setDirection(val);
                }
                else
                {
                    LogUnknownKeyWarning(node.Tag(), first);
                }
            }
        }
        
        inline void save(YAML::Emitter& out, ConstTruelightTransformRcPtr t)
        {
            
            out << YAML::VerbatimTag("TruelightTransform");
            out << YAML::Flow << YAML::BeginMap;
            if(strcmp(t->getConfigRoot(), "") != 0)
            {
                out << YAML::Key << "config_root";
                out << YAML::Value << YAML::Flow << t->getConfigRoot();
            }
            if(strcmp(t->getProfile(), "") != 0)
            {
                out << YAML::Key << "profile";
                out << YAML::Value << YAML::Flow << t->getProfile();
            }
            if(strcmp(t->getCamera(), "") != 0)
            {
                out << YAML::Key << "camera";
                out << YAML::Value << YAML::Flow << t->getCamera();
            }
            if(strcmp(t->getInputDisplay(), "") != 0)
            {
                out << YAML::Key << "input_display";
                out << YAML::Value << YAML::Flow << t->getInputDisplay();
            }
            if(strcmp(t->getRecorder(), "") != 0)
            {
                out << YAML::Key << "recorder";
                out << YAML::Value << YAML::Flow << t->getRecorder();
            }
            if(strcmp(t->getPrint(), "") != 0)
            {
                out << YAML::Key << "print";
                out << YAML::Value << YAML::Flow << t->getPrint();
            }
            if(strcmp(t->getLamp(), "") != 0)
            {
                out << YAML::Key << "lamp";
                out << YAML::Value << YAML::Flow << t->getLamp();
            }
            if(strcmp(t->getOutputCamera(), "") != 0)
            {
                out << YAML::Key << "output_camera";
                out << YAML::Value << YAML::Flow << t->getOutputCamera();
            }
            if(strcmp(t->getDisplay(), "") != 0)
            {
                out << YAML::Key << "display";
                out << YAML::Value << YAML::Flow << t->getDisplay();
            }
            if(strcmp(t->getCubeInput(), "") != 0)
            {
                out << YAML::Key << "cube_input";
                out << YAML::Value << YAML::Flow << t->getCubeInput();
            }
            
            EmitBaseTransformKeyValues(out, t);
            
            out << YAML::EndMap;
        }
        
        // Transform
        
        void load(const YAML::Node& node, TransformRcPtr& t)
        {
            if(node.Type() != YAML::NodeType::Map)
            {
                std::ostringstream os;
                os << "Unsupported Transform type encountered: (" << node.Type() << ") in OCIO profile. ";
                os << "Only Mapping types supported.";
                throw Exception(os.str().c_str());
            }
            
            std::string type = node.Tag();
            
            if(type == "AllocationTransform") {
                AllocationTransformRcPtr temp;
                load(node, temp);
                t = temp;
            }
            else if(type == "CDLTransform") {
                CDLTransformRcPtr temp;
                load(node, temp);
                t = temp;
            }
            else if(type == "ColorSpaceTransform")  {
                ColorSpaceTransformRcPtr temp;
                load(node, temp);
                t = temp;
            }
            // TODO: DisplayTransform
            else if(type == "ExponentTransform")  {
                ExponentTransformRcPtr temp;
                load(node, temp);
                t = temp;
            }
            else if(type == "FileTransform")  {
                FileTransformRcPtr temp;
                load(node, temp);
                t = temp;
            }
            else if(type == "GroupTransform") {
                GroupTransformRcPtr temp;
                load(node, temp);
                t = temp;
            }
            else if(type == "LogTransform") {
                LogTransformRcPtr temp;
                load(node, temp);
                t = temp;
            }
            else if(type == "LookTransform") {
                LookTransformRcPtr temp;
                load(node, temp);
                t = temp;
            }
            else if(type == "MatrixTransform")  {
                MatrixTransformRcPtr temp;
                load(node, temp);
                t = temp;
            }
            else if(type == "TruelightTransform")  {
                TruelightTransformRcPtr temp;
                load(node, temp);
                t = temp;
            }
            else
            {
                // TODO: add a new empty (better name?) aka passthru Transform()
                // which does nothing. This is so upsupported !<tag> types don't
                // throw an exception. Alternativly this could be caught in the
                // GroupTransformRcPtr >> operator with some type of
                // supported_tag() method
                
                // TODO: consider the forwards-compatibility implication of
                // throwing an exception.  Should this be a warning, instead?
                
                //  t = EmptyTransformRcPtr(new EmptyTransform(), &deleter);
                std::ostringstream os;
                os << "Unsupported transform type !<" << type << "> in OCIO profile. ";
                throw Exception(os.str().c_str());
            }
        }
        
        void save(YAML::Emitter& out, ConstTransformRcPtr t)
        {
            if(ConstAllocationTransformRcPtr Allocation_tran = \
                DynamicPtrCast<const AllocationTransform>(t))
                save(out, Allocation_tran);
            else if(ConstCDLTransformRcPtr CDL_tran = \
                DynamicPtrCast<const CDLTransform>(t))
                save(out, CDL_tran);
            else if(ConstColorSpaceTransformRcPtr ColorSpace_tran = \
                DynamicPtrCast<const ColorSpaceTransform>(t))
                save(out, ColorSpace_tran);
            else if(ConstExponentTransformRcPtr Exponent_tran = \
                DynamicPtrCast<const ExponentTransform>(t))
                save(out, Exponent_tran);
            else if(ConstFileTransformRcPtr File_tran = \
                DynamicPtrCast<const FileTransform>(t))
                save(out, File_tran);
            else if(ConstGroupTransformRcPtr Group_tran = \
                DynamicPtrCast<const GroupTransform>(t))
                save(out, Group_tran);
            else if(ConstLogTransformRcPtr Log_tran = \
                DynamicPtrCast<const LogTransform>(t))
                save(out, Log_tran);
            else if(ConstLookTransformRcPtr Look_tran = \
                DynamicPtrCast<const LookTransform>(t))
                save(out, Look_tran);
            else if(ConstMatrixTransformRcPtr Matrix_tran = \
                DynamicPtrCast<const MatrixTransform>(t))
                save(out, Matrix_tran);
            else if(ConstTruelightTransformRcPtr Truelight_tran = \
                DynamicPtrCast<const TruelightTransform>(t))
                save(out, Truelight_tran);
            else
                throw Exception("Unsupported Transform() type for serialization.");
        }
        
        // ColorSpace
        
        inline void load(const YAML::Node& node, ColorSpaceRcPtr& cs)
        {
            if(node.Tag() != "ColorSpace")
                return; // not a !<ColorSpace> tag
            
            std::string key, stringval;
            bool boolval;
            
            for (Iterator iter = node.begin();
                 iter != node.end();
                 ++iter)
            {
                const YAML::Node& first = get_first(iter);
                const YAML::Node& second = get_second(iter);
                
                load(first, key);
                
                if (second.Type() == YAML::NodeType::Null) continue;
                
                if(key == "name")
                {
                    load(second, stringval);
                    cs->setName(stringval.c_str());
                }
                else if(key == "description")
                {
                    load(second, stringval);
                    cs->setDescription(stringval.c_str());
                }
                else if(key == "family")
                {
                    load(second, stringval);
                    cs->setFamily(stringval.c_str());
                }
                else if(key == "equalitygroup")
                {
                    load(second, stringval);
                    cs->setEqualityGroup(stringval.c_str());
                }
                else if(key == "bitdepth")
                {
                    BitDepth ret;
                    load(second, ret);
                    cs->setBitDepth(ret);
                }
                else if(key == "isdata")
                {
                    load(second, boolval);
                    cs->setIsData(boolval);
                }
                else if(key == "allocation")
                {
                    Allocation val;
                    load(second, val);
                    cs->setAllocation(val);
                }
                else if(key == "allocationvars")
                {
                    std::vector<float> val;
                    load(second, val);
                    if(!val.empty())
                        cs->setAllocationVars(static_cast<int>(val.size()), &val[0]);
                }
                else if(key == "to_reference")
                {
                    TransformRcPtr val;
                    load(second, val);
                    cs->setTransform(val, COLORSPACE_DIR_TO_REFERENCE);
                }
                else if(key == "from_reference")
                {
                    TransformRcPtr val;
                    load(second, val);
                    cs->setTransform(val, COLORSPACE_DIR_FROM_REFERENCE);
                }
                else
                {
                    LogUnknownKeyWarning(node.Tag(), first);
                }
            }
        }
        
        inline void save(YAML::Emitter& out, ConstColorSpaceRcPtr cs)
        {
            out << YAML::VerbatimTag("ColorSpace");
            out << YAML::BeginMap;
            
            out << YAML::Key << "name" << YAML::Value << cs->getName();
            out << YAML::Key << "family" << YAML::Value << cs->getFamily();
            out << YAML::Key << "equalitygroup" << YAML::Value << cs->getEqualityGroup();
            out << YAML::Key << "bitdepth" << YAML::Value;
            save(out, cs->getBitDepth());
            if(cs->getDescription() != NULL && strlen(cs->getDescription()) > 0)
            {
                out << YAML::Key << "description";
                out << YAML::Value << YAML::Literal << cs->getDescription();
            }
            out << YAML::Key << "isdata" << YAML::Value << cs->isData();
            
            out << YAML::Key << "allocation" << YAML::Value;
            save(out, cs->getAllocation());
            if(cs->getAllocationNumVars() > 0)
            {
                std::vector<float> allocationvars(cs->getAllocationNumVars());
                cs->getAllocationVars(&allocationvars[0]);
                out << YAML::Key << "allocationvars";
                out << YAML::Flow << YAML::Value << allocationvars;
            }
            
            ConstTransformRcPtr toref = \
                cs->getTransform(COLORSPACE_DIR_TO_REFERENCE);
            if(toref)
            {
                out << YAML::Key << "to_reference" << YAML::Value;
                save(out, toref);
            }
            
            ConstTransformRcPtr fromref = \
                cs->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
            if(fromref)
            {
                out << YAML::Key << "from_reference" << YAML::Value;
                save(out, fromref);
            }
            
            out << YAML::EndMap;
            out << YAML::Newline;
        }
        
        // Look
        
        inline void load(const YAML::Node& node, LookRcPtr& look)
        {
            if(node.Tag() != "Look")
                return;
            
            std::string key, stringval;
            
            for (Iterator iter = node.begin();
                 iter != node.end();
                 ++iter)
            {
                const YAML::Node& first = get_first(iter);
                const YAML::Node& second = get_second(iter);
                
                load(first, key);
                
                if (second.Type() == YAML::NodeType::Null) continue;
                
                if(key == "name")
                {
                    load(second, stringval);
                    look->setName(stringval.c_str());
                }
                else if(key == "process_space")
                {
                    load(second, stringval);
                    look->setProcessSpace(stringval.c_str());
                }
                else if(key == "transform")
                {
                    TransformRcPtr val;
                    load(second, val);
                    look->setTransform(val);
                }
                else if(key == "inverse_transform")
                {
                    TransformRcPtr val;
                    load(second, val);
                    look->setInverseTransform(val);
                }
                else
                {
                    LogUnknownKeyWarning(node.Tag(), first);
                }
            }
        }
        
        inline void save(YAML::Emitter& out, ConstLookRcPtr look)
        {
            out << YAML::VerbatimTag("Look");
            out << YAML::BeginMap;
            out << YAML::Key << "name" << YAML::Value << look->getName();
            out << YAML::Key << "process_space" << YAML::Value << look->getProcessSpace();
            
            if(look->getTransform())
            {
                out << YAML::Key << "transform";
                out << YAML::Value;
                save(out, look->getTransform());
            }
            
            if(look->getInverseTransform())
            {
                out << YAML::Key << "inverse_transform";
                out << YAML::Value;
                save(out, look->getInverseTransform());
            }
            
            out << YAML::EndMap;
            out << YAML::Newline;
        }
        
        // Config
        
        inline void load(const YAML::Node& node, ConfigRcPtr& c, const char* filename)
        {
            
            // check profile version
            int profile_version = 0;
#ifdef OLDYAML
            if(node.FindValue("ocio_profile_version") == NULL)
#else
            if(node["ocio_profile_version"] == NULL)
#endif
            {
                std::ostringstream os;
                os << "The specified file ";
                os << "does not appear to be an OCIO configuration.";
                throw Exception (os.str().c_str());
            }
            
            load(node["ocio_profile_version"], profile_version);
            if(profile_version > 1)
            {
                std::ostringstream os;
                os << "This .ocio config ";
                if(filename && *filename)
                {
                    os << " '" << filename << "' ";
                }
                os << "is version " << profile_version << ". ";
                os << "This version of the OpenColorIO library (" << OCIO_VERSION ") ";
                os << "is not known to be able to load this profile. ";
                os << "An attempt will be made, but there are no guarantees that the ";
                os << "results will be accurate. Continue at your own risk.";
                LogWarning(os.str());
            }
            
            std::string key, stringval;
            bool boolval = false;
            EnvironmentMode mode = ENV_ENVIRONMENT_LOAD_ALL;
            
            for (Iterator iter = node.begin();
                 iter != node.end();
                 ++iter)
            {
                const YAML::Node& first = get_first(iter);
                const YAML::Node& second = get_second(iter);
                
                load(first, key);
                
                if (second.Type() == YAML::NodeType::Null) continue;
                
                if(key == "ocio_profile_version") { } // Already handled above.
                else if(key == "environment")
                {
                    mode = ENV_ENVIRONMENT_LOAD_PREDEFINED;
                    if(second.Type() != YAML::NodeType::Map)
                    {
                        std::ostringstream os;
                        os << "'environment' field needs to be a (name: key) map.";
                        throw Exception(os.str().c_str());
                    }
                    for (Iterator it = second.begin();
                         it != second.end();
                         ++it)
                    {
                        std::string k, v;
                        load(get_first(it), k);
                        load(get_second(it), v);
                        c->addEnvironmentVar(k.c_str(), v.c_str());
                    }
                }
                else if(key == "search_path" || key == "resource_path")
                {
                    load(second, stringval);
                    c->setSearchPath(stringval.c_str());
                }
                else if(key == "strictparsing")
                {
                    load(second, boolval);
                    c->setStrictParsingEnabled(boolval);
                }
                else if(key == "description")
                {
                    load(second, stringval);
                    c->setDescription(stringval.c_str());
                }
                else if(key == "luma")
                {
                    std::vector<float> val;
                    load(second, val);
                    if(val.size() != 3)
                    {
                        std::ostringstream os;
                        os << "'luma' field must be 3 ";
                        os << "floats. Found '" << val.size() << "'.";
                        throw Exception(os.str().c_str());
                    }
                    c->setDefaultLumaCoefs(&val[0]);
                }
                else if(key == "roles")
                {
                    if(second.Type() != YAML::NodeType::Map)
                    {
                        std::ostringstream os;
                        os << "'roles' field needs to be a (name: key) map.";
                        throw Exception(os.str().c_str());
                    }
                    for (Iterator it = second.begin();
                         it != second.end();
                         ++it)
                    {
                        std::string k, v;
                        load(get_first(it), k);
                        load(get_second(it), v);
                        c->setRole(k.c_str(), v.c_str());
                    }
                }
                else if(key == "displays")
                {
                    if(second.Type() != YAML::NodeType::Map)
                    {
                        std::ostringstream os;
                        os << "'displays' field needs to be a (name: key) map.";
                        throw Exception(os.str().c_str());
                    }
                    for (Iterator it = second.begin();
                         it != second.end();
                         ++it)
                    {
                        std::string display;
                        load(get_first(it), display);
                        const YAML::Node& dsecond = get_second(it);
                        for(unsigned i = 0; i < dsecond.size(); ++i)
                        {
                            View view;
                            load(dsecond[i], view);
                            c->addDisplay(display.c_str(), view.name.c_str(),
                                          view.colorspace.c_str(), view.looks.c_str());
                        }
                    }
                }
                else if(key == "active_displays")
                {
                    std::vector<std::string> display;
                    load(second, display);
                    const char* displays = JoinStringEnvStyle(display).c_str();
                    c->setActiveDisplays(displays);
                }
                else if(key == "active_views")
                {
                    std::vector<std::string> view;
                    load(second, view);
                    const char* views = JoinStringEnvStyle(view).c_str();
                    c->setActiveViews(views);
                }
                else if(key == "colorspaces")
                {
                    if(second.Type() != YAML::NodeType::Sequence)
                    {
                        std::ostringstream os;
                        os << "'colorspaces' field needs to be a (- !<ColorSpace>) list.";
                        throw Exception(os.str().c_str());
                    }
                    for(unsigned i = 0; i < second.size(); ++i)
                    {
                        if(second[i].Tag() == "ColorSpace")
                        {
                            ColorSpaceRcPtr cs = ColorSpace::Create();
                            load(second[i], cs);
                            for(int ii = 0; ii < c->getNumColorSpaces(); ++ii)
                            {
                                if(strcmp(c->getColorSpaceNameByIndex(ii), cs->getName()) == 0)
                                {
                                    std::ostringstream os;
                                    os << "Colorspace with name '" << cs->getName() << "' already defined.";
                                    throw Exception(os.str().c_str());
                                }
                            }
                            c->addColorSpace(cs);
                        }
                        else
                        {
                            std::ostringstream os;
                            os << "Unknown element found in colorspaces:";
                            os << second[i].Tag() << ". Only ColorSpace(s)";
                            os << " currently handled.";
                            LogWarning(os.str());
                        }
                    }
                }
                else if(key == "looks")
                {
                    if(second.Type() != YAML::NodeType::Sequence)
                    {
                        std::ostringstream os;
                        os << "'looks' field needs to be a (- !<Look>) list.";
                        throw Exception(os.str().c_str());
                    }
                    
                    for(unsigned i = 0; i < second.size(); ++i)
                    {
                        if(second[i].Tag() == "Look")
                        {
                            LookRcPtr look = Look::Create();
                            load(second[i], look);
                            c->addLook(look);
                        }
                        else
                        {
                            std::ostringstream os;
                            os << "Unknown element found in looks:";
                            os << second[i].Tag() << ". Only Look(s)";
                            os << " currently handled.";
                            LogWarning(os.str());
                        }
                    }
                }
                else
                {
                    LogUnknownKeyWarning("profile", first);
                }
            }
            
            if(filename)
            {
                std::string realfilename = pystring::os::path::abspath(filename);
                std::string configrootdir = pystring::os::path::dirname(realfilename);
                c->setWorkingDir(configrootdir.c_str());
            }
            
            c->setEnvironmentMode(mode);
            c->loadEnvironment();
            
            if(mode == ENV_ENVIRONMENT_LOAD_ALL)
            {
                std::ostringstream os;
                os << "This .ocio config ";
                if(filename && *filename)
                {
                    os << " '" << filename << "' ";
                }
                os << "has no environment section defined. The default behaviour is to ";
                os << "load all environment variables (" << c->getNumEnvironmentVars() << ")";
                os << ", which reduces the efficiency of OCIO's caching. Considering ";
                os << "predefining the environment variables used.";
                LogDebug(os.str());
            }
            
        }
        
        inline void save(YAML::Emitter& out, const Config* c)
        {
            out << YAML::Block;
            out << YAML::BeginMap;
            out << YAML::Key << "ocio_profile_version" << YAML::Value << 1;
            out << YAML::Newline;
#ifndef OLDYAML
            out << YAML::Newline;
#endif
            
            if(c->getNumEnvironmentVars() > 0)
            {
                out << YAML::Key << "environment";
                out << YAML::Value << YAML::BeginMap;
                for(unsigned i = 0; i < c->getNumEnvironmentVars(); ++i)
                {   
                    const char* name = c->getEnvironmentVarNameByIndex(i);
                    out << YAML::Key << name;
                    out << YAML::Value << c->getEnvironmentVarDefault(name);
                }
                out << YAML::EndMap;
                out << YAML::Newline;
            }
            out << YAML::Key << "search_path" << YAML::Value << c->getSearchPath();
            out << YAML::Key << "strictparsing" << YAML::Value << c->isStrictParsingEnabled();
            
            std::vector<float> luma(3, 0.f);
            c->getDefaultLumaCoefs(&luma[0]);
            out << YAML::Key << "luma" << YAML::Value << YAML::Flow << luma;
            
            if(c->getDescription() != NULL && strlen(c->getDescription()) > 0)
            {
                out << YAML::Newline;
                out << YAML::Key << "description";
                out << YAML::Value << c->getDescription();
                out << YAML::Newline;
            }
            
            // Roles
            out << YAML::Newline;
#ifndef OLDYAML
            out << YAML::Newline;
#endif
            out << YAML::Key << "roles";
            out << YAML::Value << YAML::BeginMap;
            for(unsigned i = 0; i < c->getNumRoles(); ++i)
            {
                const char* role = c->getRoleName(i);
                out << YAML::Key << role;
                out << YAML::Value << c->getColorSpace(role)->getName();
            }
            out << YAML::EndMap;
#ifndef OLDYAML
            out << YAML::Newline;
#endif
            
            // Displays
            out << YAML::Newline;
            out << YAML::Key << "displays";
            out << YAML::Value << YAML::BeginMap;
            for(unsigned i = 0; i < c->getNumDisplaysAll(); ++i)
            {
                const char* display = c->getDisplayAll(i);
                out << YAML::Key << display;
                out << YAML::Value << YAML::BeginSeq;
                for(unsigned v = 0; v < c->getNumViews(display); ++v)
                {
                    View dview;
                    dview.name = c->getView(display, v);
                    dview.colorspace = c->getDisplayColorSpaceName(display, dview.name.c_str());
                    if(c->getDisplayLooks(display, dview.name.c_str()) != NULL)
                        dview.looks = c->getDisplayLooks(display, dview.name.c_str());
                    save(out, dview);
                
                }
                out << YAML::EndSeq;
            }
            out << YAML::EndMap;
            
#ifndef OLDYAML
            out << YAML::Newline;
#endif
            out << YAML::Newline;
            out << YAML::Key << "active_displays";
            std::vector<std::string> active_displays;
            if(c->getActiveDisplays() != NULL && strlen(c->getActiveDisplays()) > 0)
                SplitStringEnvStyle(active_displays, c->getActiveDisplays());
            out << YAML::Value << YAML::Flow << active_displays;
            out << YAML::Key << "active_views";
            std::vector<std::string> active_views;
            if(c->getActiveViews() != NULL && strlen(c->getActiveViews()) > 0)
                SplitStringEnvStyle(active_views, c->getActiveViews());
            out << YAML::Value << YAML::Flow << active_views;
#ifndef OLDYAML
            out << YAML::Newline;
#endif
            
            // Looks
            if(c->getNumLooks() > 0)
            {
                out << YAML::Newline;
                out << YAML::Key << "looks";
                out << YAML::Value << YAML::BeginSeq;
                for(unsigned i = 0; i < c->getNumLooks(); ++i)
                {
                    const char* name = c->getLookNameByIndex(i);
                    save(out, c->getLook(name));
                }
                out << YAML::EndSeq;
                out << YAML::Newline;
            }
            
            // ColorSpaces
            {
                out << YAML::Newline;
                out << YAML::Key << "colorspaces";
                out << YAML::Value << YAML::BeginSeq;
                for(unsigned i = 0; i < c->getNumColorSpaces(); ++i)
                {
                    const char* name = c->getColorSpaceNameByIndex(i);
                    save(out, c->getColorSpace(name));
                }
                out << YAML::EndSeq;
            }
            
            out << YAML::EndMap;
        }
        
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    void OCIOYaml::open(std::istream& istream, ConfigRcPtr& c, const char* filename) const
    {
        try
        {
#ifdef OLDYAML
            YAML::Parser parser(istream);
            YAML::Node node;
            parser.GetNextDocument(node);
#else
            YAML::Node node = YAML::Load(istream);
#endif
            load(node, c, filename);
        }
        catch(const std::exception & e)
        {
            std::ostringstream os;
            os << "Error: Loading the OCIO profile ";
            if(filename) os << "'" << filename << "' ";
            os << "failed. " << e.what();
            throw Exception(os.str().c_str());
        }
    }
    
    void OCIOYaml::write(std::ostream& ostream, const Config* c) const
    {
        YAML::Emitter out;
        save(out, c);
        ostream << out.c_str();
    }
    
}
OCIO_NAMESPACE_EXIT

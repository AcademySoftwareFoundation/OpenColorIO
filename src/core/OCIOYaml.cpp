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

#include "Logging.h"
#include "MathUtils.h"
#include "OCIOYaml.h"

OCIO_NAMESPACE_ENTER
{
    ///////////////////////////////////////////////////////////////////////////
    //  Core
    
    void LogUnknownKeyWarning(const std::string & name, const YAML::Node& tag)
    {
        std::string key;
        tag >> key;
        
        std::ostringstream os;
        os << "Unknown key in " << name << ": ";
        os << "'" << key << "'. (line ";
        os << (tag.GetMark().line+1) << ", column "; // (yaml line numbers start at 0)
        os << tag.GetMark().column << ")";
        LogWarning(os.str());
    }
    
    void operator >> (const YAML::Node& node, ColorSpaceRcPtr& cs)
    {
        if(node.Tag() != "ColorSpace")
            return; // not a !<ColorSpace> tag
        
        std::string key, stringval;
        bool boolval;
        
        for (YAML::Iterator iter = node.begin();
             iter != node.end();
             ++iter)
        {
            iter.first() >> key;
            
            if(key == "name")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<std::string>(stringval))
                    cs->setName(stringval.c_str());
            }
            else if(key == "description")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<std::string>(stringval))
                    cs->setDescription(stringval.c_str());
            }
            else if(key == "family")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<std::string>(stringval))
                    cs->setFamily(stringval.c_str());
            }
            else if(key == "equalitygroup")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<std::string>(stringval))
                    cs->setEqualityGroup(stringval.c_str());
            }
            else if(key == "bitdepth")
            {
                BitDepth ret;
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<BitDepth>(ret))
                    cs->setBitDepth(ret);
            }
            else if(key == "isdata")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<bool>(boolval))
                    cs->setIsData(boolval);
            }
            else if(key == "allocation")
            {
                Allocation val;
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<Allocation>(val))
                    cs->setAllocation(val);
            }
            else if(key == "allocationvars")
            {
                std::vector<float> val;
                if (iter.second().Type() != YAML::NodeType::Null)
                {
                    iter.second() >> val;
                    if(!val.empty())
                    {
                        cs->setAllocationVars(static_cast<int>(val.size()), &val[0]);
                    }
                }
            }
            else if(key == "to_reference")
            {
                TransformRcPtr val;
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<TransformRcPtr>(val))
                  cs->setTransform(val, COLORSPACE_DIR_TO_REFERENCE);
            }
            else if(key == "from_reference")
            {
                TransformRcPtr val;
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<TransformRcPtr>(val))
                  cs->setTransform(val, COLORSPACE_DIR_FROM_REFERENCE);
            }
            else
            {
                LogUnknownKeyWarning(node.Tag(), iter.first());
            }
        }
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, ColorSpaceRcPtr cs)
    {
        out << YAML::VerbatimTag("ColorSpace");
        out << YAML::BeginMap;
        
        out << YAML::Key << "name" << YAML::Value << cs->getName();
        out << YAML::Key << "family" << YAML::Value << cs->getFamily();
        out << YAML::Key << "equalitygroup" << YAML::Value << cs->getEqualityGroup();
        out << YAML::Key << "bitdepth" << YAML::Value << cs->getBitDepth();
        if(strlen(cs->getDescription()) > 0)
        {
            out << YAML::Key << "description";
            out << YAML::Value << YAML::Literal << cs->getDescription();
        }
        out << YAML::Key << "isdata" << YAML::Value << cs->isData();
        
        out << YAML::Key << "allocation" << YAML::Value << cs->getAllocation();
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
            out << YAML::Key << "to_reference" << YAML::Value << toref;
        
        ConstTransformRcPtr fromref = \
            cs->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
        if(fromref)
            out << YAML::Key << "from_reference" << YAML::Value << fromref;
        
        out << YAML::EndMap;
        out << YAML::Newline;
        
        return out;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    // Look. (not the transform, the top-level class)
    
    void operator >> (const YAML::Node& node, LookRcPtr& look)
    {
        if(node.Tag() != "Look")
            return;
        
        std::string key, stringval;
        
        for (YAML::Iterator iter = node.begin();
             iter != node.end();
             ++iter)
        {
            iter.first() >> key;
            
            if(key == "name")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<std::string>(stringval))
                    look->setName(stringval.c_str());
            }
            else if(key == "process_space")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<std::string>(stringval))
                    look->setProcessSpace(stringval.c_str());
            }
            else if(key == "transform")
            {
                TransformRcPtr val;
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<TransformRcPtr>(val))
                    look->setTransform(val);
            }
            else if(key == "inverse_transform")
            {
                TransformRcPtr val;
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<TransformRcPtr>(val))
                    look->setInverseTransform(val);
            }
            else
            {
                LogUnknownKeyWarning(node.Tag(), iter.first());
            }
        }
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, LookRcPtr look)
    {
        out << YAML::VerbatimTag("Look");
        out << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value << look->getName();
        out << YAML::Key << "process_space" << YAML::Value << look->getProcessSpace();
        
        if(look->getTransform())
        {
            out << YAML::Key << "transform";
            out << YAML::Value << look->getTransform();
        }
        
        if(look->getInverseTransform())
        {
            out << YAML::Key << "inverse_transform";
            out << YAML::Value << look->getInverseTransform();
        }
        
        out << YAML::EndMap;
        out << YAML::Newline;
        
        return out;
    }
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    namespace
    {
        void EmitBaseTransformKeyValues(YAML::Emitter & out,
                                        const ConstTransformRcPtr & t)
        {
            if(t->getDirection() != TRANSFORM_DIR_FORWARD)
            {
                out << YAML::Key << "direction";
                out << YAML::Value << YAML::Flow << t->getDirection();
            }
        }
    }
    
    void operator >> (const YAML::Node& node, TransformRcPtr& t)
    {
        if(node.Type() != YAML::NodeType::Map)
        {
            std::ostringstream os;
            os << "Unsupported Transform type encountered: (" << node.Type() << ") in OCIO profile. ";
            os << "Only Mapping types supported. (line ";
            os << (node.GetMark().line+1) << ", column "; // (yaml line numbers start at 0)
            os << node.GetMark().column << ")";
            throw Exception(os.str().c_str());
        }
        
        std::string type = node.Tag();
        
        if(type == "AllocationTransform") {
            AllocationTransformRcPtr temp;
            node.Read<AllocationTransformRcPtr>(temp);
            t = temp;
        }
        else if(type == "CDLTransform") {
            CDLTransformRcPtr temp;
            node.Read<CDLTransformRcPtr>(temp);
            t = temp;
        }
        else if(type == "ColorSpaceTransform")  {
            ColorSpaceTransformRcPtr temp;
            node.Read<ColorSpaceTransformRcPtr>(temp);
            t = temp;
        }
        // TODO: DisplayTransform
        else if(type == "ExponentTransform")  {
            ExponentTransformRcPtr temp;
            node.Read<ExponentTransformRcPtr>(temp);
            t = temp;
        }
        else if(type == "FileTransform")  {
            FileTransformRcPtr temp;
            node.Read<FileTransformRcPtr>(temp);
            t = temp;
        }
        else if(type == "GroupTransform") {
            GroupTransformRcPtr temp;
            node.Read<GroupTransformRcPtr>(temp);
            t = temp;
        }
        else if(type == "LogTransform") {
            LogTransformRcPtr temp;
            node.Read<LogTransformRcPtr>(temp);
            t = temp;
        }
        else if(type == "LookTransform") {
            LookTransformRcPtr temp;
            node.Read<LookTransformRcPtr>(temp);
            t = temp;
        }
        else if(type == "MatrixTransform")  {
            MatrixTransformRcPtr temp;
            node.Read<MatrixTransformRcPtr>(temp);
            t = temp;
        }
        else if(type == "TruelightTransform")  {
            TruelightTransformRcPtr temp;
            node.Read<TruelightTransformRcPtr>(temp);
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
            os << " (line ";
            os << (node.GetMark().line+1) << ", column "; // (yaml line numbers start at 0)
            os << node.GetMark().column << ")";
            throw Exception(os.str().c_str());
        }
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, ConstTransformRcPtr t)
    {
        if(ConstAllocationTransformRcPtr Allocation_tran = \
            DynamicPtrCast<const AllocationTransform>(t))
            out << Allocation_tran;
        else if(ConstCDLTransformRcPtr CDL_tran = \
            DynamicPtrCast<const CDLTransform>(t))
            out << CDL_tran;
        else if(ConstColorSpaceTransformRcPtr ColorSpace_tran = \
            DynamicPtrCast<const ColorSpaceTransform>(t))
            out << ColorSpace_tran;
        else if(ConstExponentTransformRcPtr Exponent_tran = \
            DynamicPtrCast<const ExponentTransform>(t))
            out << Exponent_tran;
        else if(ConstFileTransformRcPtr File_tran = \
            DynamicPtrCast<const FileTransform>(t))
            out << File_tran;
        else if(ConstGroupTransformRcPtr Group_tran = \
            DynamicPtrCast<const GroupTransform>(t))
            out << Group_tran;
        else if(ConstLogTransformRcPtr Log_tran = \
            DynamicPtrCast<const LogTransform>(t))
            out << Log_tran;
        else if(ConstLookTransformRcPtr Look_tran = \
            DynamicPtrCast<const LookTransform>(t))
            out << Look_tran;
        else if(ConstMatrixTransformRcPtr Matrix_tran = \
            DynamicPtrCast<const MatrixTransform>(t))
            out << Matrix_tran;
        else if(ConstTruelightTransformRcPtr Truelight_tran = \
            DynamicPtrCast<const TruelightTransform>(t))
            out << Truelight_tran;
        else
            throw Exception("Unsupported Transform() type for serialization.");
        
        return out;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    //  Transforms
    
    void operator >> (const YAML::Node& node, GroupTransformRcPtr& t)
    {
        t = GroupTransform::Create();
        
        std::string key;
        
        for (YAML::Iterator iter = node.begin();
             iter != node.end();
             ++iter)
        {
            iter.first() >> key;
            
            if(key == "children")
            {
                const YAML::Node & children = iter.second();
                for(unsigned i = 0; i <children.size(); ++i)
                {
                    TransformRcPtr childTransform;
                    children[i].Read<TransformRcPtr>(childTransform);
                    
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
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<TransformDirection>(val))
                  t->setDirection(val);
            }
            else
            {
                LogUnknownKeyWarning(node.Tag(), iter.first());
            }
        }
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, ConstGroupTransformRcPtr t)
    {
        out << YAML::VerbatimTag("GroupTransform");
        out << YAML::BeginMap;
        EmitBaseTransformKeyValues(out, t);
        
        out << YAML::Key << "children";
        out << YAML::Value;
        
        out << YAML::BeginSeq;
        for(int i = 0; i < t->size(); ++i)
        {
            out << t->getTransform(i);
        }
        out << YAML::EndSeq;
        
        out << YAML::EndMap;
        
        return out;
    }
    
    
    
    void operator >> (const YAML::Node& node, FileTransformRcPtr& t)
    {
        t = FileTransform::Create();
        
        std::string key, stringval;
        
        for (YAML::Iterator iter = node.begin();
             iter != node.end();
             ++iter)
        {
            iter.first() >> key;
            
            if(key == "src")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<std::string>(stringval))
                  t->setSrc(stringval.c_str());
            }
            else if(key == "cccid")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<std::string>(stringval))
                  t->setCCCId(stringval.c_str());
            }
            else if(key == "interpolation")
            {
                Interpolation val;
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<Interpolation>(val))
                  t->setInterpolation(val);
            }
            else if(key == "direction")
            {
                TransformDirection val;
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<TransformDirection>(val))
                  t->setDirection(val);
            }
            else
            {
                LogUnknownKeyWarning(node.Tag(), iter.first());
            }
        }
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, ConstFileTransformRcPtr t)
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
        out << YAML::Value << t->getInterpolation();
        
        EmitBaseTransformKeyValues(out, t);
        out << YAML::EndMap;
        return out;
    }
    
    void operator >> (const YAML::Node& node, ColorSpaceTransformRcPtr& t)
    {
        t = ColorSpaceTransform::Create();
        
        std::string key, stringval;
        
        for (YAML::Iterator iter = node.begin();
             iter != node.end();
             ++iter)
        {
            iter.first() >> key;
            
            if(key == "src")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<std::string>(stringval))
                  t->setSrc(stringval.c_str());
            }
            else if(key == "dst")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<std::string>(stringval))
                  t->setDst(stringval.c_str());
            }
            else if(key == "direction")
            {
                TransformDirection val;
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<TransformDirection>(val))
                  t->setDirection(val);
            }
            else
            {
                LogUnknownKeyWarning(node.Tag(), iter.first());
            }
        }
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, ConstColorSpaceTransformRcPtr t)
    {
        out << YAML::VerbatimTag("ColorSpaceTransform");
        out << YAML::Flow << YAML::BeginMap;
        out << YAML::Key << "src" << YAML::Value << t->getSrc();
        out << YAML::Key << "dst" << YAML::Value << t->getDst();
        EmitBaseTransformKeyValues(out, t);
        out << YAML::EndMap;
        return out;
    }
    
    void operator >> (const YAML::Node& node, LookTransformRcPtr& t)
    {
        t = LookTransform::Create();
        
        std::string key, stringval;
        
        for (YAML::Iterator iter = node.begin();
             iter != node.end();
             ++iter)
        {
            iter.first() >> key;
            
            if(key == "src")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<std::string>(stringval))
                  t->setSrc(stringval.c_str());
            }
            else if(key == "dst")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<std::string>(stringval))
                  t->setDst(stringval.c_str());
            }
            else if(key == "looks")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<std::string>(stringval))
                  t->setLooks(stringval.c_str());
            }
            else if(key == "direction")
            {
                TransformDirection val;
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<TransformDirection>(val))
                  t->setDirection(val);
            }
            else
            {
                LogUnknownKeyWarning(node.Tag(), iter.first());
            }
        }
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, ConstLookTransformRcPtr t)
    {
        out << YAML::VerbatimTag("LookTransform");
        out << YAML::Flow << YAML::BeginMap;
        out << YAML::Key << "src" << YAML::Value << t->getSrc();
        out << YAML::Key << "dst" << YAML::Value << t->getDst();
        out << YAML::Key << "looks" << YAML::Value << t->getLooks();
        EmitBaseTransformKeyValues(out, t);
        out << YAML::EndMap;
        return out;
    }
    
    void operator >> (const YAML::Node& node, ExponentTransformRcPtr& t)
    {
        t = ExponentTransform::Create();
        
        std::string key;
        
        for (YAML::Iterator iter = node.begin();
             iter != node.end();
             ++iter)
        {
            iter.first() >> key;
            
            if(key == "value")
            {
                std::vector<float> val;
                if (iter.second().Type() != YAML::NodeType::Null)
                {
                    iter.second() >> val;
                    if(val.size() != 4)
                    {
                        std::ostringstream os;
                        os << "ExponentTransform parse error, value field must be 4 ";
                        os << "floats. Found '" << val.size() << "'.";
                        throw Exception(os.str().c_str());
                    }
                    t->setValue(&val[0]);
                }
            }
            else if(key == "direction")
            {
                TransformDirection val;
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<TransformDirection>(val))
                  t->setDirection(val);
            }
            else
            {
                LogUnknownKeyWarning(node.Tag(), iter.first());
            }
        }
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, ConstExponentTransformRcPtr t)
    {
        out << YAML::VerbatimTag("ExponentTransform");
        out << YAML::Flow << YAML::BeginMap;
        
        std::vector<float> value(4, 0.0);
        t->getValue(&value[0]);
        out << YAML::Key << "value";
        out << YAML::Value << YAML::Flow << value;
        EmitBaseTransformKeyValues(out, t);
        out << YAML::EndMap;
        return out;
    }
    
    void operator >> (const YAML::Node& node, LogTransformRcPtr& t)
    {
        t = LogTransform::Create();
        
        std::string key;
        
        for (YAML::Iterator iter = node.begin();
             iter != node.end();
             ++iter)
        {
            iter.first() >> key;
            
            if(key == "base")
            {
                float val = 0.0f;
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<float>(val))
                  t->setBase(val);
            }
            else if(key == "direction")
            {
                TransformDirection val;
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<TransformDirection>(val))
                  t->setDirection(val);
            }
            else
            {
                LogUnknownKeyWarning(node.Tag(), iter.first());
            }
        }
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, ConstLogTransformRcPtr t)
    {
        out << YAML::VerbatimTag("LogTransform");
        out << YAML::Flow << YAML::BeginMap;
        out << YAML::Key << "base" << YAML::Value << t->getBase();
        EmitBaseTransformKeyValues(out, t);
        out << YAML::EndMap;
        return out;
    }
    
    void operator >> (const YAML::Node& node, MatrixTransformRcPtr& t)
    {
        t = MatrixTransform::Create();
        
        std::string key;
        
        for (YAML::Iterator iter = node.begin();
             iter != node.end();
             ++iter)
        {
            iter.first() >> key;
            
            if(key == "matrix")
            {
                std::vector<float> val;
                if (iter.second().Type() != YAML::NodeType::Null)
                {
                    iter.second() >> val;
                    if(val.size() != 16)
                    {
                        std::ostringstream os;
                        os << "MatrixTransform parse error, matrix field must be 16 ";
                        os << "floats. Found '" << val.size() << "'.";
                        throw Exception(os.str().c_str());
                    }
                    t->setMatrix(&val[0]);
                }
            }
            else if(key == "offset")
            {
                std::vector<float> val;
                if (iter.second().Type() != YAML::NodeType::Null)
                {
                    iter.second() >> val;
                    if(val.size() != 4)
                    {
                        std::ostringstream os;
                        os << "MatrixTransform parse error, offset field must be 4 ";
                        os << "floats. Found '" << val.size() << "'.";
                        throw Exception(os.str().c_str());
                    }
                    t->setOffset(&val[0]);
                }
            }
            else if(key == "direction")
            {
                TransformDirection val;
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<TransformDirection>(val))
                  t->setDirection(val);
            }
            else
            {
                LogUnknownKeyWarning(node.Tag(), iter.first());
            }
        }
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, ConstMatrixTransformRcPtr t)
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
        return out;
    }
    
    void operator >> (const YAML::Node& node, CDLTransformRcPtr& t)
    {
        t = CDLTransform::Create();
        
        std::string key;
        std::vector<float> floatvecval;
        
        for (YAML::Iterator iter = node.begin();
             iter != node.end();
             ++iter)
        {
            iter.first() >> key;
            
            if(key == "slope")
            {
                if (iter.second().Type() != YAML::NodeType::Null)
                {
                    iter.second() >> floatvecval;
                    if(floatvecval.size() != 3)
                    {
                        std::ostringstream os;
                        os << "CDLTransform parse error, 'slope' field must be 3 ";
                        os << "floats. Found '" << floatvecval.size() << "'.";
                        throw Exception(os.str().c_str());
                    }
                    t->setSlope(&floatvecval[0]);
                }
            }
            else if(key == "offset")
            {
                if (iter.second().Type() != YAML::NodeType::Null)
                {
                    iter.second() >> floatvecval;
                    if(floatvecval.size() != 3)
                    {
                        std::ostringstream os;
                        os << "CDLTransform parse error, 'offset' field must be 3 ";
                        os << "floats. Found '" << floatvecval.size() << "'.";
                        throw Exception(os.str().c_str());
                    }
                    t->setOffset(&floatvecval[0]);
                }
            }
            else if(key == "power")
            {
                if (iter.second().Type() != YAML::NodeType::Null)
                {
                    iter.second() >> floatvecval;
                    if(floatvecval.size() != 3)
                    {
                        std::ostringstream os;
                        os << "CDLTransform parse error, 'power' field must be 3 ";
                        os << "floats. Found '" << floatvecval.size() << "'.";
                        throw Exception(os.str().c_str());
                    }
                    t->setPower(&floatvecval[0]);
                }
            }
            else if(key == "saturation" || key == "sat")
            {
                float val = 0.0f;
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<float>(val))
                  t->setSat(val);
            }
            else if(key == "direction")
            {
                TransformDirection val;
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<TransformDirection>(val))
                  t->setDirection(val);
            }
            else
            {
                LogUnknownKeyWarning(node.Tag(), iter.first());
            }
        }
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, ConstCDLTransformRcPtr t)
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
        return out;
    }
    
    void operator >> (const YAML::Node& node, AllocationTransformRcPtr& t)
    {
        t = AllocationTransform::Create();
        
        std::string key;
        
        for (YAML::Iterator iter = node.begin();
             iter != node.end();
             ++iter)
        {
            iter.first() >> key;
            
            if(key == "allocation")
            {
                Allocation val;
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<Allocation>(val))
                  t->setAllocation(val);
            }
            else if(key == "vars")
            {
                std::vector<float> val;
                if (iter.second().Type() != YAML::NodeType::Null)
                {
                    iter.second() >> val;
                    if(!val.empty())
                    {
                        t->setVars(static_cast<int>(val.size()), &val[0]);
                    }
                }
            }
            else if(key == "direction")
            {
                TransformDirection val;
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<TransformDirection>(val))
                  t->setDirection(val);
            }
            else
            {
                LogUnknownKeyWarning(node.Tag(), iter.first());
            }
        }
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, ConstAllocationTransformRcPtr t)
    {
        out << YAML::VerbatimTag("AllocationTransform");
        out << YAML::Flow << YAML::BeginMap;
        
        out << YAML::Key << "allocation";
        out << YAML::Value << YAML::Flow << t->getAllocation();
        
        if(t->getNumVars() > 0)
        {
            std::vector<float> vars(t->getNumVars());
            t->getVars(&vars[0]);
            out << YAML::Key << "vars";
            out << YAML::Flow << YAML::Value << vars;
        }
        
        EmitBaseTransformKeyValues(out, t);
        out << YAML::EndMap;
        return out;
    }
    
    void operator >> (const YAML::Node& node, TruelightTransformRcPtr& t)
    {
        t = TruelightTransform::Create();
        
        std::string key, stringval;
        
        for (YAML::Iterator iter = node.begin();
             iter != node.end();
             ++iter)
        {
            iter.first() >> key;
            
            if(key == "config_root")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<std::string>(stringval))
                  t->setConfigRoot(stringval.c_str());
            }
            else if(key == "profile")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<std::string>(stringval))
                  t->setProfile(stringval.c_str());
            }
            else if(key == "camera")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<std::string>(stringval))
                  t->setCamera(stringval.c_str());
            }
            else if(key == "input_display")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<std::string>(stringval))
                  t->setInputDisplay(stringval.c_str());
            }
            else if(key == "recorder")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<std::string>(stringval))
                  t->setRecorder(stringval.c_str());
            }
            else if(key == "print")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<std::string>(stringval))
                  t->setPrint(stringval.c_str());
            }
            else if(key == "lamp")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<std::string>(stringval))
                  t->setLamp(stringval.c_str());
            }
            else if(key == "output_camera")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<std::string>(stringval))
                  t->setOutputCamera(stringval.c_str());
            }
            else if(key == "display")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<std::string>(stringval))
                  t->setDisplay(stringval.c_str());
            }
            else if(key == "cube_input")
            {
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<std::string>(stringval))
                  t->setCubeInput(stringval.c_str());
            }
            else if(key == "direction")
            {
                TransformDirection val;
                if (iter.second().Type() != YAML::NodeType::Null && 
                    iter.second().Read<TransformDirection>(val))
                  t->setDirection(val);
            }
            else
            {
                LogUnknownKeyWarning(node.Tag(), iter.first());
            }
        }
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, ConstTruelightTransformRcPtr t)
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
        return out;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    //  Enums
    
    YAML::Emitter& operator << (YAML::Emitter& out, BitDepth depth) {
        out << BitDepthToString(depth);
        return out;
    }
    
    void operator >> (const YAML::Node& node, BitDepth& depth) {
        std::string str;
        node.Read<std::string>(str);
        depth = BitDepthFromString(str.c_str());
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, Allocation alloc) {
        out << AllocationToString(alloc);
        return out;
    }
    
    void operator >> (const YAML::Node& node, Allocation& alloc) {
        std::string str;
        node.Read<std::string>(str);
        alloc = AllocationFromString(str.c_str());
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, ColorSpaceDirection dir) {
        out << ColorSpaceDirectionToString(dir);
        return out;
    }
    
    void operator >> (const YAML::Node& node, ColorSpaceDirection& dir) {
        std::string str;
        node.Read<std::string>(str);
        dir = ColorSpaceDirectionFromString(str.c_str());
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, TransformDirection dir) {
        out << TransformDirectionToString(dir);
        return out;
    }
    
    void operator >> (const YAML::Node& node, TransformDirection& dir) {
        std::string str;
        node.Read<std::string>(str);
        dir = TransformDirectionFromString(str.c_str());
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, Interpolation interp) {
        out << InterpolationToString(interp);
        return out;
    }
    
    void operator >> (const YAML::Node& node, Interpolation& interp) {
        std::string str;
        node.Read<std::string>(str);
        interp = InterpolationFromString(str.c_str());
    }
    
}
OCIO_NAMESPACE_EXIT

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

#include "OCIOYaml.h"

OCIO_NAMESPACE_ENTER
{
    ///////////////////////////////////////////////////////////////////////////
    //  Core
    
    void operator >> (const YAML::Node& node, ColorSpaceRcPtr& cs)
    {
        if(node.Tag() != "ColorSpace")
            return; // not a !<ColorSpace> tag
        if(node.FindValue("name") != NULL)  {
            std::string ret;
            if (node["name"].Read<std::string>(ret))
              cs->setName(ret.c_str());
        }
        if(node.FindValue("description") != NULL) {
            std::string ret;
            if (node["description"].Read<std::string>(ret))
              cs->setDescription(ret.c_str());
        }
        if(node.FindValue("family") != NULL)  {
            std::string ret;
            if (node["family"].Read<std::string>(ret))
              cs->setFamily(ret.c_str());
        }
        if(node.FindValue("bitdepth") != NULL)  {
            BitDepth ret;
            if (node["bitdepth"].Read<BitDepth>(ret))
              cs->setBitDepth(ret);
        }
        if(node.FindValue("isdata") != NULL)  {
            bool ret;
            if (node["isdata"].Read<bool>(ret))
              cs->setIsData(ret);
        }
        
        if(node.FindValue("allocation") != NULL)  {
            Allocation ret;
            if (node["allocation"].Read<Allocation>(ret))
              cs->setAllocation(ret);
        }
        // Backwards compatibility
        else if(node.FindValue("gpuallocation") != NULL)  {
            Allocation ret;
            if (node["gpuallocation"].Read<Allocation>(ret))
              cs->setAllocation(ret);
        }
        
        if(node.FindValue("allocationvars") != NULL)
        {
            std::vector<float> value;
            node["allocationvars"] >> value;
            if(!value.empty())
            {
                cs->setAllocationVars(static_cast<int>(value.size()), &value[0]);
            }
        }
        else if( (node.FindValue("gpumin") != NULL) &&
                 (node.FindValue("gpumax") != NULL) )
        {
            // Backwards compatibility
            std::vector<float> value(2);
            node["gpumin"].Read<float>(value[0]);
            node["gpumax"].Read<float>(value[1]);
            cs->setAllocationVars(static_cast<int>(value.size()), &value[0]);
        }
        
        if(node.FindValue("to_reference") != NULL)  {
            TransformRcPtr ret;
            if (node["to_reference"].Read<TransformRcPtr>(ret))
              cs->setTransform(ret, COLORSPACE_DIR_TO_REFERENCE);
        }
        if(node.FindValue("from_reference") != NULL)  {
            TransformRcPtr ret;
            if (node["from_reference"].Read<TransformRcPtr>(ret))
              cs->setTransform(ret, COLORSPACE_DIR_FROM_REFERENCE);
        }
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, ColorSpaceRcPtr cs)
    {
        out << YAML::VerbatimTag("ColorSpace");
        out << YAML::BeginMap;
        
        out << YAML::Key << "name" << YAML::Value << cs->getName();
        out << YAML::Key << "family" << YAML::Value << cs->getFamily();
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
        if(toref && cs->isTransformSpecified(COLORSPACE_DIR_TO_REFERENCE))
            out << YAML::Key << "to_reference" << YAML::Value << toref;
        
        ConstTransformRcPtr fromref = \
            cs->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
        if(fromref && cs->isTransformSpecified(COLORSPACE_DIR_FROM_REFERENCE))
            out << YAML::Key << "from_reference" << YAML::Value << fromref;
        
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
            // Default direction is always forward.
            if(t->getDirection() == TRANSFORM_DIR_FORWARD) return;
            
            out << YAML::Key << "direction";
            out << YAML::Value << YAML::Flow << t->getDirection();
        }
        
        void ReadBaseTransformKeyValues(const YAML::Node & node, TransformRcPtr & t)
        {
            if(node.FindValue("direction") != NULL) {
                TransformDirection ret;
                if (node["direction"].Read<TransformDirection>(ret))
                  t->setDirection(ret);
            }
            else
                t->setDirection(TRANSFORM_DIR_FORWARD);
        }
    }
    
    void operator >> (const YAML::Node& node, TransformRcPtr& t)
    {
        // TODO: when a Transform() types are registered, add logic here so that
        // it calls the correct Transform()::Create() for the !<tag> type
        
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
        // TODO: add DisplayTransform
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
            //  t = EmptyTransformRcPtr(new EmptyTransform(), &deleter);
            std::ostringstream os;
            os << "Unsupported transform type !<" << type << "> in OCIO profile";
            throw Exception(os.str().c_str());
        }
        
        ReadBaseTransformKeyValues(node, t);
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
    
    
    void operator >> (const YAML::Node& node, LookRcPtr& look)
    {
        if(node.Tag() != "Look")
            return;
        if(node.FindValue("name") != NULL)  {
            std::string ret;
            if (node["name"].Read<std::string>(ret))
              look->setName(ret.c_str());
        }
        if(node.FindValue("process_space") != NULL) {
            std::string ret;
            if (node["process_space"].Read<std::string>(ret))
              look->setProcessSpace(ret.c_str());
        }
        if(node.FindValue("transform") != NULL)  {
            TransformRcPtr ret;
            if (node["transform"].Read<TransformRcPtr>(ret))
              look->setTransform(ret);
        }
        if(node.FindValue("inverse_transform") != NULL)  {
            TransformRcPtr ret;
            if (node["inverse_transform"].Read<TransformRcPtr>(ret))
              look->setInverseTransform(ret);
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
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    //  Transforms
    
    void operator >> (const YAML::Node& node, GroupTransformRcPtr& t)
    {
        t = GroupTransform::Create();
        
        if(const YAML::Node * children = node.FindValue("children"))
        {
            for(unsigned i = 0; i <children->size(); ++i)
            {
                TransformRcPtr childTransform;
                (*children)[i].Read<TransformRcPtr>(childTransform);

                if(!childTransform)
                {
                    throw Exception("Child transform could not be parsed.");
                }
                
                t->push_back(childTransform);
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
        if(node.FindValue("src") != NULL) {
            std::string ret;
            if (node["src"].Read<std::string>(ret))
              t->setSrc(ret.c_str());
        }
        if(node.FindValue("cccid") != NULL) {
            std::string ret;
            if (node["cccid"].Read<std::string>(ret))
            t->setCCCId(ret.c_str());
        }
        if(node.FindValue("interpolation") != NULL) {
            Interpolation ret;
            if (node["interpolation"].Read<Interpolation>(ret))
              t->setInterpolation(ret);
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
        if(node.FindValue("src") != NULL) {
            std::string ret;
            if (node["src"].Read<std::string>(ret))
              t->setSrc(ret.c_str());
        }
        if(node.FindValue("dst") != NULL) {
            std::string ret;
            if (node["dst"].Read<std::string>(ret))
            t->setDst(ret.c_str());
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
        if(node.FindValue("src") != NULL) {
            std::string ret;
            if (node["src"].Read<std::string>(ret))
              t->setSrc(ret.c_str());
        }
        if(node.FindValue("dst") != NULL) {
            std::string ret;
            if (node["dst"].Read<std::string>(ret))
            t->setDst(ret.c_str());
        }
        if(node.FindValue("looks") != NULL) {
            std::string ret;
            if (node["looks"].Read<std::string>(ret))
            t->setLooks(ret.c_str());
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
        if(node.FindValue("value") != NULL)
        {
            std::vector<float> value;
            node["value"] >> value;
            if(value.size() != 4)
            {
                std::ostringstream os;
                os << "ExponentTransform parse error, value field must be 4 ";
                os << "floats. Found '" << value.size() << "'.";
                throw Exception(os.str().c_str());
            }
            t->setValue(&value[0]);
        }
        else throw Exception("ExponentTransform doesn't have a 'value:' specified.");
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
        if(node.FindValue("base") != NULL)  {
            float ret;
            if (node["base"].Read<float>(ret))
              t->setBase(ret);
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
        
        std::vector<float> matrix;
        std::vector<float> offset;
        
        // matrix
        if(node.FindValue("matrix") != NULL)
        {
            node["matrix"] >> matrix;
            if(matrix.size() != 16)
            {
                std::ostringstream os;
                os << "MatrixTransform parse error, 'matrix' field must be 16 ";
                os << "floats. Found '" << matrix.size() << "'.";
                throw Exception(os.str().c_str());
            }
        }
        
        // offset
        if(node.FindValue("offset") != NULL)
        {
            node["offset"] >> offset;
            if(offset.size() != 4)
            {
                std::ostringstream os;
                os << "MatrixTransform parse error, 'offset' field must be 4 ";
                os << "floats. Found '" << offset.size() << "'.";
                throw Exception(os.str().c_str());
            }
        }
        
        t->setValue(&matrix[0], &offset[0]);
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, ConstMatrixTransformRcPtr t)
    {
        std::vector<float> matrix(16, 0.0);
        std::vector<float> offset(4, 0.0);
        t->getValue(&matrix[0], &offset[0]);
        
        out << YAML::VerbatimTag("MatrixTransform");
        out << YAML::Flow << YAML::BeginMap;
        
        out << YAML::Key << "matrix";
        out << YAML::Value << YAML::Flow << matrix;
        out << YAML::Key << "offset";
        out << YAML::Value << YAML::Flow << offset;
        
        EmitBaseTransformKeyValues(out, t);
        out << YAML::EndMap;
        return out;
    }
    
    void operator >> (const YAML::Node& node, CDLTransformRcPtr& t)
    {
        t = CDLTransform::Create();
        
        if(node.FindValue("slope") != NULL)
        {
            std::vector<float> slope;
            node["slope"] >> slope;
            if(slope.size() != 3)
            {
                std::ostringstream os;
                os << "CDLTransform parse error, 'slope' field must be 3 ";
                os << "floats. Found '" << slope.size() << "'.";
                throw Exception(os.str().c_str());
            }
            t->setSlope(&slope[0]);
        }
        else throw Exception("CDLTransform doesn't have a 'slope:' specified.");
        
        if(node.FindValue("offset") != NULL)
        {
            std::vector<float> offset;
            node["offset"] >> offset;
            if(offset.size() != 3)
            {
                std::ostringstream os;
                os << "CDLTransform parse error, 'offset' field must be 3 ";
                os << "floats. Found '" << offset.size() << "'.";
                throw Exception(os.str().c_str());
            }
            t->setOffset(&offset[0]);
        }
        else throw Exception("CDLTransform doesn't have a 'offset:' specified.");
        
        if(node.FindValue("power") != NULL)
        {
            std::vector<float> power;
            node["power"] >> power;
            if(power.size() != 3)
            {
                std::ostringstream os;
                os << "CDLTransform parse error, 'power' field must be 3 ";
                os << "floats. Found '" << power.size() << "'.";
                throw Exception(os.str().c_str());
            }
            t->setPower(&power[0]);
        }
        else throw Exception("CDLTransform doesn't have a 'power:' specified.");
        
        if(node.FindValue("saturation") != NULL)  {
            float ret;
            if (node["saturation"].Read<float>(ret))
              t->setSat(ret);
        }
        else
            throw Exception("CDLTransform doesn't have a 'saturation:' specified.");
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, ConstCDLTransformRcPtr t)
    {
        std::vector<float> slope(3, 1.0);
        t->getSlope(&slope[0]);
        std::vector<float> offset(3, 0.0);
        t->getOffset(&offset[0]);
        std::vector<float> power(3, 1.0);
        t->getPower(&power[0]);
        
        out << YAML::VerbatimTag("CDLTransform");
        out << YAML::Flow << YAML::BeginMap;
        
        out << YAML::Key << "slope";
        out << YAML::Value << YAML::Flow << slope;
        out << YAML::Key << "offset";
        out << YAML::Value << YAML::Flow << offset;
        out << YAML::Key << "power";
        out << YAML::Value << YAML::Flow << power;
        out << YAML::Key << "saturation" << YAML::Value << t->getSat();
        
        EmitBaseTransformKeyValues(out, t);
        out << YAML::EndMap;
        return out;
    }
    
    void operator >> (const YAML::Node& node, AllocationTransformRcPtr& t)
    {
        t = AllocationTransform::Create();
        
        if(node.FindValue("allocation") != NULL)  {
            Allocation ret;
            if (node["allocation"].Read<Allocation>(ret))
              t->setAllocation(ret);
        }
        
        if(node.FindValue("vars") != NULL)
        {
            std::vector<float> value;
            node["vars"] >> value;
            if(!value.empty())
            {
                t->setVars(static_cast<int>(value.size()), &value[0]);
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
        if(node.FindValue("config_root") != NULL) {
            std::string ret;
            if(node["config_root"].Read<std::string>(ret))
                t->setConfigRoot(ret.c_str());
        }
        if(node.FindValue("profile") != NULL) {
            std::string ret;
            if(node["profile"].Read<std::string>(ret))
                t->setProfile(ret.c_str());
        }
        if(node.FindValue("camera") != NULL) {
            std::string ret;
            if(node["camera"].Read<std::string>(ret))
                t->setCamera(ret.c_str());
        }
        if(node.FindValue("input_display") != NULL) {
            std::string ret;
            if(node["input_display"].Read<std::string>(ret))
                t->setInputDisplay(ret.c_str());
        }
        if(node.FindValue("recorder") != NULL) {
            std::string ret;
            if(node["recorder"].Read<std::string>(ret))
                t->setRecorder(ret.c_str());
        }
        if(node.FindValue("print") != NULL) {
            std::string ret;
            if(node["print"].Read<std::string>(ret))
                t->setPrint(ret.c_str());
        }
        if(node.FindValue("lamp") != NULL) {
            std::string ret;
            if(node["lamp"].Read<std::string>(ret))
                t->setLamp(ret.c_str());
        }
        if(node.FindValue("output_camera") != NULL) {
            std::string ret;
            if(node["output_camera"].Read<std::string>(ret))
                t->setOutputCamera(ret.c_str());
        }
        if(node.FindValue("display") != NULL) {
            std::string ret;
            if(node["display"].Read<std::string>(ret))
                t->setDisplay(ret.c_str());
        }
        if(node.FindValue("cube_input") != NULL) {
            std::string ret;
            if(node["cube_input"].Read<std::string>(ret))
                t->setCubeInput(ret.c_str());
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

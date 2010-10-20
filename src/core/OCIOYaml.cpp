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
        if(node.GetTag() != "ColorSpace")
            return; // not a !<ColorSpace> tag
        if(node.FindValue("name") != NULL)
            cs->setName(node["name"].Read<std::string>().c_str());
        if(node.FindValue("description") != NULL)
            cs->setDescription(node["description"].Read<std::string>().c_str());
        if(node.FindValue("family") != NULL)
            cs->setFamily(node["family"].Read<std::string>().c_str());
        if(node.FindValue("bitdepth") != NULL)
            cs->setBitDepth(node["bitdepth"].Read<BitDepth>());
        if(node.FindValue("isdata") != NULL)
            cs->setIsData(node["isdata"].Read<bool>());
        if(node.FindValue("gpuallocation") != NULL)
            cs->setGpuAllocation(node["gpuallocation"].Read<GpuAllocation>());
        if(node.FindValue("gpumin") != NULL)
            cs->setGpuMin(node["gpumin"].Read<float>());
        if(node.FindValue("gpumax") != NULL)
            cs->setGpuMax(node["gpumax"].Read<float>());
        if(node.FindValue("to_reference") != NULL)
            cs->setTransform(node["to_reference"].Read<TransformRcPtr>(),
                COLORSPACE_DIR_TO_REFERENCE);
        if(node.FindValue("from_reference") != NULL)
            cs->setTransform(node["from_reference"].Read<TransformRcPtr>(),
                COLORSPACE_DIR_TO_REFERENCE);
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
        out << YAML::Key << "gpuallocation" << YAML::Value << cs->getGpuAllocation();
        out << YAML::Key << "gpumin" << YAML::Value << cs->getGpuMin();
        out << YAML::Key << "gpumax" << YAML::Value << cs->getGpuMax();
        
        ConstTransformRcPtr toref = \
            cs->getTransform(COLORSPACE_DIR_TO_REFERENCE);
        if(toref && cs->isTransformSpecified(COLORSPACE_DIR_TO_REFERENCE))
            out << YAML::Key << "to_reference" << YAML::Value << toref;
        
        ConstTransformRcPtr fromref = \
            cs->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
        if(fromref && cs->isTransformSpecified(COLORSPACE_DIR_FROM_REFERENCE))
            out << YAML::Key << "from_reference" << YAML::Value << fromref;
        
        //out << YAML::Literal << "\n\n";
        //out << YAML::Null;
        
        out << YAML::EndMap;
        
        return out;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    namespace
    {
        void AddBaseTransformPropertiesToYAMLMap(YAML::Emitter & out,
                                                 const ConstTransformRcPtr & t)
        {
            out << YAML::Key << "direction";
            out << YAML::Value << YAML::Flow << t->getDirection();
        }
        
        void ReadBaseTransformPropertiesFromYAMLMap(TransformRcPtr & t,
                                                    const YAML::Node & node)
        {
            if(node.FindValue("direction") != NULL)
                t->setDirection(node["direction"].Read<TransformDirection>());
            else
                t->setDirection(TRANSFORM_DIR_FORWARD);
        }
    }
    
    void operator >> (const YAML::Node& node, TransformRcPtr& t)
    {
        // TODO: when a Transform() types are registered, add logic here so that
        // it calls the correct Transform()::Create() for the !<tag> type
        
        std::string type = node.GetTag();
        
        if(type == "CDLTransform")
            t = node.Read<CDLTransformRcPtr>();
        else if(type == "CineonLogToLinTransform")
            t = node.Read<CineonLogToLinTransformRcPtr>();
        else if(type == "ColorSpaceTransform")
            t = node.Read<ColorSpaceTransformRcPtr>();
        // TODO: add DisplayTransform
        else if(type == "ExponentTransform")
            t = node.Read<ExponentTransformRcPtr>();
        else if(type == "FileTransform")
            t = node.Read<FileTransformRcPtr>();
        else if(type == "GroupTransform")
            t = node.Read<GroupTransformRcPtr>();
        else if(type == "MatrixTransform")
            t = node.Read<MatrixTransformRcPtr>();
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
        
        ReadBaseTransformPropertiesFromYAMLMap(t, node);
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, ConstTransformRcPtr t)
    {
        if(ConstCDLTransformRcPtr CDL_tran = \
            DynamicPtrCast<const CDLTransform>(t))
            out << CDL_tran;
        else if(ConstCineonLogToLinTransformRcPtr CineonLogToLin_tran = \
            DynamicPtrCast<const CineonLogToLinTransform>(t))
            out << CineonLogToLin_tran;
        else if(ConstColorSpaceTransformRcPtr ColorSpace_tran = \
            DynamicPtrCast<const ColorSpaceTransform>(t))
            out << ColorSpace_tran;
        // ConstExponentTransformRcPtr
        else if(ConstExponentTransformRcPtr Exponent_tran = \
            DynamicPtrCast<const ExponentTransform>(t))
            out << Exponent_tran;
        else if(ConstFileTransformRcPtr File_tran = \
            DynamicPtrCast<const FileTransform>(t))
            out << File_tran;
        else if(ConstGroupTransformRcPtr Group_tran = \
            DynamicPtrCast<const GroupTransform>(t))
            out << Group_tran;
        else if(ConstMatrixTransformRcPtr tran = \
            DynamicPtrCast<const MatrixTransform>(t))
            out << tran;
        else
            throw Exception("Unsupported Transform() type for serialization.");
        
        return out;
    }
    
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    //  Transforms
    
    void operator >> (const YAML::Node& node, GroupTransformRcPtr& t)
    {
        t = GroupTransform::Create();
        
        if(const YAML::Node * children = node.FindValue("children"))
        {
            for(unsigned i = 0; i <children->size(); ++i)
            {
                t->push_back((*children)[i].Read<TransformRcPtr>());
            }
        }
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, ConstGroupTransformRcPtr t)
    {
        out << YAML::VerbatimTag("GroupTransform");
        out << YAML::BeginMap;
        AddBaseTransformPropertiesToYAMLMap(out, t);
        
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
        if(node.FindValue("src") != NULL)
            t->setSrc(node["src"].Read<std::string>().c_str());
        if(node.FindValue("interpolation") != NULL)
            t->setInterpolation(node["interpolation"].Read<Interpolation>());
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, ConstFileTransformRcPtr t)
    {
        out << YAML::VerbatimTag("FileTransform");
        out << YAML::Flow << YAML::BeginMap;
        AddBaseTransformPropertiesToYAMLMap(out, t);
        
        out << YAML::Key << "src" << YAML::Value << t->getSrc();
        out << YAML::Key << "interpolation";
        out << YAML::Value << t->getInterpolation();
        out << YAML::EndMap;
        return out;
    }
    
    void operator >> (const YAML::Node& node, ColorSpaceTransformRcPtr& t)
    {
        t = ColorSpaceTransform::Create();
        if(node.FindValue("src") != NULL)
            t->setSrc(node["src"].Read<std::string>().c_str());
        if(node.FindValue("dst") != NULL)
            t->setDst(node["dst"].Read<std::string>().c_str());
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, ConstColorSpaceTransformRcPtr t)
    {
        out << YAML::VerbatimTag("ColorSpaceTransform");
        out << YAML::Flow << YAML::BeginMap;
        AddBaseTransformPropertiesToYAMLMap(out, t);
        
        out << YAML::Key << "src" << YAML::Value << t->getSrc();
        out << YAML::Key << "dst" << YAML::Value << t->getDst();
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
        AddBaseTransformPropertiesToYAMLMap(out, t);
        
        std::vector<float> value(4, 0.0);
        t->getValue(&value[0]);
        out << YAML::Key << "value";
        out << YAML::Value << YAML::Flow << value;
        out << YAML::EndMap;
        return out;
    }
    
    void operator >> (const YAML::Node& node, CineonLogToLinTransformRcPtr& t)
    {
        t = CineonLogToLinTransform::Create();
        
        // max_aim_density
        if(node.FindValue("max_aim_density") != NULL)
        {
            std::vector<float> value;
            node["max_aim_density"] >> value;
            if(value.size() != 3)
            {
                std::ostringstream os;
                os << "CineonLogToLinTransform parse error, 'max_aim_density' ";
                os << "field must be 3 floats. Found '" << value.size() << "'.";
                throw Exception(os.str().c_str());
            }
            t->setMaxAimDensity(&value[0]);
        }
        else throw Exception("CineonLogToLinTransform doesn't have a 'max_aim_density:' specified.");
        
        // neg_gamma
        if(node.FindValue("neg_gamma") != NULL)
        {
            std::vector<float> value;
            node["neg_gamma"] >> value;
            if(value.size() != 3)
            {
                std::ostringstream os;
                os << "CineonLogToLinTransform parse error, 'neg_gamma' ";
                os << "field must be 3 floats. Found '" << value.size() << "'.";
                throw Exception(os.str().c_str());
            }
            t->setNegGamma(&value[0]);
        }
        else throw Exception("CineonLogToLinTransform doesn't have a 'neg_gamma:' specified.");
        
        // neg_gray_reference
        if(node.FindValue("neg_gray_reference") != NULL)
        {
            std::vector<float> value;
            node["neg_gray_reference"] >> value;
            if(value.size() != 3)
            {
                std::ostringstream os;
                os << "CineonLogToLinTransform parse error, 'neg_gray_reference' ";
                os << "field must be 3 floats. Found '" << value.size() << "'.";
                throw Exception(os.str().c_str());
            }
            t->setNegGrayReference(&value[0]);
        }
        else throw Exception("CineonLogToLinTransform doesn't have a 'neg_gray_reference:' specified.");
        
        // linear_gray_reference
        if(node.FindValue("linear_gray_reference") != NULL)
        {
            std::vector<float> value;
            node["linear_gray_reference"] >> value;
            if(value.size() != 3)
            {
                std::ostringstream os;
                os << "CineonLogToLinTransform parse error, 'linear_gray_reference' ";
                os << "field must be 3 floats. Found '" << value.size() << "'.";
                throw Exception(os.str().c_str());
            }
            t->setLinearGrayReference(&value[0]);
        }
        else throw Exception("CineonLogToLinTransform doesn't have a 'linear_gray_reference:' specified.");
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, ConstCineonLogToLinTransformRcPtr t)
    {
        out << YAML::VerbatimTag("CineonLogToLinTransform");
        out << YAML::Flow << YAML::BeginMap;
        AddBaseTransformPropertiesToYAMLMap(out, t);
        
        std::vector<float> max_aim_density(3, 0.0);
        t->getMaxAimDensity(&max_aim_density[0]);
        out << YAML::Key << "max_aim_density";
        out << YAML::Value << YAML::Flow << max_aim_density;
        
        std::vector<float> neg_gamma(3, 0.0);
        t->getNegGamma(&neg_gamma[0]);
        out << YAML::Key << "neg_gamma";
        out << YAML::Value << YAML::Flow << neg_gamma;
        
        std::vector<float> neg_gray_reference(3, 0.0);
        t->getNegGrayReference(&neg_gray_reference[0]);
        out << YAML::Key << "neg_gray_reference";
        out << YAML::Value << YAML::Flow << neg_gray_reference;
        
        std::vector<float> linear_gray_reference(3, 0.0);
        t->getLinearGrayReference(&linear_gray_reference[0]);
        out << YAML::Key << "linear_gray_reference";
        out << YAML::Value << YAML::Flow << linear_gray_reference;
        
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
        else throw Exception("MatrixTransform doesn't have a 'matrix:' specified.");
        
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
        else throw Exception("MatrixTransform doesn't have a 'offset:' specified.");
        
        t->setValue(&matrix[0], &offset[0]);
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, ConstMatrixTransformRcPtr t)
    {
        std::vector<float> matrix(16, 0.0);
        std::vector<float> offset(4, 0.0);
        t->getValue(&matrix[0], &offset[0]);
        
        out << YAML::VerbatimTag("MatrixTransform");
        out << YAML::Flow << YAML::BeginMap;
        AddBaseTransformPropertiesToYAMLMap(out, t);
        
        out << YAML::Key << "matrix";
        out << YAML::Value << YAML::Flow << matrix;
        out << YAML::Key << "offset";
        out << YAML::Value << YAML::Flow << offset;
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
        
        if(node.FindValue("saturation") != NULL)
            t->setSat(node["saturation"].Read<float>());
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
        AddBaseTransformPropertiesToYAMLMap(out, t);
        
        out << YAML::Key << "slope";
        out << YAML::Value << YAML::Flow << slope;
        out << YAML::Key << "offset";
        out << YAML::Value << YAML::Flow << offset;
        out << YAML::Key << "power";
        out << YAML::Value << YAML::Flow << power;
        out << YAML::Key << "saturation" << YAML::Value << t->getSat();
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
        std::string str = node.Read<std::string>();
        depth = BitDepthFromString(str.c_str());
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, GpuAllocation alloc) {
        out << GpuAllocationToString(alloc);
        return out;
    }
    
    void operator >> (const YAML::Node& node, GpuAllocation& alloc) {
        std::string str = node.Read<std::string>();
        alloc = GpuAllocationFromString(str.c_str());
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, ColorSpaceDirection dir) {
        out << ColorSpaceDirectionToString(dir);
        return out;
    }
    
    void operator >> (const YAML::Node& node, ColorSpaceDirection& dir) {
        std::string str = node.Read<std::string>();
        dir = ColorSpaceDirectionFromString(str.c_str());
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, TransformDirection dir) {
        out << TransformDirectionToString(dir);
        return out;
    }
    
    void operator >> (const YAML::Node& node, TransformDirection& dir) {
        std::string str = node.Read<std::string>();
        dir = TransformDirectionFromString(str.c_str());
    }
    
    YAML::Emitter& operator << (YAML::Emitter& out, Interpolation interp) {
        out << InterpolationToString(interp);
        return out;
    }
    
    void operator >> (const YAML::Node& node, Interpolation& interp) {
        std::string str = node.Read<std::string>();
        interp = InterpolationFromString(str.c_str());
    }
    
}
OCIO_NAMESPACE_EXIT

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

#include <fstream>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "fileformats/cdl/CDLParser.h"
#include "CDLTransform.h"
#include "ops/Exponent/ExponentOps.h"
#include "ops/Matrix/MatrixOps.h"
#include "MathUtils.h"
#include "Mutex.h"
#include "OpBuilders.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"

OCIO_NAMESPACE_ENTER
{
    namespace
    {

 /*
<ColorCorrection id="shot 042">
    <SOPNode>
        <Description>Cool look for forest scenes</Description>
        <Slope>1 1 1</Slope>
        <Offset>0 0 0</Offset>
        <Power>1 1 1</Power>
    </SOPNode>
    <SatNode>
        <Saturation>1</Saturation>
    </SatNode>
</ColorCorrection>
        
 */
        
        std::string BuildXML(const CDLTransform & cdl)
        {
            std::ostringstream os;
            std::string id(ConvertSpecialCharToXmlToken(cdl.getID()));
            os << "<ColorCorrection id=\"" << id << "\">\n";
            os << "    <SOPNode>\n";
            std::string desc(ConvertSpecialCharToXmlToken(cdl.getDescription()));
            os << "        <Description>" << desc << "</Description>\n";
            float slopeval[3];
            cdl.getSlope(slopeval);
            os << "        <Slope>" << FloatVecToString(slopeval, 3) << "</Slope>\n";
            float offsetval[3];
            cdl.getOffset(offsetval);
            os << "        <Offset>" << FloatVecToString(offsetval, 3)  << "</Offset>\n";
            float powerval[3];
            cdl.getPower(powerval);
            os << "        <Power>" << FloatVecToString(powerval, 3)  << "</Power>\n";
            os << "    </SOPNode>\n";
            os << "    <SatNode>\n";
            os << "        <Saturation>" << FloatToString(cdl.getSat()) << "</Saturation>\n";
            os << "    </SatNode>\n";
            os << "</ColorCorrection>";

            return os.str();
        }
    }
    
    void LoadCDL(CDLTransform * cdl, const char * xml)
    {
        if(!xml || (strlen(xml) == 0))
        {
            std::ostringstream os;
            os << "Error loading CDL xml. ";
            os << "Null string provided.";
            throw Exception(os.str().c_str());
        }
        
        std::string xmlstring(xml);
        std::istringstream istream(xmlstring);

        CDLParser parser("xml string");
        parser.parse(istream);

        if (!parser.isCC())
        {
            throw Exception("Error loading CDL xml. ColorCorrection expected.");
        }

        CDLTransformRcPtr cdlRcPtr = CDLTransform::Create();
        parser.getCDLTransform(cdlRcPtr);

        cdl->setID(cdlRcPtr->getID());
        cdl->setDescription(cdlRcPtr->getDescription());
        float slope[3] = { 0.f, 0.f, 0.f };
        cdlRcPtr->getSlope(slope);
        cdl->setSlope(slope);
        float offset[3] = { 0.f, 0.f, 0.f };
        cdlRcPtr->getOffset(offset);
        cdl->setOffset(offset);
        float power[3] = { 0.f, 0.f, 0.f };
        cdlRcPtr->getPower(offset);
        cdl->setPower(offset);
        cdl->setSat(cdlRcPtr->getSat());
    }
    
    CDLTransformRcPtr CDLTransform::Create()
    {
        return CDLTransformRcPtr(new CDLTransform(), &deleter);
    }
    
    namespace
    {
        std::string GetCDLLocalCacheKey(const std::string & src, const std::string & cccid)
        {
            std::ostringstream os;
            os << src << " : " << cccid;
            return os.str();
        }
        
        std::string GetCDLLocalCacheKey(const char * src,
                                        int cccindex)
        {
            std::ostringstream os;
            os << src << " : " << cccindex;
            return os.str();
        }
        
        CDLTransformMap g_cache;
        StringBoolMap g_cacheSrcIsCC;
        Mutex g_cacheMutex;
    }
    
    void ClearCDLTransformFileCache()
    {
        AutoMutex lock(g_cacheMutex);
        g_cache.clear();
    }
    
    // TODO: Expose functions for introspecting in ccc file
    // TODO: Share caching with normal cdl pathway
    
    CDLTransformRcPtr CDLTransform::CreateFromFile(const char * src, const char * cccid_)
    {
        if(!src || (strlen(src) == 0) )
        {
            std::ostringstream os;
            os << "Error loading CDL xml. ";
            os << "Source file not specified.";
            throw Exception(os.str().c_str());
        }
        
        std::string cccid;
        if(cccid_) cccid = cccid_;
        
        // Check cache
        AutoMutex lock(g_cacheMutex);
        
        // Use g_cacheSrcIsCC as a proxy for if we have loaded this source
        // file already (in which case it must be in cache, or an error)
        
        StringBoolMap::iterator srcIsCCiter = g_cacheSrcIsCC.find(src);
        if(srcIsCCiter != g_cacheSrcIsCC.end())
        {
            // If the source file is known to be a pure ColorCorrection element,
            // null out the cccid so its ignored.
            if(srcIsCCiter->second) cccid = "";
            
            // Search for the cccid by name
            CDLTransformMap::iterator iter = 
                g_cache.find(GetCDLLocalCacheKey(src, cccid));
            if(iter != g_cache.end())
            {
                return iter->second;
            }
            
            // Search for cccid by index
            int cccindex=0;
            if(StringToInt(&cccindex, cccid.c_str(), true))
            {
                iter = g_cache.find(GetCDLLocalCacheKey(src, cccindex));
                if(iter != g_cache.end())
                {
                    return iter->second;
                }
            }
            
            std::ostringstream os;
            os << "The specified cccid/cccindex '" << cccid;
            os << "' could not be loaded from the src file '";
            os << src;
            os << "'.";
            throw Exception (os.str().c_str());
        }
        
        
        // Try to read all ccs from the file, into cache
        std::ifstream istream(src);
        if(istream.fail()) {
            std::ostringstream os;
            os << "Error could not read CDL source file '" << src;
            os << "'. Please verify the file exists and appropriate ";
            os << "permissions are set.";
            throw Exception (os.str().c_str());
        }
        
        CDLParser parser(src);
        parser.parse(istream);

        if (parser.isCC())
        {
            // Load a single ColorCorrection into the cache
            CDLTransformRcPtr cdl = CDLTransform::Create();
            parser.getCDLTransform(cdl);

            cccid = "";
            g_cacheSrcIsCC[src] = true;
            g_cache[GetCDLLocalCacheKey(src, cccid)] = cdl;
        }
        else if(parser.isCCC())
        {
            // Load all CCs from the ColorCorrectionCollection
            // into the cache
            CDLTransformMap transformMap;
            CDLTransformVec transformVec;
            parser.getCDLTransforms(transformMap, transformVec);
            
            if(transformVec.empty())
            {
                std::ostringstream os;
                os << "Error loading ccc xml. ";
                os << "No ColorCorrection elements found in file '";
                os << src << "'.";
                throw Exception(os.str().c_str());
            }
            
            g_cacheSrcIsCC[src] = false;
            
            // Add all by transforms to cache
            // First by index, then by id
            for(unsigned int i=0; i<transformVec.size(); ++i)
            {
                g_cache[GetCDLLocalCacheKey(src, i)] = transformVec[i];
            }
            
            for(CDLTransformMap::iterator iter = transformMap.begin();
                iter != transformMap.end();
                ++iter)
            {
                g_cache[GetCDLLocalCacheKey(src, iter->first)] = iter->second;
            }
        }
        
        // The all transforms should be in the cache.  Look it up, and try
        // to return it.
        {
            // Search for the cccid by name
            CDLTransformMap::iterator iter = 
                g_cache.find(GetCDLLocalCacheKey(src, cccid));
            if(iter != g_cache.end())
            {
                return iter->second;
            }
            
            // Search for cccid by index
            int cccindex=0;
            if(StringToInt(&cccindex, cccid.c_str(), true))
            {
                iter = g_cache.find(GetCDLLocalCacheKey(src, cccindex));
                if(iter != g_cache.end())
                {
                    return iter->second;
                }
            }
            
            std::ostringstream os;
            os << "The specified cccid/cccindex '" << cccid;
            os << "' could not be loaded from the src file '";
            os << src;
            os << "'.";
            throw Exception (os.str().c_str());
        }
    }
    
    void CDLTransform::deleter(CDLTransform* t)
    {
        delete t;
    }
    
    class CDLTransform::Impl
    {
    public:
        TransformDirection dir_;
        
        float sop_[9];
        float sat_;
        std::string id_;
        std::string description_;

        mutable std::string xml_;
        
        Impl() :
            dir_(TRANSFORM_DIR_FORWARD),
            sat_(1.0f)
        {
            sop_[0] = 1.0f;
            sop_[1] = 1.0f;
            sop_[2] = 1.0f;
            sop_[3] = 0.0f;
            sop_[4] = 0.0f;
            sop_[5] = 0.0f;
            sop_[6] = 1.0f;
            sop_[7] = 1.0f;
            sop_[8] = 1.0f;
        }
        
        ~Impl()
        {
        
        }
        
        Impl& operator= (const Impl & rhs)
        {
            if (this != &rhs)
            {
                dir_ = rhs.dir_;

                memcpy(sop_, rhs.sop_, sizeof(float) * 9);
                sat_ = rhs.sat_;
                id_ = rhs.id_;
                description_ = rhs.description_;
            }

            return *this;
        }
        
    };
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    CDLTransform::CDLTransform()
        : m_impl(new CDLTransform::Impl)
    {
    }
    
    TransformRcPtr CDLTransform::createEditableCopy() const
    {
        CDLTransformRcPtr transform = CDLTransform::Create();
        *(transform->m_impl) = *m_impl;
        return transform;
    }
    
    CDLTransform::~CDLTransform()
    {
        delete m_impl;
        m_impl = NULL;
    }
    
    CDLTransform& CDLTransform::operator= (const CDLTransform & rhs)
    {
        if (this != &rhs)
        {
            *m_impl = *rhs.m_impl;
        }
        return *this;
    }
    
    TransformDirection CDLTransform::getDirection() const
    {
        return getImpl()->dir_;
    }
    
    void CDLTransform::setDirection(TransformDirection dir)
    {
        getImpl()->dir_ = dir;
    }
    
    void CDLTransform::validate() const
    {
        Transform::validate();

        // TODO: Uncomment in upcoming PR that contains the OpData validate.
        //       getImpl()->data_->validate()
        //       
        //       OpData classes are the enhancement of some existing 
        //       structures (like Lut1D and Lut3D) by encapsulating
        //       all the data and adding high-level behaviors.
    }

    const char * CDLTransform::getXML() const
    {
        getImpl()->xml_ = BuildXML(*this);
        return getImpl()->xml_.c_str();
    }
    
    void CDLTransform::setXML(const char * xml)
    {
        LoadCDL(this, xml);
    }
    
    // We use this approach, rather than comparing XML to get around the
    // case where setXML with extra data was provided.
    
    bool CDLTransform::equals(const ConstCDLTransformRcPtr & other) const
    {
        if(!other) return false;
        
        if(getImpl()->dir_ != other->getImpl()->dir_) return false;
        
        const float abserror = 1e-9f;
        
        for(int i=0; i<9; ++i)
        {
            if(!equalWithAbsError(getImpl()->sop_[i], other->getImpl()->sop_[i], abserror))
            {
                return false;
            }
        }
        
        if(!equalWithAbsError(getImpl()->sat_, other->getImpl()->sat_, abserror))
        {
            return false;
        }
        
        if(getImpl()->id_ != other->getImpl()->id_)
        {
            return false;
        }
        
        if(getImpl()->description_ != other->getImpl()->description_)
        {
            return false;
        }
        
        return true;
    }
    
    void CDLTransform::setSlope(const float * rgb)
    {
        memcpy(&getImpl()->sop_[0], rgb, sizeof(float)*3);
    }
    
    void CDLTransform::getSlope(float * rgb) const
    {
        memcpy(rgb, &getImpl()->sop_[0], sizeof(float)*3);
    }

    void CDLTransform::setOffset(const float * rgb)
    {
        memcpy(&getImpl()->sop_[3], rgb, sizeof(float)*3);
    }
    
    void CDLTransform::getOffset(float * rgb) const
    {
        memcpy(rgb, &getImpl()->sop_[3], sizeof(float)*3);
    }

    void CDLTransform::setPower(const float * rgb)
    {
        memcpy(&getImpl()->sop_[6], rgb, sizeof(float)*3);
    }
    
    void CDLTransform::getPower(float * rgb) const
    {
        memcpy(rgb, &getImpl()->sop_[6], sizeof(float)*3);
    }

    void CDLTransform::setSOP(const float * vec9)
    {
        memcpy(&getImpl()->sop_, vec9, sizeof(float)*9);
    }
    
    void CDLTransform::getSOP(float * vec9) const
    {
        memcpy(vec9, &getImpl()->sop_, sizeof(float)*9);
    }

    void CDLTransform::setSat(float sat)
    {
        getImpl()->sat_ = sat;
    }
    
    float CDLTransform::getSat() const
    {
        return getImpl()->sat_;
    }

    void CDLTransform::getSatLumaCoefs(float * rgb) const
    {
        if(!rgb) return;
        rgb[0] = 0.2126f;
        rgb[1] = 0.7152f;
        rgb[2] = 0.0722f;
    }
    
    void CDLTransform::setID(const char * id)
    {
        if(id)
        {
            getImpl()->id_ = id;
        }
        else
        {
            getImpl()->id_ = "";
        }
    }
    
    const char * CDLTransform::getID() const
    {
        return getImpl()->id_.c_str();
    }
    
    void CDLTransform::setDescription(const char * desc)
    {
        if(desc)
        {
            getImpl()->description_ = desc;
        }
        else
        {
            getImpl()->description_ = "";
        }
    }
    
    const char * CDLTransform::getDescription() const
    {
        return getImpl()->description_.c_str();
    }
    
    std::ostream& operator<< (std::ostream& os, const CDLTransform& t)
    {
        float sop[9];
        t.getSOP(sop);
        
        os << "<CDLTransform";
        os << " direction=" << TransformDirectionToString(t.getDirection());
        os << ", sop=";
        for (unsigned int i=0; i<9; ++i)
        {
            if(i!=0) os << " ";
            os << sop[i];
        }
        os << ", sat=" << t.getSat();
        os << ">";
        return os;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    void BuildCDLOps(OpRcPtrVec & ops,
                     const Config & /*config*/,
                     const CDLTransform & cdlTransform,
                     TransformDirection dir)
    {
        float scale4[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        cdlTransform.getSlope(scale4);
        
        float offset4[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        cdlTransform.getOffset(offset4);
        
        float power4[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        cdlTransform.getPower(power4);
        
        float lumaCoef3[] = { 1.0f, 1.0f, 1.0f };
        cdlTransform.getSatLumaCoefs(lumaCoef3);
        
        float sat = cdlTransform.getSat();
        
        TransformDirection combinedDir = CombineTransformDirections(dir,
                                                  cdlTransform.getDirection());
        
        // TODO: Confirm ASC Sat math is correct.
        // TODO: Handle Clamping conditions more explicitly
        
        if(combinedDir == TRANSFORM_DIR_FORWARD)
        {
            // 1) Scale + Offset
            CreateScaleOffsetOp(ops, scale4, offset4, TRANSFORM_DIR_FORWARD);
            
            // 2) Power + Clamp
            CreateExponentOp(ops, power4, TRANSFORM_DIR_FORWARD);
            
            // 3) Saturation + Clamp
            CreateSaturationOp(ops, sat, lumaCoef3, TRANSFORM_DIR_FORWARD);
        }
        else if(combinedDir == TRANSFORM_DIR_INVERSE)
        {
            // 3) Saturation + Clamp
            CreateSaturationOp(ops, sat, lumaCoef3, TRANSFORM_DIR_INVERSE);
            
            // 2) Power + Clamp
            CreateExponentOp(ops, power4, TRANSFORM_DIR_INVERSE);
            
            // 1) Scale + Offset
            CreateScaleOffsetOp(ops, scale4, offset4, TRANSFORM_DIR_INVERSE);
        }
    }
}
OCIO_NAMESPACE_EXIT

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"
#include "UnitTestFiles.h"

OIIO_ADD_TEST(CDLTransform, CreateFromCCFile)
{
    const std::string filePath(std::string(OCIO::getTestFilesDir())
                               + "/cdl_test1.cc");
    OCIO::CDLTransformRcPtr transform =
        OCIO::CDLTransform::CreateFromFile(filePath.c_str(), NULL);

    {
        std::string idStr(transform->getID());
        OIIO_CHECK_EQUAL("foo", idStr);
        std::string descStr(transform->getDescription());
        OIIO_CHECK_EQUAL("this is a description", descStr);
        float slope[3] = {0.f, 0.f, 0.f};
        OIIO_CHECK_NO_THROW(transform->getSlope(slope));
        OIIO_CHECK_EQUAL(1.1f, slope[0]);
        OIIO_CHECK_EQUAL(1.2f, slope[1]);
        OIIO_CHECK_EQUAL(1.3f, slope[2]);
        float offset[3] = { 0.f, 0.f, 0.f };
        OIIO_CHECK_NO_THROW(transform->getOffset(offset));
        OIIO_CHECK_EQUAL(2.1f, offset[0]);
        OIIO_CHECK_EQUAL(2.2f, offset[1]);
        OIIO_CHECK_EQUAL(2.3f, offset[2]);
        float power[3] = { 0.f, 0.f, 0.f };
        OIIO_CHECK_NO_THROW(transform->getPower(power));
        OIIO_CHECK_EQUAL(3.1f, power[0]);
        OIIO_CHECK_EQUAL(3.2f, power[1]);
        OIIO_CHECK_EQUAL(3.3f, power[2]);
        OIIO_CHECK_EQUAL(0.7f, transform->getSat());
    }

    const std::string expectedOutXML(
        "<ColorCorrection id=\"foo\">\n"
        "    <SOPNode>\n"
        "        <Description>this is a description</Description>\n"
        "        <Slope>1.1 1.2 1.3</Slope>\n"
        "        <Offset>2.1 2.2 2.3</Offset>\n"
        "        <Power>3.1 3.2 3.3</Power>\n"
        "    </SOPNode>\n"
        "    <SatNode>\n"
        "        <Saturation>0.7</Saturation>\n"
        "    </SatNode>\n"
        "</ColorCorrection>");
    std::string outXML(transform->getXML());
    OIIO_CHECK_EQUAL(expectedOutXML, outXML);

    // parse again using setXML
    OCIO::CDLTransformRcPtr transformCDL = OCIO::CDLTransform::Create();
    transformCDL->setXML(expectedOutXML.c_str());
    {
        std::string idStr(transformCDL->getID());
        OIIO_CHECK_EQUAL("foo", idStr);
        std::string descStr(transformCDL->getDescription());
        OIIO_CHECK_EQUAL("this is a description", descStr);
        float slope[3] = { 0.f, 0.f, 0.f };
        OIIO_CHECK_NO_THROW(transformCDL->getSlope(slope));
        OIIO_CHECK_EQUAL(1.1f, slope[0]);
        OIIO_CHECK_EQUAL(1.2f, slope[1]);
        OIIO_CHECK_EQUAL(1.3f, slope[2]);
        float offset[3] = { 0.f, 0.f, 0.f };
        OIIO_CHECK_NO_THROW(transformCDL->getOffset(offset));
        OIIO_CHECK_EQUAL(2.1f, offset[0]);
        OIIO_CHECK_EQUAL(2.2f, offset[1]);
        OIIO_CHECK_EQUAL(2.3f, offset[2]);
        float power[3] = { 0.f, 0.f, 0.f };
        OIIO_CHECK_NO_THROW(transformCDL->getPower(power));
        OIIO_CHECK_EQUAL(3.1f, power[0]);
        OIIO_CHECK_EQUAL(3.2f, power[1]);
        OIIO_CHECK_EQUAL(3.3f, power[2]);
        OIIO_CHECK_EQUAL(0.7f, transformCDL->getSat());
    }
}

OIIO_ADD_TEST(CDLTransform, CreateFromCCCFile)
{
    const std::string filePath(std::string(OCIO::getTestFilesDir())
                               + "/cdl_test1.ccc");
    {
        // Using ID
        OCIO::CDLTransformRcPtr transform =
            OCIO::CDLTransform::CreateFromFile(filePath.c_str(), "cc0003");
        std::string idStr(transform->getID());
        OIIO_CHECK_EQUAL("cc0003", idStr);
        std::string descStr(transform->getDescription());
        OIIO_CHECK_EQUAL("golden", descStr);
        float slope[3] = { 0.f, 0.f, 0.f };
        OIIO_CHECK_NO_THROW(transform->getSlope(slope));
        OIIO_CHECK_EQUAL(1.2f, slope[0]);
        OIIO_CHECK_EQUAL(1.1f, slope[1]);
        OIIO_CHECK_EQUAL(1.0f, slope[2]);
        float offset[3] = { 0.f, 0.f, 0.f };
        OIIO_CHECK_NO_THROW(transform->getOffset(offset));
        OIIO_CHECK_EQUAL(0.0f, offset[0]);
        OIIO_CHECK_EQUAL(0.0f, offset[1]);
        OIIO_CHECK_EQUAL(0.0f, offset[2]);
        float power[3] = { 0.f, 0.f, 0.f };
        OIIO_CHECK_NO_THROW(transform->getPower(power));
        OIIO_CHECK_EQUAL(0.9f, power[0]);
        OIIO_CHECK_EQUAL(1.0f, power[1]);
        OIIO_CHECK_EQUAL(1.2f, power[2]);
        OIIO_CHECK_EQUAL(1.0f, transform->getSat());
    }
    {
        // Using 0 based index
        OCIO::CDLTransformRcPtr transform =
            OCIO::CDLTransform::CreateFromFile(filePath.c_str(), "3");
        std::string idStr(transform->getID());
        OIIO_CHECK_EQUAL("", idStr);
        std::string descStr(transform->getDescription());
        OIIO_CHECK_EQUAL("", descStr);
        float slope[3] = { 0.f, 0.f, 0.f };
        OIIO_CHECK_NO_THROW(transform->getSlope(slope));
        OIIO_CHECK_EQUAL(4.0f, slope[0]);
        OIIO_CHECK_EQUAL(5.0f, slope[1]);
        OIIO_CHECK_EQUAL(6.0f, slope[2]);
        float offset[3] = { 0.f, 0.f, 0.f };
        OIIO_CHECK_NO_THROW(transform->getOffset(offset));
        OIIO_CHECK_EQUAL(0.0f, offset[0]);
        OIIO_CHECK_EQUAL(0.0f, offset[1]);
        OIIO_CHECK_EQUAL(0.0f, offset[2]);
        float power[3] = { 0.f, 0.f, 0.f };
        OIIO_CHECK_NO_THROW(transform->getPower(power));
        OIIO_CHECK_EQUAL(0.9f, power[0]);
        OIIO_CHECK_EQUAL(1.0f, power[1]);
        OIIO_CHECK_EQUAL(1.2f, power[2]);
        OIIO_CHECK_EQUAL(1.0f, transform->getSat());
    }
}

OIIO_ADD_TEST(CDLTransform, CreateFromCCCFileFailure)
{
    const std::string filePath(std::string(OCIO::getTestFilesDir())
        + "/cdl_test1.ccc");
    {
        // Using ID
        OIIO_CHECK_THROW_WHAT(
            OCIO::CDLTransform::CreateFromFile(filePath.c_str(), "NotFound"),
            OCIO::Exception, "could not be loaded from the src file");
    }
    {
        // Using index
        OIIO_CHECK_THROW_WHAT(
            OCIO::CDLTransform::CreateFromFile(filePath.c_str(), "42"),
            OCIO::Exception, "could not be loaded from the src file");
    }
}

OIIO_ADD_TEST(CDLTransform, EscapeXML)
{
    const std::string inputXML(
        "<ColorCorrection id=\"Esc &lt; &amp; &quot; &apos; &gt;\">\n"
        "    <SOPNode>\n"
        "        <Description>These: &lt; &amp; &quot; &apos; &gt; are escape chars</Description>\n"
        "        <Slope>1.1 1.2 1.3</Slope>\n"
        "        <Offset>2.1 2.2 2.3</Offset>\n"
        "        <Power>3.1 3.2 3.3</Power>\n"
        "    </SOPNode>\n"
        "    <SatNode>\n"
        "        <Saturation>0.7</Saturation>\n"
        "    </SatNode>\n"
        "</ColorCorrection>");

    // parse again using setXML
    OCIO::CDLTransformRcPtr transformCDL = OCIO::CDLTransform::Create();
    transformCDL->setXML(inputXML.c_str());
    {
        std::string idStr(transformCDL->getID());
        OIIO_CHECK_EQUAL("Esc < & \" ' >", idStr);
        std::string descStr(transformCDL->getDescription());
        OIIO_CHECK_EQUAL("These: < & \" ' > are escape chars", descStr);
    }
}

#endif
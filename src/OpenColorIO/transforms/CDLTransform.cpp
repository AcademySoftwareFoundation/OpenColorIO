// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <fstream>
#include <sstream>
#include <string.h>

#include <OpenColorIO/OpenColorIO.h>

#include "fileformats/cdl/CDLParser.h"
#include "CDLTransform.h"
#include "MathUtils.h"
#include "Mutex.h"
#include "OpBuilders.h"
#include "ops/CDL/CDLOpData.h"
#include "ParseUtils.h"
#include "Platform.h"
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
        // Note: OCIO v1 supported a single <Description> element as a child of <SOPNode>.
        // In OCIO v2, we added support for an arbitrary number of <Description> elements
        // as children of <SOPNode> or <SatNode>.  We also added support for <Description>,
        // <InputDescription>, and <ViewingDescription> elements as children of
        // <ColorCorrection> (per the ASC CDL spec).  All of these elements are also read
        // and written from CLF/CTF files.
        std::string BuildXML(const CDLTransform & cdl)
        {
            const auto & metadata = cdl.getFormatMetadata();
            StringVec mainDesc;
            StringVec inputDesc;
            StringVec viewingDesc;
            StringVec sopDesc;
            StringVec satDesc;
            const int nbElt = metadata.getNumChildrenElements();
            for (int i = 0; i < nbElt; ++i)
            {
                const auto & elt = metadata.getChildElement(i);
                if (0 == Platform::Strcasecmp(elt.getName(), METADATA_DESCRIPTION))
                {
                    mainDesc.push_back(ConvertSpecialCharToXmlToken(elt.getValue()));
                }
                else if (0 == Platform::Strcasecmp(elt.getName(), METADATA_INPUT_DESCRIPTION))
                {
                    inputDesc.push_back(ConvertSpecialCharToXmlToken(elt.getValue()));
                }
                else if (0 == Platform::Strcasecmp(elt.getName(), METADATA_VIEWING_DESCRIPTION))
                {
                    viewingDesc.push_back(ConvertSpecialCharToXmlToken(elt.getValue()));
                }
                else if (0 == Platform::Strcasecmp(elt.getName(), METADATA_SOP_DESCRIPTION))
                {
                    sopDesc.push_back(ConvertSpecialCharToXmlToken(elt.getValue()));
                }
                else if (0 == Platform::Strcasecmp(elt.getName(), METADATA_SAT_DESCRIPTION))
                {
                    satDesc.push_back(ConvertSpecialCharToXmlToken(elt.getValue()));
                }
            }

            std::ostringstream os;
            std::string id(ConvertSpecialCharToXmlToken(cdl.getID()));
            os << "<ColorCorrection id=\"" << id << "\">\n";
            for (const auto & desc : mainDesc)
            {
                os << "    <Description>" << desc << "</Description>\n";
            }
            for (const auto & desc : inputDesc)
            {
                os << "    <InputDescription>" << desc << "</InputDescription>\n";
            }
            for (const auto & desc : viewingDesc)
            {
                os << "    <ViewingDescription>" << desc << "</ViewingDescription>\n";
            }
            os << "    <SOPNode>\n";
            for (const auto & desc : sopDesc)
            {
                os << "        <Description>" << desc << "</Description>\n";
            }
            double slopeval[3];
            cdl.getSlope(slopeval);
            os << "        <Slope>" << DoubleVecToString(slopeval, 3) << "</Slope>\n";
            double offsetval[3];
            cdl.getOffset(offsetval);
            os << "        <Offset>" << DoubleVecToString(offsetval, 3)  << "</Offset>\n";
            double powerval[3];
            cdl.getPower(powerval);
            os << "        <Power>" << DoubleVecToString(powerval, 3)  << "</Power>\n";
            os << "    </SOPNode>\n";
            os << "    <SatNode>\n";
            for (const auto & desc : satDesc)
            {
                os << "        <Description>" << desc << "</Description>\n";
            }
            os << "        <Saturation>" << DoubleToString(cdl.getSat()) << "</Saturation>\n";
            os << "    </SatNode>\n";
            os << "</ColorCorrection>";

            return os.str();
        }
    }
    
    void LoadCDL(CDLTransform & cdl, const char * xml)
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

        CDLTransformRcPtr cdlRcPtr;
        parser.getCDLTransform(cdlRcPtr);

        cdl.setID(cdlRcPtr->getID());

        const auto & srcFormatMetadata = cdlRcPtr->getFormatMetadata();
        const auto & srcMetadata = dynamic_cast<const FormatMetadataImpl &>(srcFormatMetadata);

        auto & formatMetadata = cdl.getFormatMetadata();
        auto & metadata = dynamic_cast<FormatMetadataImpl &>(formatMetadata);
        metadata = srcMetadata;

        double slope[3] = { 0., 0., 0. };
        cdlRcPtr->getSlope(slope);
        cdl.setSlope(slope);
        double offset[3] = { 0., 0., 0. };
        cdlRcPtr->getOffset(offset);
        cdl.setOffset(offset);
        double power[3] = { 0., 0., 0. };
        cdlRcPtr->getPower(power);
        cdl.setPower(power);
        cdl.setSat(cdlRcPtr->getSat());
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
        g_cacheSrcIsCC.clear();
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
            FormatMetadataImpl metadata{ METADATA_ROOT };

            parser.getCDLTransforms(transformMap, transformVec, metadata);
            
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
    
    class CDLTransform::Impl : public CDLOpData
    {
    public:       
        Impl() 
            :   CDLOpData()
        {
        }

        Impl(const Impl &) = delete;

        ~Impl() { }
        
        Impl& operator= (const Impl & rhs)
        {
            if (this != &rhs)
            {
                m_direction = rhs.m_direction;
                CDLOpData::operator=(rhs);
            }

            return *this;
        }

        TransformDirection m_direction = TRANSFORM_DIR_FORWARD;

        mutable std::string m_xml;
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
        return getImpl()->m_direction;
    }
    
    void CDLTransform::setDirection(TransformDirection dir)
    {
        getImpl()->m_direction = dir;
    }
    
    void CDLTransform::validate() const
    {
        try
        {
            Transform::validate();
            getImpl()->validate();
        }
        catch(Exception & ex)
        {
            std::string errMsg("CDLTransform validation failed: ");
            errMsg += ex.what();
            throw Exception(errMsg.c_str());
        }
    }

    const char * CDLTransform::getXML() const
    {
        getImpl()->m_xml = BuildXML(*this);
        return getImpl()->m_xml.c_str();
    }
    
    void CDLTransform::setXML(const char * xml)
    {
        LoadCDL(*this, xml);
    }
    
    // We use this approach, rather than comparing XML to get around the
    // case where setXML with extra data was provided.

    bool CDLTransform::equals(const ConstCDLTransformRcPtr & other) const
    {
        if(!other) return false;
        
        // NB: A tolerance of 1e-9 is used when comparing the parameters.
        return *(getImpl()) == *(other->getImpl())
            && getImpl()->m_direction == other->getImpl()->m_direction;
    }
    
    void CDLTransform::setSlope(const float * rgb)
    {
        if(!rgb)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        getImpl()->setSlopeParams(CDLOpData::ChannelParams(rgb[0], rgb[1], rgb[2]));
    }
    void CDLTransform::setSlope(const double * rgb)
    {
        if (!rgb)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        getImpl()->setSlopeParams(CDLOpData::ChannelParams(rgb[0], rgb[1], rgb[2]));
    }

    
    void CDLTransform::getSlope(float * rgb) const
    {
        if(!rgb)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        const CDLOpData::ChannelParams & params = getImpl()->getSlopeParams();
        rgb[0] = (float)params[0];
        rgb[1] = (float)params[1];
        rgb[2] = (float)params[2];
    }
    void CDLTransform::getSlope(double * rgb) const
    {
        if (!rgb)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        const CDLOpData::ChannelParams & params = getImpl()->getSlopeParams();
        rgb[0] = params[0];
        rgb[1] = params[1];
        rgb[2] = params[2];
    }

    void CDLTransform::setOffset(const float * rgb)
    {
        if(!rgb)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        getImpl()->setOffsetParams(CDLOpData::ChannelParams(rgb[0], rgb[1], rgb[2]));
    }
    void CDLTransform::setOffset(const double * rgb)
    {
        if (!rgb)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        getImpl()->setOffsetParams(CDLOpData::ChannelParams(rgb[0], rgb[1], rgb[2]));
    }

    void CDLTransform::getOffset(float * rgb) const
    {
        if(!rgb)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        const CDLOpData::ChannelParams & params = getImpl()->getOffsetParams();
        rgb[0] = (float)params[0];
        rgb[1] = (float)params[1];
        rgb[2] = (float)params[2];
    }
    void CDLTransform::getOffset(double * rgb) const
    {
        if (!rgb)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        const CDLOpData::ChannelParams & params = getImpl()->getOffsetParams();
        rgb[0] = params[0];
        rgb[1] = params[1];
        rgb[2] = params[2];
    }

    void CDLTransform::setPower(const float * rgb)
    {
        if(!rgb)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        getImpl()->setPowerParams(CDLOpData::ChannelParams(rgb[0], rgb[1], rgb[2]));
    }
    void CDLTransform::setPower(const double * rgb)
    {
        if (!rgb)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        getImpl()->setPowerParams(CDLOpData::ChannelParams(rgb[0], rgb[1], rgb[2]));
    }

    void CDLTransform::getPower(float * rgb) const
    {
        if(!rgb)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        const CDLOpData::ChannelParams & params = getImpl()->getPowerParams();
        rgb[0] = (float)params[0];
        rgb[1] = (float)params[1];
        rgb[2] = (float)params[2];
    }
    void CDLTransform::getPower(double * rgb) const
    {
        if (!rgb)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        const CDLOpData::ChannelParams & params = getImpl()->getPowerParams();
        rgb[0] = params[0];
        rgb[1] = params[1];
        rgb[2] = params[2];
    }

    void CDLTransform::setSOP(const float * vec9)
    {
        if(!vec9)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        getImpl()->setSlopeParams(CDLOpData::ChannelParams(vec9[0], vec9[1], vec9[2]));
        getImpl()->setOffsetParams(CDLOpData::ChannelParams(vec9[3], vec9[4], vec9[5]));
        getImpl()->setPowerParams(CDLOpData::ChannelParams(vec9[6], vec9[7], vec9[8]));
    }
    void CDLTransform::setSOP(const double * vec9)
    {
        if (!vec9)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        getImpl()->setSlopeParams(CDLOpData::ChannelParams(vec9[0], vec9[1], vec9[2]));
        getImpl()->setOffsetParams(CDLOpData::ChannelParams(vec9[3], vec9[4], vec9[5]));
        getImpl()->setPowerParams(CDLOpData::ChannelParams(vec9[6], vec9[7], vec9[8]));
    }

    void CDLTransform::getSOP(float * vec9) const
    {
        if(!vec9)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        const CDLOpData::ChannelParams & slopes = getImpl()->getSlopeParams();
        vec9[0] = (float)slopes[0];
        vec9[1] = (float)slopes[1];
        vec9[2] = (float)slopes[2];

        const CDLOpData::ChannelParams & offsets = getImpl()->getOffsetParams();
        vec9[3] = (float)offsets[0];
        vec9[4] = (float)offsets[1];
        vec9[5] = (float)offsets[2];

        const CDLOpData::ChannelParams & powers = getImpl()->getPowerParams();
        vec9[6] = (float)powers[0];
        vec9[7] = (float)powers[1];
        vec9[8] = (float)powers[2];
    }
    void CDLTransform::getSOP(double * vec9) const
    {
        if (!vec9)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        const CDLOpData::ChannelParams & slopes = getImpl()->getSlopeParams();
        vec9[0] = slopes[0];
        vec9[1] = slopes[1];
        vec9[2] = slopes[2];

        const CDLOpData::ChannelParams & offsets = getImpl()->getOffsetParams();
        vec9[3] = offsets[0];
        vec9[4] = offsets[1];
        vec9[5] = offsets[2];

        const CDLOpData::ChannelParams & powers = getImpl()->getPowerParams();
        vec9[6] = powers[0];
        vec9[7] = powers[1];
        vec9[8] = powers[2];
    }

    void CDLTransform::setSat(double sat)
    {
        getImpl()->setSaturation(sat);
    }
    
    double CDLTransform::getSat() const
    {
        return getImpl()->getSaturation();
    }

    void CDLTransform::getSatLumaCoefs(float * rgb) const
    {
        if(!rgb)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        rgb[0] = 0.2126f;
        rgb[1] = 0.7152f;
        rgb[2] = 0.0722f;
    }
    void CDLTransform::getSatLumaCoefs(double * rgb) const
    {
        if (!rgb)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        rgb[0] = 0.2126;
        rgb[1] = 0.7152;
        rgb[2] = 0.0722;
    }

    void CDLTransform::setID(const char * id)
    {
        getImpl()->setID(id ? id : "");
    }
    
    const char * CDLTransform::getID() const
    {
        return getImpl()->getID().c_str();
    }
    
    void CDLTransform::setDescription(const char * desc)
    {
        auto & info = getImpl()->getFormatMetadata();
        info.getChildrenElements().clear();
        if (desc)
        {
            info.getChildrenElements().emplace_back(METADATA_SOP_DESCRIPTION, desc);
        }
    }
    
    const char * CDLTransform::getDescription() const
    {
        const auto & info = getImpl()->getFormatMetadata();
        int descIndex = info.getFirstChildIndex(METADATA_SOP_DESCRIPTION);
        if (descIndex == -1)
        {
            return "";
        }
        else
        {
            return info.getChildrenElements()[descIndex].getValue();
        }
    }

    FormatMetadata & CDLTransform::getFormatMetadata()
    {
        return m_impl->getFormatMetadata();
    }
    const FormatMetadata & CDLTransform::getFormatMetadata() const
    {
        return m_impl->getFormatMetadata();
    }

    std::ostream& operator<< (std::ostream& os, const CDLTransform& t)
    {
        double sop[9];
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
    
}
OCIO_NAMESPACE_EXIT

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"
#include "UnitTestUtils.h"
#include "Platform.h"


OCIO_ADD_TEST(CDLTransform, equality)
{
    const OCIO::CDLTransformRcPtr cdl1 = OCIO::CDLTransform::Create();
    const OCIO::CDLTransformRcPtr cdl2 = OCIO::CDLTransform::Create();

    OCIO_CHECK_ASSERT(cdl1->equals(cdl1));
    OCIO_CHECK_ASSERT(cdl1->equals(cdl2));
    OCIO_CHECK_ASSERT(cdl2->equals(cdl1));

    const OCIO::CDLTransformRcPtr cdl3 = OCIO::CDLTransform::Create();
    cdl3->setSat(cdl3->getSat()+0.002f);

    OCIO_CHECK_ASSERT(!cdl1->equals(cdl3));
    OCIO_CHECK_ASSERT(!cdl2->equals(cdl3));
    OCIO_CHECK_ASSERT(cdl3->equals(cdl3));
}

OCIO_ADD_TEST(CDLTransform, create_from_cc_file)
{
    const std::string filePath(std::string(OCIO::getTestFilesDir())
                               + "/cdl_test1.cc");
    OCIO::CDLTransformRcPtr transform =
        OCIO::CDLTransform::CreateFromFile(filePath.c_str(), NULL);

    {
        std::string idStr(transform->getID());
        OCIO_CHECK_EQUAL("foo", idStr);
        std::string descStr(transform->getDescription());
        OCIO_CHECK_EQUAL("this is a description", descStr);
        float slope[3] = {0.f, 0.f, 0.f};
        OCIO_CHECK_NO_THROW(transform->getSlope(slope));
        OCIO_CHECK_EQUAL(1.1f, slope[0]);
        OCIO_CHECK_EQUAL(1.2f, slope[1]);
        OCIO_CHECK_EQUAL(1.3f, slope[2]);
        float offset[3] = { 0.f, 0.f, 0.f };
        OCIO_CHECK_NO_THROW(transform->getOffset(offset));
        OCIO_CHECK_EQUAL(2.1f, offset[0]);
        OCIO_CHECK_EQUAL(2.2f, offset[1]);
        OCIO_CHECK_EQUAL(2.3f, offset[2]);
        float power[3] = { 0.f, 0.f, 0.f };
        OCIO_CHECK_NO_THROW(transform->getPower(power));
        OCIO_CHECK_EQUAL(3.1f, power[0]);
        OCIO_CHECK_EQUAL(3.2f, power[1]);
        OCIO_CHECK_EQUAL(3.3f, power[2]);
        OCIO_CHECK_EQUAL(0.7, transform->getSat());
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
    OCIO_CHECK_EQUAL(expectedOutXML, outXML);

    // parse again using setXML
    OCIO::CDLTransformRcPtr transformCDL = OCIO::CDLTransform::Create();
    transformCDL->setXML(expectedOutXML.c_str());
    {
        std::string idStr(transformCDL->getID());
        OCIO_CHECK_EQUAL("foo", idStr);

        float slope[3] = { 0.f, 0.f, 0.f };
        OCIO_CHECK_NO_THROW(transformCDL->getSlope(slope));
        OCIO_CHECK_EQUAL(1.1f, slope[0]);
        OCIO_CHECK_EQUAL(1.2f, slope[1]);
        OCIO_CHECK_EQUAL(1.3f, slope[2]);
        float offset[3] = { 0.f, 0.f, 0.f };
        OCIO_CHECK_NO_THROW(transformCDL->getOffset(offset));
        OCIO_CHECK_EQUAL(2.1f, offset[0]);
        OCIO_CHECK_EQUAL(2.2f, offset[1]);
        OCIO_CHECK_EQUAL(2.3f, offset[2]);
        float power[3] = { 0.f, 0.f, 0.f };
        OCIO_CHECK_NO_THROW(transformCDL->getPower(power));
        OCIO_CHECK_EQUAL(3.1f, power[0]);
        OCIO_CHECK_EQUAL(3.2f, power[1]);
        OCIO_CHECK_EQUAL(3.3f, power[2]);
        OCIO_CHECK_EQUAL(0.7, transformCDL->getSat());
    }
}

OCIO_ADD_TEST(CDLTransform, create_from_ccc_file)
{
    const std::string filePath(std::string(OCIO::getTestFilesDir())
                               + "/cdl_test1.ccc");
    {
        // Using ID
        OCIO::CDLTransformRcPtr transform =
            OCIO::CDLTransform::CreateFromFile(filePath.c_str(), "cc0003");
        std::string idStr(transform->getID());
        OCIO_CHECK_EQUAL("cc0003", idStr);

        const auto & metadata = transform->getFormatMetadata();
        OCIO_REQUIRE_EQUAL(metadata.getNumChildrenElements(), 6);
        OCIO_CHECK_EQUAL(std::string(metadata.getChildElement(3).getName()), "SOPDescription");
        OCIO_CHECK_EQUAL(std::string(metadata.getChildElement(3).getValue()), "golden");

        float slope[3] = { 0.f, 0.f, 0.f };
        OCIO_CHECK_NO_THROW(transform->getSlope(slope));
        OCIO_CHECK_EQUAL(1.2f, slope[0]);
        OCIO_CHECK_EQUAL(1.1f, slope[1]);
        OCIO_CHECK_EQUAL(1.0f, slope[2]);
        float offset[3] = { 0.f, 0.f, 0.f };
        OCIO_CHECK_NO_THROW(transform->getOffset(offset));
        OCIO_CHECK_EQUAL(0.0f, offset[0]);
        OCIO_CHECK_EQUAL(0.0f, offset[1]);
        OCIO_CHECK_EQUAL(0.0f, offset[2]);
        float power[3] = { 0.f, 0.f, 0.f };
        OCIO_CHECK_NO_THROW(transform->getPower(power));
        OCIO_CHECK_EQUAL(0.9f, power[0]);
        OCIO_CHECK_EQUAL(1.0f, power[1]);
        OCIO_CHECK_EQUAL(1.2f, power[2]);
        OCIO_CHECK_EQUAL(1.0f, transform->getSat());
    }
    {
        // Using 0 based index
        OCIO::CDLTransformRcPtr transform =
            OCIO::CDLTransform::CreateFromFile(filePath.c_str(), "3");
        std::string idStr(transform->getID());
        OCIO_CHECK_EQUAL("", idStr);

        float slope[3] = { 0.f, 0.f, 0.f };
        OCIO_CHECK_NO_THROW(transform->getSlope(slope));
        OCIO_CHECK_EQUAL(4.0f, slope[0]);
        OCIO_CHECK_EQUAL(5.0f, slope[1]);
        OCIO_CHECK_EQUAL(6.0f, slope[2]);
        float offset[3] = { 0.f, 0.f, 0.f };
        OCIO_CHECK_NO_THROW(transform->getOffset(offset));
        OCIO_CHECK_EQUAL(0.0f, offset[0]);
        OCIO_CHECK_EQUAL(0.0f, offset[1]);
        OCIO_CHECK_EQUAL(0.0f, offset[2]);
        float power[3] = { 0.f, 0.f, 0.f };
        OCIO_CHECK_NO_THROW(transform->getPower(power));
        OCIO_CHECK_EQUAL(0.9f, power[0]);
        OCIO_CHECK_EQUAL(1.0f, power[1]);
        OCIO_CHECK_EQUAL(1.2f, power[2]);
        OCIO_CHECK_EQUAL(1.0f, transform->getSat());
    }
}

OCIO_ADD_TEST(CDLTransform, create_from_ccc_file_failure)
{
    const std::string filePath(std::string(OCIO::getTestFilesDir())
        + "/cdl_test1.ccc");
    {
        // Using ID
        OCIO_CHECK_THROW_WHAT(
            OCIO::CDLTransform::CreateFromFile(filePath.c_str(), "NotFound"),
            OCIO::Exception, "could not be loaded from the src file");
    }
    {
        // Using index
        OCIO_CHECK_THROW_WHAT(
            OCIO::CDLTransform::CreateFromFile(filePath.c_str(), "42"),
            OCIO::Exception, "could not be loaded from the src file");
    }
}

OCIO_ADD_TEST(CDLTransform, escape_xml)
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

    // Parse again using setXML.
    OCIO::CDLTransformRcPtr transformCDL = OCIO::CDLTransform::Create();
    transformCDL->setXML(inputXML.c_str());
    {
        std::string idStr(transformCDL->getID());
        OCIO_CHECK_EQUAL("Esc < & \" ' >", idStr);

        const auto & metadata = transformCDL->getFormatMetadata();
        OCIO_REQUIRE_EQUAL(metadata.getNumChildrenElements(), 1);

        std::string descStr(metadata.getChildElement(0).getValue());
        OCIO_CHECK_EQUAL("These: < & \" ' > are escape chars", descStr);
    }
}

namespace
{
    static const std::string kContentsA = {
        "<ColorCorrectionCollection>\n"
        "    <ColorCorrection id=\"cc03343\">\n"
        "        <SOPNode>\n"
        "            <Slope>0.1 0.2 0.3 </Slope>\n"
        "            <Offset>0.8 0.1 0.3 </Offset>\n"
        "            <Power>0.5 0.5 0.5 </Power>\n"
        "        </SOPNode>\n"
        "        <SATNode>\n"
        "            <Saturation>1</Saturation>\n"
        "        </SATNode>\n"
        "    </ColorCorrection>\n"
        "    <ColorCorrection id=\"cc03344\">\n"
        "        <SOPNode>\n"
        "            <Slope>1.2 1.3 1.4 </Slope>\n"
        "            <Offset>0.3 0 0 </Offset>\n"
        "            <Power>0.75 0.75 0.75 </Power>\n"
        "        </SOPNode>\n"
        "        <SATNode>\n"
        "            <Saturation>1</Saturation>\n"
        "        </SATNode>\n"
        "    </ColorCorrection>\n"
        "</ColorCorrectionCollection>\n"
        };

    static const std::string kContentsB = {
        "<ColorCorrectionCollection>\n"
        "    <ColorCorrection id=\"cc03343\">\n"
        "        <SOPNode>\n"
        "            <Slope>1.1 2.2 3.3 </Slope>\n"
        "            <Offset>0.8 0.1 0.3 </Offset>\n"
        "            <Power>0.5 0.5 0.5 </Power>\n"
        "        </SOPNode>\n"
        "        <SATNode>\n"
        "            <Saturation>1</Saturation>\n"
        "        </SATNode>\n"
        "    </ColorCorrection>\n"
        "    <ColorCorrection id=\"cc03344\">\n"
        "        <SOPNode>\n"
        "            <Slope>1.2 1.3 1.4 </Slope>\n"
        "            <Offset>0.3 0 0 </Offset>\n"
        "            <Power>0.75 0.75 0.75 </Power>\n"
        "        </SOPNode>\n"
        "        <SATNode>\n"
        "            <Saturation>1</Saturation>\n"
        "        </SATNode>\n"
        "    </ColorCorrection>\n"
        "</ColorCorrectionCollection>\n"
        };


}

OCIO_ADD_TEST(CDLTransform, clear_caches)
{
    std::string filename;
    OCIO_CHECK_NO_THROW(OCIO::Platform::CreateTempFilename(filename, ""));

    std::fstream stream(filename, std::ios_base::out|std::ios_base::trunc);
    stream << kContentsA;
    stream.close();

    OCIO::CDLTransformRcPtr transform; 
    OCIO_CHECK_NO_THROW(transform = OCIO::CDLTransform::CreateFromFile(filename.c_str(), "cc03343"));

    float slope[3];

    OCIO_CHECK_NO_THROW(transform->getSlope(slope));
    OCIO_CHECK_EQUAL(slope[0], 0.1f);
    OCIO_CHECK_EQUAL(slope[1], 0.2f);
    OCIO_CHECK_EQUAL(slope[2], 0.3f);

    stream.open(filename, std::ios_base::out|std::ios_base::trunc);
    stream << kContentsB;
    stream.close();

    OCIO_CHECK_NO_THROW(OCIO::ClearAllCaches());

    OCIO_CHECK_NO_THROW(transform = OCIO::CDLTransform::CreateFromFile(filename.c_str(), "cc03343"));
    OCIO_CHECK_NO_THROW(transform->getSlope(slope));

    OCIO_CHECK_EQUAL(slope[0], 1.1f);
    OCIO_CHECK_EQUAL(slope[1], 2.2f);
    OCIO_CHECK_EQUAL(slope[2], 3.3f);
}

OCIO_ADD_TEST(CDLTransform, faulty_file_content)
{
    std::string filename;
    OCIO_CHECK_NO_THROW(OCIO::Platform::CreateTempFilename(filename, ""));

    {
        std::fstream stream(filename, std::ios_base::out|std::ios_base::trunc);
        stream << kContentsA << "Some Extra faulty information";
        stream.close();

        OCIO_CHECK_THROW_WHAT(OCIO::CDLTransform::CreateFromFile(filename.c_str(), "cc03343"),
                              OCIO::Exception,
                              "Error parsing ColorCorrectionCollection (). Error is: XML parsing error");
    }
    {
        // Duplicated identifier.

        std::string faultyContent = kContentsA;
        const std::size_t found = faultyContent.find("cc03344");
        OCIO_CHECK_ASSERT(found!=std::string::npos);
        faultyContent.replace(found, strlen("cc03344"), "cc03343");


        std::fstream stream(filename, std::ios_base::out|std::ios_base::trunc);
        stream << faultyContent;
        stream.close();

        OCIO_CHECK_THROW_WHAT(OCIO::CDLTransform::CreateFromFile(filename.c_str(), "cc03343"),
                              OCIO::Exception,
                              "Error loading ccc xml. Duplicate elements with 'cc03343'");
    }
}

OCIO_ADD_TEST(CDLTransform, buildops)
{
    auto cdl = OCIO::CDLTransform::Create();

    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    // Testing v1 backwards compatibility.
    config->setMajorVersion(1);

    OCIO::OpRcPtrVec ops;
    OCIO::BuildCDLOps(ops, *config, *cdl,
                      OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(ops.size(), 3);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);

    ops.clear();
    const double power[]{1.1, 1.0, 1.0};
    cdl->setPower(power);
    OCIO::BuildCDLOps(ops, *config, *cdl,
                      OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_EQUAL(ops[0]->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_EQUAL(ops[ops.size() - 1]->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    ops.clear();
    cdl->setSat(1.5);
    OCIO::BuildCDLOps(ops, *config, *cdl,
                      OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_EQUAL(ops[0]->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_EQUAL(ops[ops.size() - 1]->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    ops.clear();
    const double offset[]{ 0.0, 0.1, 0.0 };
    cdl->setOffset(offset);
    OCIO::BuildCDLOps(ops, *config, *cdl,
                      OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO_CHECK_EQUAL(ops[0]->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_EQUAL(ops[ops.size() - 1]->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(ops.size(), 3);

    // Testing v2 onward behavior.
    config->setMajorVersion(2);
    ops.clear();
    OCIO::BuildCDLOps(ops, *config, *cdl,
                      OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO::ConstOpRcPtr op = OCIO::DynamicPtrCast<const OCIO::Op>(ops[0]);
    OCIO_CHECK_EQUAL(op->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_EQUAL(op->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);
    auto cdldata = OCIO::DynamicPtrCast<const OCIO::CDLOpData>(op->data());
    OCIO_REQUIRE_ASSERT(cdldata);
}

OCIO_ADD_TEST(CDLTransform, description)
{
    auto cdl = OCIO::CDLTransform::Create();

    const std::string id("TestCDL");
    cdl->setID(id.c_str());

    const std::string initialDesc(cdl->getDescription());
    OCIO_CHECK_ASSERT(initialDesc.empty());

    auto & metadata = cdl->getFormatMetadata();
    metadata.addChildElement(OCIO::METADATA_DESCRIPTION, "Desc");
    metadata.addChildElement(OCIO::METADATA_INPUT_DESCRIPTION, "Input Desc");
    const std::string sopDesc("SOP Desc");
    metadata.addChildElement(OCIO::METADATA_SOP_DESCRIPTION, sopDesc.c_str());
    metadata.addChildElement(OCIO::METADATA_SAT_DESCRIPTION, "Sat Desc");

    OCIO_CHECK_EQUAL(sopDesc, cdl->getDescription());

    const std::string newSopDesc("SOP Desc New");
    cdl->setDescription(newSopDesc.c_str());

    OCIO_CHECK_EQUAL(newSopDesc, cdl->getDescription());
    // setDescription will replace all existing children.
    OCIO_CHECK_EQUAL(metadata.getNumChildrenElements(), 1);
    // Not the ID.
    OCIO_CHECK_EQUAL(id, cdl->getID());

    // NULL description is removing all children.
    cdl->setDescription(nullptr);
    OCIO_CHECK_EQUAL(metadata.getNumChildrenElements(), 0);
}

#endif

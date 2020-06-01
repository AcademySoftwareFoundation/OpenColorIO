// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <fstream>
#include <sstream>
#include <string.h>

#include <OpenColorIO/OpenColorIO.h>

#include "fileformats/cdl/CDLParser.h"
#include "Logging.h"
#include "MathUtils.h"
#include "Mutex.h"
#include "OpBuilders.h"
#include "ParseUtils.h"
#include "Platform.h"
#include "transforms/CDLTransform.h"


namespace OCIO_NAMESPACE
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
    StringUtils::StringVec mainDesc;
    StringUtils::StringVec inputDesc;
    StringUtils::StringVec viewingDesc;
    StringUtils::StringVec sopDesc;
    StringUtils::StringVec satDesc;
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
    if (!xml || (strlen(xml) == 0))
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
    return CDLTransformRcPtr(new CDLTransformImpl(), &CDLTransformImpl::deleter);
}

namespace
{
std::string GetCDLLocalCacheKey(const std::string & src, const std::string & cccid)
{
    std::ostringstream os;
    os << src << " : " << cccid;
    return os.str();
}

std::string GetCDLLocalCacheKey(const char * src, int cccindex)
{
    std::ostringstream os;
    os << src << " : " << cccindex;
    return os.str();
}

CDLTransformMap g_cache;
StringBoolMap g_cacheSrcIsCC;
Mutex g_cacheMutex;
} // namespace

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
    if (!src || !*src)
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
    // file already (in which case it must be in cache, or an error).

    StringBoolMap::iterator srcIsCCiter = g_cacheSrcIsCC.find(src);
    if (srcIsCCiter != g_cacheSrcIsCC.end())
    {
        // If the source file is known to be a pure ColorCorrection element,
        // null out the cccid so its ignored.
        if(srcIsCCiter->second) cccid = "";

        // Search for the cccid by name
        CDLTransformMap::iterator iter = g_cache.find(GetCDLLocalCacheKey(src, cccid));
        if(iter != g_cache.end())
        {
            return iter->second;
        }

        // Search for cccid by index
        int cccindex=0;
        if (StringToInt(&cccindex, cccid.c_str(), true))
        {
            iter = g_cache.find(GetCDLLocalCacheKey(src, cccindex));
            if (iter != g_cache.end())
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

    // Try to read all ccs from the file, into cache.
    std::ifstream istream(src);
    if (istream.fail())
    {
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
        // Load a single ColorCorrection into the cache.
        CDLTransformRcPtr cdl = CDLTransform::Create();
        parser.getCDLTransform(cdl);

        cccid = "";
        g_cacheSrcIsCC[src] = true;
        g_cache[GetCDLLocalCacheKey(src, cccid)] = cdl;
    }
    else
    {
        // Load all CCs from the ColorCorrectionCollection or from the ColorDecisionList
        // into the cache.
        CDLTransformMap transformMap;
        CDLTransformVec transformVec;
        FormatMetadataImpl metadata;

        parser.getCDLTransforms(transformMap, transformVec, metadata);

        if (transformVec.empty())
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
        for (unsigned int i = 0; i < transformVec.size(); ++i)
        {
            g_cache[GetCDLLocalCacheKey(src, i)] = transformVec[i];
        }

        for (CDLTransformMap::iterator iter = transformMap.begin();
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
        CDLTransformMap::iterator iter = g_cache.find(GetCDLLocalCacheKey(src, cccid));
        if(iter != g_cache.end())
        {
            return iter->second;
        }

        // Search for cccid by index
        int cccindex=0;
        if (StringToInt(&cccindex, cccid.c_str(), true))
        {
            iter = g_cache.find(GetCDLLocalCacheKey(src, cccindex));
            if (iter != g_cache.end())
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

void CDLTransformImpl::deleter(CDLTransform * t)
{
    delete static_cast<CDLTransformImpl *>(t);
}

TransformRcPtr CDLTransformImpl::createEditableCopy() const
{
    TransformRcPtr transform = CDLTransform::Create();
    dynamic_cast<CDLTransformImpl*>(transform.get())->data() = data();
    return transform;
}

TransformDirection CDLTransformImpl::getDirection() const  noexcept
{
    return data().getDirection();
}

void CDLTransformImpl::setDirection(TransformDirection dir) noexcept
{
    data().setDirection(dir);
}

void CDLTransformImpl::validate() const
{
    try
    {
        Transform::validate();
        data().validate();
    }
    catch (Exception & ex)
    {
        std::string errMsg("CDLTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }
}

FormatMetadata & CDLTransformImpl::getFormatMetadata() noexcept
{
    return data().getFormatMetadata();
}

const FormatMetadata & CDLTransformImpl::getFormatMetadata() const noexcept
{
    return data().getFormatMetadata();
}

const char * CDLTransformImpl::getXML() const
{
    m_xml = BuildXML(*this);
    return m_xml.c_str();
}

void CDLTransformImpl::setXML(const char * xml)
{
    LoadCDL(*this, xml);
}

// We use this approach, rather than comparing XML to get around the
// case where setXML with extra data was provided.

bool CDLTransformImpl::equals(const CDLTransform & other) const noexcept
{
    if (this == &other) return true;
    // NB: A tolerance of 1e-9 is used when comparing the parameters.
    return data() == dynamic_cast<const CDLTransformImpl*>(&other)->data();
}

CDLStyle CDLTransformImpl::getStyle() const
{
    return CDLOpData::ConvertStyle(data().getStyle());
}

void CDLTransformImpl::setStyle(CDLStyle style)
{
    const auto curDir = getDirection();
    data().setStyle(CDLOpData::ConvertStyle(style, curDir));
}

void CDLTransformImpl::setSlope(const double * rgb)
{
    if (!rgb)
    {
        throw Exception("CDLTransform: Invalid 'slope' pointer");
    }

    data().setSlopeParams(CDLOpData::ChannelParams(rgb[0], rgb[1], rgb[2]));
}

void CDLTransformImpl::getSlope(double * rgb) const
{
    if (!rgb)
    {
        throw Exception("CDLTransform: Invalid 'slope' pointer");
    }

    const CDLOpData::ChannelParams & params = data().getSlopeParams();
    rgb[0] = params[0];
    rgb[1] = params[1];
    rgb[2] = params[2];
}

void CDLTransformImpl::setOffset(const double * rgb)
{
    if (!rgb)
    {
        throw Exception("CDLTransform: Invalid 'offset' pointer");
    }

    data().setOffsetParams(CDLOpData::ChannelParams(rgb[0], rgb[1], rgb[2]));
}

void CDLTransformImpl::getOffset(double * rgb) const
{
    if (!rgb)
    {
        throw Exception("CDLTransform: Invalid 'offset' pointer");
    }

    const CDLOpData::ChannelParams & params = data().getOffsetParams();
    rgb[0] = params[0];
    rgb[1] = params[1];
    rgb[2] = params[2];
}

void CDLTransformImpl::setPower(const double * rgb)
{
    if (!rgb)
    {
        throw Exception("CDLTransform: Invalid 'power' pointer");
    }

    data().setPowerParams(CDLOpData::ChannelParams(rgb[0], rgb[1], rgb[2]));
}

void CDLTransformImpl::getPower(double * rgb) const
{
    if (!rgb)
    {
        throw Exception("CDLTransform: Invalid 'power' pointer");
    }

    const CDLOpData::ChannelParams & params = data().getPowerParams();
    rgb[0] = params[0];
    rgb[1] = params[1];
    rgb[2] = params[2];
}

void CDLTransformImpl::setSOP(const double * vec9)
{
    if (!vec9)
    {
        throw Exception("CDLTransform: Invalid 'SOP' pointer");
    }

    data().setSlopeParams(CDLOpData::ChannelParams(vec9[0], vec9[1], vec9[2]));
    data().setOffsetParams(CDLOpData::ChannelParams(vec9[3], vec9[4], vec9[5]));
    data().setPowerParams(CDLOpData::ChannelParams(vec9[6], vec9[7], vec9[8]));
}

void CDLTransformImpl::getSOP(double * vec9) const
{
    if (!vec9)
    {
        throw Exception("CDLTransform: Invalid 'SOP' pointer");
    }

    const CDLOpData::ChannelParams & slopes = data().getSlopeParams();
    vec9[0] = slopes[0];
    vec9[1] = slopes[1];
    vec9[2] = slopes[2];

    const CDLOpData::ChannelParams & offsets = data().getOffsetParams();
    vec9[3] = offsets[0];
    vec9[4] = offsets[1];
    vec9[5] = offsets[2];

    const CDLOpData::ChannelParams & powers = data().getPowerParams();
    vec9[6] = powers[0];
    vec9[7] = powers[1];
    vec9[8] = powers[2];
}

void CDLTransformImpl::setSat(double sat)
{
    data().setSaturation(sat);
}

double CDLTransformImpl::getSat() const
{
    return data().getSaturation();
}

void CDLTransformImpl::getSatLumaCoefs(double * rgb) const
{
    if (!rgb)
    {
        throw Exception("CDLTransform: Invalid 'luma' pointer");
    }

    rgb[0] = 0.2126;
    rgb[1] = 0.7152;
    rgb[2] = 0.0722;
}

void CDLTransformImpl::setID(const char * id)
{
    data().setID(id ? id : "");
}

const char * CDLTransformImpl::getID() const
{
    return data().getID().c_str();
}

void CDLTransformImpl::setDescription(const char * desc)
{
    auto & info = data().getFormatMetadata();
    info.getChildrenElements().clear();
    if (desc)
    {
        info.getChildrenElements().emplace_back(METADATA_SOP_DESCRIPTION, desc);
    }
}

const char * CDLTransformImpl::getDescription() const
{
    const auto & info = data().getFormatMetadata();
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

std::ostream & operator<< (std::ostream & os, const CDLTransform & t)
{
    double sop[9];
    t.getSOP(sop);

    os << "<CDLTransform";
    os << " direction=" << TransformDirectionToString(t.getDirection());
    os << ", sop=";
    for (unsigned int i = 0; i < 9; ++i)
    {
        if (i != 0) os << " ";
        os << sop[i];
    }
    os << ", sat=" << t.getSat();
    os << ", style=" << CDLStyleToString(t.getStyle());
    os << ">";
    return os;
}

} // namespace OCIO_NAMESPACE


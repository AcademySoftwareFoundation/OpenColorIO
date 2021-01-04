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
#include "transforms/FileTransform.h"


namespace OCIO_NAMESPACE
{
CDLTransformRcPtr CDLTransform::Create()
{
    return CDLTransformRcPtr(new CDLTransformImpl(), &CDLTransformImpl::deleter);
}

CDLTransformImplRcPtr CDLTransformImpl::Create()
{
    return CDLTransformImplRcPtr(new CDLTransformImpl(), &CDLTransformImpl::deleter);
}


CDLTransformRcPtr GetCDL(GroupTransformRcPtr & group, const std::string & cdlId)
{
    if (cdlId.empty())
    {
        // No cccid, return first cdl.
        const auto numCDL = group->getNumTransforms();
        if (numCDL > 0)
        {
            return OCIO_DYNAMIC_POINTER_CAST<CDLTransform>(group->getTransform(0));
        }
        else
        {
            throw Exception("File contains no CDL.");
        }
    }

    // Try to parse the cccid as a string id.
    for (int i = 0; i < group->getNumTransforms(); ++i)
    {
        auto cdl = OCIO_DYNAMIC_POINTER_CAST<CDLTransform>(group->getTransform(i));
        // Case sensitive.
        const char * id = cdl->getFormatMetadata().getID();
        if (id && *id && cdlId == id)
        {
            return cdl;
        }
    }

    // Try to parse the cccid as an integer index. We want to be strict, so fail if leftover chars
    // in the parse.
    int cdlindex = 0;
    if (StringToInt(&cdlindex, cdlId.c_str(), true))
    {
        int maxindex = group->getNumTransforms() - 1;
        if (cdlindex < 0 || cdlindex > maxindex)
        {
            std::ostringstream os;
            os << "The specified CDL index " << cdlindex;
            os << " is outside the valid range for this file [0,";
            os << maxindex << "]";
            throw ExceptionMissingFile(os.str().c_str());
        }

        return OCIO_DYNAMIC_POINTER_CAST<CDLTransform>(group->getTransform(cdlindex));
    }

    std::ostringstream os;
    os << "The specified CDL Id/Index '" << cdlId;
    os << "' could not be loaded from the file.";
    throw Exception(os.str().c_str());
}

CDLTransformRcPtr CDLTransform::CreateFromFile(const char * src, const char * cdlId_)
{
    if (!src || !*src)
    {
        throw Exception("Error loading CDL. Source file not specified.");
    }

    FileFormat * format = nullptr;
    CachedFileRcPtr cachedFile;

    GetCachedFileAndFormat(format, cachedFile, src, INTERP_DEFAULT);
    GroupTransformRcPtr group = cachedFile->getCDLGroup();

    const std::string cdlId{ cdlId_ ? cdlId_ : "" };
    return GetCDL(group, cdlId);
}

GroupTransformRcPtr CDLTransform::CreateGroupFromFile(const char * src)
{
    if (!src || !*src)
    {
        throw Exception("Error loading CDL. Source file not specified.");
    }

    FileFormat * format = nullptr;
    CachedFileRcPtr cachedFile;

    GetCachedFileAndFormat(format, cachedFile, src, INTERP_DEFAULT);

    return cachedFile->getCDLGroup();
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

const char * CDLTransformImpl::getID() const
{
    return m_data.getID().c_str();
}

void CDLTransformImpl::setID(const char * id)
{
    m_data.setID(id ? id : "");
}

// Note: OCIO v1 supported a single <Description> element as a child of <SOPNode>. In OCIO v2, we
// added support for an arbitrary number of <Description> elements as children of <SOPNode> or
// <SatNode>.  We also added support for <Description>, <InputDescription>, and
// <ViewingDescription> elements as children of <ColorCorrection> (per the ASC CDL spec).  All of
// these elements are also read and written from CLF/CTF files. Those elements are available in the
// CDLTransform's formatMetadata object.  This method provides a simple replacement for the
// getDescription method of OCIO v1 but the name was changed to clarify what it does.
const char * CDLTransformImpl::getFirstSOPDescription() const
{
    const auto & info = data().getFormatMetadata();
    int descIndex = info.getFirstChildIndex(METADATA_SOP_DESCRIPTION);
    if (descIndex == -1)
    {
        return "";
    }
    else
    {
        return info.getChildrenElements()[descIndex].getElementValue();
    }
}

void CDLTransformImpl::setFirstSOPDescription(const char * description)
{
    auto & info = data().getFormatMetadata();
    int descIndex = info.getFirstChildIndex(METADATA_SOP_DESCRIPTION);
    if (descIndex == -1)
    {
        if (description && *description)
        {
            info.getChildrenElements().emplace_back(METADATA_SOP_DESCRIPTION, description);
        }
    }
    else
    {
        if (description && *description)
        {
            info.getChildrenElements()[descIndex].setElementValue(description);
        }
        else
        {
            info.getChildrenElements().erase(info.getChildrenElements().begin() + descIndex);
        }
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


// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "fileformats/cdl/CDLReaderHelper.h"
#include "fileformats/xmlutils/XMLReaderUtils.h"

namespace OCIO_NAMESPACE
{

CDLReaderColorCorrectionElt::CDLReaderColorCorrectionElt(
    const std::string & name,
    ContainerEltRcPtr pParent,
    unsigned xmlLocation,
    const std::string & xmlFile)
    : XmlReaderComplexElt(name, pParent, xmlLocation, xmlFile)
    , m_transformData(std::make_shared<CDLOpData>())
{
}

void CDLReaderColorCorrectionElt::start(const char ** atts)
{
    unsigned i = 0;
    while (atts[i])
    {
        if (0 == strcmp(ATTR_ID, atts[i]))
        {
            if (atts[i + 1])
            {
                // Note: eXpat automatically replaces escaped characters with 
                //       their original values.
                m_transformData->setID(atts[i + 1]);
            }
            else
            {
                throwMessage("Missing attribute value for id");
            }
        }

        i += 2;
    }
}

void CDLReaderColorCorrectionElt::end()
{
    CDLTransformImplRcPtr transform = CDLTransformImpl::Create();

    double vec9[9];
    const CDLOpData::ChannelParams & slopes = m_transformData->getSlopeParams();
    vec9[0] = slopes[0];
    vec9[1] = slopes[1];
    vec9[2] = slopes[2];

    const CDLOpData::ChannelParams & offsets = m_transformData->getOffsetParams();
    vec9[3] = offsets[0];
    vec9[4] = offsets[1];
    vec9[5] = offsets[2];

    const CDLOpData::ChannelParams & powers = m_transformData->getPowerParams();
    vec9[6] = powers[0];
    vec9[7] = powers[1];
    vec9[8] = powers[2];
    transform->setSOP(vec9);

    transform->setSat(m_transformData->getSaturation());

    auto & formatMetadata = transform->getFormatMetadata();
    auto & metadata = dynamic_cast<FormatMetadataImpl &>(formatMetadata);
    metadata = m_transformData->getFormatMetadata();

    transform->validate();

    m_parsingInfo->m_transforms.push_back(transform);
}

void CDLReaderColorCorrectionElt::setCDLParsingInfo(const CDLParsingInfoRcPtr & pTransformList)
{
    m_parsingInfo = pTransformList;
}

void CDLReaderColorCorrectionElt::appendMetadata(const std::string & name, const std::string & value)
{
    // Keeps description as metadata with supplied name.
    FormatMetadataImpl item(name, value);
    m_transformData->getFormatMetadata().getChildrenElements().push_back(item);
}

} // namespace OCIO_NAMESPACE

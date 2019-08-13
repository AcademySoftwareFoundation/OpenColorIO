// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "fileformats/cdl/CDLReaderHelper.h"
#include "fileformats/xmlutils/XMLReaderUtils.h"

OCIO_NAMESPACE_ENTER
{

CDLReaderColorCorrectionElt::CDLReaderColorCorrectionElt(
    const std::string & name,
    ContainerEltRcPtr pParent,
    unsigned xmlLocation,
    const std::string & xmlFile)
    : XmlReaderComplexElt(name, pParent, xmlLocation, xmlFile)
    , m_transformList()
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
    CDLTransformRcPtr transform = CDLTransform::Create();

    transform->setID(m_transformData->getID().c_str());

    OpData::Descriptions & desc = m_transformData->getDescriptions();
    if (!desc.empty())
    {
        transform->setDescription(desc[0].c_str());
    }
    
    float vec9[9];
    const CDLOpData::ChannelParams & slopes = m_transformData->getSlopeParams();
    vec9[0] = (float)slopes[0];
    vec9[1] = (float)slopes[1];
    vec9[2] = (float)slopes[2];

    const CDLOpData::ChannelParams & offsets = m_transformData->getOffsetParams();
    vec9[3] = (float)offsets[0];
    vec9[4] = (float)offsets[1];
    vec9[5] = (float)offsets[2];

    const CDLOpData::ChannelParams & powers = m_transformData->getPowerParams();
    vec9[6] = (float)powers[0];
    vec9[7] = (float)powers[1];
    vec9[8] = (float)powers[2];
    transform->setSOP(vec9);

    transform->setSat((float)m_transformData->getSaturation());

    transform->validate();
    m_transformList->push_back(transform);
}

void CDLReaderColorCorrectionElt::setCDLTransformList(CDLTransformVecRcPtr pTransformList)
{
    m_transformList = pTransformList;
}

void CDLReaderColorCorrectionElt::appendDescription(const std::string & /*desc*/)
{
    // TODO: OCIO only keeps the description on the SOP.
    //m_transform->setDescription(desc.c_str());
}


}
OCIO_NAMESPACE_EXIT


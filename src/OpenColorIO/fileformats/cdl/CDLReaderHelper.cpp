/*
Copyright (c) 2018 Autodesk Inc., et al.
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


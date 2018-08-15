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

#include "CTFCDLElt.h"
#include "../opdata/OpDataCDL.h"
#include "../Platform.h"
#include "CTFReaderUtils.h"

OCIO_NAMESPACE_ENTER
{
// Private namespace to the OpData sub-directory
namespace CTF
{
// Private namespace for the xml reader utils
namespace Reader
{

CDLElt::CDLElt()
    : OpElt()
    , m_cdlOp(new OpData::CDL)
{
    // CDL op is already initialized to identity
}

CDLElt::~CDLElt()
{
    // Do not delete m_cdlOp
    m_cdlOp = 0x0;
}

void CDLElt::start(const char **atts)
{
    OpElt::start(atts);

    bool isStyleFound = false;
    for (unsigned i = 0; atts[i]; i += 2)
    {
        if (0 == Platform::Strcasecmp(ATTR_CDL_STYLE, atts[i]))
        {
            // Unrecognized CDL styles will throw an exception
            m_cdlOp->setCDLStyle(OpData::CDL::GetCDLStyle(atts[i + 1]));
            isStyleFound = true;
        }
    }

    if (!isStyleFound)
    {
        Throw("CTF CDL parsing. Required attribute 'style' is missing. ");
    }
}

void CDLElt::end()
{
    OpElt::end();

    // Validate the end result
    m_cdlOp->validate();
}

OpData::OpData* CDLElt::getOp() const
{
    return m_cdlOp;
}

CDLElt_1_7::CDLElt_1_7()
    : CDLElt()
{
}

CDLElt_1_7::~CDLElt_1_7()
{
}

void CDLElt_1_7::start(const char **atts)
{
    OpElt::start(atts);

    bool isStyleFound = false;
    for (unsigned i = 0; atts[i]; i += 2)
    {
        if (0 == Platform::Strcasecmp(ATTR_CDL_STYLE, atts[i]))
        {
            // Translate CLF styles into CTF styles.
            if (0 == Platform::Strcasecmp("Fwd", atts[i + 1]))
            {
                m_cdlOp->setCDLStyle(OpData::CDL::CDL_V1_2_FWD);
                isStyleFound = true;
            }
            else if (0 == Platform::Strcasecmp("Rev", atts[i + 1]))
            {
                m_cdlOp->setCDLStyle(OpData::CDL::CDL_V1_2_REV);
                isStyleFound = true;
            }
            else if (0 == Platform::Strcasecmp("FwdNoClamp", atts[i + 1]))
            {
                m_cdlOp->setCDLStyle(OpData::CDL::CDL_NO_CLAMP_FWD);
                isStyleFound = true;
            }
            else if (0 == Platform::Strcasecmp("RevNoClamp", atts[i + 1]))
            {
                m_cdlOp->setCDLStyle(OpData::CDL::CDL_NO_CLAMP_REV);
                isStyleFound = true;
            }

            // Otherwise try interpreting as a CTF style.
            else
            {
                // Unrecognized CDL styles will throw an exception
                m_cdlOp->setCDLStyle(OpData::CDL::GetCDLStyle(atts[i + 1]));
                isStyleFound = true;
            }
        }
    }

    if (!isStyleFound)
    {
        Throw("CTF CDL parsing. Required attribute 'style' is missing. ");
    }
}

SOPNodeBaseElt::SOPNodeBaseElt(const std::string& name,
                                ContainerElt* pParent,
                                unsigned xmlLineNumber,
                                const std::string& xmlFile)
    : ComplexElt(name, pParent, xmlLineNumber, xmlFile)
    , m_isSlopeInit(false)
    , m_isOffsetInit(false)
    , m_isPowerInit(false)
{
}

void SOPNodeBaseElt::start(const char**)
{
    m_isSlopeInit = m_isOffsetInit = m_isPowerInit = false;
}

void SOPNodeBaseElt::end()
{
    if (!m_isSlopeInit)
    {
        Throw("CTF CDL parsing. Required node 'Slope' is missing. ");
    }

    if (!m_isOffsetInit)
    {
        Throw("CTF CDL parsing. Required node 'Offset' is missing. ");
    }

    if (!m_isPowerInit)
    {
        Throw("CTF CDL parsing. Required node 'Power' is missing. ");
    }
}

void SOPNodeBaseElt::appendDescription(const std::string& desc)
{
    getCDLOp()->getDescriptions() += desc;
}

SOPNodeElt::SOPNodeElt(const std::string& name,
                        ContainerElt* pParent,
                        unsigned xmlLineNumber,
                        const std::string& xmlFile)
    : SOPNodeBaseElt(name, pParent, xmlLineNumber, xmlFile)
{
}

OpData::CDL* SOPNodeElt::getCDLOp() const
{
    return static_cast<CDLElt*>(getParent())->getCDLOp();
}

SOPValueElt::SOPValueElt(const std::string& name,
                            ContainerElt* pParent,
                            unsigned xmlLineNumber,
                            const std::string& xmlFile)
    : PlainElt(name, pParent, xmlLineNumber, xmlFile)
{
}

SOPValueElt::~SOPValueElt()
{
}

void SOPValueElt::start(const char**)
{
    m_contentData = "";
}

void SOPValueElt::end()
{
    Trim(m_contentData);

    std::vector<double> data;

    try
    {
        data = GetNumbers<double>(m_contentData.c_str(), m_contentData.size());
    }
    catch (Exception&)
    {
        const std::string s = TruncateString(m_contentData.c_str(), m_contentData.size());
        std::ostringstream oss;
        oss << "Illegal values '";
        oss << s;
        oss << "' in ";
        oss << getTypeName();
        Throw(oss.str());
    }

    if (data.size() != 3)
    {
        Throw("SOPNode: 3 values required.");
    }

    SOPNodeBaseElt* pSOPNodeElt = dynamic_cast<SOPNodeBaseElt*>(getParent());
    OpData::CDL* pCDL = pSOPNodeElt->getCDLOp();

    if (0 == Platform::Strcasecmp(getName().c_str(), CTF::TAG_SLOPE))
    {
        pCDL->setSlopeParams(OpData::CDL::ChannelParams(data[0], data[1], data[2]));
        pSOPNodeElt->setIsSlopeInit(true);
    }
    else if (0 == Platform::Strcasecmp(getName().c_str(), CTF::TAG_OFFSET))
    {
        pCDL->setOffsetParams(OpData::CDL::ChannelParams(data[0], data[1], data[2]));
        pSOPNodeElt->setIsOffsetInit(true);
    }
    else if (0 == Platform::Strcasecmp(getName().c_str(), CTF::TAG_POWER))
    {
        pCDL->setPowerParams(OpData::CDL::ChannelParams(data[0], data[1], data[2]));
        pSOPNodeElt->setIsPowerInit(true);
    }
}

void SOPValueElt::setRawData(const char* str, size_t len, unsigned)
{
    m_contentData += std::string(str, len) + " ";
}


SatNodeBaseElt::SatNodeBaseElt(const std::string& name,
                                ContainerElt* pParent,
                                unsigned xmlLineNumber,
                                const std::string& xmlFile)
    : ComplexElt(name, pParent, xmlLineNumber, xmlFile)
{
}

void SatNodeBaseElt::start(const char**)
{
}

void SatNodeBaseElt::end()
{
}

void SatNodeBaseElt::appendDescription(const std::string& desc)
{
    getCDLOp()->getDescriptions() += desc;
}


SatNodeElt::SatNodeElt(const std::string& name,
                        ContainerElt* pParent,
                        unsigned xmlLineNumber,
                        const std::string& xmlFile)
    : SatNodeBaseElt(name, pParent, xmlLineNumber, xmlFile)
{
}

OpData::CDL* SatNodeElt::getCDLOp() const
{
    return static_cast<CDLElt*>(getParent())->getCDLOp();
}


SaturationElt::SaturationElt(const std::string& name,
                                ContainerElt* pParent,
                                unsigned xmlLineNumber,
                                const std::string& xmlFile)
    : PlainElt(name, pParent, xmlLineNumber, xmlFile)
{
}

SaturationElt::~SaturationElt()
{
}

void SaturationElt::start(const char** /* atts */)
{
    m_contentData = "";
}

void SaturationElt::end()
{
    Trim(m_contentData);

    std::vector<double> data;

    try
    {
        data = GetNumbers<double>(m_contentData.c_str(), m_contentData.size());
    }
    catch (Exception& /* ce */)
    {
        const std::string s = TruncateString(m_contentData.c_str(), m_contentData.size());
        std::ostringstream oss;
        oss << "Illegal values '";
        oss << s;
        oss << "' in ";
        oss << getTypeName();
        Throw(oss.str());
    }

    if (data.size() != 1)
    {
        Throw("SatNode: non-single value. ");
    }

    SatNodeBaseElt* pSatNodeElt = dynamic_cast<SatNodeBaseElt*>(getParent());
    OpData::CDL* pCDL = pSatNodeElt->getCDLOp();

    if (0 == Platform::Strcasecmp(getName().c_str(), CTF::TAG_SATURATION))
    {
        pCDL->setSaturation(data[0]);
    }
}

void SaturationElt::setRawData(const char* str, size_t len, unsigned)
{
    m_contentData += std::string(str, len) + " ";
}


} // exit Reader namespace
} // exit CTF namespace
}
OCIO_NAMESPACE_EXIT


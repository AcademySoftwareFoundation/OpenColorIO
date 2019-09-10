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

#ifndef INCLUDED_OCIO_FILEFORMATS_CDL_CDLREADERHELPER_H
#define INCLUDED_OCIO_FILEFORMATS_CDL_CDLREADERHELPER_H

#include "fileformats/xmlutils/XMLReaderHelper.h"
#include "Op.h"
#include "transforms/CDLTransform.h"

OCIO_NAMESPACE_ENTER
{

class CDLParsingInfo
{
public:
    CDLParsingInfo()
        : m_metadata(METADATA_ROOT)
    {
    }

    CDLTransformVec m_transforms;
    FormatMetadataImpl m_metadata;
};
typedef OCIO_SHARED_PTR<CDLParsingInfo> CDLParsingInfoRcPtr;

class CDLReaderColorDecisionListElt : public XmlReaderContainerElt
{
public:
    CDLReaderColorDecisionListElt(const std::string & name,
                                  unsigned int xmlLineNumber,
                                  const std::string & xmlFile)
        : XmlReaderContainerElt(name, xmlLineNumber, xmlFile)
        , m_parsingInfo(std::make_shared<CDLParsingInfo>())
    {
    }

    void start(const char ** /*atts*/) override {}

    void end() override {}

    const std::string & getIdentifier() const override
    {
        return getName();
    }

    const char * getTypeName() const override
    {
        return getName().c_str();
    }

    const CDLParsingInfoRcPtr & getCDLParsingInfo() const
    {
        return m_parsingInfo;
    }

    void appendMetadata(const std::string & name, const std::string & value) override
    {
        m_parsingInfo->m_metadata.addChildElement(name.c_str(), value.c_str());
    }

    const FormatMetadataImpl & getMetadata() const
    {
        return m_parsingInfo->m_metadata;
    }

private:
    CDLParsingInfoRcPtr m_parsingInfo;

};

class CDLReaderColorDecisionElt : public XmlReaderComplexElt
{
public:
    CDLReaderColorDecisionElt(const std::string & name,
                              ContainerEltRcPtr pParent,
                              unsigned int xmlLineNumber,
                              const std::string & xmlFile)
        : XmlReaderComplexElt(name, pParent, xmlLineNumber, xmlFile)
        , m_metadata(METADATA_ROOT)
    {
    }

    void start(const char ** /*atts*/) override
    {
    }

    virtual void end() override
    {
    }

    void appendMetadata(const std::string & name, const std::string & value) override
    {
        m_metadata.addChildElement(name.c_str(), value.c_str());
    }


    const FormatMetadataImpl & getMetadata() const
    {
        return m_metadata;
    }

private:
    FormatMetadataImpl m_metadata;
};

class CDLReaderColorCorrectionCollectionElt : public XmlReaderContainerElt
{
public:
    CDLReaderColorCorrectionCollectionElt(const std::string & name,
                                          unsigned int xmlLineNumber,
                                          const std::string & xmlFile)
        : XmlReaderContainerElt(name, xmlLineNumber, xmlFile)
        , m_parsingInfo(std::make_shared<CDLParsingInfo>())
    {
    }

    void start(const char ** /*atts*/) override
    {
    }

    void end() override
    {
    }

    const std::string & getIdentifier() const override
    {
        return getName();
    }

    const char * getTypeName() const override
    {
        return getName().c_str();
    }

    const CDLParsingInfoRcPtr & getCDLParsingInfo() const
    {
        return m_parsingInfo;
    }

    void appendMetadata(const std::string & name, const std::string & value) override
    {
        m_parsingInfo->m_metadata.addChildElement(name.c_str(), value.c_str());
    }

    const FormatMetadataImpl & getMetadata() const
    {
        return m_parsingInfo->m_metadata;
    }

private:
    CDLParsingInfoRcPtr m_parsingInfo;

};

class CDLReaderColorCorrectionElt : public XmlReaderComplexElt
{
public:
    CDLReaderColorCorrectionElt(const std::string & name,
                                ContainerEltRcPtr pParent,
                                unsigned int xmlLocation,
                                const std::string & xmlFile);

    void start(const char ** atts) override;

    void end() override;

    const CDLOpDataRcPtr & getCDL() const { return m_transformData; }

    void setCDLParsingInfo(const CDLParsingInfoRcPtr & parsingInfo);

    void appendMetadata(const std::string & name, const std::string & value) override;

private:
    CDLParsingInfoRcPtr m_parsingInfo;
    CDLOpDataRcPtr m_transformData;
};

// Class for the SOPNode element in the CDL/CCC/CC schemas.
class CDLReaderSOPNodeCCElt : public XmlReaderSOPNodeBaseElt
{
public:
    CDLReaderSOPNodeCCElt(const std::string & name,
                          ContainerEltRcPtr pParent,
                          unsigned int xmlLocation,
                          const std::string & xmlFile)
        : XmlReaderSOPNodeBaseElt(name, pParent, xmlLocation, xmlFile)
    {
    }

    const CDLOpDataRcPtr & getCDL() const override
    {
        return static_cast<CDLReaderColorCorrectionElt*>(getParent().get())->getCDL();
    }
};

// Class for the SatNode element in the CDL/CCC/CC schemas.
class CDLReaderSatNodeCCElt : public XmlReaderSatNodeBaseElt
{
public:
    CDLReaderSatNodeCCElt(const std::string & name,
                          ContainerEltRcPtr pParent,
                          unsigned int xmlLineNumber,
                          const std::string & xmlFile)
        : XmlReaderSatNodeBaseElt(name, pParent, xmlLineNumber, xmlFile)
    {
    }

    const CDLOpDataRcPtr & getCDL() const override
    {
        return static_cast<CDLReaderColorCorrectionElt*>(getParent().get())->getCDL();
    }
};

}
OCIO_NAMESPACE_EXIT

#endif
// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_FILEFORMATS_CDL_CDLREADERHELPER_H
#define INCLUDED_OCIO_FILEFORMATS_CDL_CDLREADERHELPER_H

#include "fileformats/xmlutils/XMLReaderHelper.h"
#include "Op.h"
#include "transforms/CDLTransform.h"

namespace OCIO_NAMESPACE
{

class CDLParsingInfo
{
public:
    CDLParsingInfo()
        : m_metadata()
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
        , m_metadata()
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

} // namespace OCIO_NAMESPACE

#endif
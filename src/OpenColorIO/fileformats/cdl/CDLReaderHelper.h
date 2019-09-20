// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_FILEFORMATS_CDL_CDLREADERHELPER_H
#define INCLUDED_OCIO_FILEFORMATS_CDL_CDLREADERHELPER_H

#include "fileformats/xmlutils/XMLReaderHelper.h"
#include "Op.h"
#include "transforms/CDLTransform.h"

OCIO_NAMESPACE_ENTER
{

typedef OCIO_SHARED_PTR<CDLTransformVec> CDLTransformVecRcPtr;

class CDLReaderColorDecisionListElt : public XmlReaderContainerElt
{
public:
    CDLReaderColorDecisionListElt(const std::string & name,
                                  unsigned int xmlLineNumber,
                                  const std::string & xmlFile)
        : XmlReaderContainerElt(name, xmlLineNumber, xmlFile)
        , m_transformList(new CDLTransformVec)
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

    const CDLTransformVecRcPtr & getCDLTransformList() const
    {
        return m_transformList;
    }

    void appendDescription(const std::string & desc) override
    {
        m_descriptions.push_back(desc);
    }

    const StringVec & getDescriptions() const
    {
        return m_descriptions;
    }

private:
    CDLTransformVecRcPtr m_transformList;
    StringVec m_descriptions;

};

class CDLReaderColorDecisionElt : public XmlReaderComplexElt
{
public:
    CDLReaderColorDecisionElt(const std::string & name,
                              ContainerEltRcPtr pParent,
                              unsigned int xmlLineNumber,
                              const std::string & xmlFile)
        : XmlReaderComplexElt(name, pParent, xmlLineNumber, xmlFile)
    {
    }

    void start(const char ** /*atts*/) override
    {
    }

    virtual void end() override
    {
    }

    void appendDescription(const std::string & desc) override
    {
        m_descriptions.push_back(desc);
    }


    const StringVec & getDescriptions() const
    {
        return m_descriptions;
    }

private:
    StringVec m_descriptions;
};

class CDLReaderColorCorrectionCollectionElt : public XmlReaderContainerElt
{
public:
    CDLReaderColorCorrectionCollectionElt(const std::string & name,
                                          unsigned int xmlLineNumber,
                                          const std::string & xmlFile)
        : XmlReaderContainerElt(name, xmlLineNumber, xmlFile)
        , m_transformList(new CDLTransformVec)
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

    const CDLTransformVecRcPtr & getCDLTransformList() const
    {
        return m_transformList;
    }

    void appendDescription(const std::string & desc) override
    {
        m_descriptions.push_back(desc);
    }

    const StringVec & getDescriptions() const
    {
        return m_descriptions;
    }

private:
    CDLTransformVecRcPtr m_transformList;
    StringVec m_descriptions;

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

    void setCDLTransformList(CDLTransformVecRcPtr pTransformList);

    void appendDescription(const std::string & desc) override;

private:
    CDLTransformVecRcPtr m_transformList;
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
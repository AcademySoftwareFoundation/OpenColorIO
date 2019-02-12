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
        m_descriptions += desc;
    }

    const OpData::Descriptions & getDescriptions() const
    {
        return m_descriptions;
    }

private:
    CDLTransformVecRcPtr m_transformList;
    OpData::Descriptions m_descriptions;

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
        m_descriptions += desc;
    }


    const OpData::Descriptions & getDescriptions() const
    {
        return m_descriptions;
    }

private:
    OpData::Descriptions m_descriptions;
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
        m_descriptions += desc;
    }

    const OpData::Descriptions & getDescriptions() const
    {
        return m_descriptions;
    }

private:
    CDLTransformVecRcPtr m_transformList;
    OpData::Descriptions m_descriptions;

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
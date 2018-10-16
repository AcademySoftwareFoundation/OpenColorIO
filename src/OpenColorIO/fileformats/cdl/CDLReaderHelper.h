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

#include <string>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "transforms/CDLTransform.h"

OCIO_NAMESPACE_ENTER
{

extern const char TAG_SLOPE[];
extern const char TAG_OFFSET[];
extern const char TAG_POWER[];
extern const char TAG_SATURATION[];

void FindSubString(const char* str, size_t length,
                   size_t& start,
                   size_t& end);

// Class containing one or more human readable descriptions
class Descriptions
{
public:
    typedef std::vector<std::string> List;

public:
    Descriptions()
    {
    }

    ~Descriptions()
    {
    }

    Descriptions& operator=(const Descriptions& rhs)
    {
        if (this == &rhs) return *this;

        m_descriptions = rhs.m_descriptions;
        return *this;
    }

    Descriptions& operator +=(const Descriptions& d)
    {
        if (this != &d)
        {
            m_descriptions.insert(m_descriptions.end(),
                                  d.m_descriptions.begin(),
                                  d.m_descriptions.end());
        }
        return *this;
    }

    Descriptions& operator +=(const std::string& d)
    {
        m_descriptions.push_back(d);
        return *this;
    }

    bool operator==(const Descriptions& rhs) const
    {
        if (this == &rhs) return true;

        return (m_descriptions == rhs.m_descriptions);
    }

    const List& getList() const
    {
        return m_descriptions;
    }

private:
    List m_descriptions;
};

// Base class for all elements possible for parsing XML
class XmlReaderElement
{
public:
    XmlReaderElement(const std::string& name,
                     unsigned int xmlLineNumber,
                     const std::string& xmlFile);

    virtual ~XmlReaderElement();

    // Start the parsing of the element
    virtual void start(const char **atts) = 0;

    // End the parsing of the element
    virtual void end() = 0;

    // Is it a container which means if it can hold other elements
    virtual bool isContainer() const = 0;

    const std::string& getName() const
    {
        return m_name;
    }

    virtual const std::string& getIdentifier() const = 0;

    unsigned getXmLineNumber() const
    {
        return m_xmlLineNumber;
    }

    const std::string& getXmlFile() const;

    virtual const std::string& getTypeName() const = 0;

    // Set the element context
    void setContext(const std::string& name,
                    unsigned int xmlLineNumber,
                    const std::string& xmlFile);

    virtual bool isDummy() const
    {
        return false;
    }

protected:
    void throwMessage(const std::string & error) const;


private:
    std::string m_name;       // The name
    unsigned m_xmlLineNumber; // The location
    std::string m_xmlFile;    // The xml file

private:
    XmlReaderElement() = delete;
    XmlReaderElement(const XmlReaderElement&) = delete;
    XmlReaderElement& operator=(const XmlReaderElement&) = delete;
};

typedef OCIO_SHARED_PTR<XmlReaderElement> ElementRcPtr;
typedef OCIO_SHARED_PTR<const XmlReaderElement> ConstElementRcPtr;

// Base class for element that could contain sub-elements
class XmlReaderContainerElt : public XmlReaderElement
{
public:
    XmlReaderContainerElt(const std::string& name,
                          unsigned int xmlLineNumber,
                          const std::string& xmlFile)
        : XmlReaderElement(name, xmlLineNumber, xmlFile)
    {
    }

    // Destructor
    virtual ~XmlReaderContainerElt()
    {
    }

    // Is it a container which means if it can hold other elements
    bool isContainer() const
    {
        return true;
    }

    virtual void appendDescription(const std::string& desc) = 0;

private:
    XmlReaderContainerElt() = delete;

};

typedef OCIO_SHARED_PTR<XmlReaderContainerElt> ContainerEltRcPtr;

// Base class for all basic elements
class XmlReaderPlainElt : public XmlReaderElement
{
public:
    XmlReaderPlainElt(const std::string& name,
                      ContainerEltRcPtr pParent,
                      unsigned int xmlLineNumber,
                      const std::string& xmlFile)
        : XmlReaderElement(name, xmlLineNumber, xmlFile)
        , m_parent(pParent)
    {
    }

    // Destructor
    ~XmlReaderPlainElt()
    {
    }

    // Set the data's element
    virtual void setRawData(const char* str,
                            size_t len,
                            unsigned int xmlLine ) = 0;

    // Is it a container which means if it can hold other elements
    bool isContainer() const
    {
        return false;
    }

    ContainerEltRcPtr getParent() const
    {
        return m_parent;
    }

    const std::string& getIdentifier() const
    {
        return getName();
    }

    const std::string& getTypeName() const
    {
        return getName();
    }

private:
    XmlReaderPlainElt() = delete;

    // The element's parent
    ContainerEltRcPtr m_parent;
};

// Class Dummy to address unknwon Color::Utils::Element
class XmlReaderDummyElt : public XmlReaderPlainElt
{
    // Class Dummy to address unexpected parent for the DummyElt
    class DummyParent : public XmlReaderContainerElt
    {
    public:
        DummyParent(ConstElementRcPtr pParent)
            : XmlReaderContainerElt(pParent.get() ? pParent->getName() : "",
                                    pParent.get() ? pParent->getXmLineNumber() : 0,
                                    pParent.get() ? pParent->getXmlFile() : "")
        {
        }
        ~DummyParent()
        {
        }

        void appendDescription(const std::string& desc)
        {
        }

        const std::string& getIdentifier() const;

        void start(const char **atts)
        {
        }

        void end()
        {
        }

        const std::string& getTypeName() const
        {
            return getIdentifier();
        }
    };

public:
    XmlReaderDummyElt(const std::string& name,
                      ConstElementRcPtr pParent,
                      unsigned xmlLocation,
                      const std::string& xmlFile,
                      const char* msg);

    virtual ~XmlReaderDummyElt()
    {
    }

    const std::string& getIdentifier() const;

    void start(const char **atts)
    {
    }

    void end()
    {
    }

    void setRawData(const char* str, size_t len, unsigned xmlLine)
    {
        m_rawData.push_back(std::string(str, len));
    }

    virtual bool isDummy() const
    {
        return true;
    }

private:
    std::vector<std::string> m_rawData;
};

typedef OCIO_SHARED_PTR<XmlReaderDummyElt> DummyEltRcPtr;

// Class for the description's element
class XmlReaderDescriptionElt : public XmlReaderPlainElt
{
public:
    XmlReaderDescriptionElt(const std::string& name,
                            ContainerEltRcPtr pParent,
                            unsigned xmlLocation,
                            const std::string& xmlFile)
        : XmlReaderPlainElt(name, pParent, xmlLocation, xmlFile)
        , m_changed(false)
    {
    }

    ~XmlReaderDescriptionElt()
    {
    }

    void start(const char **atts)
    {
        m_description.resize(0);
        m_changed = false;
    }

    void end();

    void setRawData(const char* str, size_t len, unsigned xmlLine)
    {
        // keep adding to the string
        m_description += std::string(str, len);
        m_changed = true;
    }

private:
    std::string m_description;
    // true if the description was changed when reading
    bool m_changed;

};

// Base class for nested elements
class XmlReaderComplexElt : public XmlReaderContainerElt
{
public:
    XmlReaderComplexElt(const std::string& name,
                        ContainerEltRcPtr pParent,
                        unsigned int xmlLineNumber,
                        const std::string& xmlFile)
        : XmlReaderContainerElt(name, xmlLineNumber, xmlFile)
        , m_parent(pParent)
    {
    }

    ~XmlReaderComplexElt()
    {
    }

    ContainerEltRcPtr getParent() const
    {
        return m_parent;
    }

    const std::string& getIdentifier() const
    {
        return getName();
    }

    const std::string& getTypeName() const
    {
        return getName();
    }

    void appendDescription(const std::string& desc)
    {
    }

private:
    XmlReaderComplexElt() = delete;

    ContainerEltRcPtr m_parent;
};

typedef OCIO_SHARED_PTR<CDLTransformVec> CDLTransformVecRcPtr;

class XmlReaderColorDecisionListElt : public XmlReaderContainerElt
{
public:
    XmlReaderColorDecisionListElt(const std::string& name,
                                  unsigned int xmlLineNumber,
                                  const std::string& xmlFile)
        : XmlReaderContainerElt(name, xmlLineNumber, xmlFile)
        , m_transformList(new CDLTransformVec)
    {
    }

    void start(const char **atts) {}

    void end() {}

    const std::string& getIdentifier() const
    {
        return getName();
    }

    const std::string& getTypeName() const
    {
        return getName();
    }

    const CDLTransformVecRcPtr& getCDLTransformList() const
    {
        return m_transformList;
    }

    void appendDescription(const std::string& desc)
    {
        m_descriptions += desc;
    }

    const Descriptions& getDescriptions() const
    { 
        return m_descriptions;
    }

private:
    CDLTransformVecRcPtr m_transformList;
    Descriptions m_descriptions;

};

class XmlReaderColorDecisionElt : public XmlReaderComplexElt
{
public:
    XmlReaderColorDecisionElt(const std::string& name,
                              ContainerEltRcPtr pParent,
                              unsigned int xmlLineNumber,
                              const std::string& xmlFile)
        : XmlReaderComplexElt(name, pParent, xmlLineNumber, xmlFile)
    {
    }

    virtual void start(const char **atts)
    {
    }

    virtual void end()
    {
    }

    void appendDescription(const std::string& desc)
    {
        m_descriptions += desc;
    }


    const Descriptions& getDescriptions() const
    {
        return m_descriptions;
    }

private:
    Descriptions m_descriptions;
};

class XmlReaderColorCorrectionCollectionElt : public XmlReaderContainerElt
{
public:
    XmlReaderColorCorrectionCollectionElt(const std::string& name,
                                          unsigned int xmlLineNumber,
                                          const std::string& xmlFile)
        : XmlReaderContainerElt(name, xmlLineNumber, xmlFile)
        , m_transformList(new CDLTransformVec)
    {
    }

    void start(const char **atts)
    {
    }

    void end()
    {
    }

    const std::string& getIdentifier() const
    {
        return getName();
    }

    const std::string& getTypeName() const
    {
        return getName();
    }

    const CDLTransformVecRcPtr& getCDLTransformList() const
    {
        return m_transformList;
    }

    void appendDescription(const std::string& desc)
    {
        m_descriptions += desc;
    }

    const Descriptions& getDescriptions() const
    {
        return m_descriptions;
    }

private:
    CDLTransformVecRcPtr m_transformList;
    Descriptions m_descriptions;

};

class XmlReaderColorCorrectionElt : public XmlReaderComplexElt
{
public:
    //! Constructor
    XmlReaderColorCorrectionElt(const std::string& name,
                                ContainerEltRcPtr pParent,
                                unsigned int xmlLocation,
                                const std::string& xmlFile);

    virtual void start(const char **atts);

    virtual void end();

    CDLTransformRcPtr getCDL() const { return m_transform; }

    void setCDLTransformList(CDLTransformVecRcPtr pTransformList);

    void appendDescription(const std::string& desc);

private:
    CDLTransformVecRcPtr m_transformList;
    CDLTransformRcPtr m_transform;
};

class XmlReaderSOPNodeBaseElt : public XmlReaderComplexElt
{
public:
    XmlReaderSOPNodeBaseElt(const std::string& name,
                            ContainerEltRcPtr pParent,
                            unsigned int xmlLineNumber,
                            const std::string& xmlFile)
        : XmlReaderComplexElt(name, pParent, xmlLineNumber, xmlFile)
        , m_isSlopeInit(false)
        , m_isOffsetInit(false)
        , m_isPowerInit(false)
    {
    }

    void start(const char **atts)
    {
        m_isSlopeInit = m_isOffsetInit = m_isPowerInit = false;
    }

    void end()
    {
        if (!m_isSlopeInit)
        {
            throwMessage("CTF CDL parsing. Required node 'Slope' is missing. ");
        }

        if (!m_isOffsetInit)
        {
            throwMessage("CTF CDL parsing. Required node 'Offset' is missing. ");
        }

        if (!m_isPowerInit)
        {
            throwMessage("CTF CDL parsing. Required node 'Power' is missing. ");
        }
    }

    // Get the associated CDL
    virtual CDLTransformRcPtr getCDL() const = 0;

    void setIsSlopeInit(bool status) { m_isSlopeInit = status; }
    void setIsOffsetInit(bool status) { m_isOffsetInit = status; }
    void setIsPowerInit(bool status) { m_isPowerInit = status; }

    void appendDescription(const std::string& desc)
    {
        // TODO: OCIO only keep the first description
        const std::string curDesc(getCDL()->getDescription());
        if (curDesc.empty())
        {
            getCDL()->setDescription(desc.c_str());
        }
    }


private:
    XmlReaderSOPNodeBaseElt() = delete;

    bool m_isSlopeInit;
    bool m_isOffsetInit;
    bool m_isPowerInit;
};

// Class for the SOPNode element in the CDL/CCC/CC schemas
class XmlReaderSOPNodeCCElt : public XmlReaderSOPNodeBaseElt
{
public:
    XmlReaderSOPNodeCCElt(const std::string& name,
                          ContainerEltRcPtr pParent,
                          unsigned int xmlLocation,
                          const std::string& xmlFile)
        : XmlReaderSOPNodeBaseElt(name, pParent, xmlLocation, xmlFile)
    {
    }

    virtual CDLTransformRcPtr getCDL() const
    {
        return static_cast<XmlReaderColorCorrectionElt*>(getParent().get())->getCDL();
    }
};

// Base class for the SatNode element
class XmlReaderSatNodeBaseElt : public XmlReaderComplexElt
{
public:
    XmlReaderSatNodeBaseElt(const std::string& name,
                            ContainerEltRcPtr pParent,
                            unsigned int xmlLineNumber,
                            const std::string& xmlFile)
        : XmlReaderComplexElt(name, pParent, xmlLineNumber, xmlFile)
    {
    }

    void start(const char **atts)
    {
    }

    void end()
    {
    }

    // Get the associated CDL
    virtual CDLTransformRcPtr getCDL() const = 0;

private:
    XmlReaderSatNodeBaseElt() = delete;
};

// Class for the SatNode element in the CDL/CCC/CC schemas
class XmlReaderSatNodeCCElt : public XmlReaderSatNodeBaseElt
{
public:
    //! Constructor
    XmlReaderSatNodeCCElt(const std::string& name,
                          ContainerEltRcPtr pParent,
                          unsigned int xmlLineNumber,
                          const std::string& xmlFile)
        : XmlReaderSatNodeBaseElt(name, pParent, xmlLineNumber, xmlFile)
    {
    }

    virtual CDLTransformRcPtr getCDL() const
    {
        return static_cast<XmlReaderColorCorrectionElt*>(getParent().get())->getCDL();
    }
};

// Class for the slope, offset and power elements
class XmlReaderSOPValueElt : public XmlReaderPlainElt
{
public:
    XmlReaderSOPValueElt(const std::string& name,
                         ContainerEltRcPtr pParent,
                         unsigned int xmlLineNumber,
                         const std::string& xmlFile);

    ~XmlReaderSOPValueElt();

    void start(const char **atts);

    void end();

    void setRawData(const char* str, size_t len, unsigned int xmlLine);

private:
    XmlReaderSOPValueElt() = delete;

    std::string m_contentData; // The tag content data
};

// Class for the CDL Saturation element
class XmlReaderSaturationElt : public XmlReaderPlainElt
{
public:
    XmlReaderSaturationElt(const std::string& name,
                           ContainerEltRcPtr pParent,
                           unsigned int xmlLineNumber,
                           const std::string& xmlFile);

    // Destructor
    ~XmlReaderSaturationElt();

    void start(const char **atts);

    void end();

    void setRawData(const char* str, size_t len, unsigned int xmlLine);

private:
    XmlReaderSaturationElt() = delete;

    std::string m_contentData; // The tag content data
};


// Stack of elements
class XmlReaderElementStack
{
    typedef std::vector<ElementRcPtr> Stack;

public:
    XmlReaderElementStack();

    virtual ~XmlReaderElementStack();

    std::string dump() const;

    bool empty() const;

    unsigned size() const;

    void push_back(ElementRcPtr pElt);

    void pop_back();

    ElementRcPtr back() const;

    ElementRcPtr front() const;

    void clear();

private:
    Stack m_elms; // The list of elements
};

}
OCIO_NAMESPACE_EXIT

#endif

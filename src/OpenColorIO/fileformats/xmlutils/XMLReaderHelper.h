// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_FILEFORMATS_XMLUTILS_XMLREADERHELPER_H
#define INCLUDED_OCIO_FILEFORMATS_XMLUTILS_XMLREADERHELPER_H


#include <string.h>

#include <OpenColorIO/OpenColorIO.h>

#include "fileformats/xmlutils/XMLReaderUtils.h"
#include "ops/cdl/CDLOpData.h"
#include "PrivateTypes.h"
#include "transforms/CDLTransform.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{
// Base class for all elements possible for parsing XML.
class XmlReaderElement
{
public:
    XmlReaderElement(const std::string & name,
                     unsigned int xmlLineNumber,
                     const std::string & xmlFile);

    virtual ~XmlReaderElement();

    // Start the parsing of the element.
    virtual void start(const char ** atts) = 0;

    // End the parsing of the element.
    virtual void end() = 0;

    // Is it a container which means if it can hold other elements.
    virtual bool isContainer() const = 0;

    const std::string & getName() const
    {
        return m_name;
    }

    virtual const std::string & getIdentifier() const = 0;

    unsigned int getXmlLineNumber() const
    {
        return m_xmlLineNumber;
    }

    const std::string & getXmlFile() const;

    virtual const char * getTypeName() const = 0;

    // Set the element context.
    void setContext(const std::string & name,
                    unsigned int xmlLineNumber,
                    const std::string & xmlFile);

    // Is it a dummy element?
    // Only XmlReaderDummyElt will return true.
    virtual bool isDummy() const
    {
        return false;
    }

    void throwMessage(const std::string & error) const;

    void logParameterWarning(const char * param) const;

protected:
    template<typename T>
    void parseScalarAttribute(const char * name, const char * attrib, T & value)
    {
        const size_t len = strlen(attrib);
        std::vector<T> data;

        try
        {
            data = GetNumbers<T>(attrib, len);
        }
        catch (Exception & ce)
        {
            std::ostringstream oss;
            oss << "For parameter: '";
            oss << name << "'. ";
            oss << ce.what();
            throwMessage(oss.str());
        }

        if (data.size() != 1)
        {
            std::ostringstream oss;
            oss << "For parameter: '";
            oss << name << "'. ";
            oss << "Expecting 1 value, found " << data.size() << " values.";
            throwMessage(oss.str());
        }

        value = data[0];
    }

private:
    std::string  m_name;
    unsigned int m_xmlLineNumber;
    std::string  m_xmlFile;

private:
    XmlReaderElement() = delete;
    XmlReaderElement(const XmlReaderElement &) = delete;
    XmlReaderElement & operator=(const XmlReaderElement &) = delete;
};

typedef OCIO_SHARED_PTR<XmlReaderElement> ElementRcPtr;

// Base class for element that could contain sub-elements.
class XmlReaderContainerElt : public XmlReaderElement
{
public:
    XmlReaderContainerElt(const std::string & name,
                          unsigned int xmlLineNumber,
                          const std::string & xmlFile)
        : XmlReaderElement(name, xmlLineNumber, xmlFile)
    {
    }

    virtual ~XmlReaderContainerElt()
    {
    }

    // Is it a container which means if it can hold other elements.
    bool isContainer() const override
    {
        return true;
    }

    virtual void appendMetadata(const std::string & name, const std::string & value) = 0;

private:
    XmlReaderContainerElt() = delete;

};

typedef OCIO_SHARED_PTR<XmlReaderContainerElt> ContainerEltRcPtr;

// Base class for all basic elements.
class XmlReaderPlainElt : public XmlReaderElement
{
public:
    XmlReaderPlainElt(const std::string & name,
                      ContainerEltRcPtr pParent,
                      unsigned int xmlLineNumber,
                      const std::string & xmlFile)
        : XmlReaderElement(name, xmlLineNumber, xmlFile)
        , m_parent(pParent)
    {
    }

    ~XmlReaderPlainElt()
    {
    }

    virtual void setRawData(const char * str,
                            size_t len,
                            unsigned int xmlLine) = 0;

    bool isContainer() const override
    {
        return false;
    }

    const ContainerEltRcPtr & getParent() const
    {
        return m_parent;
    }

    const std::string & getIdentifier() const override
    {
        return getName();
    }

    const char * getTypeName() const override
    {
        return getName().c_str();
    }

private:
    XmlReaderPlainElt() = delete;

    // The element's parent.
    ContainerEltRcPtr m_parent;
};

// Dummy to address unknown Element. When parsing meets some unrecognized
// data, a dummy is used to continue parsing.
class XmlReaderDummyElt : public XmlReaderPlainElt
{
    // Dummy to address unexpected parent for the DummyElt.
    class DummyParent : public XmlReaderContainerElt
    {
    public:
        DummyParent() = delete;
        DummyParent(ElementRcPtr & pParent)
            : XmlReaderContainerElt(pParent.get() ? pParent->getName() : "",
                                    pParent.get() ? pParent->getXmlLineNumber() : 0,
                                    pParent.get() ? pParent->getXmlFile() : "")
        {
        }
        ~DummyParent()
        {
        }

        void appendMetadata(const std::string & /*name*/, const std::string & /*value*/) override
        {
        }

        const std::string & getIdentifier() const override;

        void start(const char ** /* atts */) override
        {
        }

        void end() override
        {
        }

        const char * getTypeName() const override
        {
            return getIdentifier().c_str();
        }
    };

public:
    XmlReaderDummyElt(const std::string & name,
                      ElementRcPtr pParent,
                      unsigned int xmlLocation,
                      const std::string & xmlFile,
                      const char * msg);

    virtual ~XmlReaderDummyElt()
    {
    }

    const std::string & getIdentifier() const override;

    void start(const char ** /* atts */) override
    {
    }

    void end() override
    {
    }

    void setRawData(const char * str, size_t len, unsigned int /* xmlLine */) override
    {
        m_rawData.push_back(std::string(str, len));
    }

    bool isDummy() const  override
    {
        return true;
    }

private:
    StringUtils::StringVec m_rawData;
};

typedef OCIO_SHARED_PTR<XmlReaderDummyElt> DummyEltRcPtr;

// Class for the description's element.
class XmlReaderDescriptionElt : public XmlReaderPlainElt
{
public:
    XmlReaderDescriptionElt() = delete;
    XmlReaderDescriptionElt(const std::string & name,
                            ContainerEltRcPtr & pParent,
                            unsigned int xmlLocation,
                            const std::string & xmlFile)
        : XmlReaderPlainElt(name, pParent, xmlLocation, xmlFile)
        , m_changed(false)
    {
    }

    ~XmlReaderDescriptionElt()
    {
    }

    void start(const char ** /* atts */) override
    {
        m_description.resize(0);
        m_changed = false;
    }

    void end() override;

    void setRawData(const char * str, size_t len, unsigned int /* xmlLine */) override
    {
        // Keep adding to the string.
        m_description += std::string(str, len);
        m_changed = true;
    }

private:
    std::string m_description;
    bool m_changed;

};

// Base class for nested elements.
class XmlReaderComplexElt : public XmlReaderContainerElt
{
public:
    XmlReaderComplexElt(const std::string & name,
                        ContainerEltRcPtr pParent,
                        unsigned int xmlLineNumber,
                        const std::string & xmlFile)
        : XmlReaderContainerElt(name, xmlLineNumber, xmlFile)
        , m_parent(pParent)
    {
    }

    ~XmlReaderComplexElt()
    {
    }

    const ContainerEltRcPtr & getParent() const
    {
        return m_parent;
    }

    const std::string & getIdentifier() const override
    {
        return getName();
    }

    const char * getTypeName() const override
    {
        return getName().c_str();
    }

    void appendMetadata(const std::string & /*name*/, const std::string & /*value*/) override
    {
    }

private:
    XmlReaderComplexElt() = delete;

    ContainerEltRcPtr m_parent;
};

class XmlReaderSOPNodeBaseElt : public XmlReaderComplexElt
{
public:
    XmlReaderSOPNodeBaseElt(const std::string & name,
                            ContainerEltRcPtr pParent,
                            unsigned int xmlLineNumber,
                            const std::string & xmlFile)
        : XmlReaderComplexElt(name, pParent, xmlLineNumber, xmlFile)
        , m_isSlopeInit(false)
        , m_isOffsetInit(false)
        , m_isPowerInit(false)
    {
    }

    void start(const char ** /* atts */) override
    {
        m_isSlopeInit = m_isOffsetInit = m_isPowerInit = false;
    }

    void end() override
    {
        if (!m_isSlopeInit)
        {
            throwMessage("Required node 'Slope' is missing. ");
        }

        if (!m_isOffsetInit)
        {
            throwMessage("Required node 'Offset' is missing. ");
        }

        if (!m_isPowerInit)
        {
            throwMessage("Required node 'Power' is missing. ");
        }
    }

    virtual const CDLOpDataRcPtr & getCDL() const = 0;

    void setIsSlopeInit(bool status) { m_isSlopeInit = status; }
    void setIsOffsetInit(bool status) { m_isOffsetInit = status; }
    void setIsPowerInit(bool status) { m_isPowerInit = status; }

    void appendMetadata(const std::string & /* name */, const std::string & value) override
    {
        // Add description to parent and override name.
        FormatMetadataImpl item(METADATA_SOP_DESCRIPTION, value);
        getCDL()->getFormatMetadata().getChildrenElements().push_back(item);
    }

private:
    XmlReaderSOPNodeBaseElt() = delete;

    bool m_isSlopeInit;
    bool m_isOffsetInit;
    bool m_isPowerInit;
};

// Class for the slope, offset and power elements.
class XmlReaderSOPValueElt : public XmlReaderPlainElt
{
public:
    XmlReaderSOPValueElt(const std::string & name,
                         ContainerEltRcPtr pParent,
                         unsigned int xmlLineNumber,
                         const std::string & xmlFile);

    ~XmlReaderSOPValueElt();

    void start(const char ** atts) override;

    void end() override;

    void setRawData(const char * str, size_t len, unsigned int xmlLine) override;

private:
    XmlReaderSOPValueElt() = delete;

    std::string m_contentData;
};

// Base class for the SatNode element.
class XmlReaderSatNodeBaseElt : public XmlReaderComplexElt
{
public:
    XmlReaderSatNodeBaseElt(const std::string & name,
                            ContainerEltRcPtr pParent,
                            unsigned int xmlLineNumber,
                            const std::string & xmlFile)
        : XmlReaderComplexElt(name, pParent, xmlLineNumber, xmlFile)
    {
    }

    void start(const char ** /* atts */) override
    {
    }

    void end() override
    {
    }

    virtual const CDLOpDataRcPtr & getCDL() const = 0;

    void appendMetadata(const std::string & /* name */, const std::string & value) override
    {
        // Add description to parent and override name.
        FormatMetadataImpl item(METADATA_SAT_DESCRIPTION, value);
        getCDL()->getFormatMetadata().getChildrenElements().push_back(item);
    }


private:
    XmlReaderSatNodeBaseElt() = delete;
};

// Class for the CDL Saturation element.
class XmlReaderSaturationElt : public XmlReaderPlainElt
{
public:
    XmlReaderSaturationElt(const std::string & name,
                           ContainerEltRcPtr pParent,
                           unsigned int xmlLineNumber,
                           const std::string & xmlFile);

    ~XmlReaderSaturationElt();

    void start(const char ** atts) override;

    void end() override;

    void setRawData(const char * str, size_t len, unsigned int xmlLine) override;

private:
    XmlReaderSaturationElt() = delete;

    std::string m_contentData;
};

// Stack of elements.
class XmlReaderElementStack
{
    typedef std::vector<ElementRcPtr> Stack;

public:
    XmlReaderElementStack();

    XmlReaderElementStack(const XmlReaderElementStack &) = delete;
    XmlReaderElementStack & operator=(const XmlReaderElementStack &) = delete;

    virtual ~XmlReaderElementStack();

    bool empty() const;

    unsigned size() const;

    void push_back(ElementRcPtr pElt);

    void pop_back();

    ElementRcPtr back() const;

    ElementRcPtr front() const;

    void clear();

private:
    Stack m_elms;
};

} // namespace OCIO_NAMESPACE

#endif

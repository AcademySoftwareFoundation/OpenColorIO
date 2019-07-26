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

#include <sstream>

#include "fileformats/ctf/CTFReaderUtils.h"
#include "fileformats/ctf/CTFTransform.h"
#include "fileformats/xmlutils/XMLReaderUtils.h"
#include "HashUtils.h"
#include "ops/CDL/CDLOpData.h"
#include "ops/Exponent/ExponentOps.h"
#include "ops/exposurecontrast/ExposureContrastOpData.h"
#include "ops/FixedFunction/FixedFunctionOpData.h"
#include "ops/Gamma/GammaOpData.h"
#include "ops/Log/LogOpData.h"
#include "ops/Lut1D/Lut1DOpData.h"
#include "ops/Lut3D/Lut3DOpData.h"
#include "ops/Matrix/MatrixOpData.h"
#include "ops/Range/RangeOpData.h"
#include "ops/reference/ReferenceOpData.h"
#include "Platform.h"

OCIO_NAMESPACE_ENTER
{

void CTFVersion::ReadVersion(const std::string & versionString,
                             CTFVersion & versionOut)
{
    unsigned int numDot = 0;
    unsigned int numInt = 0;
    bool canBeDot = false;
    std::string::const_iterator it = versionString.begin();
    while (it != versionString.end())
    {
        if (::isdigit(*it))
        {
            numInt = numDot + 1;
            canBeDot = true;
            ++it;
        }
        else if (it[0] == '.' && canBeDot)
        {
            canBeDot = false;
            numDot += 1;
            ++it;
        }
        else
        {
            break;
        }
    }
    if (versionString.empty()
        || it != versionString.end()
        || numInt == 0
        || numInt > 3
        || numInt == numDot)
    {
        std::ostringstream os;
        os << "'";
        os << versionString;
        os << "' is not a valid version. ";
        os << "Expecting MAJOR[.MINOR[.REVISION]] ";
        throw Exception(os.str().c_str());
    }

    versionOut.m_major = 0;
    versionOut.m_minor = 0;
    versionOut.m_revision = 0;

    sscanf(versionString.c_str(), "%d.%d.%d",
           &versionOut.m_major,
           &versionOut.m_minor,
           &versionOut.m_revision);
}

CTFVersion & CTFVersion::operator=(const CTFVersion & rhs)
{
    if (this != &rhs)
    {
        m_major = rhs.m_major;
        m_minor = rhs.m_minor;
        m_revision = rhs.m_revision;
    }
    return *this;
}

bool CTFVersion::operator==(const CTFVersion & rhs) const
{
    if (this == &rhs) return true;

    return m_major == rhs.m_major
        && m_minor == rhs.m_minor
        && m_revision == rhs.m_revision;
}

bool CTFVersion::operator<=(const CTFVersion & rhs) const
{
    if (*this == rhs)
    {
        return true;
    }
    return *this < rhs;
}

bool CTFVersion::operator>=(const CTFVersion & rhs) const
{
    return rhs <= *this;
}

bool CTFVersion::operator<(const CTFVersion & rhs) const
{
    if (this == &rhs) return false;

    if (m_major < rhs.m_major)
    {
        return true;
    }
    else if (m_major > rhs.m_major)
    {
        return false;
    }
    else
    {
        if (m_minor < rhs.m_minor)
        {
            return true;
        }
        else if (m_minor > rhs.m_minor)
        {
            return false;
        }
        else
        {
            if (m_revision < rhs.m_revision)
            {
                return true;
            }
            return false;
        }
    }
}

bool CTFVersion::operator>(const CTFVersion & rhs) const
{
    return rhs < *this;
}

CTFReaderTransform::CTFReaderTransform()
    : m_infoMetadata(METADATA_INFO)
    , m_version(CTF_PROCESS_LIST_VERSION)
    , m_versionCLF(0, 0)
{
}

CTFReaderTransform::CTFReaderTransform(const OpRcPtrVec & ops,
                                       const FormatMetadataImpl & metadata)
    : m_infoMetadata(METADATA_INFO)
    , m_version(CTF_PROCESS_LIST_VERSION)
    , m_versionCLF(0, 0)
{
    fromMetadata(metadata);

    for (ConstOpRcPtr op : ops)
    {
        auto opData = op->data();
        m_ops.push_back(opData);
    }
}

void CTFReaderTransform::setCTFVersion(const CTFVersion & ver)
{
    m_version = ver;
}

void CTFReaderTransform::setCLFVersion(const CTFVersion & ver)
{
    m_versionCLF = ver;
}

const CTFVersion & CTFReaderTransform::getCTFVersion() const
{
    return m_version;
}

void CTFReaderTransform::validate()
{
    BitDepth bitdepth = BIT_DEPTH_UNKNOWN;

    const size_t max = m_ops.size();
    for (size_t i = 0; i<max; ++i)
    {
        ConstOpDataRcPtr & op = m_ops[i];

        op->validate();

        if (i > 0 && bitdepth != op->getInputBitDepth())
        {
            std::ostringstream os;
            os << "Bitdepth missmatch between ops";
            os << "'. Op " << i - 1;
            os << " (" << m_ops[i - 1]->getID();
            os << ") output bitdepth is ";
            os << bitdepth << ". Op " << i;
            os << " (" << op->getID();
            os << ") intput bitdepth is ";
            os << op->getInputBitDepth();
            throw Exception(os.str().c_str());
        }

        bitdepth = op->getOutputBitDepth();
    }
}

void GetElementsValues(const FormatMetadataImpl::Elements & elements, const std::string & name, StringVec & values)
{
    for (auto & element : elements)
    {
        if (0 == Platform::Strcasecmp(name.c_str(), element.getName()))
        {
            values.push_back(element.getValue());
        }
    }
}

namespace
{
const char * GetFirstElementValue(const FormatMetadataImpl::Elements & elements, const std::string & name)
{
    for (auto & it : elements)
    {
        if (0 == Platform::Strcasecmp(name.c_str(), it.getName()))
        {
            return it.getValue();
        }
    }
    return "";
}

const char * GetLastElementValue(const FormatMetadataImpl::Elements & elements, const std::string & name)
{
    for (auto it = elements.rbegin(); it != elements.rend(); ++it)
    {
        if (0 == Platform::Strcasecmp(name.c_str(), it->getName()))
        {
            return it->getValue();
        }
    }
    return "";
}
}

// This method copies the metadata from the argument into the transform object.
// Only attributes and elements that are expected parts of the CLF spec are
// preserved. This corresponds to the top level metadata in the CLF ProcessList,
// note that any metadata in the individual process nodes are stored separately
// in their opData.  Here is what is preserved:
// -- ProcessList attributes "name", "id", and "inverseOf". Other attributes are ignored.
// -- ProcessList sub-elements "InputDescriptor" and "OutputDescriptor". The value
//    of these elements is preserved but no additional attributes or sub-elements.
//    Only the first InputDescriptor and last OutputDescriptor in the metadata is preserved.
// -- ProcessList "Description" sub-elements. All of these elements are preserved,
//    but only their value strings, no attributes or sub-elements.
// -- ProcessList "Info" sub-elements. If there is more than one, they are merged into
//    a single Info element. All attributes and sub-elements are preserved.
// -- Any other sub-elements or attributes are ignored.
void CTFReaderTransform::fromMetadata(const FormatMetadataImpl & metadata)
{
    // Name & id handled as attributes of the root metadata.
    m_name = metadata.getAttributeValue(METADATA_NAME);
    m_id = metadata.getAttributeValue(METADATA_ID);
    m_inverseOfId = metadata.getAttributeValue(ATTR_INVERSE_OF);

    // Preserve first InputDescriptor, last OutputDescriptor, and all Descriptions.
    m_inDescriptor = GetFirstElementValue(metadata.getChildrenElements(), TAG_INPUT_DESCRIPTOR);
    m_outDescriptor = GetLastElementValue(metadata.getChildrenElements(), TAG_OUTPUT_DESCRIPTOR);
    GetElementsValues(metadata.getChildrenElements(), METADATA_DESCRIPTION, m_descriptions);

    // Combine all Info elements.
    for (auto elt : metadata.getChildrenElements())
    {
        if (0 == Platform::Strcasecmp(elt.getName(), METADATA_INFO))
        {
            m_infoMetadata.combine(elt);
        }
    }
}

namespace
{
void AddNonEmptyElement(FormatMetadataImpl & metadata, const char * name, const std::string & value)
{
    if (!value.empty())
    {
        metadata.addChildElement(name, value.c_str());
    }
}

void AddNonEmptyAttribute(FormatMetadataImpl & metadata, const char * name, const std::string & value)
{
    if (!value.empty())
    {
        metadata.addAttribute(name, value.c_str());
    }
}
}

void CTFReaderTransform::toMetadata(FormatMetadataImpl & metadata) const
{
    // Put CTF processList information into the FormatMetadata.
    AddNonEmptyAttribute(metadata, METADATA_NAME, getName());
    AddNonEmptyAttribute(metadata, METADATA_ID, getID());
    AddNonEmptyAttribute(metadata, ATTR_INVERSE_OF, getInverseOfId());

    AddNonEmptyElement(metadata, TAG_INPUT_DESCRIPTOR, getInputDescriptor());
    AddNonEmptyElement(metadata, TAG_OUTPUT_DESCRIPTOR, getOutputDescriptor());
    for (auto & desc : m_descriptions)
    {
        metadata.addChildElement(METADATA_DESCRIPTION, desc.c_str());
    }
    const std::string infoValue(m_infoMetadata.getValue());
    if (m_infoMetadata.getNumAttributes() || m_infoMetadata.getNumChildrenElements() ||
        !infoValue.empty())
    {
        metadata.getChildrenElements().push_back(m_infoMetadata);
    }
}


///////////////////////////////////////////////////////////////////////////////

namespace
{
void WriteDescriptions(XmlFormatter & fmt, const StringVec & descriptions)
{
    for (auto & it : descriptions)
    {
        fmt.writeContentTag(TAG_DESCRIPTION, it);
    }
}

template <typename T>
typename std::enable_if<std::is_floating_point<T>::value, void>::type WriteValue(T value, std::ostream& stream)
{
    if (IsNan(value))
    {
        stream << "nan";
    }
    else if (value == std::numeric_limits<T>::infinity())
    {
        stream << "inf";
    }
    else if (std::is_signed<T>::value &&
        value == -std::numeric_limits<T>::infinity())
    {
        stream << "-inf";
    }
    else
    {
        stream << value;
    }
}

template <typename T>
typename std::enable_if<!std::is_floating_point<T>::value, void>::type WriteValue(T value, std::ostream& stream)
{
    stream << value;
}

template <typename T>
void SetOStream(T, std::ostream & xml)
{
    xml.width(11);
    xml.precision(8);
}

template <>
void SetOStream<double>(double, std::ostream & xml)
{
    xml.width(19);
    xml.precision(15);
}


template<typename Iter>
void WriteValues(XmlFormatter & formatter,
                 Iter valuesBegin,
                 Iter valuesEnd,
                 unsigned valuesPerLine,
                 BitDepth bitDepth,
                 unsigned iterStep)
{
    std::ostream& xml = formatter.getStream();

    for (Iter it(valuesBegin); it != valuesEnd; it += iterStep)
    {
        switch (bitDepth)
        {
        case BIT_DEPTH_UINT8:
        {
            xml.width(3);
            xml << *it;
            break;
        }
        case BIT_DEPTH_UINT10:
        {
            xml.width(4);
            xml << *it;
            break;
        }

        case BIT_DEPTH_UINT12:
        {
            xml.width(4);
            xml << *it;
            break;
        }

        case BIT_DEPTH_UINT16:
        {
            xml.width(5);
            xml << *it;
            break;
        }

        case BIT_DEPTH_F16:
        {
            xml.width(11);
            xml.precision(5);
            WriteValue(*it, xml);
            break;
        }

        case BIT_DEPTH_F32:
        {
            SetOStream(*it, xml);
            WriteValue(*it, xml);
            break;
        }

        default:
        {
            throw Exception("Unknown bitdepth.");
            break;
        }
        }

        if (std::distance(valuesBegin, it) % valuesPerLine
            == valuesPerLine - 1)
        {
            xml << std::endl;
        }
        else
        {
            xml << " ";
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

class OpWriter : public XmlElementWriter
{
public:
    OpWriter() = delete;
    OpWriter(const OpWriter&) = delete;
    OpWriter& operator=(const OpWriter&) = delete;

    OpWriter(XmlFormatter & formatter);
    virtual ~OpWriter();

    void write() const override;

protected:
    virtual ConstOpDataRcPtr getOp() const = 0;
    virtual const char * getTagName() const = 0;
    virtual void getAttributes(XmlFormatter::Attributes & attributes) const;
    virtual void writeContent() const = 0;
};

OpWriter::OpWriter(XmlFormatter & formatter)
    : XmlElementWriter(formatter)
{
}

OpWriter::~OpWriter()
{
}

void OpWriter::write() const
{
    XmlFormatter::Attributes attributes;
    getAttributes(attributes);

    const char* tagName = getTagName();
    m_formatter.writeStartTag(tagName, attributes);
    {
        XmlScopeIndent scopeIndent(m_formatter);
        auto op = getOp();
        StringVec desc;
        GetElementsValues(op->getFormatMetadata().getChildrenElements(),
                          TAG_DESCRIPTION, desc);
        WriteDescriptions(m_formatter, desc);

        // TODO: Bypass Dynamic Property converts to Look Dynamic Property in ctf format.
        /*if (op->getBypass()->isDynamic())
        {
            attributes.clear();
            attributes.push_back(XmlFormatter::Attribute(ATTR_PARAM, TAG_DYN_PROP_LOOK));
            m_formatter.writeEmptyTag(TAG_DYNAMIC_PARAMETER, attributes);
        }*/

        writeContent();
    }
    m_formatter.writeEndTag(tagName);
}

void OpWriter::getAttributes(XmlFormatter::Attributes & attributes) const
{
    auto op = getOp();
    const std::string& id = op->getID();
    if (!id.empty())
    {
        attributes.push_back(XmlFormatter::Attribute(ATTR_ID, id));
    }

    const std::string& name = op->getName();
    if (!name.empty())
    {
        attributes.push_back(XmlFormatter::Attribute(ATTR_NAME, name));
    }

    const char* inBitDepthName = BitDepthToString(op->getInputBitDepth());
    attributes.push_back(XmlFormatter::Attribute(ATTR_BITDEPTH_IN,
                                                 inBitDepthName));

    const char* outBitDepthName = BitDepthToString(op->getOutputBitDepth());
    attributes.push_back(XmlFormatter::Attribute(ATTR_BITDEPTH_OUT,
                                                 outBitDepthName));

    /* TODO: bypass implementation
    if (op->getBypass()->getValue())
    {
        attributes.push_back(XmlFormatter::Attribute(ATTR_BYPASS, "true"));
    }*/
}

///////////////////////////////////////////////////////////////////////////////

class CDLWriter : public OpWriter
{
public:
    CDLWriter(XmlFormatter & formatter,
              ConstCDLOpDataRcPtr cdl);
    virtual ~CDLWriter();

protected:
    ConstOpDataRcPtr getOp() const override;
    const char * getTagName() const override;
    void getAttributes(XmlFormatter::Attributes& attributes) const override;
    void writeContent() const override;

private:
    ConstCDLOpDataRcPtr m_cdl;
};


CDLWriter::CDLWriter(XmlFormatter & formatter,
                     ConstCDLOpDataRcPtr cdl)
    : OpWriter(formatter)
    , m_cdl(cdl)
{
}

CDLWriter::~CDLWriter()
{
}

ConstOpDataRcPtr CDLWriter::getOp() const
{
    return m_cdl;
}

const char * CDLWriter::getTagName() const
{
    return TAG_CDL;
}
   
void CDLWriter::getAttributes(XmlFormatter::Attributes & attributes) const
{
    OpWriter::getAttributes(attributes);
    const std::string style = CDLOpData::GetStyleName(m_cdl->getStyle());
    attributes.push_back(XmlFormatter::Attribute(ATTR_STYLE, style));
}

void CDLWriter::writeContent() const
{
    XmlFormatter::Attributes attributes;

    // SOPNode.
    m_formatter.writeStartTag(TAG_SOPNODE, attributes);
    {
        XmlScopeIndent scopeIndent(m_formatter);

        m_formatter.writeContentTag(TAG_SLOPE, m_cdl->getSlopeString());
        m_formatter.writeContentTag(TAG_OFFSET, m_cdl->getOffsetString());
        m_formatter.writeContentTag(TAG_POWER, m_cdl->getPowerString());
    }
    m_formatter.writeEndTag(TAG_SOPNODE);

    // SatNode.
    m_formatter.writeStartTag(TAG_SATNODE, attributes);
    {
        XmlScopeIndent scopeIndent(m_formatter);

        m_formatter.writeContentTag(TAG_SATURATION, m_cdl->getSaturationString());
    }
    m_formatter.writeEndTag(TAG_SATNODE);
}

///////////////////////////////////////////////////////////////////////////////

class ExposureContrastWriter : public OpWriter
{
public:
    ExposureContrastWriter(XmlFormatter & formatter,
                           ConstExposureContrastOpDataRcPtr ec);
    virtual ~ExposureContrastWriter();

protected:
    ConstOpDataRcPtr getOp() const override;
    const char * getTagName() const override;
    void getAttributes(XmlFormatter::Attributes& attributes) const override;
    void writeContent() const override;

private:
    ConstExposureContrastOpDataRcPtr m_ec;
};


ExposureContrastWriter::ExposureContrastWriter(XmlFormatter & formatter,
                                               ConstExposureContrastOpDataRcPtr ec)
    : OpWriter(formatter)
    , m_ec(ec)
{
}

ExposureContrastWriter::~ExposureContrastWriter()
{
}

ConstOpDataRcPtr ExposureContrastWriter::getOp() const
{
    return m_ec;
}

const char * ExposureContrastWriter::getTagName() const
{
    return TAG_EXPOSURE_CONTRAST;
}

void ExposureContrastWriter::getAttributes(XmlFormatter::Attributes& attributes) const
{
    OpWriter::getAttributes(attributes);
    const std::string & style = ExposureContrastOpData::ConvertStyleToString(m_ec->getStyle());
    attributes.push_back(XmlFormatter::Attribute(ATTR_STYLE, style));
}

void ExposureContrastWriter::writeContent() const
{
    XmlFormatter::Attributes attributes;
    {
        std::stringstream expAttr;
        WriteValue(m_ec->getExposure(), expAttr);
        attributes.push_back(XmlFormatter::Attribute(ATTR_EXPOSURE,
                                                     expAttr.str()));
    }
    {
        std::stringstream contAttr;
        WriteValue(m_ec->getContrast(), contAttr);
        attributes.push_back(XmlFormatter::Attribute(ATTR_CONTRAST,
                                                     contAttr.str()));
    }
    {
        std::stringstream gammaAttr;
        WriteValue(m_ec->getGamma(), gammaAttr);
        attributes.push_back(XmlFormatter::Attribute(ATTR_GAMMA,
                                                     gammaAttr.str()));
    }
    {
        std::ostringstream oss;
        WriteValue(m_ec->getPivot(), oss);
        attributes.push_back(XmlFormatter::Attribute(ATTR_PIVOT, oss.str()));
    }

    if (m_ec->getLogExposureStep() != ExposureContrastOpData::LOGEXPOSURESTEP_DEFAULT)
    {
        std::ostringstream oss;
        WriteValue(m_ec->getLogExposureStep(), oss);
        attributes.push_back(XmlFormatter::Attribute(ATTR_LOGEXPOSURESTEP, oss.str()));
    }

    if (m_ec->getLogMidGray() != ExposureContrastOpData::LOGMIDGRAY_DEFAULT)
    {
        std::ostringstream oss;
        WriteValue(m_ec->getLogMidGray(), oss);
        attributes.push_back(XmlFormatter::Attribute(ATTR_LOGMIDGRAY, oss.str()));
    }

    m_formatter.writeEmptyTag(TAG_EC_PARAMS, attributes);

    if (m_ec->getExposureProperty()->isDynamic())
    {
        attributes.clear();
        attributes.push_back(XmlFormatter::Attribute(ATTR_PARAM, TAG_DYN_PROP_EXPOSURE));
        m_formatter.writeEmptyTag(TAG_DYNAMIC_PARAMETER, attributes);
    }

    if (m_ec->getContrastProperty()->isDynamic())
    {
        attributes.clear();
        attributes.push_back(XmlFormatter::Attribute(ATTR_PARAM, TAG_DYN_PROP_CONTRAST));
        m_formatter.writeEmptyTag(TAG_DYNAMIC_PARAMETER, attributes);
    }

    if (m_ec->getGammaProperty()->isDynamic())
    {
        attributes.clear();
        attributes.push_back(XmlFormatter::Attribute(ATTR_PARAM, TAG_DYN_PROP_GAMMA));
        m_formatter.writeEmptyTag(TAG_DYNAMIC_PARAMETER, attributes);
    }
}

///////////////////////////////////////////////////////////////////////////////

class FixedFunctionWriter : public OpWriter
{
public:
    FixedFunctionWriter(XmlFormatter & formatter,
                        ConstFixedFunctionOpDataRcPtr ff);
    virtual ~FixedFunctionWriter();

protected:
    ConstOpDataRcPtr getOp() const override;
    const char * getTagName() const override;
    void getAttributes(XmlFormatter::Attributes& attributes) const override;
    void writeContent() const override;

private:
    ConstFixedFunctionOpDataRcPtr m_ff;
};

FixedFunctionWriter::FixedFunctionWriter(XmlFormatter & formatter,
                                         ConstFixedFunctionOpDataRcPtr ff)
    : OpWriter(formatter)
    , m_ff(ff)
{
}

FixedFunctionWriter::~FixedFunctionWriter()
{
}

ConstOpDataRcPtr FixedFunctionWriter::getOp() const
{
    return m_ff;
}

const char * FixedFunctionWriter::getTagName() const
{
    return TAG_FIXED_FUNCTION;
}

void FixedFunctionWriter::getAttributes(XmlFormatter::Attributes& attributes) const
{
    OpWriter::getAttributes(attributes);
    const std::string style = FixedFunctionOpData::ConvertStyleToString(m_ff->getStyle(), false);
    attributes.push_back(XmlFormatter::Attribute(ATTR_STYLE, style));
    const FixedFunctionOpData::Params & params = m_ff->getParams();
    const size_t numParams = params.size();
    if (numParams != 0)
    {
        size_t i = 0;
        std::stringstream ffParams;
        WriteValue(params[i], ffParams);
        while (++i < numParams)
        {
            ffParams << " ";
            WriteValue(params[i], ffParams);
        }
        attributes.push_back(XmlFormatter::Attribute(ATTR_PARAMS, ffParams.str()));
    }
}

void FixedFunctionWriter::writeContent() const
{
}

///////////////////////////////////////////////////////////////////////////////

class GammaWriter : public OpWriter
{
public:
    GammaWriter(XmlFormatter & formatter,
                ConstGammaOpDataRcPtr gamma);
    virtual ~GammaWriter();

protected:
    ConstOpDataRcPtr getOp() const override;
    const char * getTagName() const override;
    void getAttributes(XmlFormatter::Attributes& attributes) const override;
    void writeContent() const override;

private:
    ConstGammaOpDataRcPtr m_gamma;
};


GammaWriter::GammaWriter(XmlFormatter & formatter,
                         ConstGammaOpDataRcPtr gamma)
    : OpWriter(formatter)
    , m_gamma(gamma)
{
}

GammaWriter::~GammaWriter()
{
}

ConstOpDataRcPtr GammaWriter::getOp() const
{
    return m_gamma;
}

const char * GammaWriter::getTagName() const
{
    return TAG_GAMMA;
}

void GammaWriter::getAttributes(XmlFormatter::Attributes& attributes) const
{
    OpWriter::getAttributes(attributes);
    const std::string & style = GammaOpData::ConvertStyleToString(m_gamma->getStyle());
    attributes.push_back(XmlFormatter::Attribute(ATTR_STYLE,
                                                 style));
}

namespace
{
// Add the attributes for the GammaParams element.
void AddGammaParams(XmlFormatter::Attributes & attributes,
                    const GammaOpData::Params & params,
                    const GammaOpData::Style style)
{
    std::stringstream gammaAttr;
    gammaAttr << params[0];

    attributes.push_back(XmlFormatter::Attribute(ATTR_GAMMA,
                                                 gammaAttr.str()));

    switch (style)
    {
    case GammaOpData::MONCURVE_FWD:
    case GammaOpData::MONCURVE_REV:
    {
        std::stringstream offsetAttr;
        offsetAttr << params[1];
        attributes.push_back(XmlFormatter::Attribute(ATTR_OFFSET,
                                                     offsetAttr.str()));
        break;
    }
    case GammaOpData::BASIC_FWD:
    case GammaOpData::BASIC_REV:
        break;
    }
}
}

void GammaWriter::writeContent() const
{
    if (m_gamma->isNonChannelDependent())
    {
        // RGB channels equal and A is identity, just write one element.
        XmlFormatter::Attributes attributes;

        AddGammaParams(attributes,
                       m_gamma->getRedParams(),
                       m_gamma->getStyle());

        m_formatter.writeEmptyTag(TAG_GAMMA_PARAMS, attributes);
    }
    else
    {
        // Red.
        XmlFormatter::Attributes attributesR;
        attributesR.push_back(XmlFormatter::Attribute(ATTR_CHAN,
                                                      "R"));
        AddGammaParams(attributesR,
                       m_gamma->getRedParams(),
                       m_gamma->getStyle());

        m_formatter.writeEmptyTag(TAG_GAMMA_PARAMS, attributesR);

        // Green.
        XmlFormatter::Attributes attributesG;
        attributesG.push_back(XmlFormatter::Attribute(ATTR_CHAN,
                                                      "G"));
        AddGammaParams(attributesG,
                       m_gamma->getGreenParams(),
                       m_gamma->getStyle());

        m_formatter.writeEmptyTag(TAG_GAMMA_PARAMS, attributesG);

        // Blue.
        XmlFormatter::Attributes attributesB;
        attributesB.push_back(XmlFormatter::Attribute(ATTR_CHAN,
                                                      "B"));
        AddGammaParams(attributesB,
                       m_gamma->getBlueParams(),
                       m_gamma->getStyle());

        m_formatter.writeEmptyTag(TAG_GAMMA_PARAMS, attributesB);

        // TODO: sc uses minimun version.
        //if (getOpMinimumVersion(m_gamma) >= 1.5f)
        {
            // Alpha
            XmlFormatter::Attributes attributesA;
            attributesA.push_back(XmlFormatter::Attribute(ATTR_CHAN,
                                                          "A"));
            AddGammaParams(attributesA,
                           m_gamma->getAlphaParams(),
                           m_gamma->getStyle());

            m_formatter.writeEmptyTag(TAG_GAMMA_PARAMS, attributesA);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

class LogWriter : public OpWriter
{
public:
    LogWriter(XmlFormatter & formatter,
              ConstLogOpDataRcPtr log);
    virtual ~LogWriter();

protected:
    ConstOpDataRcPtr getOp() const override;
    const char * getTagName() const override;
    void getAttributes(XmlFormatter::Attributes& attributes) const override;
    void writeContent() const override;

private:
    ConstLogOpDataRcPtr m_log;
};

LogWriter::LogWriter(XmlFormatter & formatter,
                     ConstLogOpDataRcPtr log)
    : OpWriter(formatter)
    , m_log(log)
{
}

LogWriter::~LogWriter()
{
}

ConstOpDataRcPtr LogWriter::getOp() const
{
    return m_log;
}

const char * LogWriter::getTagName() const
{
    return TAG_LOG;
}

void LogWriter::getAttributes(XmlFormatter::Attributes& attributes) const
{
    OpWriter::getAttributes(attributes);

    const TransformDirection dir = m_log->getDirection();
    std::string style;
    if (m_log->isLog2())
    {
        style = dir == TRANSFORM_DIR_FORWARD ? LOG_LOG2 : LOG_ANTILOG2;
    }
    else if (m_log->isLog10())
    {
        style = dir == TRANSFORM_DIR_FORWARD ? LOG_LOG10 : LOG_ANTILOG10;
    }
    else
    {
        style = dir == TRANSFORM_DIR_FORWARD ? LOG_LINTOLOG : LOG_LOGTOLIN;
    }
    
    attributes.push_back(XmlFormatter::Attribute(ATTR_STYLE, style));
}


namespace
{
void AddLogParam(XmlFormatter::Attributes & attributes,
                 const char * attrName,
                 double attrValue)
{
    std::stringstream stream;
    stream << attrValue;
    attributes.push_back(XmlFormatter::Attribute(attrName, stream.str()));
}

void AddLogParams(XmlFormatter::Attributes & attributes,
                  const LogOpData::Params & params, const double base)
{
    AddLogParam(attributes, ATTR_LINSIDESLOPE, params[LIN_SIDE_SLOPE]);
    AddLogParam(attributes, ATTR_LINSIDEOFFSET, params[LIN_SIDE_OFFSET]);
    AddLogParam(attributes, ATTR_LOGSIDESLOPE, params[LOG_SIDE_SLOPE]);
    AddLogParam(attributes, ATTR_LOGSIDEOFFSET, params[LOG_SIDE_OFFSET]);
    AddLogParam(attributes, ATTR_BASE, base);
}
}

void LogWriter::writeContent() const
{
    if (m_log->isLog2() || m_log->isLog10())
    {
        // No parameters to save.
        return;
    }

    if (m_log->allComponentsEqual())
    {
        // All channels equal, just write one element.
        XmlFormatter::Attributes attributes;

        AddLogParams(attributes, m_log->getRedParams(), m_log->getBase());

        m_formatter.writeEmptyTag(TAG_LOG_PARAMS, attributes);
    }
    else
    {
        // Red.
        XmlFormatter::Attributes attributesR;
        attributesR.push_back(XmlFormatter::Attribute(ATTR_CHAN, "R"));
        AddLogParams(attributesR, m_log->getRedParams(), m_log->getBase());

        m_formatter.writeEmptyTag(TAG_LOG_PARAMS, attributesR);

        // Green.
        XmlFormatter::Attributes attributesG;
        attributesG.push_back(XmlFormatter::Attribute(ATTR_CHAN, "G"));
        AddLogParams(attributesG, m_log->getGreenParams(), m_log->getBase());

        m_formatter.writeEmptyTag(TAG_LOG_PARAMS, attributesG);

        // Blue.
        XmlFormatter::Attributes attributesB;
        attributesB.push_back(XmlFormatter::Attribute(ATTR_CHAN, "B"));
        AddLogParams(attributesB, m_log->getBlueParams(), m_log->getBase());

        m_formatter.writeEmptyTag(TAG_LOG_PARAMS, attributesB);
    }
}


///////////////////////////////////////////////////////////////////////////////

class Lut1DWriter : public OpWriter
{
public:
    Lut1DWriter(XmlFormatter & formatter,
                ConstLut1DOpDataRcPtr lut);
    virtual ~Lut1DWriter();

protected:
    ConstOpDataRcPtr getOp() const override;
    const char * getTagName() const override;
    void getAttributes(XmlFormatter::Attributes& attributes) const override;
    void writeContent() const override;

private:
    ConstLut1DOpDataRcPtr m_lut;
};

Lut1DWriter::Lut1DWriter(XmlFormatter & formatter,
                         ConstLut1DOpDataRcPtr lut)
    : OpWriter(formatter)
    , m_lut(lut)
{
}

Lut1DWriter::~Lut1DWriter()
{
}

ConstOpDataRcPtr Lut1DWriter::getOp() const
{
    return m_lut;
}
   
const char * Lut1DWriter::getTagName() const
{
    if (m_lut->getDirection() == TRANSFORM_DIR_FORWARD)
    {
        return TAG_LUT1D;
    }
    else
    {
        return TAG_INVLUT1D;
    }
}

void Lut1DWriter::getAttributes(XmlFormatter::Attributes & attributes) const
{
    OpWriter::getAttributes(attributes);
    Interpolation interpolation = m_lut->getInterpolation();
    if (interpolation != INTERP_DEFAULT)
    {
        const char * interpolationName = GetInterpolation1DName(interpolation);
        attributes.push_back(XmlFormatter::Attribute(ATTR_INTERPOLATION,
                                                     interpolationName));
    }

    if (m_lut->isInputHalfDomain())
    {
        attributes.push_back(XmlFormatter::Attribute(ATTR_HALF_DOMAIN,
                                                     "true"));
    }

    if (m_lut->isOutputRawHalfs())
    {
        attributes.push_back(XmlFormatter::Attribute(ATTR_RAW_HALFS,
                                                     "true"));
    }

    Lut1DOpData::HueAdjust hueAdjust = m_lut->getHueAdjust();
    if (hueAdjust == Lut1DOpData::HUE_DW3)
    {
        static_assert(Lut1DOpData::HUE_NB_STYLES == 2, "Adjust code if new styles are added.");
        attributes.push_back(XmlFormatter::Attribute(ATTR_HUE_ADJUST,
                                                     "dw3"));
    }

}
    
void Lut1DWriter::writeContent() const
{
    // Note: As of CTF v1.7 we support IndexMaps and that member of the LUT is
    // populated, however, since we convert it to a Range on reading, we do not
    // want to write out the IndexMap.

    const Array & array = m_lut->getArray();
    std::stringstream dimension;
    dimension << array.getLength() << " "
              << array.getNumColorComponents();

    XmlFormatter::Attributes attributes;
    attributes.push_back(XmlFormatter::Attribute(ATTR_DIMENSION,
                                                 dimension.str()));

    m_formatter.writeStartTag(TAG_ARRAY, attributes);

    if (m_lut->isOutputRawHalfs())
    {
        std::vector<unsigned> values;

        const size_t maxValues = array.getNumValues();
        values.resize(maxValues);

        for (size_t i = 0; i<maxValues; ++i)
        {
            half h = array.getValues()[i];
            values[i] = h.bits();
        }

        WriteValues(m_formatter,
                    values.begin(),
                    values.end(),
                    array.getNumColorComponents(),
                    BIT_DEPTH_UINT16,
                    array.getNumColorComponents() == 1 ? 3 : 1);
    }
    else
    {
        const Array::Values & values = array.getValues();
        WriteValues(m_formatter,
                    values.begin(),
                    values.end(),
                    array.getNumColorComponents(),
                    m_lut->getOutputBitDepth(),
                    array.getNumColorComponents() == 1 ? 3 : 1);
    }

    m_formatter.writeEndTag(TAG_ARRAY);
}

///////////////////////////////////////////////////////////////////////////////

class Lut3DWriter : public OpWriter
{
public:
    Lut3DWriter(XmlFormatter & formatter,
                ConstLut3DOpDataRcPtr lut);
    virtual ~Lut3DWriter();

protected:
    ConstOpDataRcPtr getOp() const override;
    const char * getTagName() const override;
    void getAttributes(XmlFormatter::Attributes& attributes) const override;
    void writeContent() const override;

private:
    ConstLut3DOpDataRcPtr m_lut;
};

Lut3DWriter::Lut3DWriter(XmlFormatter & formatter,
                         ConstLut3DOpDataRcPtr lut)
    : OpWriter(formatter)
    , m_lut(lut)
{
}

Lut3DWriter::~Lut3DWriter()
{
}

ConstOpDataRcPtr Lut3DWriter::getOp() const
{
    return m_lut;
}

const char * Lut3DWriter::getTagName() const
{
    if (m_lut->getDirection() == TRANSFORM_DIR_FORWARD)
    {
        return TAG_LUT3D;
    }
    else
    {
        return TAG_INVLUT3D;
    }
}

void Lut3DWriter::getAttributes(XmlFormatter::Attributes & attributes) const
{
    OpWriter::getAttributes(attributes);
    Interpolation interpolation = m_lut->getInterpolation();
    if (interpolation != INTERP_DEFAULT)
    {
        const char* interpolationName = GetInterpolation3DName(interpolation);
        attributes.push_back(XmlFormatter::Attribute(ATTR_INTERPOLATION,
                                                     interpolationName));
    }
}

void Lut3DWriter::writeContent() const
{
    // Note: As of CTF v1.7 we support IndexMaps and that member of the Lut is
    // populated, however, since we convert it to a Range on reading, we do not
    // want to write out the IndexMap.

    const Array & array = m_lut->getArray();
    std::stringstream dimension;
    dimension << array.getLength() << " "
              << array.getLength() << " "
              << array.getLength() << " "
              << array.getNumColorComponents();

    XmlFormatter::Attributes attributes;
    attributes.push_back(XmlFormatter::Attribute(ATTR_DIMENSION,
                         dimension.str()));

    m_formatter.writeStartTag(TAG_ARRAY, attributes);

    WriteValues(m_formatter,
                array.getValues().begin(),
                array.getValues().end(),
                3,
                m_lut->getOutputBitDepth(),
                1);

    m_formatter.writeEndTag(TAG_ARRAY);
}

///////////////////////////////////////////////////////////////////////////////

class MatrixWriter : public OpWriter
{
public:
    MatrixWriter(XmlFormatter & formatter,
                 ConstMatrixOpDataRcPtr matrix);
    virtual ~MatrixWriter();

protected:
    ConstOpDataRcPtr getOp() const override;
    const char * getTagName() const override;
    void writeContent() const override;

private:
    ConstMatrixOpDataRcPtr m_matrix;
};

MatrixWriter::MatrixWriter(XmlFormatter & formatter,
                           ConstMatrixOpDataRcPtr matrix)
    : OpWriter(formatter)
    , m_matrix(matrix)
{
}

MatrixWriter::~MatrixWriter()
{
}

ConstOpDataRcPtr MatrixWriter::getOp() const
{
    return m_matrix;
}

const char * MatrixWriter::getTagName() const
{
    return TAG_MATRIX;
}

void MatrixWriter::writeContent() const
{
    // Matrix Op supports 4 xml formats:
    //   1) 4x5x4, matrix with alpha and offsets.
    //   2) 4x4x4, matrix with alpha and no offsets.
    //   3) 3x4x3, matrix only with offsets and no alpha.
    //   4) 3x3x3, matrix with no alpha and no offsets.

    std::stringstream dimensionAttr;
    if (m_matrix->hasAlpha())
    {
        if (m_matrix->hasOffsets())
        {
            dimensionAttr << "4 5 4";
        }
        else
        {
            dimensionAttr << "4 4 4";
        }
    }
    else if (m_matrix->hasOffsets())
    {
        dimensionAttr << "3 4 3";
    }
    else
    {
        dimensionAttr << "3 3 3";
    }

    XmlFormatter::Attributes attributes;
    attributes.push_back(XmlFormatter::Attribute(ATTR_DIMENSION,
                                                 dimensionAttr.str()));

    m_formatter.writeStartTag(TAG_ARRAY, attributes);

    const ArrayDouble::Values& values = m_matrix->getArray().getValues();
    const MatrixOpData::Offsets& offsets = m_matrix->getOffsets();

    if (m_matrix->hasAlpha())
    {
        if (m_matrix->hasOffsets())
        {
            // Write in 4x5x4 mode.
            const double v[20]
            {
                values[0] , values[1] , values[2] , values[3] , offsets[0],
                values[4] , values[5] , values[6] , values[7] , offsets[1],
                values[8] , values[9] , values[10], values[11], offsets[2],
                values[12], values[13], values[14], values[15], offsets[3]
            };

            WriteValues(m_formatter, v, v + 20, 5, BIT_DEPTH_F32, 1);
        }
        else
        {
            // Write in 4x4x4 compact mode.
            const double v[16]
            {
                values[0] , values[1] , values[2] , values[3] ,
                values[4] , values[5] , values[6] , values[7] ,
                values[8] , values[9] , values[10], values[11],
                values[12], values[13], values[14], values[15]
            };

            WriteValues(m_formatter, v, v + 16, 4, BIT_DEPTH_F32, 1);
        }
    }
    else if (m_matrix->hasOffsets())
    {
        // Write in 3x4x3 compact mode.
        const double v[12]
        {
            values[0] , values[1] , values[2] , offsets[0],
            values[4] , values[5] , values[6] , offsets[1],
            values[8] , values[9] , values[10], offsets[2]
        };

        WriteValues(m_formatter, v, v + 12, 4, BIT_DEPTH_F32, 1);
    }
    else
    {
        // Write in 3x3x3 compact mode.
        const double v[9]
        {
            values[0] , values[1] , values[2] ,
            values[4] , values[5] , values[6] ,
            values[8] , values[9] , values[10]
        };

        WriteValues(m_formatter, v, v + 9, 3, BIT_DEPTH_F32, 1);
    }

    m_formatter.writeEndTag(TAG_ARRAY);
}

///////////////////////////////////////////////////////////////////////////////

class RangeWriter : public OpWriter
{
public:
    RangeWriter(XmlFormatter & formatter,
                ConstRangeOpDataRcPtr range);
    virtual ~RangeWriter();

protected:
    ConstOpDataRcPtr getOp() const override;
    const char * getTagName() const override;
    void writeContent() const override;

private:
    ConstRangeOpDataRcPtr m_range;
};

RangeWriter::RangeWriter(XmlFormatter & formatter,
                         ConstRangeOpDataRcPtr range)
    : OpWriter(formatter)
    , m_range(range)
{
}

RangeWriter::~RangeWriter()
{
}

ConstOpDataRcPtr RangeWriter::getOp() const
{
    return m_range;
}

const char * RangeWriter::getTagName() const
{
    return TAG_RANGE;
}

void WriteTag(XmlFormatter & fmt, const char * tag, double value)
{
    std::ostringstream o;
    o.precision(15);
    o << value;
    fmt.writeContentTag(tag, ' ' + o.str() + ' ');
}

void RangeWriter::writeContent() const
{
    if (!m_range->minIsEmpty())
    {
        WriteTag(m_formatter, TAG_MIN_IN_VALUE, m_range->getMinInValue());
    }

    if (!m_range->maxIsEmpty())
    {
        WriteTag(m_formatter, TAG_MAX_IN_VALUE, m_range->getMaxInValue());
    }

    if (!m_range->minIsEmpty())
    {
        WriteTag(m_formatter, TAG_MIN_OUT_VALUE, m_range->getMinOutValue());
    }

    if (!m_range->maxIsEmpty())
    {
        WriteTag(m_formatter, TAG_MAX_OUT_VALUE, m_range->getMaxOutValue());
    }
}

///////////////////////////////////////////////////////////////////////////////

CTFVersion GetOpMinimumVersion(const ConstOpDataRcPtr & op)
{
    CTFVersion minVersion{ CTF_PROCESS_LIST_VERSION };

    switch (op->getType())
    {
    case OpData::CDLType:
    case OpData::ExposureContrastType:
    case OpData::MatrixType:
    case OpData::RangeType:
    {
        minVersion = CTF_PROCESS_LIST_VERSION_1_3;
        break;
    }
    case OpData::FixedFunctionType:
    case OpData::LogType:
    {
        minVersion = CTF_PROCESS_LIST_VERSION_2_0;
        break;
    }
    case OpData::ExponentType:
    {
        auto exp = OCIO_DYNAMIC_POINTER_CAST<const ExponentOpData>(op);

        minVersion = (exp->m_exp4[3] == 1.) ? CTF_PROCESS_LIST_VERSION_1_3 :
                                              CTF_PROCESS_LIST_VERSION_1_5;
       
        break;
    }
    case OpData::GammaType:
    {
        auto gamma = OCIO_DYNAMIC_POINTER_CAST<const GammaOpData>(op);
        minVersion = gamma->isAlphaComponentIdentity() ?
            CTF_PROCESS_LIST_VERSION_1_3 :
            CTF_PROCESS_LIST_VERSION_1_5;
        break;
    }
    case OpData::Lut1DType:
    {
        auto lut = OCIO_DYNAMIC_POINTER_CAST<const Lut1DOpData>(op);
        if (lut->getDirection() == TRANSFORM_DIR_FORWARD)
        {
            minVersion = (lut->getHueAdjust() != Lut1DOpData::HUE_NONE) ?
                CTF_PROCESS_LIST_VERSION_1_4 :
                CTF_PROCESS_LIST_VERSION_1_3;
        }
        else
        {
            minVersion = (lut->getHueAdjust() != Lut1DOpData::HUE_NONE || lut->isInputHalfDomain()) ?
                CTF_PROCESS_LIST_VERSION_1_6 :
                CTF_PROCESS_LIST_VERSION_1_3;
        }
        break;
    }
    case OpData::Lut3DType:
    {
        auto lut = OCIO_DYNAMIC_POINTER_CAST<const Lut3DOpData>(op);
        if (lut->getDirection() == TRANSFORM_DIR_FORWARD)
        {
            minVersion = CTF_PROCESS_LIST_VERSION_1_3;
        }
        else
        {
            minVersion = CTF_PROCESS_LIST_VERSION_1_6;
        }
        break;
    }
    case OpData::ReferenceType:
    {
        throw Exception("Reference ops should have been replaced by their content.");
        break;
    }
    case OpData::NoOpType:
    {
        minVersion = CTF_PROCESS_LIST_VERSION_1_3;
        break;
    }
    }

    return minVersion;
}

CTFVersion GetMinimumVersion(const ConstCTFReaderTransformPtr & transform)
{
    auto & opList = transform->getOps();

    // Need to specify the minimum version here.  Some test transforms have no ops.
    CTFVersion minimumVersion = CTF_PROCESS_LIST_VERSION_1_3;

    for (auto & op : opList)
    {
        const CTFVersion version = GetOpMinimumVersion(op);
        if (version > minimumVersion)
        {
            minimumVersion = version;
        }
    }

    return minimumVersion;
}

} // Anonymous namespace.

///////////////////////////////////////////////////////////////////////////////

TransformWriter::TransformWriter(XmlFormatter & formatter,
                                 ConstCTFReaderTransformPtr transform,
                                 bool isCLF)
    : XmlElementWriter(formatter)
    , m_transform(transform)
    , m_isCLF(isCLF)
{
}

TransformWriter::~TransformWriter()
{
}

void TransformWriter::write() const
{
    std::string processListTag(TAG_PROCESS_LIST);

    XmlFormatter::Attributes attributes;

    std::ostringstream fversion;
    if (m_isCLF)
    {
        // Save with CLF version 2.
        fversion << 2;
        attributes.push_back(XmlFormatter::Attribute(ATTR_COMP_CLF_VERSION,
                                                     fversion.str()));

    }
    else
    {
        fversion << GetMinimumVersion(m_transform);
    
        attributes.push_back(XmlFormatter::Attribute(ATTR_VERSION,
                                                     fversion.str()));
    
    }

    std::string id = m_transform->getID();
    if (id.empty())
    {
        auto & ops = m_transform->getOps();
        for (auto op : ops)
        {
            id += op->getCacheID();
        }
        
        id = CacheIDHash(id.c_str(), (int)id.size());
    }
    attributes.push_back(XmlFormatter::Attribute(ATTR_ID, id));

    const std::string& name = m_transform->getName();
    if (!name.empty())
    {
        attributes.push_back(XmlFormatter::Attribute(ATTR_NAME, name));
    }

    const std::string & inverseOfId = m_transform->getInverseOfId();
    if (!inverseOfId.empty())
    {
        attributes.push_back(XmlFormatter::Attribute(ATTR_INVERSE_OF, inverseOfId));
    }

    m_formatter.writeStartTag(processListTag, attributes);
    {
        XmlScopeIndent scopeIndent(m_formatter);

        WriteDescriptions(m_formatter, m_transform->getDescriptions());

        const std::string & inputDesc = m_transform->getInputDescriptor();
        if (!inputDesc.empty())
        {
            m_formatter.writeContentTag(TAG_INPUT_DESCRIPTOR, inputDesc);
        }

        const std::string & outputDesc = m_transform->getOutputDescriptor();
        if (!outputDesc.empty())
        {
            m_formatter.writeContentTag(TAG_OUTPUT_DESCRIPTOR, outputDesc);
        }

        const FormatMetadataImpl & info = m_transform->getInfoMetadata();
        {
            writeProcessListMetadata(info);
        }

        writeOps();
    }

    m_formatter.writeEndTag(processListTag);
}

void TransformWriter::writeProcessListMetadata(const FormatMetadataImpl& m) const
{
    if (m.getChildrenElements().size() == 0)
    {
        const std::string infoValue(m.getValue());
        if (m.getNumAttributes() || !infoValue.empty())
        {
            m_formatter.writeContentTag(m.getName(), m.getAttributes(), m.getValue());
        }
    }
    else
    {
        m_formatter.writeStartTag(m.getName(), m.getAttributes());
        const std::string value{ m.getValue() };
        if (!value.empty())
        {
            m_formatter.writeContent(m.getValue());
        }

        const auto items = m.getChildrenElements();
        for (auto it = items.begin(), end = items.end(); it != end; ++it)
        {
            XmlScopeIndent scopeIndent(m_formatter);
            writeProcessListMetadata(*it);
        }

        m_formatter.writeEndTag(m.getName());
    }
}

namespace
{
void ThrowWriteOp(const std::string & type)
{
    std::ostringstream oss;
    oss << "Transform uses the " << type << " op which cannot be written "
           "as CLF.  Use CTF format or Bake the transform.";
    throw Exception(oss.str().c_str());
}
}

void TransformWriter::writeOps() const
{
    auto & ops = m_transform->getOps();
    for (auto op : ops)
    {
        static_assert(OpData::NoOpType == 11, "Add new types");
        switch (op->getType())
        {
        case OpData::CDLType:
        {
            auto cdl = OCIO_DYNAMIC_POINTER_CAST<const CDLOpData>(op);
            CDLWriter opWriter(m_formatter, cdl);
            opWriter.write();
            break;
        }
        case OpData::ExponentType:
        {
            if (m_isCLF)
            {
                ThrowWriteOp("Exponent");
            }
            auto exp = OCIO_DYNAMIC_POINTER_CAST<const ExponentOpData>(op);

            GammaOpData::Params paramR{ exp->m_exp4[0] };
            GammaOpData::Params paramG{ exp->m_exp4[1] };
            GammaOpData::Params paramB{ exp->m_exp4[2] };
            GammaOpData::Params paramA{ exp->m_exp4[3] };

            GammaOpDataRcPtr gammaData
                = std::make_shared<GammaOpData>(BIT_DEPTH_F32, BIT_DEPTH_F32,
                                                exp->getFormatMetadata(),
                                                GammaOpData::BASIC_FWD,
                                                paramR, paramG, paramB, paramA);

            GammaWriter opWriter(m_formatter, gammaData);
            opWriter.write();
            break;
        }
        case OpData::ExposureContrastType:
        {
            if (m_isCLF)
            {
                ThrowWriteOp("ExposureContrast");
            }

            auto ec = OCIO_DYNAMIC_POINTER_CAST<const ExposureContrastOpData>(op);
            ExposureContrastWriter opWriter(m_formatter, ec);
            opWriter.write();
            break;
        }
        case OpData::FixedFunctionType:
        {
            if (m_isCLF) 
            {
                ThrowWriteOp("FixedFunction");
            }

            auto ff = OCIO_DYNAMIC_POINTER_CAST<const FixedFunctionOpData>(op);
            FixedFunctionWriter opWriter(m_formatter, ff);
            opWriter.write();
            break;
        }
        case OpData::GammaType:
        {
            if (m_isCLF)
            {
                ThrowWriteOp("Gamma");
            }

            auto gamma = OCIO_DYNAMIC_POINTER_CAST<const GammaOpData>(op);
            GammaWriter opWriter(m_formatter, gamma);
            opWriter.write();
            break;
        }
        case OpData::LogType:
        {
            if (m_isCLF) 
            {
                ThrowWriteOp("Log");
            }

            auto log = OCIO_DYNAMIC_POINTER_CAST<const LogOpData>(op);
            LogWriter opWriter(m_formatter, log);
            opWriter.write();
            break;
        }
        case OpData::Lut1DType:
        {
            auto lut = OCIO_DYNAMIC_POINTER_CAST<const Lut1DOpData>(op);
            Lut1DWriter opWriter(m_formatter, lut);
            opWriter.write();
            break;
        }
        case OpData::Lut3DType:
        {
            auto lut = OCIO_DYNAMIC_POINTER_CAST<const Lut3DOpData>(op);
            Lut3DWriter opWriter(m_formatter, lut);
            opWriter.write();
            break;
        }
        case OpData::MatrixType:
        {
            auto mat = OCIO_DYNAMIC_POINTER_CAST<const MatrixOpData>(op);
            MatrixWriter opWriter(m_formatter, mat);
            opWriter.write();
            break;
        }
        case OpData::RangeType:
        {
            auto range = OCIO_DYNAMIC_POINTER_CAST<const RangeOpData>(op);
            RangeWriter opWriter(m_formatter, range);
            opWriter.write();
            break;
        }
        case OpData::ReferenceType:
        {
            throw Exception("Reference ops should have been replaced by their content.");
            break;
        }
        case OpData::NoOpType:
        {
            break;
        }
        }
    }
}

}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "transforms/FileTransform.h"
#include "ops/Matrix/MatrixOpData.h"
#include "UnitTest.h"
#include "UnitTestUtils.h"

OCIO_ADD_TEST(CTFVersion, read_version)
{
    {
        const OCIO::CTFVersion version1(1, 2, 3);
        const OCIO::CTFVersion version2(1, 2, 3);
        OCIO_CHECK_EQUAL(version1, version2);
        {
            const OCIO::CTFVersion version3(0, 0, 1);
            OCIO_CHECK_ASSERT(false == (version1 == version3));
            OCIO_CHECK_ASSERT(version3 < version1);
        }
        {
            const OCIO::CTFVersion version3(0, 1, 0);
            OCIO_CHECK_ASSERT(false == (version1 == version3));
            OCIO_CHECK_ASSERT(version3 < version1);
        }
        {
            const OCIO::CTFVersion version3(1, 0, 0);
            OCIO_CHECK_ASSERT(false == (version1 == version3));
            OCIO_CHECK_ASSERT(version3 < version1);
        }
        {
            const OCIO::CTFVersion version3(1, 2, 0);
            OCIO_CHECK_ASSERT(false == (version1 == version3));
            OCIO_CHECK_ASSERT(version3 < version1);
        }
        {
            const OCIO::CTFVersion version3(1, 2, 2);
            OCIO_CHECK_ASSERT(false == (version1 == version3));
            OCIO_CHECK_ASSERT(version3 < version1);
        }
    }

    OCIO::CTFVersion versionRead;
    {
        OCIO_CHECK_NO_THROW(OCIO::CTFVersion::ReadVersion("1.2.3", versionRead));
        const OCIO::CTFVersion version(1, 2, 3);
        OCIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OCIO_CHECK_NO_THROW(OCIO::CTFVersion::ReadVersion("1.2", versionRead));
        const OCIO::CTFVersion version(1, 2, 0);
        OCIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OCIO_CHECK_NO_THROW(OCIO::CTFVersion::ReadVersion("1", versionRead));
        const OCIO::CTFVersion version(1, 0, 0);
        OCIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OCIO_CHECK_NO_THROW(OCIO::CTFVersion::ReadVersion("1.10", versionRead));
        const OCIO::CTFVersion version(1, 10, 0);
        OCIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OCIO_CHECK_NO_THROW(OCIO::CTFVersion::ReadVersion("1.1.0", versionRead));
        const OCIO::CTFVersion version(1, 1, 0);
        OCIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OCIO_CHECK_NO_THROW(OCIO::CTFVersion::ReadVersion("1.01", versionRead));
        const OCIO::CTFVersion version(1, 1, 0);
        OCIO_CHECK_EQUAL(version, versionRead);
    }

    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion("", versionRead),
                          OCIO::Exception, 
                          "is not a valid version");
    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion("1 2", versionRead),
                          OCIO::Exception, 
                          "is not a valid version");
    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion("1-2", versionRead),
                          OCIO::Exception, 
                          "is not a valid version");
    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion("a", versionRead),
                          OCIO::Exception, 
                          "is not a valid version");
    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion("1.", versionRead),
                          OCIO::Exception, 
                          "is not a valid version");
    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion(".2", versionRead),
                          OCIO::Exception, 
                          "is not a valid version");
    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion("1.0 2", versionRead),
                          OCIO::Exception,
                          "is not a valid version");
    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion("-1", versionRead),
                          OCIO::Exception,
                          "is not a valid version");
}

OCIO_ADD_TEST(CTFVersion, version_write)
{
    {
        const OCIO::CTFVersion version(1, 2, 3);
        std::ostringstream ostream;
        ostream << version;
        OCIO_CHECK_EQUAL(ostream.str(), "1.2.3");
    }
    {
        const OCIO::CTFVersion version(1, 0, 3);
        std::ostringstream ostream;
        ostream << version;
        OCIO_CHECK_EQUAL(ostream.str(), "1.0.3");
    }
    {
        const OCIO::CTFVersion version(1, 2, 0);
        std::ostringstream ostream;
        ostream << version;
        OCIO_CHECK_EQUAL(ostream.str(), "1.2");
    }
    {
        const OCIO::CTFVersion version(1, 20, 0);
        std::ostringstream ostream;
        ostream << version;
        OCIO_CHECK_EQUAL(ostream.str(), "1.20");
    }
    {
        const OCIO::CTFVersion version(1, 0, 0);
        std::ostringstream ostream;
        ostream << version;
        OCIO_CHECK_EQUAL(ostream.str(), "1");
    }
    {
        const OCIO::CTFVersion version(0, 0, 0);
        std::ostringstream ostream;
        ostream << version;
        OCIO_CHECK_EQUAL(ostream.str(), "0");
    }
}

OCIO_ADD_TEST(CTFReaderTransform, accessors)
{
    OCIO::CTFReaderTransform t;
    {
        const OCIO::CTFReaderTransform & ct = t;

        OCIO::FormatMetadataImpl & info = t.getInfoMetadata();
        const OCIO::FormatMetadataImpl & cinfo = t.getInfoMetadata();

        OCIO_CHECK_EQUAL(std::string(info.getName()), OCIO::METADATA_INFO);
        OCIO_CHECK_EQUAL(std::string(cinfo.getName()), OCIO::METADATA_INFO);

        OCIO_CHECK_EQUAL(t.getID(), "");
        OCIO_CHECK_EQUAL(ct.getID(), "");
        OCIO_CHECK_EQUAL(t.getName(), "");
        OCIO_CHECK_EQUAL(ct.getName(), "");
        OCIO_CHECK_EQUAL(t.getInverseOfId(), "");
        OCIO_CHECK_EQUAL(ct.getInverseOfId(), "");
        OCIO_CHECK_EQUAL(t.getInputDescriptor(), "");
        OCIO_CHECK_EQUAL(ct.getInputDescriptor(), "");
        OCIO_CHECK_EQUAL(t.getOutputDescriptor(), "");
        OCIO_CHECK_EQUAL(ct.getOutputDescriptor(), "");

        OCIO_CHECK_ASSERT(t.getOps().empty());
        OCIO_CHECK_ASSERT(ct.getOps().empty());

        OCIO_CHECK_ASSERT(t.getDescriptions().empty());
        OCIO_CHECK_ASSERT(ct.getDescriptions().empty());
    }
    t.setName("Name");
    t.setID("123");
    t.setInverseOfId("654");
    t.setInputDescriptor("input");
    t.setOutputDescriptor("output");
    
    auto matrixOp = std::make_shared<OCIO::MatrixOpData>();
    t.getOps().push_back(matrixOp);

    t.getDescriptions().push_back("One");
    t.getDescriptions().push_back("Two");

    {
        const OCIO::CTFReaderTransform & ct = t;

        OCIO_CHECK_EQUAL(t.getID(), "123");
        OCIO_CHECK_EQUAL(ct.getID(), "123");
        OCIO_CHECK_EQUAL(t.getName(), "Name");
        OCIO_CHECK_EQUAL(ct.getName(), "Name");
        OCIO_CHECK_EQUAL(t.getInverseOfId(), "654");
        OCIO_CHECK_EQUAL(ct.getInverseOfId(), "654");
        OCIO_CHECK_EQUAL(t.getInputDescriptor(), "input");
        OCIO_CHECK_EQUAL(ct.getInputDescriptor(), "input");
        OCIO_CHECK_EQUAL(t.getOutputDescriptor(), "output");
        OCIO_CHECK_EQUAL(ct.getOutputDescriptor(), "output");

        OCIO_CHECK_EQUAL(t.getOps().size(), 1);
        OCIO_CHECK_EQUAL(ct.getOps().size(), 1);

        OCIO_CHECK_EQUAL(t.getDescriptions().size(), 2);
        OCIO_CHECK_EQUAL(ct.getDescriptions().size(), 2);
        OCIO_CHECK_EQUAL(t.getDescriptions()[0], "One");
        OCIO_CHECK_EQUAL(ct.getDescriptions()[0], "One");
        OCIO_CHECK_EQUAL(t.getDescriptions()[1], "Two");
        OCIO_CHECK_EQUAL(ct.getDescriptions()[1], "Two");
    }
}

OCIO_ADD_TEST(CTFReaderTransform, validate)
{
    OCIO::CTFReaderTransform t;
    auto matrix = std::make_shared<OCIO::MatrixOpData>(OCIO::BIT_DEPTH_UINT10,
                                                       OCIO::BIT_DEPTH_F32);
    t.getOps().push_back(matrix);

    matrix = std::make_shared<OCIO::MatrixOpData>(OCIO::BIT_DEPTH_F32,
                                                  OCIO::BIT_DEPTH_F32);

    t.getOps().push_back(matrix);

    OCIO_CHECK_NO_THROW(t.validate());

    matrix = std::make_shared<OCIO::MatrixOpData>(OCIO::BIT_DEPTH_F16,
                                                  OCIO::BIT_DEPTH_F32);
    t.getOps().push_back(matrix);

    OCIO_CHECK_THROW_WHAT(t.validate(), OCIO::Exception,
                          "Bitdepth missmatch between ops");

    matrix->setInputBitDepth(OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_NO_THROW(t.validate());
}


OCIO_ADD_TEST(CTFTransform, load_save_matrix)
{
    const std::string ctfFile("matrix_example.clf");
    OCIO::ConstProcessorRcPtr processor;
    OCIO_CHECK_NO_THROW(processor = OCIO::GetFileTransformProcessor(ctfFile));
    OCIO_REQUIRE_ASSERT(processor);

    // Create empty Config to use.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    std::ostringstream outputTransform;
    processor->write(OCIO::FILEFORMAT_CTF, outputTransform);

    std::istringstream inputTransform;
    inputTransform.str(outputTransform.str());
    
    // Output matrix array as '3 3 3'.
    std::string line;
    OCIO_CHECK_NO_THROW(std::getline(inputTransform, line));
    OCIO_CHECK_EQUAL(line, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");

    OCIO_CHECK_NO_THROW(std::getline(inputTransform, line));
    OCIO_CHECK_EQUAL(line, "<ProcessList version=\"1.3\""
                           " id=\"b5cc7aed-d405-4d8b-b64b-382b2341a378\""
                           " name=\"matrix example\">");
    OCIO_CHECK_NO_THROW(std::getline(inputTransform, line));
    OCIO_CHECK_EQUAL(pystring::strip(line),
                     "<Description>Convert output-referred XYZ values to "
                     "linear RGB.</Description>");
    OCIO_CHECK_NO_THROW(std::getline(inputTransform, line));
    OCIO_CHECK_EQUAL(pystring::strip(line),
                     "<InputDescriptor>XYZ</InputDescriptor>");
    OCIO_CHECK_NO_THROW(std::getline(inputTransform, line));
    OCIO_CHECK_EQUAL(pystring::strip(line),
                     "<OutputDescriptor>RGB</OutputDescriptor>");
    OCIO_CHECK_NO_THROW(std::getline(inputTransform, line));
    OCIO_CHECK_EQUAL(pystring::strip(line),
                     "<Matrix id=\"c61daf06-539f-4254-81fc-9800e6d02a37\""
                     " inBitDepth=\"32f\" outBitDepth=\"32f\">");
    OCIO_CHECK_NO_THROW(std::getline(inputTransform, line));
    OCIO_CHECK_EQUAL(pystring::strip(line),
                    "<Description>XYZ to sRGB matrix</Description>");
    OCIO_CHECK_NO_THROW(std::getline(inputTransform, line));
    OCIO_CHECK_EQUAL(pystring::strip(line), "<Array dim=\"3 3 3\">");
    OCIO_CHECK_NO_THROW(std::getline(inputTransform, line));
    OCIO_CHECK_EQUAL(pystring::strip(line), "3.24              -1.537             -0.4985");
    OCIO_CHECK_NO_THROW(std::getline(inputTransform, line));
    OCIO_CHECK_EQUAL(pystring::strip(line), "-0.9693               1.876             0.04156");
    OCIO_CHECK_NO_THROW(std::getline(inputTransform, line));
    OCIO_CHECK_EQUAL(pystring::strip(line), "0.0556              -0.204              1.0573");
}

OCIO_ADD_TEST(CTFTransform, save_matrix_444)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    OCIO::MatrixTransformRcPtr mat = OCIO::MatrixTransform::Create();
    double m[16]{  1.,  0., 0., 0.,
                   0.,  1., 0., 0., 
                   0.,  0., 1., 0., 
                  0.5, 0.5, 0., 1. };
    mat->setMatrix(m);
    OCIO::ConstProcessorRcPtr processor = config->getProcessor(mat);

    OCIO::GroupTransformRcPtr group = processor->createGroupTransform();
    OCIO_CHECK_EQUAL(group->size(), 1);

    std::ostringstream outputTransform;
    processor->write(OCIO::FILEFORMAT_CTF, outputTransform);

    // Output matrix array as '4 4 4'.
    OCIO_CHECK_NE(outputTransform.str().find("\"4 4 4\""), std::string::npos);
}

OCIO_ADD_TEST(CTFTransform, load_edit_save_matrix)
{
    const std::string ctfFile("matrix_example.clf");
    OCIO::ConstProcessorRcPtr processor;
    OCIO_CHECK_NO_THROW(processor = OCIO::GetFileTransformProcessor(ctfFile));
    OCIO_REQUIRE_ASSERT(processor);
    OCIO::GroupTransformRcPtr group = processor->createGroupTransform();

    group->getFormatMetadata().addAttribute(OCIO::ATTR_INVERSE_OF, "added inverseOf");
    group->getFormatMetadata().addAttribute("Unknown", "not saved");
    group->getFormatMetadata().addChildElement("Unknown", "not saved");
    auto & info = group->getFormatMetadata().addChildElement(OCIO::METADATA_INFO, "Preserved");
    info.addAttribute("attrib", "value");
    info.addChildElement("Child", "Preserved");

    OCIO_REQUIRE_EQUAL(group->size(), 1);
    auto transform = group->getTransform(0);
    auto matTrans = OCIO::DynamicPtrCast<OCIO::MatrixTransform>(transform);

    // Validate how escape characters are saved.
    const std::string shortName("A ' short ' \" name");
    const std::string description1("A \" short \" description with a ' inside");
    const std::string description2("<test\"'&>");
    auto & desc = matTrans->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, description1.c_str());
    desc.addAttribute("Unknown", "not saved");
    matTrans->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, description2.c_str());
    
    matTrans->getFormatMetadata().addAttribute(OCIO::METADATA_NAME, shortName.c_str());

    const double offset[] = {0.1, 1.2, 2.3456789123456, 0.0};
    matTrans->setOffset(offset);

    // Create empty Config to use.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    // Get the processor corresponding to the transform.
    OCIO::ConstProcessorRcPtr processorGroup = config->getProcessor(group);

    std::ostringstream outputTransform;
    processorGroup->write(OCIO::FILEFORMAT_CTF, outputTransform);

    // Output matrix array as '3 4 3'.
    const std::string expectedCTF{
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<ProcessList version=\"1.3\" id=\"b5cc7aed-d405-4d8b-b64b-382b2341a378\" name=\"matrix example\" inverseOf=\"added inverseOf\">\n"
        "    <Description>Convert output-referred XYZ values to linear RGB.</Description>\n"
        "    <InputDescriptor>XYZ</InputDescriptor>\n"
        "    <OutputDescriptor>RGB</OutputDescriptor>\n"
        "    <Info attrib=\"value\">\n"
        "    Preserved\n"
        "        <Child>Preserved</Child>\n"
        "    </Info>\n"
        "    <Matrix id=\"c61daf06-539f-4254-81fc-9800e6d02a37\" name=\"A &apos; short &apos; &quot; name\" inBitDepth=\"32f\" outBitDepth=\"32f\">\n"
        "        <Description>XYZ to sRGB matrix</Description>\n"
        "        <Description>A &quot; short &quot; description with a &apos; inside</Description>\n"
        "        <Description>&lt;test&quot;&apos;&amp;&gt;</Description>\n"
        "        <Array dim=\"3 4 3\">\n"
        "               3.24              -1.537             -0.4985                 0.1\n"
        "            -0.9693               1.876             0.04156                 1.2\n"
        "             0.0556              -0.204              1.0573     2.3456789123456\n"
        "        </Array>\n"
        "    </Matrix>\n"
        "</ProcessList>\n" };

    OCIO_CHECK_EQUAL(expectedCTF.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expectedCTF, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, load_edit_save_matrix_clf)
{
    const std::string ctfFile("matrix_example.clf");
    OCIO::ConstProcessorRcPtr processor;
    OCIO_CHECK_NO_THROW(processor = OCIO::GetFileTransformProcessor(ctfFile));
    OCIO_REQUIRE_ASSERT(processor);
    OCIO::GroupTransformRcPtr group = processor->createGroupTransform();
    OCIO_REQUIRE_EQUAL(group->size(), 1);
    auto transform = group->getTransform(0);
    auto matTrans = OCIO::DynamicPtrCast<OCIO::MatrixTransform>(transform);
    const std::string newDescription{ "Added description" };
    matTrans->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, newDescription.c_str());
    const double offset[] = { 0.1, 1.2, 2.3, 1.0 };
    matTrans->setOffset(offset);

    // Create empty Config to use.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    // Get the processor corresponding to the transform.
    OCIO::ConstProcessorRcPtr processorGroup = config->getProcessor(group);

    std::ostringstream outputTransform;
    processorGroup->write(OCIO::FILEFORMAT_CLF, outputTransform);

    // Output matrix array as '4 5 4'.
    const std::string expectedCLF{
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<ProcessList compCLFversion=\"2\" id=\"b5cc7aed-d405-4d8b-b64b-382b2341a378\" name=\"matrix example\">\n"
        "    <Description>Convert output-referred XYZ values to linear RGB.</Description>\n"
        "    <InputDescriptor>XYZ</InputDescriptor>\n"
        "    <OutputDescriptor>RGB</OutputDescriptor>\n"
        "    <Matrix id=\"c61daf06-539f-4254-81fc-9800e6d02a37\" inBitDepth=\"32f\" outBitDepth=\"32f\">\n"
        "        <Description>XYZ to sRGB matrix</Description>\n"
        "        <Description>Added description</Description>\n"
        "        <Array dim=\"4 5 4\">\n"
        "               3.24              -1.537             -0.4985                   0                 0.1\n"
        "            -0.9693               1.876             0.04156                   0                 1.2\n"
        "             0.0556              -0.204              1.0573                   0                 2.3\n"
        "                  0                   0                   0                   1                   1\n"
        "        </Array>\n"
        "    </Matrix>\n"
        "</ProcessList>\n" };

    OCIO_CHECK_EQUAL(expectedCLF.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expectedCLF, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, legacy_cdl)
{
    // Create empty legacy Config to use.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(1);

    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    const double sop[] = { 1.0, 1.1, 1.2,
                           0.2, 0.3, 0.4,
                           3.1, 3.2, 3.3 };
    cdl->setSOP(sop);
    cdl->setSat(2.1);
    
    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    // Need to specify an id so that it does not get generated.
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "cdl0");

    group->push_back(cdl);

    // Get the processor corresponding to the transform.
    OCIO::ConstProcessorRcPtr processorGroup = config->getProcessor(group);

    std::ostringstream outputTransform;
    processorGroup->write(OCIO::FILEFORMAT_CTF, outputTransform);

    // For OCIO v1, an ASC CDL was implemented as a Matrix/Gamma/Matrix rather
    // than as a dedicated op as in v2 and onward.
    const std::string expected{
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<ProcessList version=\"1.3\" id=\"cdl0\">\n"
        "    <Matrix inBitDepth=\"32f\" outBitDepth=\"32f\">\n"
        "        <Array dim=\"3 4 3\">\n"
        "                  1                   0                   0                 0.2\n"
        "                  0                 1.1                   0                 0.3\n"
        "                  0                   0                 1.2                 0.4\n"
        "        </Array>\n"
        "    </Matrix>\n"
        "    <Gamma inBitDepth=\"32f\" outBitDepth=\"32f\" style=\"basicFwd\">\n"
        "        <GammaParams channel=\"R\" gamma=\"3.1\" />\n"
        "        <GammaParams channel=\"G\" gamma=\"3.2\" />\n"
        "        <GammaParams channel=\"B\" gamma=\"3.3\" />\n"
        "        <GammaParams channel=\"A\" gamma=\"1\" />\n"
        "    </Gamma>\n"
        // Output matrix array as '3 3 3'.
        "    <Matrix inBitDepth=\"32f\" outBitDepth=\"32f\">\n"
        "        <Array dim=\"3 3 3\">\n"
        "            1.86614            -0.78672            -0.07942\n"
        "           -0.23386             1.31328            -0.07942\n"
        "           -0.23386            -0.78672             2.02058\n"
        "        </Array>\n"
        "    </Matrix>\n"
        "</ProcessList>\n" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, cdl_clf)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    const double sop[] = { 1.0, 1.1, 1.2,
                           0.2, 0.3, 0.4,
                           3.1, 3.2, 3.3 };
    cdl->setSOP(sop);
    cdl->setSat(2.1);
    cdl->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, "CDL node for unit test");
    cdl->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, "Adding another description");
    cdl->getFormatMetadata().addAttribute(OCIO::METADATA_NAME, "TestCDL");
    cdl->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "CDL42");

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    // Need to specify an id so that it does not get generated.
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "cdl1");

    group->push_back(cdl);

    auto & info = group->getFormatMetadata().addChildElement(OCIO::METADATA_INFO);
    info.addChildElement("Release", "2019");
    auto & sub = info.addChildElement("Directors");
    auto & subSub0 = sub.addChildElement("Director");
    subSub0.addAttribute("FirstName", "David");
    subSub0.addAttribute("LastName", "Cronenberg");
    auto & subSub1 = sub.addChildElement("Director");
    subSub1.addAttribute("FirstName", "David");
    subSub1.addAttribute("LastName", "Lynch");
    auto & subSub2 = sub.addChildElement("Director");
    subSub2.addAttribute("FirstName", "David");
    subSub2.addAttribute("LastName", "Fincher");
    auto & subSub3 = sub.addChildElement("Director");
    subSub3.addAttribute("FirstName", "David");
    subSub3.addAttribute("LastName", "Lean");

    // Get the processor corresponding to the transform.
    OCIO::ConstProcessorRcPtr processorGroup = config->getProcessor(group);

    std::ostringstream outputTransform;
    processorGroup->write(OCIO::FILEFORMAT_CLF, outputTransform);

    const std::string expected{
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<ProcessList compCLFversion=\"2\" id=\"cdl1\">\n"
        "    <Info>\n"
        "        <Release>2019</Release>\n"
        "        <Directors>\n"
        "            <Director FirstName=\"David\" LastName=\"Cronenberg\"></Director>\n"
        "            <Director FirstName=\"David\" LastName=\"Lynch\"></Director>\n"
        "            <Director FirstName=\"David\" LastName=\"Fincher\"></Director>\n"
        "            <Director FirstName=\"David\" LastName=\"Lean\"></Director>\n"
        "        </Directors>\n"
        "    </Info>\n"
        "    <ASC_CDL id=\"CDL42\" name=\"TestCDL\" inBitDepth=\"32f\" outBitDepth=\"32f\" style=\"Fwd\">\n"
        "        <Description>CDL node for unit test</Description>\n"
        "        <Description>Adding another description</Description>\n"
        "        <SOPNode>\n"
        "            <Slope>1, 1.1, 1.2</Slope>\n"
        "            <Offset>0.2, 0.3, 0.4</Offset>\n"
        "            <Power>3.1, 3.2, 3.3</Power>\n"
        "        </SOPNode>\n"
        "        <SatNode>\n"
        "            <Saturation>2.1</Saturation>\n"
        "        </SatNode>\n"
        "    </ASC_CDL>\n"
        "</ProcessList>\n" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, cdl_ctf)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    OCIO::CDLTransformRcPtr cdl = OCIO::CDLTransform::Create();
    const double sop[] = { 1.0, 1.1, 1.2,
                           0.2, 0.3, 0.4,
                           3.1, 3.2, 3.3 };
    cdl->setSOP(sop);
    cdl->setSat(2.1);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    // Need to specify an id so that it does not get generated.
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "cdl2");

    group->push_back(cdl);

    // Get the processor corresponding to the transform.
    OCIO::ConstProcessorRcPtr processorGroup = config->getProcessor(group);

    std::ostringstream outputTransform;
    processorGroup->write(OCIO::FILEFORMAT_CTF, outputTransform);

    const std::string expected{
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<ProcessList version=\"1.3\" id=\"cdl2\">\n"
        "    <ASC_CDL inBitDepth=\"32f\" outBitDepth=\"32f\" style=\"Fwd\">\n"
        "        <SOPNode>\n"
        "            <Slope>1, 1.1, 1.2</Slope>\n"
        "            <Offset>0.2, 0.3, 0.4</Offset>\n"
        "            <Power>3.1, 3.2, 3.3</Power>\n"
        "        </SOPNode>\n"
        "        <SatNode>\n"
        "            <Saturation>2.1</Saturation>\n"
        "        </SatNode>\n"
        "    </ASC_CDL>\n"
        "</ProcessList>\n" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, range_ctf)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    // Non-clamping range are converted to matrix.
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setStyle(OCIO::RANGE_NO_CLAMP);
    range->setMinInValue(0.1);
    range->setMaxInValue(0.9);
    range->setMinOutValue(0.0);
    range->setMaxOutValue(1.2);
    range->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, "Range node for unit test");
    range->getFormatMetadata().addAttribute(OCIO::METADATA_NAME, "TestRange");
    range->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "Range42");

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    // Need to specify an id so that it does not get generated.
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "mat0");
    
    group->getFormatMetadata().addChildElement(OCIO::TAG_INPUT_DESCRIPTOR, "Input descpriptor");
    group->getFormatMetadata().addChildElement(OCIO::TAG_OUTPUT_DESCRIPTOR, "Output descpriptor");

    group->push_back(range);

    OCIO::ConstProcessorRcPtr processorGroup = config->getProcessor(group);
    std::ostringstream outputTransform;
    processorGroup->write(OCIO::FILEFORMAT_CTF, outputTransform);

    const std::string expected{
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<ProcessList version=\"1.3\" id=\"mat0\">\n"
        "    <InputDescriptor>Input descpriptor</InputDescriptor>\n"
        "    <OutputDescriptor>Output descpriptor</OutputDescriptor>\n"
        "    <Matrix id=\"Range42\" name=\"TestRange\" inBitDepth=\"32f\" outBitDepth=\"32f\">\n"
        "        <Description>Range node for unit test</Description>\n"
        "        <Array dim=\"3 4 3\">\n"
        "                1.5                   0                   0               -0.15\n"
        "                  0                 1.5                   0               -0.15\n"
        "                  0                   0                 1.5               -0.15\n"
        "        </Array>\n"
        "    </Matrix>\n"
        "</ProcessList>\n" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, range_clf)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setStyle(OCIO::RANGE_CLAMP);
    range->setMinInValue(0.1);
    range->setMaxInValue(0.9);
    range->setMinOutValue(0.0);
    range->setMaxOutValue(1.2);
    range->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, "Range node for unit test");
    range->getFormatMetadata().addAttribute(OCIO::METADATA_NAME, "TestRange");
    range->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "Range42");

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");
    group->getFormatMetadata().addChildElement(OCIO::TAG_INPUT_DESCRIPTOR, "Input descpriptor");
    group->getFormatMetadata().addChildElement(OCIO::TAG_OUTPUT_DESCRIPTOR, "Output descpriptor");
    group->push_back(range);

    OCIO::ConstProcessorRcPtr processorGroup = config->getProcessor(group);
    std::ostringstream outputTransform;
    processorGroup->write(OCIO::FILEFORMAT_CLF, outputTransform);

    const std::string expected{
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<ProcessList compCLFversion=\"2\" id=\"UID42\">\n"
        "    <InputDescriptor>Input descpriptor</InputDescriptor>\n"
        "    <OutputDescriptor>Output descpriptor</OutputDescriptor>\n"
        "    <Range id=\"Range42\" name=\"TestRange\" inBitDepth=\"32f\" outBitDepth=\"32f\">\n"
        "        <Description>Range node for unit test</Description>\n"
        "        <minInValue> 0.1 </minInValue>\n"
        "        <maxInValue> 0.9 </maxInValue>\n"
        "        <minOutValue> 0 </minOutValue>\n"
        "        <maxOutValue> 1.2 </maxOutValue>\n"
        "    </Range>\n"
        "</ProcessList>\n" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, exponent_ctf)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    OCIO::ExponentWithLinearTransformRcPtr exp = OCIO::ExponentWithLinearTransform::Create();
    const double gamma[] = { 1.1, 1.2, 1.3, 1.0 };
    exp->setGamma(gamma);
    const double offset[] = { 0.1, 0.2, 0.1, 0.0 };
    exp->setOffset(offset);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UID42");
    group->push_back(exp);

    OCIO::ConstProcessorRcPtr processorGroup = config->getProcessor(group);
    std::ostringstream outputTransform;
    processorGroup->write(OCIO::FILEFORMAT_CTF, outputTransform);

    const std::string expected{
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<ProcessList version=\"1.3\" id=\"UID42\">\n"
        "    <Gamma inBitDepth=\"32f\" outBitDepth=\"32f\" style=\"moncurveFwd\">\n"
        "        <GammaParams channel=\"R\" gamma=\"1.1\" offset=\"0.1\" />\n"
        "        <GammaParams channel=\"G\" gamma=\"1.2\" offset=\"0.2\" />\n"
        "        <GammaParams channel=\"B\" gamma=\"1.3\" offset=\"0.1\" />\n"
        "        <GammaParams channel=\"A\" gamma=\"1\" offset=\"0\" />\n"
        "    </Gamma>\n"
        "</ProcessList>\n" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, fixed_function_ctf)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    OCIO::FixedFunctionTransformRcPtr ff = OCIO::FixedFunctionTransform::Create();
    ff->setStyle(OCIO::FIXED_FUNCTION_REC2100_SURROUND);
    const double val = 0.5;
    ff->setParams(&val, 1);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDFF42");
    group->push_back(ff);

    OCIO::ConstProcessorRcPtr processorGroup = config->getProcessor(group);
    std::ostringstream outputTransform;
    processorGroup->write(OCIO::FILEFORMAT_CTF, outputTransform);

    const std::string expected{
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<ProcessList version=\"2\" id=\"UIDFF42\">\n"
        "    <FixedFunction inBitDepth=\"32f\" outBitDepth=\"32f\" style=\"Rec2100Surround\" params=\"0.5\">\n"
        "    </FixedFunction>\n"
        "</ProcessList>\n" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

OCIO_ADD_TEST(CTFTransform, exposure_contrast_ctf)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    OCIO::ExposureContrastTransformRcPtr ec = OCIO::ExposureContrastTransform::Create();

    ec->setStyle(OCIO::EXPOSURE_CONTRAST_LOGARITHMIC);

    ec->setExposure(0.65);
    ec->setContrast(1.2);
    ec->setGamma(0.5);
    ec->setPivot(1.0);
    ec->setLogExposureStep(0.1);
    ec->setLogMidGray(0.5);

    ec->makeExposureDynamic();

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "UIDEC42");
    group->push_back(ec);

    OCIO::ConstProcessorRcPtr processorGroup = config->getProcessor(group);
    std::ostringstream outputTransform;
    processorGroup->write(OCIO::FILEFORMAT_CTF, outputTransform);

    const std::string expected{
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<ProcessList version=\"1.3\" id=\"UIDEC42\">\n"
        "    <ExposureContrast inBitDepth=\"32f\" outBitDepth=\"32f\" style=\"log\">\n"
        "        <ECParams exposure=\"0.65\" contrast=\"1.2\" gamma=\"0.5\" pivot=\"1\" logExposureStep=\"0.1\" logMidGray=\"0.5\" />\n"
        "        <DynamicParameter param=\"EXPOSURE\" />\n"
        "    </ExposureContrast>\n"
        "</ProcessList>\n" };

    OCIO_CHECK_EQUAL(expected.size(), outputTransform.str().size());
    OCIO_CHECK_EQUAL(expected, outputTransform.str());
}

// TODO:  Bring over tests.
// synColor: xmlTransformWriter_test.cpp


#endif

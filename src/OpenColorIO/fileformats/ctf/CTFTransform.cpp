// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include "BitDepthUtils.h"
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
#include "transforms/CDLTransform.h"

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

CTFVersion GetOpMinimumVersion(const ConstOpDataRcPtr & op)
{
    CTFVersion minVersion{ CTF_PROCESS_LIST_VERSION };

    static_assert(OpData::NoOpType == 11, "Need to handle new type here");
    switch (op->getType())
    {
    case OpData::CDLType:
    {
        minVersion = CTF_PROCESS_LIST_VERSION_1_7;
        break;
    }
    case OpData::ExposureContrastType:
    {
        minVersion = CTF_PROCESS_LIST_VERSION_1_3;

        auto ec = OCIO_DYNAMIC_POINTER_CAST<const ExposureContrastOpData>(op);
        if (ec->getLogExposureStep() != ExposureContrastOpData::LOGEXPOSURESTEP_DEFAULT
            || ec->getLogMidGray() != ExposureContrastOpData::LOGMIDGRAY_DEFAULT)
        {
            minVersion = CTF_PROCESS_LIST_VERSION_2_0;
        }
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
            minVersion = (lut->getHueAdjust() != HUE_NONE) ?
                CTF_PROCESS_LIST_VERSION_1_4 :
                CTF_PROCESS_LIST_VERSION_1_3;
        }
        else
        {
            minVersion = (lut->getHueAdjust() != HUE_NONE || lut->isInputHalfDomain()) ?
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
    case OpData::MatrixType:
    case OpData::RangeType:
    {
        minVersion = CTF_PROCESS_LIST_VERSION_1_3;
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
void WriteDescriptions(XmlFormatter & fmt, const char * tag, const StringVec & descriptions)
{
    for (auto & it : descriptions)
    {
        fmt.writeContentTag(tag, it);
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

template<typename Iter, typename scaleType>
void WriteValues(XmlFormatter & formatter,
                 Iter valuesBegin,
                 Iter valuesEnd,
                 unsigned valuesPerLine,
                 BitDepth bitDepth,
                 unsigned iterStep,
                 scaleType scale)
{
    std::ostream& xml = formatter.getStream();

    for (Iter it(valuesBegin); it != valuesEnd; it += iterStep)
    {
        switch (bitDepth)
        {
        case BIT_DEPTH_UINT8:
        {
            xml.width(3);
            xml << (*it) * scale;
            break;
        }
        case BIT_DEPTH_UINT10:
        {
            xml.width(4);
            xml << (*it) * scale;
            break;
        }

        case BIT_DEPTH_UINT12:
        {
            xml.width(4);
            xml << (*it) * scale;
            break;
        }

        case BIT_DEPTH_UINT16:
        {
            xml.width(5);
            xml << (*it) * scale;
            break;
        }

        case BIT_DEPTH_F16:
        {
            xml.width(11);
            xml.precision(5);
            WriteValue((*it) * scale, xml);
            break;
        }

        case BIT_DEPTH_F32:
        {
            SetOStream(*it, xml);
            WriteValue((*it) * scale, xml);
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

    explicit OpWriter(XmlFormatter & formatter);
    virtual ~OpWriter();

    void write() const override;

    inline void setInputBitdepth(BitDepth in) { m_inBitDepth = in; }
    inline void setOutputBitdepth(BitDepth out) { m_outBitDepth = out; }

protected:
    virtual ConstOpDataRcPtr getOp() const = 0;
    virtual const char * getTagName() const = 0;
    virtual void getAttributes(XmlFormatter::Attributes & attributes) const;
    virtual void writeContent() const = 0;
    virtual void writeFormatMetadata() const;

    BitDepth m_inBitDepth = BIT_DEPTH_UNKNOWN;
    BitDepth m_outBitDepth = BIT_DEPTH_UNKNOWN;
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
        writeFormatMetadata();

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

void OpWriter::writeFormatMetadata() const
{
    auto op = getOp();
    StringVec desc;
    GetElementsValues(op->getFormatMetadata().getChildrenElements(),
                      TAG_DESCRIPTION, desc);
    WriteDescriptions(m_formatter, TAG_DESCRIPTION, desc);
}

const char * BitDepthToCLFString(BitDepth bitDepth)
{
    if (bitDepth == BIT_DEPTH_UINT8) return "8i";
    else if (bitDepth == BIT_DEPTH_UINT10) return "10i";
    else if (bitDepth == BIT_DEPTH_UINT12) return "12i";
    else if (bitDepth == BIT_DEPTH_UINT16) return "16i";
    else if (bitDepth == BIT_DEPTH_F16) return "16f";
    else if (bitDepth == BIT_DEPTH_F32) return "32f";
    
    throw Exception("Bitdepth has been validated before calling this.");
}

const char * GetTypeName(OpData::Type type)
{
    static_assert(OpData::NoOpType == 11, "Need to handle new type here");
    switch (type)
    {
    case OpData::CDLType:
        return "CDL";
    case OpData::ExponentType:
        return "Exponent";
    case OpData::ExposureContrastType:
        return "ExposureContrast";
    case OpData::FixedFunctionType:
        return "FixedFunction";
    case OpData::GammaType:
        return "Gamma";
    case OpData::LogType:
        return "Log";
    case OpData::Lut1DType:
        return "LUT1D";
    case OpData::Lut3DType:
        return "LUT3D";
    case OpData::MatrixType:
        return "Matrix";
    case OpData::RangeType:
        return "Range";
    case OpData::ReferenceType:
    case OpData::NoOpType:
        break;
    }
    throw Exception("Unexpected op type.");
}

BitDepth GetValidatedFileBitDepth(BitDepth bd, OpData::Type type)
{
    // If we get BIT_DEPTH_UNKNOWN here, it means the client has not
    // requested any specific bit-depth to write to the file,
    // so we will use a default of F32.
    if (bd == BIT_DEPTH_UNKNOWN)
    {
        return BIT_DEPTH_F32;
    }
    if ((bd == BIT_DEPTH_UINT8) ||
        (bd == BIT_DEPTH_UINT10) ||
        (bd == BIT_DEPTH_UINT12) ||
        (bd == BIT_DEPTH_UINT16) ||
        (bd == BIT_DEPTH_F16) ||
        (bd == BIT_DEPTH_F32))
    {
        return bd;
    }

    const std::string typeName(GetTypeName(type));
    std::ostringstream oss;
    oss << "Op " << typeName << ". Bit-depth: " << bd
        << " is not supported for writing to CLF/CTF.";
    throw Exception(oss.str().c_str());
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

    const char* inBitDepthName = BitDepthToCLFString(m_inBitDepth);
    attributes.push_back(XmlFormatter::Attribute(ATTR_BITDEPTH_IN,
                                                 inBitDepthName));

    const char* outBitDepthName = BitDepthToCLFString(m_outBitDepth);
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
    CDLWriter() = delete;
    CDLWriter(const CDLWriter&) = delete;
    CDLWriter& operator=(const CDLWriter&) = delete;
    CDLWriter(XmlFormatter & formatter,
              ConstCDLOpDataRcPtr cdl);
    virtual ~CDLWriter();

protected:
    ConstOpDataRcPtr getOp() const override;
    const char * getTagName() const override;
    void getAttributes(XmlFormatter::Attributes& attributes) const override;
    void writeContent() const override;
    void writeFormatMetadata() const override;

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
    auto op = getOp();
    // SOPNode.
    m_formatter.writeStartTag(TAG_SOPNODE, attributes);
    {
        XmlScopeIndent scopeIndent(m_formatter);

        StringVec desc;
        GetElementsValues(op->getFormatMetadata().getChildrenElements(),
                          METADATA_SOP_DESCRIPTION, desc);
        WriteDescriptions(m_formatter, TAG_DESCRIPTION, desc);

        m_formatter.writeContentTag(TAG_SLOPE, m_cdl->getSlopeString());
        m_formatter.writeContentTag(TAG_OFFSET, m_cdl->getOffsetString());
        m_formatter.writeContentTag(TAG_POWER, m_cdl->getPowerString());
    }
    m_formatter.writeEndTag(TAG_SOPNODE);

    // SatNode.
    m_formatter.writeStartTag(TAG_SATNODE, attributes);
    {
        XmlScopeIndent scopeIndent(m_formatter);

        StringVec desc;
        GetElementsValues(op->getFormatMetadata().getChildrenElements(),
                          METADATA_SAT_DESCRIPTION, desc);
        WriteDescriptions(m_formatter, TAG_DESCRIPTION, desc);

        m_formatter.writeContentTag(TAG_SATURATION, m_cdl->getSaturationString());
    }
    m_formatter.writeEndTag(TAG_SATNODE);
}

void CDLWriter::writeFormatMetadata() const
{
    auto op = getOp();
    StringVec desc;
    GetElementsValues(op->getFormatMetadata().getChildrenElements(),
                      METADATA_DESCRIPTION, desc);
    WriteDescriptions(m_formatter, TAG_DESCRIPTION, desc);
    desc.clear();
    GetElementsValues(op->getFormatMetadata().getChildrenElements(),
                      METADATA_INPUT_DESCRIPTION, desc);
    WriteDescriptions(m_formatter, METADATA_INPUT_DESCRIPTION, desc);
    desc.clear();
    GetElementsValues(op->getFormatMetadata().getChildrenElements(),
                      METADATA_VIEWING_DESCRIPTION, desc);
    WriteDescriptions(m_formatter, METADATA_VIEWING_DESCRIPTION, desc);
}

///////////////////////////////////////////////////////////////////////////////

class ExposureContrastWriter : public OpWriter
{
public:
    ExposureContrastWriter() = delete;
    ExposureContrastWriter(const ExposureContrastWriter&) = delete;
    ExposureContrastWriter& operator=(const ExposureContrastWriter&) = delete;
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
    FixedFunctionWriter() = delete;
    FixedFunctionWriter(const FixedFunctionWriter&) = delete;
    FixedFunctionWriter& operator=(const FixedFunctionWriter&) = delete;
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
    GammaWriter() = delete;
    GammaWriter(const GammaWriter&) = delete;
    GammaWriter& operator=(const GammaWriter&) = delete;
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

        if (GetOpMinimumVersion(m_gamma) >= CTF_PROCESS_LIST_VERSION_1_5)
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
    LogWriter() = delete;
    LogWriter(const LogWriter&) = delete;
    LogWriter& operator=(const LogWriter&) = delete;
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
    Lut1DWriter() = delete;
    Lut1DWriter(const Lut1DWriter&) = delete;
    Lut1DWriter& operator=(const Lut1DWriter&) = delete;
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
    // If the client requests INTERP_LINEAR, we want to write it to the
    // attribute (even though linear is what CLF specifies as its default,
    // some clients may want to lock down that behavior). INTERP_DEFAULT
    // means "do not write the attribute".
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

    LUT1DHueAdjust hueAdjust = m_lut->getHueAdjust();
    if (hueAdjust == HUE_DW3)
    {
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

    // To avoid needing to duplicate the const objects,
    // we scale the values on-the-fly while writing.
    const auto fbd = m_outBitDepth;
    const auto bd = m_lut->getOutputBitDepth();
    const float scale = (float)(GetBitDepthMaxValue(fbd) / GetBitDepthMaxValue(bd));

    if (m_lut->isOutputRawHalfs())
    {
        std::vector<unsigned> values;

        const size_t maxValues = array.getNumValues();
        values.resize(maxValues);

        for (size_t i = 0; i<maxValues; ++i)
        {
            half h = (array.getValues()[i] * scale);
            values[i] = h.bits();
        }

        WriteValues(m_formatter,
                    values.begin(),
                    values.end(),
                    array.getNumColorComponents(),
                    BIT_DEPTH_UINT16,
                    array.getNumColorComponents() == 1 ? 3 : 1,
                    1.0f);
    }
    else
    {
        const Array::Values & values = array.getValues();
        WriteValues(m_formatter,
                    values.begin(),
                    values.end(),
                    array.getNumColorComponents(),
                    fbd,
                    array.getNumColorComponents() == 1 ? 3 : 1,
                    scale);
    }

    m_formatter.writeEndTag(TAG_ARRAY);
}

///////////////////////////////////////////////////////////////////////////////

class Lut3DWriter : public OpWriter
{
public:
    Lut3DWriter() = delete;
    Lut3DWriter(const Lut3DWriter&) = delete;
    Lut3DWriter& operator=(const Lut3DWriter&) = delete;
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
    // Please see comment in Lut1DWriter.
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

    // To avoid needing to duplicate the const objects,
    // we scale the values on-the-fly while writing.
    const auto fbd = m_outBitDepth;
    const auto bd = m_lut->getOutputBitDepth();
    const float scale = (float)(GetBitDepthMaxValue(fbd) / GetBitDepthMaxValue(bd));
    WriteValues(m_formatter,
                array.getValues().begin(),
                array.getValues().end(),
                3,
                fbd,
                1,
                scale);

    m_formatter.writeEndTag(TAG_ARRAY);
}

///////////////////////////////////////////////////////////////////////////////

class MatrixWriter : public OpWriter
{
public:
    MatrixWriter() = delete;
    MatrixWriter(const MatrixWriter&) = delete;
    MatrixWriter& operator=(const MatrixWriter&) = delete;
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

            WriteValues(m_formatter, v, v + 20, 5, BIT_DEPTH_F32, 1, 1.0);
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

            WriteValues(m_formatter, v, v + 16, 4, BIT_DEPTH_F32, 1, 1.0);
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

        WriteValues(m_formatter, v, v + 12, 4, BIT_DEPTH_F32, 1, 1.0);
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

        WriteValues(m_formatter, v, v + 9, 3, BIT_DEPTH_F32, 1, 1.0);
    }

    m_formatter.writeEndTag(TAG_ARRAY);
}

///////////////////////////////////////////////////////////////////////////////

class RangeWriter : public OpWriter
{
public:
    RangeWriter() = delete;
    RangeWriter(const RangeWriter&) = delete;
    RangeWriter& operator=(const RangeWriter&) = delete;
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

        WriteDescriptions(m_formatter, TAG_DESCRIPTION, m_transform->getDescriptions());

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

BitDepth GetInputFileBD(ConstOpDataRcPtr op)
{
    const auto type = op->getType();
    if (type == OpData::MatrixType)
    {
        auto mat = OCIO_DYNAMIC_POINTER_CAST<const MatrixOpData>(op);
        const auto bd = mat->getFileInputBitDepth();
        return GetValidatedFileBitDepth(bd, type);
    }
    else if (type == OpData::RangeType)
    {
        auto range = OCIO_DYNAMIC_POINTER_CAST<const RangeOpData>(op);
        const auto bd = range->getFileInputBitDepth();
        return GetValidatedFileBitDepth(bd, type);
    }
    return BIT_DEPTH_F32;
}

}

void TransformWriter::writeOps() const
{
    BitDepth inBD = BIT_DEPTH_F32;
    BitDepth outBD = BIT_DEPTH_F32;

    auto & ops = m_transform->getOps();
    size_t numOps = ops.size();
    size_t numSavedOps = 0;
    if (numOps)
    {
        inBD = GetInputFileBD(ops[0]);
        for (size_t i = 0; i < numOps; ++i)
        {
            static_assert(OpData::NoOpType == 11, "Add new types");

            auto & op = ops[i];

            if (i + 1 < numOps)
            {
                auto & nextOp = ops[i + 1];
                // Return file input bit-depth for Matrix & Range, F32 for others.
                outBD = GetInputFileBD(nextOp);
            }

            const auto type = op->getType();

            if (type != OpData::NoOpType)
            {
                op->validate();
                ++numSavedOps;
            }

            switch (type)
            {
            case OpData::CDLType:
            {
                auto cdl = OCIO_DYNAMIC_POINTER_CAST<const CDLOpData>(op);
                CDLWriter opWriter(m_formatter, cdl);
                opWriter.setInputBitdepth(inBD);
                opWriter.setOutputBitdepth(outBD);
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
                opWriter.setInputBitdepth(inBD);
                opWriter.setOutputBitdepth(outBD);
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
                opWriter.setInputBitdepth(inBD);
                opWriter.setOutputBitdepth(outBD);
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
                opWriter.setInputBitdepth(inBD);
                opWriter.setOutputBitdepth(outBD);
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
                opWriter.setInputBitdepth(inBD);
                opWriter.setOutputBitdepth(outBD);
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
                opWriter.setInputBitdepth(inBD);
                opWriter.setOutputBitdepth(outBD);
                opWriter.write();
                break;
            }
            case OpData::Lut1DType:
            {
                auto lut = OCIO_DYNAMIC_POINTER_CAST<const Lut1DOpData>(op);
                if (m_isCLF)
                {
                    if (lut->getDirection() != TRANSFORM_DIR_FORWARD)
                    {
                        ThrowWriteOp("InverseLUT1D");
                    }
                }
                // Avoid copying LUT, write will take bit-depth into account.
                Lut1DWriter opWriter(m_formatter, lut);

                outBD = GetValidatedFileBitDepth(lut->getFileOutputBitDepth(), type);
                opWriter.setInputBitdepth(inBD);
                opWriter.setOutputBitdepth(outBD);

                opWriter.write();
                break;
            }
            case OpData::Lut3DType:
            {
                auto lut = OCIO_DYNAMIC_POINTER_CAST<const Lut3DOpData>(op);
                if (m_isCLF)
                {
                    if (lut->getDirection() != TRANSFORM_DIR_FORWARD)
                    {
                        ThrowWriteOp("InverseLUT3D");
                    }
                }
                // Avoid copying LUT, write will take bit-depth into account.
                Lut3DWriter opWriter(m_formatter, lut);

                outBD = GetValidatedFileBitDepth(lut->getFileOutputBitDepth(), type);
                opWriter.setInputBitdepth(inBD);
                opWriter.setOutputBitdepth(outBD);

                opWriter.write();
                break;
            }
            case OpData::MatrixType:
            {
                auto matSrc = OCIO_DYNAMIC_POINTER_CAST<const MatrixOpData>(op);

                if (m_isCLF)
                {
                    if (matSrc->hasAlpha())
                    {
                        std::ostringstream oss;
                        oss << "Transform uses a Matrix op that has an alpha "
                               "component, so it cannot be written as CLF.  "
                               "Use CTF format for this transform.";
                        throw Exception(oss.str().c_str());
                    }
                }

                auto mat = matSrc->clone();

                outBD = GetValidatedFileBitDepth(mat->getFileOutputBitDepth(), type);
                // inBD has already been set at previous iteration.
                // inBD can be:
                // - This op input file bit-depth if previous op
                //   does not define an output file bit-depth.
                // - Previous op output file bit-depth if previous op
                //   is a LUT, a Matrix or a Range.

                mat->setInputBitDepth(inBD);
                mat->setOutputBitDepth(outBD);
                MatrixWriter opWriter(m_formatter, mat);
                opWriter.setInputBitdepth(inBD);
                opWriter.setOutputBitdepth(outBD);

                opWriter.write();
                break;
            }
            case OpData::RangeType:
            {
                auto rangeSrc = OCIO_DYNAMIC_POINTER_CAST<const RangeOpData>(op);
                auto range = rangeSrc->clone();

                outBD = GetValidatedFileBitDepth(range->getFileOutputBitDepth(), type);
                // inBD has already been set at previous iteration.

                range->setInputBitDepth(inBD);
                range->setOutputBitDepth(outBD);
                RangeWriter opWriter(m_formatter, range);
                opWriter.setInputBitdepth(inBD);
                opWriter.setOutputBitdepth(outBD);

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

            // For next op.
            inBD = outBD;
            outBD = BIT_DEPTH_F32;
        }
    }
    if (numSavedOps == 0)
    {
        // When there are no ops, save an identity matrix.
        auto mat = std::make_shared<MatrixOpData>();

        MatrixWriter opWriter(m_formatter, mat);
        opWriter.setInputBitdepth(BIT_DEPTH_F32);
        opWriter.setOutputBitdepth(BIT_DEPTH_F32);

        opWriter.write();
    }

}

}
OCIO_NAMESPACE_EXIT

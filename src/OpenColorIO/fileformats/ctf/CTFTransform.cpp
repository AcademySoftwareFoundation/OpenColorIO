// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include "BitDepthUtils.h"
#include "fileformats/ctf/CTFReaderUtils.h"
#include "fileformats/ctf/CTFTransform.h"
#include "fileformats/xmlutils/XMLReaderUtils.h"
#include "HashUtils.h"
#include "ops/cdl/CDLOpData.h"
#include "ops/exponent/ExponentOp.h"
#include "ops/exposurecontrast/ExposureContrastOpData.h"
#include "ops/fixedfunction/FixedFunctionOpData.h"
#include "ops/gamma/GammaOpData.h"
#include "ops/gradingprimary/GradingPrimaryOpData.h"
#include "ops/gradingrgbcurve/GradingRGBCurve.h"
#include "ops/gradingrgbcurve/GradingRGBCurveOpData.h"
#include "ops/gradingtone/GradingToneOpData.h"
#include "ops/log/LogOpData.h"
#include "ops/log/LogUtils.h"
#include "ops/lut1d/Lut1DOpData.h"
#include "ops/lut3d/Lut3DOpData.h"
#include "ops/matrix/MatrixOpData.h"
#include "ops/range/RangeOpData.h"
#include "ops/reference/ReferenceOpData.h"
#include "ParseUtils.h"
#include "Platform.h"
#include "transforms/CDLTransform.h"

namespace OCIO_NAMESPACE
{

// Note: 17 would allow exactly restoring most doubles but anything higher than 15,
// introduces some serialization issues such as: 81.9 -> 81.90000000000001.
// This results in less pretty output and also causes problems for some unit tests.  
static constexpr unsigned DOUBLE_PRECISION = 15;


void CTFVersion::ReadVersion(const std::string & versionString, CTFVersion & versionOut)
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
    : m_infoMetadata(METADATA_INFO, "")
    , m_version(CTF_PROCESS_LIST_VERSION)
    , m_versionCLF(0, 0)
{
}

CTFReaderTransform::CTFReaderTransform(const OpRcPtrVec & ops,
                                       const FormatMetadataImpl & metadata)
    : m_infoMetadata(METADATA_INFO, "")
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

const CTFVersion & CTFReaderTransform::getCLFVersion() const
{
    return m_versionCLF;
}

bool CTFReaderTransform::isCLF() const
{
    return !(m_versionCLF == CTFVersion(0, 0));
}

void GetElementsValues(const FormatMetadataImpl::Elements & elements, 
                       const std::string & name,
                       StringUtils::StringVec & values)
{
    for (auto & element : elements)
    {
        if (0 == Platform::Strcasecmp(name.c_str(), element.getElementName()))
        {
            values.push_back(element.getElementValue());
        }
    }
}

namespace
{

CTFVersion GetOpMinimumVersion(const ConstOpDataRcPtr & op)
{
    CTFVersion minVersion{ CTF_PROCESS_LIST_VERSION };

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
    case OpData::GradingPrimaryType:
    case OpData::GradingRGBCurveType:
    case OpData::GradingToneType:
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
        const auto style = gamma->getStyle();
        switch (style)
        {
        case GammaOpData::BASIC_FWD:
        case GammaOpData::BASIC_REV:
        case GammaOpData::MONCURVE_FWD:
        case GammaOpData::MONCURVE_REV:
            minVersion = gamma->isAlphaComponentIdentity() ?
                CTF_PROCESS_LIST_VERSION_1_3 :
                CTF_PROCESS_LIST_VERSION_1_5;
            break;
        case GammaOpData::BASIC_MIRROR_FWD:
        case GammaOpData::BASIC_MIRROR_REV:
        case GammaOpData::BASIC_PASS_THRU_FWD:
        case GammaOpData::BASIC_PASS_THRU_REV:
        case GammaOpData::MONCURVE_MIRROR_FWD:
        case GammaOpData::MONCURVE_MIRROR_REV:
            minVersion = CTF_PROCESS_LIST_VERSION_2_0;
            break;
        }
        break;
    }
    case OpData::Lut1DType:
    {
        auto lut = OCIO_DYNAMIC_POINTER_CAST<const Lut1DOpData>(op);
        switch (lut->getDirection())
        {
        case TRANSFORM_DIR_FORWARD:
            minVersion = (lut->getHueAdjust() != HUE_NONE) ? CTF_PROCESS_LIST_VERSION_1_4 :
                                                             CTF_PROCESS_LIST_VERSION_1_3;
            break;
        case TRANSFORM_DIR_INVERSE:
            minVersion = (lut->getHueAdjust() != HUE_NONE || lut->isInputHalfDomain()) ?
                         CTF_PROCESS_LIST_VERSION_1_6 :
                         CTF_PROCESS_LIST_VERSION_1_3;
            break;
        }
        break;
    }
    case OpData::Lut3DType:
    {
        auto lut = OCIO_DYNAMIC_POINTER_CAST<const Lut3DOpData>(op);
        switch (lut->getDirection())
        {
        case TRANSFORM_DIR_FORWARD:
            minVersion = CTF_PROCESS_LIST_VERSION_1_3;
            break;
        case TRANSFORM_DIR_INVERSE:
            minVersion = CTF_PROCESS_LIST_VERSION_1_6;
            break;
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
        if (0 == Platform::Strcasecmp(name.c_str(), it.getElementName()))
        {
            return it.getElementValue();
        }
    }
    return "";
}

const char * GetLastElementValue(const FormatMetadataImpl::Elements & elements, const std::string & name)
{
    for (auto it = elements.rbegin(); it != elements.rend(); ++it)
    {
        if (0 == Platform::Strcasecmp(name.c_str(), it->getElementName()))
        {
            return it->getElementValue();
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
    m_name = metadata.getAttributeValueString(METADATA_NAME);
    m_id = metadata.getAttributeValueString(METADATA_ID);
    m_inverseOfId = metadata.getAttributeValueString(ATTR_INVERSE_OF);

    // Preserve first InputDescriptor, last OutputDescriptor, and all Descriptions.
    m_inDescriptor = GetFirstElementValue(metadata.getChildrenElements(), METADATA_INPUT_DESCRIPTOR);
    m_outDescriptor = GetLastElementValue(metadata.getChildrenElements(), METADATA_OUTPUT_DESCRIPTOR);
    GetElementsValues(metadata.getChildrenElements(), METADATA_DESCRIPTION, m_descriptions);

    // Combine all Info elements.
    for (auto elt : metadata.getChildrenElements())
    {
        if (0 == Platform::Strcasecmp(elt.getElementName(), METADATA_INFO))
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

    AddNonEmptyElement(metadata, METADATA_INPUT_DESCRIPTOR, getInputDescriptor());
    AddNonEmptyElement(metadata, METADATA_OUTPUT_DESCRIPTOR, getOutputDescriptor());
    for (auto & desc : m_descriptions)
    {
        metadata.addChildElement(METADATA_DESCRIPTION, desc.c_str());
    }
    const std::string infoValue(m_infoMetadata.getElementValue());
    if (m_infoMetadata.getNumAttributes() || m_infoMetadata.getNumChildrenElements() ||
        !infoValue.empty())
    {
        metadata.getChildrenElements().push_back(m_infoMetadata);
    }
}


///////////////////////////////////////////////////////////////////////////////

namespace
{
void WriteDescriptions(XmlFormatter & fmt, const char * tag, const StringUtils::StringVec & descriptions)
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
    xml.precision(DOUBLE_PRECISION);
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
    // Method used to write an array of values of the same type.

    std::ostream & xml = formatter.getStream();
    std::ostringstream oss;

    // The numbers in a CLF/CTF file may always contain fractional values, regardless of the
    // bit-depth attributes.  E.g., even if the bit-depth is 8i, the array could contain values
    // such as [-0.1, 8, 100.234, 305].  However, we do use the bit-depth to initialize the most
    // likely formatting to use when printing arrays with large numbers of values such as LUTs.
    // And if the array really does only have integers, it is nicer to print those without
    // decimal points.

    switch (bitDepth)
    {
    case BIT_DEPTH_UINT8:
    {
        oss.width(3);
        break;
    }
    case BIT_DEPTH_UINT10:
    {
        oss.width(4);
        break;
    }

    case BIT_DEPTH_UINT12:
    {
        oss.width(4);
        break;
    }

    case BIT_DEPTH_UINT16:
    {
        oss.width(5);
        break;
    }

    case BIT_DEPTH_F16:
    {
        oss.width(11);
        oss.precision(5);
        break;
    }

    case BIT_DEPTH_F32:
    {
        SetOStream(*valuesBegin, oss);
        break;
    }

    case BIT_DEPTH_UINT14:
    case BIT_DEPTH_UINT32:
    {
        throw Exception("Unsupported bitdepth.");
        break;
    }

    case BIT_DEPTH_UNKNOWN:
    {
        throw Exception("Unknown bitdepth.");
        break;
    }
    }

    const bool floatValues = (bitDepth == BIT_DEPTH_F16) || (bitDepth == BIT_DEPTH_F32);

    for (Iter it(valuesBegin); it != valuesEnd; it += iterStep)
    {
        oss.str("");

        if (floatValues)
        {
            WriteValue((*it) * scale, oss);
        }
        else
        {
            oss << (*it) * scale;
        }

        const std::string value = oss.str();
        if (value.length() > (size_t)oss.width())
        {
            // The imposed precision requires more characters so the code
            // recomputes the width to better align the values for the next lines. 
            oss.width(value.length());
        }

        xml << value;

        if (std::distance(valuesBegin, it) % valuesPerLine == valuesPerLine - 1)
        {
            // std::endln writes a newline and flushes the output buffer where '\n' only
            // writes a newline. Flushing the buffer for all floats of large LUTs
            // introduces a huge performance hit so only use '\n'.
            xml << "\n";
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
    StringUtils::StringVec desc;
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

    std::ostringstream oss;
    oss.precision(DOUBLE_PRECISION);

    CDLOpData::ChannelParams params;

    // SOPNode.
    m_formatter.writeStartTag(TAG_SOPNODE, attributes);
    {
        XmlScopeIndent scopeIndent(m_formatter);

        StringUtils::StringVec desc;
        GetElementsValues(op->getFormatMetadata().getChildrenElements(),
                          METADATA_SOP_DESCRIPTION, desc);
        WriteDescriptions(m_formatter, TAG_DESCRIPTION, desc);

        oss.str("");
        params = m_cdl->getSlopeParams();
        oss << params[0] << ", " << params[1] << ", " << params[2];
        m_formatter.writeContentTag(TAG_SLOPE, oss.str());

        oss.str("");
        params = m_cdl->getOffsetParams();
        oss << params[0] << ", " << params[1] << ", " << params[2];
        m_formatter.writeContentTag(TAG_OFFSET, oss.str());

        oss.str("");
        params = m_cdl->getPowerParams();
        oss << params[0] << ", " << params[1] << ", " << params[2];
        m_formatter.writeContentTag(TAG_POWER, oss.str());
    }
    m_formatter.writeEndTag(TAG_SOPNODE);

    // SatNode.
    m_formatter.writeStartTag(TAG_SATNODE, attributes);
    {
        XmlScopeIndent scopeIndent(m_formatter);

        StringUtils::StringVec desc;
        GetElementsValues(op->getFormatMetadata().getChildrenElements(),
                          METADATA_SAT_DESCRIPTION, desc);
        WriteDescriptions(m_formatter, TAG_DESCRIPTION, desc);

        oss.str("");
        oss << m_cdl->getSaturation();
        m_formatter.writeContentTag(TAG_SATURATION, oss.str());
    }
    m_formatter.writeEndTag(TAG_SATNODE);
}

void CDLWriter::writeFormatMetadata() const
{
    auto op = getOp();
    StringUtils::StringVec desc;
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
    std::ostringstream oss;
    oss.precision(DOUBLE_PRECISION);

    XmlFormatter::Attributes attributes;
    {
        oss.str("");
        WriteValue(m_ec->getExposure(), oss);
        attributes.push_back(XmlFormatter::Attribute(ATTR_EXPOSURE, oss.str()));
    }
    {
        oss.str("");
        WriteValue(m_ec->getContrast(), oss);
        attributes.push_back(XmlFormatter::Attribute(ATTR_CONTRAST, oss.str()));
    }
    {
        oss.str("");
        WriteValue(m_ec->getGamma(), oss);
        attributes.push_back(XmlFormatter::Attribute(ATTR_GAMMA, oss.str()));
    }
    {
        oss.str("");
        WriteValue(m_ec->getPivot(), oss);
        attributes.push_back(XmlFormatter::Attribute(ATTR_PIVOT, oss.str()));
    }

    if (m_ec->getLogExposureStep() != ExposureContrastOpData::LOGEXPOSURESTEP_DEFAULT)
    {
        oss.str("");
        WriteValue(m_ec->getLogExposureStep(), oss);
        attributes.push_back(XmlFormatter::Attribute(ATTR_LOGEXPOSURESTEP, oss.str()));
    }

    if (m_ec->getLogMidGray() != ExposureContrastOpData::LOGMIDGRAY_DEFAULT)
    {
        oss.str("");
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
    FixedFunctionOpData::Params params = m_ff->getParams();

    const size_t numParams = params.size();
    if (numParams != 0)
    {
        size_t i = 0;
        std::stringstream ffParams;
        ffParams.precision(DOUBLE_PRECISION);
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
    GammaWriter(XmlFormatter & formatter, const CTFVersion & version,
                ConstGammaOpDataRcPtr gamma);
    virtual ~GammaWriter();

protected:
    ConstOpDataRcPtr getOp() const override;
    const char * getTagName() const override;
    void getAttributes(XmlFormatter::Attributes& attributes) const override;
    void writeContent() const override;

private:
    const CTFVersion m_version;
    ConstGammaOpDataRcPtr m_gamma;
};


GammaWriter::GammaWriter(XmlFormatter & formatter, const CTFVersion & version,
                         ConstGammaOpDataRcPtr gamma)
    : OpWriter(formatter)
    , m_version(version)
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
    if (m_version < CTF_PROCESS_LIST_VERSION_2_0)
    {
        return TAG_GAMMA;
    }
    return TAG_EXPONENT;
}

void GammaWriter::getAttributes(XmlFormatter::Attributes& attributes) const
{
    OpWriter::getAttributes(attributes);
    const std::string & style = GammaOpData::ConvertStyleToString(m_gamma->getStyle());
    attributes.push_back(XmlFormatter::Attribute(ATTR_STYLE, style));
}

namespace
{
// Add the attributes for the GammaParams element.
void AddGammaParams(XmlFormatter::Attributes & attributes,
                    const GammaOpData::Params & params,
                    const GammaOpData::Style style,
                    bool useGamma)
{
    std::stringstream oss;
    oss.precision(DOUBLE_PRECISION);

    oss << params[0];
    attributes.push_back(XmlFormatter::Attribute(useGamma ? ATTR_GAMMA : ATTR_EXPONENT,
                                                 oss.str()));

    switch (style)
    {
    case GammaOpData::MONCURVE_FWD:
    case GammaOpData::MONCURVE_REV:
    case GammaOpData::MONCURVE_MIRROR_FWD:
    case GammaOpData::MONCURVE_MIRROR_REV:
    {
        oss.str("");
        oss << params[1];
        attributes.push_back(XmlFormatter::Attribute(ATTR_OFFSET, oss.str()));
        break;
    }
    case GammaOpData::BASIC_FWD:
    case GammaOpData::BASIC_REV:
    case GammaOpData::BASIC_MIRROR_FWD:
    case GammaOpData::BASIC_MIRROR_REV:
    case GammaOpData::BASIC_PASS_THRU_FWD:
    case GammaOpData::BASIC_PASS_THRU_REV:
        break;
    }
}
}

void GammaWriter::writeContent() const
{
    const bool useGamma{ m_version < CTF_PROCESS_LIST_VERSION_2_0 };
    const std::string paramsTag{ useGamma ? TAG_GAMMA_PARAMS : TAG_EXPONENT_PARAMS };
    if (m_gamma->isNonChannelDependent())
    {
        // RGB channels equal and A is identity, just write one element.
        XmlFormatter::Attributes attributes;

        AddGammaParams(attributes,
                       m_gamma->getRedParams(),
                       m_gamma->getStyle(), useGamma);

        m_formatter.writeEmptyTag(paramsTag, attributes);
    }
    else
    {
        // Red.
        XmlFormatter::Attributes attributesR;
        attributesR.push_back(XmlFormatter::Attribute(ATTR_CHAN,
                                                      "R"));
        AddGammaParams(attributesR,
                       m_gamma->getRedParams(),
                       m_gamma->getStyle(), useGamma);

        m_formatter.writeEmptyTag(paramsTag, attributesR);

        // Green.
        XmlFormatter::Attributes attributesG;
        attributesG.push_back(XmlFormatter::Attribute(ATTR_CHAN,
                                                      "G"));
        AddGammaParams(attributesG,
                       m_gamma->getGreenParams(),
                       m_gamma->getStyle(), useGamma);

        m_formatter.writeEmptyTag(paramsTag, attributesG);

        // Blue.
        XmlFormatter::Attributes attributesB;
        attributesB.push_back(XmlFormatter::Attribute(ATTR_CHAN,
                                                      "B"));
        AddGammaParams(attributesB,
                       m_gamma->getBlueParams(),
                       m_gamma->getStyle(), useGamma);

        m_formatter.writeEmptyTag(paramsTag, attributesB);

        if (!m_gamma->isAlphaComponentIdentity())
        {
            // Alpha
            XmlFormatter::Attributes attributesA;
            attributesA.push_back(XmlFormatter::Attribute(ATTR_CHAN,
                                                          "A"));
            AddGammaParams(attributesA,
                           m_gamma->getAlphaParams(),
                           m_gamma->getStyle(), useGamma);

            m_formatter.writeEmptyTag(paramsTag, attributesA);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

class GradingPrimaryWriter : public OpWriter
{
public:
    GradingPrimaryWriter() = delete;
    GradingPrimaryWriter(const GradingPrimaryWriter&) = delete;
    GradingPrimaryWriter& operator=(const GradingPrimaryWriter&) = delete;
    GradingPrimaryWriter(XmlFormatter & formatter, ConstGradingPrimaryOpDataRcPtr primary);
    virtual ~GradingPrimaryWriter();

protected:
    ConstOpDataRcPtr getOp() const override;
    const char * getTagName() const override;
    void getAttributes(XmlFormatter::Attributes & attributes) const override;
    void writeContent() const override;

private:
    void writeRGBM(const char * tag,
                   const GradingRGBM & defaultVal,
                   const GradingRGBM & val) const;
    void writeScalarElement(const char * tag, double defaultVal, double val) const;
    void addAttribute(XmlFormatter::Attributes & attributes, const char * attr,
                      double val) const;
    void addAttribute(XmlFormatter::Attributes & attributes, const char * attr,
                      double defaultVal, double val) const;

    ConstGradingPrimaryOpDataRcPtr m_primary;
};

GradingPrimaryWriter::GradingPrimaryWriter(XmlFormatter & formatter,
                                           ConstGradingPrimaryOpDataRcPtr primary)
    : OpWriter(formatter)
    , m_primary(primary)
{
}

GradingPrimaryWriter::~GradingPrimaryWriter()
{
}

ConstOpDataRcPtr GradingPrimaryWriter::getOp() const
{
    return m_primary;
}

const char * GradingPrimaryWriter::getTagName() const
{
    return TAG_PRIMARY;
}

void GradingPrimaryWriter::getAttributes(XmlFormatter::Attributes& attributes) const
{
    OpWriter::getAttributes(attributes);

    const auto style = m_primary->getStyle();
    const auto dir = m_primary->getDirection();

    const auto styleStr = ConvertGradingStyleAndDirToString(style, dir);
    attributes.push_back(XmlFormatter::Attribute(ATTR_STYLE, styleStr));
}

void GradingPrimaryWriter::writeRGBM(const char * tag,
                                     const GradingRGBM & defaultVal,
                                     const GradingRGBM & val) const
{
    if (val != defaultVal)
    {
        XmlFormatter::Attributes attributes;

        std::stringstream rgb;
        rgb.precision(DOUBLE_PRECISION);
        rgb << val.m_red << " " << val.m_green << " " << val.m_blue;
        attributes.push_back(XmlFormatter::Attribute(ATTR_RGB, rgb.str()));
        std::stringstream master;
        master.precision(DOUBLE_PRECISION);
        master << val.m_master;
        attributes.push_back(XmlFormatter::Attribute(ATTR_MASTER, master.str()));

        m_formatter.writeEmptyTag(tag, attributes);
    }
}

void GradingPrimaryWriter::writeScalarElement(const char * tag,
                                              double defaultVal,
                                              double val) const
{
    if (val != defaultVal)
    {
        XmlFormatter::Attributes attributes;
        std::stringstream stream;
        stream.precision(DOUBLE_PRECISION);
        stream << val;
        attributes.push_back(XmlFormatter::Attribute(ATTR_MASTER, stream.str()));
        m_formatter.writeEmptyTag(tag, attributes);
    }
}

void GradingPrimaryWriter::addAttribute(XmlFormatter::Attributes & attributes, const char * attr,
                                        double defaultVal, double val) const
{
    if (val != defaultVal)
    {
        addAttribute(attributes, attr, val);
    }
}

void GradingPrimaryWriter::addAttribute(XmlFormatter::Attributes & attributes, const char * attr,
                                        double val) const
{
    std::stringstream master;
    master.precision(DOUBLE_PRECISION);
    master << val;
    attributes.push_back(XmlFormatter::Attribute(attr, master.str()));
}

void GradingPrimaryWriter::writeContent() const
{
    const auto style = m_primary->getStyle();
    const auto & vals = m_primary->getValue();
    switch (style)
    {
    case GRADING_LOG:
    {
        GradingPrimary defaultVals(style);
        writeRGBM(TAG_PRIMARY_BRIGHTNESS, defaultVals.m_brightness, vals.m_brightness);
        writeRGBM(TAG_PRIMARY_CONTRAST, defaultVals.m_contrast, vals.m_contrast);
        writeRGBM(TAG_PRIMARY_GAMMA, defaultVals.m_gamma, vals.m_gamma);

        writeScalarElement(TAG_PRIMARY_SATURATION, defaultVals.m_saturation, vals.m_saturation);

        // Pivot.
        {
            XmlFormatter::Attributes attributes;
            if (defaultVals.m_contrast != vals.m_contrast)
            {
                // Always write pivot contrast when constrast is not default.
                addAttribute(attributes, ATTR_PRIMARY_CONTRAST, vals.m_pivot);
            }
            else
            {
                addAttribute(attributes, ATTR_PRIMARY_CONTRAST, defaultVals.m_pivot, vals.m_pivot);
            }
            addAttribute(attributes, ATTR_PRIMARY_BLACK, defaultVals.m_pivotBlack, vals.m_pivotBlack);
            addAttribute(attributes, ATTR_PRIMARY_WHITE, defaultVals.m_pivotWhite, vals.m_pivotWhite);
            if (!attributes.empty())
            {
                m_formatter.writeEmptyTag(TAG_PRIMARY_PIVOT, attributes);
            }
        }
        break;
    }
    case GRADING_LIN:
    {
        GradingPrimary defaultVals(style);
        writeRGBM(TAG_PRIMARY_OFFSET, defaultVals.m_offset, vals.m_offset);
        writeRGBM(TAG_PRIMARY_EXPOSURE, defaultVals.m_exposure, vals.m_exposure);
        writeRGBM(TAG_PRIMARY_CONTRAST, defaultVals.m_contrast, vals.m_contrast);

        writeScalarElement(TAG_PRIMARY_SATURATION, defaultVals.m_saturation, vals.m_saturation);

        // Pivot.
        {
            XmlFormatter::Attributes attributes;
            if (defaultVals.m_contrast != vals.m_contrast)
            {
                // Always write pivot contrast when constrast is not default.
                addAttribute(attributes, ATTR_PRIMARY_CONTRAST, vals.m_pivot);
            }
            else
            {
                addAttribute(attributes, ATTR_PRIMARY_CONTRAST, defaultVals.m_pivot, vals.m_pivot);
            }
            if (!attributes.empty())
            {
                m_formatter.writeEmptyTag(TAG_PRIMARY_PIVOT, attributes);
            }
        }
        break;
    }
    case GRADING_VIDEO:
    {
        GradingPrimary defaultVals(style);
        writeRGBM(TAG_PRIMARY_LIFT, defaultVals.m_lift, vals.m_lift);
        writeRGBM(TAG_PRIMARY_GAMMA, defaultVals.m_gamma, vals.m_gamma);
        writeRGBM(TAG_PRIMARY_GAIN, defaultVals.m_gain, vals.m_gain);
        writeRGBM(TAG_PRIMARY_OFFSET, defaultVals.m_offset, vals.m_offset);

        writeScalarElement(TAG_PRIMARY_SATURATION, defaultVals.m_saturation, vals.m_saturation);

        // Pivot.
        {
            XmlFormatter::Attributes attributes;
            addAttribute(attributes, ATTR_PRIMARY_BLACK, defaultVals.m_pivotBlack, vals.m_pivotBlack);
            addAttribute(attributes, ATTR_PRIMARY_WHITE, defaultVals.m_pivotWhite, vals.m_pivotWhite);
            if (!attributes.empty())
            {
                m_formatter.writeEmptyTag(TAG_PRIMARY_PIVOT, attributes);
            }
        }
        break;
    }
    }
    // Clamp.
    {
        GradingPrimary defaultVals(GRADING_LOG);
        XmlFormatter::Attributes attributes;
        addAttribute(attributes, ATTR_PRIMARY_BLACK, defaultVals.m_clampBlack, vals.m_clampBlack);
        addAttribute(attributes, ATTR_PRIMARY_WHITE, defaultVals.m_clampWhite, vals.m_clampWhite);
        if (!attributes.empty())
        {
            m_formatter.writeEmptyTag(TAG_PRIMARY_CLAMP, attributes);
        }
    }
    if (m_primary->isDynamic())
    {
        XmlFormatter::Attributes attributes;
        attributes.push_back(XmlFormatter::Attribute(ATTR_PARAM, TAG_DYN_PROP_PRIMARY));
        m_formatter.writeEmptyTag(TAG_DYNAMIC_PARAMETER, attributes);
    }
}

///////////////////////////////////////////////////////////////////////////////

class GradingRGBCurveWriter : public OpWriter
{
public:
    GradingRGBCurveWriter() = delete;
    GradingRGBCurveWriter(const GradingRGBCurveWriter&) = delete;
    GradingRGBCurveWriter& operator=(const GradingRGBCurveWriter&) = delete;
    GradingRGBCurveWriter(XmlFormatter & formatter, ConstGradingRGBCurveOpDataRcPtr primary);
    virtual ~GradingRGBCurveWriter();

protected:
    ConstOpDataRcPtr getOp() const override;
    const char * getTagName() const override;
    void getAttributes(XmlFormatter::Attributes & attributes) const override;
    void writeContent() const override;

private:
    void writeCurve(const char * tag, const ConstGradingBSplineCurveRcPtr & curve) const;
    ConstGradingRGBCurveOpDataRcPtr m_curves;
};

GradingRGBCurveWriter::GradingRGBCurveWriter(XmlFormatter & formatter,
                                             ConstGradingRGBCurveOpDataRcPtr curves)
    : OpWriter(formatter)
    , m_curves(curves)
{
}

GradingRGBCurveWriter::~GradingRGBCurveWriter()
{
}

ConstOpDataRcPtr GradingRGBCurveWriter::getOp() const
{
    return m_curves;
}

const char * GradingRGBCurveWriter::getTagName() const
{
    return TAG_RGB_CURVE;
}

void GradingRGBCurveWriter::getAttributes(XmlFormatter::Attributes& attributes) const
{
    OpWriter::getAttributes(attributes);

    const auto style = m_curves->getStyle();
    const auto dir = m_curves->getDirection();

    const auto styleStr = ConvertGradingStyleAndDirToString(style, dir);
    attributes.push_back(XmlFormatter::Attribute(ATTR_STYLE, styleStr));

    if (m_curves->getBypassLinToLog())
    {
        attributes.push_back(XmlFormatter::Attribute(ATTR_BYPASS_LIN_TO_LOG, "true"));
    }
}

void GradingRGBCurveWriter::writeCurve(const char * tag,
                                       const ConstGradingBSplineCurveRcPtr & curve) const
{
    m_formatter.writeStartTag(tag, XmlFormatter::Attributes());
    {
        XmlScopeIndent si0(m_formatter);
        m_formatter.writeStartTag(TAG_CURVE_CTRL_PNTS, XmlFormatter::Attributes());
        {
            XmlScopeIndent si1(m_formatter);
            const size_t numPnts = curve->getNumControlPoints();

            // Write 1 control point per line in the form of "X Y"
            for (size_t i = 0; i < numPnts; ++i)
            {
                const auto & ctPt = curve->getControlPoint(i);
                std::ostringstream oss;
                SetOStream(0.f, oss);
                oss << ctPt.m_x << " " << ctPt.m_y;
                m_formatter.writeContent(oss.str());
            }
        }
        m_formatter.writeEndTag(TAG_CURVE_CTRL_PNTS);

        if (!curve->slopesAreDefault())
        {
            m_formatter.writeStartTag(TAG_CURVE_SLOPES, XmlFormatter::Attributes());
            {
                XmlScopeIndent si1(m_formatter);
                // (Number of slopes is always the same as control points.)
                const size_t numSlopes = curve->getNumControlPoints();
                std::ostringstream oss;
                SetOStream(0.f, oss);
                for (size_t i = 0; i < numSlopes; ++i)
                {
                    const float val = curve->getSlope(i);
                    oss << val << " ";
                }
                m_formatter.writeContent(oss.str());
            }
            m_formatter.writeEndTag(TAG_CURVE_SLOPES);
        }
    }

    m_formatter.writeEndTag(tag);
}

void GradingRGBCurveWriter::writeContent() const
{
    const auto & vals = m_curves->getValue();

    auto & defCurve = m_curves->getStyle() == GRADING_LIN ? GradingRGBCurveImpl::DefaultLin :
                                                            GradingRGBCurveImpl::Default;

    static const std::vector<const char *> curveTags = {
        TAG_RGB_CURVE_RED,
        TAG_RGB_CURVE_GREEN,
        TAG_RGB_CURVE_BLUE,
        TAG_RGB_CURVE_MASTER };
    for (int c = 0; c < RGB_NUM_CURVES; ++c)
    {
        const auto & curve = vals->getCurve(static_cast<RGBCurveType>(c));
        if ((*curve != defCurve) || !(curve->slopesAreDefault()))
        {
            writeCurve(curveTags[c], curve);
        }
    }
    if (m_curves->isDynamic())
    {
        XmlFormatter::Attributes attributes;
        attributes.push_back(XmlFormatter::Attribute(ATTR_PARAM, TAG_DYN_PROP_RGBCURVE));
        m_formatter.writeEmptyTag(TAG_DYNAMIC_PARAMETER, attributes);
    }
}

///////////////////////////////////////////////////////////////////////////////

class GradingToneWriter : public OpWriter
{
public:
    GradingToneWriter() = delete;
    GradingToneWriter(const GradingToneWriter&) = delete;
    GradingToneWriter& operator=(const GradingToneWriter&) = delete;
    GradingToneWriter(XmlFormatter & formatter, ConstGradingToneOpDataRcPtr tone);
    virtual ~GradingToneWriter();

protected:
    ConstOpDataRcPtr getOp() const override;
    const char * getTagName() const override;
    void getAttributes(XmlFormatter::Attributes & attributes) const override;
    void writeContent() const override;

private:
    void writeRGBMSW(const char * tag,
                     const GradingRGBMSW & defaultVal,
                     const GradingRGBMSW & val,
                     bool center, bool pivot) const;
    void writeScalarElement(const char * tag, double defaultVal, double val) const;
    void addAttribute(XmlFormatter::Attributes & attributes, const char * attr,
        double val) const;
    void addAttribute(XmlFormatter::Attributes & attributes, const char * attr,
        double defaultVal, double val) const;

    ConstGradingToneOpDataRcPtr m_tone;
};

GradingToneWriter::GradingToneWriter(XmlFormatter & formatter,
                                     ConstGradingToneOpDataRcPtr tone)
    : OpWriter(formatter)
    , m_tone(tone)
{
}

GradingToneWriter::~GradingToneWriter()
{
}

ConstOpDataRcPtr GradingToneWriter::getOp() const
{
    return m_tone;
}

const char * GradingToneWriter::getTagName() const
{
    return TAG_TONE;
}

void GradingToneWriter::getAttributes(XmlFormatter::Attributes& attributes) const
{
    OpWriter::getAttributes(attributes);

    const auto style = m_tone->getStyle();
    const auto dir = m_tone->getDirection();

    const auto styleStr = ConvertGradingStyleAndDirToString(style, dir);
    attributes.push_back(XmlFormatter::Attribute(ATTR_STYLE, styleStr));
}

void GradingToneWriter::writeRGBMSW(const char * tag,
                                    const GradingRGBMSW & defaultVal,
                                    const GradingRGBMSW & val,
                                    bool center, bool pivot) const
{
    if (val != defaultVal)
    {
        XmlFormatter::Attributes attributes;

        std::ostringstream oss;
        oss.precision(DOUBLE_PRECISION);
        oss << val.m_red << " " << val.m_green << " " << val.m_blue;
        attributes.push_back(XmlFormatter::Attribute(ATTR_RGB, oss.str()));

        oss.str("");
        oss << val.m_master;
        attributes.push_back(XmlFormatter::Attribute(ATTR_MASTER, oss.str()));

        oss.str("");
        oss << val.m_start;
        attributes.push_back(XmlFormatter::Attribute(center ? ATTR_CENTER : ATTR_START,
                                                     oss.str()));

        oss.str("");
        oss << val.m_width;
        attributes.push_back(XmlFormatter::Attribute(pivot ? ATTR_PIVOT : ATTR_WIDTH,
                                                     oss.str()));

        m_formatter.writeEmptyTag(tag, attributes);
    }
}

void GradingToneWriter::writeScalarElement(const char * tag,
                                           double defaultVal, double val) const
{
    if (val != defaultVal)
    {
        XmlFormatter::Attributes attributes;
        std::stringstream stream;
        stream.precision(DOUBLE_PRECISION);
        stream << val;
        attributes.push_back(XmlFormatter::Attribute(ATTR_MASTER, stream.str()));
        m_formatter.writeEmptyTag(tag, attributes);
    }
}

void GradingToneWriter::addAttribute(XmlFormatter::Attributes & attributes, const char * attr,
                                     double defaultVal, double val) const
{
    if (val != defaultVal)
    {
        addAttribute(attributes, attr, val);
    }
}

void GradingToneWriter::addAttribute(XmlFormatter::Attributes & attributes, const char * attr,
                                     double val) const
{
    std::stringstream master;
    master.precision(DOUBLE_PRECISION);
    master << val;
    attributes.push_back(XmlFormatter::Attribute(attr, master.str()));
}

void GradingToneWriter::writeContent() const
{
    const auto & vals = m_tone->getValue();
    GradingTone defaultVals(m_tone->getStyle());

    writeRGBMSW(TAG_TONE_BLACKS, defaultVals.m_blacks, vals.m_blacks, false, false);
    writeRGBMSW(TAG_TONE_SHADOWS, defaultVals.m_shadows, vals.m_shadows, false, true);
    writeRGBMSW(TAG_TONE_MIDTONES, defaultVals.m_midtones, vals.m_midtones, true, false);
    writeRGBMSW(TAG_TONE_HIGHLIGHTS, defaultVals.m_highlights, vals.m_highlights, false, true);
    writeRGBMSW(TAG_TONE_WHITES, defaultVals.m_whites, vals.m_whites, false, false);
    writeScalarElement(TAG_TONE_SCONTRAST, defaultVals.m_scontrast, vals.m_scontrast);

    if (m_tone->isDynamic())
    {
        XmlFormatter::Attributes attributes;
        attributes.push_back(XmlFormatter::Attribute(ATTR_PARAM, TAG_DYN_PROP_TONE));
        m_formatter.writeEmptyTag(TAG_DYNAMIC_PARAMETER, attributes);
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
        style = dir == TRANSFORM_DIR_FORWARD ? LogUtil::LOG2_STR : LogUtil::ANTI_LOG2_STR;
    }
    else if (m_log->isLog10())
    {
        style = dir == TRANSFORM_DIR_FORWARD ? LogUtil::LOG10_STR : LogUtil::ANTI_LOG10_STR;
    }
    else if (m_log->isCamera())
    {
        style = dir == TRANSFORM_DIR_FORWARD ? LogUtil::CAMERA_LIN_TO_LOG_STR :
                                               LogUtil::CAMERA_LOG_TO_LIN_STR;
    }
    else
    {
        style = dir == TRANSFORM_DIR_FORWARD ? LogUtil::LIN_TO_LOG_STR : LogUtil::LOG_TO_LIN_STR;
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
    stream.precision(DOUBLE_PRECISION);
    stream << attrValue;
    attributes.push_back(XmlFormatter::Attribute(attrName, stream.str()));
}

void AddLogParams(XmlFormatter::Attributes & attributes,
                  const LogOpData::Params & params, const double base)
{
    // LogOpData::validate ensure that params size is between 4 & 6.
    AddLogParam(attributes, ATTR_BASE, base);
    AddLogParam(attributes, ATTR_LINSIDESLOPE, params[LIN_SIDE_SLOPE]);
    AddLogParam(attributes, ATTR_LINSIDEOFFSET, params[LIN_SIDE_OFFSET]);
    AddLogParam(attributes, ATTR_LOGSIDESLOPE, params[LOG_SIDE_SLOPE]);
    AddLogParam(attributes, ATTR_LOGSIDEOFFSET, params[LOG_SIDE_OFFSET]);
    if (params.size() > 4)
    {
        AddLogParam(attributes, ATTR_LINSIDEBREAK, params[LIN_SIDE_BREAK]);
    }
    if (params.size() > 5)
    {
        AddLogParam(attributes, ATTR_LINEARSLOPE, params[LINEAR_SLOPE]);
    }
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

    // If the client requests INTERP_LINEAR, we want to write it to the
    // attribute (even though linear is what CLF specifies as its default,
    // some clients may want to lock down that behavior). INTERP_DEFAULT
    // means "do not write the attribute".
    const Interpolation interpolation = m_lut->getInterpolation();
    const char * interpolationName = GetInterpolation1DName(interpolation);
    if (interpolationName && *interpolationName)
    {
        attributes.push_back(XmlFormatter::Attribute(ATTR_INTERPOLATION, interpolationName));
    }

    if (m_lut->isInputHalfDomain())
    {
        attributes.push_back(XmlFormatter::Attribute(ATTR_HALF_DOMAIN, "true"));
    }

    if (m_lut->isOutputRawHalfs())
    {
        attributes.push_back(XmlFormatter::Attribute(ATTR_RAW_HALFS, "true"));
    }

    const Lut1DHueAdjust hueAdjust = m_lut->getHueAdjust();
    if (hueAdjust == HUE_DW3)
    {
        attributes.push_back(XmlFormatter::Attribute(ATTR_HUE_ADJUST, "dw3"));
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
    const float scale = (float)GetBitDepthMaxValue(m_outBitDepth);

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
                    m_outBitDepth,
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
    const char * interpolationName = GetInterpolation3DName(interpolation);

    // Please see comment in Lut1DWriter.
    if (interpolationName && *interpolationName)
    {

        attributes.push_back(XmlFormatter::Attribute(ATTR_INTERPOLATION, interpolationName));
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
    const float scale = (float)GetBitDepthMaxValue(m_outBitDepth);
    WriteValues(m_formatter,
                array.getValues().begin(),
                array.getValues().end(),
                3,
                m_outBitDepth,
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
    MatrixWriter(XmlFormatter & formatter, const CTFVersion & version,
                 ConstMatrixOpDataRcPtr matrix);
    virtual ~MatrixWriter();

protected:
    ConstOpDataRcPtr getOp() const override;
    const char * getTagName() const override;
    void writeContent() const override;

private:
    const CTFVersion m_version;
    ConstMatrixOpDataRcPtr m_matrix;
};

MatrixWriter::MatrixWriter(XmlFormatter & formatter, const CTFVersion & version,
                           ConstMatrixOpDataRcPtr matrix)
    : OpWriter(formatter)
    , m_version(version)
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

    const bool saveDim3{ m_version < CTF_PROCESS_LIST_VERSION_2_0 };

    auto matrix = m_matrix;
    if (matrix->getDirection() == TRANSFORM_DIR_INVERSE)
    {
        matrix = matrix->getAsForward();
    }

    std::stringstream dimensionAttr;
    if (matrix->hasAlpha())
    {
        if (matrix->hasOffsets())
        {
            dimensionAttr << (saveDim3 ? "4 5 4" : "4 5");
        }
        else
        {
            dimensionAttr << (saveDim3 ? "4 4 4" : "4 4");
        }
    }
    else if (matrix->hasOffsets())
    {
        dimensionAttr << (saveDim3 ? "3 4 3" : "3 4");
    }
    else
    {
        dimensionAttr << (saveDim3 ? "3 3 3" : "3 3");
    }

    XmlFormatter::Attributes attributes;
    attributes.push_back(XmlFormatter::Attribute(ATTR_DIMENSION,
                                                 dimensionAttr.str()));

    m_formatter.writeStartTag(TAG_ARRAY, attributes);

    const ArrayDouble::Values & values = matrix->getArray().getValues();
    const MatrixOpData::Offsets & offsets = matrix->getOffsets();

    const double outScale = GetBitDepthMaxValue(m_outBitDepth);
    const double inOutScale = outScale / GetBitDepthMaxValue(m_inBitDepth);

    if (matrix->hasAlpha())
    {
        if (matrix->hasOffsets())
        {
            // Write in 4x5x4 mode.
            const double v[20]
            {
                values[0]  * inOutScale, values[1] * inOutScale, values[2]  * inOutScale, values[3]  * inOutScale, offsets[0] * outScale,
                values[4]  * inOutScale, values[5] * inOutScale, values[6]  * inOutScale, values[7]  * inOutScale, offsets[1] * outScale,
                values[8]  * inOutScale, values[9] * inOutScale, values[10] * inOutScale, values[11] * inOutScale, offsets[2] * outScale,
                values[12] * inOutScale, values[13]* inOutScale, values[14] * inOutScale, values[15] * inOutScale, offsets[3] * outScale
            };

            WriteValues(m_formatter, v, v + 20, 5, BIT_DEPTH_F32, 1, 1.0);
        }
        else
        {
            // Write in 4x4x4 compact mode.
            const double v[16]
            {
                values[0]  * inOutScale, values[1]  * inOutScale, values[2]  * inOutScale, values[3]  * inOutScale,
                values[4]  * inOutScale, values[5]  * inOutScale, values[6]  * inOutScale, values[7]  * inOutScale,
                values[8]  * inOutScale, values[9]  * inOutScale, values[10] * inOutScale, values[11] * inOutScale,
                values[12] * inOutScale, values[13] * inOutScale, values[14] * inOutScale, values[15] * inOutScale
            };

            WriteValues(m_formatter, v, v + 16, 4, BIT_DEPTH_F32, 1, 1.0);
        }
    }
    else if (matrix->hasOffsets())
    {
        // Write in 3x4x3 compact mode.
        const double v[12]
        {
            values[0] * inOutScale, values[1] * inOutScale, values[2]  * inOutScale, offsets[0] * outScale,
            values[4] * inOutScale, values[5] * inOutScale, values[6]  * inOutScale, offsets[1] * outScale,
            values[8] * inOutScale, values[9] * inOutScale, values[10] * inOutScale, offsets[2] * outScale
        };

        WriteValues(m_formatter, v, v + 12, 4, BIT_DEPTH_F32, 1, 1.0);
    }
    else
    {
        // Write in 3x3x3 compact mode.
        const double v[9]
        {
            values[0] * inOutScale, values[1] * inOutScale, values[2]  * inOutScale,
            values[4] * inOutScale, values[5] * inOutScale, values[6]  * inOutScale,
            values[8] * inOutScale, values[9] * inOutScale, values[10] * inOutScale
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
    o.precision(DOUBLE_PRECISION);
    o << value;
    fmt.writeContentTag(tag, ' ' + o.str() + ' ');
}

void RangeWriter::writeContent() const
{
    auto range = m_range;
    if (range->getDirection() == TRANSFORM_DIR_INVERSE)
    {
        range = range->getAsForward();
    }

    const double outScale = GetBitDepthMaxValue(m_outBitDepth);
    const double inScale = GetBitDepthMaxValue(m_inBitDepth);

    if (!range->minIsEmpty())
    {
        WriteTag(m_formatter, TAG_MIN_IN_VALUE, range->getMinInValue() * inScale);
    }

    if (!range->maxIsEmpty())
    {
        WriteTag(m_formatter, TAG_MAX_IN_VALUE, range->getMaxInValue() * inScale);
    }

    if (!range->minIsEmpty())
    {
        WriteTag(m_formatter, TAG_MIN_OUT_VALUE, range->getMinOutValue() * outScale);
    }

    if (!range->maxIsEmpty())
    {
        WriteTag(m_formatter, TAG_MAX_OUT_VALUE, range->getMaxOutValue() * outScale);
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

    CTFVersion writeVersion{ CTF_PROCESS_LIST_VERSION_2_0 };
    
    std::ostringstream fversion;
    if (m_isCLF)
    {
        // Save with CLF version 3.
        fversion << 3;
        attributes.push_back(XmlFormatter::Attribute(ATTR_COMP_CLF_VERSION,
                                                     fversion.str()));

    }
    else
    {
        writeVersion = GetMinimumVersion(m_transform);
        fversion << writeVersion;

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
            m_formatter.writeContentTag(METADATA_INPUT_DESCRIPTOR, inputDesc);
        }

        const std::string & outputDesc = m_transform->getOutputDescriptor();
        if (!outputDesc.empty())
        {
            m_formatter.writeContentTag(METADATA_OUTPUT_DESCRIPTOR, outputDesc);
        }

        const FormatMetadataImpl & info = m_transform->getInfoMetadata();
        {
            writeProcessListMetadata(info);
        }

        writeOps(writeVersion);
    }

    m_formatter.writeEndTag(processListTag);
}

void TransformWriter::writeProcessListMetadata(const FormatMetadataImpl& m) const
{
    if (m.getChildrenElements().size() == 0)
    {
        const std::string infoValue(m.getElementValue());
        if (m.getNumAttributes() || !infoValue.empty())
        {
            m_formatter.writeContentTag(m.getElementName(), m.getAttributes(),
                                        m.getElementValue());
        }
    }
    else
    {
        m_formatter.writeStartTag(m.getElementName(), m.getAttributes());
        const std::string value{ m.getElementValue() };
        if (!value.empty())
        {
            m_formatter.writeContent(m.getElementValue());
        }

        const auto items = m.getChildrenElements();
        for (auto it = items.begin(), end = items.end(); it != end; ++it)
        {
            XmlScopeIndent scopeIndent(m_formatter);
            writeProcessListMetadata(*it);
        }

        m_formatter.writeEndTag(m.getElementName());
    }
}

namespace
{
void ThrowWriteOp(const std::string & type)
{
    std::ostringstream oss;
    oss << "Transform uses the '" << type << "' op which cannot be written "
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

void TransformWriter::writeOps(const CTFVersion & version) const
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
                auto exp = OCIO_DYNAMIC_POINTER_CAST<const ExponentOpData>(op);

                GammaOpData::Params paramR{ exp->m_exp4[0] };
                GammaOpData::Params paramG{ exp->m_exp4[1] };
                GammaOpData::Params paramB{ exp->m_exp4[2] };
                GammaOpData::Params paramA{ exp->m_exp4[3] };

                GammaOpDataRcPtr gammaData
                    = std::make_shared<GammaOpData>(GammaOpData::BASIC_FWD,
                                                    paramR, paramG, paramB, paramA);
                gammaData->getFormatMetadata() = exp->getFormatMetadata();
                
                if (m_isCLF && !gammaData->isAlphaComponentIdentity())
                {
                    ThrowWriteOp("Exponent with alpha");
                }

                GammaWriter opWriter(m_formatter, version, gammaData);
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
                auto gamma = OCIO_DYNAMIC_POINTER_CAST<const GammaOpData>(op);
                if (m_isCLF)
                {
                    if (!gamma->isAlphaComponentIdentity())
                    {
                        ThrowWriteOp("Gamma with alpha component");
                    }
                }

                GammaWriter opWriter(m_formatter, version, gamma);
                opWriter.setInputBitdepth(inBD);
                opWriter.setOutputBitdepth(outBD);
                opWriter.write();
                break;
            }
            case OpData::GradingPrimaryType:
            {
                if (m_isCLF)
                {
                    ThrowWriteOp("GradingPrimary");
                }

                auto prim = OCIO_DYNAMIC_POINTER_CAST<const GradingPrimaryOpData>(op);
                GradingPrimaryWriter opWriter(m_formatter, prim);
                opWriter.setInputBitdepth(inBD);
                opWriter.setOutputBitdepth(outBD);
                opWriter.write();
                break;
            }
            case OpData::GradingRGBCurveType:
            {
                if (m_isCLF)
                {
                    ThrowWriteOp("GradingRGBCurve");
                }

                auto rgb = OCIO_DYNAMIC_POINTER_CAST<const GradingRGBCurveOpData>(op);
                GradingRGBCurveWriter opWriter(m_formatter, rgb);
                opWriter.setInputBitdepth(inBD);
                opWriter.setOutputBitdepth(outBD);
                opWriter.write();
                break;
            }
            case OpData::GradingToneType:
            {
                if (m_isCLF)
                {
                    ThrowWriteOp("GradingTone");
                }

                auto tone = OCIO_DYNAMIC_POINTER_CAST<const GradingToneOpData>(op);
                GradingToneWriter opWriter(m_formatter, tone);
                opWriter.setInputBitdepth(inBD);
                opWriter.setOutputBitdepth(outBD);
                opWriter.write();
                break;
            }
            case OpData::LogType:
            {
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
                        ThrowWriteOp("Matrix with alpha component");
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

                MatrixWriter opWriter(m_formatter, version, mat);
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

                RangeWriter opWriter(m_formatter, range);
                opWriter.setInputBitdepth(inBD);
                opWriter.setOutputBitdepth(outBD);

                opWriter.write();
                break;
            }
            case OpData::ReferenceType:
            {
                throw Exception("Reference ops should have been replaced by their content.");
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

        MatrixWriter opWriter(m_formatter, version, mat);
        opWriter.setInputBitdepth(BIT_DEPTH_F32);
        opWriter.setOutputBitdepth(BIT_DEPTH_F32);

        opWriter.write();
    }

}

} // namespace OCIO_NAMESPACE

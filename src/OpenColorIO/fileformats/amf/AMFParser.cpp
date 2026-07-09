// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>
#include <unordered_map>
#include <stack>
#include <regex>
#include <cstring>

#include "fileformats/amf/AMFParser.h"
#include "expat.h"

#include <Platform.h>

namespace OCIO_NAMESPACE
{

// ----------------------------------------------------------------------------
// NOTE(aces-amf): Potential future integration of the official ACES AMF library
// ----------------------------------------------------------------------------
// The Academy is developing an official ACES AMF library (currently Python at
// https://github.com/ampas/aces-amf, with a C++ version planned before its
// official release). If/when a stable, packaged C++ library is available, it
// would be a good candidate to replace the AMF *front-end* below, likely as an
// optional dependency (AMF support built only when the library is present).
//
// The library would REPLACE (XML parsing / model / validation front-end):
//   * The expat-based parse layer: parse(), StartElementHandler,
//     EndElementHandler, CharacterDataHandler, the Handle*StartElement /
//     Handle*EndElement helpers and IsValidElement.
//   * The intermediate parsed representation: the AMFTransform /
//     AMFInputTransform / AMFOutputTransform structures and the m_input /
//     m_output / m_look / m_clipId members (the library exposes its own
//     AMF object model).
//   * Ad-hoc file/format validation: checkLutPath(), the file-open check in
//     parse(), extractThreeFloats(), getCCCId(), getFileDescription() -- the
//     library provides schema + semantic validation and typed accessors.
//   * mustApply() and amfReferencesAces2Transforms(), which would read the
//     'applied' attribute / transform IDs from the library's object model.
//
// The library would NOT replace (OCIO-specific AMF -> Config mapping; stays):
//   * initAMFConfig(), processInputTransform(), processOutputTransform(),
//     processOutputTransformId(), processLookTransforms()/processLookTransform(),
//     loadCdlWsTransform(), handleWorkingLocation(), determineClipColorSpace().
//   * Reference config selection + transform-ID resolution against the OCIO
//     config: loadACESRefConfig(), searchColorSpaces()/searchViewTransforms()/
//     searchLookTransforms(), ElementMatchesTransformId(), refRoleColorSpace(),
//     clearCopiedInteropIds().
//
// Integration boundary: feed the library's parsed AMF object model into the
// process*() methods in place of the m_input/m_output/m_look/m_clipId members.
//
// Validation: the boundary above was prototyped end-to-end against the Python
// aces-amf-lib v0.1.0 (parse -> object model -> this file's mapping logic ->
// OCIO processor). Its object model exposes everything process*() needs, as
// typed values: pipeline.input_transform / .look_transforms / .output_transform
// with .applied, .transform_id, .file, .asc_sop (.slope/.offset/.power as
// floats), .asc_sat.saturation, and .cdl_working_space.to/from_cdl_working_space
// .transform_id; clip_id.clip_name; plus a working-location helper. A C++ port
// is expected to mirror this shape.
//
// Tradeoff to weigh before adopting: the official library is currently AMF
// v2.0-only and strictly validates the schema and required fields (e.g. it
// rejects AMFs missing amfInfo/pipelineInfo/clipId uuid or dateTime, and does
// not read amf:v1.0 documents). This expat-based reader is intentionally more
// permissive and also handles v1.0. Adopting the library would tighten
// validation (a feature) but would reject looser/older AMFs this reader accepts.
// ----------------------------------------------------------------------------

static constexpr char ACES[] = "ACES2065-1";

static constexpr char ACES_LOOK_NAME[] = "ACES Look Transform";
static constexpr char CONTEXT_NAME[] = "SHOT_LOOKS";

static constexpr char AMF_TAG_CLIPID[] = "aces:clipId";
static constexpr char AMF_TAG_CLIPNAME[] = "aces:clipName";
static constexpr char AMF_TAG_UUID[] = "aces:uuid";
static constexpr char AMF_TAG_DESC[] = "aces:description";

static constexpr char AMF_TAG_INPUT_TRANSFORM[] = "aces:inputTransform";
static constexpr char AMF_TAG_OUTPUT_TRANSFORM[] = "aces:outputTransform";
static constexpr char AMF_TAG_LOOK_TRANSFORM[] = "aces:lookTransform";
static constexpr char AMF_TAG_WORKING_LOCATION[] = "aces:workingLocation";

static constexpr char AMF_TAG_TRANSFORMID[] = "aces:transformId";
static constexpr char AMF_TAG_FILE[] = "aces:file";
static constexpr char AMF_TAG_CDLCCR[] = "cdl:ColorCorrectionRef";

static constexpr char AMF_TAG_IODT[] = "aces:inverseOutputDeviceTransform";
static constexpr char AMF_TAG_IRRT[] = "aces:inverseReferenceRenderingTransform";
static constexpr char AMF_TAG_ODT[] = "aces:outputDeviceTransform";
static constexpr char AMF_TAG_RRT[] = "aces:referenceRenderingTransform";

static constexpr char AMF_TAG_CDLWS[] = "aces:cdlWorkingSpace";
static constexpr char AMF_TAG_TOCDLWS[] = "aces:toCdlWorkingSpace";
static constexpr char AMF_TAG_FROMCDLWS[] = "aces:fromCdlWorkingSpace";
static constexpr char AMF_TAG_SOPNODE[] = "cdl:SOPNode";
static constexpr char AMF_TAG_ASCSOP[] = "cdl:ASC_SOP";
static constexpr char AMF_TAG_SLOPE[] = "cdl:Slope";
static constexpr char AMF_TAG_OFFSET[] = "cdl:Offset";
static constexpr char AMF_TAG_POWER[] = "cdl:Power";
static constexpr char AMF_TAG_SATNODE[] = "cdl:SatNode";
static constexpr char AMF_TAG_ASCSAT[] = "cdl:ASC_SAT";
static constexpr char AMF_TAG_SAT[] = "cdl:Saturation";

static constexpr char AMF_TAG_PIPELINE[] = "aces:pipeline";

static constexpr char AMF_NO_WORKING_LOCATION[] = "";
static constexpr char AMF_PRE_WORKING_LOCATION[] = "Pre-working-location";
static constexpr char AMF_POST_WORKING_LOCATION[] = "Post-working-location";

// Table of mappings from all log camera color spaces in the current Studio config
// to their linearized camera color space.
static const std::unordered_map<std::string, std::string> CAMERA_MAPPING =
{
    {"ARRI LogC3 (EI800)", "Linear ARRI Wide Gamut 3"},
    {"ARRI LogC4", "Linear ARRI Wide Gamut 4"},
    {"BMDFilm WideGamut Gen5", "Linear BMD WideGamut Gen5"},
    {"CanonLog2 CinemaGamut D55", "Linear CinemaGamut D55"},
    {"CanonLog3 CinemaGamut D55", "Linear CinemaGamut D55"},
    {"V-Log V-Gamut", "Linear V-Gamut"},
    {"Log3G10 REDWideGamutRGB", "Linear REDWideGamutRGB"},
    {"S-Log3 S-Gamut3", "Linear S-Gamut3"},
    {"S-Log3 S-Gamut3.Cine", "Linear S-Gamut3.Cine"},
    {"S-Log3 Venice S-Gamut3", "Linear Venice S-Gamut3"},
    {"S-Log3 Venice S-Gamut3.Cine", "Linear Venice S-Gamut3.Cine"}
};

namespace
{

// The "amf_transform_ids" interchange attribute stores a newline-separated list
// of ACES Transform IDs.  Split it into individual (trimmed) IDs.
std::vector<std::string> SplitAmfTransformIds(const char* ids)
{
    std::vector<std::string> result;
    if (!ids || !*ids)
        return result;

    std::istringstream iss(ids);
    std::string line;
    while (std::getline(iss, line))
    {
        const size_t begin = line.find_first_not_of(" \t\r\n");
        if (begin == std::string::npos)
            continue;
        const size_t end = line.find_last_not_of(" \t\r\n");
        result.push_back(line.substr(begin, end - begin + 1));
    }
    return result;
}

// NOTE(aces-amf): this resolves an AMF Transform ID against the *OCIO config*
// and therefore stays OCIO-specific even if the ACES AMF library is adopted.
// The library (and the official ACES Transform ID Registry) could still be used
// upstream of this to canonicalize IDs (e.g. resolve v1.5 <-> v2.0 equivalents)
// before they reach this matcher.
//
// Determine whether a config element (color space, view transform or look)
// corresponds to the supplied ACES Transform ID.
//
// For OCIO 2.5+ configs (useAmfIds == true) the match is done against the
// dedicated "amf_transform_ids" interchange attribute, which holds the exact
// list of Transform IDs the element implements.  For older configs the ID is
// matched as a substring of the element description (the legacy heuristic).
template <typename ElementRcPtr>
bool ElementMatchesTransformId(const ElementRcPtr & element,
                               const std::string & acesId,
                               bool useAmfIds)
{
    if (useAmfIds)
    {
        const std::vector<std::string> ids =
            SplitAmfTransformIds(element->getInterchangeAttribute("amf_transform_ids"));
        for (const auto & id : ids)
        {
            if (id == acesId)
                return true;
        }
        return false;
    }

    const std::string desc = element->getDescription();
    return desc.find(acesId) != std::string::npos;
}

} // anonymous namespace

class AMFParser::Impl
{
private:
    class AMFTransform;
    typedef OCIO_SHARED_PTR<const AMFTransform> ConstAMFTransformRcPtr;
    typedef OCIO_SHARED_PTR<AMFTransform> AMFTransformRcPtr;

    class AMFTransform
    {
    public:
        void reset()
        {
            m_subElements.clear();
            m_attributes.clear();
        }

        static AMFTransformRcPtr Create()
        {
            return AMFTransformRcPtr(new AMFTransform(), &deleter);
        }

        static void deleter(AMFTransform* t)
        {
            delete t;
        }

        void addSubElement(const std::string& name, const std::string& value)
        {
            m_subElements.push_back(std::make_pair(name, value));
        }

        void addAttribute(const std::string& name, const std::string& value)
        {
            m_attributes.push_back(std::make_pair(name, value));
        }

        bool empty()
        {
            return m_attributes.empty() && m_subElements.empty();
        }

        std::vector<std::pair<std::string, std::string>> m_subElements;
        std::vector<std::pair<std::string, std::string>> m_attributes;
    };

    class AMFOutputTransform : public AMFTransform
    {
    public:
        void reset()
        {
            AMFTransform::reset();
            m_tldTemp = std::stack<std::string>();
            m_tldElements.clear();
        }

        void addTld(const std::string& name)
        {
            m_tldTemp.push(name);
        }

        void removeTld()
        {
            m_tldTemp.pop();
        }

        void addTldElement(const std::string& name, const std::string& value)
        {
            m_tldElements.push_back(std::make_pair(name, value));
        }

        bool empty()
        {
            return m_attributes.empty() && m_tldElements.empty() && m_subElements.empty();
        }

        std::stack<std::string> m_tldTemp;
        std::vector<std::pair<std::string, std::string>> m_tldElements;
    };

    class AMFInputTransform : public AMFOutputTransform
    {
    public:
        AMFInputTransform() : m_isInverse(false) {};

        void reset()
        {
            AMFOutputTransform::reset();
            m_isInverse = false;
        }

        bool empty()
        {
            return m_attributes.empty() && m_tldElements.empty() && m_subElements.empty();
        }

        bool m_isInverse;
    };

public:
    Impl(const Impl &) = delete;
    Impl & operator=(const Impl &) = delete;

    explicit Impl()
        : m_parser(XML_ParserCreate(NULL))
        , m_lineNumber(0)
        , m_refConfig(NULL)
        , m_amfConfig(NULL)
        , m_amfInfoObject(NULL)
        , m_isInsideInputTransform(false)
        , m_isInsideOutputTransform(false)
        , m_isInsideLookTransform(false)
        , m_isInsideClipId(false)
        , m_isInsidePipeline(false)
        , m_numLooksBeforeWorkingLocation(-1)
        , m_useAmfIds(false) {};

    ~Impl()
    {
        reset();
        XML_ParserFree(m_parser);
    }

    ConstConfigRcPtr parse(AMFInfoRcPtr amfInfoObject, const char* amfFilePath, const char* configFilePath = NULL);

private:
    void reset();

    void parse(const std::string & buffer, bool lastLine);

    static void StartElementHandler(void* userData, const XML_Char* name, const XML_Char** atts);
    static bool HandleInputTransformStartElement(AMFParser::Impl* pImpl, const XML_Char* name, const XML_Char** atts);
    static bool HandleOutputTransformStartElement(AMFParser::Impl* pImpl, const XML_Char* name, const XML_Char** atts);
    static bool HandleLookTransformStartElement(AMFParser::Impl* pImpl, const XML_Char* name, const XML_Char** atts);
    static bool HandleClipIdStartElement(AMFParser::Impl* pImpl, const XML_Char* name, const XML_Char** atts);
    static bool HandlePipelineStartElement(AMFParser::Impl* pImpl, const XML_Char* name);

    static void EndElementHandler(void* userData, const XML_Char* name);
    static bool HandleInputTransformEndElement(AMFParser::Impl* pImpl, const XML_Char* name);
    static bool HandleOutputTransformEndElement(AMFParser::Impl* pImpl, const XML_Char* name);
    static bool HandleLookTransformEndElement(AMFParser::Impl* pImpl, const XML_Char* name);
    static bool HandleClipIdEndElement(AMFParser::Impl* pImpl, const XML_Char* name);
    static bool HandlePipelineEndElement(AMFParser::Impl* pImpl, const XML_Char* name);

    static void CharacterDataHandler(void* userData, const XML_Char* s, int len);

    static bool IsValidElement(AMFParser::Impl* pImpl, const XML_Char* name);

    void processInputTransform();
    void processOutputTransform();
    void processLookTransforms();
    void processClipId();

    void loadACESRefConfig(const char* configFilePath = NULL);
    void initAMFConfig();

    // Returns true if the parsed AMF references any ACES v2.0 transform IDs,
    // used to auto-select an ACES 2 reference config when none is supplied.
    bool amfReferencesAces2Transforms() const;
    // Name of the reference config color space assigned to the given role,
    // or the supplied fallback when the role is not defined.
    std::string refRoleColorSpace(const char* role, const char* fallback) const;
    // Clear interop IDs on color spaces copied from the reference config. The
    // generated AMF config is a minimal derived config that does not carry the
    // reference config's full set of interop targets, so leaving the IDs in
    // place would fail config validation.
    void clearCopiedInteropIds();

    void processOutputTransformId(const char* transformId, TransformDirection tDirection);
    void addInactiveCS(const char* csName);
    ConstViewTransformRcPtr searchViewTransforms(std::string acesId);

    bool processLookTransform(AMFTransform& look, int index, std::string workingLocation);
    void loadCdlWsTransform(AMFTransform& amft, bool isTo, TransformRcPtr& t);
    void extractThreeFloats(std::string str, double* arr);
    bool mustApply(AMFTransform& amft);
    void getCCCId(AMFTransform& amft, std::string& cccId);
    LookRcPtr searchLookTransforms(std::string acesId);

    ConstColorSpaceRcPtr searchColorSpaces(std::string acesId);
    void getFileDescription(AMFTransform& amft, std::string& desc);
    void getPath(std::string& path);
    void checkLutPath(std::string& lutPath);
    void determineClipColorSpace();
    void handleWorkingLocation();

    void throwMessage(const std::string& error) const;

    XML_Parser m_parser;
    std::string m_xmlFilePath;
    std::ifstream m_xmlStream;
    unsigned int m_lineNumber;

    ConstConfigRcPtr m_refConfig;
    ConfigRcPtr m_amfConfig;

    AMFInfoRcPtr m_amfInfoObject;
    AMFTransform m_clipId;
    AMFInputTransform m_input;
    AMFOutputTransform m_output;
    std::vector<AMFTransformRcPtr> m_look;
    bool m_isInsideInputTransform, m_isInsideOutputTransform, m_isInsideLookTransform, m_isInsideClipId, m_isInsidePipeline;
    std::string m_currentElement, m_clipName;
    // Number of look transforms encountered before the workingLocation marker.
    // A value of -1 means the AMF has no workingLocation marker, so this must be
    // a signed type (a size_t sentinel of -1 wraps to SIZE_MAX and breaks the
    // "< 0" checks below).
    long m_numLooksBeforeWorkingLocation;

    // Whether the reference config exposes the "amf_transform_ids" interchange
    // attribute (OCIO 2.5+). When false, transform IDs are matched against the
    // color space / view transform / look description text instead.
    bool m_useAmfIds;
};

void AMFParser::Impl::reset()
{
    m_xmlFilePath.clear();
    if (m_xmlStream.is_open())
        m_xmlStream.close();
    m_lineNumber = 0;
    m_refConfig = m_amfConfig = NULL;
    m_amfInfoObject = NULL;
    m_clipId.reset();
    m_input.reset();
    m_output.reset();
    for (auto& elem : m_look)
        elem.reset();
    m_isInsideInputTransform = m_isInsideOutputTransform = m_isInsideLookTransform = m_isInsideClipId = m_isInsidePipeline = false;
    m_currentElement.clear();
    m_clipName.clear();
    m_numLooksBeforeWorkingLocation = -1;
    m_useAmfIds = false;
}

ConstConfigRcPtr AMFParser::Impl::parse(AMFInfoRcPtr amfInfoObject, const char* amfFilePath, const char* configFilePath)
{
    reset();

    m_xmlFilePath = amfFilePath;
    m_xmlStream.open(m_xmlFilePath, std::ios_base::in);
    if (!m_xmlStream.is_open())
    {
        throw Exception(("Could not open AMF file: " + m_xmlFilePath).c_str());
    }
    m_amfInfoObject = amfInfoObject;

    XML_SetUserData(m_parser, this);
    XML_SetCharacterDataHandler(m_parser, CharacterDataHandler);
    XML_SetElementHandler(m_parser, StartElementHandler, EndElementHandler);

    // NOTE(aces-amf): swap point. An official ACES AMF C++ library would replace
    // this expat parse loop (and the handlers/structs it feeds) with a single
    // call that loads + validates the AMF into a typed object model. The
    // process*() calls below would then read from that model instead of
    // m_input/m_output/m_look/m_clipId. See the file header for the full map.
    std::string line;
    m_lineNumber = 0;
    while (m_xmlStream.good())
    {
        std::getline(m_xmlStream, line);
        line.push_back('\n');
        ++m_lineNumber;

        parse(line, !m_xmlStream.good());
    }

    // The reference config is selected only after parsing, so that the AMF's
    // own transform IDs can drive the choice between an ACES 1.x and an ACES 2
    // reference config when the caller did not supply one explicitly.
    loadACESRefConfig(configFilePath);

    initAMFConfig();

    processClipId();
    processInputTransform();
    processLookTransforms();
    processOutputTransform();

    handleWorkingLocation();

    m_amfInfoObject->displayName = m_amfConfig->getDisplay(0);
    m_amfInfoObject->viewName = m_amfConfig->getView(m_amfInfoObject->displayName, 0);
    determineClipColorSpace();

    std::regex nonAlnum("[^0-9a-zA-Z_]");
    std::string newName = std::regex_replace(m_clipName, nonAlnum, "");
    std::string roleName = "amf_clip_" + newName;
    m_amfConfig->setRole(roleName.c_str(), m_amfInfoObject->clipColorSpaceName);

    int numRoles = m_amfConfig->getNumRoles();
    for (int i = 0; i < numRoles; i++)
    {
        if (0 == Platform::Strcasecmp(m_amfConfig->getRoleName(i), roleName.c_str()))
        {
            m_amfInfoObject->clipIdentifier = m_amfConfig->getRoleName(i);
            break;
        }
    }

    clearCopiedInteropIds();

    m_amfConfig->validate();

    return m_amfConfig;
}

void AMFParser::Impl::parse(const std::string& buffer, bool lastLine)
{
    const int done = lastLine ? 1 : 0;

    if (XML_STATUS_ERROR == XML_Parse(m_parser, buffer.c_str(), (int)buffer.size(), done))
    {
        std::string error("XML parsing error: ");
        error += XML_ErrorString(XML_GetErrorCode(m_parser));
        throwMessage(error);
    }

}

void AMFParser::Impl::StartElementHandler(void* userData, const XML_Char* name, const XML_Char* *atts)
{
    AMFParser::Impl* pImpl = (AMFParser::Impl*)userData;
    if (IsValidElement(pImpl, name))
    {
        if (HandleClipIdStartElement(pImpl, name, atts))
        {
        }
        else if (HandlePipelineStartElement(pImpl, name))
        {
            if (0 == Platform::Strcasecmp(name, AMF_TAG_WORKING_LOCATION))
            {
                pImpl->m_numLooksBeforeWorkingLocation = static_cast<long>(pImpl->m_look.size());
            }
            else if (HandleInputTransformStartElement(pImpl, name, atts) ||
                        HandleOutputTransformStartElement(pImpl, name, atts) ||
                        HandleLookTransformStartElement(pImpl, name, atts))
            {
            }
        }
    }
}

bool AMFParser::Impl::HandleInputTransformStartElement(AMFParser::Impl* pImpl, const XML_Char* name, const XML_Char** atts)
{
    if ((0 == Platform::Strcasecmp(name, AMF_TAG_INPUT_TRANSFORM)))
    {
        pImpl->m_isInsideInputTransform = true;
        for (int i = 0; atts[i]; i += 2)
        {
            const char* attrName = atts[i];
            const char* attrValue = atts[i + 1];
            pImpl->m_input.addAttribute(attrName, attrValue);
        }
        pImpl->m_input.addTld(name);
        return true;
    }
    else if (pImpl->m_isInsideInputTransform)
    {
        pImpl->m_currentElement = name;
        if (0 == Platform::Strcasecmp(name, AMF_TAG_IODT) || 0 == Platform::Strcasecmp(name, AMF_TAG_IRRT))
        {
            pImpl->m_input.m_isInverse = true;
            pImpl->m_input.addTld(name);
        }
        return true;
    }

    return false;
}

bool AMFParser::Impl::HandleOutputTransformStartElement(AMFParser::Impl* pImpl, const XML_Char* name, const XML_Char** atts)
{
    if ((0 == Platform::Strcasecmp(name, AMF_TAG_OUTPUT_TRANSFORM)))
    {
        pImpl->m_isInsideOutputTransform = true;
        for (int i = 0; atts[i]; i += 2)
        {
            const char* attrName = atts[i];
            const char* attrValue = atts[i + 1];
            pImpl->m_output.addAttribute(attrName, attrValue);
        }
        pImpl->m_output.addTld(name);
        return true;
    }
    else if (pImpl->m_isInsideOutputTransform)
    {
        pImpl->m_currentElement = name;
        if (0 == Platform::Strcasecmp(name, AMF_TAG_ODT) || 0 == Platform::Strcasecmp(name, AMF_TAG_RRT))
            pImpl->m_output.addTld(name);
        return true;
    }

    return false;
}

bool AMFParser::Impl::HandleLookTransformStartElement(AMFParser::Impl* pImpl, const XML_Char* name, const XML_Char** atts)
{
    if ((0 == Platform::Strcasecmp(name, AMF_TAG_LOOK_TRANSFORM)))
    {
        pImpl->m_isInsideLookTransform = true;
        AMFTransformRcPtr amfTransform = AMFTransform::Create();
        pImpl->m_look.push_back(amfTransform);
        for (int i = 0; atts[i]; i += 2)
        {
            const char* attrName = atts[i];
            const char* attrValue = atts[i + 1];
            amfTransform->addAttribute(attrName, attrValue);
        }
        return true;
    }
    else if (pImpl->m_isInsideLookTransform)
    {
        pImpl->m_currentElement = name;
        if ((0 == Platform::Strcasecmp(name, AMF_TAG_CDLCCR)))
        {
            for (int i = 0; atts[i]; i += 2)
            {
                const char* attrValue = atts[i + 1];
                pImpl->m_look.back()->addSubElement(AMF_TAG_CDLCCR, attrValue);
            }
        }
        return true;
    }

    return false;
}

bool AMFParser::Impl::HandleClipIdStartElement(AMFParser::Impl* pImpl, const XML_Char* name, const XML_Char** atts)
{
    if ((0 == Platform::Strcasecmp(name, AMF_TAG_CLIPID)))
    {
        pImpl->m_isInsideClipId = true;
        for (int i = 0; atts[i]; i += 2)
        {
            const char* attrName = atts[i];
            const char* attrValue = atts[i + 1];
            pImpl->m_clipId.addAttribute(attrName, attrValue);
        }
        return true;
    }
    else if (pImpl->m_isInsideClipId)
    {
        pImpl->m_currentElement = name;
        return true;
    }

    return false;
}

bool AMFParser::Impl::HandlePipelineStartElement(AMFParser::Impl* pImpl, const XML_Char* name)
{
    if ((0 == Platform::Strcasecmp(name, AMF_TAG_PIPELINE)))
    {
        pImpl->m_isInsidePipeline = true;
        return true;
    }
    else if (pImpl->m_isInsidePipeline)
    {
        return true;
    }

    return false;
}

void AMFParser::Impl::EndElementHandler(void* userData, const XML_Char* name)
{
    AMFParser::Impl* pImpl = (AMFParser::Impl*)userData;
    if (IsValidElement(pImpl, name))
    {
        if (HandleClipIdEndElement(pImpl, name))
        {
        }
        else if (HandlePipelineEndElement(pImpl, name))
        {
            if (HandleInputTransformEndElement(pImpl, name) ||
                HandleOutputTransformEndElement(pImpl, name) ||
                HandleLookTransformEndElement(pImpl, name))
            {
            }
        }
    }
}

bool AMFParser::Impl::HandleInputTransformEndElement(AMFParser::Impl* pImpl, const XML_Char* name)
{
    if ((0 == Platform::Strcasecmp(name, AMF_TAG_INPUT_TRANSFORM)))
    {
        pImpl->m_isInsideInputTransform = false;
        pImpl->m_input.removeTld();
        return true;
    }
    else if (pImpl->m_isInsideInputTransform)
    {
        pImpl->m_currentElement.clear();
        if (0 == Platform::Strcasecmp(name, AMF_TAG_IODT) || 0 == Platform::Strcasecmp(name, AMF_TAG_IRRT))
            pImpl->m_input.removeTld();
        return true;
    }

    return false;
}

bool AMFParser::Impl::HandleOutputTransformEndElement(AMFParser::Impl* pImpl, const XML_Char* name)
{
    if ((0 == Platform::Strcasecmp(name, AMF_TAG_OUTPUT_TRANSFORM)))
    {
        pImpl->m_isInsideOutputTransform = false;
        pImpl->m_output.removeTld();
        return true;
    }
    else if (pImpl->m_isInsideOutputTransform)
    {
        pImpl->m_currentElement.clear();
        if (0 == Platform::Strcasecmp(name, AMF_TAG_ODT) || 0 == Platform::Strcasecmp(name, AMF_TAG_RRT))
            pImpl->m_output.removeTld();
        return true;
    }

    return false;
}

bool AMFParser::Impl::HandleLookTransformEndElement(AMFParser::Impl* pImpl, const XML_Char* name)
{
    if ((0 == Platform::Strcasecmp(name, AMF_TAG_LOOK_TRANSFORM)))
    {
        pImpl->m_isInsideLookTransform = false;
        return true;
    }
    else if (pImpl->m_isInsideLookTransform)
    {
        pImpl->m_currentElement.clear();
        return true;
    }

    return false;
}

bool AMFParser::Impl::HandleClipIdEndElement(AMFParser::Impl* pImpl, const XML_Char* name)
{
    if ((0 == Platform::Strcasecmp(name, AMF_TAG_CLIPID)))
    {
        pImpl->m_isInsideClipId = false;
        return true;
    }
    else if (pImpl->m_isInsideClipId)
    {
        pImpl->m_currentElement.clear();
        return true;
    }

    return false;
}

bool AMFParser::Impl::HandlePipelineEndElement(AMFParser::Impl* pImpl, const XML_Char* name)
{
    if ((0 == Platform::Strcasecmp(name, AMF_TAG_PIPELINE)))
    {
        pImpl->m_isInsidePipeline = false;
        return true;
    }
    else if (pImpl->m_isInsidePipeline)
    {
        return true;
    }

    return false;
}

void AMFParser::Impl::CharacterDataHandler(void* userData, const XML_Char* s, int len)
{
    AMFParser::Impl* pImpl = (AMFParser::Impl*)userData;

    if (len == 0)
        return;
    if (len < 0 || !s || !*s)
        pImpl->throwMessage("XML parsing error: attribute illegal");
    if (len == 1 && s[0] == '\n')
        return;

    std::string value(s, len);
    if (pImpl->m_isInsideInputTransform && !pImpl->m_currentElement.empty())
    {
        std::string currentParentElement = pImpl->m_input.m_tldTemp.empty() ? "" : pImpl->m_input.m_tldTemp.top();
        if (0 == Platform::Strcasecmp(currentParentElement.c_str(), AMF_TAG_INPUT_TRANSFORM))
            pImpl->m_input.addTldElement(pImpl->m_currentElement, value);
        else if (0 == Platform::Strcasecmp(currentParentElement.c_str(), AMF_TAG_IODT))
            pImpl->m_input.addSubElement(pImpl->m_currentElement, value);
    }
    else if (pImpl->m_isInsideOutputTransform && !pImpl->m_currentElement.empty())
    {
        std::string currentParentElement = pImpl->m_output.m_tldTemp.empty() ? "" : pImpl->m_output.m_tldTemp.top();
        if (0 == Platform::Strcasecmp(currentParentElement.c_str(), AMF_TAG_OUTPUT_TRANSFORM))
            pImpl->m_output.addTldElement(pImpl->m_currentElement, value);
        else if (0 == Platform::Strcasecmp(currentParentElement.c_str(), AMF_TAG_ODT))
            pImpl->m_output.addSubElement(pImpl->m_currentElement, value);
    }
    else if (pImpl->m_isInsideLookTransform && !pImpl->m_currentElement.empty())
        pImpl->m_look.back()->addSubElement(pImpl->m_currentElement, value);
    else if (pImpl->m_isInsideClipId && !pImpl->m_currentElement.empty())
        pImpl->m_clipId.addSubElement(pImpl->m_currentElement, value);
}

bool AMFParser::Impl::IsValidElement(AMFParser::Impl* pImpl, const XML_Char* name)
{
    if (!pImpl)
    {
        throw Exception("Internal AMF parsing error.");
    }

    if (!name || !*name)
    {
        pImpl->throwMessage("Internal parsing error");
    }

    return true;
}

void AMFParser::Impl::processInputTransform()
{
    for (auto& elem : m_input.m_tldElements)
    {
        if (0 == Platform::Strcasecmp(elem.first.c_str(), AMF_TAG_TRANSFORMID))
        {
            ConstColorSpaceRcPtr cs = searchColorSpaces(elem.second.c_str());
            if (cs != NULL)
            {
                m_amfConfig->addColorSpace(cs);
                m_amfInfoObject->inputColorSpaceName = m_amfConfig->getColorSpace(cs->getName())->getName();

                auto it = CAMERA_MAPPING.find(cs->getName());
                if (it != CAMERA_MAPPING.end())
                {
                    ConstColorSpaceRcPtr lin_cs = m_refConfig->getColorSpace(it->second.c_str());
                    m_amfConfig->addColorSpace(lin_cs);
                }
            }
        }
        else if (0 == Platform::Strcasecmp(elem.first.c_str(), AMF_TAG_FILE))
        {
            FileTransformRcPtr ft = FileTransform::Create();
            checkLutPath(elem.second);
            ft->setSrc(elem.second.c_str());
            ft->setCCCId("");
            ft->setInterpolation(INTERP_BEST);
            ft->setDirection(TRANSFORM_DIR_FORWARD);

            std::string name = "AMF Input Transform -- " + m_clipName;
            std::string family = "AMF/" + m_clipName;
            ColorSpaceRcPtr cs = ColorSpace::Create();
            cs->setName(name.c_str());
            cs->setFamily(family.c_str());
            cs->addCategory("file-io");
            cs->setTransform(ft, COLORSPACE_DIR_TO_REFERENCE);

            m_amfConfig->addColorSpace(cs);
            m_amfInfoObject->inputColorSpaceName = m_amfConfig->getColorSpace(cs->getName())->getName();
        }
    }

    auto it = m_input.m_subElements.begin();
    while (it != m_input.m_subElements.end())
    {
        if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_IODT))
        {
            for (++it; it != m_input.m_subElements.end(); ++it)
            {
                if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_TRANSFORMID))
                {
                    processOutputTransformId(it->second.c_str(), TRANSFORM_DIR_INVERSE);
                }
                else if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_FILE))
                {
                    FileTransformRcPtr odtFt = FileTransform::Create();
                    checkLutPath(it->second);
                    odtFt->setSrc(it->second.c_str());
                    odtFt->setCCCId("");
                    odtFt->setInterpolation(INTERP_BEST);
                    odtFt->setDirection(TRANSFORM_DIR_INVERSE);

                    FileTransformRcPtr rrtFt = FileTransform::Create();
                    for (auto itRrt = m_input.m_subElements.begin(); itRrt != m_input.m_subElements.end(); itRrt++)
                    {
                        if (0 == Platform::Strcasecmp(itRrt->first.c_str(), AMF_TAG_RRT))
                        {
                            ++itRrt;
                            while (itRrt != m_input.m_subElements.end() && Platform::Strcasecmp(itRrt->first.c_str(), AMF_TAG_RRT))
                            {
                                if (0 == Platform::Strcasecmp(itRrt->first.c_str(), AMF_TAG_FILE))
                                {
                                    checkLutPath(itRrt->second);
                                    odtFt->setSrc(itRrt->second.c_str());
                                    odtFt->setCCCId("");
                                    odtFt->setInterpolation(INTERP_BEST);
                                    odtFt->setDirection(TRANSFORM_DIR_INVERSE);
                                    break;
                                }
                            }
                        }
                    }

                    std::string name = "AMF Input Transform LUT -- " + m_clipName;
                    std::string viewName = name;
                    std::string dispName;
                    getFileDescription(m_input, dispName);
                    std::string family = "AMF/" + m_clipName;
                    ColorSpaceRcPtr cs = ColorSpace::Create();
                    cs->setName(name.c_str());
                    cs->setFamily(family.c_str());
                    cs->addCategory("file-io");

                    GroupTransformRcPtr gt = GroupTransform::Create();
                    if (rrtFt)
                        gt->appendTransform(rrtFt);
                    gt->appendTransform(odtFt);
                    cs->setTransform(gt, COLORSPACE_DIR_FROM_REFERENCE);
                    m_amfConfig->addDisplayView(dispName.c_str(), viewName.c_str(), name.c_str(), ACES_LOOK_NAME);
                    addInactiveCS(name.c_str());
                    m_amfConfig->setActiveDisplays(dispName.c_str());
                    m_amfConfig->setActiveViews(viewName.c_str());
                    m_amfConfig->addColorSpace(cs);
                    m_amfInfoObject->inputColorSpaceName = m_amfConfig->getColorSpace(cs->getName())->getName();
                }
            }
        }
    }

    if (m_input.empty())
    {
        // No input transform: the clip is already in ACES2065-1. Fetch the ACES
        // color space directly by name (it cannot be found by amf_transform_ids).
        ConstColorSpaceRcPtr cs = m_refConfig->getColorSpace(ACES);
        if (cs != NULL)
        {
            m_amfConfig->addColorSpace(cs);
            m_amfInfoObject->inputColorSpaceName = m_amfConfig->getColorSpace(cs->getName())->getName();

            auto it = CAMERA_MAPPING.find(cs->getName());
            if (it != CAMERA_MAPPING.end())
            {
                ConstColorSpaceRcPtr lin_cs = m_refConfig->getColorSpace(it->second.c_str());
                m_amfConfig->addColorSpace(lin_cs);
            }
        }
    }
    else if ((NULL == m_amfInfoObject->inputColorSpaceName) || (0 == strlen(m_amfInfoObject->inputColorSpaceName)))
    {
        throwMessage("Input transform not found.");
    }
}

void AMFParser::Impl::processOutputTransform()
{
    //handle missing outputTransform
    if (m_output.empty())
    {
        m_amfConfig->addDisplayView("None", "Raw", "Raw", NULL);
        /* A config with a display color space must have a view transform.
        Either need to remove 'CIE-XYZ-D65' or add a view transform. */
        m_amfConfig->addViewTransform(m_refConfig->getViewTransform("Un-tone-mapped"));
        return;
    }

    for (auto& elem : m_output.m_tldElements)
    {
        if (0 == Platform::Strcasecmp(elem.first.c_str(), AMF_TAG_TRANSFORMID))
        {
            processOutputTransformId(elem.second.c_str(), TRANSFORM_DIR_FORWARD);
            return;
        }
        else if (0 == Platform::Strcasecmp(elem.first.c_str(), AMF_TAG_FILE))
        {
            FileTransformRcPtr ft = FileTransform::Create();
            checkLutPath(elem.second);
            ft->setSrc(elem.second.c_str());
            ft->setCCCId("");
            ft->setInterpolation(INTERP_BEST);
            ft->setDirection(TRANSFORM_DIR_FORWARD);

            std::string name = "AMF Output Transform LUT -- " + m_clipName;
            std::string viewName = name;
            std::string dispName;
            getFileDescription(m_output, dispName);
            std::string family = "AMF/" + m_clipName;
            ColorSpaceRcPtr cs = ColorSpace::Create();
            cs->setName(name.c_str());
            cs->setFamily(family.c_str());
            cs->addCategory("file-io");
            cs->setTransform(ft, COLORSPACE_DIR_FROM_REFERENCE);

            m_amfConfig->addDisplayView(dispName.c_str(), viewName.c_str(), name.c_str(), ACES_LOOK_NAME);
            addInactiveCS(name.c_str());
            m_amfConfig->setActiveDisplays(dispName.c_str());
            m_amfConfig->setActiveViews(viewName.c_str());

            m_amfConfig->addColorSpace(cs);
            return;
        }
    }

    auto it = m_output.m_subElements.begin();
    while (it != m_output.m_subElements.end())
    {
        if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_ODT))
        {
            for (++it; it != m_output.m_subElements.end(); ++it)
            {
                if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_TRANSFORMID))
                {
                    processOutputTransformId(it->second.c_str(), TRANSFORM_DIR_FORWARD);
                }
                else if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_FILE))
                {
                    FileTransformRcPtr odtFt = FileTransform::Create();
                    checkLutPath(it->second);
                    odtFt->setSrc(it->second.c_str());
                    odtFt->setCCCId("");
                    odtFt->setInterpolation(INTERP_BEST);
                    odtFt->setDirection(TRANSFORM_DIR_FORWARD);

                    FileTransformRcPtr rrtFt = FileTransform::Create();
                    for (auto itRrt = m_output.m_subElements.begin(); itRrt != m_output.m_subElements.end(); itRrt++)
                    {
                        if (0 == Platform::Strcasecmp(itRrt->first.c_str(), AMF_TAG_RRT))
                        {
                            ++itRrt;
                            while (itRrt != m_output.m_subElements.end() && Platform::Strcasecmp(itRrt->first.c_str(), AMF_TAG_RRT))
                            {
                                if (0 == Platform::Strcasecmp(itRrt->first.c_str(), AMF_TAG_FILE))
                                {
                                    checkLutPath(itRrt->second);
                                    odtFt->setSrc(itRrt->second.c_str());
                                    odtFt->setCCCId("");
                                    odtFt->setInterpolation(INTERP_BEST);
                                    odtFt->setDirection(TRANSFORM_DIR_FORWARD);
                                    break;
                                }
                            }
                        }
                    }

                    std::string name = "AMF Output Transform LUT -- " + m_clipName;
                    std::string viewName = name;
                    std::string dispName;
                    getFileDescription(m_output, dispName);
                    std::string family = "AMF/" + m_clipName;
                    ColorSpaceRcPtr cs = ColorSpace::Create();
                    cs->setName(name.c_str());
                    cs->setFamily(family.c_str());
                    cs->addCategory("file-io");

                    GroupTransformRcPtr gt = GroupTransform::Create();
                    if (rrtFt)
                        gt->appendTransform(rrtFt);
                    gt->appendTransform(odtFt);
                    cs->setTransform(gt, COLORSPACE_DIR_FROM_REFERENCE);
                    m_amfConfig->addDisplayView(dispName.c_str(), viewName.c_str(), name.c_str(), ACES_LOOK_NAME);
                    addInactiveCS(name.c_str());
                    m_amfConfig->setActiveDisplays(dispName.c_str());
                    m_amfConfig->setActiveViews(viewName.c_str());
                    m_amfConfig->addColorSpace(cs);
                }
            }
        }
    }
}

void AMFParser::Impl::processLookTransforms()
{
    m_amfInfoObject->numLooksApplied = 0;
    auto index = 1;
    for (auto it = m_look.begin(); it != m_look.end(); it++)
    {
        if (processLookTransform(**it, index, (m_numLooksBeforeWorkingLocation < 0 ?
                                                AMF_NO_WORKING_LOCATION :
                                                (index <= m_numLooksBeforeWorkingLocation ? AMF_PRE_WORKING_LOCATION : AMF_POST_WORKING_LOCATION))))
            m_amfInfoObject->numLooksApplied++;
        index++;
    }

    //Add a NamedTransform that combines all unapplied individual looks, for use in views.
    GroupTransformRcPtr gt_unapplied = GroupTransform::Create();
    auto numLooks = m_amfConfig->getNumLooks();
    for (auto index = 0; index < numLooks; index++)
    {
        std::string lookName = m_amfConfig->getLookNameByIndex(index);
        if ((lookName.find("Applied)") != std::string::npos) || (0 == Platform::Strcasecmp(lookName.c_str(), ACES_LOOK_NAME)))
        {
        }
        else
        {
            LookTransformRcPtr lkt = LookTransform::Create();
            lkt->setSrc(ACES);
            lkt->setDst(ACES);
            lkt->setLooks(lookName.c_str());
            lkt->setSkipColorSpaceConversion(false);
            lkt->setDirection(TRANSFORM_DIR_FORWARD);

            gt_unapplied->appendTransform(lkt);
        }
    }
    if (gt_unapplied->getNumTransforms() > 0)
    {
        std::string name = "AMF Unapplied Look Transforms -- " + m_clipName;
        std::string family = "AMF/" + m_clipName;
        NamedTransformRcPtr nt = NamedTransform::Create();
        nt->setName(name.c_str());
        nt->clearAliases();
        nt->setFamily(family.c_str());
        nt->setDescription("");
        nt->setTransform(gt_unapplied, TRANSFORM_DIR_FORWARD);
        nt->clearCategories();
        m_amfConfig->addNamedTransform(nt);

        m_amfConfig->addEnvironmentVar(CONTEXT_NAME, name.c_str());
    }
}

void AMFParser::Impl::processClipId()
{
    for (auto& elem : m_clipId.m_subElements)
    {
        if (0 == Platform::Strcasecmp(elem.first.c_str(), AMF_TAG_CLIPNAME))
        {
            m_clipName = elem.second.c_str();
            break;
        }
        if (0 == Platform::Strcasecmp(elem.first.c_str(), AMF_TAG_UUID))
        {
            m_clipName = elem.second.c_str();
            break;
        }
    }

    if (m_clipName.empty())
    {
        std::string name = m_xmlFilePath.substr(m_xmlFilePath.find_last_of("/") + 1);
        m_clipName = name.substr(0, name.find_last_of("."));
    }
}

bool AMFParser::Impl::amfReferencesAces2Transforms() const
{
    auto scan = [](const std::vector<std::pair<std::string, std::string>> & elems) -> bool
    {
        for (const auto & elem : elems)
        {
            if (0 == Platform::Strcasecmp(elem.first.c_str(), AMF_TAG_TRANSFORMID) &&
                elem.second.find(":v2.0:") != std::string::npos)
            {
                return true;
            }
        }
        return false;
    };

    if (scan(m_input.m_tldElements) || scan(m_input.m_subElements) ||
        scan(m_output.m_tldElements) || scan(m_output.m_subElements))
    {
        return true;
    }
    for (const auto & look : m_look)
    {
        if (scan(look->m_subElements))
            return true;
    }
    return false;
}

std::string AMFParser::Impl::refRoleColorSpace(const char* role, const char* fallback) const
{
    if (m_refConfig->hasRole(role))
    {
        const char* name = m_refConfig->getRoleColorSpace(role);
        if (name && *name)
            return name;
    }
    return fallback;
}

void AMFParser::Impl::clearCopiedInteropIds()
{
    // AMFInfo string members point into color space name storage owned by the
    // config. Capture the current names by value because replacing a color
    // space below invalidates those pointers.
    const std::string inputName =
        m_amfInfoObject->inputColorSpaceName ? m_amfInfoObject->inputColorSpaceName : "";
    const std::string clipName =
        m_amfInfoObject->clipColorSpaceName ? m_amfInfoObject->clipColorSpaceName : "";

    std::vector<std::string> names;
    const int numColorSpaces =
        m_amfConfig->getNumColorSpaces(SEARCH_REFERENCE_SPACE_ALL, COLORSPACE_ALL);
    for (int i = 0; i < numColorSpaces; ++i)
    {
        names.push_back(
            m_amfConfig->getColorSpaceNameByIndex(SEARCH_REFERENCE_SPACE_ALL, COLORSPACE_ALL, i));
    }

    for (const auto & name : names)
    {
        ConstColorSpaceRcPtr cs = m_amfConfig->getColorSpace(name.c_str());
        if (cs && cs->getInteropID() && *cs->getInteropID())
        {
            ColorSpaceRcPtr editable = cs->createEditableCopy();
            editable->setInteropID("");
            m_amfConfig->addColorSpace(editable);
        }
    }

    // Re-point AMFInfo at the (possibly replaced) color space name storage.
    if (!inputName.empty())
    {
        ConstColorSpaceRcPtr cs = m_amfConfig->getColorSpace(inputName.c_str());
        if (cs)
            m_amfInfoObject->inputColorSpaceName = cs->getName();
    }
    if (!clipName.empty())
    {
        ConstColorSpaceRcPtr cs = m_amfConfig->getColorSpace(clipName.c_str());
        if (cs)
            m_amfInfoObject->clipColorSpaceName = cs->getName();
    }
}

void AMFParser::Impl::loadACESRefConfig(const char* configFilePath)
{
    if (configFilePath != NULL)
    {
        m_refConfig = Config::CreateFromFile(configFilePath);
    }
    else
    {
        // Auto-select a builtin reference config based on the ACES version of
        // the transforms referenced by the AMF. ACES 2 output transforms require
        // OCIO 2.4+ builtins, which the running library provides.
        const char* builtinConfig = amfReferencesAces2Transforms()
            ? "studio-config-latest"
            : "studio-config-v2.1.0_aces-v1.3_ocio-v2.3";
        m_refConfig = Config::CreateFromBuiltinConfig(builtinConfig);
    }

    // Configs at profile version 2.5+ carry the "amf_transform_ids" interchange
    // attribute, which allows resolving AMF Transform IDs precisely.  Older
    // configs only have the IDs embedded in the color space descriptions.
    m_useAmfIds = (m_refConfig->getMajorVersion() > 2) ||
                  (m_refConfig->getMajorVersion() == 2 && m_refConfig->getMinorVersion() >= 5);
}

void AMFParser::Impl::initAMFConfig()
{
    m_amfConfig = Config::CreateRaw()->createEditableCopy();
    // Match the reference config version so color spaces copied from it (which
    // may use features newer than 2.3, e.g. ACES 2 builtin transforms) remain
    // valid in the generated config.
    m_amfConfig->setVersion(m_refConfig->getMajorVersion(), m_refConfig->getMinorVersion());

    m_amfConfig->removeDisplayView("sRGB", "Raw");
    m_amfConfig->removeColorSpace("Raw");

    // Resolve the standard color spaces via roles so this works with both the
    // ACES 1.x and ACES 2 reference configs (which name some spaces differently,
    // e.g. the CIE-XYZ-D65 interchange space).
    const std::string sceneLinearName = refRoleColorSpace("scene_linear", "ACEScg");
    const std::string colorTimingName = refRoleColorSpace("color_timing", "ACEScct");
    const std::string cieName         = refRoleColorSpace("cie_xyz_d65_interchange", "CIE-XYZ-D65");
    const std::string rawName         = refRoleColorSpace("data", "Raw");

    ConstColorSpaceRcPtr cs = m_refConfig->getColorSpace(ACES);
    if (!cs)
    {
        throwMessage("Reference config is missing the ACES2065-1 interchange color space.");
        return;
    }
    m_amfConfig->addColorSpace(cs);
    m_amfConfig->addColorSpace(m_refConfig->getColorSpace(sceneLinearName.c_str()));
    m_amfConfig->addColorSpace(m_refConfig->getColorSpace(colorTimingName.c_str()));
    m_amfConfig->addColorSpace(m_refConfig->getColorSpace(cieName.c_str()));
    m_amfConfig->addColorSpace(m_refConfig->getColorSpace(rawName.c_str()));

    m_amfConfig->setInactiveColorSpaces(cieName.c_str());

    m_amfConfig->setRole("scene_linear", sceneLinearName.c_str());
    m_amfConfig->setRole("aces_interchange", ACES);
    m_amfConfig->setRole("cie_xyz_d65_interchange", cieName.c_str());
    m_amfConfig->setRole("color_timing", colorTimingName.c_str());
    m_amfConfig->setRole("compositing_log", colorTimingName.c_str());
    m_amfConfig->setRole("default", NULL);

    FileRulesRcPtr rules = FileRules::Create()->createEditableCopy();
    rules->setDefaultRuleColorSpace(ACES);
    m_amfConfig->setFileRules(rules);

    ColorSpaceTransformRcPtr cst = ColorSpaceTransform::Create();
    cst->setSrc("$SHOT_LOOKS");
    cst->setDst(ACES);
    cst->setDirection(TRANSFORM_DIR_FORWARD);
    cst->setDataBypass(true);
    LookRcPtr look = Look::Create();
    look->setName(ACES_LOOK_NAME);
    look->setProcessSpace(ACES);
    look->setTransform(cst);
    look->setDescription("");
    m_amfConfig->addLook(look);

    m_amfConfig->addEnvironmentVar(CONTEXT_NAME, ACES);

    std::string amfPath = m_xmlFilePath;
    getPath(amfPath);
    m_amfConfig->addSearchPath(amfPath.c_str());
}

void AMFParser::Impl::processOutputTransformId(const char* transformId, TransformDirection tDirection)
{
    ConstColorSpaceRcPtr dcs = searchColorSpaces(transformId);
    ConstViewTransformRcPtr vt = searchViewTransforms(transformId);

    if (dcs && vt)
    {
        m_amfConfig->addColorSpace(dcs);
        m_amfConfig->addViewTransform(vt);

        m_amfConfig->addSharedView(vt->getName(), vt->getName(), "<USE_DISPLAY_NAME>", ACES_LOOK_NAME, "", "");
        int numViews = m_amfConfig->getNumViews(dcs->getName());
        bool bViewExists = false;
        for (int i = 0; i < numViews; i++)
            if (0 == Platform::Strcasecmp(m_amfConfig->getView(dcs->getName(), i), vt->getName()))
                bViewExists = true;
        if (!bViewExists)
            m_amfConfig->addDisplaySharedView(dcs->getName(), vt->getName());

        if (tDirection == TRANSFORM_DIR_INVERSE)
        {
            DisplayViewTransformRcPtr dvt = DisplayViewTransform::Create();
            dvt->setSrc(ACES);
            dvt->setDisplay(dcs->getName());
            dvt->setView(vt->getName());
            dvt->setDirection(tDirection);
            dvt->setLooksBypass(true);

            std::string name = "AMF Input Transform -- " + m_clipName;
            std::string family = "AMF/" + m_clipName;
            ColorSpaceRcPtr cs = ColorSpace::Create();
            cs->setName(name.c_str());
            cs->setTransform(dvt, COLORSPACE_DIR_TO_REFERENCE);
            cs->setFamily(family.c_str());
            cs->addCategory("file-io");

            m_amfConfig->addColorSpace(cs);
            m_amfInfoObject->inputColorSpaceName = m_amfConfig->getColorSpace(cs->getName())->getName();
        }
        else
        {
            m_amfConfig->setActiveDisplays(dcs->getName());
            m_amfConfig->setActiveViews(vt->getName());
        }
    }
}

void AMFParser::Impl::addInactiveCS(const char* csName)
{
    std::string spaces = m_amfConfig->getInactiveColorSpaces();
    spaces += std::string(", ") + std::string(csName);
    m_amfConfig->setInactiveColorSpaces(spaces.c_str());
}

ConstViewTransformRcPtr AMFParser::Impl::searchViewTransforms(std::string acesId)
{
    auto numViewTransforms = m_refConfig->getNumViewTransforms();
    for (auto index = 0; index < numViewTransforms; index++)
    {
        ConstViewTransformRcPtr vt = m_refConfig->getViewTransform(m_refConfig->getViewTransformNameByIndex(index));
        if (ElementMatchesTransformId(vt, acesId, m_useAmfIds))
            return vt;
    }
    return NULL;
}

bool AMFParser::Impl::processLookTransform(AMFTransform& look, int index, std::string workingLocation)
{
    auto wasApplied = !mustApply(look);

    std::string desc;
    getFileDescription(look, desc);

    std::string lookName = "AMF Look " + std::to_string(index);
    if (workingLocation.empty())
    {
        if (wasApplied)
            lookName += " (Applied)";
    }
    else
    {
        if (wasApplied)
            lookName += " (" + workingLocation + " and Applied)";
        else
            lookName += " (" + workingLocation + ")";
    }
    lookName += " -- " + m_clipName;

    for (auto it = look.m_subElements.begin(); it != look.m_subElements.end(); it++)
    {
        if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_TRANSFORMID))
        {
            LookRcPtr lk = searchLookTransforms(it->second.c_str());
            if (lk != NULL)
            {
                lk->setName(lookName.c_str());
                m_amfConfig->addLook(lk);
                return wasApplied;
            }
        }
        else if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_FILE))
        {
            std::string cccid;
            getCCCId(look, cccid);
            if (!cccid.empty())
                desc += " (" + cccid + ")";

            FileTransformRcPtr ft = FileTransform::Create();
            checkLutPath(it->second);
            ft->setSrc(it->second.c_str());
            ft->setCCCId(cccid.c_str());
            ft->setInterpolation(INTERP_BEST);
            ft->setDirection(TRANSFORM_DIR_FORWARD);

            LookRcPtr lk = Look::Create();
            lk->setName(lookName.c_str());
            lk->setProcessSpace(ACES);
            lk->setTransform(ft);
            lk->setDescription(desc.c_str());

            m_amfConfig->addLook(lk);
            return wasApplied;
        }
    }

    bool hasCdl = false;
    std::string slope, offset, power, sat;

    for (auto it = look.m_subElements.begin(); it != look.m_subElements.end();)
    {
        if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_SOPNODE))
        {
            hasCdl = true;
            ++it;
            while (it != look.m_subElements.end())
            {
                if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_SLOPE))
                {
                    slope = it->second.c_str();
                }
                else if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_OFFSET))
                {
                    offset = it->second.c_str();
                }
                else if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_POWER))
                {
                    power = it->second.c_str();
                }
                ++it;
            }
            continue;
        }
        ++it;
    }

    for (auto it = look.m_subElements.begin(); it != look.m_subElements.end();)
    {
        if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_ASCSOP))
        {
            hasCdl = true;
            ++it;
            while (it != look.m_subElements.end())
            {
                if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_SLOPE))
                {
                    slope = it->second.c_str();
                }
                else if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_OFFSET))
                {
                    offset = it->second.c_str();
                }
                else if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_POWER))
                {
                    power = it->second.c_str();
                }
                ++it;
            }
            continue;
        }
        ++it;
    }

    for (auto it = look.m_subElements.begin(); it != look.m_subElements.end();)
    {
        if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_SATNODE))
        {
            hasCdl = true;
            ++it;
            while (it != look.m_subElements.end())
            {
                if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_SAT))
                {
                    sat = it->second.c_str();
                }
                ++it;
            }
            continue;
        }
        ++it;
    }

    for (auto it = look.m_subElements.begin(); it != look.m_subElements.end();)
    {
        if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_ASCSAT))
        {
            hasCdl = true;
            ++it;
            while (it != look.m_subElements.end())
            {
                if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_SAT))
                {
                    sat = it->second.c_str();
                }
                ++it;
            }
            continue;
        }
        ++it;
    }

    if (hasCdl)
    {
        GroupTransformRcPtr gt = GroupTransform::Create();

        CDLTransformRcPtr cdl = CDLTransform::Create();
        double arr[] = { 0.0, 0.0, 0.0 };
        extractThreeFloats(slope, arr);
        cdl->setSlope(arr);
        extractThreeFloats(offset, arr);
        cdl->setOffset(arr);
        extractThreeFloats(power, arr);
        cdl->setPower(arr);
        cdl->setSat(std::stod(sat));

        TransformRcPtr toTransform = NULL;
        TransformRcPtr fromTransform = NULL;
        loadCdlWsTransform(look, true, toTransform);
        loadCdlWsTransform(look, false, fromTransform);

        if (toTransform == NULL && fromTransform == NULL)
        {
            gt->appendTransform(cdl);
        }
        else if (toTransform && fromTransform)
        {
            gt->appendTransform(toTransform);
            gt->appendTransform(cdl);
            gt->appendTransform(fromTransform);
        }
        else if (toTransform)
        {
            gt->appendTransform(toTransform);
            gt->appendTransform(cdl);
            toTransform->setDirection(TRANSFORM_DIR_INVERSE);
            gt->appendTransform(toTransform);
        }
        else if (fromTransform)
        {
            fromTransform->setDirection(TRANSFORM_DIR_INVERSE);
            gt->appendTransform(fromTransform);
            gt->appendTransform(cdl);
            fromTransform->setDirection(TRANSFORM_DIR_FORWARD);
            gt->appendTransform(fromTransform);
        }

        LookRcPtr lk = Look::Create()->createEditableCopy();
        lk->setName(lookName.c_str());
        lk->setProcessSpace(ACES);
        lk->setTransform(gt);
        lk->setDescription("ASC CDL");
        m_amfConfig->addLook(lk);
        return wasApplied;
    }
    return false;
}

void AMFParser::Impl::loadCdlWsTransform(AMFTransform& amft, bool isTo, TransformRcPtr& t)
{
    for (auto it = amft.m_subElements.begin(); it != amft.m_subElements.end();)
    {
        if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_CDLWS))
        {
            ++it;
            while (it != amft.m_subElements.end())
            {
                if (0 == Platform::Strcasecmp(it->first.c_str(), isTo ? AMF_TAG_TOCDLWS : AMF_TAG_FROMCDLWS))
                {
                    while (it != amft.m_subElements.end())
                    {
                        if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_TRANSFORMID))
                        {
                            ConstColorSpaceRcPtr cs = searchColorSpaces(it->second.c_str());
                            m_amfConfig->addColorSpace(cs);

                            ColorSpaceTransformRcPtr cst = ColorSpaceTransform::Create();
                            cst->setDst(isTo ? cs->getName() : ACES);
                            cst->setSrc(isTo ? ACES : cs->getName());
                            cst->setDirection(TRANSFORM_DIR_FORWARD);
                            t = cst;
                            break;
                        }
                        else if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_FILE))
                        {
                            FileTransformRcPtr ft = FileTransform::Create();
                            checkLutPath(it->second);
                            ft->setSrc(it->second.c_str());
                            ft->setCCCId("");
                            ft->setInterpolation(INTERP_BEST);
                            ft->setDirection(TRANSFORM_DIR_FORWARD);
                            t = ft;
                            break;
                        }
                        ++it;
                    }
                }
                ++it;
            }
        }
        if (it != amft.m_subElements.end())
            ++it;
    }
}

void AMFParser::Impl::extractThreeFloats(std::string str, double* arr)
{
    std::istringstream iss(str);
    iss >> arr[0] >> arr[1] >> arr[2];
}

bool AMFParser::Impl::mustApply(AMFTransform& amft)
{
    for (auto it = amft.m_attributes.begin(); it != amft.m_attributes.end(); it++)
    {
        if (0 == Platform::Strcasecmp(it->first.c_str(), "applied"))
        {
            if (0 == Platform::Strcasecmp(it->second.c_str(), "true"))
            {
                return false;
            }
        }
    }
    return true;
}

void AMFParser::Impl::getCCCId(AMFTransform& amft, std::string& cccId)
{
    for (auto it = amft.m_subElements.begin(); it != amft.m_subElements.end(); it++)
    {
        if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_CDLCCR))
        {
            cccId = it->second.c_str();
            return;
        }
    }
}

LookRcPtr AMFParser::Impl::searchLookTransforms(std::string acesId)
{
    auto numLooks = m_refConfig->getNumLooks();
    for (auto index = 0; index < numLooks; index++)
    {
        ConstLookRcPtr lk = m_refConfig->getLook(m_refConfig->getLookNameByIndex(index));
        if (ElementMatchesTransformId(lk, acesId, m_useAmfIds))
            return lk->createEditableCopy();
    }
    return NULL;
}

ConstColorSpaceRcPtr AMFParser::Impl::searchColorSpaces(std::string acesId)
{
    auto numColorSpaces = m_refConfig->getNumColorSpaces(SEARCH_REFERENCE_SPACE_ALL, COLORSPACE_ALL);
    for (auto index = 0; index < numColorSpaces; index++)
    {
        ConstColorSpaceRcPtr cs = m_refConfig->getColorSpace(m_refConfig->getColorSpaceNameByIndex(SEARCH_REFERENCE_SPACE_ALL, COLORSPACE_ALL, index));
        if (ElementMatchesTransformId(cs, acesId, m_useAmfIds))
            return cs;
    }
    return NULL;
}

void AMFParser::Impl::getFileDescription(AMFTransform& amft, std::string& desc)
{
    for (auto it = amft.m_subElements.begin(); it != amft.m_subElements.end(); it++)
    {
        if (0 == Platform::Strcasecmp(it->first.c_str(), AMF_TAG_DESC))
        {
            desc = it->second.c_str();
            return;
        }
    }
}

void AMFParser::Impl::getPath(std::string& path)
{
    path = path.substr(0, path.find_last_of("/")) + "/";
}

void AMFParser::Impl::checkLutPath(std::string& lutPath)
{
    std::ifstream file(lutPath);
    if (file.good())
        return;

    if (lutPath.find("/") == 0)
    {
        throwMessage("File transform refers to path that does not exist: " + lutPath);
    }
    else
    {
        std::string prefix = m_xmlFilePath.substr(0, m_xmlFilePath.find_last_of("/"));
        std::string abs_path = prefix + "/" + lutPath;
        std::ifstream file2(abs_path);
        if (file2.good())
        {
        }
        else
        {
            throwMessage("File transform refers to path that does not exist: " + lutPath);
        }
    }
}

void AMFParser::Impl::determineClipColorSpace()
{
    bool mustApplyInput = mustApply(m_input);
    bool mustApplyOutput = mustApply(m_output);
    if (!m_output.empty() && !mustApplyOutput)
    {
        m_amfInfoObject->clipColorSpaceName = m_amfConfig->getDisplay(0);
        return;
    }
    else if (mustApplyInput)
    {
        m_amfInfoObject->clipColorSpaceName = m_amfInfoObject->inputColorSpaceName;
        return;
    }
    m_amfInfoObject->clipColorSpaceName = ACES;
}

void AMFParser::Impl::handleWorkingLocation()
{
    if (m_numLooksBeforeWorkingLocation < 0)
        return;

    bool outputApplied = false;
    bool outputExists = !m_output.empty();
    if (outputExists)
        outputApplied = !mustApply(m_output);

    GroupTransformRcPtr gt_unapplied = GroupTransform::Create();

    bool workingForward = true;
    if (outputApplied)
        workingForward = false;
    else if (m_amfInfoObject->numLooksApplied < m_numLooksBeforeWorkingLocation)
        workingForward = true;
    else if (m_amfInfoObject->numLooksApplied > m_numLooksBeforeWorkingLocation)
        workingForward = false;
    else if (m_amfInfoObject->numLooksApplied == m_numLooksBeforeWorkingLocation)
        workingForward = true;
    if (workingForward)
    {
        if (mustApply(m_input))
        {
            ColorSpaceTransformRcPtr cst = ColorSpaceTransform::Create();
            cst->setSrc(m_amfInfoObject->inputColorSpaceName);
            cst->setDst(ACES);
            cst->setDirection(TRANSFORM_DIR_FORWARD);
            cst->setDataBypass(true);
            gt_unapplied->appendTransform(cst);
        }

        auto i = 1, numLooks = m_amfConfig->getNumLooks();
        for (auto index = 0; index < numLooks; index++)
        {
            std::string lookName = m_amfConfig->getLookNameByIndex(index);
            if (lookName.find("Applied)") != std::string::npos)
                i++;
            else if (0 == Platform::Strcasecmp(lookName.c_str(), ACES_LOOK_NAME))
                continue;
            else if (i <= m_numLooksBeforeWorkingLocation)
            {
                LookTransformRcPtr lkt = LookTransform::Create();
                lkt->setSrc(ACES);
                lkt->setDst(ACES);
                lkt->setLooks(lookName.c_str());
                lkt->setSkipColorSpaceConversion(false);
                lkt->setDirection(TRANSFORM_DIR_FORWARD);
                gt_unapplied->appendTransform(lkt);
                i++;
            }
            else
                i++;
        }
    }
    else
    {
        if (outputExists && outputApplied)
        {
            DisplayViewTransformRcPtr dvt = DisplayViewTransform::Create();
            dvt->setSrc(ACES);
            dvt->setDisplay(m_amfConfig->getActiveDisplays());
            dvt->setView(m_amfConfig->getActiveViews());
            dvt->setDirection(TRANSFORM_DIR_INVERSE);
            gt_unapplied->appendTransform(dvt);
        }

        auto i = 1, numLooks = m_amfConfig->getNumLooks();
        for (auto index = numLooks - 1; index >= 0; index--)
        {
            std::string lookName = m_amfConfig->getLookNameByIndex(index);
            if (lookName.find("Applied)") != std::string::npos)
            {
                if (i <= m_numLooksBeforeWorkingLocation)
                {
                    LookTransformRcPtr lkt = LookTransform::Create();
                    lkt->setSrc(ACES);
                    lkt->setDst(ACES);
                    lkt->setLooks(lookName.c_str());
                    lkt->setSkipColorSpaceConversion(false);
                    lkt->setDirection(TRANSFORM_DIR_INVERSE);
                    gt_unapplied->appendTransform(lkt);
                }
                i--;
            }
            else if (0 == Platform::Strcasecmp(lookName.c_str(), ACES_LOOK_NAME))
                continue;
            else
                i--;
        }
    }

    if (gt_unapplied->getNumTransforms() == 0)
    {
        MatrixTransformRcPtr mt = MatrixTransform::Create();
        mt->Identity(NULL, NULL);
        gt_unapplied->appendTransform(mt);
    }

    std::string name = "AMF Clip to Working Space Transform -- " + m_clipName;
    std::string family = "AMF/" + m_clipName;
    NamedTransformRcPtr nt = NamedTransform::Create();
    nt->setName(name.c_str());
    nt->clearAliases();
    nt->setFamily(family.c_str());
    nt->setDescription("");
    nt->setTransform(gt_unapplied, TRANSFORM_DIR_FORWARD);
    nt->clearCategories();
    m_amfConfig->addNamedTransform(nt);
}

void AMFParser::Impl::throwMessage(const std::string& error) const
{
    std::ostringstream os;
    os << "Error is: " << error.c_str();
    os << ". At line (" << m_lineNumber << ")";
    throw Exception(os.str().c_str());
}

AMFParser::AMFParser() : m_impl(NULL)
{
}

AMFParser::~AMFParser()
{
    if (m_impl == NULL)
        return;

    delete m_impl;
    m_impl = NULL;
}

ConstConfigRcPtr AMFParser::buildConfig(AMFInfoRcPtr amfInfoObject, const char* amfFilePath, const char* configFilePath)
{
    if (m_impl == NULL)
        m_impl = new Impl();
    return m_impl->parse(amfInfoObject, amfFilePath, configFilePath);
}

OCIOEXPORT ConstConfigRcPtr CreateFromAMF(AMFInfoRcPtr amfInfoObject, const char* amfFilePath, const char* configFilePath)
{
    AMFParser p;
    return p.buildConfig(amfInfoObject, amfFilePath, configFilePath);
}

} // namespace OCIO_NAMESPACE

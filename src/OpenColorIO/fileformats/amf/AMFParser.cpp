// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>
#include <unordered_map>
#include <stack>
#include <regex>
#include <cstdlib>
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
//   * Ad-hoc file/format validation: checkRootElement() (the AMF v2.0 schema
//     gate), checkLutPath(), the file-open check in parse(), extractThreeFloats(),
//     getCCCId(), getFileDescription() -- the library provides schema + semantic
//     validation and typed accessors.
//   * mustApply(), which would read the 'applied' attribute from the library's
//     object model.
//
// The library would NOT replace (OCIO-specific AMF -> Config mapping; stays):
//   * initAMFConfig(), processInputTransform(), processOutputTransform(),
//     processOutputTransformId(), processLookTransforms()/processLookTransform(),
//     loadCdlWsTransform(), handleWorkingLocation(), determineClipColorSpace().
//   * Reference config selection + transform-ID resolution against the OCIO
//     config: loadACESRefConfig(), searchColorSpaces(), resolveDisplayView(),
//     searchLookTransforms(), GetElementTransformIds()/ElementMatchesTransformId(),
//     refRoleColorSpace(), clearCopiedInteropIds().
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
// Like the official library, this reader is AMF v2.0-only (checkRootElement
// rejects amf:v1.0 documents) and resolves transforms against the ACES 2
// reference config. It is, however, looser on required fields (it does not
// enforce that amfInfo/pipelineInfo/clipId carry uuid/dateTime). Adopting the
// library would add that schema + required-field validation.
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

// AMF schema namespace prefix; the trailing "vN.N" is the schema version. Only
// AMF v2.0+ is supported (ACES 1.x / AMF v1.x support was removed for OCIO 2.6).
static constexpr char AMF_ACES_NS_PREFIX[] = "urn:ampas:aces:amf:v";

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

// The list of ACES Transform IDs a config element (color space, view transform
// or look) implements, read from the dedicated "amf_transform_ids" interchange
// attribute. This is the single place the reader gets a transform URN off a
// config element; the reference config must populate this metadata (the ACES 2
// studio builtin config does). ACES 1.x support was removed for OCIO 2.6, so
// there is no legacy description-text fallback: a user who wants a different
// config must populate its amf_transform_ids fields.
template <typename ElementRcPtr>
std::vector<std::string> GetElementTransformIds(const ElementRcPtr & element)
{
    return SplitAmfTransformIds(element->getInterchangeAttribute("amf_transform_ids"));
}

// NOTE(aces-amf): this resolves an AMF Transform ID against the *OCIO config*
// and therefore stays OCIO-specific even if the ACES AMF library is adopted.
// Determine whether a config element corresponds to the supplied ACES Transform
// ID by exact membership in its amf_transform_ids.
template <typename ElementRcPtr>
bool ElementMatchesTransformId(const ElementRcPtr & element, const std::string & acesId)
{
    const std::vector<std::string> ids = GetElementTransformIds(element);
    for (const auto & id : ids)
    {
        if (id == acesId)
            return true;
    }
    return false;
}

} // anonymous namespace

AMFInfoRcPtr AMFInfo::Create()
{
    return AMFInfoRcPtr(new AMFInfo(), &deleter);
}

void AMFInfo::deleter(AMFInfo * c)
{
    delete c;
}

class AMFInfo::Impl
{
public:
    std::string m_clipIdentifier;
    std::string m_clipColorSpaceName;
    std::string m_inputColorSpaceName;
    int m_numLooksApplied = 0;
    std::string m_displayName;
    std::string m_viewName;

    Impl() = default;
    Impl(const Impl &) = delete;
    Impl & operator=(const Impl &) = delete;
    ~Impl() = default;
};

AMFInfo::AMFInfo()
    : m_impl(new AMFInfo::Impl)
{
}

AMFInfo::~AMFInfo()
{
    delete m_impl;
    m_impl = nullptr;
}

const char * AMFInfo::getClipIdentifier() const
{
    return getImpl()->m_clipIdentifier.c_str();
}

void AMFInfo::setClipIdentifier(const char * value)
{
    getImpl()->m_clipIdentifier = value ? value : "";
}

const char * AMFInfo::getClipColorSpaceName() const
{
    return getImpl()->m_clipColorSpaceName.c_str();
}

void AMFInfo::setClipColorSpaceName(const char * value)
{
    getImpl()->m_clipColorSpaceName = value ? value : "";
}

const char * AMFInfo::getInputColorSpaceName() const
{
    return getImpl()->m_inputColorSpaceName.c_str();
}

void AMFInfo::setInputColorSpaceName(const char * value)
{
    getImpl()->m_inputColorSpaceName = value ? value : "";
}

int AMFInfo::getNumLooksApplied() const
{
    return getImpl()->m_numLooksApplied;
}

void AMFInfo::setNumLooksApplied(int value)
{
    getImpl()->m_numLooksApplied = value;
}

const char * AMFInfo::getDisplayName() const
{
    return getImpl()->m_displayName.c_str();
}

void AMFInfo::setDisplayName(const char * value)
{
    getImpl()->m_displayName = value ? value : "";
}

const char * AMFInfo::getViewName() const
{
    return getImpl()->m_viewName.c_str();
}

void AMFInfo::setViewName(const char * value)
{
    getImpl()->m_viewName = value ? value : "";
}

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
        : m_parser(XML_ParserCreate(nullptr))
        , m_lineNumber(0)
        , m_refConfig(nullptr)
        , m_amfConfig(nullptr)
        , m_amfInfoObject(nullptr)
        , m_isInsideInputTransform(false)
        , m_isInsideOutputTransform(false)
        , m_isInsideLookTransform(false)
        , m_isInsideClipId(false)
        , m_isInsidePipeline(false)
        , m_numLooksBeforeWorkingLocation(-1)
        , m_rootChecked(false) {};

    ~Impl()
    {
        reset();
        XML_ParserFree(m_parser);
    }

    ConstConfigRcPtr parse(AMFInfoRcPtr amfInfoObject, const char* amfFilePath, const char* configFilePath = nullptr);

private:
    void reset();

    void parse(const std::string & buffer, bool lastLine);

    static void StartElementHandler(void* userData, const XML_Char* name, const XML_Char** atts);
    // Validate the root aces:acesMetadataFile element declares an AMF v2.0+
    // schema namespace; throw otherwise. Only AMF v2.0+ is supported.
    void checkRootElement(const XML_Char* name, const XML_Char** atts);
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

    void loadACESRefConfig(const char* configFilePath = nullptr);
    void initAMFConfig();

    // Name of the reference config color space assigned to the given role,
    // or the supplied fallback when the role is not defined.
    std::string refRoleColorSpace(const char* role, const char* fallback) const;
    // Clear interop IDs on color spaces copied from the reference config. The
    // generated AMF config is a minimal derived config that does not carry the
    // reference config's full set of interop targets, so leaving the IDs in
    // place would fail config validation.
    void clearCopiedInteropIds();

    // Returns true if the transform ID resolved to a display color space and
    // view transform in the reference config, false otherwise.
    bool processOutputTransformId(const char* transformId, TransformDirection tDirection);
    // A config that exposes a display color space must also declare a view
    // transform, because initAMFConfig adds the display-referred CIE-XYZ-D65
    // interchange space. File-based (LUT) output/input transforms create a
    // display via addDisplayView but do not resolve a view transform, so add
    // the reference config's Un-tone-mapped view transform to keep the config
    // valid (matching the missing-output and transform-ID paths).
    void addDefaultViewTransform();
    void addInactiveCS(const char* csName);
    // Resolve an ACES Output Transform ID to the single (display, view) pair in
    // the reference config: the display whose display color space carries the
    // ID, then the view under that display whose view transform carries the ID.
    // Returns false if no display or no matching view is found.
    bool resolveDisplayView(const char* transformId, std::string& display,
                            std::string& view, std::string& viewTransform);

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

    // Set once the root aces:acesMetadataFile element has been validated to be
    // an AMF v2.0+ document (see StartElementHandler).
    bool m_rootChecked;
};

void AMFParser::Impl::reset()
{
    m_xmlFilePath.clear();
    if (m_xmlStream.is_open())
        m_xmlStream.close();
    m_lineNumber = 0;
    m_refConfig = m_amfConfig = nullptr;
    m_amfInfoObject = nullptr;
    m_clipId.reset();
    m_input.reset();
    m_output.reset();
    for (auto& elem : m_look)
        elem.reset();
    m_isInsideInputTransform = m_isInsideOutputTransform = m_isInsideLookTransform = m_isInsideClipId = m_isInsidePipeline = false;
    m_currentElement.clear();
    m_clipName.clear();
    m_numLooksBeforeWorkingLocation = -1;
    m_rootChecked = false;
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

    // Load the reference config used to resolve transform IDs: the caller's
    // config if supplied, otherwise the ACES 2 studio builtin config.
    loadACESRefConfig(configFilePath);

    initAMFConfig();

    processClipId();
    processInputTransform();
    processLookTransforms();
    processOutputTransform();

    handleWorkingLocation();

    // The transform-ID output path sets these to the resolved (display, view)
    // pair directly. Only fall back to the config's first display/view for
    // paths that don't (e.g. file-based outputs, which set up a display via
    // addDisplayView but don't populate AMFInfo).
    if (0 == std::strlen(m_amfInfoObject->getDisplayName()))
        m_amfInfoObject->setDisplayName(m_amfConfig->getDisplay(0));
    if (0 == std::strlen(m_amfInfoObject->getViewName()))
        m_amfInfoObject->setViewName(m_amfConfig->getView(m_amfInfoObject->getDisplayName(), 0));
    determineClipColorSpace();

    std::regex nonAlnum("[^0-9a-zA-Z_]");
    std::string newName = std::regex_replace(m_clipName, nonAlnum, "");
    std::string roleName = "amf_clip_" + newName;
    m_amfConfig->setRole(roleName.c_str(), m_amfInfoObject->getClipColorSpaceName());

    int numRoles = m_amfConfig->getNumRoles();
    for (int i = 0; i < numRoles; i++)
    {
        if (0 == Platform::Strcasecmp(m_amfConfig->getRoleName(i), roleName.c_str()))
        {
            m_amfInfoObject->setClipIdentifier(m_amfConfig->getRoleName(i));
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
        // The very first element must be the AMF root; validate its schema
        // version (v2.0+) before processing anything else.
        if (!pImpl->m_rootChecked)
            pImpl->checkRootElement(name, atts);

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

void AMFParser::Impl::checkRootElement(const XML_Char* name, const XML_Char** atts)
{
    m_rootChecked = true;

    // Match the root by local name, tolerating any namespace prefix.
    const char* rootLocal = std::strchr(name, ':');
    rootLocal = rootLocal ? rootLocal + 1 : name;
    if (0 != Platform::Strcasecmp(rootLocal, "acesMetadataFile"))
    {
        throwMessage("Not an ACES Metadata File: the root element must be acesMetadataFile.");
    }

    // Find the namespace declaration that binds the AMF schema URI, and the
    // prefix it binds it to. Expat runs in non-namespace mode, so xmlns
    // declarations arrive as ordinary attributes on the root element.
    const size_t prefixLen = std::strlen(AMF_ACES_NS_PREFIX);
    const char* ns = nullptr;
    std::string boundPrefix;
    for (int i = 0; atts && atts[i]; i += 2)
    {
        const std::string attrName(atts[i]);
        const char* attrValue = atts[i + 1];
        if (attrName != "xmlns" && attrName.rfind("xmlns:", 0) != 0)
            continue;
        if (attrValue && 0 == std::strncmp(attrValue, AMF_ACES_NS_PREFIX, prefixLen))
        {
            ns = attrValue;
            boundPrefix = (attrName == "xmlns") ? "" : attrName.substr(std::strlen("xmlns:"));
            break;
        }
    }

    if (!ns)
    {
        throwMessage("Missing ACES namespace; expected an "
                     "'urn:ampas:aces:amf:vN.N' declaration on the root element.");
    }

    // Only AMF v2.0+ is supported. Parse the major version from the "vN.N"
    // suffix of the namespace URN.
    const std::string version = std::string(ns).substr(prefixLen);
    if (std::atoi(version.c_str()) < 2)
    {
        throwMessage("Unsupported AMF schema version '" + version +
                     "'; only AMF v2.0 and later are supported.");
    }

    // The reader matches elements by the 'aces:' prefix, so the ACES namespace
    // must be bound to that prefix. Supporting an arbitrary prefix (or the
    // default namespace) would require namespace-aware XML parsing (future work).
    if (boundPrefix != "aces")
    {
        throwMessage("The ACES namespace must be bound to the 'aces' prefix; "
                     "prefix '" + boundPrefix + "' is not supported.");
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
            if (cs != nullptr)
            {
                m_amfConfig->addColorSpace(cs);
                m_amfInfoObject->setInputColorSpaceName(m_amfConfig->getColorSpace(cs->getName())->getName());

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
            m_amfInfoObject->setInputColorSpaceName(m_amfConfig->getColorSpace(cs->getName())->getName());
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
                    for (auto itRrt = m_input.m_subElements.begin(); itRrt != m_input.m_subElements.end(); ++itRrt)
                    {
                        if (0 == Platform::Strcasecmp(itRrt->first.c_str(), AMF_TAG_RRT))
                        {
                            for (++itRrt; itRrt != m_input.m_subElements.end() &&
                                 0 != Platform::Strcasecmp(itRrt->first.c_str(), AMF_TAG_RRT); ++itRrt)
                            {
                                if (0 == Platform::Strcasecmp(itRrt->first.c_str(), AMF_TAG_FILE))
                                {
                                    checkLutPath(itRrt->second);
                                    rrtFt->setSrc(itRrt->second.c_str());
                                    rrtFt->setCCCId("");
                                    rrtFt->setInterpolation(INTERP_BEST);
                                    rrtFt->setDirection(TRANSFORM_DIR_INVERSE);
                                    break;
                                }
                            }
                            break;
                        }
                    }

                    std::string name = "AMF Input Transform LUT -- " + m_clipName;
                    std::string viewName = name;
                    std::string dispName;
                    getFileDescription(m_input, dispName);
                    if (dispName.empty())
                        dispName = "AMF Input -- " + m_clipName;
                    std::string family = "AMF/" + m_clipName;
                    ColorSpaceRcPtr cs = ColorSpace::Create();
                    cs->setName(name.c_str());
                    cs->setFamily(family.c_str());
                    cs->addCategory("file-io");

                    GroupTransformRcPtr gt = GroupTransform::Create();
                    if (rrtFt->getSrc() && *rrtFt->getSrc())
                        gt->appendTransform(rrtFt);
                    gt->appendTransform(odtFt);
                    cs->setTransform(gt, COLORSPACE_DIR_FROM_REFERENCE);
                    m_amfConfig->addDisplayView(dispName.c_str(), viewName.c_str(), name.c_str(), ACES_LOOK_NAME);
                    addInactiveCS(name.c_str());
                    m_amfConfig->setActiveDisplays(dispName.c_str());
                    m_amfConfig->setActiveViews(viewName.c_str());
                    m_amfConfig->addColorSpace(cs);
                    addDefaultViewTransform();
                    m_amfInfoObject->setInputColorSpaceName(m_amfConfig->getColorSpace(cs->getName())->getName());
                }
            }
        }
    }

    if (m_input.empty())
    {
        // No input transform: the clip is already in ACES2065-1. Fetch the ACES
        // color space directly by name (it cannot be found by amf_transform_ids).
        ConstColorSpaceRcPtr cs = m_refConfig->getColorSpace(ACES);
        if (cs != nullptr)
        {
            m_amfConfig->addColorSpace(cs);
            m_amfInfoObject->setInputColorSpaceName(m_amfConfig->getColorSpace(cs->getName())->getName());

            auto it = CAMERA_MAPPING.find(cs->getName());
            if (it != CAMERA_MAPPING.end())
            {
                ConstColorSpaceRcPtr lin_cs = m_refConfig->getColorSpace(it->second.c_str());
                m_amfConfig->addColorSpace(lin_cs);
            }
        }
    }
    else if (0 == strlen(m_amfInfoObject->getInputColorSpaceName()))
    {
        throwMessage("Input transform not found.");
    }
}

void AMFParser::Impl::processOutputTransform()
{
    //handle missing outputTransform
    if (m_output.empty())
    {
        m_amfConfig->addDisplayView("None", "Raw", "Raw", nullptr);
        addDefaultViewTransform();
        return;
    }

    for (auto& elem : m_output.m_tldElements)
    {
        if (0 == Platform::Strcasecmp(elem.first.c_str(), AMF_TAG_TRANSFORMID))
        {
            if (!processOutputTransformId(elem.second.c_str(), TRANSFORM_DIR_FORWARD))
                throwMessage("Output transform not found: " + elem.second);
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
            if (dispName.empty())
                dispName = "AMF Output -- " + m_clipName;
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
            addDefaultViewTransform();
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
                    if (!processOutputTransformId(it->second.c_str(), TRANSFORM_DIR_FORWARD))
                        throwMessage("Output transform not found: " + it->second);
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
                    for (auto itRrt = m_output.m_subElements.begin(); itRrt != m_output.m_subElements.end(); ++itRrt)
                    {
                        if (0 == Platform::Strcasecmp(itRrt->first.c_str(), AMF_TAG_RRT))
                        {
                            for (++itRrt; itRrt != m_output.m_subElements.end() &&
                                 0 != Platform::Strcasecmp(itRrt->first.c_str(), AMF_TAG_RRT); ++itRrt)
                            {
                                if (0 == Platform::Strcasecmp(itRrt->first.c_str(), AMF_TAG_FILE))
                                {
                                    checkLutPath(itRrt->second);
                                    rrtFt->setSrc(itRrt->second.c_str());
                                    rrtFt->setCCCId("");
                                    rrtFt->setInterpolation(INTERP_BEST);
                                    rrtFt->setDirection(TRANSFORM_DIR_FORWARD);
                                    break;
                                }
                            }
                            break;
                        }
                    }

                    std::string name = "AMF Output Transform LUT -- " + m_clipName;
                    std::string viewName = name;
                    std::string dispName;
                    getFileDescription(m_output, dispName);
                    if (dispName.empty())
                        dispName = "AMF Output -- " + m_clipName;
                    std::string family = "AMF/" + m_clipName;
                    ColorSpaceRcPtr cs = ColorSpace::Create();
                    cs->setName(name.c_str());
                    cs->setFamily(family.c_str());
                    cs->addCategory("file-io");

                    GroupTransformRcPtr gt = GroupTransform::Create();
                    if (rrtFt->getSrc() && *rrtFt->getSrc())
                        gt->appendTransform(rrtFt);
                    gt->appendTransform(odtFt);
                    cs->setTransform(gt, COLORSPACE_DIR_FROM_REFERENCE);
                    m_amfConfig->addDisplayView(dispName.c_str(), viewName.c_str(), name.c_str(), ACES_LOOK_NAME);
                    addInactiveCS(name.c_str());
                    m_amfConfig->setActiveDisplays(dispName.c_str());
                    m_amfConfig->setActiveViews(viewName.c_str());
                    m_amfConfig->addColorSpace(cs);
                    addDefaultViewTransform();
                }
            }
        }
    }
}

void AMFParser::Impl::processLookTransforms()
{
    m_amfInfoObject->setNumLooksApplied(0);
    auto index = 1;
    for (auto it = m_look.begin(); it != m_look.end(); it++)
    {
        if (processLookTransform(**it, index, (m_numLooksBeforeWorkingLocation < 0 ?
                                                AMF_NO_WORKING_LOCATION :
                                                (index <= m_numLooksBeforeWorkingLocation ? AMF_PRE_WORKING_LOCATION : AMF_POST_WORKING_LOCATION))))
            m_amfInfoObject->setNumLooksApplied(m_amfInfoObject->getNumLooksApplied() + 1);
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
}

void AMFParser::Impl::loadACESRefConfig(const char* configFilePath)
{
    if (configFilePath != nullptr)
    {
        m_refConfig = Config::CreateFromFile(configFilePath);
    }
    else
    {
        // OCIO 2.6 targets ACES 2 by default. Always use the latest ACES 2
        // studio builtin config, which carries the "amf_transform_ids"
        // interchange metadata the reader resolves transform IDs against. An
        // ACES 1.x pipeline must supply its own config with that metadata
        // populated (ACES 1.x is no longer supported via a builtin config).
        m_refConfig = Config::CreateFromBuiltinConfig("studio-config-latest");
    }
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
    m_amfConfig->setRole("default", nullptr);

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

bool AMFParser::Impl::processOutputTransformId(const char* transformId, TransformDirection tDirection)
{
    std::string display, view, viewTransform;
    if (!resolveDisplayView(transformId, display, view, viewTransform))
        return false;

    ConstColorSpaceRcPtr dcs = m_refConfig->getColorSpace(display.c_str());
    ConstViewTransformRcPtr vt = m_refConfig->getViewTransform(viewTransform.c_str());

    m_amfConfig->addColorSpace(dcs);
    m_amfConfig->addViewTransform(vt);

    // Reproduce the reference config's (display, view) pairing in the built
    // config, using the real view name resolved from the reference config.
    m_amfConfig->addSharedView(view.c_str(), viewTransform.c_str(), "<USE_DISPLAY_NAME>", ACES_LOOK_NAME, "", "");
    int numViews = m_amfConfig->getNumViews(display.c_str());
    bool bViewExists = false;
    for (int i = 0; i < numViews; i++)
        if (0 == Platform::Strcasecmp(m_amfConfig->getView(display.c_str(), i), view.c_str()))
            bViewExists = true;
    if (!bViewExists)
        m_amfConfig->addDisplaySharedView(display.c_str(), view.c_str());

    if (tDirection == TRANSFORM_DIR_INVERSE)
    {
        DisplayViewTransformRcPtr dvt = DisplayViewTransform::Create();
        dvt->setSrc(ACES);
        dvt->setDisplay(display.c_str());
        dvt->setView(view.c_str());
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
        m_amfInfoObject->setInputColorSpaceName(m_amfConfig->getColorSpace(cs->getName())->getName());
    }
    else
    {
        m_amfConfig->setActiveDisplays(display.c_str());
        m_amfConfig->setActiveViews(view.c_str());
        m_amfInfoObject->setDisplayName(display.c_str());
        m_amfInfoObject->setViewName(view.c_str());
    }
    return true;
}

void AMFParser::Impl::addDefaultViewTransform()
{
    // WORKAROUND: this exists only to satisfy config validation. initAMFConfig
    // injects the display-referred CIE-XYZ-D65 interchange space, and OCIO
    // requires a config with any display-referred color space to declare at
    // least one view transform. A file-based (LUT) output has no resolvable
    // view transform, so we add the reference config's "Un-tone-mapped" view
    // transform even though it is unrelated to the AMF's actual pipeline.
    // The file-based ODT workflow needs further validation (whether a
    // colorspace-view-only config is the right model, or whether CIE-XYZ-D65
    // should be injected at all) -- kept as-is for now; revisit separately.
    m_amfConfig->addViewTransform(m_refConfig->getViewTransform("Un-tone-mapped"));
}

void AMFParser::Impl::addInactiveCS(const char* csName)
{
    std::string spaces = m_amfConfig->getInactiveColorSpaces();
    spaces += std::string(", ") + std::string(csName);
    m_amfConfig->setInactiveColorSpaces(spaces.c_str());
}

bool AMFParser::Impl::resolveDisplayView(const char* transformId, std::string& display,
                                         std::string& view, std::string& viewTransform)
{
    const std::string urn(transformId);

    // Find the (display, view) pair carrying the transform ID: a display whose
    // display color space lists the ID, and a view under it whose view
    // transform lists the ID. If a matching display has no matching view, keep
    // scanning the remaining displays so the pair is found wherever it lives in
    // the config, rather than committing to the first display.
    const int numDisplays = m_refConfig->getNumDisplays();
    for (int d = 0; d < numDisplays; d++)
    {
        const char* displayName = m_refConfig->getDisplay(d);
        ConstColorSpaceRcPtr dcs = m_refConfig->getColorSpace(displayName);
        if (!dcs || !ElementMatchesTransformId(dcs, urn))
            continue;

        const int numViews = m_refConfig->getNumViews(displayName);
        for (int v = 0; v < numViews; v++)
        {
            const char* viewName = m_refConfig->getView(displayName, v);
            const char* vtName = m_refConfig->getDisplayViewTransformName(displayName, viewName);
            if (!vtName || !*vtName)
                continue;
            ConstViewTransformRcPtr vt = m_refConfig->getViewTransform(vtName);
            if (vt && ElementMatchesTransformId(vt, urn))
            {
                display = displayName;
                view = viewName;
                viewTransform = vtName;
                return true;
            }
        }
    }
    return false;
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
            // NOTE: the sub-elements are flattened, so this transformId is not
            // necessarily the look's own transform -- a CDL look carries its
            // cdlWorkingSpace CSC transformId here too. Only consume it when it
            // resolves to an actual look; otherwise fall through to the CDL /
            // file handling below (do not hard-fail on a non-look id).
            LookRcPtr lk = searchLookTransforms(it->second.c_str());
            if (lk != nullptr)
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
        if (!sat.empty())
        {
            double satValue = 1.0;
            std::istringstream(sat) >> satValue;
            cdl->setSat(satValue);
        }

        TransformRcPtr toTransform = nullptr;
        TransformRcPtr fromTransform = nullptr;
        loadCdlWsTransform(look, true, toTransform);
        loadCdlWsTransform(look, false, fromTransform);

        if (toTransform == nullptr && fromTransform == nullptr)
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
                            if (cs == nullptr)
                                throwMessage("CDL working space transform ID not found: " + it->second);
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
    if (iss.fail())
        throwMessage("CDL SOP node requires three numeric values but got: '" + str + "'");
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
        if (ElementMatchesTransformId(lk, acesId))
            return lk->createEditableCopy();
    }
    return nullptr;
}

ConstColorSpaceRcPtr AMFParser::Impl::searchColorSpaces(std::string acesId)
{
    auto numColorSpaces = m_refConfig->getNumColorSpaces(SEARCH_REFERENCE_SPACE_ALL, COLORSPACE_ALL);
    for (auto index = 0; index < numColorSpaces; index++)
    {
        ConstColorSpaceRcPtr cs = m_refConfig->getColorSpace(m_refConfig->getColorSpaceNameByIndex(SEARCH_REFERENCE_SPACE_ALL, COLORSPACE_ALL, index));
        if (ElementMatchesTransformId(cs, acesId))
            return cs;
    }
    return nullptr;
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
        m_amfInfoObject->setClipColorSpaceName(m_amfConfig->getDisplay(0));
        return;
    }
    else if (mustApplyInput)
    {
        m_amfInfoObject->setClipColorSpaceName(m_amfInfoObject->getInputColorSpaceName());
        return;
    }
    m_amfInfoObject->setClipColorSpaceName(ACES);
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
    else if (m_amfInfoObject->getNumLooksApplied() < m_numLooksBeforeWorkingLocation)
        workingForward = true;
    else if (m_amfInfoObject->getNumLooksApplied() > m_numLooksBeforeWorkingLocation)
        workingForward = false;
    else if (m_amfInfoObject->getNumLooksApplied() == m_numLooksBeforeWorkingLocation)
        workingForward = true;
    if (workingForward)
    {
        if (mustApply(m_input))
        {
            ColorSpaceTransformRcPtr cst = ColorSpaceTransform::Create();
            cst->setSrc(m_amfInfoObject->getInputColorSpaceName());
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
        mt->Identity(nullptr, nullptr);
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

AMFParser::AMFParser() : m_impl(nullptr)
{
}

AMFParser::~AMFParser()
{
    if (m_impl == nullptr)
        return;

    delete m_impl;
    m_impl = nullptr;
}

ConstConfigRcPtr AMFParser::buildConfig(AMFInfoRcPtr amfInfoObject, const char* amfFilePath, const char* configFilePath)
{
    if (m_impl == nullptr)
        m_impl = new Impl();
    return m_impl->parse(amfInfoObject, amfFilePath, configFilePath);
}

OCIOEXPORT ConstConfigRcPtr CreateFromAMF(AMFInfoRcPtr amfInfoObject, const char* amfFilePath, const char* configFilePath)
{
    AMFParser p;
    return p.buildConfig(amfInfoObject, amfFilePath, configFilePath);
}

} // namespace OCIO_NAMESPACE

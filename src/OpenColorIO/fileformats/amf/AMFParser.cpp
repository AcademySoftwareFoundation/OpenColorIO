// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>
#include <unordered_map>

#include <OpenColorIO/FileRules.h>

#include "fileformats/amf/AMFParser.h"
#include "expat.h"

namespace OCIO_NAMESPACE
{

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

static constexpr char AMF_TAG_TRANSFORMID[] = "aces:transformId";
static constexpr char AMF_TAG_FILE[] = "aces:file";
static constexpr char AMF_TAG_CDLCCR[] = "cdl:ColorCorrectionRef";

static constexpr char AMF_TAG_ODT[] = "aces:outputDeviceTransform";
static constexpr char AMF_TAG_RRT[] = "aces:referenceRenderingTransform";

static constexpr char AMF_TAG_CDLWS[] = "aces:cdlWorkingSpace";
static constexpr char AMF_TAG_TOCDLWS[] = "aces:toCdlWorkingSpace";
static constexpr char AMF_TAG_FROMCDLWS[] = "aces:fromCdlWorkingSpace";
static constexpr char AMF_TAG_SOPNODE[] = "cdl:SOPNode";
static constexpr char AMF_TAG_SLOPE[] = "cdl:Slope";
static constexpr char AMF_TAG_OFFSET[] = "cdl:Offset";
static constexpr char AMF_TAG_POWER[] = "cdl:Power";
static constexpr char AMF_TAG_SATNODE[] = "cdl:SatNode";
static constexpr char AMF_TAG_SAT[] = "cdl:Saturation";

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

class AMFParser::Impl
{
private:
    class AMFTransform
    {
    public:
        std::vector<std::pair<std::string, std::string>> m_subElements;
        std::vector<std::pair<std::string, std::string>> m_attributes;

        void addSubElement(const std::string& name, const std::string& value)
        {
            m_subElements.push_back(std::make_pair(name, value));
        }

        void addAttribute(const std::string& name, const std::string& value)
        {
            m_attributes.push_back(std::make_pair(name, value));
        }
    };

public:
    Impl() = delete;
    Impl(const Impl &) = delete;
    Impl & operator=(const Impl &) = delete;

    explicit Impl(AMFInfo& amfInfoObject, const char* amfFilePath)
        : m_parser(XML_ParserCreate(NULL))
        , m_xmlFilePath(amfFilePath)
        , m_xmlStream(m_xmlFilePath)
        , m_lineNumber(0)
        , m_refConfig(NULL)
        , m_amfConfig(NULL)
        , m_amfInfoObject(amfInfoObject)
        , m_isInsideInputTransform(false)
        , m_isInsideOutputTransform(false)
        , m_isInsideLookTransform(false)
        , m_isInsideClipId(false) {};

    ~Impl()
    {
        XML_ParserFree(m_parser);
    }

    void parse();

    const ConstConfigRcPtr& getConfig() const
    {
        return m_amfConfig;
    }

    void writeConfig()
    {
        std::string ocioFilePath = m_xmlFilePath + std::string(".ocio");
        std::ofstream ocioFile(ocioFilePath);
        m_amfConfig->serialize(ocioFile);
    }

private:
    void parse(const std::string & buffer, bool lastLine);

    static void StartElementHandler(void* userData, const XML_Char* name, const XML_Char** atts);
    static bool HandleInputTransformStartElement(AMFParser::Impl* pImpl, const XML_Char* name, const XML_Char** atts);
    static bool HandleOutputTransformStartElement(AMFParser::Impl* pImpl, const XML_Char* name, const XML_Char** atts);
    static bool HandleLookTransformStartElement(AMFParser::Impl* pImpl, const XML_Char* name, const XML_Char** atts);
    static bool HandleClipIdStartElement(AMFParser::Impl* pImpl, const XML_Char* name, const XML_Char** atts);

    static void EndElementHandler(void* userData, const XML_Char* name);
    static bool HandleInputTransformEndElement(AMFParser::Impl* pImpl, const XML_Char* name);
    static bool HandleOutputTransformEndElement(AMFParser::Impl* pImpl, const XML_Char* name);
    static bool HandleLookTransformEndElement(AMFParser::Impl* pImpl, const XML_Char* name);
    static bool HandleClipIdEndElement(AMFParser::Impl* pImpl, const XML_Char* name);

    static void CharacterDataHandler(void* userData, const XML_Char* s, int len);

    static bool IsValidElement(AMFParser::Impl* pImpl, const XML_Char* name);

    void processInputTransforms();
    void processOutputTransforms();
    void processLookTransforms();
    void processClipId();

    void loadACESRefConfig();
    void initAMFConfig();

    void processOutputTransformId(const char* transformId, TransformDirection tDirection);
    void addInactiveCS(const char* csName);
    ConstViewTransformRcPtr searchViewTransforms(std::string acesId);

    void processLookTransform(AMFTransform& look, int index);
    void loadCdlWsTransform(AMFTransform& amft, bool isTo, TransformRcPtr& t);
    void extractThreeFloats(std::string str, double* arr);
    bool mustApply(AMFTransform& amft, bool isLook);
    void getCCCId(AMFTransform& amft, std::string& cccId);
    LookRcPtr searchLookTransforms(std::string acesId);

    ConstColorSpaceRcPtr searchColorSpaces(std::string acesId);
    void getFileDescription(AMFTransform& amft, std::string& desc);
    void checkLutPath(const char* lutPath);

    void throwMessage(const std::string& error) const;

    XML_Parser m_parser;
    std::string m_xmlFilePath;
    std::ifstream m_xmlStream;
    unsigned int m_lineNumber;

    ConstConfigRcPtr m_refConfig;
    ConfigRcPtr m_amfConfig;

    AMFInfo& m_amfInfoObject;
    AMFTransform m_input, m_output, m_clipId;
    std::vector<AMFTransform*> m_look;
    bool m_isInsideInputTransform, m_isInsideOutputTransform, m_isInsideLookTransform, m_isInsideClipId;
    std::string m_currentElement, m_clipName;
};

void AMFParser::Impl::parse()
{
    loadACESRefConfig();

    initAMFConfig();

    XML_SetUserData(m_parser, this);
    XML_SetCharacterDataHandler(m_parser, CharacterDataHandler);
    XML_SetElementHandler(m_parser, StartElementHandler, EndElementHandler);

    std::string line;
    m_lineNumber = 0;
    while (m_xmlStream.good())
    {
        std::getline(m_xmlStream, line);
        line.push_back('\n');
        ++m_lineNumber;

        parse(line, !m_xmlStream.good());
    }

    processClipId();
    processInputTransforms();
    //processOutputTransforms();
    processLookTransforms();
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

void AMFParser::Impl::StartElementHandler(void *userData, const XML_Char *name, const XML_Char **atts)
{
    AMFParser::Impl* pImpl = (AMFParser::Impl*)userData;
    if (IsValidElement(pImpl, name))
    {
        if (HandleInputTransformStartElement(pImpl, name, atts) ||
            HandleOutputTransformStartElement(pImpl, name, atts) ||
            HandleLookTransformStartElement(pImpl, name, atts) ||
            HandleClipIdStartElement(pImpl, name, atts))
        {
        }
    }
}

bool AMFParser::Impl::HandleInputTransformStartElement(AMFParser::Impl* pImpl, const XML_Char *name, const XML_Char** atts)
{
    if ((0 == strcmp(name, AMF_TAG_INPUT_TRANSFORM)))
    {
        pImpl->m_isInsideInputTransform = true;
        for (int i = 0; atts[i]; i += 2)
        {
            const char* attrName = atts[i];
            const char* attrValue = atts[i + 1];
            pImpl->m_input.addAttribute(attrName, attrValue);
        }
        return true;
    }
    else if (pImpl->m_isInsideInputTransform)
    {
        pImpl->m_currentElement = name;
        return true;
    }

    return false;
}

bool AMFParser::Impl::HandleOutputTransformStartElement(AMFParser::Impl* pImpl, const XML_Char *name, const XML_Char** atts)
{
    if ((0 == strcmp(name, AMF_TAG_OUTPUT_TRANSFORM)))
    {
        pImpl->m_isInsideOutputTransform = true;
        for (int i = 0; atts[i]; i += 2)
        {
            const char* attrName = atts[i];
            const char* attrValue = atts[i + 1];
            pImpl->m_output.addAttribute(attrName, attrValue);
        }
        return true;
    }
    else if (pImpl->m_isInsideOutputTransform)
    {
        pImpl->m_currentElement = name;
        return true;
    }

    return false;
}

bool AMFParser::Impl::HandleLookTransformStartElement(AMFParser::Impl* pImpl, const XML_Char *name, const XML_Char** atts)
{
    if ((0 == strcmp(name, AMF_TAG_LOOK_TRANSFORM)))
    {
        pImpl->m_isInsideLookTransform = true;
        AMFTransform* amfTransform = new AMFTransform();
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
        return true;
    }

    return false;
}

bool AMFParser::Impl::HandleClipIdStartElement(AMFParser::Impl* pImpl, const XML_Char* name, const XML_Char** atts)
{
    if ((0 == strcmp(name, AMF_TAG_CLIPID)))
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

void AMFParser::Impl::EndElementHandler(void *userData, const XML_Char *name)
{
    AMFParser::Impl* pImpl = (AMFParser::Impl*)userData;
    if (IsValidElement(pImpl, name))
    {
        if (HandleInputTransformEndElement(pImpl, name) ||
            HandleOutputTransformEndElement(pImpl, name) ||
            HandleLookTransformEndElement(pImpl, name) ||
            HandleClipIdEndElement(pImpl, name))
        {
        }
    }
}

bool AMFParser::Impl::HandleInputTransformEndElement(AMFParser::Impl* pImpl, const XML_Char* name)
{
    if ((0 == strcmp(name, AMF_TAG_INPUT_TRANSFORM)))
    {
        pImpl->m_isInsideInputTransform = false;
        return true;
    }
    else if (pImpl->m_isInsideInputTransform)
    {
        pImpl->m_currentElement.clear();
        return true;
    }

    return false;
}

bool AMFParser::Impl::HandleOutputTransformEndElement(AMFParser::Impl* pImpl, const XML_Char* name)
{
    if ((0 == strcmp(name, AMF_TAG_OUTPUT_TRANSFORM)))
    {
        pImpl->m_isInsideOutputTransform = false;
        return true;
    }
    else if (pImpl->m_isInsideOutputTransform)
    {
        pImpl->m_currentElement.clear();
        return true;
    }

    return false;
}

bool AMFParser::Impl::HandleLookTransformEndElement(AMFParser::Impl* pImpl, const XML_Char* name)
{
    if ((0 == strcmp(name, AMF_TAG_LOOK_TRANSFORM)))
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
    if ((0 == strcmp(name, AMF_TAG_CLIPID)))
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
void AMFParser::Impl::CharacterDataHandler(void *userData, const XML_Char *s, int len)
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
        pImpl->m_input.addSubElement(pImpl->m_currentElement, value);
    else if (pImpl->m_isInsideOutputTransform && !pImpl->m_currentElement.empty())
        pImpl->m_output.addSubElement(pImpl->m_currentElement, value);
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

void AMFParser::Impl::processInputTransforms()
{
    for (auto& elem : m_input.m_subElements)
    {
        if (0 == strcmp(elem.first.c_str(), AMF_TAG_TRANSFORMID))
        {
            ConstColorSpaceRcPtr cs = searchColorSpaces(elem.second.c_str());
            if (cs != NULL)
            {
                m_amfConfig->addColorSpace(cs);

                auto it = CAMERA_MAPPING.find(cs->getName());
                if (it != CAMERA_MAPPING.end())
                {
                    ConstColorSpaceRcPtr lin_cs = m_refConfig->getColorSpace(it->second.c_str());
                    m_amfConfig->addColorSpace(lin_cs);
                }
            }
        }
        else if (0 == strcmp(elem.first.c_str(), AMF_TAG_FILE))
        {
            FileTransformRcPtr ft = FileTransform::Create();
            checkLutPath(elem.second.c_str());
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
        }
    }
    //To Do: InputTransform is an inverse OutputTransform case.
    //cs_name = load_output_transform(config, amf_config, base_path, input_elem, clip_name, is_inverse = True)
}

void AMFParser::Impl::processOutputTransforms()
{
    /*
    * To Do: need to find a way to locate top level elements
    for (auto& elem : m_output.m_subElements)
    {
        if (0 == strcmp(elem.first.c_str(), AMF_TAG_TRANSFORMID))
        {
            processOutputTransformId(elem.second.c_str(), TRANSFORM_DIR_FORWARD);
            return;
        }
        else if (0 == strcmp(elem.first.c_str(), AMF_TAG_FILE))
        {
            FileTransformRcPtr ft = FileTransform::Create();
            checkLutPath(elem.second.c_str());
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
    */

    for (auto it = m_output.m_subElements.begin(); it != m_output.m_subElements.end();)
    {
        if (0 == strcmp(it->first.c_str(), AMF_TAG_ODT))
        {
            ++it;
            while (it != m_output.m_subElements.end() && strcmp(it->first.c_str(), AMF_TAG_ODT))
            {
                if (0 == strcmp(it->first.c_str(), AMF_TAG_TRANSFORMID))
                {
                    processOutputTransformId(it->second.c_str(), TRANSFORM_DIR_FORWARD);
                }
                else if (0 == strcmp(it->first.c_str(), AMF_TAG_FILE))
                {
                    FileTransformRcPtr odtFt = FileTransform::Create();
                    checkLutPath(it->second.c_str());
                    odtFt->setSrc(it->second.c_str());
                    odtFt->setCCCId("");
                    odtFt->setInterpolation(INTERP_BEST);
                    odtFt->setDirection(TRANSFORM_DIR_FORWARD);

                    FileTransformRcPtr rrtFt = FileTransform::Create();
                    for (auto itRrt = m_output.m_subElements.begin(); itRrt != m_output.m_subElements.end(); itRrt++)
                    {
                        if (0 == strcmp(itRrt->first.c_str(), AMF_TAG_RRT))
                        {
                            ++itRrt;
                            while (itRrt != m_output.m_subElements.end() && strcmp(itRrt->first.c_str(), AMF_TAG_RRT))
                            {
                                if (0 == strcmp(itRrt->first.c_str(), AMF_TAG_FILE))
                                {
                                    checkLutPath(itRrt->second.c_str());
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
                ++it;
            }
        }
        ++it;
    }
}

void AMFParser::Impl::processLookTransforms()
{
    auto index = 1;
    for (auto it = m_look.begin(); it != m_look.end(); it++)
    {
        processLookTransform(**it, index);
        index++;
    }

    //Add a NamedTransform that combines all unapplied individual looks, for use in views.
    GroupTransformRcPtr gt_unapplied = GroupTransform::Create();
    auto numLooks = m_amfConfig->getNumLooks();
    for (auto index = 0; index < numLooks; index++)
    {
        std::string lookName = m_amfConfig->getLookNameByIndex(index);
        if ((lookName.find("(Applied)") != std::string::npos) || (0 == strcmp(lookName.c_str(), ACES_LOOK_NAME)))
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
        if (0 == strcmp(elem.first.c_str(), AMF_TAG_CLIPNAME))
        {
            m_clipName = elem.second.c_str();
            return;
        }
        if (0 == strcmp(elem.first.c_str(), AMF_TAG_UUID))
        {
            m_clipName = elem.second.c_str();
        }
    }

    if (m_clipName.empty())
        //To Do: need to add code that will use AMF file name instead
        m_clipName = "AMF Clip Name";
}


void AMFParser::Impl::loadACESRefConfig()
{
    //To Do: need to find a way to remove the hard coding for 2.3 and 2.4 as this is not future proof
    std::string ver = GetVersion();
    if (ver.find("2.3") != std::string::npos || ver.find("2.4") != std::string::npos)
    {
        m_refConfig = Config::CreateFromBuiltinConfig("studio-config-v2.1.0_aces-v1.3_ocio-v2.3");
        return;
    }
    throwMessage("Requires OCIO library version 2.3.0 or higher.");
}

void AMFParser::Impl::initAMFConfig()
{
    m_amfConfig = Config::CreateRaw()->createEditableCopy();
    m_amfConfig->setVersion(2, 3);

    m_amfConfig->removeDisplayView("sRGB", "Raw");
    m_amfConfig->removeColorSpace("Raw");

    ConstColorSpaceRcPtr cs = m_refConfig->getColorSpace(ACES);
    if (!cs)
    {
        throwMessage("Reference config is missing ACES color space.");
        return;
    }
    m_amfConfig->addColorSpace(cs);
    m_amfConfig->addColorSpace(m_refConfig->getColorSpace("ACEScg"));
    m_amfConfig->addColorSpace(m_refConfig->getColorSpace("ACEScct"));
    m_amfConfig->addColorSpace(m_refConfig->getColorSpace("CIE-XYZ-D65"));
    m_amfConfig->addColorSpace(m_refConfig->getColorSpace("Raw"));

    m_amfConfig->setRole("scene_linear", "ACEScg");
    m_amfConfig->setRole("aces_interchange", ACES);
    m_amfConfig->setRole("cie_xyz_d65_interchange", "CIE-XYZ-D65");
    m_amfConfig->setRole("color_timing", "ACEScct");
    m_amfConfig->setRole("compositing_log", "ACEScct");
    //To Do: is NULL a valid value for default role?
    m_amfConfig->setRole("default", NULL);

    FileRulesRcPtr rules = FileRules::Create()->createEditableCopy();
    rules->setDefaultRuleColorSpace(ACES);
    m_amfConfig->setFileRules(rules);

    ColorSpaceTransformRcPtr cst = ColorSpaceTransform::Create();
    cst->setSrc(ACES);
    cst->setDst(CONTEXT_NAME);
    cst->setDirection(TRANSFORM_DIR_FORWARD);
    cst->setDataBypass(true);
    LookRcPtr look = Look::Create();
    look->setName(ACES_LOOK_NAME);
    look->setProcessSpace(ACES);
    look->setTransform(cst);
    //To Do: NULL is not allowed according to header but Py script is setting it to None so how do we achieve the same in C++?
    //look->setInverseTransform(NULL);
    look->setDescription("");
    m_amfConfig->addLook(look);

    m_amfConfig->addEnvironmentVar(CONTEXT_NAME, ACES);

    m_amfConfig->setSearchPath(".");
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
        m_amfConfig->addDisplaySharedView(vt->getName(), vt->getName());

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
        std::string desc = vt->getDescription();
        if (desc.find(acesId) != std::string::npos)
            return vt;
    }
    return NULL;
}

void AMFParser::Impl::processLookTransform(AMFTransform& look, int index)
{
    auto wasApplied = mustApply(look, "Look");

    std::string lookName = "AMF Look " + std::to_string(index);
    if (wasApplied)
        lookName += " (Applied)";
    lookName += " -- " + m_clipName;

    for (auto it = look.m_subElements.begin(); it != look.m_subElements.end(); it++)
    {
        if (0 == strcmp(it->first.c_str(), AMF_TAG_TRANSFORMID))
        {
            LookRcPtr lk = searchLookTransforms(it->second.c_str());
            if (lk != NULL)
            {
                lk->setName(lookName.c_str());
                m_amfConfig->addLook(lk);
                return;
            }
        }
        else if (0 == strcmp(it->first.c_str(), AMF_TAG_FILE))
        {
            std::string desc;
            getFileDescription(look, desc);

            std::string cccid;
            getCCCId(look, cccid);
            if (!cccid.empty())
                desc += " (" + cccid + ")";

            FileTransformRcPtr ft = FileTransform::Create();
            checkLutPath(it->second.c_str());
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
            return;
        }
    }

    bool hasCdl = false;
    std::string slope, offset, power, sat;

    for (auto it = look.m_subElements.begin(); it != look.m_subElements.end();)
    {
        if (0 == strcmp(it->first.c_str(), AMF_TAG_SOPNODE))
        {
            hasCdl = true;
            ++it;
            while (it != look.m_subElements.end())
            {
                if (0 == strcmp(it->first.c_str(), AMF_TAG_SLOPE))
                {
                    slope = it->second.c_str();
                }
                else if (0 == strcmp(it->first.c_str(), AMF_TAG_OFFSET))
                {
                    offset = it->second.c_str();
                }
                else if (0 == strcmp(it->first.c_str(), AMF_TAG_POWER))
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
        if (0 == strcmp(it->first.c_str(), AMF_TAG_SATNODE))
        {
            hasCdl = true;
            ++it;
            while (it != look.m_subElements.end())
            {
                if (0 == strcmp(it->first.c_str(), AMF_TAG_SAT))
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
    }
}

void AMFParser::Impl::loadCdlWsTransform(AMFTransform& amft, bool isTo, TransformRcPtr& t)
{
    for (auto it = amft.m_subElements.begin(); it != amft.m_subElements.end();)
    {
        if (0 == strcmp(it->first.c_str(), AMF_TAG_CDLWS))
        {
            ++it;
            while (it != amft.m_subElements.end())
            {
                if (0 == strcmp(it->first.c_str(), isTo ? AMF_TAG_TOCDLWS : AMF_TAG_FROMCDLWS))
                {
                    while (it != amft.m_subElements.end())
                    {
                        if (0 == strcmp(it->first.c_str(), AMF_TAG_TRANSFORMID))
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
                        else if (0 == strcmp(it->first.c_str(), AMF_TAG_FILE))
                        {
                            FileTransformRcPtr ft = FileTransform::Create();
                            checkLutPath(it->second.c_str());
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

bool AMFParser::Impl::mustApply(AMFTransform& amft, bool isLook)
{
    for (auto it = amft.m_attributes.begin(); it != amft.m_attributes.end(); it++)
    {
        if (0 == strcmp(it->first.c_str(), "applied"))
        {
            if (0 == _strcmpi(it->second.c_str(), "true"))
            {
                if (isLook)
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
        if (0 == strcmp(it->first.c_str(), AMF_TAG_CDLCCR))
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
        std::string desc = lk->getDescription();
        if (desc.find(acesId) != std::string::npos)
            return lk->createEditableCopy();
    }
    return NULL;
}

ConstColorSpaceRcPtr AMFParser::Impl::searchColorSpaces(std::string acesId)
{
    auto numColorSpaces = m_refConfig->getNumColorSpaces();
    for (auto index = 0; index < numColorSpaces; index++)
    {
        ConstColorSpaceRcPtr cs = m_refConfig->getColorSpace(m_refConfig->getColorSpaceNameByIndex(index));
        std::string desc = cs->getDescription();
        if (desc.find(acesId) != std::string::npos)
            return cs;
    }
    return NULL;
}

void AMFParser::Impl::getFileDescription(AMFTransform& amft, std::string& desc)
{
    for (auto it = amft.m_subElements.begin(); it != amft.m_subElements.end(); it++)
    {
        if (0 == strcmp(it->first.c_str(), AMF_TAG_DESC))
        {
            desc = it->second.c_str();
            return;
        }
    }
}

void AMFParser::Impl::checkLutPath(const char* lutPath)
{
    std::ifstream file(lutPath);
    if (file.good())
        return;

    std::string error = std::string("Invalid LUT Path: ") + std::string(lutPath);
    throwMessage(error);
}

void AMFParser::Impl::throwMessage(const std::string& error) const
{
    std::ostringstream os;
    os << "Error is: " << error.c_str();
    os << ". At line (" << m_lineNumber << ")";
    throw Exception(os.str().c_str());
}

ConstConfigRcPtr AMFParser::buildConfig(AMFInfo& amfInfoObject, const char* amfFilePath)
{
    if (!m_impl)
    {
        delete m_impl;
        m_impl = NULL;
    }
    m_impl = new Impl(amfInfoObject, amfFilePath);
    m_impl->parse();
    m_impl->writeConfig();
    return m_impl->getConfig();
}

OCIOEXPORT ConstConfigRcPtr CreateFromAMF(AMFInfo& amfInfoObject, const char* amfFilePath)
{
    AMFParser p;
    return p.buildConfig(amfInfoObject, amfFilePath);
}

/*
* To Do
* 
* need to add following functions:
* determine_clip_colorspace
*/

} // namespace OCIO_NAMESPACE

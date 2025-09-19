// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <unordered_set>

#include <pystring.h>

#include <OpenColorIO/OpenColorIO.h>

#include "Display.h"
#include "FileRules.h"
#include "Logging.h"
#include "MathUtils.h"
#include "OCIOYaml.h"
#include "ops/exposurecontrast/ExposureContrastOpData.h"
#include "ops/gradingprimary/GradingPrimaryOpData.h"
#include "ops/gradingrgbcurve/GradingRGBCurve.h"
#include "ops/gradingtone/GradingToneOpData.h"
#include "ops/log/LogUtils.h"
#include "ParseUtils.h"
#include "PathUtils.h"
#include "Platform.h"
#include "utils/StringUtils.h"
#include "ViewingRules.h"
#include "yaml-cpp/yaml.h"


namespace OCIO_NAMESPACE
{

namespace
{
typedef YAML::const_iterator Iterator;

std::string SanitizeNewlines(const std::string &input)
{
    if (input.empty()) 
        return input;

    // YAML is changing the trailing newlines when reading them:
    // - Written as YAML::Literal (starts with a "|"), descriptions will be read back with a
    //   single newline. One is  added if there was none, only one is kept if there were
    //   several.
    // - Written as YAML::Value string (does not start with "|"), all trailing newlines ('\n')
    //   are preserved.
    // Trailing newlines are inconsistently preserved, lets remove them in all cases.
    std::string str = input;
    auto last = str.back();
    while (last == '\n' && str.length())
    {
        str.pop_back();
        last = str.back();
    }

    // Also, note that a \n is only interpreted as a newline if it is used in a string that is
    // within double quotes.  E.g., "A string \n with embedded \n newlines."  Indeed, without the
    // double quotes the backslash is generally not interpreted as an escape character in YAML.

    return str;
}

// Basic types

inline void load(const YAML::Node& node, bool& x)
{
    try
    {
        x = node.as<bool>();
    }
    catch (const std::exception & e)
    {
        std::ostringstream os;
         os << "At line " << (node.Mark().line + 1)
            << ", '" << node.Tag() << "' parsing boolean failed "
            << "with: " << e.what();
        throw Exception(os.str().c_str());
    }
}

inline void load(const YAML::Node& node, double& x)
{
    try
    {
        x = node.as<double>();
    }
    catch (const std::exception & e)
    {
        std::ostringstream os;
        os << "At line " << (node.Mark().line + 1)
            << ", '" << node.Tag() << "' parsing double failed "
            << "with: " << e.what();
        throw Exception(os.str().c_str());
    }
}

inline void load(const YAML::Node& node, std::string& x)
{
    try
    {
        x = node.as<std::string>();
    }
    catch (const std::exception & e)
    {
        std::ostringstream os;
        os << "At line " << (node.Mark().line + 1)
            << ", '" << node.Tag() << "' parsing string failed "
            << "with: " << e.what();
        throw Exception(os.str().c_str());
    }
}

inline void load(const YAML::Node & node, StringUtils::StringVec & x)
{
    try
    {
        x = node.as<StringUtils::StringVec>();
    }
    catch (const std::exception & e)
    {
        std::ostringstream os;
        os << "At line " << (node.Mark().line + 1)
           << ", '" << node.Tag() << "' parsing StringVec failed "
           << "with: " << e.what();
        throw Exception(os.str().c_str());
    }
}

inline void load(const YAML::Node & node, std::vector<float> & x)
{
    try
    {
        x = node.as<std::vector<float> >();
    }
    catch (const std::exception & e)
    {
        std::ostringstream os;
        os << "At line " << (node.Mark().line + 1)
           << ", '" << node.Tag() << "' parsing vector<float> failed "
           << "with: " << e.what();
        throw Exception(os.str().c_str());
    }
}

inline void load(const YAML::Node& node, std::vector<double>& x)
{
    try
    {
        x = node.as<std::vector<double> >();
    }
    catch (const std::exception & e)
    {
        std::ostringstream os;
        os << "At line " << (node.Mark().line + 1)
           << ", '" << node.Tag() << "' parsing vector<double> failed "
           << "with: " << e.what();
        throw Exception(os.str().c_str());
    }
}

// Enums

inline void load(const YAML::Node& node, BitDepth& depth)
{
    std::string str;
    load(node, str);
    depth = BitDepthFromString(str.c_str());
}

inline void save(YAML::Emitter& out, BitDepth depth)
{
    out << BitDepthToString(depth);
}

inline void load(const YAML::Node& node, Allocation& alloc)
{
    std::string str;
    load(node, str);
    alloc = AllocationFromString(str.c_str());
}

inline void save(YAML::Emitter& out, Allocation alloc)
{
    out << AllocationToString(alloc);
}


inline void load(const YAML::Node& node, TransformDirection& dir)
{
    std::string str;
    load(node, str);
    dir = TransformDirectionFromString(str.c_str());
}

inline void save(YAML::Emitter& out, TransformDirection dir)
{
    out << TransformDirectionToString(dir);
}

inline void load(const YAML::Node& node, Interpolation& interp)
{
    std::string str;
    load(node, str);
    interp = InterpolationFromString(str.c_str());
}

inline void save(YAML::Emitter& out, Interpolation interp)
{
    out << InterpolationToString(interp);
}

inline void loadDescription(const YAML::Node& node, std::string& x)
{
    load(node, x);
    x = SanitizeNewlines(x);
}

inline void saveDescription(YAML::Emitter & out, const char * desc)
{
    if (desc && *desc)
    {
        // Remove trailing newlines so that only one is saved because they won't be read back.
        std::string descStr = SanitizeNewlines(desc);

        out << YAML::Key << "description" << YAML::Value;
        if (descStr.find_first_of('\n') != std::string::npos)
        {
            out << YAML::Literal;
        }
        out << descStr;
    }
}

inline void LogUnknownKeyWarning(const YAML::Node & node,
                                 const YAML::Node & key)
{
    std::string keyName;
    load(key, keyName);

    std::ostringstream os;
    os << "At line " << (key.Mark().line + 1)
        << ", unknown key '" << keyName << "' in '" << node.Tag() << "'.";

    LogWarning(os.str());
}

inline void LogUnknownKeyWarning(const std::string & name,
                                 const YAML::Node & tag)
{
    std::string key;
    load(tag, key);

    std::ostringstream os;
    os << "Unknown key in " << name << ": '" << key << "'.";
    LogWarning(os.str());
}

inline void throwError(const YAML::Node & node,
                       const std::string & msg)
{
    std::ostringstream os;
    os << "At line " << (node.Mark().line + 1)
        << ", '" << node.Tag() << "' parsing failed: " 
        << msg;

    throw Exception(os.str().c_str());
}

inline void throwValueError(const std::string & nodeName,
                            const YAML::Node & key,
                            const std::string & msg)
{
    std::string keyName;
    load(key, keyName);

    std::ostringstream os;
    os << "At line " << (key.Mark().line + 1)
        << ", the value parsing of the key '" << keyName 
        << "' from '" << nodeName << "' failed: " << msg;

    throw Exception(os.str().c_str());
}

inline void throwValueError(const YAML::Node & key,
                            const std::string & msg)
{
    std::string keyName;
    load(key, keyName);

    std::ostringstream os;
    os << "At line " << (key.Mark().line + 1)
        << ", the value parsing of the key '" << keyName 
        << "' failed: " << msg;

    throw Exception(os.str().c_str());
}

// Duplicate Checker

inline void CheckDuplicates(const YAML::Node & node)
{
    std::unordered_set<std::string> keyset;

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (keyset.find(key) == keyset.end())
        {
            keyset.insert(key);
        }
        else
        {
            std::ostringstream os;
            os << "Key-value pair with key '" << key;
            os << "' specified more than once. ";
            throwValueError(node.Tag(), iter->first, os.str());
        }
    }
}

// Custom Key Loader

struct CustomKeysLoader
{
    using Type = std::vector<std::pair<YAML::Node, YAML::Node>>;
    Type m_keyVals;
};

inline void loadCustomKeys(const YAML::Node& node, CustomKeysLoader & ck, const char* sectionName)
{
    if (node.Type() == YAML::NodeType::Map)
    {
        for (Iterator iter = node.begin(); iter != node.end(); ++iter)
        {
            ck.m_keyVals.push_back(std::make_pair(iter->first, iter->second));
        }
    }
    else
    {
        std::ostringstream ss;
        ss << "Expected a YAML map in the " << sectionName << " section.";
        throwError(node, ss.str().c_str());
    }
}

// View

inline void load(const YAML::Node& node, View& v)
{
    if(node.Tag() != "View")
        return;

    CheckDuplicates(node);

    bool expectingSceneCS = false;
    bool expectingDisplayCS = false;

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if(key == "name")
        {
            load(iter->second, v.m_name);
        }
        else if (key == "view_transform")
        {
            expectingDisplayCS = true;
            load(iter->second, v.m_viewTransform);
        }
        else if(key == "colorspace")
        {
            expectingSceneCS = true;
            load(iter->second, v.m_colorspace);
        }
        else if (key == "display_colorspace")
        {
            expectingDisplayCS = true;
            load(iter->second, v.m_colorspace);
        }
        else if(key == "looks" || key == "look")
        {
            load(iter->second, v.m_looks);
        }
        else if (key == "rule")
        {
            load(iter->second, v.m_rule);
        }
        else if (key == "description")
        {
            load(iter->second, v.m_description);
        }
        else
        {
            LogUnknownKeyWarning(node, iter->first);
        }
    }
    if(v.m_name.empty())
    {
        throwError(node, "View does not specify 'name'.");
    }
    if (expectingDisplayCS == expectingSceneCS)
    {
        std::ostringstream os;
        os << "View '" << v.m_name <<
              "' must specify colorspace or view_transform and display_colorspace.";
        throwError(node, os.str().c_str());
    }
    if(v.m_colorspace.empty())
    {
        std::ostringstream os;
        os << "View '" << v.m_name << "' does not specify colorspace.";
        throwError(node, os.str().c_str());
    }
}

inline void save(YAML::Emitter& out, const View & view)
{
    out << YAML::VerbatimTag("View");
    out << YAML::Flow;
    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << view.m_name;
    if (view.m_viewTransform.empty())
    {
        out << YAML::Key << "colorspace" << YAML::Value << view.m_colorspace;
    }
    else
    {
        out << YAML::Key << "view_transform" << YAML::Value << view.m_viewTransform;
        out << YAML::Key << "display_colorspace" << YAML::Value << view.m_colorspace;
    }
    if (!view.m_looks.empty())
    {
        out << YAML::Key << "looks" << YAML::Value << view.m_looks;
    }
    if (!view.m_rule.empty())
    {
        out << YAML::Key << "rule" << YAML::Value << view.m_rule;
    }
    saveDescription(out, view.m_description.c_str());
    out << YAML::EndMap;
}

// Common Transform

inline void EmitBaseTransformKeyValues(YAML::Emitter & out,
                                        const ConstTransformRcPtr & t)
{
    switch (t->getDirection())
    {
    case TRANSFORM_DIR_FORWARD:
        break;
    case TRANSFORM_DIR_INVERSE:
        out << YAML::Key << "direction";
        out << YAML::Value << YAML::Flow;
        save(out, t->getDirection());
        break;
    }
}

inline void EmitTransformName(YAML::Emitter & out,
                              const FormatMetadata & metadata)
{
    const FormatMetadataImpl & data = dynamic_cast<const FormatMetadataImpl &>(metadata);
    const std::string & name = data.getName();
    if (!name.empty())
    {
        out << YAML::Key << "name" << YAML::Value << name;
    }
}

// AllocationTransform

inline void load(const YAML::Node& node, AllocationTransformRcPtr& t)
{
    t = AllocationTransform::Create();

    CheckDuplicates(node);

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if(key == "allocation")
        {
            Allocation val;
            load(iter->second, val);
            t->setAllocation(val);
        }
        else if(key == "vars")
        {
            std::vector<float> val;
            load(iter->second, val);
            if(!val.empty())
            {
                t->setVars(static_cast<int>(val.size()), &val[0]);
            }
        }
        else if(key == "direction")
        {
            TransformDirection val;
            load(iter->second, val);
            t->setDirection(val);
        }
        else
        {
            LogUnknownKeyWarning(node, iter->first);
        }
    }
}

inline void save(YAML::Emitter& out, ConstAllocationTransformRcPtr t)
{
    out << YAML::VerbatimTag("AllocationTransform");
    out << YAML::Flow << YAML::BeginMap;

    out << YAML::Key << "allocation";
    out << YAML::Value << YAML::Flow;
    save(out, t->getAllocation());

    if(t->getNumVars() > 0)
    {
        std::vector<float> vars(t->getNumVars());
        t->getVars(&vars[0]);
        out << YAML::Key << "vars";
        out << YAML::Flow << YAML::Value << vars;
    }

    EmitBaseTransformKeyValues(out, t);
    out << YAML::EndMap;
}

// BuiltinTransform

inline void load(const YAML::Node & node, BuiltinTransformRcPtr & t)
{
    t = BuiltinTransform::Create();

    std::string key;

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if (key == "style")
        {
            std::string transformStyle;
            load(iter->second, transformStyle);
            t->setStyle(transformStyle.c_str());
        }
        else if (key == "direction")
        {
            TransformDirection dir = TRANSFORM_DIR_FORWARD;
            load(iter->second, dir);
            t->setDirection(dir);
        }
        else
        {
            LogUnknownKeyWarning(node, iter->first);
        }
    }
}

inline void save(YAML::Emitter & out, const ConstBuiltinTransformRcPtr & t)
{
    out << YAML::VerbatimTag("BuiltinTransform");
    out << YAML::Flow << YAML::BeginMap;

    out << YAML::Key << "style";
    out << YAML::Value << YAML::Flow << t->getStyle();

    EmitBaseTransformKeyValues(out, t);

    out << YAML::EndMap;
}

// CDLTransform

inline void load(const YAML::Node& node, CDLTransformRcPtr& t)
{
    t = CDLTransform::Create();

    CheckDuplicates(node);

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if (key == "slope")
        {
            std::vector<double> floatvecval;
            load(iter->second, floatvecval);
            if(floatvecval.size() != 3)
            {
                std::ostringstream os;
                os << "'slope' values must be 3 ";
                os << "floats. Found '" << floatvecval.size() << "'.";
                throwValueError(node.Tag(), iter->first, os.str());
            }
            t->setSlope(&floatvecval[0]);
        }
        else if (key == "offset")
        {
            std::vector<double> floatvecval;
            load(iter->second, floatvecval);
            if(floatvecval.size() != 3)
            {
                std::ostringstream os;
                os << "'offset' values must be 3 ";
                os << "floats. Found '" << floatvecval.size() << "'.";
                throwValueError(node.Tag(), iter->first, os.str());
            }
            t->setOffset(&floatvecval[0]);
        }
        else if (key == "power")
        {
            std::vector<double> floatvecval;
            load(iter->second, floatvecval);
            if(floatvecval.size() != 3)
            {
                std::ostringstream os;
                os << "'power' values must be 3 ";
                os << "floats. Found '" << floatvecval.size() << "'.";
                throwValueError(node.Tag(), iter->first, os.str());
            }
            t->setPower(&floatvecval[0]);
        }
        else if (key == "saturation" || key == "sat")
        {
            double val = 0.0f;
            load(iter->second, val);
            t->setSat(val);
        }
        else if (key == "style")
        {
            std::string style;
            load(iter->second, style);
            t->setStyle(CDLStyleFromString(style.c_str()));
        }
        else if (key == "direction")
        {
            TransformDirection val;
            load(iter->second, val);
            t->setDirection(val);
        }
        else if (key == "name")
        {
            std::string name;
            load(iter->second, name);
            t->getFormatMetadata().setName(name.c_str());
        }
        else
        {
            LogUnknownKeyWarning(node, iter->first);
        }
    }
}

inline void save(YAML::Emitter& out, ConstCDLTransformRcPtr t, unsigned int majorVersion)
{
    out << YAML::VerbatimTag("CDLTransform");
    out << YAML::Flow << YAML::BeginMap;

    if (majorVersion >= 2)
    {
        EmitTransformName(out, t->getFormatMetadata());
    }

    std::vector<double> slope(3);
    t->getSlope(&slope[0]);
    if (!IsVecEqualToOne(&slope[0], 3))
    {
        out << YAML::Key << "slope";
        out << YAML::Value << YAML::Flow << slope;
    }

    std::vector<double> offset(3);
    t->getOffset(&offset[0]);
    if (!IsVecEqualToZero(&offset[0], 3))
    {
        out << YAML::Key << "offset";
        out << YAML::Value << YAML::Flow << offset;
    }

    std::vector<double> power(3);
    t->getPower(&power[0]);
    if (!IsVecEqualToOne(&power[0], 3))
    {
        out << YAML::Key << "power";
        out << YAML::Value << YAML::Flow << power;
    }

    if (!IsScalarEqualToOne(t->getSat()))
    {
        out << YAML::Key << "sat" << YAML::Value << t->getSat();
    }

    if (t->getStyle() != CDL_TRANSFORM_DEFAULT)
    {
        out << YAML::Key << "style" << YAML::Value << CDLStyleToString(t->getStyle());
    }

    EmitBaseTransformKeyValues(out, t);
    out << YAML::EndMap;
}

// ColorSpaceTransform

inline void load(const YAML::Node& node, ColorSpaceTransformRcPtr& t)
{
    t = ColorSpaceTransform::Create();

    CheckDuplicates(node);

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if(key == "src")
        {
            std::string stringval;
            load(iter->second, stringval);
            t->setSrc(stringval.c_str());
        }
        else if(key == "dst")
        {
            std::string stringval;
            load(iter->second, stringval);
            t->setDst(stringval.c_str());
        }
        else if(key == "direction")
        {
            TransformDirection val;
            load(iter->second, val);
            t->setDirection(val);
        }
        else if(key == "data_bypass")
        {
            bool val;
            load(iter->second, val);
            t->setDataBypass(val);
        }
        else
        {
            LogUnknownKeyWarning(node, iter->first);
        }
    }
}

inline void save(YAML::Emitter& out, ConstColorSpaceTransformRcPtr t)
{
    out << YAML::VerbatimTag("ColorSpaceTransform");
    out << YAML::Flow << YAML::BeginMap;
    out << YAML::Key << "src" << YAML::Value << t->getSrc();
    out << YAML::Key << "dst" << YAML::Value << t->getDst();
    const bool bypass = t->getDataBypass();
    if (!bypass)
    {
        // NB: Will log a warning if read by a v1 library.
        out << YAML::Key << "data_bypass" << YAML::Value << bypass;
    }

    EmitBaseTransformKeyValues(out, t);
    out << YAML::EndMap;
}

// DisplayViewTransform

inline void load(const YAML::Node& node, DisplayViewTransformRcPtr& t)
{
    t = DisplayViewTransform::Create();

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if (key == "src")
        {
            std::string stringval;
            load(iter->second, stringval);
            t->setSrc(stringval.c_str());
        }
        else if (key == "display")
        {
            std::string stringval;
            load(iter->second, stringval);
            t->setDisplay(stringval.c_str());
        }
        else if (key == "view")
        {
            std::string stringval;
            load(iter->second, stringval);
            t->setView(stringval.c_str());
        }
        else if (key == "direction")
        {
            TransformDirection val;
            load(iter->second, val);
            t->setDirection(val);
        }
        else if (key == "looks_bypass")
        {
            bool boolval{ true };
            load(iter->second, boolval);
            t->setLooksBypass(boolval);
        }
        else if (key == "data_bypass")
        {
            bool boolval{ true };
            load(iter->second, boolval);
            t->setDataBypass(boolval);
        }
        else
        {
            LogUnknownKeyWarning(node, iter->first);
        }
    }
}

inline void save(YAML::Emitter& out, ConstDisplayViewTransformRcPtr t)
{
    out << YAML::VerbatimTag("DisplayViewTransform");
    out << YAML::Flow << YAML::BeginMap;
    out << YAML::Key << "src" << YAML::Value << t->getSrc();
    out << YAML::Key << "display" << YAML::Value << t->getDisplay();
    out << YAML::Key << "view" << YAML::Value << t->getView();
    const bool looksBypass = t->getLooksBypass();
    if (looksBypass)
    {
        out << YAML::Key << "looks_bypass" << YAML::Value << looksBypass;
    }
    const bool dataBypass = t->getDataBypass();
    if (!dataBypass)
    {
        out << YAML::Key << "data_bypass" << YAML::Value << dataBypass;
    }

    EmitBaseTransformKeyValues(out, t);
    out << YAML::EndMap;
}

// ExponentTransform

inline void load(const YAML::Node& node, ExponentTransformRcPtr& t)
{
    t = ExponentTransform::Create();

    CheckDuplicates(node);

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if (key == "value")
        {
            std::vector<double> val;
            if (iter->second.Type() == YAML::NodeType::Sequence)
            {
                load(iter->second, val);
            }
            else
            {
                // If a single value is supplied...
                double singleVal;
                load(iter->second, singleVal);
                val.resize(4, singleVal);
                val[3] = 1.0;
            }
            if (val.size() != 4)
            {
                std::ostringstream os;
                os << "'value' values must be 4 ";
                os << "floats. Found '" << val.size() << "'.";
                throwValueError(node.Tag(), iter->first, os.str());
            }
            const double v[4] = { val[0], val[1], val[2], val[3] };
            t->setValue(v);
        }
        else if (key == "style")
        {
            std::string style;
            load(iter->second, style);
            t->setNegativeStyle(NegativeStyleFromString(style.c_str()));
        }
        else if(key == "direction")
        {
            TransformDirection val;
            load(iter->second, val);
            t->setDirection(val);
        }
        else if(key == "name")
        {
            std::string name;
            load(iter->second, name);
            t->getFormatMetadata().setName(name.c_str());
        }
        else
        {
            LogUnknownKeyWarning(node, iter->first);
        }
    }
}

inline void save(YAML::Emitter& out, ConstExponentTransformRcPtr t, unsigned int majorVersion)
{
    out << YAML::VerbatimTag("ExponentTransform");
    out << YAML::Flow << YAML::BeginMap;

    if (majorVersion >= 2)
    {
        EmitTransformName(out, t->getFormatMetadata());
    }

    double value[4];
    t->getValue(value);
    if (majorVersion >= 2 && value[0] == value[1] && value[0] == value[2] && value[3] == 1.)
    {
        out << YAML::Key << "value" << YAML::Value << value[0];
    }
    else
    {
        std::vector<double> v;
        v.assign(value, value + 4);

        out << YAML::Key << "value";
        out << YAML::Value << YAML::Flow << v;
    }

    auto style = t->getNegativeStyle();
    if (style != NEGATIVE_CLAMP)
    {
        // NB: Will log a warning if read by a v1 library.
        out << YAML::Key << "style";
        out << YAML::Value << YAML::Flow << NegativeStyleToString(style);
    }
    EmitBaseTransformKeyValues(out, t);

    out << YAML::EndMap;
}

// ExponentWithLinear

inline void load(const YAML::Node& node, ExponentWithLinearTransformRcPtr& t)
{
    t = ExponentWithLinearTransform::Create();

    enum FieldFound
    {
        NOTHING_FOUND = 0x00,
        GAMMA_FOUND   = 0x01,
        OFFSET_FOUND  = 0x02,
        FIELDS_FOUND  = (GAMMA_FOUND|OFFSET_FOUND)
    };

    FieldFound fields = NOTHING_FOUND;

    static const std::string err("ExponentWithLinear parse error, ");

    CheckDuplicates(node);

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if (key == "gamma")
        {
            std::vector<double> val;
            if (iter->second.Type() == YAML::NodeType::Sequence)
            {
                load(iter->second, val);
            }
            else
            {
                // If a single value is supplied...
                double singleVal;
                load(iter->second, singleVal);
                val.resize(4, singleVal);
                val[3] = 1.0;
            }
            if (val.size() != 4)
            {
                std::ostringstream os;
                os << err 
                    << "gamma field must be 4 floats. Found '" 
                    << val.size() 
                    << "'.";
                throw Exception(os.str().c_str());
            }
            const double v[4] = { val[0], val[1], val[2], val[3] };
            t->setGamma(v);
            fields = FieldFound(fields|GAMMA_FOUND);
        }
        else if(key == "offset")
        {
            std::vector<double> val;
            if (iter->second.Type() == YAML::NodeType::Sequence)
            {
                load(iter->second, val);
            }
            else
            {
                // If a single value is supplied...
                double singleVal;
                load(iter->second, singleVal);
                val.resize(4, singleVal);
                val[3] = 0.0;
            }
            if (val.size() != 4)
            {
                std::ostringstream os;
                os << err 
                    << "offset field must be 4 floats. Found '" 
                    << val.size() 
                    << "'.";
                throw Exception(os.str().c_str());
            }
            const double v[4] = { val[0], val[1], val[2], val[3] };
            t->setOffset(v);
            fields = FieldFound(fields|OFFSET_FOUND);
        }
        else if (key == "style")
        {
            std::string style;
            load(iter->second, style);
            t->setNegativeStyle(NegativeStyleFromString(style.c_str()));
        }
        else if(key == "direction")
        {
            TransformDirection val;
            load(iter->second, val);
            t->setDirection(val);
        }
        else if(key == "name")
        {
            std::string name;
            load(iter->second, name);
            t->getFormatMetadata().setName(name.c_str());
        }
        else
        {
            LogUnknownKeyWarning(node.Tag(), iter->first);
        }
    }

    if(fields!=FIELDS_FOUND)
    {
        std::string e = err;
        if(fields==NOTHING_FOUND)
        {
            e += "gamma and offset fields are missing";
        }
        else if((fields&GAMMA_FOUND)!=GAMMA_FOUND)
        {
            e += "gamma field is missing";
        }
        else
        {
            e += "offset field is missing";
        }

        throw Exception(e.c_str());
    }
}

inline void save(YAML::Emitter& out, ConstExponentWithLinearTransformRcPtr t)
{
    out << YAML::VerbatimTag("ExponentWithLinearTransform");
    out << YAML::Flow << YAML::BeginMap;

    EmitTransformName(out, t->getFormatMetadata());

    std::vector<double> v;

    double gamma[4];
    t->getGamma(gamma);
    if (gamma[0] == gamma[1] && gamma[0] == gamma[2] && gamma[3] == 1.)
    {
        out << YAML::Key << "gamma" << YAML::Value << gamma[0];
    }
    else
    {
        v.assign(gamma, gamma + 4);

        out << YAML::Key << "gamma";
        out << YAML::Value << YAML::Flow << v;
    }

    double offset[4];
    t->getOffset(offset);
    
    if (offset[0] == offset[1] && offset[0] == offset[2] && offset[3] == 0.)
    {
        out << YAML::Key << "offset" << YAML::Value << offset[0];
    }
    else
    {
        v.assign(offset, offset + 4);

        out << YAML::Key << "offset";
        out << YAML::Value << YAML::Flow << v;
    }

    // Only save style if not default
    auto style = t->getNegativeStyle();
    if (style != NEGATIVE_LINEAR)
    {
        out << YAML::Key << "style";
        out << YAML::Value << YAML::Flow << NegativeStyleToString(style);
    }

    EmitBaseTransformKeyValues(out, t);
    out << YAML::EndMap;
}

// ExposureContrastTransform

inline void load(const YAML::Node& node, ExposureContrastTransformRcPtr& t)
{
    t = ExposureContrastTransform::Create();

    CheckDuplicates(node);

    bool dynExposure = true;
    bool dynContrast = true;
    bool dynGamma    = true;

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if (key == "exposure")
        {
            double param;
            load(iter->second, param);
            t->setExposure(param);
            dynExposure = false;
        }
        else if (key == "contrast")
        {
            double param;
            load(iter->second, param);
            t->setContrast(param);
            dynContrast = false;
        }
        else if (key == "gamma")
        {
            double param;
            load(iter->second, param);
            t->setGamma(param);
            dynGamma = false;
        }
        else if (key == "pivot")
        {
            double param;
            load(iter->second, param);
            t->setPivot(param);
        }
        else if (key == "log_exposure_step")
        {
            double param;
            load(iter->second, param);
            t->setLogExposureStep(param);
        }
        else if (key == "log_midway_gray")
        {
            double param;
            load(iter->second, param);
            t->setLogMidGray(param);
        }
        else if (key == "style")
        {
            std::string style;
            load(iter->second, style);
            t->setStyle(ExposureContrastStyleFromString(style.c_str()));
        }
        else if (key == "direction")
        {
            TransformDirection val;
            load(iter->second, val);
            t->setDirection(val);
        }
        else if (key == "name")
        {
            std::string name;
            load(iter->second, name);
            t->getFormatMetadata().setName(name.c_str());
        }
        else
        {
            LogUnknownKeyWarning(node, iter->first);
        }
    }

    // Missing values are dynamic.
    if (dynExposure)
    {
        t->makeExposureDynamic();
    }
    if (dynContrast)
    {
        t->makeContrastDynamic();
    }
    if (dynGamma)
    {
        t->makeGammaDynamic();
    }
}

inline void save(YAML::Emitter& out, ConstExposureContrastTransformRcPtr t)
{
    out << YAML::VerbatimTag("ExposureContrastTransform");
    out << YAML::Flow << YAML::BeginMap;

    EmitTransformName(out, t->getFormatMetadata());

    out << YAML::Key << "style";
    out << YAML::Value << YAML::Flow << ExposureContrastStyleToString(t->getStyle());

    // Do not save dynamic values.

    if (!t->isExposureDynamic())
    {
        out << YAML::Key << "exposure" << YAML::Value << YAML::Flow << t->getExposure();
    }
    if (!t->isContrastDynamic())
    {
        out << YAML::Key << "contrast" << YAML::Value << YAML::Flow << t->getContrast();
    }
    if (!t->isGammaDynamic())
    {
        out << YAML::Key << "gamma" << YAML::Value << YAML::Flow << t->getGamma();
    }

    out << YAML::Key << "pivot";
    out << YAML::Value << YAML::Flow << t->getPivot();

    const auto les = t->getLogExposureStep();
    if (les != ExposureContrastOpData::LOGEXPOSURESTEP_DEFAULT)
    {
        out << YAML::Key << "log_exposure_step";
        out << YAML::Value << YAML::Flow << les;
    }

    const auto lmg = t->getLogMidGray();
    if (lmg != ExposureContrastOpData::LOGMIDGRAY_DEFAULT)
    {
        out << YAML::Key << "log_midway_gray";
        out << YAML::Value << YAML::Flow << lmg;
    }

    EmitBaseTransformKeyValues(out, t);
    out << YAML::EndMap;
}

// FileTransform

inline void load(const YAML::Node& node, FileTransformRcPtr& t)
{
    t = FileTransform::Create();

    CheckDuplicates(node);

    std::string stringval;

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if (key == "src")
        {
            load(iter->second, stringval);
            t->setSrc(stringval.c_str());
        }
        else if (key == "cccid")
        {
            load(iter->second, stringval);
            t->setCCCId(stringval.c_str());
        }
        else if (key == "cdl_style")
        {
            load(iter->second, stringval);
            t->setCDLStyle(CDLStyleFromString(stringval.c_str()));
        }
        else if (key == "interpolation")
        {
            Interpolation val;
            load(iter->second, val);
            t->setInterpolation(val);
        }
        else if (key == "direction")
        {
            TransformDirection val;
            load(iter->second, val);
            t->setDirection(val);
        }
        else
        {
            LogUnknownKeyWarning(node, iter->first);
        }
    }
}

inline void save(YAML::Emitter& out, ConstFileTransformRcPtr t, unsigned int majorVersion)
{
    out << YAML::VerbatimTag("FileTransform");
    out << YAML::Flow << YAML::BeginMap;
    out << YAML::Key << "src" << YAML::Value << t->getSrc();
    const char * cccid = t->getCCCId();
    if (cccid && *cccid)
    {
        out << YAML::Key << "cccid" << YAML::Value << t->getCCCId();
    }
    if (t->getCDLStyle() != CDL_TRANSFORM_DEFAULT)
    {
        // NB: Will log a warning if read by a v1 library.
        out << YAML::Key << "cdl_style" << YAML::Value << CDLStyleToString(t->getCDLStyle());
    }
    Interpolation interp = t->getInterpolation();
    if (majorVersion == 1 && interp == INTERP_DEFAULT)
    {
        // The DEFAULT method is not available in a v1 library.  If the v1 config is read by a v1
        // library and the file is a LUT, a missing interp would end up set to UNKNOWN and a
        // throw would happen when the processor is built.  Setting to LINEAR to provide more
        // robust compatibility.
        interp = INTERP_LINEAR;
    }
    if (interp != INTERP_DEFAULT)
    {
        out << YAML::Key << "interpolation";
        out << YAML::Value;
        save(out, interp);
    }

    EmitBaseTransformKeyValues(out, t);
    out << YAML::EndMap;
}

// FixedFunctionTransform

inline void load(const YAML::Node& node, FixedFunctionTransformRcPtr& t)
{
    t = FixedFunctionTransform::Create(FIXED_FUNCTION_ACES_RED_MOD_03);

    CheckDuplicates(node);

    bool styleFound{ false };

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if(key == "params")
        {
            std::vector<double> params;
            load(iter->second, params);
            if (!params.empty())
            {
                t->setParams(&params[0], params.size());
            }
        }
        else if(key == "style")
        {
            std::string style;
            load(iter->second, style);
            t->setStyle( FixedFunctionStyleFromString(style.c_str()) );
            styleFound = true;

            const FixedFunctionStyle styleID = t->getStyle();
            if (styleID == FIXED_FUNCTION_ACES_OUTPUT_TRANSFORM_20
                || styleID == FIXED_FUNCTION_ACES_RGB_TO_JMH_20
                || styleID == FIXED_FUNCTION_ACES_TONESCALE_COMPRESS_20
                || styleID == FIXED_FUNCTION_ACES_GAMUT_COMPRESS_20)
            {
                std::ostringstream os;
                os << "FixedFunction style is experimental and may be removed in a future release: '" << style << "'.";
                LogWarning(os.str());
            }
        }
        else if(key == "direction")
        {
            TransformDirection val;
            load(iter->second, val);
            t->setDirection(val);
        }
        else if(key == "name")
        {
            std::string name;
            load(iter->second, name);
            t->getFormatMetadata().setName(name.c_str());
        }
        else
        {
            LogUnknownKeyWarning(node.Tag(), iter->first);
        }
    }
    if (!styleFound)
    {
        throwError(node, "style value is missing.");
    }
}

inline void save(YAML::Emitter& out, ConstFixedFunctionTransformRcPtr t)
{
    out << YAML::VerbatimTag("FixedFunctionTransform");
    out << YAML::Flow << YAML::BeginMap;

    EmitTransformName(out, t->getFormatMetadata());

    out << YAML::Key << "style";
    out << YAML::Value << YAML::Flow << FixedFunctionStyleToString(t->getStyle());

    const FixedFunctionStyle styleID = t->getStyle();
    if (styleID == FIXED_FUNCTION_ACES_OUTPUT_TRANSFORM_20
        || styleID == FIXED_FUNCTION_ACES_RGB_TO_JMH_20
        || styleID == FIXED_FUNCTION_ACES_TONESCALE_COMPRESS_20
        || styleID == FIXED_FUNCTION_ACES_GAMUT_COMPRESS_20)
    {
        std::ostringstream os;
        os << "FixedFunction style is experimental and may be removed in a future release: '" << FixedFunctionStyleToString(t->getStyle()) << "'.";
        LogWarning(os.str());
    }

    const size_t numParams = t->getNumParams();
    if(numParams>0)
    {
        std::vector<double> params(numParams, 0.);
        t->getParams(&params[0]);
        out << YAML::Key << "params";
        out << YAML::Value << YAML::Flow << params;
    }

    EmitBaseTransformKeyValues(out, t);
    out << YAML::EndMap;
}

// GradingPrimaryTransform

inline void load(const YAML::Node & parent, const YAML::Node & node, GradingRGBM & rgbm)
{
    if (node.Type() == YAML::NodeType::Map)
    {
        bool rgbOK{ false };
        bool masterOK{ false };

        for (Iterator iter = node.begin(); iter != node.end(); ++iter)
        {
            const std::string & key = iter->first.as<std::string>();

            if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

            if (key == "rgb")
            {
                std::vector<double> vals;
                load(iter->second, vals);
                if (vals.size() != 3)
                {
                    throwError(iter->first, "The RGB value needs to be a 3 doubles.");
                }
                rgbm.m_red = vals[0];
                rgbm.m_green = vals[1];
                rgbm.m_blue = vals[2];
                rgbOK = true;
            }
            else if (key == "master")
            {
                load(iter->second, rgbm.m_master);
                masterOK = true;
            }
            else
            {
                LogUnknownKeyWarning(parent, iter->first);
            }
        }
        if (!rgbOK || !masterOK)
        {
            throwValueError(parent, "Both rgb and master values are required.");
        }
    }
    else
    {
        throwValueError(parent, "The value needs to be a map.");
    }
}

inline void loadPivot(const YAML::Node & parent, const YAML::Node & node,
                      double & val, bool & valLoaded,
                      double & blackVal, bool & blackValLoaded,
                      double & whiteVal, bool & whiteValLoaded)
{
    if (node.Type() == YAML::NodeType::Map)
    {
        for (Iterator iter = node.begin(); iter != node.end(); ++iter)
        {
            const std::string & key = iter->first.as<std::string>();

            if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

            if (key == "contrast")
            {
                load(iter->second, val);
                valLoaded = true;
            }
            else if (key == "black")
            {
                load(iter->second, blackVal);
                blackValLoaded = true;
            }
            else if (key == "white")
            {
                load(iter->second, whiteVal);
                whiteValLoaded = true;
            }
            else
            {
                LogUnknownKeyWarning(node, iter->first);
            }
        }

        if (!valLoaded && !blackValLoaded && !whiteValLoaded)
        {
            throwValueError(parent, "At least one of the pivot values must be provided.");
        }
    }
    else
    {
        throwValueError(parent, "The value needs to be a map.");
    }
}

inline void loadClamp(const YAML::Node & parent, const YAML::Node & node,
                      double & blackVal, bool & blackValLoaded,
                      double & whiteVal, bool & whiteValLoaded)
{
    if (node.Type() == YAML::NodeType::Map)
    {
        for (Iterator iter = node.begin(); iter != node.end(); ++iter)
        {
            const std::string & key = iter->first.as<std::string>();

            if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

            if (key == "black")
            {
                load(iter->second, blackVal);
                blackValLoaded = true;
            }
            else if (key == "white")
            {
                load(iter->second, whiteVal);
                whiteValLoaded = true;
            }
            else
            {
                LogUnknownKeyWarning(node, iter->first);
            }
        }

        if (!blackValLoaded && !whiteValLoaded)
        {
            throwValueError(parent, "At least one of the clamp values must be provided.");
        }
    }
    else
    {
        throwValueError(parent, "The value needs to be a map.");
    }
}

inline void load(const YAML::Node & node, GradingPrimaryTransformRcPtr & t)
{
    CheckDuplicates(node);

    t = GradingPrimaryTransform::Create(GRADING_LOG);
    GradingPrimary values{ GRADING_LOG };
    bool brightnessLoaded{ false };
    bool contrastLoaded{ false };
    bool gammaLoaded{ false };
    bool offsetLoaded{ false };
    bool exposureLoaded{ false };
    bool liftLoaded{ false };
    bool gainLoaded{ false };
    bool saturationLoaded{ false };
    bool pivotLoaded{ false };
    bool pivotBlackLoaded{ false };
    bool pivotWhiteLoaded{ false };
    bool clampBlackLoaded{ false };
    bool clampWhiteLoaded{ false };

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if (key == "style")
        {
            std::string style;
            load(iter->second, style);
            t->setStyle(GradingStyleFromString(style.c_str()));
        }
        else if (key == "direction")
        {
            TransformDirection val;
            load(iter->second, val);
            t->setDirection(val);
        }
        else if (key == "brightness")
        {
            brightnessLoaded = true;
            load(iter->first, iter->second, values.m_brightness);
        }
        else if (key == "contrast")
        {
            contrastLoaded = true;
            load(iter->first, iter->second, values.m_contrast);
        }
        else if (key == "gamma")
        {
            gammaLoaded = true;
            load(iter->first, iter->second, values.m_gamma);
        }
        else if (key == "offset")
        {
            offsetLoaded = true;
            load(iter->first, iter->second, values.m_offset);
        }
        else if (key == "exposure")
        {
            exposureLoaded = true;
            load(iter->first, iter->second, values.m_exposure);
        }
        else if (key == "lift")
        {
            liftLoaded = true;
            load(iter->first, iter->second, values.m_lift);
        }
        else if (key == "gain")
        {
            gainLoaded = true;
            load(iter->first, iter->second, values.m_gain);
        }
        else if (key == "pivot")
        {
            loadPivot(iter->first, iter->second, values.m_pivot, pivotLoaded,
                      values.m_pivotBlack, pivotBlackLoaded, values.m_pivotWhite, pivotWhiteLoaded);
        }
        else if (key == "saturation")
        {
            saturationLoaded = true;
            load(iter->second, values.m_saturation);
        }
        else if (key == "clamp")
        {
            loadClamp(iter->first, iter->second, values.m_clampBlack, clampBlackLoaded,
                      values.m_clampWhite, clampWhiteLoaded);
        }
        else if (key == "name")
        {
            std::string name;
            load(iter->second, name);
            t->getFormatMetadata().setName(name.c_str());
        }
        else
        {
            LogUnknownKeyWarning(node.Tag(), iter->first);
        }
    }

    GradingPrimary valuesSet{ t->getStyle() };
    if (brightnessLoaded)
    {
        valuesSet.m_brightness = values.m_brightness;
    }
    if (contrastLoaded)
    {
        valuesSet.m_contrast = values.m_contrast;
    }
    if (gammaLoaded)
    {
        valuesSet.m_gamma = values.m_gamma;
    }
    if (offsetLoaded)
    {
        valuesSet.m_offset = values.m_offset;
    }
    if (exposureLoaded)
    {
        valuesSet.m_exposure = values.m_exposure;
    }
    if (liftLoaded)
    {
        valuesSet.m_lift = values.m_lift;
    }
    if (gainLoaded)
    {
        valuesSet.m_gain = values.m_gain;
    }
    if (saturationLoaded)
    {
        valuesSet.m_saturation = values.m_saturation;
    }
    if (pivotLoaded)
    {
        valuesSet.m_pivot = values.m_pivot;
    }
    if (pivotBlackLoaded)
    {
        valuesSet.m_pivotBlack = values.m_pivotBlack;
    }
    if (pivotWhiteLoaded)
    {
        valuesSet.m_pivotWhite = values.m_pivotWhite;
    }
    if (clampBlackLoaded)
    {
        valuesSet.m_clampBlack = values.m_clampBlack;
    }
    if (clampWhiteLoaded)
    {
        valuesSet.m_clampWhite = values.m_clampWhite;
    }

    t->setValue(valuesSet);
}

inline void save(YAML::Emitter & out, const char * paramName, const GradingRGBM & rgbm,
                 const GradingRGBM & defaultRgbm)
{
    if (rgbm != defaultRgbm)
    {
        std::vector<double> vals(3);
        vals[0] = rgbm.m_red;
        vals[1] = rgbm.m_green;
        vals[2] = rgbm.m_blue;
        out << YAML::Key << paramName;
        out << YAML::Value << YAML::Flow << YAML::BeginMap;
        out << YAML::Key << "rgb" << YAML::Value << YAML::Flow << vals;
        out << YAML::Key << "master";
        out << YAML::Value << YAML::Flow << rgbm.m_master;
        out << YAML::EndMap;
    }
}

inline void save(YAML::Emitter & out, const char * paramName, double val, double defaultVal)
{
    if (val != defaultVal)
    {
        out << YAML::Key << paramName << YAML::Value << YAML::Flow << val;
    }
}

inline void savePivot(YAML::Emitter & out, double val, bool saveContrast,
                      double blackVal, double defaultBlackVal,
                      double whiteVal, double defaultWhiteVal)
{
    if (saveContrast || blackVal != defaultBlackVal || whiteVal != defaultWhiteVal)
    {
        out << YAML::Key << "pivot";
        out << YAML::Value << YAML::Flow << YAML::BeginMap;
        if (saveContrast)
        {
            out << YAML::Key << "contrast" << YAML::Value << YAML::Flow << val;
        }
        save(out, "black", blackVal, defaultBlackVal);
        save(out, "white", whiteVal, defaultWhiteVal);
        out << YAML::EndMap;
    }
}

inline void saveClamp(YAML::Emitter & out,
                      double blackVal, double defaultBlackVal,
                      double whiteVal, double defaultWhiteVal)
{
    if (blackVal != defaultBlackVal || whiteVal != defaultWhiteVal)
    {
        out << YAML::Key << "clamp";
        out << YAML::Value << YAML::Flow << YAML::BeginMap;
        save(out, "black", blackVal, defaultBlackVal);
        save(out, "white", whiteVal, defaultWhiteVal);
        out << YAML::EndMap;
    }
}

inline void save(YAML::Emitter & out, ConstGradingPrimaryTransformRcPtr t)
{
    out << YAML::VerbatimTag("GradingPrimaryTransform");

    const auto style = t->getStyle();
    const auto & vals = t->getValue();
    const GradingPrimary defaultVals{ style };

    if (vals == defaultVals) out << YAML::Flow;
    out << YAML::BeginMap;

    EmitTransformName(out, t->getFormatMetadata());

    out << YAML::Key << "style";
    out << YAML::Value << YAML::Flow << GradingStyleToString(style);
    switch (style)
    {
    case GRADING_LOG:
    {
        save(out, "brightness", vals.m_brightness, defaultVals.m_brightness);
        save(out, "contrast", vals.m_contrast, defaultVals.m_contrast);
        save(out, "gamma", vals.m_gamma, defaultVals.m_gamma);
        save(out, "saturation", vals.m_saturation, defaultVals.m_saturation);
        const bool forcePivot = (vals.m_contrast != defaultVals.m_contrast) ||
                                (vals.m_pivot != defaultVals.m_pivot);
        savePivot(out, vals.m_pivot, forcePivot,
                  vals.m_pivotBlack, defaultVals.m_pivotBlack,
                  vals.m_pivotWhite, defaultVals.m_pivotWhite);
        break;
    }
    case GRADING_LIN:
    {
        save(out, "offset", vals.m_offset, defaultVals.m_offset);
        save(out, "exposure", vals.m_exposure, defaultVals.m_exposure);
        save(out, "contrast", vals.m_contrast, defaultVals.m_contrast);
        save(out, "saturation", vals.m_saturation, defaultVals.m_saturation);
        const bool forcePivot = (vals.m_contrast != defaultVals.m_contrast) ||
                                (vals.m_pivot != defaultVals.m_pivot);
        savePivot(out, vals.m_pivot, forcePivot, 0., 0., 0., 0.);
        break;
    }
    case GRADING_VIDEO:
    {
        save(out, "lift", vals.m_lift, defaultVals.m_lift);
        save(out, "gamma", vals.m_gamma, defaultVals.m_gamma);
        save(out, "gain", vals.m_gain, defaultVals.m_gain);
        save(out, "offset", vals.m_offset, defaultVals.m_offset);
        save(out, "saturation", vals.m_saturation, defaultVals.m_saturation);
        savePivot(out, 0., false, vals.m_pivotBlack, defaultVals.m_pivotBlack,
                  vals.m_pivotWhite, defaultVals.m_pivotWhite);
        break;
    }
    }
    saveClamp(out, vals.m_clampBlack, defaultVals.m_clampBlack,
              vals.m_clampWhite, defaultVals.m_clampWhite);

    EmitBaseTransformKeyValues(out, t);
    out << YAML::EndMap;
}

// GradingRGBCurveTransform

inline void load(const YAML::Node & parent, const YAML::Node & node, GradingBSplineCurveRcPtr & sc)
{
    if (node.Type() == YAML::NodeType::Map)
    {
        bool cpOK{ false };
        for (Iterator iter = node.begin(); iter != node.end(); ++iter)
        {
            const std::string & key = iter->first.as<std::string>();

            if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

            if (key == "control_points")
            {
                std::vector<float> vals;
                load(iter->second, vals);
                const size_t numVals = vals.size();
                if (numVals % 2 != 0)
                {
                    throwValueError(node.Tag(), iter->first, "An even number of float values is "
                        "required.");
                }
                const size_t numCtPts = numVals / 2;
                sc->setNumControlPoints(numCtPts);
                for (size_t c = 0; c < numCtPts; ++c)
                {
                    auto & pt = sc->getControlPoint(c);
                    pt.m_x = vals[2 * c];
                    pt.m_y = vals[2 * c + 1];
                }
                cpOK = true;
            }
            else if (key == "slopes")
            {
                std::vector<float> vals;
                load(iter->second, vals);
                const size_t numVals = vals.size();
                const size_t numCtPts = sc->getNumControlPoints();
                if (numVals != numCtPts)
                {
                    throwValueError(node.Tag(), iter->first, "Number of slopes must match number "
                        "of control points.");
                }
                for (size_t i = 0; i < numVals; ++i)
                {
                    sc->setSlope(i, vals[i]);
                }
            }
            else
            {
                LogUnknownKeyWarning(parent, iter->first);
            }
        }

        if (!cpOK)
        {
            throwValueError(parent, "control_points is required.");
        }
    }
    else
    {
        throwValueError(parent, "The value needs to be a map.");
    }
}

inline void load(const YAML::Node & node, GradingRGBCurveTransformRcPtr & t)
{
    CheckDuplicates(node);

    t = GradingRGBCurveTransform::Create(GRADING_LOG);

    GradingBSplineCurveRcPtr red;
    GradingBSplineCurveRcPtr green;
    GradingBSplineCurveRcPtr blue;
    GradingBSplineCurveRcPtr master;

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if (key == "style")
        {
            std::string style;
            load(iter->second, style);
            t->setStyle(GradingStyleFromString(style.c_str()));
        }
        else if (key == "direction")
        {
            TransformDirection val;
            load(iter->second, val);
            t->setDirection(val);
        }
        else if (key == "lintolog_bypass")
        {
            bool bypass = true;
            load(iter->second, bypass);
            t->setBypassLinToLog(bypass);
        }
        else if (key == "red")
        {
            red = GradingBSplineCurve::Create(0);
            load(iter->first, iter->second, red);
        }
        else if (key == "green")
        {
            green = GradingBSplineCurve::Create(0);
            load(iter->first, iter->second, green);
        }
        else if (key == "blue")
        {
            blue = GradingBSplineCurve::Create(0);
            load(iter->first, iter->second, blue);
        }
        else if (key == "master")
        {
            master = GradingBSplineCurve::Create(0);
            load(iter->first, iter->second, master);
        }
        else if (key == "name")
        {
            std::string name;
            load(iter->second, name);
            t->getFormatMetadata().setName(name.c_str());
        }
        else
        {
            LogUnknownKeyWarning(node.Tag(), iter->first);
        }
    }

    auto & defCurve = t->getStyle() == GRADING_LIN ? GradingRGBCurveImpl::DefaultLin :
                                                     GradingRGBCurveImpl::Default;

    if (!red) red = defCurve.createEditableCopy();
    if (!green) green = defCurve.createEditableCopy();
    if (!blue) blue = defCurve.createEditableCopy();
    if (!master) master = defCurve.createEditableCopy();
    auto curves = GradingRGBCurve::Create(red, green, blue, master);

    t->setValue(curves);
}

inline void save(YAML::Emitter & out, const char * paramName, const ConstGradingBSplineCurveRcPtr & curve)
{
    std::vector<float> ctPts;
    const size_t numCtPts = curve->getNumControlPoints();
    for (size_t c = 0; c < numCtPts; ++c)
    {
        const auto & pt = curve->getControlPoint(c);
        ctPts.push_back(pt.m_x);
        ctPts.push_back(pt.m_y);
    }
    out << YAML::Key << paramName;
    out << YAML::Flow << YAML::BeginMap;
    out << YAML::Key << "control_points" << YAML::Value << ctPts;
    if (!curve->slopesAreDefault())
    {
        // (Number of slopes is always the same as control points.)
        const size_t numSlopes = curve->getNumControlPoints();
        std::vector<float> slopes;
        for (size_t i = 0; i < numSlopes; ++i)
        {
            const float val = curve->getSlope(i);
            slopes.push_back(val);
        }
        out << YAML::Key << "slopes" << YAML::Value << slopes;
    }
    out << YAML::EndMap;
}

inline void save(YAML::Emitter & out, ConstGradingRGBCurveTransformRcPtr t)
{
    const auto & vals = t->getValue();
    auto & defCurve = t->getStyle() == GRADING_LIN ? GradingRGBCurveImpl::DefaultLin :
                                                     GradingRGBCurveImpl::Default;
    bool useLineBreaks = false;
    for (int c = 0; c < RGB_NUM_CURVES; ++c)
    {
        const auto & curve = vals->getCurve(static_cast<RGBCurveType>(c));
        if (*curve != defCurve)
        {
            useLineBreaks = true;
            break;
        }
    }

    out << YAML::VerbatimTag("GradingRGBCurveTransform");
    if (!useLineBreaks) out << YAML::Flow;
    out << YAML::BeginMap;

    EmitTransformName(out, t->getFormatMetadata());

    const auto style = t->getStyle();
    out << YAML::Key << "style";
    out << YAML::Value << YAML::Flow << GradingStyleToString(style);

    if (t->getBypassLinToLog())
    {
        out << YAML::Key << "lintolog_bypass";
        out << YAML::Value << YAML::Flow << true;
    }

    static const std::vector<const char *> curveNames = { "red", "green", "blue", "master" };
    for (int c = 0; c < RGB_NUM_CURVES; ++c)
    {
        const auto & curve = vals->getCurve(static_cast<RGBCurveType>(c));
        if ((*curve != defCurve) || !(curve->slopesAreDefault()))
        {
            save(out, curveNames[c], curve);
        }
    }

    EmitBaseTransformKeyValues(out, t);
    out << YAML::EndMap;
}

// GradingToneTransform

inline void load(const YAML::Node & parent, const YAML::Node & node, GradingRGBMSW & rgbm,
                 bool center, bool pivot)
{
    if (node.Type() == YAML::NodeType::Map)
    {
        bool rgbOK{ false };
        bool masterOK{ false };
        bool startOK{ false };
        bool widthOK{ false };

        for (Iterator iter = node.begin(); iter != node.end(); ++iter)
        {
            const std::string & key = iter->first.as<std::string>();

            if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

            if (key == "rgb")
            {
                std::vector<double> vals;
                load(iter->second, vals);
                if (vals.size() != 3)
                {
                    throwError(iter->first, "The RGB value needs to be a 3 doubles.");
                }
                rgbm.m_red = vals[0];
                rgbm.m_green = vals[1];
                rgbm.m_blue = vals[2];
                rgbOK = true;
            }
            else if (key == "master")
            {
                load(iter->second, rgbm.m_master);
                masterOK = true;
            }
            else if (key == (center ? "center" : "start"))
            {
                load(iter->second, rgbm.m_start);
                startOK = true;
            }
            else if (key == (pivot ? "pivot" : "width"))
            {
                load(iter->second, rgbm.m_width);
                widthOK = true;
            }
            else
            {
                LogUnknownKeyWarning(parent, iter->first);
            }
        }

        if (!rgbOK || !masterOK || !startOK || !widthOK)
        {
            std::ostringstream oss;
            oss << "Rgb, master, " << (center ? "center" : "start") << ", and "
                << (pivot ? "pivot" : "width") << " values are required.";
            throwValueError(parent, oss.str());
        }
    }
    else
    {
        throwValueError(parent, "The value needs to be a map.");
    }
}

inline void load(const YAML::Node & node, GradingToneTransformRcPtr & t)
{
    CheckDuplicates(node);

    t = GradingToneTransform::Create(GRADING_LOG);

    bool blacksLoaded = false;
    bool shadowsLoaded = false;
    bool midtonesLoaded = false;
    bool highlightsLoaded = false;
    bool whitesLoaded = false;
    GradingRGBMSW blacks, shadows, midtones, highlights, whites;
    double scontrast = 1.0;

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if (key == "style")
        {
            std::string style;
            load(iter->second, style);
            t->setStyle(GradingStyleFromString(style.c_str()));
        }
        else if (key == "direction")
        {
            TransformDirection val;
            load(iter->second, val);
            t->setDirection(val);
        }
        else if (key == "blacks")
        {
            blacksLoaded = true;
            load(iter->first, iter->second, blacks, false, false);
        }
        else if (key == "shadows")
        {
            shadowsLoaded = true;
            load(iter->first, iter->second, shadows, false, true);
        }
        else if (key == "midtones")
        {
            midtonesLoaded = true;
            load(iter->first, iter->second, midtones, true, false);
        }
        else if (key == "highlights")
        {
            highlightsLoaded = true;
            load(iter->first, iter->second, highlights, false, true);
        }
        else if (key == "whites")
        {
            whitesLoaded = true;
            load(iter->first, iter->second, whites, false, false);
        }
        else if (key == "s_contrast")
        {
            load(iter->second, scontrast);
        }
        else if (key == "name")
        {
            std::string name;
            load(iter->second, name);
            t->getFormatMetadata().setName(name.c_str());
        }
        else
        {
            LogUnknownKeyWarning(node.Tag(), iter->first);
        }
    }

    GradingTone values{ t->getStyle() };
    values.m_scontrast = scontrast;
    if (blacksLoaded) values.m_blacks = blacks;
    if (shadowsLoaded) values.m_shadows = shadows;
    if (midtonesLoaded) values.m_midtones = midtones;
    if (highlightsLoaded) values.m_highlights = highlights;
    if (whitesLoaded) values.m_whites = whites;

    t->setValue(values);
}

inline void save(YAML::Emitter & out, const char * paramName, const GradingRGBMSW & rgbm,
                 const GradingRGBMSW & defaultRgbm, bool center, bool pivot)
{
    if (rgbm != defaultRgbm)
    {
        std::vector<double> vals{ rgbm.m_red, rgbm.m_green, rgbm.m_blue };
        out << YAML::Key << paramName;
        out << YAML::Value << YAML::Flow << YAML::BeginMap;
        out << YAML::Key << "rgb" << YAML::Value << YAML::Flow << vals;
        out << YAML::Key << "master" << YAML::Value << YAML::Flow << rgbm.m_master;
        out << YAML::Key << (center ? "center" : "start") << YAML::Value << YAML::Flow 
            << rgbm.m_start;
        out << YAML::Key << (pivot ? "pivot" : "width") << YAML::Value << YAML::Flow
            << rgbm.m_width;
        out << YAML::EndMap;
    }
}

inline void save(YAML::Emitter & out, ConstGradingToneTransformRcPtr t)
{
    out << YAML::VerbatimTag("GradingToneTransform");

    const auto style = t->getStyle();
    const auto & vals = t->getValue();
    const GradingTone defaultVals(style);

    if (vals == defaultVals) out << YAML::Flow;
    out << YAML::BeginMap;

    EmitTransformName(out, t->getFormatMetadata());

    out << YAML::Key << "style";
    out << YAML::Value << YAML::Flow << GradingStyleToString(style);
    save(out, "blacks", vals.m_blacks, defaultVals.m_blacks, false, false);
    save(out, "shadows", vals.m_shadows, defaultVals.m_shadows, false, true);
    save(out, "midtones", vals.m_midtones, defaultVals.m_midtones, true, false);
    save(out, "highlights", vals.m_highlights, defaultVals.m_highlights, false, true);
    save(out, "whites", vals.m_whites, defaultVals.m_whites, false, false);
    save(out, "s_contrast", vals.m_scontrast, defaultVals.m_scontrast);

    EmitBaseTransformKeyValues(out, t);
    out << YAML::EndMap;
}

// GroupTransform

void load(const YAML::Node& node, TransformRcPtr& t);
void save(YAML::Emitter& out, ConstTransformRcPtr t, unsigned int majorVersion);

inline void load(const YAML::Node& node, GroupTransformRcPtr& t)
{
    t = GroupTransform::Create();

    CheckDuplicates(node);

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if(key == "children")
        {
            for (std::size_t i = 0; i < iter->second.size(); i++)
            {
                const YAML::Node & val = iter->second[i];

                TransformRcPtr childTransform;
                load(val, childTransform);

                // TODO: consider the forwards-compatibility implication of
                // throwing an exception.  Should this be a warning, instead?
                if(!childTransform)
                {
                    throwValueError(node.Tag(), iter->first, 
                                    "Child transform could not be parsed.");
                }

                t->appendTransform(childTransform);
            }
        }
        else if(key == "direction")
        {
            TransformDirection val;
            load(iter->second, val);
            t->setDirection(val);
        }
        else if(key == "name")
        {
            std::string name;
            load(iter->second, name);
            t->getFormatMetadata().setName(name.c_str());
        }
        else
        {
            LogUnknownKeyWarning(node, iter->first);
        }
    }
}

inline void save(YAML::Emitter& out, ConstGroupTransformRcPtr t, unsigned int majorVersion)
{
    out << YAML::VerbatimTag("GroupTransform");
    out << YAML::BeginMap;

    if (majorVersion >= 2)
    {
        EmitTransformName(out, t->getFormatMetadata());
    }
    EmitBaseTransformKeyValues(out, t);

    out << YAML::Key << "children";
    out << YAML::Value;

    out << YAML::BeginSeq;
    for(int i = 0; i < t->getNumTransforms(); ++i)
    {
        save(out, t->getTransform(i), majorVersion);
    }
    out << YAML::EndSeq;

    out << YAML::EndMap;
}

// LogAffineTransform

inline void loadLogParam(const YAML::Node & node,
                         double(&param)[3],
                         const std::string & paramName)
{
    if (node.size() == 0)
    {
        // If a single value is provided.
        double val = 0.0;
        load(node, val);
        param[0] = val;
        param[1] = val;
        param[2] = val;
    }
    else
    {
        std::vector<double> val;
        load(node, val);
        if (val.size() != 3)
        {
            std::ostringstream os;
            os << "LogAffine/CameraTransform parse error, " << paramName;
            os << " value field must have 3 components. Found '" << val.size() << "'.";
            throw Exception(os.str().c_str());
        }
        param[0] = val[0];
        param[1] = val[1];
        param[2] = val[2];
    }
}

inline void load(const YAML::Node& node, LogAffineTransformRcPtr& t)
{
    t = LogAffineTransform::Create();

    CheckDuplicates(node);

    double base = 2.0;
    double logSlope[3] = { 1.0, 1.0, 1.0 };
    double linSlope[3] = { 1.0, 1.0, 1.0 };
    double linOffset[3] = { 0.0, 0.0, 0.0 };
    double logOffset[3] = { 0.0, 0.0, 0.0 };

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if (key == "base")
        {
            size_t nb = iter->second.size();
            if (nb == 0)
            {
                load(iter->second, base);
            }
            else
            {
                std::ostringstream os;
                os << "LogAffineTransform parse error, base must be a ";
                os << "single double. Found " << nb << ".";
                throw Exception(os.str().c_str());
            }
        }
        else if (key == "lin_side_offset")
        {
            loadLogParam(iter->second, linOffset, key);
        }
        else if (key == "lin_side_slope")
        {
            loadLogParam(iter->second, linSlope, key);
        }
        else if (key == "log_side_offset")
        {
            loadLogParam(iter->second, logOffset, key);
        }
        else if (key == "log_side_slope")
        {
            loadLogParam(iter->second, logSlope, key);
        }
        else if(key == "direction")
        {
            TransformDirection val;
            load(iter->second, val);
            t->setDirection(val);
        }
        else if(key == "name")
        {
            std::string name;
            load(iter->second, name);
            t->getFormatMetadata().setName(name.c_str());
        }
        else
        {
            LogUnknownKeyWarning(node, iter->first);
        }
    }
    t->setBase(base);
    t->setLogSideSlopeValue(logSlope);
    t->setLinSideSlopeValue(linSlope);
    t->setLinSideOffsetValue(linOffset);
    t->setLogSideOffsetValue(logOffset);
}

inline void saveLogParam(YAML::Emitter& out, const double(&param)[3],
                         double defaultVal, const char * paramName)
{
    // (See test in Config_test.cpp that verifies double precision is preserved.)
    if (param[0] == param[1] && param[0] == param[2])
    {
        // Set defaultVal to NaN if there is no default value. It will always write param,
        // otherwise default params are not saved.
        if (param[0] != defaultVal)
        {
            out << YAML::Key << paramName << YAML::Value << param[0];
        }
    }
    else
    {
        std::vector<double> vals;
        vals.assign(param, param + 3);
        out << YAML::Key << paramName << YAML::Value << vals;
    }
}

inline void save(YAML::Emitter& out, ConstLogAffineTransformRcPtr t)
{
    out << YAML::VerbatimTag("LogAffineTransform");
    out << YAML::Flow << YAML::BeginMap;

    EmitTransformName(out, t->getFormatMetadata());

    double logSlope[3] = { 1.0, 1.0, 1.0 };
    double linSlope[3] = { 1.0, 1.0, 1.0 };
    double linOffset[3] = { 0.0, 0.0, 0.0 };
    double logOffset[3] = { 0.0, 0.0, 0.0 };
    t->getLogSideSlopeValue(logSlope);
    t->getLogSideOffsetValue(logOffset);
    t->getLinSideSlopeValue(linSlope);
    t->getLinSideOffsetValue(linOffset);

    const double baseVal = t->getBase();
    if (baseVal != 2.0)
    {
        out << YAML::Key << "base" << YAML::Value << baseVal;
    }
    saveLogParam(out, logSlope, 1.0, "log_side_slope");
    saveLogParam(out, logOffset, 0.0, "log_side_offset");
    saveLogParam(out, linSlope, 1.0, "lin_side_slope");
    saveLogParam(out, linOffset, 0.0, "lin_side_offset");

    EmitBaseTransformKeyValues(out, t);
    out << YAML::EndMap;
}

// LogCameraTransform

inline void load(const YAML::Node & node, LogCameraTransformRcPtr & t)
{
    double linBreak[3] = { 0.0, 0.0, 0.0 };
    t = LogCameraTransform::Create(linBreak);

    CheckDuplicates(node);

    double base = 2.0;
    double logSlope[3] = { 1.0, 1.0, 1.0 };
    double linSlope[3] = { 1.0, 1.0, 1.0 };
    double linOffset[3] = { 0.0, 0.0, 0.0 };
    double logOffset[3] = { 0.0, 0.0, 0.0 };
    double linearSlope[3] = { 1.0, 1.0, 1.0 };
    bool linBreakFound = false;
    bool linearSlopeFound = false;

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if (key == "base")
        {
            size_t nb = iter->second.size();
            if (nb == 0)
            {
                load(iter->second, base);
            }
            else
            {
                std::ostringstream os;
                os << "LogCameraTransform parse error, base must be a ";
                os << "single double. Found " << nb << ".";
                throw Exception(os.str().c_str());
            }
        }
        else if (key == "lin_side_offset")
        {
            loadLogParam(iter->second, linOffset, key);
        }
        else if (key == "lin_side_slope")
        {
            loadLogParam(iter->second, linSlope, key);
        }
        else if (key == "log_side_offset")
        {
            loadLogParam(iter->second, logOffset, key);
        }
        else if (key == "log_side_slope")
        {
            loadLogParam(iter->second, logSlope, key);
        }
        else if (key == "lin_side_break")
        {
            linBreakFound = true;
            loadLogParam(iter->second, linBreak, key);
        }
        else if (key == "linear_slope")
        {
            linearSlopeFound = true;
            loadLogParam(iter->second, linearSlope, key);
        }
        else if (key == "direction")
        {
            TransformDirection val;
            load(iter->second, val);
            t->setDirection(val);
        }
        else if (key == "name")
        {
            std::string name;
            load(iter->second, name);
            t->getFormatMetadata().setName(name.c_str());
        }
        else
        {
            LogUnknownKeyWarning(node, iter->first);
        }
    }
    if (!linBreakFound)
    {
        throw Exception("LogCameraTransform parse error: lin_side_break values are missing.");
    }
    t->setBase(base);
    t->setLogSideSlopeValue(logSlope);
    t->setLinSideSlopeValue(linSlope);
    t->setLinSideOffsetValue(linOffset);
    t->setLogSideOffsetValue(logOffset);
    t->setLinSideBreakValue(linBreak);
    if (linearSlopeFound)
    {
        t->setLinearSlopeValue(linearSlope);
    }
}

inline void save(YAML::Emitter& out, ConstLogCameraTransformRcPtr t)
{
    out << YAML::VerbatimTag("LogCameraTransform");
    out << YAML::Flow << YAML::BeginMap;

    EmitTransformName(out, t->getFormatMetadata());

    double logSlope[3] = { 1.0, 1.0, 1.0 };
    double linSlope[3] = { 1.0, 1.0, 1.0 };
    double linOffset[3] = { 0.0, 0.0, 0.0 };
    double logOffset[3] = { 0.0, 0.0, 0.0 };
    double linBreak[3] = { 0.0, 0.0, 0.0 };
    double linearSlope[3] = { 1.0, 1.0, 1.0 };
    t->getLogSideSlopeValue(logSlope);
    t->getLogSideOffsetValue(logOffset);
    t->getLinSideSlopeValue(linSlope);
    t->getLinSideOffsetValue(linOffset);
    t->getLinSideBreakValue(linBreak);
    const bool hasLinearSlope = t->getLinearSlopeValue(linearSlope);

    const double baseVal = t->getBase();
    if (baseVal != 2.0)
    {
        out << YAML::Key << "base" << YAML::Value << baseVal;
    }
    saveLogParam(out, logSlope, 1.0, "log_side_slope");
    saveLogParam(out, logOffset, 0.0, "log_side_offset");
    saveLogParam(out, linSlope, 1.0, "lin_side_slope");
    saveLogParam(out, linOffset, 0.0, "lin_side_offset");
    saveLogParam(out, linBreak, std::numeric_limits<double>::quiet_NaN(), "lin_side_break");
    if (hasLinearSlope)
    {
        saveLogParam(out, linearSlope, std::numeric_limits<double>::quiet_NaN(), "linear_slope");
    }

    EmitBaseTransformKeyValues(out, t);
    out << YAML::EndMap;
}

// LogTransform

inline void load(const YAML::Node& node, LogTransformRcPtr& t)
{
    t = LogTransform::Create();

    CheckDuplicates(node);

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if (key == "base")
        {
            double base = 2.0;
            size_t nb = iter->second.size();
            if (nb == 0)
            {
                load(iter->second, base);
            }
            else
            {
                std::ostringstream os;
                os << "LogTransform parse error, base must be a ";
                os << " single double. Found " << nb << ".";
                throw Exception(os.str().c_str());
            }
            t->setBase(base);
        }
        else if (key == "direction")
        {
            TransformDirection val;
            load(iter->second, val);
            t->setDirection(val);
        }
        else if (key == "name")
        {
            std::string name;
            load(iter->second, name);
            t->getFormatMetadata().setName(name.c_str());
        }
        else
        {
            LogUnknownKeyWarning(node.Tag(), iter->first);
        }
    }
}

inline void save(YAML::Emitter& out, ConstLogTransformRcPtr t, unsigned int majorVersion)
{
    out << YAML::VerbatimTag("LogTransform");
    out << YAML::Flow << YAML::BeginMap;

    if (majorVersion >= 2)
    {
        EmitTransformName(out, t->getFormatMetadata());
    }

    const double baseVal = t->getBase();
    if (baseVal != 2.0 || majorVersion < 2)
    {
        out << YAML::Key << "base" << YAML::Value << baseVal;
    }
    EmitBaseTransformKeyValues(out, t);
    out << YAML::EndMap;
}

// LookTransform

inline void load(const YAML::Node& node, LookTransformRcPtr& t)
{
    t = LookTransform::Create();

    CheckDuplicates(node);

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if(key == "src")
        {
            std::string stringval;
            load(iter->second, stringval);
            t->setSrc(stringval.c_str());
        }
        else if(key == "dst")
        {
            std::string stringval;
            load(iter->second, stringval);
            t->setDst(stringval.c_str());
        }
        else if(key == "looks")
        {
            std::string stringval;
            load(iter->second, stringval);
            t->setLooks(stringval.c_str());
        }
        else if(key == "direction")
        {
            TransformDirection val;
            load(iter->second, val);
            t->setDirection(val);
        }
        else
        {
            LogUnknownKeyWarning(node, iter->first);
        }
    }
}

inline void save(YAML::Emitter& out, ConstLookTransformRcPtr t)
{
    out << YAML::VerbatimTag("LookTransform");
    out << YAML::Flow << YAML::BeginMap;
    out << YAML::Key << "src" << YAML::Value << t->getSrc();
    out << YAML::Key << "dst" << YAML::Value << t->getDst();
    out << YAML::Key << "looks" << YAML::Value << t->getLooks();
    EmitBaseTransformKeyValues(out, t);
    out << YAML::EndMap;
}

// MatrixTransform

inline void load(const YAML::Node& node, MatrixTransformRcPtr& t)
{
    t = MatrixTransform::Create();

    CheckDuplicates(node);

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if(key == "matrix")
        {
            std::vector<double> val;
            load(iter->second, val);
            if(val.size() != 16)
            {
                std::ostringstream os;
                os << "'matrix' values must be 16 ";
                os << "numbers. Found '" << val.size() << "'.";
                throwValueError(node.Tag(), iter->first, os.str());
            }
            t->setMatrix(&val[0]);
        }
        else if(key == "offset")
        {
            std::vector<double> val;
            load(iter->second, val);
            if(val.size() != 4)
            {
                std::ostringstream os;
                os << "'offset' values must be 4 ";
                os << "numbers. Found '" << val.size() << "'.";
                throwValueError(node.Tag(), iter->first, os.str());
            }
            t->setOffset(&val[0]);
        }
        else if(key == "direction")
        {
            TransformDirection val;
            load(iter->second, val);
            t->setDirection(val);
        }
        else if(key == "name")
        {
            std::string name;
            load(iter->second, name);
            t->getFormatMetadata().setName(name.c_str());
        }
        else
        {
            LogUnknownKeyWarning(node, iter->first);
        }
    }
}

inline void save(YAML::Emitter& out, ConstMatrixTransformRcPtr t, unsigned int majorVersion)
{
    out << YAML::VerbatimTag("MatrixTransform");
    out << YAML::Flow << YAML::BeginMap;

    if (majorVersion >= 2)
    {
        EmitTransformName(out, t->getFormatMetadata());
    }

    std::vector<double> matrix(16, 0.0);
    t->getMatrix(&matrix[0]);
    if(!IsM44Identity(&matrix[0]))
    {
        out << YAML::Key << "matrix";
        out << YAML::Value << YAML::Flow << matrix;
    }

    std::vector<double> offset(4, 0.0);
    t->getOffset(&offset[0]);
    if(!IsVecEqualToZero(&offset[0],4))
    {
        out << YAML::Key << "offset";
        out << YAML::Value << YAML::Flow << offset;
    }

    EmitBaseTransformKeyValues(out, t);
    out << YAML::EndMap;
}

// RangeTransform

inline void load(const YAML::Node& node, RangeTransformRcPtr& t)
{
    t = RangeTransform::Create();

    CheckDuplicates(node);

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        double val = 0.0;

        // TODO: parsing could be more strict (same applies for other transforms)
        // Could enforce that second is 1 float only and that keys
        // are only there once.
        if(key == "min_in_value")
        {
            load(iter->second, val);
            t->setMinInValue(val);
        }
        else if(key == "max_in_value")
        {
            load(iter->second, val);
            t->setMaxInValue(val);
        }
        else if(key == "min_out_value")
        {
            load(iter->second, val);
            t->setMinOutValue(val);
        }
        else if(key == "max_out_value")
        {
            load(iter->second, val);
            t->setMaxOutValue(val);
        }
        else if(key == "style")
        {
            std::string style;
            load(iter->second, style);
            t->setStyle(RangeStyleFromString(style.c_str()));
        }
        else if(key == "direction")
        {
            TransformDirection dir;
            load(iter->second, dir);
            t->setDirection(dir);
        }
        else if(key == "name")
        {
            std::string name;
            load(iter->second, name);
            t->getFormatMetadata().setName(name.c_str());
        }
        else
        {
            LogUnknownKeyWarning(node, iter->first);
        }
    }
}

inline void save(YAML::Emitter& out, ConstRangeTransformRcPtr t)
{
    out << YAML::VerbatimTag("RangeTransform");
    out << YAML::Flow << YAML::BeginMap;

    EmitTransformName(out, t->getFormatMetadata());

    if(t->hasMinInValue())
    {
        out << YAML::Key << "min_in_value";
        out << YAML::Value << YAML::Flow << t->getMinInValue();
    }

    if(t->hasMaxInValue())
    {
        out << YAML::Key << "max_in_value";
        out << YAML::Value << YAML::Flow << t->getMaxInValue();
    }

    if(t->hasMinOutValue())
    {
        out << YAML::Key << "min_out_value";
        out << YAML::Value << YAML::Flow << t->getMinOutValue();
    }

    if(t->hasMaxOutValue())
    {
        out << YAML::Key << "max_out_value";
        out << YAML::Value << YAML::Flow << t->getMaxOutValue();
    }

    if(t->getStyle()!=RANGE_CLAMP)
    {
        out << YAML::Key << "style";
        out << YAML::Value << YAML::Flow << RangeStyleToString(t->getStyle());
    }

    EmitBaseTransformKeyValues(out, t);
    out << YAML::EndMap;
}

// Transform

void load(const YAML::Node& node, TransformRcPtr& t)
{
    if(node.Type() != YAML::NodeType::Map)
    {
        std::ostringstream os;
        os << "Unsupported Transform type encountered: (" 
            << node.Type() << ") in OCIO profile. "
            << "Only Mapping types supported.";
        throwError(node, os.str());
    }

    std::string type = node.Tag();

    if(type == "AllocationTransform")
    {
        AllocationTransformRcPtr temp;
        load(node, temp);
        t = temp;
    }
    else if(type == "BuiltinTransform")
    {
        BuiltinTransformRcPtr temp;
        load(node, temp);
        t = temp;
    }
    else if(type == "CDLTransform")
    {
        CDLTransformRcPtr temp;
        load(node, temp);
        t = temp;
    }
    else if(type == "ColorSpaceTransform")
    {
        ColorSpaceTransformRcPtr temp;
        load(node, temp);
        t = temp;
    }
    else if (type == "DisplayViewTransform")
    {
        DisplayViewTransformRcPtr temp;
        load(node, temp);
        t = temp;
    }
    else if(type == "ExponentTransform")
    {
        ExponentTransformRcPtr temp;
        load(node, temp);
        t = temp;
    }
    else if (type == "ExponentWithLinearTransform")
    {
        ExponentWithLinearTransformRcPtr temp;
        load(node, temp);
        t = temp;
    }
    else if (type == "ExposureContrastTransform")
    {
        ExposureContrastTransformRcPtr temp;
        load(node, temp);
        t = temp;
    }
    else if(type == "FileTransform")
    {
        FileTransformRcPtr temp;
        load(node, temp);
        t = temp;
    }
    else if(type == "FixedFunctionTransform")
    {
        FixedFunctionTransformRcPtr temp;
        load(node, temp);
        t = temp;
    }
    else if (type == "GradingPrimaryTransform")
    {
        GradingPrimaryTransformRcPtr temp;
        load(node, temp);
        t = temp;
    }
    else if (type == "GradingRGBCurveTransform")
    {
        GradingRGBCurveTransformRcPtr temp;
        load(node, temp);
        t = temp;
    }
    else if (type == "GradingToneTransform")
    {
        GradingToneTransformRcPtr temp;
        load(node, temp);
        t = temp;
    }
    else if (type == "GroupTransform")
    {
        GroupTransformRcPtr temp;
        load(node, temp);
        t = temp;
    }
    else if(type == "LogAffineTransform")
    {
        LogAffineTransformRcPtr temp;
        load(node, temp);
        t = temp;
    }
    else if (type == "LogCameraTransform")
    {
        LogCameraTransformRcPtr temp;
        load(node, temp);
        t = temp;
    }
    else if(type == "LogTransform")
    {
        LogTransformRcPtr temp;
        load(node, temp);
        t = temp;
    }
    else if(type == "LookTransform")
    {
        LookTransformRcPtr temp;
        load(node, temp);
        t = temp;
    }
    else if(type == "MatrixTransform")
    {
        MatrixTransformRcPtr temp;
        load(node, temp);
        t = temp;
    }
    else if(type == "RangeTransform")
    {
        RangeTransformRcPtr temp;
        load(node, temp);
        t = temp;
    }
    else
    {
        // TODO: add a new empty (better name?) aka passthru Transform()
        // which does nothing. This is so unsupported !<tag> types don't
        // throw an exception. Alternatively this could be caught in the
        // GroupTransformRcPtr >> operator with some type of
        // supported_tag() method

        // TODO: consider the forwards-compatibility implication of
        // throwing an exception.  Should this be a warning, instead?

        //  t = EmptyTransformRcPtr(new EmptyTransform(), &deleter);
        std::ostringstream os;
        os << "Unsupported transform type !<" << type << "> in OCIO profile. ";
        throwError(node, os.str());
    }
}

void save(YAML::Emitter& out, ConstTransformRcPtr t, unsigned int majorVersion)
{
    if(ConstAllocationTransformRcPtr Allocation_tran = \
        DynamicPtrCast<const AllocationTransform>(t))
        save(out, Allocation_tran);
    else if (ConstBuiltinTransformRcPtr builtin_tran = \
        DynamicPtrCast<const BuiltinTransform>(t))
        save(out, builtin_tran);
    else if(ConstCDLTransformRcPtr CDL_tran = \
        DynamicPtrCast<const CDLTransform>(t))
        save(out, CDL_tran, majorVersion);
    else if(ConstColorSpaceTransformRcPtr ColorSpace_tran = \
        DynamicPtrCast<const ColorSpaceTransform>(t))
        save(out, ColorSpace_tran);
    else if (ConstDisplayViewTransformRcPtr Display_tran = \
        DynamicPtrCast<const DisplayViewTransform>(t))
        save(out, Display_tran);
    else if(ConstExponentTransformRcPtr Exponent_tran = \
        DynamicPtrCast<const ExponentTransform>(t))
        save(out, Exponent_tran, majorVersion);
    else if (ConstExponentWithLinearTransformRcPtr ExpLinear_tran = \
        DynamicPtrCast<const ExponentWithLinearTransform>(t))
        save(out, ExpLinear_tran);
    else if(ConstFileTransformRcPtr File_tran = \
        DynamicPtrCast<const FileTransform>(t))
        save(out, File_tran, majorVersion);
    else if (ConstExposureContrastTransformRcPtr File_tran = \
        DynamicPtrCast<const ExposureContrastTransform>(t))
        save(out, File_tran);
    else if(ConstFixedFunctionTransformRcPtr Func_tran = \
        DynamicPtrCast<const FixedFunctionTransform>(t))
        save(out, Func_tran);
    else if (ConstGradingPrimaryTransformRcPtr GP_tran = \
        DynamicPtrCast<const GradingPrimaryTransform>(t))
        save(out, GP_tran);
    else if (ConstGradingRGBCurveTransformRcPtr GC_tran = \
        DynamicPtrCast<const GradingRGBCurveTransform>(t))
        save(out, GC_tran);
    else if (ConstGradingToneTransformRcPtr GT_tran = \
        DynamicPtrCast<const GradingToneTransform>(t))
        save(out, GT_tran);
    else if(ConstGroupTransformRcPtr Group_tran = \
        DynamicPtrCast<const GroupTransform>(t))
        save(out, Group_tran, majorVersion);
    else if(ConstLogAffineTransformRcPtr Log_tran = \
        DynamicPtrCast<const LogAffineTransform>(t))
        save(out, Log_tran);
    else if (ConstLogCameraTransformRcPtr Log_tran = \
        DynamicPtrCast<const LogCameraTransform>(t))
        save(out, Log_tran);
    else if(ConstLogTransformRcPtr Log_tran = \
        DynamicPtrCast<const LogTransform>(t))
        save(out, Log_tran, majorVersion);
    else if(ConstLookTransformRcPtr Look_tran = \
        DynamicPtrCast<const LookTransform>(t))
        save(out, Look_tran);
    else if(ConstMatrixTransformRcPtr Matrix_tran = \
        DynamicPtrCast<const MatrixTransform>(t))
        save(out, Matrix_tran, majorVersion);
    else if(ConstRangeTransformRcPtr Range_tran = \
        DynamicPtrCast<const RangeTransform>(t))
        save(out, Range_tran);
    else
        throw Exception("Unsupported Transform() type for serialization.");
}

// ColorSpace

inline void load(const YAML::Node& node, ColorSpaceRcPtr& cs, unsigned int majorVersion)
{
    if(node.Tag() != "ColorSpace")
        return; // not a !<ColorSpace> tag

    if(node.Type() != YAML::NodeType::Map)
    {
        std::ostringstream os;
        os << "The '!<ColorSpace>' content needs to be a map.";
        throwError(node, os.str());
    }

    CheckDuplicates(node);

    std::string stringval;
    bool boolval;

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if(key == "name")
        {
            load(iter->second, stringval);
            cs->setName(stringval.c_str());
        }
        else if (key == "aliases")
        {
            StringUtils::StringVec aliases;
            load(iter->second, aliases);
            for (const auto & alias : aliases)
            {
                cs->addAlias(alias.c_str());
            }
        }
        else if (key == "interop_id") 
        {
            load(iter->second, stringval);
            cs->setInteropID(stringval.c_str());
        }
        else if(key == "description")
        {
            loadDescription(iter->second, stringval);
            cs->setDescription(stringval.c_str());
        }
        else if (key == "interchange")
        {
            CustomKeysLoader kv;
            loadCustomKeys(iter->second, kv, "ColorSpace interchange");

            for (const auto& keyval : kv.m_keyVals)
            {
                std::string keystr = keyval.first.as<std::string>();
                std::string valstr = keyval.second.as<std::string>();
                valstr = SanitizeNewlines(valstr);
                
                // OCIO exception means the key is not recognized. Convert that to a warning.
                try
                {
                    cs->setInterchangeAttribute(keystr.c_str(), valstr.c_str());
                }
                catch (Exception &)
                {
                    LogUnknownKeyWarning(key, keyval.first);
                }
            }
        }
        else if(key == "family")
        {
            load(iter->second, stringval);
            cs->setFamily(stringval.c_str());
        }
        else if(key == "equalitygroup")
        {
            load(iter->second, stringval);
            cs->setEqualityGroup(stringval.c_str());
        }
        else if(key == "bitdepth")
        {
            BitDepth ret;
            load(iter->second, ret);
            cs->setBitDepth(ret);
        }
        else if(key == "isdata")
        {
            load(iter->second, boolval);
            cs->setIsData(boolval);
        }
        else if(key == "categories")
        {
            StringUtils::StringVec categories;
            load(iter->second, categories);
            for(auto name : categories)
            {
                cs->addCategory(name.c_str());
            }
        }
        else if (key == "encoding")
        {
            load(iter->second, stringval);
            cs->setEncoding(stringval.c_str());
        }
        else if(key == "allocation")
        {
            Allocation val;
            load(iter->second, val);
            cs->setAllocation(val);
        }
        else if(key == "allocationvars")
        {
            std::vector<float> val;
            load(iter->second, val);
            if(!val.empty())
                cs->setAllocationVars(static_cast<int>(val.size()), &val[0]);
        }
        else if(key == "to_reference" || (majorVersion >= 2 && key == "to_scene_reference"))
        {
            if (cs->getReferenceSpaceType() == REFERENCE_SPACE_DISPLAY)
            {
                throwError(node, "'to_reference' or 'to_scene_reference' cannot be used for a "
                                 "display color space.");
            }
            TransformRcPtr val;
            load(iter->second, val);
            cs->setTransform(val, COLORSPACE_DIR_TO_REFERENCE);
        }
        else if (key == "to_display_reference")
        {
            if (cs->getReferenceSpaceType() == REFERENCE_SPACE_SCENE)
            {
                throwError(node, "'to_display_reference' cannot be used for a "
                                 "non-display color space.");
            }
            TransformRcPtr val;
            load(iter->second, val);
            cs->setTransform(val, COLORSPACE_DIR_TO_REFERENCE);
        }
        else if(key == "from_reference" || (majorVersion >= 2 && key == "from_scene_reference"))
        {
            if (cs->getReferenceSpaceType() == REFERENCE_SPACE_DISPLAY)
            {
                throwError(node, "'from_reference' or 'from_scene_reference' cannot be used for "
                                 "a display color space.");
            }
            TransformRcPtr val;
            load(iter->second, val);
            cs->setTransform(val, COLORSPACE_DIR_FROM_REFERENCE);
        }
        else if (key == "from_display_reference")
        {
            if (cs->getReferenceSpaceType() == REFERENCE_SPACE_SCENE)
            {
                throwError(node, "'from_display_reference' cannot be used for a "
                                 "non-display color space.");
            }
            TransformRcPtr val;
            load(iter->second, val);
            cs->setTransform(val, COLORSPACE_DIR_FROM_REFERENCE);
        }
        else
        {
            LogUnknownKeyWarning(node, iter->first);
        }
    }
}

inline void save(YAML::Emitter& out, ConstColorSpaceRcPtr cs, unsigned int majorVersion)
{
    out << YAML::VerbatimTag("ColorSpace");
    out << YAML::BeginMap;

    out << YAML::Key << "name" << YAML::Value << cs->getName();
    const size_t numAliases = cs->getNumAliases();
    if (majorVersion >= 2 && numAliases)
    {
        out << YAML::Key << "aliases";
        StringUtils::StringVec aliases;
        for (size_t aidx = 0; aidx < numAliases; ++aidx)
        {
            aliases.push_back(cs->getAlias(aidx));
        }
        out << YAML::Flow << YAML::Value << aliases;
    }

    const std::string interopID{ cs->getInteropID() };
    if (!interopID.empty())
    {
        out << YAML::Key << "interop_id";
        out << YAML::Value << interopID;
    }

    out << YAML::Key << "family" << YAML::Value << cs->getFamily();

    out << YAML::Key << "equalitygroup" << YAML::Value << cs->getEqualityGroup();

    out << YAML::Key << "bitdepth" << YAML::Value;
    save(out, cs->getBitDepth());
    
    saveDescription(out, cs->getDescription());

    out << YAML::Key << "isdata" << YAML::Value << cs->isData();

    if(cs->getNumCategories() > 0)
    {
        StringUtils::StringVec categories;
        for(int idx=0; idx<cs->getNumCategories(); ++idx)
        {
            categories.push_back(cs->getCategory(idx));
        }
        out << YAML::Key << "categories";
        out << YAML::Flow << YAML::Value << categories;
    }

    const std::string is{ cs->getEncoding() };
    if (!is.empty())
    {
        out << YAML::Key << "encoding";
        out << YAML::Value << is;
    }

    auto interchangemap = cs->getInterchangeAttributes();
    if (interchangemap.size())
    {
        out << YAML::Key << "interchange";
        out << YAML::Value;
        out << YAML::BeginMap;
        for (const auto& keyval : interchangemap)
        {
            std::string valStr = SanitizeNewlines(keyval.second);

            out << YAML::Key << keyval.first << YAML::Value;
            if (valStr.find_first_of('\n') != std::string::npos) 
            {
                out << YAML::Literal;
            }
            out << valStr;
        }

        out << YAML::EndMap;
    }

    out << YAML::Key << "allocation" << YAML::Value;
    save(out, cs->getAllocation());
    if(cs->getAllocationNumVars() > 0)
    {
        std::vector<float> allocationvars(cs->getAllocationNumVars());
        cs->getAllocationVars(&allocationvars[0]);
        out << YAML::Key << "allocationvars";
        out << YAML::Flow << YAML::Value << allocationvars;
    }

    const auto isDisplay = (cs->getReferenceSpaceType() == REFERENCE_SPACE_DISPLAY);
    ConstTransformRcPtr toref = cs->getTransform(COLORSPACE_DIR_TO_REFERENCE);
    if(toref)
    {
        out << YAML::Key << (isDisplay ? "to_display_reference" :
                                         (majorVersion < 2) ? "to_reference" :
                                                              "to_scene_reference") << YAML::Value;
        save(out, toref, majorVersion);
    }

    ConstTransformRcPtr fromref = cs->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
    if(fromref)
    {
        out << YAML::Key << (isDisplay ? "from_display_reference" :
                                         (majorVersion < 2) ? "from_reference" :
                                                              "from_scene_reference") << YAML::Value;
        save(out, fromref, majorVersion);
    }

    out << YAML::EndMap;
    out << YAML::Newline;
}

// Look

inline void load(const YAML::Node& node, LookRcPtr& look)
{
    if(node.Tag() != "Look")
        return;

    CheckDuplicates(node);

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if(key == "name")
        {
            std::string stringval;
            load(iter->second, stringval);
            look->setName(stringval.c_str());
        }
        else if(key == "process_space")
        {
            std::string stringval;
            load(iter->second, stringval);
            look->setProcessSpace(stringval.c_str());
        }
        else if(key == "transform")
        {
            TransformRcPtr val;
            load(iter->second, val);
            look->setTransform(val);
        }
        else if(key == "inverse_transform")
        {
            TransformRcPtr val;
            load(iter->second, val);
            look->setInverseTransform(val);
        }
        else if(key == "description")
        {
            std::string stringval;
            loadDescription(iter->second, stringval);
            look->setDescription(stringval.c_str());
        }
        else
        {
            LogUnknownKeyWarning(node, iter->first);
        }
    }
}

inline void save(YAML::Emitter& out, ConstLookRcPtr look, unsigned int majorVersion)
{
    out << YAML::VerbatimTag("Look");
    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << look->getName();
    out << YAML::Key << "process_space" << YAML::Value << look->getProcessSpace();
    saveDescription(out, look->getDescription());

    if(look->getTransform())
    {
        out << YAML::Key << "transform";
        out << YAML::Value;
        save(out, look->getTransform(), majorVersion);
    }

    if(look->getInverseTransform())
    {
        out << YAML::Key << "inverse_transform";
        out << YAML::Value;
        save(out, look->getInverseTransform(), majorVersion);
    }

    out << YAML::EndMap;
    out << YAML::Newline;
}

// View transform

inline ReferenceSpaceType peekViewTransformReferenceSpace(const YAML::Node & node)
{
    if (node.Type() != YAML::NodeType::Map)
    {
        throwError(node, "The '!<ViewTransform>' content needs to be a map.");
    }

    bool isScene = false;
    bool isDisplay = false;

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if (key == "to_scene_reference")
        {
            isScene = true;
        }
        else if (key == "to_display_reference")
        {
            isDisplay = true;
        }
        else if (key == "from_scene_reference")
        {
            isScene = true;
        }
        else if (key == "from_display_reference")
        {
            isDisplay = true;
        }
    }

    if (!isScene && !isDisplay)
    {
        throwError(node, "The '!<ViewTransform>' needs to refer to a transform.");
    }
    else if (isScene && isDisplay)
    {
        throwError(node, "The '!<ViewTransform>' cannot have both to/from_reference and "
                         "to/from_display_reference transforms.");
    }

    return isDisplay ? REFERENCE_SPACE_DISPLAY : REFERENCE_SPACE_SCENE;
}

inline void load(const YAML::Node & node, ViewTransformRcPtr & vt)
{
    if (node.Tag() != "ViewTransform")
    {
        return; // not a !<ViewTransform> tag
    }

    if (node.Type() != YAML::NodeType::Map)
    {
        std::ostringstream os;
        os << "The '!<ViewTransform>' content needs to be a map.";
        throwError(node, os.str());
    }
    
    CheckDuplicates(node);

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if (key == "name")
        {
            std::string stringval;
            load(iter->second, stringval);
            vt->setName(stringval.c_str());
        }
        else if (key == "description")
        {
            std::string stringval;
            loadDescription(iter->second, stringval);
            vt->setDescription(stringval.c_str());
        }
        else if (key == "family")
        {
            std::string stringval;
            load(iter->second, stringval);
            vt->setFamily(stringval.c_str());
        }
        else if (key == "categories")
        {
            StringUtils::StringVec categories;
            load(iter->second, categories);
            for (auto name : categories)
            {
                vt->addCategory(name.c_str());
            }
        }
        else if (key == "to_scene_reference")
        {
            TransformRcPtr val;
            load(iter->second, val);
            vt->setTransform(val, VIEWTRANSFORM_DIR_TO_REFERENCE);
        }
        else if (key == "to_display_reference")
        {
            TransformRcPtr val;
            load(iter->second, val);
            vt->setTransform(val, VIEWTRANSFORM_DIR_TO_REFERENCE);
        }
        else if (key == "from_scene_reference")
        {
            TransformRcPtr val;
            load(iter->second, val);
            vt->setTransform(val, VIEWTRANSFORM_DIR_FROM_REFERENCE);
        }
        else if (key == "from_display_reference")
        {
            TransformRcPtr val;
            load(iter->second, val);
            vt->setTransform(val, VIEWTRANSFORM_DIR_FROM_REFERENCE);
        }
        else
        {
            LogUnknownKeyWarning(node, iter->first);
        }
    }
}

inline void save(YAML::Emitter & out, ConstViewTransformRcPtr & vt, unsigned int majorVersion)
{
    out << YAML::VerbatimTag("ViewTransform");
    out << YAML::BeginMap;

    out << YAML::Key << "name" << YAML::Value << vt->getName();
    const char * family = vt->getFamily();
    if (family && *family)
    {
        out << YAML::Key << "family" << YAML::Value << family;
    }
    saveDescription(out, vt->getDescription());

    if (vt->getNumCategories() > 0)
    {
        StringUtils::StringVec categories;
        for (int idx = 0; idx < vt->getNumCategories(); ++idx)
        {
            categories.push_back(vt->getCategory(idx));
        }
        out << YAML::Key << "categories";
        out << YAML::Flow << YAML::Value << categories;
    }

    const auto isDisplay = (vt->getReferenceSpaceType() == REFERENCE_SPACE_DISPLAY);
    ConstTransformRcPtr toref = vt->getTransform(VIEWTRANSFORM_DIR_TO_REFERENCE);
    if (toref)
    {
        out << YAML::Key << (isDisplay ? "to_display_reference" : "to_scene_reference") << YAML::Value;
        save(out, toref, majorVersion);
    }

    ConstTransformRcPtr fromref = vt->getTransform(VIEWTRANSFORM_DIR_FROM_REFERENCE);
    if (fromref)
    {
        out << YAML::Key << (isDisplay ? "from_display_reference" : "from_scene_reference") << YAML::Value;
        save(out, fromref, majorVersion);
    }

    out << YAML::EndMap;
    out << YAML::Newline;
}

// NamedTransform

inline void load(const YAML::Node & node, NamedTransformRcPtr & nt)
{
    if (node.Tag() != "NamedTransform")
    {
        return; // not a !<NamedTransform> tag
    }

    if (node.Type() != YAML::NodeType::Map)
    {
        std::ostringstream os;
        os << "The '!<NamedTransform>' content needs to be a map.";
        throwError(node, os.str());
    }

    CheckDuplicates(node);

    std::string stringval;

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if (key == "name")
        {
            load(iter->second, stringval);
            nt->setName(stringval.c_str());
        }
        else if (key == "aliases")
        {
            StringUtils::StringVec aliases;
            load(iter->second, aliases);
            for (const auto & alias : aliases)
            {
                nt->addAlias(alias.c_str());
            }
        }
        else if (key == "description")
        {
            load(iter->second, stringval);
            nt->setDescription(stringval.c_str());
        }
        else if (key == "family")
        {
            load(iter->second, stringval);
            nt->setFamily(stringval.c_str());
        }
        else if (key == "categories")
        {
            StringUtils::StringVec categories;
            load(iter->second, categories);
            for (auto name : categories)
            {
                nt->addCategory(name.c_str());
            }
        }
        else if (key == "encoding")
        {
            load(iter->second, stringval);
            nt->setEncoding(stringval.c_str());
        }
        else if (key == "transform")
        {
            TransformRcPtr val;
            load(iter->second, val);
            nt->setTransform(val, TRANSFORM_DIR_FORWARD);
        }
        else if (key == "inverse_transform")
        {
            TransformRcPtr val;
            load(iter->second, val);
            nt->setTransform(val, TRANSFORM_DIR_INVERSE);
        }
        else
        {
            LogUnknownKeyWarning(node, iter->first);
        }
    }
}

inline void save(YAML::Emitter & out, ConstNamedTransformRcPtr & nt, unsigned int majorVersion)
{
    out << YAML::VerbatimTag("NamedTransform");
    out << YAML::BeginMap;

    out << YAML::Key << "name" << YAML::Value << nt->getName();

    const size_t numAliases = nt->getNumAliases();
    if (majorVersion >= 2 && numAliases)
    {
        out << YAML::Key << "aliases";
        StringUtils::StringVec aliases;
        for (size_t aidx = 0; aidx < numAliases; ++aidx)
        {
            aliases.push_back(nt->getAlias(aidx));
        }
        out << YAML::Flow << YAML::Value << aliases;
    }

    saveDescription(out, nt->getDescription());

    const char * family = nt->getFamily();
    if (family && *family)
    {
        out << YAML::Key << "family" << YAML::Value << family;
    }

    if (nt->getNumCategories() > 0)
    {
        StringUtils::StringVec categories;
        for (int idx = 0; idx < nt->getNumCategories(); ++idx)
        {
            categories.push_back(nt->getCategory(idx));
        }
        out << YAML::Key << "categories";
        out << YAML::Flow << YAML::Value << categories;
    }

    const char * encoding = nt->getEncoding();
    if (encoding && *encoding)
    {
        out << YAML::Key << "encoding" << YAML::Value << encoding;
    }

    ConstTransformRcPtr transform = nt->getTransform(TRANSFORM_DIR_FORWARD);
    if (transform)
    {
        out << YAML::Key << "transform" << YAML::Value;
        save(out, transform, majorVersion);
    }

    transform = nt->getTransform(TRANSFORM_DIR_INVERSE);
    if (transform)
    {
        out << YAML::Key << "inverse_transform" << YAML::Value;
        save(out, transform, majorVersion);
    }

    out << YAML::EndMap;
    out << YAML::Newline;
}

// File rules

inline void load(const YAML::Node & node, FileRulesRcPtr & fr, bool & defaultRuleFound)
{
    if (node.Tag() != "Rule")
        return;

    CheckDuplicates(node);

    std::string stringval;
    std::string name, colorspace, pattern, extension, regex;
    CustomKeysLoader::Type keyVals;

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if (key == FileRuleUtils::Name)
        {
            load(iter->second, stringval);
            name = stringval;
        }
        else if (key == FileRuleUtils::ColorSpace)
        {
            load(iter->second, stringval);
            colorspace = stringval;
        }
        else if (key == FileRuleUtils::Pattern)
        {
            load(iter->second, stringval);
            pattern = stringval;
        }
        else if (key == FileRuleUtils::Extension)
        {
            load(iter->second, stringval);
            extension = stringval;
        }
        else if (key == FileRuleUtils::Regex)
        {
            load(iter->second, stringval);
            regex = stringval;
        }
        else if (key == FileRuleUtils::CustomKey)
        {
            CustomKeysLoader kv;
            loadCustomKeys(iter->second, kv, "file_rules custom attribute");
            keyVals = kv.m_keyVals;
        }
        else
        {
            LogUnknownKeyWarning(node, iter->first);
        }
    }

    try
    {
        const auto pos = fr->getNumEntries() - 1;
        if (0==Platform::Strcasecmp(name.c_str(), FileRules::DefaultRuleName))
        {
            if (!regex.empty() || !pattern.empty() || !extension.empty())
            {
                std::ostringstream oss;
                oss << "'" << FileRules::DefaultRuleName << "' "
                    << "rule can't use pattern, extension or regex.";
                throw Exception(oss.str().c_str());
            }

            if (colorspace.empty())
            {
                std::ostringstream oss;
                oss << "'" << FileRules::DefaultRuleName << "' "
                    << "rule cannot have an empty color space name.";
                throw Exception(oss.str().c_str());
            }

            defaultRuleFound = true;
            fr->setColorSpace(pos, colorspace.c_str());
        }
        else if (0==Platform::Strcasecmp(name.c_str(), FileRules::FilePathSearchRuleName))
        {
            if (!regex.empty() || !pattern.empty() || !extension.empty())
            {
                std::ostringstream oss;
                oss << "'" << FileRules::FilePathSearchRuleName << "' "
                    << "rule can't use pattern, extension or regex.";
                throw Exception(oss.str().c_str());
            }
            fr->insertPathSearchRule(pos);
        }
        else
        {
            if (!regex.empty() && (!pattern.empty() || !extension.empty()))
            {
                std::ostringstream oss;
                oss << "File rule '" << name << "' can't use regex '" << regex << "' and "
                    << "pattern & extension '" << pattern << "' '" << extension << "'.";
                throw Exception(oss.str().c_str());
            }
            
            if (colorspace.empty())
            {
                std::ostringstream oss;
                oss << "File rule '" << name << "' cannot have an empty color space name.";
                throw Exception(oss.str().c_str());
            }

            if (regex.empty())
            {
                fr->insertRule(pos, name.c_str(), colorspace.c_str(),
                               pattern.c_str(), extension.c_str());
            }
            else
            {
                fr->insertRule(pos, name.c_str(), colorspace.c_str(), regex.c_str());
            }
        }

        for (const auto& keyval : keyVals)
        {
            fr->setCustomKey(
                pos, 
                keyval.first.as<std::string>().c_str(),
                keyval.second.as<std::string>().c_str());
        }

    }
    catch (Exception & ex)
    {
        std::ostringstream os;
        os << "File rules: " << ex.what();
        throwError(node, os.str().c_str());
    }
}

inline void save(YAML::Emitter & out, ConstFileRulesRcPtr & fr, size_t position)
{
    out << YAML::VerbatimTag("Rule");
    out << YAML::Flow;
    out << YAML::BeginMap;
    out << YAML::Key << FileRuleUtils::Name << YAML::Value << fr->getName(position);
    const char * cs{ fr->getColorSpace(position) };
    if (cs && *cs)
    {
        out << YAML::Key << FileRuleUtils::ColorSpace << YAML::Value << std::string(cs);
    }
    const char * regex{ fr->getRegex(position) };
    if (regex && *regex)
    {
        out << YAML::Key << FileRuleUtils::Regex << YAML::Value << std::string(regex);
    }
    const char * pattern{ fr->getPattern(position) };
    if (pattern && *pattern)
    {
        out << YAML::Key << FileRuleUtils::Pattern << YAML::Value << std::string(pattern);
    }
    const char * extension{ fr->getExtension(position) };
    if (extension && *extension)
    {
        out << YAML::Key << FileRuleUtils::Extension << YAML::Value << std::string(extension);
    }
    const auto numKeys = fr->getNumCustomKeys(position);
    if (numKeys)
    {
        out << YAML::Key << FileRuleUtils::CustomKey;
        out << YAML::Value;
        out << YAML::BeginMap;

        for (size_t i = 0; i < numKeys; ++i)
        {
            out << YAML::Key << fr->getCustomKeyName(position, i)
                << YAML::Value << fr->getCustomKeyValue(position, i);
        }
        out << YAML::EndMap;
    }
    out << YAML::EndMap;
}

// Viewing rules

inline void load(const YAML::Node & node, ViewingRulesRcPtr & vr)
{
    if (node.Tag() != "Rule")
        return;

    std::string stringval;
    std::string name;
    StringUtils::StringVec colorspaces, encodings;
    CustomKeysLoader::Type keyVals;

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if (key == ViewingRuleUtils::Name)
        {
            load(iter->second, stringval);
            name = stringval;
        }
        else if (key == ViewingRuleUtils::ColorSpaces)
        {
            if (iter->second.Type() == YAML::NodeType::Sequence)
            {
                load(iter->second, colorspaces);
            }
            else
            {
                // If a single value is supplied...
                load(iter->second, stringval);
                colorspaces.emplace_back(stringval);
            }
        }
        else if (key == ViewingRuleUtils::Encodings)
        {
            if (iter->second.Type() == YAML::NodeType::Sequence)
            {
                load(iter->second, encodings);
            }
            else
            {
                // If a single value is supplied...
                load(iter->second, stringval);
                encodings.emplace_back(stringval);
            }
        }
        else if (key == ViewingRuleUtils::CustomKey)
        {
            CustomKeysLoader kv;
            loadCustomKeys(iter->second, kv, "viewing_rules custom attribute");
            keyVals = kv.m_keyVals;
        }
        else
        {
            LogUnknownKeyWarning(node, iter->first);
        }
    }

    try
    {
        const auto pos = vr->getNumEntries();
        vr->insertRule(pos, name.c_str());

        for (const auto & cs : colorspaces)
        {
            vr->addColorSpace(pos, cs.c_str());
        }
        for (const auto & is : encodings)
        {
            vr->addEncoding(pos, is.c_str());
        }

        for (const auto& keyval : keyVals) 
        {
            vr->setCustomKey(
                pos, 
                keyval.first.as<std::string>().c_str(),
                keyval.second.as<std::string>().c_str());
        }
    }
    catch (Exception & ex)
    {
        std::ostringstream os;
        os << "Viewing rules: " << ex.what();
        throwError(node, os.str().c_str());
    }
}

inline void save(YAML::Emitter & out, ConstViewingRulesRcPtr & vr, size_t position)
{
    out << YAML::VerbatimTag("Rule");
    out << YAML::Flow;
    out << YAML::BeginMap;
    out << YAML::Key << ViewingRuleUtils::Name << YAML::Value << vr->getName(position);
    const size_t numcs = vr->getNumColorSpaces(position);
    if (numcs == 1)
    {
        out << YAML::Key << ViewingRuleUtils::ColorSpaces;
        out << YAML::Value << vr->getColorSpace(position, 0);
    }
    else if (numcs > 1)
    {
        StringUtils::StringVec colorspaces;
        for (size_t i = 0; i < numcs; ++i)
        {
            colorspaces.emplace_back(vr->getColorSpace(position, i));
        }
        out << YAML::Key << ViewingRuleUtils::ColorSpaces;
        out << YAML::Value << YAML::Flow << colorspaces;
    }
    const size_t numenc = vr->getNumEncodings(position);
    if (numenc == 1)
    {
        out << YAML::Key << ViewingRuleUtils::Encodings;
        out << YAML::Value << vr->getEncoding(position, 0);
    }
    else if (numenc > 1)
    {
        StringUtils::StringVec encodings;
        for (size_t i = 0; i < numenc; ++i)
        {
            encodings.emplace_back(vr->getEncoding(position, i));
        }
        out << YAML::Key << ViewingRuleUtils::Encodings;
        out << YAML::Value << YAML::Flow << encodings;
    }
    const auto numKeys = vr->getNumCustomKeys(position);
    if (numKeys)
    {
        out << YAML::Key << ViewingRuleUtils::CustomKey;
        out << YAML::Value;
        out << YAML::BeginMap;

        for (size_t i = 0; i < numKeys; ++i)
        {
            out << YAML::Key << vr->getCustomKeyName(position, i)
                << YAML::Value << vr->getCustomKeyValue(position, i);
        }
        out << YAML::EndMap;
    }
    out << YAML::EndMap;
}


// Config

inline void load(const YAML::Node& node, ConfigRcPtr & config, const char* filename)
{

    // check profile version
    int profile_major_version = 0;
    int profile_minor_version = 0;

    bool faulty_version = !node["ocio_profile_version"].IsDefined();

    std::string version;
    std::vector< std::string > results;

    if(!faulty_version)
    {
        load(node["ocio_profile_version"], version);

        results = StringUtils::Split(version, '.');

        if(results.size()==1)
        {
            profile_major_version = std::stoi(results[0].c_str());
            profile_minor_version = 0;
        }
        else if(results.size()==2)
        {
            profile_major_version = std::stoi(results[0].c_str());
            profile_minor_version = std::stoi(results[1].c_str());
        }
        else
        {
            faulty_version = true;
        }
    }

    if(faulty_version)
    {
        std::ostringstream os;

        os << "The specified OCIO configuration file "
            << ((filename && *filename) ? filename : "<null>")
            << " does not appear to have a valid version "
            << (version.empty() ? "<null>" : version)
            << ".";

        throwError(node, os.str());
    }

    try
    {
        config->setVersion((unsigned int)profile_major_version,
                           (unsigned int)profile_minor_version);
    }
    catch(Exception & ex)
    {
        std::ostringstream os;
        os << "This .ocio config ";
        if(filename && *filename)
        {
            os << " '" << filename << "' ";
        }

        os << "is version " << profile_major_version 
            << "." << profile_minor_version
            << ". ";

        os << "This version of the OpenColorIO library (" << GetVersion() << ") ";
        os << "is not able to load that config version.";
        os << std::endl << ex.what();

        throw Exception(os.str().c_str());
    }

    bool fileRulesFound = false;
    bool defaultFileRuleFound = false;
    auto fileRules = config->getFileRules()->createEditableCopy();

    CheckDuplicates(node);

    std::string stringval;
    bool boolval = false;
    EnvironmentMode mode = ENV_ENVIRONMENT_LOAD_ALL;

    for (Iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();

        if (iter->second.IsNull() || !iter->second.IsDefined()) continue;

        if(key == "ocio_profile_version") { } // Already handled above.
        else if(key == "environment")
        {
            mode = ENV_ENVIRONMENT_LOAD_PREDEFINED;
            if(iter->second.Type() != YAML::NodeType::Map)
            {
                throwValueError(node.Tag(), iter->first, 
                                "The value type of key 'environment' needs to be a map.");
            }
            for (Iterator it = iter->second.begin(); it != iter->second.end(); ++it)
            {
                const std::string & k = it->first.as<std::string>();
                const std::string & v = it->second.as<std::string>();
                config->addEnvironmentVar(k.c_str(), v.c_str());
            }
        }
        else if(key == "search_path" || key == "resource_path")
        {
            if (iter->second.size() == 0)
            {
                load(iter->second, stringval);
                config->setSearchPath(stringval.c_str());
            }
            else
            {
                StringUtils::StringVec paths;
                load(iter->second, paths);
                for (const auto & path : paths)
                {
                    config->addSearchPath(path.c_str());
                }
            }
        }
        else if(key == "strictparsing")
        {
            load(iter->second, boolval);
            config->setStrictParsingEnabled(boolval);
        }
        else if (key == "name")
        {
            loadDescription(iter->second, stringval);
            config->setName(stringval.c_str());
        }
        else if (key=="family_separator")
        {
            // Check that the key is not present in a v1 config (checkVersionConsistency is not
            // able to detect this).
            if (config->getMajorVersion() < 2)
            {
                throwError(iter->first, "Config v1 can't have 'family_separator'.");
            }

            load(iter->second, stringval);
            if(stringval.size()!=1)
            {
                std::ostringstream os;
                os << "'family_separator' value must be a single character.";
                os << " Found '" << stringval << "'.";
                throwValueError(node.Tag(), iter->first, os.str());
            }
            config->setFamilySeparator(stringval[0]);
        }
        else if(key == "description")
        {
            loadDescription(iter->second, stringval);
            config->setDescription(stringval.c_str());
        }
        else if(key == "luma")
        {
            std::vector<double> val;
            load(iter->second, val);
            if(val.size() != 3)
            {
                std::ostringstream os;
                os << "'luma' values must be 3 ";
                os << "floats. Found '" << val.size() << "'.";
                throwValueError(node.Tag(), iter->first, os.str());
            }
            config->setDefaultLumaCoefs(&val[0]);
        }
        else if(key == "roles")
        {
            if(iter->second.Type() != YAML::NodeType::Map)
            {
                throwValueError(node.Tag(), iter->first,
                                "The value type of the key 'roles' needs to be a map.");
            }
            for (Iterator it = iter->second.begin(); it != iter->second.end(); ++it)
            {
                const std::string & k = it->first.as<std::string>();
                const std::string & v = it->second.as<std::string>();
                config->setRole(k.c_str(), v.c_str());
            }
        }
        else if (key == "file_rules")
        {
            // Check that the key is not present in a v1 config (checkVersionConsistency is not
            // able to detect this).
            if (config->getMajorVersion() < 2)
            {
                throwError(iter->first, "Config v1 can't use 'file_rules'");
            }

            if (iter->second.Type() != YAML::NodeType::Sequence)
            {
                throwError(iter->second, "The 'file_rules' field needs to be a (- !<Rule>) list.");
            }

            for (std::size_t i = 0; i < iter->second.size(); i++)
            {
                const YAML::Node & val = iter->second[i];

                if (val.Tag() == "Rule")
                {
                    if (defaultFileRuleFound)
                    {
                        throwError(iter->second, "The 'file_rules' Default rule has to be "
                                           "the last rule.");
                    }
                    load(val, fileRules, defaultFileRuleFound);
                }
                else
                {
                    std::ostringstream os;
                    os << "Unknown element found in file_rules:";
                    os << val.Tag() << ". Only Rule(s) are currently handled.";
                    LogWarning(os.str());
                }
            }

            if (!defaultFileRuleFound)
            {
                throwError(iter->first, "The 'file_rules' does not contain a Default <Rule>.");
            }
            fileRulesFound = true;
        }
        else if (key == "viewing_rules")
        {
            if (iter->second.Type() != YAML::NodeType::Sequence)
            {
                throwError(iter->second, "The 'viewing_rules' field needs to be a (- !<Rule>) list.");
            }

            auto viewingRules = ViewingRules::Create();

            for (std::size_t i = 0; i < iter->second.size(); i++)
            {
                const YAML::Node & val = iter->second[i];

                if (val.Tag() == "Rule")
                {
                    load(val, viewingRules);
                }
                else
                {
                    std::ostringstream os;
                    os << "Unknown element found in viewing_rules:";
                    os << val.Tag() << ". Only Rule(s) are currently handled.";
                    LogWarning(os.str());
                }
            }

            config->setViewingRules(viewingRules);
        }
        else if (key == "shared_views")
        {
            if (iter->second.Type() != YAML::NodeType::Sequence)
            {
                throwValueError(node.Tag(), iter->first, "The view list is a sequence.");
            }

            for (std::size_t i = 0; i < iter->second.size(); i++)
            {
                const YAML::Node & val = iter->second[i];

                View view;
                load(val, view);
                config->addSharedView(view.m_name.c_str(),
                                      view.m_viewTransform.c_str(), view.m_colorspace.c_str(),
                                      view.m_looks.c_str(), view.m_rule.c_str(),
                                      view.m_description.c_str());
            }
        }
        else if (key == "displays")
        {
            if(iter->second.Type() != YAML::NodeType::Map)
            {
                throwValueError(node.Tag(), iter->first,
                                "The value type of the key 'displays' needs to be a map.");
            }
            for (Iterator it = iter->second.begin(); it != iter->second.end(); ++it)
            {
                const std::string & display = it->first.as<std::string>();

                if (it->second.Type() != YAML::NodeType::Sequence)
                {
                    throwValueError(node.Tag(), iter->first, "The view list is a sequence.");
                }

                for (std::size_t i = 0; i < it->second.size(); i++)
                {
                    const YAML::Node & node = it->second[i];

                    if (node.Tag() == "View")
                    {
                        View view;
                        load(node, view);
                        config->addDisplayView(display.c_str(), view.m_name.c_str(),
                                               view.m_viewTransform.c_str(), view.m_colorspace.c_str(),
                                               view.m_looks.c_str(), view.m_rule.c_str(),
                                               view.m_description.c_str());
                    }
                    else if (node.Tag() == "Views")
                    {
                        StringUtils::StringVec views;
                        load(node, views);
                        for (const auto & sharedView : views)
                        {
                            config->addDisplaySharedView(display.c_str(), sharedView.c_str());
                        }
                    }
                }
            }
        }
        else if (key == "virtual_display")
        {
            if (iter->second.Type() != YAML::NodeType::Sequence)
            {
                throwValueError(node.Tag(), iter->first, "The view list is a sequence.");
            }

            for (std::size_t i = 0; i < iter->second.size(); i++)
            {
                const YAML::Node & val = iter->second[i];

                if (val.Tag() == "View")
                {
                    View view;
                    load(val, view);
                    config->addVirtualDisplayView(view.m_name.c_str(),
                                                  view.m_viewTransform.c_str(),
                                                  view.m_colorspace.c_str(),
                                                  view.m_looks.c_str(),
                                                  view.m_rule.c_str(),
                                                  view.m_description.c_str());
                }
                else if (val.Tag() == "Views")
                {
                    StringUtils::StringVec views;
                    load(val, views);
                    for (const auto & sharedView : views)
                    {
                        config->addVirtualDisplaySharedView(sharedView.c_str());
                    }
                }
                else
                {
                    std::ostringstream os;
                    os << "Unknown element found in virtual_display:";
                    os << val.Tag() << ".";
                    LogWarning(os.str());
                }
            }
        }
        else if(key == "active_displays")
        {
            StringUtils::StringVec display;
            load(iter->second, display);
            std::string displays = JoinStringEnvStyle(display);
            config->setActiveDisplays(displays.c_str());
        }
        else if(key == "active_views")
        {
            StringUtils::StringVec view;
            load(iter->second, view);
            std::string views = JoinStringEnvStyle(view);
            config->setActiveViews(views.c_str());
        }
        else if(key == "inactive_colorspaces")
        {
            StringUtils::StringVec inactiveCSs;
            load(iter->second, inactiveCSs);
            const std::string inactivecCSsStr = JoinStringEnvStyle(inactiveCSs);
            config->setInactiveColorSpaces(inactivecCSsStr.c_str());
        }
        else if(key == "colorspaces")
        {
            if (iter->second.Type() != YAML::NodeType::Sequence)
            {
                throwError(iter->second, "'colorspaces' field needs to be a (- !<ColorSpace>) list.");
            }

            for (std::size_t i = 0; i < iter->second.size(); i++)
            {
                const YAML::Node & val = iter->second[i];

                if(val.Tag() == "ColorSpace")
                {
                    ColorSpaceRcPtr cs = ColorSpace::Create(REFERENCE_SPACE_SCENE);
                    load(val, cs, config->getMajorVersion());
                    for(int ii = 0; ii < config->getNumColorSpaces(); ++ii)
                    {
                        if(strcmp(config->getColorSpaceNameByIndex(ii), cs->getName()) == 0)
                        {
                            std::ostringstream os;
                            os << "Colorspace with name '" << cs->getName() << "' already defined.";
                            throwError(iter->second, os.str());
                        }
                    }
                    config->addColorSpace(cs);
                }
                else
                {
                    std::ostringstream os;
                    os << "Unknown element found in colorspaces:";
                    os << val.Tag() << ". Only ColorSpace(s)";
                    os << " currently handled.";
                    LogWarning(os.str());
                }
            }
        }
        else if (key == "display_colorspaces")
        {
            if (iter->second.Type() != YAML::NodeType::Sequence)
            {
                throwError(iter->second, "'display_colorspaces' field needs to be a (- !<ColorSpace>) list.");
            }

            for (std::size_t i = 0; i < iter->second.size(); i++)
            {
                const YAML::Node & val = iter->second[i];

                if (val.Tag() == "ColorSpace")
                {
                    ColorSpaceRcPtr cs = ColorSpace::Create(REFERENCE_SPACE_DISPLAY);
                    load(val, cs, config->getMajorVersion());
                    for (int ii = 0; ii < config->getNumColorSpaces(); ++ii)
                    {
                        if (strcmp(config->getColorSpaceNameByIndex(ii), cs->getName()) == 0)
                        {
                            std::ostringstream os;
                            os << "Colorspace with name '" << cs->getName() << "' already defined.";
                            throwError(iter->second, os.str());
                        }
                    }
                    config->addColorSpace(cs);
                }
                else
                {
                    std::ostringstream os;
                    os << "Unknown element found in colorspaces:";
                    os << val.Tag() << ". Only ColorSpace(s)";
                    os << " currently handled.";
                    LogWarning(os.str());
                }
            }
        }
        else if (key == "looks")
        {
            if (iter->second.Type() != YAML::NodeType::Sequence)
            {
                throwError(iter->second, "'looks' field needs to be a (- !<Look>) list.");
            }

            for (std::size_t i = 0; i < iter->second.size(); i++)
            {
                const YAML::Node & val = iter->second[i];

                if(val.Tag() == "Look")
                {
                    LookRcPtr look = Look::Create();
                    load(val, look);
                    config->addLook(look);
                }
                else
                {
                    std::ostringstream os;
                    os << "Unknown element found in looks:";
                    os << val.Tag() << ". Only Look(s)";
                    os << " currently handled.";
                    LogWarning(os.str());
                }
            }
        }
        else if (key == "view_transforms")
        {
            if (iter->second.Type() != YAML::NodeType::Sequence)
            {
                throwError(iter->second, "'view_transforms' field needs to be a (- !<ViewTransform>) list.");
            }

            for (std::size_t i = 0; i < iter->second.size(); i++)
            {
                const YAML::Node & val = iter->second[i];

                if (val.Tag() == "ViewTransform")
                {
                    ReferenceSpaceType rst = peekViewTransformReferenceSpace(val);
                    ViewTransformRcPtr vt = ViewTransform::Create(rst);
                    load(val, vt);
                    config->addViewTransform(vt);
                }
                else
                {
                    std::ostringstream os;
                    os << "Unknown element found in view_transforms:";
                    os << val.Tag() << ". Only ViewTransform(s)";
                    os << " currently handled.";
                    LogWarning(os.str());
                }
            }
        }
        else if (key == "default_view_transform")
        {
            load(iter->second, stringval);
            config->setDefaultViewTransformName(stringval.c_str());
        }
        else if (key == "named_transforms")
        {
            if (iter->second.Type() != YAML::NodeType::Sequence)
            {
                throwError(iter->second, 
                           "'named_transforms' field needs to be a (- !<NamedTransform>) list.");
            }

            for (std::size_t i = 0; i < iter->second.size(); i++)
            {
                const YAML::Node & val = iter->second[i];

                if (val.Tag() == "NamedTransform")
                {
                    auto nt = NamedTransform::Create();
                    load(val, nt);
                    if (nt->getName())
                    {
                        // Test that the name transform definitions are unique.
                        if (config->getNamedTransform(nt->getName()))
                        {
                            std::ostringstream oss;
                            oss << "NamedTransform: There is already one NamedTransform named: '";
                            oss << std::string(nt->getName()) << "'.";
                            throw Exception(oss.str().c_str());
                        }
                    }
                    // Will throw if name is empty.
                    config->addNamedTransform(nt);
                }
                else
                {
                    std::ostringstream os;
                    os << "Unknown element found in named_transforms:";
                    os << val.Tag() << ". Only NamedTransform(s)";
                    os << " currently handled.";
                    LogWarning(os.str());
                }
            }
        }
        else
        {
            LogUnknownKeyWarning("profile", iter->first);
        }
    }

    // Do not set the working dir when the filename is empty or contains the special string 
    // "from Archive/ConfigIOProxy".
    if (filename && filename[0] && 
        Platform::Strcasecmp(filename, "from Archive/ConfigIOProxy") != 0)
    {
        std::string realfilename = AbsPath(filename);
        std::string configrootdir = pystring::os::path::dirname(realfilename);
        config->setWorkingDir(configrootdir.c_str());
    }

    if (!fileRulesFound)
    {
        if (config->getMajorVersion() >= 2)
        {
            if (!config->hasRole(ROLE_DEFAULT))
            {
                // Note that no validation of the default color space is done (e.g. to check that
                // it exists in the config) in order to enable loading configs that are only
                // partially complete. The caller may use config->validate() after, if desired.
                throwError(node, "The config must contain either a Default file rule or "
                                 "the 'default' role.");
            }
        }        
        else
        {
            // In order to use Config::getColorSpaceFromFilepath() method for any version of
            // config instance, the method updates the in-memory file rules created by a v1 config
            // to have valid file rules and most importantly, to mimic
            // Config::parseColorSpaceFromString() which is now deprecated since v2.
            UpdateFileRulesFromV1ToV2(*config.get(), fileRules);

            config->setFileRules(fileRules);
        }
    }
    else
    {
        // If default role is also defined.
        auto defaultCS = config->getColorSpace(ROLE_DEFAULT);
        if (defaultCS)
        {
            const auto defaultRule = fileRules->getNumEntries() - 1;
            const std::string defaultRuleCS{ fileRules->getColorSpace(defaultRule) };
            if (defaultRuleCS != ROLE_DEFAULT)
            {
                if (defaultRuleCS != defaultCS->getName())
                {
                    std::ostringstream oss;
                    oss << "file_rules: defines a default rule using color-space '"
                        << defaultRuleCS << "' that does not match the default role '"
                        << std::string(defaultCS->getName()) << "'.";
                    LogWarning(oss.str());
                }
            }
        }
        config->setFileRules(fileRules);
    }

    config->setEnvironmentMode(mode);
    config->loadEnvironment();

    if (mode == ENV_ENVIRONMENT_LOAD_ALL)
    {
        std::ostringstream os;
        os << "This .ocio config ";
        if(filename && *filename)
        {
            os << " '" << filename << "' ";
        }
        os << "has no environment section defined. The default behaviour is to ";
        os << "load all environment variables (" << config->getNumEnvironmentVars() << ")";
        os << ", which reduces the efficiency of OCIO's caching. Consider ";
        os << "predefining the environment variables used.";

        LogDebug(os.str());
    }
}

inline void save(YAML::Emitter & out, const Config & config)
{
    std::stringstream ss;
    const unsigned configMajorVersion = config.getMajorVersion();
    const unsigned configMinorVersion = config.getMinorVersion();

    ss << configMajorVersion;
    if(configMinorVersion != 0)
    {
        ss << "." << configMinorVersion;
    }

    out << YAML::Block;
    out << YAML::BeginMap;
    out << YAML::Key << "ocio_profile_version" << YAML::Value << ss.str();
    out << YAML::Newline;
    out << YAML::Newline;

    if (configMajorVersion >= 2 || config.getNumEnvironmentVars() > 0)
    {
        // For v2 configs, write the environment section, even if empty.
        out << YAML::Key << "environment";
        out << YAML::Value << YAML::BeginMap;
        for(int i = 0; i < config.getNumEnvironmentVars(); ++i)
        {   
            const char* name = config.getEnvironmentVarNameByIndex(i);
            out << YAML::Key << name;
            out << YAML::Value << config.getEnvironmentVarDefault(name);
        }
        out << YAML::EndMap;
        out << YAML::Newline;
    }

    if (configMajorVersion < 2)
    {
        // Save search paths as a single string.
        out << YAML::Key << "search_path" << YAML::Value << config.getSearchPath();
    }
    else
    {
        StringUtils::StringVec searchPaths;
        const int numSP = config.getNumSearchPaths();
        for (int i = 0; i < config.getNumSearchPaths(); ++i)
        {
            searchPaths.emplace_back(config.getSearchPath(i));
        }

        if (numSP == 0)
        {
            out << YAML::Key << "search_path" << YAML::Value << "";
        }
        else if (numSP == 1)
        {
            out << YAML::Key << "search_path" << YAML::Value << searchPaths[0];
        }
        else
        {
            out << YAML::Key << "search_path" << YAML::Value << searchPaths;
        }
    }
    out << YAML::Key << "strictparsing" << YAML::Value << config.isStrictParsingEnabled();

    if (configMajorVersion >= 2)
    {
        const char familySeparator = config.getFamilySeparator();
        if (familySeparator != '/')
        {
            out << YAML::Key << "family_separator" << YAML::Value << familySeparator;
        }
    }

    std::vector<double> luma(3, 0.f);
    config.getDefaultLumaCoefs(&luma[0]);
    out << YAML::Key << "luma" << YAML::Value << YAML::Flow << luma;

    if (configMajorVersion >= 2)
    {
        const std::string name{ config.getName() };
        if (!name.empty())
        {
            out << YAML::Key << "name" << YAML::Value << name;
        }
    }
    saveDescription(out, config.getDescription());

    // Roles
    out << YAML::Newline;
    out << YAML::Newline;
    out << YAML::Key << "roles";
    out << YAML::Value << YAML::BeginMap;
    for(int i = 0; i < config.getNumRoles(); ++i)
    {
        const char* role = config.getRoleName(i);
        if(role && *role)
        {
            // Note that no validation of the name strings is done here (e.g. to check that
            // they exist in the config) in order to enable serializing configs that are only
            // partially complete. The caller may use config->validate() first, if desired.
            out << YAML::Key << role;
            out << YAML::Value << config.getRoleColorSpace(i);
        }
    }
    out << YAML::EndMap;
    out << YAML::Newline;

    // File rules
    if (configMajorVersion >= 2)
    {
        auto rules = config.getFileRules();
        out << YAML::Newline;
        out << YAML::Key << "file_rules";
        out << YAML::Value << YAML::BeginSeq;
        for (size_t i = 0; i < rules->getNumEntries(); ++i)
        {
            save(out, rules, i);
        }
        out << YAML::EndSeq;
        out << YAML::Newline;
    }

    // Viewing rules
    if (configMajorVersion >= 2)
    {
        auto rules = config.getViewingRules();
        const auto numRules = rules->getNumEntries();
        if (numRules)
        {
            out << YAML::Newline;
            out << YAML::Key << "viewing_rules";
            out << YAML::Value << YAML::BeginSeq;
            for (size_t i = 0; i < numRules; ++i)
            {
                save(out, rules, i);
            }
            out << YAML::EndSeq;
            out << YAML::Newline;
        }
    }

    // Shared views
    const int numSharedViews = config.getNumViews(VIEW_SHARED, nullptr);
    if (numSharedViews)
    {
        out << YAML::Newline;
        out << YAML::Key << "shared_views";
        out << YAML::Value << YAML::BeginSeq;
        for (int v = 0; v < numSharedViews; ++v)
        {
            const char * name = config.getView(VIEW_SHARED, nullptr, v);
            const View dview{ name,
                              config.getDisplayViewTransformName(nullptr, name),
                              config.getDisplayViewColorSpaceName(nullptr, name),
                              config.getDisplayViewLooks(nullptr, name),
                              config.getDisplayViewRule(nullptr, name),
                              config.getDisplayViewDescription(nullptr, name) };
            save(out, dview);
        }
        out << YAML::EndSeq;
        out << YAML::Newline;
    }

    // Displays.
    out << YAML::Newline;
    out << YAML::Key << "displays";
    out << YAML::Value << YAML::BeginMap;
    // All displays are saved (not just active ones).
    for(int i = 0; i < config.getNumDisplaysAll(); ++i)
    {
        // Do not save displays instantiated from a virtual display.
        if (!config.isDisplayTemporary(i))
        {
            const char * display = config.getDisplayAll(i);

            out << YAML::Key << display;
            out << YAML::Value << YAML::BeginSeq;
            for(int v = 0; v < config.getNumViews(VIEW_DISPLAY_DEFINED, display); ++v)
            {
                const char * name = config.getView(VIEW_DISPLAY_DEFINED, display, v);
                const View dview{ name,
                                  config.getDisplayViewTransformName(display, name),
                                  config.getDisplayViewColorSpaceName(display, name),
                                  config.getDisplayViewLooks(display, name),
                                  config.getDisplayViewRule(display, name),
                                  config.getDisplayViewDescription(display, name) };
                save(out, dview);
            }

            StringUtils::StringVec sharedViews;
            for (int v = 0; v < config.getNumViews(VIEW_SHARED, display); ++v)
            {
                sharedViews.push_back(config.getView(VIEW_SHARED, display, v));
            }
            if (!sharedViews.empty())
            {
                out << YAML::VerbatimTag("Views");
                out << YAML::Flow << sharedViews;
            }
            out << YAML::EndSeq;
        }
    }
    out << YAML::EndMap;

    // Virtual Display.
    const int numVirtualDisplayViews
        = config.getVirtualDisplayNumViews(VIEW_DISPLAY_DEFINED) 
        + config.getVirtualDisplayNumViews(VIEW_SHARED);

    if (configMajorVersion >= 2 && numVirtualDisplayViews > 0)
    {
        out << YAML::Newline;
        out << YAML::Newline;
        out << YAML::Key << "virtual_display";
        out << YAML::Value << YAML::BeginSeq;
    
        for(int idx = 0; idx < config.getVirtualDisplayNumViews(VIEW_DISPLAY_DEFINED); ++idx)
        {
            const char * viewName = config.getVirtualDisplayView(VIEW_DISPLAY_DEFINED, idx);
            const View view{ viewName,
                             config.getVirtualDisplayViewTransformName(viewName),
                             config.getVirtualDisplayViewColorSpaceName(viewName),
                             config.getVirtualDisplayViewLooks(viewName),
                             config.getVirtualDisplayViewRule(viewName),
                             config.getVirtualDisplayViewDescription(viewName) };
            save(out, view);
        }
    
        StringUtils::StringVec sharedViews;
        for (int idx = 0; idx < config.getVirtualDisplayNumViews(VIEW_SHARED); ++idx)
        {
            sharedViews.push_back(config.getVirtualDisplayView(VIEW_SHARED, idx));
        }
        if (!sharedViews.empty())
        {
            out << YAML::VerbatimTag("Views");
            out << YAML::Flow << sharedViews;
        }
    
        out << YAML::EndSeq;
    }

    out << YAML::Newline;
    out << YAML::Newline;
    out << YAML::Key << "active_displays";
    StringUtils::StringVec active_displays;
    int nDisplays = config.getNumActiveDisplays();
    active_displays.reserve( nDisplays );
    for (int i = 0; i < nDisplays; i++)
    {
        active_displays.push_back(config.getActiveDisplay(i));
    }

    // The YAML library will wrap names that use a comma in quotes.
    out << YAML::Value << YAML::Flow << active_displays;

    out << YAML::Key << "active_views";
    StringUtils::StringVec active_views;
    int nViews = config.getNumActiveViews();
    active_views.reserve( nViews );
    for (int i = 0; i < nViews; i++)
    {
        active_views.push_back(config.getActiveView(i));
    }
    
    // The YAML library will wrap names that use a comma in quotes.
    out << YAML::Value << YAML::Flow << active_views;

    const std::string inactiveCSs = config.getInactiveColorSpaces();
    if (!inactiveCSs.empty())
    {
        const StringUtils::StringVec inactive_colorspaces{ SplitStringEnvStyle(inactiveCSs) };
        out << YAML::Key << "inactive_colorspaces";
        out << YAML::Value << YAML::Flow << inactive_colorspaces;
    }

    out << YAML::Newline;

    // Looks
    if(config.getNumLooks() > 0)
    {
        out << YAML::Newline;
        out << YAML::Key << "looks";
        out << YAML::Value << YAML::BeginSeq;
        for(int i = 0; i < config.getNumLooks(); ++i)
        {
            const char* name = config.getLookNameByIndex(i);
            save(out, config.getLook(name), configMajorVersion);
        }
        out << YAML::EndSeq;
        out << YAML::Newline;
    }

    // View transforms.
    const std::string defVT{ config.getDefaultViewTransformName() };
    if (!defVT.empty())
    {
        out << YAML::Newline;
        out << YAML::Key << "default_view_transform" << YAML::Value << defVT;
        out << YAML::Newline;
    }
    const int numVT = config.getNumViewTransforms();
    if (numVT > 0)
    {
        out << YAML::Newline;
        out << YAML::Key << "view_transforms";
        out << YAML::Value << YAML::BeginSeq;
        for (int i = 0; i < numVT; ++i)
        {
            auto name = config.getViewTransformNameByIndex(i);
            auto vt = config.getViewTransform(name);
            save(out, vt, configMajorVersion);
        }
        out << YAML::EndSeq;
    }

    std::vector<ConstColorSpaceRcPtr> sceneCS;
    std::vector<ConstColorSpaceRcPtr> displayCS;
    for (int i = 0; i < config.getNumColorSpaces(SEARCH_REFERENCE_SPACE_ALL, COLORSPACE_ALL); ++i)
    {
        const char * name
            = config.getColorSpaceNameByIndex(SEARCH_REFERENCE_SPACE_ALL, COLORSPACE_ALL, i);

        auto cs = config.getColorSpace(name);
        if (cs->getReferenceSpaceType() == REFERENCE_SPACE_DISPLAY)
        {
            // Display color spaces instantiated from a virtual display must not be saved.
            // Check them using their name as they have the same name as the display.

            const int idx = config.getDisplayAllByName(name);
            if (idx==-1 || !config.isDisplayTemporary(idx))
            {
                displayCS.push_back(cs);
            }
        }
        else
        {
            sceneCS.push_back(cs);
        }
    }

    // Display ColorSpaces
    if (!displayCS.empty())
    {
        out << YAML::Newline;
        out << YAML::Key << "display_colorspaces";
        out << YAML::Value << YAML::BeginSeq;
        for (const auto & cs : displayCS)
        {
            save(out, cs, configMajorVersion);
        }
        out << YAML::EndSeq;
    }

    // ColorSpaces
    {
        out << YAML::Newline;
        out << YAML::Key << "colorspaces";
        out << YAML::Value << YAML::BeginSeq;
        for (const auto & cs : sceneCS)
        {
            save(out, cs, configMajorVersion);
        }
        out << YAML::EndSeq;
    }

    // Named transforms.
    const int numNT = config.getNumNamedTransforms(NAMEDTRANSFORM_ALL);
    if (numNT > 0)
    {
        out << YAML::Newline;
        out << YAML::Key << "named_transforms";
        out << YAML::Value << YAML::BeginSeq;
        for (int i = 0; i < numNT; ++i)
        {
            auto name = config.getNamedTransformNameByIndex(NAMEDTRANSFORM_ALL, i);
            auto nt = config.getNamedTransform(name);
            save(out, nt, configMajorVersion);
        }
        out << YAML::EndSeq;
    }

    out << YAML::EndMap;
}

}

///////////////////////////////////////////////////////////////////////////

void OCIOYaml::Read(std::istream & istream, ConfigRcPtr & config, const char * filename)
{
    try
    {
        YAML::Node node = YAML::Load(istream);
        load(node, config, filename);
    }
    catch(const std::exception & e)
    {
        std::ostringstream os;
        os << "Error: Loading the OCIO profile ";
        if (filename && filename[0] && 
            Platform::Strcasecmp(filename, "from Archive/ConfigIOProxy") != 0)
        {
            os << "'" << filename << "' ";
        }
        os << "failed. " << e.what();
        throw Exception(os.str().c_str());
    }
}

void OCIOYaml::Write(std::ostream & ostream, const Config & config)
{
    YAML::Emitter out;
    out.SetDoublePrecision(std::numeric_limits<double>::digits10);
    out.SetFloatPrecision(7);
    save(out, config);
    ostream << out.c_str();
}

} // namespace OCIO_NAMESPACE

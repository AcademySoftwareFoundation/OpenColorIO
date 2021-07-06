// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ViewingRules.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(ViewingRules, basic)
{
    OCIO::ViewingRulesRcPtr vrules = OCIO::ViewingRules::Create();
    OCIO_REQUIRE_ASSERT(vrules);
    OCIO_CHECK_EQUAL(vrules->getNumEntries(), 0);

    // Rules have to exist to be accessed.
    OCIO_CHECK_THROW_WHAT(vrules->getName(0), OCIO::Exception,
                          "Viewing rules: rule index '0' invalid.");
    OCIO_CHECK_THROW_WHAT(vrules->insertRule(1, "test"), OCIO::Exception,
                          "Viewing rules: rule index '1' invalid.");
    // New rules must have a name.
    OCIO_CHECK_THROW_WHAT(vrules->insertRule(0, ""), OCIO::Exception,
                          "Viewing rules: rule must have a non-empty name.");
    OCIO_CHECK_THROW_WHAT(vrules->insertRule(0, nullptr), OCIO::Exception,
                          "Viewing rules: rule must have a non-empty name.");

    // Add rules.
    const std::string ruleName0{ "Rule0" };
    OCIO_CHECK_NO_THROW(vrules->insertRule(0, ruleName0.c_str()));

    const std::string ruleName2{ "Rule2" };
    OCIO_CHECK_NO_THROW(vrules->insertRule(1, ruleName2.c_str()));

    // Adding Rule1 at index 1 will move Rule2 at index 2.
    const std::string ruleName1{ "Rule1" };
    OCIO_CHECK_NO_THROW(vrules->insertRule(1, ruleName1.c_str()));

    OCIO_REQUIRE_EQUAL(vrules->getNumEntries(), 3);
    std::string stringVal;
    OCIO_CHECK_NO_THROW(stringVal = vrules->getName(0));
    OCIO_CHECK_EQUAL(stringVal, ruleName0);
    OCIO_CHECK_NO_THROW(stringVal = vrules->getName(1));
    OCIO_CHECK_EQUAL(stringVal, ruleName1);
    OCIO_CHECK_NO_THROW(stringVal = vrules->getName(2));
    OCIO_CHECK_EQUAL(stringVal, ruleName2);

    // Only have 3 rules, index 3 is invalid.
    OCIO_CHECK_THROW_WHAT(vrules->getName(3), OCIO::Exception,
                          "Viewing rules: rule index '3' invalid.");

    // Rule names are unique.
    OCIO_CHECK_THROW_WHAT(vrules->insertRule(1, ruleName1.c_str()), OCIO::Exception,
                          "A rule named 'Rule1' already exists");

    // Check added rules properties are empty.
    size_t numOf{ 0 };
    for (size_t r = 0; r < 3; ++r)
    {
        OCIO_CHECK_NO_THROW(numOf = vrules->getNumColorSpaces(r));
        OCIO_CHECK_EQUAL(numOf, 0);
        OCIO_CHECK_NO_THROW(numOf = vrules->getNumEncodings(r));
        OCIO_CHECK_EQUAL(numOf, 0);
        OCIO_CHECK_NO_THROW(numOf = vrules->getNumCustomKeys(r));
        OCIO_CHECK_EQUAL(numOf, 0);
    }

    // Set colorspaces and verify.
    const std::string cs0{ "colorspace0" };
    const std::string cs1{ "colorspace1" };
    OCIO_CHECK_NO_THROW(vrules->addColorSpace(0, cs0.c_str()));
    OCIO_CHECK_NO_THROW(vrules->addColorSpace(0, cs1.c_str()));
    OCIO_CHECK_NO_THROW(numOf = vrules->getNumColorSpaces(0));
    OCIO_CHECK_EQUAL(numOf, 2);
    OCIO_CHECK_NO_THROW(stringVal = vrules->getColorSpace(0, 0));
    OCIO_CHECK_EQUAL(stringVal, cs0);
    OCIO_CHECK_NO_THROW(stringVal = vrules->getColorSpace(0, 1));
    OCIO_CHECK_EQUAL(stringVal, cs1);
    // Can not access non existing colorspaces.
    OCIO_CHECK_THROW_WHAT(vrules->getColorSpace(0, 2), OCIO::Exception,
                          "rule 'Rule0' at index '0': colorspace index '2' is invalid.");
    // Remove color space.
    OCIO_CHECK_THROW_WHAT(vrules->removeColorSpace(3, 0), OCIO::Exception,
                          "Viewing rules: rule index '3' invalid.");
    OCIO_CHECK_THROW_WHAT(vrules->removeColorSpace(0, 2), OCIO::Exception,
                          "rule 'Rule0' at index '0': colorspace index '2' is invalid.");
    OCIO_CHECK_NO_THROW(vrules->removeColorSpace(0, 0));
    OCIO_CHECK_NO_THROW(numOf = vrules->getNumColorSpaces(0));
    OCIO_CHECK_EQUAL(numOf, 1);
    OCIO_CHECK_NO_THROW(stringVal = vrules->getColorSpace(0, 0));
    OCIO_CHECK_EQUAL(stringVal, cs1);
    // Re-add color space.
    OCIO_CHECK_NO_THROW(vrules->addColorSpace(0, cs0.c_str()));

    // Same with encodings.
    const std::string enc0{ "encoding0" };
    const std::string enc1{ "encoding1" };
    OCIO_CHECK_THROW_WHAT(vrules->addEncoding(0, enc0.c_str()), OCIO::Exception,
                          "encoding can't be added if there are colorspaces.");
    OCIO_CHECK_NO_THROW(vrules->addEncoding(1, enc0.c_str()));
    OCIO_CHECK_NO_THROW(vrules->addEncoding(1, enc1.c_str()));
    OCIO_CHECK_THROW_WHAT(vrules->addColorSpace(1, cs0.c_str()), OCIO::Exception,
                          "colorspace can't be added if there are encodings.");
    OCIO_CHECK_NO_THROW(numOf = vrules->getNumEncodings(1));
    OCIO_CHECK_EQUAL(numOf, 2);
    OCIO_CHECK_NO_THROW(stringVal = vrules->getEncoding(1, 0));
    OCIO_CHECK_EQUAL(stringVal, enc0);
    OCIO_CHECK_NO_THROW(stringVal = vrules->getEncoding(1, 1));
    OCIO_CHECK_EQUAL(stringVal, enc1);
    OCIO_CHECK_THROW_WHAT(vrules->getEncoding(1, 2), OCIO::Exception,
                          "rule 'Rule1' at index '1': encoding index '2' is invalid.");
    // Remove encoding.
    OCIO_CHECK_THROW_WHAT(vrules->removeEncoding(3, 0), OCIO::Exception,
                          "Viewing rules: rule index '3' invalid.");
    OCIO_CHECK_THROW_WHAT(vrules->removeEncoding(1, 2), OCIO::Exception,
                          "rule 'Rule1' at index '1': encoding index '2' is invalid.");
    OCIO_CHECK_NO_THROW(vrules->removeEncoding(1, 0));
    OCIO_CHECK_NO_THROW(numOf = vrules->getNumEncodings(1));
    OCIO_CHECK_EQUAL(numOf, 1);
    OCIO_CHECK_NO_THROW(stringVal = vrules->getEncoding(1, 0));
    OCIO_CHECK_EQUAL(stringVal, enc1);
    // Re-add encoding.
    OCIO_CHECK_NO_THROW(vrules->addEncoding(1, enc0.c_str()));

    // Same with custom keys.
    const std::string key0{ "key0" };
    const std::string value0{ "value0" };
    const std::string key1{ "key1" };
    const std::string value1{ "value1" };
    OCIO_CHECK_NO_THROW(vrules->setCustomKey(0, key0.c_str(), value0.c_str()));
    OCIO_CHECK_NO_THROW(vrules->setCustomKey(0, key1.c_str(), value1.c_str()));
    OCIO_CHECK_NO_THROW(numOf = vrules->getNumCustomKeys(0));
    OCIO_CHECK_EQUAL(numOf, 2);
    OCIO_CHECK_NO_THROW(stringVal = vrules->getCustomKeyName(0, 0));
    OCIO_CHECK_EQUAL(stringVal, key0);
    OCIO_CHECK_NO_THROW(stringVal = vrules->getCustomKeyValue(0, 0));
    OCIO_CHECK_EQUAL(stringVal, value0);
    OCIO_CHECK_NO_THROW(stringVal = vrules->getCustomKeyName(0, 1));
    OCIO_CHECK_EQUAL(stringVal, key1);
    OCIO_CHECK_NO_THROW(stringVal = vrules->getCustomKeyValue(0, 1));
    OCIO_CHECK_EQUAL(stringVal, value1);
    OCIO_CHECK_THROW_WHAT(vrules->getCustomKeyName(0, 2), OCIO::Exception,
                          "rule named 'Rule0' error: Key index '2' is invalid");
    OCIO_CHECK_THROW_WHAT(vrules->getCustomKeyValue(0, 2), OCIO::Exception,
                          "rule named 'Rule0' error: Key index '2' is invalid");

    const std::string newvalue0{ "newvalue0" };
    OCIO_CHECK_NO_THROW(vrules->setCustomKey(0, key0.c_str(), newvalue0.c_str()));
    OCIO_CHECK_NO_THROW(numOf = vrules->getNumCustomKeys(0));
    OCIO_CHECK_EQUAL(numOf, 2);
    OCIO_CHECK_NO_THROW(stringVal = vrules->getCustomKeyValue(0, 0));
    OCIO_CHECK_EQUAL(stringVal, newvalue0);

    // Test serialization.
    std::ostringstream oss;
    oss << *vrules;
    constexpr char OUTPUT[]{ R"(<ViewingRule name=Rule0, colorspaces=[colorspace1, colorspace0], customKeys=[(key0, newvalue0), (key1, value1)]>
<ViewingRule name=Rule1, encodings=[encoding1, encoding0]>
<ViewingRule name=Rule2>)" };

    OCIO_CHECK_EQUAL(oss.str(), OUTPUT);

    // Removing a rule: throws if index is not valid.
    const auto numRules = vrules->getNumEntries();
    OCIO_CHECK_THROW_WHAT(vrules->removeRule(numRules), OCIO::Exception,
                          "rule index '3' invalid. There are only '3' rules");
    OCIO_REQUIRE_EQUAL(vrules->getNumEntries(), numRules);
    OCIO_CHECK_NO_THROW(numOf = vrules->getNumColorSpaces(0));
    OCIO_CHECK_EQUAL(numOf, 2);
    OCIO_CHECK_NO_THROW(numOf = vrules->getNumEncodings(1));
    OCIO_CHECK_EQUAL(numOf, 2);

    // Remove rule, and check it is gone.
    OCIO_CHECK_NO_THROW(vrules->removeRule(1));
    OCIO_REQUIRE_EQUAL(vrules->getNumEntries(), 2);
    OCIO_CHECK_NO_THROW(stringVal = vrules->getName(0));
    OCIO_CHECK_EQUAL(stringVal, ruleName0);
    OCIO_CHECK_NO_THROW(stringVal = vrules->getName(1));
    OCIO_CHECK_EQUAL(stringVal, ruleName2);

    // Get index of a rule.
    OCIO_CHECK_NO_THROW(vrules->getIndexForRule(ruleName0.c_str()));
    OCIO_CHECK_EQUAL(0, vrules->getIndexForRule(ruleName0.c_str()));
    OCIO_CHECK_NO_THROW(vrules->getIndexForRule(ruleName2.c_str()));
    OCIO_CHECK_EQUAL(1, vrules->getIndexForRule(ruleName2.c_str()));
    OCIO_CHECK_THROW_WHAT(vrules->getIndexForRule("I am not there"), OCIO::Exception,
                          "rule name 'I am not there' not found");
}

OCIO_ADD_TEST(ViewingRules, config_io)
{
    // Create a config with viewing rules.
    OCIO::ConfigRcPtr config = OCIO::Config::CreateRaw()->createEditableCopy();

    OCIO::ViewingRulesRcPtr vrules = OCIO::ViewingRules::Create();
    const std::string ruleName0{ "Rule0" };
    OCIO_CHECK_NO_THROW(vrules->insertRule(0, ruleName0.c_str()));
    const std::string ruleName1{ "Rule1" };
    OCIO_CHECK_NO_THROW(vrules->insertRule(1, ruleName1.c_str()));
    
    const std::string key0{ "key0" };
    const std::string value0{ "value0" };
    const std::string key1{ "key1" };
    const std::string value1{ "value1" };
    OCIO_CHECK_NO_THROW(vrules->setCustomKey(0, key0.c_str(), value0.c_str()));
    OCIO_CHECK_NO_THROW(vrules->setCustomKey(0, key1.c_str(), value1.c_str()));

    const std::string enc0{ "encoding0" };
    const std::string enc1{ "encoding1" };
    OCIO_CHECK_NO_THROW(vrules->addEncoding(1, enc0.c_str()));
    OCIO_CHECK_NO_THROW(vrules->addEncoding(1, enc1.c_str()));

    // Rules have to refer to colorspace or encoding.
    config->setViewingRules(vrules);
    OCIO_CHECK_THROW_WHAT(config->validate(), OCIO::Exception,
                          "must have either a color space or an encoding");

    const std::string cs0{ "colorspace0" };
    OCIO_CHECK_NO_THROW(vrules->addColorSpace(0, cs0.c_str()));
    auto cs = OCIO::ColorSpace::Create();
    cs->setName(cs0.c_str());
    config->addColorSpace(cs);

    cs->setName("cs_enc0");
    cs->setEncoding(enc0.c_str());
    config->addColorSpace(cs);

    cs->setName("cs_enc1");
    cs->setEncoding(enc1.c_str());
    config->addColorSpace(cs);

    config->setViewingRules(vrules);
    OCIO_CHECK_NO_THROW(config->validate());

    // Save config and load back.
    std::ostringstream oss;
    oss << *config.get();
    auto configStr = oss.str();
    std::istringstream back;
    back.str(configStr);
    auto configBack = OCIO::Config::CreateFromStream(back);

    // Verify rules have been loaded.
    auto vr = configBack->getViewingRules();
    OCIO_REQUIRE_EQUAL(vr->getNumEntries(), 2);

    std::string stringVal;
    OCIO_CHECK_NO_THROW(stringVal = vr->getName(0));
    OCIO_CHECK_EQUAL(stringVal, ruleName0);
    OCIO_CHECK_NO_THROW(stringVal = vr->getName(1));
    OCIO_CHECK_EQUAL(stringVal, ruleName1);

    size_t numOf{ 0 };
    OCIO_CHECK_NO_THROW(numOf = vr->getNumColorSpaces(0));
    OCIO_CHECK_EQUAL(numOf, 1);
    OCIO_CHECK_NO_THROW(numOf = vr->getNumEncodings(0));
    OCIO_CHECK_EQUAL(numOf, 0);
    OCIO_CHECK_NO_THROW(numOf = vr->getNumCustomKeys(0));
    OCIO_CHECK_EQUAL(numOf, 2);

    OCIO_CHECK_NO_THROW(stringVal = vr->getColorSpace(0, 0));
    OCIO_CHECK_EQUAL(stringVal, cs0);
    OCIO_CHECK_NO_THROW(stringVal = vr->getCustomKeyName(0, 0));
    OCIO_CHECK_EQUAL(stringVal, key0);
    OCIO_CHECK_NO_THROW(stringVal = vr->getCustomKeyValue(0, 0));
    OCIO_CHECK_EQUAL(stringVal, value0);
    OCIO_CHECK_NO_THROW(stringVal = vr->getCustomKeyName(0, 1));
    OCIO_CHECK_EQUAL(stringVal, key1);
    OCIO_CHECK_NO_THROW(stringVal = vr->getCustomKeyValue(0, 1));
    OCIO_CHECK_EQUAL(stringVal, value1);

    OCIO_CHECK_NO_THROW(numOf = vr->getNumColorSpaces(1));
    OCIO_CHECK_EQUAL(numOf, 0);
    OCIO_CHECK_NO_THROW(numOf = vr->getNumEncodings(1));
    OCIO_CHECK_EQUAL(numOf, 2);
    OCIO_CHECK_NO_THROW(numOf = vr->getNumCustomKeys(1));
    OCIO_CHECK_EQUAL(numOf, 0);
    OCIO_CHECK_NO_THROW(stringVal = vr->getEncoding(1, 0));
    OCIO_CHECK_EQUAL(stringVal, enc0);
    OCIO_CHECK_NO_THROW(stringVal = vr->getEncoding(1, 1));
    OCIO_CHECK_EQUAL(stringVal, enc1);
}

OCIO_ADD_TEST(ViewingRules, filtered_views)
{
    constexpr char SIMPLE_CONFIG[]{ R"(ocio_profile_version: 2

environment:
  {}
search_path: ""
strictparsing: true
luma: [0.2126, 0.7152, 0.0722]

roles:
  default: raw
  scene_linear: c3

file_rules:
  - !<Rule> {name: ColorSpaceNamePathSearch}
  - !<Rule> {name: Default, colorspace: raw}

viewing_rules:
  - !<Rule> {name: Rule_1, colorspaces: c1}
  - !<Rule> {name: Rule_2, colorspaces: [c2, c3]}
  - !<Rule> {name: Rule_3, colorspaces: scene_linear}
  - !<Rule> {name: Rule_4, colorspaces: [c3, c4]}
  - !<Rule> {name: Rule_5, encodings: log}
  - !<Rule> {name: Rule_6, encodings: [log, video]}

shared_views:
  - !<View> {name: SView_a, colorspace: raw, rule: Rule_2}
  - !<View> {name: SView_b, colorspace: raw, rule: Rule_3}
  - !<View> {name: SView_c, colorspace: raw}
  - !<View> {name: SView_d, colorspace: raw, rule: Rule_5}
  - !<View> {name: SView_e, colorspace: raw}

displays:
  sRGB:
    - !<View> {name: View_a, colorspace: raw, rule: Rule_1}
    - !<View> {name: View_b, colorspace: raw, rule: Rule_2}
    - !<View> {name: View_c, colorspace: raw, rule: Rule_2}
    - !<View> {name: View_d, colorspace: raw, rule: Rule_3}
    - !<View> {name: View_e, colorspace: raw, rule: Rule_4}
    - !<View> {name: View_f, colorspace: raw, rule: Rule_5}
    - !<View> {name: View_g, colorspace: raw, rule: Rule_6}
    - !<View> {name: View_h, colorspace: raw}
    - !<Views> [SView_a, SView_b, SView_d, SView_e]

active_displays: []
active_views: []

colorspaces:
  - !<ColorSpace>
    name: raw
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    isdata: false
    allocation: uniform

  - !<ColorSpace>
    name: c1
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    isdata: false
    encoding: video
    allocation: uniform

  - !<ColorSpace>
    name: c2
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    isdata: false
    allocation: uniform

  - !<ColorSpace>
    name: c3
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    isdata: false
    allocation: uniform

  - !<ColorSpace>
    name: c4
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    isdata: false
    encoding: log
    allocation: uniform

  - !<ColorSpace>
    name: c5
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    isdata: false
    encoding: data
    allocation: uniform

  - !<ColorSpace>
    name: c6
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    isdata: false
    encoding: video
    allocation: uniform
)" };

    std::istringstream is(SIMPLE_CONFIG);
    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    // Check 2 rules of 2 non-existing display/views.
    std::string rule;
    rule = config->getDisplayViewRule("no", "unknown");
    OCIO_CHECK_EQUAL(rule, "");
    rule = config->getDisplayViewRule("sRGB", "unknown");
    OCIO_CHECK_EQUAL(rule, "");
    // sRGB/View_b uses Rule_2.
    rule = config->getDisplayViewRule("sRGB", "View_b");
    OCIO_CHECK_EQUAL(rule, "Rule_2");

    int numViews{ 0 };
    std::string view;
    // Access views by colorspace on non existing display: 0 and empty string.
    OCIO_CHECK_NO_THROW(numViews = config->getNumViews("no", "unknown"));
    OCIO_CHECK_EQUAL(numViews, 0);
    OCIO_CHECK_NO_THROW(view = config->getView("no", "unknown", 0));
    OCIO_CHECK_EQUAL(view, "");

    // When display exists, colorspace has to exist or it will throw.
    OCIO_CHECK_THROW_WHAT(config->getNumViews("sRGB", "unknown"), OCIO::Exception,
                          "Could not find source color space 'unknown'.");
    OCIO_CHECK_THROW_WHAT(config->getView("sRGB", "unknown", 0), OCIO::Exception,
                          "Could not find source color space 'unknown'.");

    // Views by existing colorspace on existing display.
    OCIO_CHECK_NO_THROW(numViews = config->getNumViews("sRGB", "c6"));
    OCIO_CHECK_EQUAL(numViews, 3);
    OCIO_CHECK_NO_THROW(view = config->getView("sRGB", "c6", 0));
    // View_g rule is Rule_6 that lists encoding video, c6 has encoding video.
    OCIO_CHECK_EQUAL(view, "View_g");
    OCIO_CHECK_NO_THROW(view = config->getView("sRGB", "c6", 1));
    // View_h has no rule.
    OCIO_CHECK_EQUAL(view, "View_h");
    OCIO_CHECK_NO_THROW(view = config->getView("sRGB", "c6", 2));
    // Shared SView_e has no rule.
    OCIO_CHECK_EQUAL(view, "SView_e");
    // There is no 4th view: empty string.
    OCIO_CHECK_NO_THROW(view = config->getView("sRGB", "c6", 3));
    OCIO_CHECK_ASSERT(view.empty());

    OCIO_CHECK_NO_THROW(numViews = config->getNumViews("sRGB", "c3"));
    OCIO_CHECK_EQUAL(numViews, 8);
    OCIO_CHECK_NO_THROW(view = config->getView("sRGB", "c3", 0));
    // View_b rule is Rule_2 that lists c3.
    OCIO_CHECK_EQUAL(view, "View_b");
    OCIO_CHECK_NO_THROW(view = config->getView("sRGB", "c3", 1));
    // View_c rule is Rule_2 that lists c3.
    OCIO_CHECK_EQUAL(view, "View_c");
    OCIO_CHECK_NO_THROW(view = config->getView("sRGB", "c3", 2));
    // View_d rule is Rule_3 that lists c3.
    OCIO_CHECK_EQUAL(view, "View_d");
    OCIO_CHECK_NO_THROW(view = config->getView("sRGB", "c3", 3));
    // View_e rule is Rule_4 that lists c3.
    OCIO_CHECK_EQUAL(view, "View_e");
    OCIO_CHECK_NO_THROW(view = config->getView("sRGB", "c3", 4));
    // View_h has no rule.
    OCIO_CHECK_EQUAL(view, "View_h");
    OCIO_CHECK_NO_THROW(view = config->getView("sRGB", "c3", 5));
    // SView_a has rule Rule_2 that lists c3.
    OCIO_CHECK_EQUAL(view, "SView_a");
    OCIO_CHECK_NO_THROW(view = config->getView("sRGB", "c3", 6));
    // SView_b has rule Rule_3 that lists c3.
    OCIO_CHECK_EQUAL(view, "SView_b");
    OCIO_CHECK_NO_THROW(view = config->getView("sRGB", "c3", 7));
    // SView_e has no rule.
    OCIO_CHECK_EQUAL(view, "SView_e");

    OCIO_CHECK_NO_THROW(numViews = config->getNumViews("sRGB", "c4"));
    OCIO_CHECK_EQUAL(numViews, 6);
    OCIO_CHECK_NO_THROW(view = config->getView("sRGB", "c4", 0));
    // View_e rule is Rule_4 that lists c4.
    OCIO_CHECK_EQUAL(view, "View_e");
    OCIO_CHECK_NO_THROW(view = config->getView("sRGB", "c4", 1));
    // View_f rule is Rule_5 that lists encoding log, c4 has encoding log.
    OCIO_CHECK_EQUAL(view, "View_f");
    OCIO_CHECK_NO_THROW(view = config->getView("sRGB", "c4", 2));
    // View_g rule is Rule_6 that lists encoding log, c4 has encoding log.
    OCIO_CHECK_EQUAL(view, "View_g");
    OCIO_CHECK_NO_THROW(view = config->getView("sRGB", "c4", 3));
    // View_h has no rule.
    OCIO_CHECK_EQUAL(view, "View_h");
    OCIO_CHECK_NO_THROW(view = config->getView("sRGB", "c4", 4));
    // SView_d rule is Rule_5 that lists encoding log, c4 has encoding log.
    OCIO_CHECK_EQUAL(view, "SView_d");
    OCIO_CHECK_NO_THROW(view = config->getView("sRGB", "c4", 5));
    // SView_e has no rule.
    OCIO_CHECK_EQUAL(view, "SView_e");

    std::stringstream os;
    os << *config.get();
    OCIO_CHECK_EQUAL(os.str(), SIMPLE_CONFIG);

    // Copy to set active views.
    OCIO::ConfigRcPtr configav = config->createEditableCopy();
    configav->setActiveViews("SView_e, View_h, SView_d, View_d, SView_a, View_b");
    OCIO_CHECK_NO_THROW(configav->validate());

    // Viewing rule results are further filtered and re-ordered by the active views list.
    OCIO_CHECK_NO_THROW(numViews = configav->getNumViews("sRGB", "c3"));
    OCIO_CHECK_EQUAL(numViews, 5);
    OCIO_CHECK_NO_THROW(view = configav->getView("sRGB", "c3", 0));
    OCIO_CHECK_EQUAL(view, "SView_e");
    OCIO_CHECK_NO_THROW(view = configav->getView("sRGB", "c3", 1));
    OCIO_CHECK_EQUAL(view, "View_h");
    OCIO_CHECK_NO_THROW(view = configav->getView("sRGB", "c3", 2));
    OCIO_CHECK_EQUAL(view, "View_d");
    OCIO_CHECK_NO_THROW(view = configav->getView("sRGB", "c3", 3));
    OCIO_CHECK_EQUAL(view, "SView_a");
    OCIO_CHECK_NO_THROW(view = configav->getView("sRGB", "c3", 4));
    OCIO_CHECK_EQUAL(view, "View_b");

    // Test the default methods.

    OCIO_CHECK_EQUAL(std::string("sRGB"), std::string(configav->getDefaultDisplay()));
    OCIO_CHECK_EQUAL(std::string(configav->getView("sRGB", "c3", 0)), 
                     std::string(configav->getDefaultView("sRGB", "c3")));
}

// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <fstream>

#include "FileRules.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestLogUtils.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(FileRules, config_v1)
{
    static const char CONFIG[] = 
        "ocio_profile_version: 1\n"
        "strictparsing: false\n"
        "roles:\n"
        "  default: raw\n"
        "displays:\n"
        "  sRGB:\n"
        "  - !<View> {name: Raw, colorspace: raw}\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "      name: raw\n";

    std::istringstream is;
    is.str(CONFIG);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    OCIO_CHECK_EQUAL(config->getFileRules()->getNumEntries(), 1);
    OCIO_CHECK_EQUAL(std::string(config->getFileRules()->getName(0)), "Default");
}

OCIO_ADD_TEST(FileRules, config_read_only)
{
    auto config = OCIO::Config::CreateRaw();
    auto fileRules = config->getFileRules();
    OCIO_REQUIRE_EQUAL(fileRules->getNumEntries(), 2);
    OCIO_CHECK_EQUAL(std::string(fileRules->getName(0)), OCIO::FileRuleUtils::ParseName);
    OCIO_CHECK_EQUAL(fileRules->getIndexForRule(OCIO::FileRuleUtils::ParseName), 0);
    OCIO_CHECK_ASSERT(!fileRules->getPattern(0));
    OCIO_CHECK_ASSERT(!fileRules->getExtension(0));
    OCIO_CHECK_ASSERT(!fileRules->getRegex(0));
    OCIO_CHECK_ASSERT(std::string(fileRules->getColorSpace(0)).empty());
    OCIO_CHECK_EQUAL(std::string(fileRules->getName(1)), OCIO::FileRuleUtils::DefaultName);
    OCIO_CHECK_EQUAL(fileRules->getIndexForRule(OCIO::FileRuleUtils::DefaultName), 1);
    OCIO_CHECK_ASSERT(!fileRules->getPattern(1));
    OCIO_CHECK_ASSERT(!fileRules->getExtension(1));
    OCIO_CHECK_ASSERT(!fileRules->getRegex(1));
    OCIO_CHECK_EQUAL(std::string(fileRules->getColorSpace(1)), OCIO::ROLE_DEFAULT);

    OCIO_CHECK_THROW_WHAT(fileRules->getName(2), 
                          OCIO::Exception, 
                          "rule index '2' invalid. There are only '2' rules.");

    OCIO_CHECK_THROW_WHAT(fileRules->getIndexForRule("toto"), 
                          OCIO::Exception, 
                          "rule name 'toto' not found");
}

OCIO_ADD_TEST(FileRules, config_insert_rule)
{
    auto configRaw = OCIO::Config::CreateRaw();
    auto config = configRaw->createEditableCopy();
    auto fr = config->getFileRules();
    auto fileRules = fr->createEditableCopy();
    OCIO_REQUIRE_EQUAL(fileRules->getNumEntries(), 2);
    OCIO_CHECK_NO_THROW(fileRules->insertRule(0, "rule", "raw", "*", "a"));
    OCIO_REQUIRE_EQUAL(fileRules->getNumEntries(), 3);
    OCIO_CHECK_NO_THROW(fileRules->insertRule(0, "TIFF rule", "raw", R"(.*\.TIF?F$)"));
    OCIO_REQUIRE_EQUAL(fileRules->getNumEntries(), 4);
    OCIO_CHECK_THROW_WHAT(fileRules->insertRule(0, "rule", "raw", "*", "b"),
                          OCIO::Exception, "A rule named 'rule' already exists");
    OCIO_CHECK_THROW_WHAT(fileRules->insertRule(4, "rule2", "raw", "*", "a"),
                          OCIO::Exception, "rule index '4' invalid");
    OCIO_CHECK_THROW_WHAT(fileRules->removeRule(4), OCIO::Exception, "invalid");
    OCIO_CHECK_THROW_WHAT(fileRules->removeRule(3), OCIO::Exception, "is the default rule");
    OCIO_CHECK_NO_THROW(fileRules->removeRule(2));
    OCIO_CHECK_NO_THROW(fileRules->removeRule(1));
    OCIO_CHECK_NO_THROW(fileRules->removeRule(0));
    OCIO_CHECK_EQUAL(fileRules->getNumEntries(), 1);

    OCIO_CHECK_THROW_WHAT(fileRules->insertRule(0, OCIO::FileRuleUtils::ParseName,
                                                "colorspace", "", ""),
                          OCIO::Exception, "does not accept any color space");
    OCIO_CHECK_THROW_WHAT(fileRules->insertRule(0, OCIO::FileRuleUtils::ParseName,
                                                "", "pattern", ""),
                          OCIO::Exception, "do not accept any pattern");
    OCIO_CHECK_THROW_WHAT(fileRules->insertRule(0, OCIO::FileRuleUtils::ParseName,
                                                "", "", "extension"),
                          OCIO::Exception, "do not accept any extension");

    OCIO_CHECK_NO_THROW(fileRules->insertRule(0, OCIO::FileRuleUtils::ParseName, "", "", ""));

    OCIO_CHECK_THROW_WHAT(fileRules->insertRule(0, OCIO::FileRuleUtils::ParseName,
                                                "", "", "extension"),
                          OCIO::Exception, 
                          "File rules: A rule named 'ColorSpaceNamePathSearch' already exists.");

    OCIO_CHECK_THROW_WHAT(fileRules->insertRule(0, "default", "", "", "extension"),
                          OCIO::Exception, 
                          "File rules: A rule named 'default' already exists.");

    OCIO_CHECK_THROW_WHAT(fileRules->insertRule(0, "defauLT", "", "", "extension"),
                          OCIO::Exception, 
                          "File rules: A rule named 'defauLT' already exists.");

    OCIO_CHECK_THROW_WHAT(fileRules->insertRule(0, "   Default   ", "", "", "extension"),
                          OCIO::Exception, 
                          "File rules: A rule named 'Default' already exists.");

    OCIO_CHECK_NO_THROW(fileRules->removeRule(0));
    OCIO_CHECK_NO_THROW(fileRules->insertRule(0, OCIO::FileRuleUtils::ParseName,
                                              nullptr, nullptr, nullptr));

    // Adds a rule with empty name.
    OCIO_CHECK_THROW_WHAT(fileRules->insertRule(0, nullptr, "raw", "*", "a"),
                          OCIO::Exception, "rule should have a non empty name");

    // Pattern and extension can't be null.
    OCIO_CHECK_THROW_WHAT(fileRules->insertRule(0, "rule", "raw", nullptr, "a"),
                          OCIO::Exception, "file pattern is empty");
    OCIO_CHECK_THROW_WHAT(fileRules->insertRule(0, "rule", "raw", "*", nullptr),
                          OCIO::Exception, "file extension is empty");

    // Invalid glob pattern.
    OCIO_CHECK_THROW_WHAT(fileRules->insertRule(0, "rule", "raw", "[", "a"),
                          OCIO::Exception, "invalid regular expression");

    // Invalid regex.
    OCIO_CHECK_THROW_WHAT(fileRules->insertRule(0, "rule", "raw", "(.*)(\bwhat"),
                          OCIO::Exception, "invalid regular expression");
}

OCIO_ADD_TEST(FileRules, config_rule_customkeys)
{
    auto configRaw = OCIO::Config::CreateRaw();
    auto fr = configRaw->getFileRules();
    auto fileRules = fr->createEditableCopy();
    OCIO_REQUIRE_EQUAL(fileRules->getNumEntries(), 2);
    OCIO_CHECK_NO_THROW(fileRules->insertRule(0, "rule", "raw", "*", "a"));
    OCIO_REQUIRE_EQUAL(fileRules->getNumEntries(), 3);
    OCIO_CHECK_EQUAL(fileRules->getNumCustomKeys(0), 0);
    OCIO_CHECK_EQUAL(fileRules->getNumCustomKeys(1), 0);
    OCIO_CHECK_EQUAL(fileRules->getNumCustomKeys(2), 0);
    OCIO_CHECK_THROW_WHAT(fileRules->getNumCustomKeys(3), OCIO::Exception,
                          "rule index '3' invalid");
    OCIO_CHECK_THROW_WHAT(fileRules->getCustomKeyName(0, 0), OCIO::Exception,
                          "key index '0' is invalid");
    OCIO_CHECK_THROW_WHAT(fileRules->getCustomKeyName(1, 0), OCIO::Exception,
                          "key index '0' is invalid");
    OCIO_CHECK_THROW_WHAT(fileRules->getCustomKeyName(2, 0), OCIO::Exception,
                          "key index '0' is invalid");
    OCIO_CHECK_THROW_WHAT(fileRules->getCustomKeyValue(0, 0), OCIO::Exception,
                          "key index '0' is invalid");
    OCIO_CHECK_THROW_WHAT(fileRules->getCustomKeyValue(1, 0), OCIO::Exception,
                          "key index '0' is invalid");
    OCIO_CHECK_THROW_WHAT(fileRules->getCustomKeyValue(2, 0), OCIO::Exception,
                          "key index '0' is invalid");
    OCIO_CHECK_NO_THROW(fileRules->setCustomKey(0, "key", "val"));
    OCIO_CHECK_NO_THROW(fileRules->setCustomKey(1, "key", "val"));
    OCIO_CHECK_NO_THROW(fileRules->setCustomKey(2, "keyDef", "valDef"));
    OCIO_CHECK_THROW_WHAT(fileRules->setCustomKey(0, nullptr, "val"), OCIO::Exception,
                          "key has to be a non empty string");
    OCIO_CHECK_THROW_WHAT(fileRules->setCustomKey(0, "", "val"), OCIO::Exception,
                          "key has to be a non empty string");
    OCIO_REQUIRE_EQUAL(fileRules->getNumCustomKeys(0), 1);
    OCIO_REQUIRE_EQUAL(fileRules->getNumCustomKeys(1), 1);
    OCIO_REQUIRE_EQUAL(fileRules->getNumCustomKeys(2), 1);
    OCIO_CHECK_EQUAL(std::string(fileRules->getCustomKeyName(0, 0)), "key");
    OCIO_CHECK_EQUAL(std::string(fileRules->getCustomKeyValue(0, 0)), "val");
    OCIO_CHECK_NO_THROW(fileRules->setCustomKey(0, "key", ""));
    OCIO_CHECK_EQUAL(fileRules->getNumCustomKeys(0), 0);
    OCIO_CHECK_NO_THROW(fileRules->setCustomKey(0, "key", "val"));
    OCIO_CHECK_NO_THROW(fileRules->setCustomKey(0, "key", nullptr));
    OCIO_CHECK_EQUAL(fileRules->getNumCustomKeys(0), 0);
    OCIO_CHECK_NO_THROW(fileRules->setCustomKey(0, "key1", "val"));
    OCIO_CHECK_EQUAL(fileRules->getNumCustomKeys(0), 1);
    OCIO_CHECK_NO_THROW(fileRules->setCustomKey(0, "key1", "new val"));
    OCIO_REQUIRE_EQUAL(fileRules->getNumCustomKeys(0), 1);
    OCIO_CHECK_EQUAL(std::string(fileRules->getCustomKeyValue(0, 0)), "new val");
    OCIO_CHECK_NO_THROW(fileRules->setCustomKey(0, "key2", "val2"));
    OCIO_REQUIRE_EQUAL(fileRules->getNumCustomKeys(0), 2);
    OCIO_CHECK_NO_THROW(fileRules->setCustomKey(0, "key3", "3"));
    OCIO_REQUIRE_EQUAL(fileRules->getNumCustomKeys(0), 3);
    OCIO_CHECK_NO_THROW(fileRules->setCustomKey(0, "4", "val4"));
    OCIO_REQUIRE_EQUAL(fileRules->getNumCustomKeys(0), 4);
    OCIO_CHECK_EQUAL(std::string(fileRules->getCustomKeyName(0, 1)), "key1");
    OCIO_CHECK_EQUAL(std::string(fileRules->getCustomKeyValue(0, 1)), "new val");
    OCIO_CHECK_EQUAL(std::string(fileRules->getCustomKeyName(0, 2)), "key2");
    OCIO_CHECK_EQUAL(std::string(fileRules->getCustomKeyValue(0, 2)), "val2");
    OCIO_CHECK_EQUAL(std::string(fileRules->getCustomKeyName(0, 3)), "key3");
    OCIO_CHECK_EQUAL(std::string(fileRules->getCustomKeyValue(0, 3)), "3");
    OCIO_CHECK_EQUAL(std::string(fileRules->getCustomKeyName(0, 0)), "4");
    OCIO_CHECK_EQUAL(std::string(fileRules->getCustomKeyValue(0, 0)), "val4");

    auto config = configRaw->createEditableCopy();
    config->setFileRules(fileRules);

    std::ostringstream os;
    config->serialize(os);

    const std::string expected{ R"(ocio_profile_version: 2

search_path: ""
strictparsing: false
luma: [0.2126, 0.7152, 0.0722]

roles:
  default: raw

file_rules:
  - !<Rule> {name: rule, colorspace: raw, pattern: "*", extension: a, custom: {4: val4, key1: new val, key2: val2, key3: 3}}
  - !<Rule> {name: ColorSpaceNamePathSearch, custom: {key: val}}
  - !<Rule> {name: Default, colorspace: default, custom: {keyDef: valDef}}

displays:
  sRGB:
    - !<View> {name: Raw, colorspace: raw}

active_displays: []
active_views: []

colorspaces:
  - !<ColorSpace>
    name: raw
    family: raw
    equalitygroup: ""
    bitdepth: 32f
    description: |
      A raw color space. Conversions to and from this space are no-ops.
    isdata: true
    allocation: uniform
)" };

    OCIO_CHECK_EQUAL(expected, os.str());

    // Reload.
    std::istringstream is_reload;
    is_reload.str(expected);
    OCIO::ConstConfigRcPtr config_reloaded;
    OCIO_CHECK_NO_THROW(config_reloaded = OCIO::Config::CreateFromStream(is_reload));
    auto rules_reloaded = config_reloaded->getFileRules();

    OCIO_REQUIRE_EQUAL(rules_reloaded->getNumEntries(), 3);

    OCIO_REQUIRE_EQUAL(rules_reloaded->getNumCustomKeys(0), 4);
    OCIO_CHECK_EQUAL(std::string(rules_reloaded->getCustomKeyName(0, 1)), "key1");
    OCIO_CHECK_EQUAL(std::string(rules_reloaded->getCustomKeyValue(0, 1)), "new val");
    OCIO_CHECK_EQUAL(std::string(rules_reloaded->getCustomKeyName(0, 2)), "key2");
    OCIO_CHECK_EQUAL(std::string(rules_reloaded->getCustomKeyValue(0, 2)), "val2");
    OCIO_CHECK_EQUAL(std::string(rules_reloaded->getCustomKeyName(0, 3)), "key3");
    OCIO_CHECK_EQUAL(std::string(rules_reloaded->getCustomKeyValue(0, 3)), "3");
    OCIO_CHECK_EQUAL(std::string(rules_reloaded->getCustomKeyName(0, 0)), "4");
    OCIO_CHECK_EQUAL(std::string(rules_reloaded->getCustomKeyValue(0, 0)), "val4");

    OCIO_REQUIRE_EQUAL(rules_reloaded->getNumCustomKeys(1), 1);
    OCIO_CHECK_EQUAL(std::string(rules_reloaded->getCustomKeyName(1, 0)), "key");
    OCIO_CHECK_EQUAL(std::string(rules_reloaded->getCustomKeyValue(1, 0)), "val");

    OCIO_REQUIRE_EQUAL(rules_reloaded->getNumCustomKeys(2), 1);
    OCIO_CHECK_EQUAL(std::string(rules_reloaded->getCustomKeyName(2, 0)), "keyDef");
    OCIO_CHECK_EQUAL(std::string(rules_reloaded->getCustomKeyValue(2, 0)), "valDef");
}

OCIO_ADD_TEST(FileRules, config_rule_u8)
{
    auto configRaw = OCIO::Config::CreateRaw();
    auto fr = configRaw->getFileRules();
    auto fileRules = fr->createEditableCopy();
    OCIO_REQUIRE_EQUAL(fileRules->getNumEntries(), 2);
    OCIO_CHECK_NO_THROW(fileRules->insertRule(0, u8"éÀÂÇÉÈç$€", "raw", "*", "a"));
    OCIO_REQUIRE_EQUAL(fileRules->getNumEntries(), 3);
    OCIO_CHECK_NO_THROW(fileRules->setCustomKey(0, u8"key£", u8"val€"));
    OCIO_CHECK_EQUAL(std::string(fileRules->getCustomKeyName(0, 0)), u8"key£");
    OCIO_CHECK_EQUAL(std::string(fileRules->getCustomKeyValue(0, 0)), u8"val€");

    auto config = configRaw->createEditableCopy();
    config->setFileRules(fileRules);

    std::ostringstream os;
    config->serialize(os);

    // Reload.
    std::istringstream is_reload;
    is_reload.str(os.str());
    OCIO::ConstConfigRcPtr config_reloaded;
    OCIO_CHECK_NO_THROW(config_reloaded = OCIO::Config::CreateFromStream(is_reload));
    auto rules_reloaded = config_reloaded->getFileRules();

    OCIO_REQUIRE_EQUAL(rules_reloaded->getNumEntries(), 3);

    OCIO_CHECK_EQUAL(std::string(fileRules->getName(0)), u8"éÀÂÇÉÈç$€");

    OCIO_REQUIRE_EQUAL(rules_reloaded->getNumCustomKeys(0), 1);
    OCIO_CHECK_EQUAL(std::string(fileRules->getCustomKeyName(0, 0)), u8"key£");
    OCIO_CHECK_EQUAL(std::string(fileRules->getCustomKeyValue(0, 0)), u8"val€");
}

namespace
{

constexpr char g_config[] = { R"(ocio_profile_version: 2
strictparsing: true
roles:
  default: raw
  role1: cs1
  role2: cs2
displays:
  sRGB:
  - !<View> {name: Raw, colorspace: raw}
colorspaces:
  - !<ColorSpace>
      name: raw
  - !<ColorSpace>
      name: cs1
  - !<ColorSpace>
      name: cs2
  - !<ColorSpace>
      name: other_cs1
)" };

constexpr char g_name[] = "rule1";
constexpr char g_fileExt[] = "exr";
constexpr char g_filePattern[] = "*";

}

OCIO_ADD_TEST(FileRules, rule_invalid)
{
    std::istringstream is;
    is.str(g_config);
    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());

    OCIO_CHECK_NO_THROW(config->sanityCheck());
    auto rules = config->getFileRules()->createEditableCopy();
    OCIO_REQUIRE_EQUAL(rules->getNumEntries(), 1);

    OCIO_CHECK_NO_THROW(rules->insertRule(0, g_name, "cs1", g_filePattern, g_fileExt));
    config->setFileRules(rules);
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    OCIO_CHECK_NO_THROW(rules->setColorSpace(0, "role1"));
    config->setFileRules(rules);
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    OCIO_CHECK_NO_THROW(rules->setColorSpace(0, "invalid_color_space"));
    config->setFileRules(rules);
    OCIO_CHECK_THROW_WHAT(config->sanityCheck(), OCIO::Exception, "does not exist");
}

OCIO_ADD_TEST(FileRules, pattern_error)
{
    auto configRaw = OCIO::Config::CreateRaw();
    auto fr = configRaw->getFileRules();
    auto rules = fr->createEditableCopy();

    OCIO_CHECK_NO_THROW(rules->insertRule(0, "new rule", "raw", "*", "a"));
    OCIO_REQUIRE_EQUAL(rules->getNumEntries(), 3);

    OCIO_CHECK_THROW_WHAT(rules->setPattern(0, nullptr), OCIO::Exception, "file pattern is empty");
    OCIO_CHECK_NO_THROW(rules->setPattern(0, ""));

    OCIO_CHECK_THROW_WHAT(rules->setPattern(0, "[]"), OCIO::Exception,
                          "invalid regular expression");
    OCIO_CHECK_THROW_WHAT(rules->setPattern(0, "[!]"), OCIO::Exception,
                          "invalid regular expression");
    OCIO_CHECK_THROW_WHAT(rules->setPattern(0, "[a-b"), OCIO::Exception,
                          "invalid regular expression");
    OCIO_CHECK_THROW_WHAT(rules->setPattern(0, "[a-b]]"), OCIO::Exception,
                          "invalid regular expression");
    OCIO_CHECK_THROW_WHAT(rules->setPattern(0, "[[a-b]]"), OCIO::Exception,
                          "invalid regular expression");
    OCIO_CHECK_THROW_WHAT(rules->setPattern(0, "[*]"), OCIO::Exception,
                          "invalid regular expression");
}

OCIO_ADD_TEST(FileRules, extension_error)
{
    auto configRaw = OCIO::Config::CreateRaw();
    auto fr = configRaw->getFileRules();
    auto rules = fr->createEditableCopy();

    OCIO_CHECK_NO_THROW(rules->insertRule(0, "new rule", "raw", "*", "a"));
    OCIO_REQUIRE_EQUAL(rules->getNumEntries(), 3);

    OCIO_CHECK_THROW_WHAT(rules->setExtension(0, nullptr), OCIO::Exception,
                          "file extension is empty");
    OCIO_CHECK_NO_THROW(rules->setExtension(0, ""));
}

OCIO_ADD_TEST(FileRules, multiple_rules)
{
    std::istringstream is;
    is.str(g_config);
    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());

    OCIO_CHECK_NO_THROW(config->sanityCheck());
    auto rules = config->getFileRules()->createEditableCopy();

    // Create multiple rules.
    const size_t nbRulesToCreate = 42;
    const size_t nbDefaultRules = rules->getNumEntries();
    size_t nbRulesCreated = 0;
    while (nbRulesCreated < nbRulesToCreate)
    {
        std::string ruleName = "rule" + std::to_string(nbRulesCreated);
        OCIO_CHECK_NO_THROW(rules->insertRule(0, ruleName.c_str(), "cs1",
                                              g_filePattern, g_fileExt));
        nbRulesCreated++;
        OCIO_CHECK_EQUAL(rules->getNumEntries(), nbRulesCreated + nbDefaultRules);
    }

    config->setFileRules(rules);

    // Serialize the config.
    std::ostringstream oss;
    config->serialize(oss);

    // Reload config.
    std::istringstream is_reload;
    is_reload.str(oss.str());
    OCIO::ConstConfigRcPtr config_reloaded;
    OCIO_CHECK_NO_THROW(config_reloaded = OCIO::Config::CreateFromStream(is_reload));
    auto rules_reloaded = config_reloaded->getFileRules();

    // Validate that we have the correct number of rules.
    OCIO_CHECK_EQUAL(rules_reloaded->getNumEntries(), nbRulesCreated + nbDefaultRules);
}

OCIO_ADD_TEST(FileRules, rules_filepattern)
{
    std::istringstream is;
    is.str(g_config);
    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());
    auto rules = config->getFileRules()->createEditableCopy();

    // Add pattern + extension rule.
    OCIO_CHECK_NO_THROW(rules->insertRule(0, g_name, "cs1", "*", "[eE][xX][r]"));
    config->setFileRules(rules);

    size_t rulePosition = 0;
    config->getColorSpaceFromFilepath("/An/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/Arbitrary/Path/MyFile.eXr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/Arbitrary/Path/MyFile.EXR", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule. R must be lower case.
    config->getColorSpaceFromFilepath("/An/Arbitrary/Path/MyFileexr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/Arbitrary/Path/MyFile.jpeg", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/Arbitrary.exr/Path/MyFileexr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "gamma"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/An/gamma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "*gamma"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/An/gamma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "gamma*"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/An/gamma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "*gamma*"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/An/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/GaMma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/gamma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gammaArbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "*ga?ma*"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/An/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/GaMma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/gamma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gammaArbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gatmaArbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gatmaArbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gatttttttmaArbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/gamaArbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "*ga*ma*"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/An/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/GaMma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/gamma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gammaArbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gatmaArbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gatmaArbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gatttttttmaArbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gamaArbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "*g?mm*"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/An/gamma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gImma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gImmaaa/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "*g*mm*"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/An/gamma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gImma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gIIImmaaa/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gmm/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "*g?m?a*"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/An/gamma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gImma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gImIa/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/gIIImmaaa/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "*g[a]mma*"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/An/gmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/gamma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gbmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "*g[!a]mma*"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/An/gmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/gamma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/gbmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "*g[abcd]mma*"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/An/gamma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gbmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gcmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gdmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/gemma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/gabmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "*g[!abcd]mma*"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/An/gamma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/gbmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/gcmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/gdmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/gmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/gemma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gabmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/gefmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "*g[a-d]mma*"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/An/gamma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gbmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gcmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gdmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("/An/gmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/gemma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/gabmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("/An/gefmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "g[!a-d]mma*"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("gamma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("gbmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("gcmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("gdmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("gmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("gemma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("gabmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath("gefmma/Arbitrary/Path/MyFile.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "g[!a-d][\\*][e-g]mma"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("ge*fmma.exr", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);

    // Add pattern + extension rule.
    OCIO_CHECK_NO_THROW(rules->insertRule(0, "rule0", "cs1", "*", "jpg"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("test.jpg", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("test.Jpg", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("test.jpG", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("test.Jpeg", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 2); // Default rule.

    OCIO_CHECK_NO_THROW(rules->setExtension(0, "jp[gG]"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("test.jpg", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("test.jpG", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
    config->getColorSpaceFromFilepath("test.Jpg", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 2); // Default rule.

    OCIO_CHECK_NO_THROW(rules->setExtension(0, "[Jj]pg"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/mnt/media/image.Jpg", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);

    OCIO_CHECK_NO_THROW(rules->setExtension(0, "?pg"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/mnt/media/image.Jpg", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);

    OCIO_CHECK_NO_THROW(rules->setExtension(0, "jpg"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/mnt/media/image.Jpg", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);

    OCIO_CHECK_NO_THROW(rules->setExtension(0, "JPG"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/mnt/media/image.Jpg", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);

    OCIO_CHECK_NO_THROW(rules->setExtension(0, "jp[gG]"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/mnt/media/image.Jpg", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 2);

    OCIO_CHECK_NO_THROW(rules->setExtension(0, "?PG"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/mnt/media/image.Jpg", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 2);

    OCIO_CHECK_NO_THROW(rules->setExtension(0, "jP*"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/mnt/media/image.Jpg", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 2);

    OCIO_CHECK_NO_THROW(rules->setExtension(0, "*g"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/mnt/media/image.Jpg", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "*[^]*"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/mnt/me^ia/image.Jpg", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);

    config->getColorSpaceFromFilepath("/mnt/media/image.Jpg", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 2);

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "*(name)*"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath("/mnt/(name)/image.Jpg", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);
}

OCIO_ADD_TEST(FileRules, rules_regex)
{
    std::istringstream is;
    is.str(g_config);
    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());
    auto rules = config->getFileRules()->createEditableCopy();

    // Add pattern + extension rule.
    OCIO_CHECK_NO_THROW(rules->insertRule(0, g_name, "cs1", R"((.*)(\bmine\b|\byours\b)(.*))"));
    config->setFileRules(rules);

    size_t rulePosition = 0;
    config->getColorSpaceFromFilepath(R"(mnt/mine/media/image.jpg)", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);

    config->getColorSpaceFromFilepath(R"(mnt/miner/media/image.jpg)", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1);

    config->getColorSpaceFromFilepath(R"(yours/mnt/media/image.jpg)", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);

    config->getColorSpaceFromFilepath(R"(mnt\media\yours\image.jpg)", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);

    config->getColorSpaceFromFilepath(R"(mine/media/image.jpg)", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 0);

    // The error details may be different on each platform.
    OCIO_CHECK_THROW_WHAT(rules->insertRule(1, "invalid", "cs1", R"((.*)(\bmine\b|\byours\b(.*))"),
                          OCIO::Exception, "invalid regular expression");
}

OCIO_ADD_TEST(FileRules, rules_long_filepattern)
{
    std::istringstream is;
    is.str(g_config);
    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());
    auto rules = config->getFileRules()->createEditableCopy();

    // Add pattern + extension rule.
    OCIO_CHECK_NO_THROW(rules->insertRule(0, g_name, "cs1", "*", "exr"));
    config->setFileRules(rules);

    // The file path existence is not tested
    constexpr char arbitraryPath[] = "/Users/hodoulp/Documents/work/Color Management/ocio-images"
                                     ".1.0v4/spi-vfx/marci_512_srgb.exr";

    size_t rulePos = rules->getNumEntries();
    config->getColorSpaceFromFilepath(arbitraryPath, rulePos);
    OCIO_CHECK_EQUAL(rulePos, 0);

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "*Col?r*"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath(arbitraryPath, rulePos);
    OCIO_CHECK_EQUAL(rulePos, 0);

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "*****************************************************"
                                              "*******"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath(arbitraryPath, rulePos);
    OCIO_CHECK_EQUAL(rulePos, 0);

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "*?"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath(arbitraryPath, rulePos);
    OCIO_CHECK_EQUAL(rulePos, 0);

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "?*"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath(arbitraryPath, rulePos);
    OCIO_CHECK_EQUAL(rulePos, 0);

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "?*"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath(arbitraryPath, rulePos);
    OCIO_CHECK_EQUAL(rulePos, 0);

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "*?*"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath(arbitraryPath, rulePos);
    OCIO_CHECK_EQUAL(rulePos, 0);

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "*.1.0v4*"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath(arbitraryPath, rulePos);
    OCIO_CHECK_EQUAL(rulePos, 0);

    OCIO_CHECK_NO_THROW(rules->setPattern(0, "*.1.*"));
    config->setFileRules(rules);
    config->getColorSpaceFromFilepath(arbitraryPath, rulePos);
    OCIO_CHECK_EQUAL(rulePos, 0);
}

OCIO_ADD_TEST(FileRules, rules_test)
{
    std::istringstream is;
    is.str(g_config);
    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());
    auto rules = config->getFileRules()->createEditableCopy();
    rules->insertRule(0, OCIO::FileRuleUtils::ParseName, nullptr, nullptr);
    rules->insertRule(1, "dpx file", "raw", "", "dpx");
    config->setFileRules(rules);

    size_t rulePos = 0;
    const char * colorSpace = nullptr;
    colorSpace = config->getColorSpaceFromFilepath("/mnt/user/show/img_cs1.dpx", rulePos);
    OCIO_CHECK_EQUAL(rulePos, 0);
    OCIO_CHECK_ASSERT(colorSpace != nullptr && 0 == strcmp(colorSpace, "cs1"));

    colorSpace = config->getColorSpaceFromFilepath("show/cs2/img_cs1.exr", rulePos);
    OCIO_CHECK_EQUAL(rulePos, 0);
    // The first color space name from the right.
    OCIO_CHECK_ASSERT(colorSpace != nullptr && 0 == strcmp(colorSpace, "cs1"));

    colorSpace = config->getColorSpaceFromFilepath("show/cs1/img_other_cs1.exr", rulePos);
    OCIO_CHECK_EQUAL(rulePos, 0);
    // If there are 2 cs names ending the same position, the longest is used.
    OCIO_CHECK_ASSERT(colorSpace != nullptr && 0 == strcmp(colorSpace, "other_cs1"));

    colorSpace = config->getColorSpaceFromFilepath("show/other_cs1/img_cs1.exr", rulePos);
    OCIO_CHECK_EQUAL(rulePos, 0);
    OCIO_CHECK_ASSERT(colorSpace != nullptr && 0 == strcmp(colorSpace, "cs1"));

    colorSpace = config->getColorSpaceFromFilepath("/mnt/user/unknown.dpx", rulePos);
    OCIO_CHECK_EQUAL(rulePos, 1);
    OCIO_CHECK_ASSERT(colorSpace != nullptr && 0 == strcmp(colorSpace, "raw"));

    colorSpace = config->getColorSpaceFromFilepath("/mnt/user/unknown.jpg", rulePos);
    OCIO_CHECK_EQUAL(rulePos, 2); // The default rule. 
    OCIO_CHECK_ASSERT(colorSpace != nullptr && 0 == strcmp(colorSpace, OCIO::ROLE_DEFAULT));
}

OCIO_ADD_TEST(FileRules, rules_priority)
{
    std::istringstream is;
    is.str(g_config);
    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());
    auto rules = config->getFileRules()->createEditableCopy();
    rules->insertRule(0, "pattern dpx file", "raw", "*cs2*", "dpx");
    rules->insertRule(1, OCIO::FileRuleUtils::ParseName, nullptr, nullptr);
    rules->insertRule(2, "regex rule", "cs5", ".*cs5.dpx");
    config->setFileRules(rules);

    size_t rulePos = 0;
    const char * colorSpace;
    colorSpace = config->getColorSpaceFromFilepath("/mnt/media/cs2.dpx", rulePos);
    OCIO_CHECK_EQUAL(rulePos, 0);
    OCIO_CHECK_ASSERT(colorSpace != nullptr && 0 == strcmp(colorSpace, "raw"));

    colorSpace = config->getColorSpaceFromFilepath("/mnt/media/cs2.exr", rulePos);
    OCIO_CHECK_EQUAL(rulePos, 1);
    OCIO_CHECK_ASSERT(colorSpace != nullptr && 0 == strcmp(colorSpace, "cs2"));

    colorSpace = config->getColorSpaceFromFilepath("/mnt/media/cs5.dpx", rulePos);
    OCIO_CHECK_EQUAL(rulePos, 2);
    OCIO_CHECK_ASSERT(colorSpace != nullptr && 0 == strcmp(colorSpace, "cs5"));

    colorSpace = config->getColorSpaceFromFilepath("/mnt/media/cs5.DPX", rulePos);
    OCIO_CHECK_EQUAL(rulePos, 3);
    OCIO_CHECK_ASSERT(colorSpace != nullptr && 0 == strcmp(colorSpace, OCIO::ROLE_DEFAULT));
}

OCIO_ADD_TEST(FileRules, config_no_default)
{
    constexpr char configNoDefault[] = { R"(ocio_profile_version: 2
strictparsing: true
roles:
  role1: cs1
  role2: cs2
displays:
  sRGB:
  - !<View> {name: Raw, colorspace: raw}
colorspaces:
  - !<ColorSpace>
      name: raw
  - !<ColorSpace>
      name: cs1
  - !<ColorSpace>
      name: cs2
)" };

    std::istringstream is;
    is.str(configNoDefault);
    OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                          "must contain either a Default file rule or the 'default' role");
}

OCIO_ADD_TEST(FileRules, config_default_missmatch)
{
    constexpr char configDefaultMissmatch[] = { R"(ocio_profile_version: 2
strictparsing: true
roles:
  default: raw
  role1: cs1
  role2: cs2
displays:
  sRGB:
  - !<View> {name: Raw, colorspace: raw}
colorspaces:
  - !<ColorSpace>
      name: raw
  - !<ColorSpace>
      name: cs1
  - !<ColorSpace>
      name: cs2
file_rules:
  - !<Rule> {name: Default, colorspace: cs1}
)" };

    // As a warning message is expected, please mute it.
    OCIO::LogGuard guard;

    std::istringstream is;
    is.str(configDefaultMissmatch);
    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());

    const auto foundString = guard.output().find("that does not match the default role");
    OCIO_CHECK_ASSERT(foundString != std::string::npos);

    auto rules = config->getFileRules();
    OCIO_REQUIRE_ASSERT(rules);
    OCIO_REQUIRE_EQUAL(rules->getNumEntries(), 1);
    // File defined default role is preserved.
    auto cs = config->getColorSpace(OCIO::ROLE_DEFAULT);
    OCIO_CHECK_EQUAL(std::string("raw"), cs->getName());
}

OCIO_ADD_TEST(FileRules, config_no_default_rule)
{
    constexpr char configNoDefaultRule[] = { R"(ocio_profile_version: 2
strictparsing: true
roles:
  default: raw
  role1: cs1
  role2: cs2
displays:
  sRGB:
  - !<View> {name: Raw, colorspace: raw}
colorspaces:
  - !<ColorSpace>
      name: raw
  - !<ColorSpace>
      name: cs1
  - !<ColorSpace>
      name: cs2
file_rules:
  - !<Rule> {name: Custom, pattern: "*", extension: jpg, colorspace: cs1}
)" };

    std::istringstream is;
    is.str(configNoDefaultRule);
    OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                          "'file_rules' does not contain a Default <Rule>");
}

OCIO_ADD_TEST(FileRules, config_v1_faulty)
{
    constexpr char config_v1[] = { R"(ocio_profile_version: 1
strictparsing: true
roles:
  default: raw
  role1: cs1
  role2: cs2
displays:
  sRGB:
  - !<View> {name: Raw, colorspace: raw}
colorspaces:
  - !<ColorSpace>
      name: raw
  - !<ColorSpace>
      name: cs1
  - !<ColorSpace>
      name: cs2
file_rules:
  - !<Rule> {name: Default, colorspace: default}
)" };

    std::istringstream is;
    is.str(config_v1);
    OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                          "Config v1 can't use 'file_rules'");

}

OCIO_ADD_TEST(FileRules, config_v1_to_v2)
{
    {
        constexpr char config_v1[] = { R"(ocio_profile_version: 1
strictparsing: true
roles:
  default: raw
  role1: cs1
  role2: cs2
displays:
  sRGB:
  - !<View> {name: Raw, colorspace: raw}
colorspaces:
  - !<ColorSpace>
      name: raw
  - !<ColorSpace>
      name: cs1
  - !<ColorSpace>
      name: cs2
)" };

        std::istringstream is;
        is.str(config_v1);
        OCIO::ConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());

        OCIO_CHECK_NO_THROW(config->sanityCheck());
        config->setMajorVersion(2);

        OCIO_CHECK_NO_THROW(config->sanityCheck());

        auto rules = config->getFileRules()->createEditableCopy();
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(rules->getNumEntries() - 1)),
                         "default");
        rules->setDefaultRuleColorSpace("cs1");
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(rules->getNumEntries() - 1)),
                         "cs1");
    }

    {
        constexpr char config_v1_no_default[] = { R"(ocio_profile_version: 1
strictparsing: true
roles:
  role1: cs1
  role2: cs2
displays:
  sRGB:
  - !<View> {name: Raw, colorspace: raw}
colorspaces:
  - !<ColorSpace>
      name: raw
  - !<ColorSpace>
      name: cs1
  - !<ColorSpace>
      name: cs2
)" };

        std::istringstream is;
        is.str(config_v1_no_default);
        OCIO::ConstConfigRcPtr constConfig;
        OCIO_CHECK_NO_THROW(constConfig = OCIO::Config::CreateFromStream(is));
        OCIO::ConfigRcPtr config = constConfig->createEditableCopy();

        OCIO_CHECK_NO_THROW(config->sanityCheck());
        config->setMajorVersion(2);

        // Default rule is using 'Default' role that does not exist.
        OCIO_CHECK_THROW_WHAT(config->sanityCheck(), OCIO::Exception, "does not exist");

        config = constConfig->createEditableCopy();
        OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 3);

        // Upgrading is making sure that a valid v1 config will be a valid v2 config.
        config->upgradeToLatestVersion();

        OCIO_CHECK_EQUAL(config->getMajorVersion(), 2);
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        // 'Default' does not exist, 'Raw' is not a data color-space, so a new color-space has
        // been created with a unique name.
        OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 3);
        OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                   OCIO::COLORSPACE_ALL), 4);
        OCIO_CHECK_EQUAL(std::string(config->getColorSpaceNameByIndex(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                                      OCIO::COLORSPACE_ALL, 3)),
                         "added_default_rule_colorspace");
    }

    {
        constexpr char config_v1_no_default[] = { R"(ocio_profile_version: 1
strictparsing: true
roles:
  role1: cs1
  role2: cs2
displays:
  sRGB:
  - !<View> {name: Raw, colorspace: raw}
colorspaces:
  - !<ColorSpace>
      name: rAw
      isdata: true
  - !<ColorSpace>
      name: cs1
  - !<ColorSpace>
      name: cs2
)" };

        std::istringstream is;
        is.str(config_v1_no_default);
        OCIO::ConstConfigRcPtr constConfig;
        OCIO_CHECK_NO_THROW(constConfig = OCIO::Config::CreateFromStream(is));
        OCIO::ConfigRcPtr config = constConfig->createEditableCopy();

        OCIO_CHECK_NO_THROW(config->sanityCheck());
        config->setMajorVersion(2);

        // Default rule is using 'Default' role that does not exist.
        OCIO_CHECK_THROW_WHAT(config->sanityCheck(), OCIO::Exception, "rule named 'Default' is "
                              "referencing color space 'default' that does not exist");

        config = constConfig->createEditableCopy();
        OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 3);

        // Upgrading is making sure that a valid v1 config will be a valid v2 config.
        config->upgradeToLatestVersion();

        OCIO_CHECK_EQUAL(config->getMajorVersion(), 2);
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        // 'Default' does not exist, 'Raw' is a data color-space.
        OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 3);
        auto rules = config->getFileRules();
        const auto numRules = rules->getNumEntries();
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(numRules - 1)), "rAw");
    }
}

OCIO_ADD_TEST(FileRules, config_v2_wrong_rule)
{
    // 2 default rules.
    {
        std::string config_v2{ g_config };
        config_v2 += R"(file_rules:
  - !<Rule> {name: Default, colorspace: default}
  - !<Rule> {name: Default, colorspace: cs1}
)";
        std::istringstream is;
        is.str(config_v2);
        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "Default rule has to be the last rule");

    }
    // Default rule parameters.
    {
        std::string config_v2{ g_config };
        config_v2 += R"(file_rules:
  - !<Rule> {name: Default, colorspace: cs2, regex: ".*\\.TIF?F$"}
)";
        std::istringstream is;
        is.str(config_v2);
        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "'Default' rule can't use pattern, extension or regex.");

    }
    // Default should be at the end.
    {
        std::string config_v2{ g_config };
        config_v2 += R"(file_rules:
  - !<Rule> {name: Default, colorspace: raw}
  - !<Rule> {name: Custom, colorspace: cs1, pattern: "*", extension: jpg}
)";
        std::istringstream is;
        is.str(config_v2);
        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "Default rule has to be the last rule");

    }
    // 2 parse rules.
    {
        std::string config_v2{ g_config };
        config_v2 += R"(file_rules:
  - !<Rule> {name: ColorSpaceNamePathSearch}
  - !<Rule> {name: ColorSpaceNamePathSearch}
  - !<Rule> {name: Default, colorspace: cs1}
)";
        std::istringstream is;
        is.str(config_v2);
        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "A rule named 'ColorSpaceNamePathSearch' already exists");

    }
    // Rule with regex & glob.
    {
        std::string config_v2{ g_config };
        config_v2 += R"(file_rules:
  - !<Rule> {name: Custom, colorspace: cs1, pattern: "*", extension: jpg, regex: ".*\\.TIF?F$"}
  - !<Rule> {name: Default, colorspace: cs1}
)";
        std::istringstream is;
        is.str(config_v2);
        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              R"(can't use regex '.*\.TIF?F$' and pattern & extension)");

    }
}

OCIO_ADD_TEST(FileRules, rule_move)
{
    std::istringstream is;
    is.str(g_config);
    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());

    OCIO_CHECK_NO_THROW(config->sanityCheck());
    auto rules = config->getFileRules()->createEditableCopy();

    OCIO_CHECK_NO_THROW(rules->insertRule(0, "rule0", "cs1", g_filePattern, g_fileExt));
    OCIO_CHECK_NO_THROW(rules->insertRule(1, "rule1", "cs1", g_filePattern, g_fileExt));
    OCIO_CHECK_NO_THROW(rules->insertRule(2, "rule2", "cs1", g_filePattern, g_fileExt));
    OCIO_CHECK_NO_THROW(rules->insertRule(3, "rule3", "cs1", g_filePattern, g_fileExt));
    OCIO_CHECK_NO_THROW(rules->insertRule(4, "rule4", "cs1", g_filePattern, g_fileExt));
    OCIO_CHECK_EQUAL(rules->getNumEntries(), 6);

    OCIO_CHECK_THROW_WHAT(rules->increaseRulePriority(0), OCIO::Exception,
                          "may not be moved to index '-1'");
    OCIO_CHECK_THROW_WHAT(rules->decreaseRulePriority(4), OCIO::Exception,
                          "may not be moved to index '5'");

    OCIO_CHECK_THROW_WHAT(rules->increaseRulePriority(5), OCIO::Exception, "is the default rule");
    OCIO_CHECK_THROW_WHAT(rules->decreaseRulePriority(5), OCIO::Exception, "is the default rule");

    OCIO_CHECK_NO_THROW(rules->decreaseRulePriority(2));
    OCIO_CHECK_EQUAL(rules->getNumEntries(), 6);
    OCIO_CHECK_EQUAL(std::string(rules->getName(2)), "rule3");
    OCIO_CHECK_EQUAL(std::string(rules->getName(3)), "rule2");

    OCIO_CHECK_NO_THROW(rules->increaseRulePriority(3));
    OCIO_CHECK_EQUAL(rules->getNumEntries(), 6);
    OCIO_CHECK_EQUAL(std::string(rules->getName(2)), "rule2");
    OCIO_CHECK_EQUAL(std::string(rules->getName(3)), "rule3");

    OCIO_CHECK_NO_THROW(rules->decreaseRulePriority(2));
    OCIO_CHECK_NO_THROW(rules->decreaseRulePriority(3));
    OCIO_CHECK_EQUAL(rules->getNumEntries(), 6);
    OCIO_CHECK_EQUAL(std::string(rules->getName(2)), "rule3");
    OCIO_CHECK_EQUAL(std::string(rules->getName(3)), "rule4");
    OCIO_CHECK_EQUAL(std::string(rules->getName(4)), "rule2");

    OCIO_CHECK_NO_THROW(rules->increaseRulePriority(4));
    OCIO_CHECK_NO_THROW(rules->increaseRulePriority(3));
    OCIO_CHECK_EQUAL(rules->getNumEntries(), 6);
    OCIO_CHECK_EQUAL(std::string(rules->getName(2)), "rule2");
    OCIO_CHECK_EQUAL(std::string(rules->getName(3)), "rule3");
    OCIO_CHECK_EQUAL(std::string(rules->getName(4)), "rule4");
}


OCIO_ADD_TEST(FileRules, clone)
{
    // Validate that 'FileRules::createEditableCopy()' does not share FileRule instances.

    auto config    = OCIO::Config::CreateRaw()->createEditableCopy();
    auto fileRules = config->getFileRules()->createEditableCopy();
    OCIO_CHECK_NO_THROW(fileRules->insertRule(0, "rule", "raw", "*", "a"));
    OCIO_REQUIRE_EQUAL(fileRules->getNumEntries(), 3);

    auto newFileRules = fileRules->createEditableCopy();
    OCIO_REQUIRE_EQUAL(newFileRules->getNumEntries(), 3);

    OCIO_CHECK_EQUAL(std::string(newFileRules->getPattern(0)), std::string(fileRules->getPattern(0)));

    OCIO_CHECK_NO_THROW(newFileRules->setPattern(0, "*A"));
    OCIO_CHECK_NE(std::string(newFileRules->getPattern(0)), std::string(fileRules->getPattern(0)));

    OCIO_CHECK_NO_THROW(fileRules->setPattern(0, "*B"));
    OCIO_CHECK_NE(std::string(newFileRules->getPattern(0)), std::string(fileRules->getPattern(0)));

    OCIO_CHECK_NO_THROW(newFileRules->setPattern(0, "*"));
    OCIO_CHECK_NO_THROW(fileRules->setPattern(0, "*"));
    OCIO_CHECK_EQUAL(std::string(newFileRules->getPattern(0)), std::string(fileRules->getPattern(0)));
}

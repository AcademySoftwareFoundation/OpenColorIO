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
    // From a v1 config create valid file rules.
  
    {
        static const char CONFIG[] = 
            "ocio_profile_version: 1\n"
            "\n"
            "search_path: \"\"\n"
            "strictparsing: false\n"
            "luma: [0.2126, 0.7152, 0.0722]\n"
            "\n"
            "roles:\n"
            "  default: raw\n"
            "\n"
            "displays:\n"
            "  sRGB:\n"
            "    - !<View> {name: Raw, colorspace: raw}\n"
            "\n"
            "active_displays: []\n"
            "active_views: []\n"
            "\n"
            "colorspaces:\n"
            "  - !<ColorSpace>\n"
            "    name: raw\n"
            "    family: \"\"\n"
            "    equalitygroup: \"\"\n"
            "    bitdepth: unknown\n"
            "    isdata: false\n"
            "    allocation: uniform\n";

        std::istringstream is;
        is.str(CONFIG);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        OCIO_REQUIRE_EQUAL(config->getFileRules()->getNumEntries(), 2);
        OCIO_CHECK_EQUAL(std::string(config->getFileRules()->getName(0)), OCIO::FileRules::FilePathSearchRuleName);
        OCIO_CHECK_EQUAL(std::string(config->getFileRules()->getName(1)), OCIO::FileRules::DefaultRuleName);

        OCIO_CHECK_EQUAL(std::string(config->getFileRules()->getColorSpace(1)), std::string("default"));

        // Check that the file rules are not saved in a v1 config.
        std::stringstream ss;
        OCIO_CHECK_NO_THROW(ss << *config.get());
        OCIO_CHECK_EQUAL(ss.str(), std::string(CONFIG));
    }

    // Test fallback 1: The default role is missing and there is a data color space named 'raw'.

    {
        static const char CONFIG[] = 
            "ocio_profile_version: 1\n"
            "displays:\n"
            "  sRGB:\n"
            "    - !<View> {name: Raw, colorspace: raw}\n"
            "colorspaces:\n"
            "  - !<ColorSpace>\n"
            "    name: cs2\n"
            "  - !<ColorSpace>\n"
            "    name: raw\n"
            "    isdata: true\n";

        std::istringstream is;
        is.str(CONFIG);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        OCIO_REQUIRE_EQUAL(config->getFileRules()->getNumEntries(), 2);
        OCIO_CHECK_EQUAL(std::string(config->getFileRules()->getName(0)), OCIO::FileRules::FilePathSearchRuleName);
        OCIO_CHECK_EQUAL(std::string(config->getFileRules()->getName(1)), OCIO::FileRules::DefaultRuleName);

        OCIO_CHECK_EQUAL(std::string(config->getFileRules()->getColorSpace(1)), std::string("raw"));
    }

    // Test fallback 2: The default role is missing and there is a data color space.
    // But 'raw' is not a data color space.

    {
        static const char CONFIG[] = 
            "ocio_profile_version: 1\n"
            "displays:\n"
            "  sRGB:\n"
            "    - !<View> {name: Raw, colorspace: raw}\n"
            "colorspaces:\n"
            "  - !<ColorSpace>\n"
            "    name: cs2\n"
            "  - !<ColorSpace>\n"
            "    name: raw\n"
            "  - !<ColorSpace>\n"
            "    name: cs3\n"
            "    isdata: true\n";

        std::istringstream is;
        is.str(CONFIG);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        OCIO_REQUIRE_EQUAL(config->getFileRules()->getNumEntries(), 2);
        OCIO_CHECK_EQUAL(std::string(config->getFileRules()->getName(0)), OCIO::FileRules::FilePathSearchRuleName);
        OCIO_CHECK_EQUAL(std::string(config->getFileRules()->getName(1)), OCIO::FileRules::DefaultRuleName);

        OCIO_CHECK_EQUAL(std::string(config->getFileRules()->getColorSpace(1)), std::string("cs3"));
    }

    // Test fallback 3: The default role is missing and there is no data color space but there is
    // an active color space.

    {
        static const char CONFIG[] = 
            "ocio_profile_version: 1\n"
            "displays:\n"
            "  sRGB:\n"
            "    - !<View> {name: Raw, colorspace: raw}\n"
            "colorspaces:\n"
            "  - !<ColorSpace>\n"
            "    name: cs2\n"
            "  - !<ColorSpace>\n"
            "    name: raw\n";

        std::istringstream is;
        is.str(CONFIG);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        OCIO_REQUIRE_EQUAL(config->getFileRules()->getNumEntries(), 2);
        OCIO_CHECK_EQUAL(std::string(config->getFileRules()->getName(0)), OCIO::FileRules::FilePathSearchRuleName);
        OCIO_CHECK_EQUAL(std::string(config->getFileRules()->getName(1)), OCIO::FileRules::DefaultRuleName);

        OCIO_CHECK_EQUAL(std::string(config->getFileRules()->getColorSpace(1)), std::string("cs2"));
    }

    // Test that getColorSpaceFromFilePath works even with a v1 config (that pre-dates the
    // introduction of file rules).

    {
        static const char CONFIG[] = 
            "ocio_profile_version: 1\n"
            "roles:\n"
            "  default: raw\n"
            "displays:\n"
            "  sRGB:\n"
            "    - !<View> {name: Raw, colorspace: raw}\n"
            "colorspaces:\n"
            "  - !<ColorSpace>\n"
            "    name: cs2\n"
            "  - !<ColorSpace>\n"
            "    name: raw\n"
            "  - !<ColorSpace>\n"
            "    name: cs3\n"
            "    isdata: true\n";

        std::istringstream is;
        is.str(CONFIG);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        OCIO_REQUIRE_EQUAL(config->getFileRules()->getNumEntries(), 2);
        OCIO_CHECK_EQUAL(std::string(config->getFileRules()->getName(0)), OCIO::FileRules::FilePathSearchRuleName);
        OCIO_CHECK_EQUAL(std::string(config->getFileRules()->getName(1)), OCIO::FileRules::DefaultRuleName);

        OCIO_CHECK_EQUAL(std::string(config->getFileRules()->getColorSpace(1)), std::string("default"));

        // Test the file path search rule i.e. implemented using Config::parseColorSpaceFromString()

        size_t ruleIndex = size_t(-1);

        OCIO_CHECK_EQUAL(std::string(config->getColorSpaceFromFilepath("/usr/cs2_file.exr", ruleIndex)),
                         std::string("cs2"));
        OCIO_CHECK_EQUAL(ruleIndex, 0);
        OCIO_CHECK_ASSERT(!config->filepathOnlyMatchesDefaultRule("/usr/cs2_file.exr"));

        OCIO_CHECK_EQUAL(std::string(config->getColorSpaceFromFilepath("/usr/cs3/file.exr", ruleIndex)),
                         std::string("cs3"));
        OCIO_CHECK_EQUAL(ruleIndex, 0);
        OCIO_CHECK_ASSERT(!config->filepathOnlyMatchesDefaultRule("/usr/cs3/file.exr"));

        OCIO_CHECK_EQUAL(std::string(config->getColorSpaceFromFilepath("/usr/cs3/cs2_file.exr", ruleIndex)),
                         std::string("cs2"));
        OCIO_CHECK_EQUAL(ruleIndex, 0);
        OCIO_CHECK_ASSERT(!config->filepathOnlyMatchesDefaultRule("/usr/cs3/cs2_file.exr"));

        // Test that it fallbacks to the default rule when nothing found.

        OCIO_CHECK_EQUAL(std::string(config->getColorSpaceFromFilepath("/usr/file.exr", ruleIndex)),
                         std::string("default"));
        OCIO_CHECK_EQUAL(ruleIndex, 1);
        OCIO_CHECK_ASSERT(config->filepathOnlyMatchesDefaultRule("/usr/file.exr"));
    }
}

OCIO_ADD_TEST(FileRules, config_read_only)
{
    auto config = OCIO::Config::CreateRaw();
    auto fileRules = config->getFileRules();
    OCIO_REQUIRE_EQUAL(fileRules->getNumEntries(), 1);
    OCIO_CHECK_EQUAL(std::string(fileRules->getName(0)), OCIO::FileRules::DefaultRuleName);
    OCIO_CHECK_EQUAL(fileRules->getIndexForRule(OCIO::FileRules::DefaultRuleName), 0);
    OCIO_CHECK_EQUAL(std::string(fileRules->getPattern(0)), "");
    OCIO_CHECK_EQUAL(std::string(fileRules->getExtension(0)), "");
    OCIO_CHECK_EQUAL(std::string(fileRules->getRegex(0)), "");
    OCIO_CHECK_EQUAL(std::string(fileRules->getColorSpace(0)), OCIO::ROLE_DEFAULT);

    OCIO_CHECK_THROW_WHAT(fileRules->getName(1), 
                          OCIO::Exception, 
                          "rule index '1' invalid. There are only '1' rules.");

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
    OCIO_REQUIRE_EQUAL(fileRules->getNumEntries(), 1);
    OCIO_CHECK_NO_THROW(fileRules->insertRule(0, "rule", "raw", "*", "a"));
    OCIO_REQUIRE_EQUAL(fileRules->getNumEntries(), 2);
    OCIO_CHECK_NO_THROW(fileRules->insertRule(0, "TIFF rule", "raw", R"(.*\.TIF?F$)"));
    OCIO_REQUIRE_EQUAL(fileRules->getNumEntries(), 3);
    OCIO_CHECK_THROW_WHAT(fileRules->insertRule(0, "rule", "raw", "*", "b"),
                          OCIO::Exception, "A rule named 'rule' already exists");
    OCIO_CHECK_THROW_WHAT(fileRules->insertRule(4, "rule2", "raw", "*", "a"),
                          OCIO::Exception, "rule index '4' invalid");
    OCIO_CHECK_THROW_WHAT(fileRules->removeRule(3), OCIO::Exception, "invalid");
    OCIO_CHECK_THROW_WHAT(fileRules->removeRule(2), OCIO::Exception, "is the default rule");
    OCIO_CHECK_NO_THROW(fileRules->removeRule(1));
    OCIO_CHECK_NO_THROW(fileRules->removeRule(0));
    OCIO_CHECK_EQUAL(fileRules->getNumEntries(), 1);

    OCIO_CHECK_THROW_WHAT(fileRules->insertRule(0, OCIO::FileRules::FilePathSearchRuleName,
                                                "colorspace", "", ""),
                          OCIO::Exception, "does not accept any color space");
    OCIO_CHECK_THROW_WHAT(fileRules->insertRule(0, OCIO::FileRules::FilePathSearchRuleName,
                                                "", "pattern", ""),
                          OCIO::Exception, "do not accept any pattern");
    OCIO_CHECK_THROW_WHAT(fileRules->insertRule(0, OCIO::FileRules::FilePathSearchRuleName,
                                                "", "", "extension"),
                          OCIO::Exception, "do not accept any extension");

    OCIO_CHECK_NO_THROW(fileRules->insertRule(0, OCIO::FileRules::FilePathSearchRuleName, "", "", ""));

    OCIO_CHECK_THROW_WHAT(fileRules->insertRule(0, OCIO::FileRules::FilePathSearchRuleName,
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
    OCIO_CHECK_NO_THROW(fileRules->insertPathSearchRule(0));

    // Adds a rule with empty name.
    OCIO_CHECK_THROW_WHAT(fileRules->insertRule(0, nullptr, "raw", "*", "a"),
                          OCIO::Exception, "rule should have a non-empty name");

    // Pattern and extension can't be null.
    OCIO_CHECK_THROW_WHAT(fileRules->insertRule(0, "rule", "raw", nullptr, "a"),
                          OCIO::Exception, "file name pattern is empty");
    OCIO_CHECK_THROW_WHAT(fileRules->insertRule(0, "rule", "raw", "*", nullptr),
                          OCIO::Exception, "file extension pattern is empty");

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
    OCIO_REQUIRE_EQUAL(fileRules->getNumEntries(), 1);
    OCIO_CHECK_NO_THROW(fileRules->insertRule(0, "rule", "raw", "*", "a"));
    OCIO_REQUIRE_EQUAL(fileRules->getNumEntries(), 2);
    OCIO_CHECK_EQUAL(fileRules->getNumCustomKeys(0), 0);
    OCIO_CHECK_EQUAL(fileRules->getNumCustomKeys(1), 0);
    OCIO_CHECK_THROW_WHAT(fileRules->getNumCustomKeys(2), OCIO::Exception,
                          "rule index '2' invalid");
    OCIO_CHECK_THROW_WHAT(fileRules->getCustomKeyName(0, 0), OCIO::Exception,
                          "Key index '0' is invalid");
    OCIO_CHECK_THROW_WHAT(fileRules->getCustomKeyName(1, 0), OCIO::Exception,
                          "Key index '0' is invalid");
    OCIO_CHECK_THROW_WHAT(fileRules->getCustomKeyValue(0, 0), OCIO::Exception,
                          "Key index '0' is invalid");
    OCIO_CHECK_THROW_WHAT(fileRules->getCustomKeyValue(1, 0), OCIO::Exception,
                          "Key index '0' is invalid");
    OCIO_CHECK_NO_THROW(fileRules->setCustomKey(0, "key", "val"));
    OCIO_CHECK_NO_THROW(fileRules->setCustomKey(1, "keyDef", "valDef"));
    OCIO_CHECK_THROW_WHAT(fileRules->setCustomKey(0, nullptr, "val"), OCIO::Exception,
                          "Key has to be a non-empty string");
    OCIO_CHECK_THROW_WHAT(fileRules->setCustomKey(0, "", "val"), OCIO::Exception,
                          "Key has to be a non-empty string");
    OCIO_REQUIRE_EQUAL(fileRules->getNumCustomKeys(0), 1);
    OCIO_REQUIRE_EQUAL(fileRules->getNumCustomKeys(1), 1);
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

environment:
  {}
search_path: ""
strictparsing: false
luma: [0.2126, 0.7152, 0.0722]

roles:
  default: raw

file_rules:
  - !<Rule> {name: rule, colorspace: raw, pattern: "*", extension: a, custom: {4: val4, key1: new val, key2: val2, key3: 3}}
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
    description: A raw color space. Conversions to and from this space are no-ops.
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

    OCIO_REQUIRE_EQUAL(rules_reloaded->getNumEntries(), 2);

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
    OCIO_CHECK_EQUAL(std::string(rules_reloaded->getCustomKeyName(1, 0)), "keyDef");
    OCIO_CHECK_EQUAL(std::string(rules_reloaded->getCustomKeyValue(1, 0)), "valDef");
}

OCIO_ADD_TEST(FileRules, config_rule_u8)
{
    auto configRaw = OCIO::Config::CreateRaw();
    auto fr = configRaw->getFileRules();
    auto fileRules = fr->createEditableCopy();
    OCIO_REQUIRE_EQUAL(fileRules->getNumEntries(), 1);
    OCIO_CHECK_NO_THROW(fileRules->insertRule(0, U8("éÀÂÇÉÈç$€"), "raw", "*", "a"));
    OCIO_REQUIRE_EQUAL(fileRules->getNumEntries(), 2);
    OCIO_CHECK_NO_THROW(fileRules->setCustomKey(0, U8("key£"), U8("val€")));
    OCIO_CHECK_EQUAL(std::string(fileRules->getCustomKeyName(0, 0)), U8("key£"));
    OCIO_CHECK_EQUAL(std::string(fileRules->getCustomKeyValue(0, 0)), U8("val€"));

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

    OCIO_REQUIRE_EQUAL(rules_reloaded->getNumEntries(), 2);

    OCIO_CHECK_EQUAL(std::string(fileRules->getName(0)), U8("éÀÂÇÉÈç$€"));

    OCIO_REQUIRE_EQUAL(rules_reloaded->getNumCustomKeys(0), 1);
    OCIO_CHECK_EQUAL(std::string(fileRules->getCustomKeyName(0, 0)), U8("key£"));
    OCIO_CHECK_EQUAL(std::string(fileRules->getCustomKeyValue(0, 0)), U8("val€"));
}

namespace
{

constexpr char g_config[] = { R"(ocio_profile_version: 2
environment:
  {}
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

    OCIO_CHECK_NO_THROW(config->validate());
    auto rules = config->getFileRules()->createEditableCopy();
    OCIO_REQUIRE_EQUAL(rules->getNumEntries(), 1);

    OCIO_CHECK_NO_THROW(rules->insertRule(0, g_name, "cs1", g_filePattern, g_fileExt));
    config->setFileRules(rules);
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO_CHECK_NO_THROW(rules->setColorSpace(0, "role1"));
    config->setFileRules(rules);
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO_CHECK_NO_THROW(rules->setColorSpace(0, "invalid_color_space"));
    config->setFileRules(rules);
    OCIO_CHECK_THROW_WHAT(config->validate(), OCIO::Exception, "rule named 'rule1' is referencing "
                          "'invalid_color_space' that is neither a color space nor a named "
                          "transform");
}

OCIO_ADD_TEST(FileRules, pattern_error)
{
    auto configRaw = OCIO::Config::CreateRaw();
    auto fr = configRaw->getFileRules();
    auto rules = fr->createEditableCopy();

    OCIO_CHECK_NO_THROW(rules->insertRule(0, OCIO::FileRules::FilePathSearchRuleName, "", "", ""));
    OCIO_CHECK_NO_THROW(rules->insertRule(0, "new rule", "raw", "*", "a"));
    OCIO_REQUIRE_EQUAL(rules->getNumEntries(), 3);

    OCIO_CHECK_THROW_WHAT(rules->setPattern(0, nullptr), OCIO::Exception, "file name pattern is empty");
    OCIO_CHECK_THROW_WHAT(rules->setPattern(0, ""),      OCIO::Exception, "file name pattern is empty");

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

OCIO_ADD_TEST(FileRules, with_defaults)
{
    // Validate some default behaviours.

    auto config = OCIO::Config::CreateRaw()->createEditableCopy();
    auto rules = config->getFileRules()->createEditableCopy();

    OCIO_CHECK_NO_THROW(rules->insertRule(0, OCIO::FileRules::FilePathSearchRuleName, "", "", ""));
    OCIO_REQUIRE_EQUAL(rules->getNumEntries(), 2);

    // Null or empty pattern and/or extension throw.

    OCIO_CHECK_THROW(rules->insertRule(0, "new rule2", "raw", nullptr, "a"), OCIO::Exception);
    OCIO_CHECK_THROW(rules->insertRule(0, "new rule2", "raw", "", "a"), OCIO::Exception);

    OCIO_CHECK_THROW(rules->insertRule(0, "new rule3", "raw", "a", nullptr), OCIO::Exception);
    OCIO_CHECK_THROW(rules->insertRule(0, "new rule3", "raw", "a", ""), OCIO::Exception);

    // Null or empty regex throws.

    OCIO_CHECK_THROW(rules->insertRule(0, "new rule2", "raw", ""), OCIO::Exception);
    OCIO_CHECK_THROW(rules->insertRule(0, "new rule2", "raw", nullptr), OCIO::Exception);
}

OCIO_ADD_TEST(FileRules, extension_error)
{
    auto configRaw = OCIO::Config::CreateRaw();
    auto fr = configRaw->getFileRules();
    auto rules = fr->createEditableCopy();

    OCIO_CHECK_NO_THROW(rules->insertRule(0, OCIO::FileRules::FilePathSearchRuleName, "", "", ""));
    OCIO_CHECK_NO_THROW(rules->insertRule(0, "new rule", "raw", "*", "a"));
    OCIO_REQUIRE_EQUAL(rules->getNumEntries(), 3);

    OCIO_CHECK_THROW_WHAT(rules->setExtension(0, nullptr), OCIO::Exception,
                          "file extension pattern is empty");
    OCIO_CHECK_THROW_WHAT(rules->setExtension(0, ""), OCIO::Exception,
                          "file extension pattern is empty");
}

OCIO_ADD_TEST(FileRules, multiple_rules)
{
    std::istringstream is;
    is.str(g_config);
    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());

    OCIO_CHECK_NO_THROW(config->validate());
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
    config->getColorSpaceFromFilepath("", rulePosition);
    OCIO_CHECK_EQUAL(rulePosition, 1); // Default rule.
    config->getColorSpaceFromFilepath(nullptr, rulePosition);
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
    OCIO_CHECK_NO_THROW(rules->insertPathSearchRule(0));
    OCIO_CHECK_NO_THROW(rules->insertRule(1, "dpx file", "raw", "*", "dpx"));
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

    // Note that parseColorSpaceFromString (used by the pathSearch rule) is tested with aliases
    // and inactive color spaces in OCIO_ADD_TEST(Config, use_alias).
}

OCIO_ADD_TEST(FileRules, rules_priority)
{
    std::istringstream is;
    is.str(g_config);
    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());
    auto rules = config->getFileRules()->createEditableCopy();
    OCIO_CHECK_NO_THROW(rules->insertRule(0, "pattern dpx file", "raw", "*cs2*", "dpx"));
    OCIO_CHECK_NO_THROW(rules->insertPathSearchRule(1));
    OCIO_CHECK_NO_THROW(rules->insertRule(2, "regex rule", "cs5", ".*cs5.dpx"));
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
environment:
  {}
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
    OCIO_CHECK_EQUAL(std::string(rules->getName(0)), OCIO::FileRules::DefaultRuleName);
    OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(0)), "cs1");
    OCIO_CHECK_EQUAL(std::string(config->getColorSpaceFromFilepath("anything")), "cs1");

    // The color space of the default role is preserved.
    auto cs = config->getColorSpace(OCIO::ROLE_DEFAULT);
    OCIO_CHECK_EQUAL(std::string("raw"), cs->getName());
}

OCIO_ADD_TEST(FileRules, config_no_default_role)
{
    // Test with a config that does not have a default role, nor a default color space.
    // Default rule points to an existing color space.
    constexpr char configNoDefault[] = { R"(ocio_profile_version: 2
environment:
  {}
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
file_rules:
  - !<Rule> {name: Default, colorspace: cs1}
)" };

    // As a warning message is expected, please mute it.
    OCIO::LogGuard guard;

    std::istringstream is;
    is.str(configNoDefault);
    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));

    OCIO_CHECK_ASSERT(guard.output().empty());

    OCIO_CHECK_NO_THROW(config->validate());
}

OCIO_ADD_TEST(FileRules, config_default_no_colorspace)
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
  - !<Rule> {name: Default}
)" };

    std::istringstream is;
    is.str(configDefaultMissmatch);
    OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                          "'Default' rule cannot have an empty color space name");
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

OCIO_ADD_TEST(FileRules, config_filerule_no_colorspace)
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
  - !<Rule> {name: Custom, pattern: "*", extension: jpg}
  - !<Rule> {name: Default, colorspace: default}
)" };

    std::istringstream is;
    is.str(configNoDefaultRule);
    OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                          "File rule 'Custom' cannot have an empty color space name");
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

OCIO_ADD_TEST(FileRules, config_v1_to_v2_from_file)
{
    // The unit test checks the file rules when loading a v1 config, the upgrade from v1 to v2
    // and finally, the use of file rules with the upgraded v2 in-memory config.
    //
    // Note: For now, only the file rules and the versions are impacted by the upgrade.

    {
        // Test the common use case i.e. read a v1 config file and upgrade it to v2.

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
        OCIO_CHECK_NO_THROW(config->validate());

        // Check the version.

        OCIO_CHECK_EQUAL(config->getMajorVersion(), 1);

        // Check the file rules.

        auto rules = config->getFileRules();
        OCIO_CHECK_EQUAL(rules->getNumEntries(), 2);
        OCIO_CHECK_EQUAL(std::string(rules->getName(0)), OCIO::FileRules::FilePathSearchRuleName);
        OCIO_CHECK_EQUAL(std::string(rules->getName(1)), OCIO::FileRules::DefaultRuleName);
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(1)), "default");

        // Check the v1 in-memory file rules are working.

        // It checks that the rule 'FileRules::FilePathSearchRuleName' exists.
        OCIO_CHECK_EQUAL(std::string(config->getColorSpaceFromFilepath("/usr/cs2_file.exr")), "cs2");
        // It checks that the rule 'Default' exists.
        OCIO_CHECK_EQUAL(std::string(config->getColorSpaceFromFilepath("/usr/file.exr")), "default");

        // Upgrading is making sure to build a valid v2 config.

        config->upgradeToLatestVersion();
        
        {
          OCIO::LogGuard logGuard;
          OCIO_CHECK_NO_THROW(config->validate());
          // Check that the log contains the expected error messages for the missing roles and mute 
          // them so that (only) those messages don't appear in the test output.
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteSceneLinearRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteCompositingLogRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteColorTimingRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteAcesInterchangeRoleError(logGuard));
          // If there are any unexpected log messages, print them to the shell.
          logGuard.print();
        }

        // Check the new version.

        OCIO_CHECK_EQUAL(config->getMajorVersion(), 2);

        // Check the new file rules.

        rules = config->getFileRules();
        OCIO_CHECK_EQUAL(rules->getNumEntries(), 2);
        OCIO_CHECK_EQUAL(std::string(rules->getName(0)), OCIO::FileRules::FilePathSearchRuleName);
        OCIO_CHECK_EQUAL(std::string(rules->getName(1)), OCIO::FileRules::DefaultRuleName);
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(1)), "default");

        // Check the v1 in-memory file rules are working.

        OCIO_CHECK_EQUAL(std::string(config->getColorSpaceFromFilepath("/usr/cs2_file.exr")), "cs2");
        OCIO_CHECK_EQUAL(std::string(config->getColorSpaceFromFilepath("/usr/file.exr")), "default");
    }

    {
        // The default role is missing and there is a 'data' color space named rAw.

        constexpr char config_v1[] = { R"(ocio_profile_version: 1
strictparsing: true
roles:
  role1: cs1
  role2: cs2
displays:
  sRGB:
  - !<View> {name: Raw, colorspace: rAw}
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
        is.str(config_v1);
        OCIO::ConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());
        OCIO_CHECK_NO_THROW(config->validate());

        // Check the version.

        OCIO_CHECK_EQUAL(config->getMajorVersion(), 1);

        // Check the file rules.

        auto rules = config->getFileRules();
        OCIO_CHECK_EQUAL(rules->getNumEntries(), 2);
        OCIO_CHECK_EQUAL(std::string(rules->getName(0)), OCIO::FileRules::FilePathSearchRuleName);
        OCIO_CHECK_EQUAL(std::string(rules->getName(1)), OCIO::FileRules::DefaultRuleName);
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(1)), "rAw");

        // Check the v1 in-memory file rules are working.

        OCIO_CHECK_EQUAL(std::string(config->getColorSpaceFromFilepath("/usr/cs2_file.exr")), "cs2");
        OCIO_CHECK_EQUAL(std::string(config->getColorSpaceFromFilepath("/usr/file.exr")), "rAw");

        // Upgrading is making sure to build a valid v2 config.

        config->upgradeToLatestVersion();

        {
          OCIO::LogGuard logGuard;
          OCIO_CHECK_NO_THROW(config->validate());
          // Ignore (only) the errors logged regarding the missing roles that are required in 
          // configs with version >= 2.2.
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteSceneLinearRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteCompositingLogRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteColorTimingRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteAcesInterchangeRoleError(logGuard));
          // If there are any unexpected log messages, print them to the shell.
          logGuard.print();
        }

        // Check the new version.

        OCIO_CHECK_EQUAL(config->getMajorVersion(), 2);

        // Check the new file rules.

        rules = config->getFileRules();
        OCIO_CHECK_EQUAL(rules->getNumEntries(), 2);
        OCIO_CHECK_EQUAL(std::string(rules->getName(0)), OCIO::FileRules::FilePathSearchRuleName);
        OCIO_CHECK_EQUAL(std::string(rules->getName(1)), OCIO::FileRules::DefaultRuleName);
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(1)), "rAw");

        OCIO_CHECK_EQUAL(std::string(config->getColorSpaceFromFilepath("/usr/cs2_file.exr")), "cs2");
        OCIO_CHECK_EQUAL(std::string(config->getColorSpaceFromFilepath("/usr/file.exr")), "rAw");
    }

    {
        // The default role is missing and there is no 'data' color space so, the first
        // color space is used in v1, and the first active color space is used in v2.

        // Note that inactive color spaces do not exist in v1 explaining why the first color
        // space is used.

        constexpr char config_v1[] = { R"(ocio_profile_version: 1
strictparsing: true
roles:
  role1: cs1
  role2: cs2
displays:
  sRGB:
  - !<View> {name: Raw, colorspace: rAw}
colorspaces:
  - !<ColorSpace>
      name: cs1
  - !<ColorSpace>
      name: cs2
  - !<ColorSpace>
      name: rAw
)" };

        std::istringstream is;
        is.str(config_v1);
        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        // Check the version.

        OCIO_CHECK_EQUAL(config->getMajorVersion(), 1);

        // Check the file rules.

        auto rules = config->getFileRules();
        OCIO_CHECK_EQUAL(rules->getNumEntries(), 2);
        OCIO_CHECK_EQUAL(std::string(rules->getName(0)), OCIO::FileRules::FilePathSearchRuleName);
        OCIO_CHECK_EQUAL(std::string(rules->getName(1)), OCIO::FileRules::DefaultRuleName);
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(1)), "cs1");

        // Check the v1 in-memory file rules are working.

        OCIO_CHECK_EQUAL(std::string(config->getColorSpaceFromFilepath("/usr/cs2_file.exr")), "cs2");
        OCIO_CHECK_EQUAL(std::string(config->getColorSpaceFromFilepath("/usr/file.exr")), "cs1");

        {
            // In v2, the first active color space is then used for the 'Default' rule.

            OCIO::ConfigRcPtr cfg;
            OCIO_CHECK_NO_THROW(cfg = config->createEditableCopy());

            // Upgrading is making sure to build a valid v2 config.

            cfg->setInactiveColorSpaces("cs1");
            cfg->upgradeToLatestVersion();

            {
              OCIO::LogGuard logGuard;
              OCIO_CHECK_NO_THROW(cfg->validate());
              // Ignore (only) the errors logged regarding the missing roles that are required in 
              // configs with version >= 2.2.
              OCIO_CHECK_ASSERT(OCIO::checkAndMuteSceneLinearRoleError(logGuard));
              OCIO_CHECK_ASSERT(OCIO::checkAndMuteCompositingLogRoleError(logGuard));
              OCIO_CHECK_ASSERT(OCIO::checkAndMuteColorTimingRoleError(logGuard));
              OCIO_CHECK_ASSERT(OCIO::checkAndMuteAcesInterchangeRoleError(logGuard));
              // If there are any unexpected log messages, print them to the shell.
              logGuard.print();
            }

            // Check the new version.

            OCIO_CHECK_EQUAL(cfg->getMajorVersion(), 2);

            // Check the new file rules.

            rules = cfg->getFileRules();
            OCIO_CHECK_EQUAL(rules->getNumEntries(), 2);
            OCIO_CHECK_EQUAL(std::string(rules->getName(0)), OCIO::FileRules::FilePathSearchRuleName);
            OCIO_CHECK_EQUAL(std::string(rules->getName(1)), OCIO::FileRules::DefaultRuleName);
            OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(1)), "cs2");

            OCIO_CHECK_EQUAL(std::string(cfg->getColorSpaceFromFilepath("/usr/cs1_file.exr")), "cs1");
            OCIO_CHECK_EQUAL(std::string(cfg->getColorSpaceFromFilepath("/usr/file.exr")), "cs2");
        }

        {
            // In v2, the first color space is used for the 'Default' rule because there no active
            // color spaces.

            OCIO::ConfigRcPtr cfg;
            OCIO_CHECK_NO_THROW(cfg = config->createEditableCopy());

            // Upgrading is making sure to build a valid v2 config.

            cfg->setInactiveColorSpaces("cs1, cs2, raw");

            {
                OCIO::LogGuard l;      
        
                cfg->upgradeToLatestVersion();
                
                OCIO_CHECK_EQUAL(
                    std::string("[OpenColorIO Warning]: The default rule creation falls back to the"\
                                " first color space because no suitable color space exists.\n"), 
                    l.output());
            }

            {
              OCIO::LogGuard logGuard;
              OCIO_CHECK_NO_THROW(cfg->validate());
              // Ignore (only) the errors logged regarding the missing roles that are required in 
              // configs with version >= 2.2.
              OCIO_CHECK_ASSERT(OCIO::checkAndMuteSceneLinearRoleError(logGuard));
              OCIO_CHECK_ASSERT(OCIO::checkAndMuteCompositingLogRoleError(logGuard));
              OCIO_CHECK_ASSERT(OCIO::checkAndMuteColorTimingRoleError(logGuard));
              OCIO_CHECK_ASSERT(OCIO::checkAndMuteAcesInterchangeRoleError(logGuard));
              // If there are any unexpected log messages, print them to the shell.
              logGuard.print();
            }

            // Check the new version.

            OCIO_CHECK_EQUAL(cfg->getMajorVersion(), 2);

            // Check the new file rules.

            rules = cfg->getFileRules();
            OCIO_CHECK_EQUAL(rules->getNumEntries(), 2);
            OCIO_CHECK_EQUAL(std::string(rules->getName(0)), OCIO::FileRules::FilePathSearchRuleName);
            OCIO_CHECK_EQUAL(std::string(rules->getName(1)), OCIO::FileRules::DefaultRuleName);
            OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(1)), "cs1");

            OCIO_CHECK_EQUAL(std::string(cfg->getColorSpaceFromFilepath("/usr/raw_file.exr")), "rAw");
            OCIO_CHECK_EQUAL(std::string(cfg->getColorSpaceFromFilepath("/usr/file.exr")), "cs1");
        }
    }
}

OCIO_ADD_TEST(FileRules, config_v1_to_v2_from_memory)
{
    // The unit test checks the file rules from an in-memory v1 config, the upgrade from v1 to v2,
    // and finally, the file rules in the upgraded v2 in-memory config.
    //
    // Note: For now, only the file rules and the versions are impacted by the upgrade.

    // The following tests manually create an in-memory v1 config with faulty file rules. As the
    // config file read (which automatically updates in-memory v1 file rules like in previous tests)
    // is not used, only an explicit upgrade to the latest version, can fix the file rules.

    {
      // The default role is missing but there is an active 'data' color space.

        OCIO::ConfigRcPtr config = OCIO::Config::Create();
        config->setMajorVersion(1);
        config->addDisplayView("disp1", "view1", "cs1", nullptr);
        OCIO::ColorSpaceRcPtr cs1 = OCIO::ColorSpace::Create();
        cs1->setName("cs1");
        cs1->setIsData(true);
        config->addColorSpace(cs1);
        OCIO::ColorSpaceRcPtr raw = OCIO::ColorSpace::Create();
        raw->setName("rAw");
        config->addColorSpace(raw);
        OCIO_CHECK_NO_THROW(config->validate());  // (does not fail since the major version is 1)

        // Default rule is using 'Default' role that does not exist.
        config->setMajorVersion(2);

        {
          OCIO::LogGuard logGuard;
          OCIO_CHECK_THROW_WHAT(config->validate(), OCIO::Exception, "rule named 'Default' is "
                                "referencing 'default' that is neither a color space nor a named "
                                "transform");
          // Ignore (only) the errors logged regarding the missing roles that are required in 
          // configs with version >= 2.2.
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteSceneLinearRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteCompositingLogRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteColorTimingRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteAcesInterchangeRoleError(logGuard));
          // If there are any unexpected log messages, print them to the shell.
          logGuard.print();
        }

        // Upgrading is making sure to build a valid v2 config.
        config->setMajorVersion(1);
        config->upgradeToLatestVersion();

        {
          OCIO::LogGuard logGuard;
          OCIO_CHECK_NO_THROW(config->validate());
          // Ignore (only) the errors logged regarding the missing roles that are required in 
          // configs with version >= 2.2.
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteSceneLinearRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteCompositingLogRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteColorTimingRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteAcesInterchangeRoleError(logGuard));
          // If there are any unexpected log messages, print them to the shell.
          logGuard.print();
        }

        // Check the new version.

        OCIO_CHECK_EQUAL(config->getMajorVersion(), 2);

        // 'cs1' is an active & 'data' color space.

        auto rules = config->getFileRules();
        OCIO_CHECK_EQUAL(rules->getNumEntries(), 2);
        OCIO_CHECK_EQUAL(std::string(rules->getName(0)), OCIO::FileRules::FilePathSearchRuleName);
        OCIO_CHECK_EQUAL(std::string(rules->getName(1)), OCIO::FileRules::DefaultRuleName);
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(1)), "cs1");
    }

    // The default role is missing and there is no 'data' color space.

    {
        OCIO::ConfigRcPtr config = OCIO::Config::Create();
        config->setMajorVersion(1);
        config->addDisplayView("disp1", "view1", "cs1", nullptr);
        OCIO::ColorSpaceRcPtr cs1 = OCIO::ColorSpace::Create();
        cs1->setName("cs1");
        config->addColorSpace(cs1);
        OCIO::ColorSpaceRcPtr raw = OCIO::ColorSpace::Create();
        raw->setName("rAw");
        config->addColorSpace(raw);
        OCIO_CHECK_NO_THROW(config->validate());  // (does not fail since the major version is 1)

        // Default rule is using 'Default' role but the associated color space does not exist.
        config->setMajorVersion(2);
        
        {
          OCIO::LogGuard logGuard;
          OCIO_CHECK_THROW_WHAT(config->validate(), OCIO::Exception, "rule named 'Default' is "
                                "referencing 'default' that is neither a color space nor a named "
                                "transform");
          // Ignore (only) the errors logged regarding the missing roles that are required in 
          // configs with version >= 2.2.
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteSceneLinearRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteCompositingLogRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteColorTimingRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteAcesInterchangeRoleError(logGuard));
          // If there are any unexpected log messages, print them to the shell.
          logGuard.print();
        }

        // Upgrading is making sure to build a valid v2 config.
        config->setMajorVersion(1);
        config->upgradeToLatestVersion();

        {
          OCIO::LogGuard logGuard;
          OCIO_CHECK_NO_THROW(config->validate());
          // Ignore (only) the errors logged regarding the missing roles that are required in 
          // configs with version >= 2.2.
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteSceneLinearRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteCompositingLogRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteColorTimingRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteAcesInterchangeRoleError(logGuard));
          // If there are any unexpected log messages, print them to the shell.
          logGuard.print();
        }

        // Check the new version.

        OCIO_CHECK_EQUAL(config->getMajorVersion(), 2);

        // 'Default' role does not exist, 'Raw' is not a data color-space, so use the first active
        // color space.

        auto rules = config->getFileRules();
        OCIO_CHECK_EQUAL(rules->getNumEntries(), 2);
        OCIO_CHECK_EQUAL(std::string(rules->getName(0)), OCIO::FileRules::FilePathSearchRuleName);
        OCIO_CHECK_EQUAL(std::string(rules->getName(1)), OCIO::FileRules::DefaultRuleName);
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(1)), "cs1");
    }

    // The default role is missing and there is no 'data' & active color space. The algorithm then
    // fallbacks to the first available color space and logs a warning.

    {
        OCIO::ConfigRcPtr config = OCIO::Config::Create();
        config->setMajorVersion(1);
        config->addDisplayView("disp1", "view1", "cs1", nullptr);
        OCIO::ColorSpaceRcPtr cs1 = OCIO::ColorSpace::Create();
        cs1->setName("cs1");
        config->addColorSpace(cs1);
        OCIO_CHECK_NO_THROW(config->validate());  // (does not fail since the major version is 1)

        // Default rule is using 'Default' role but the associated color space does not exist.
        config->setInactiveColorSpaces("cs1");
        config->setMajorVersion(2);

        {
          OCIO::LogGuard logGuard;
          OCIO_CHECK_THROW_WHAT(config->validate(), OCIO::Exception, "rule named 'Default' is "
                                "referencing 'default' that is neither a color space nor a named "
                                "transform");
          // Ignore (only) the errors logged regarding the missing roles that are required in 
          // configs with version >= 2.2.
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteSceneLinearRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteCompositingLogRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteColorTimingRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteAcesInterchangeRoleError(logGuard));
          // If there are any unexpected log messages, print them to the shell.
          logGuard.print();
        }

        config->setMajorVersion(1);

        {
            OCIO::LogGuard logGuard;
            config->upgradeToLatestVersion();
            StringUtils::StringVec svec = StringUtils::SplitByLines(logGuard.output());
            OCIO_CHECK_ASSERT(
                StringUtils::Contain(
                    svec, 
                    "[OpenColorIO Warning]: The default rule creation falls back to the"\
                    " first color space because no suitable color space exists."
                )
            );
        }
        
        {
          OCIO::LogGuard logGuard; 
          OCIO_CHECK_NO_THROW(config->validate());
          // Ignore (only) the errors logged regarding the missing roles that are required in 
          // configs with version >= 2.2.
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteSceneLinearRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteCompositingLogRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteColorTimingRoleError(logGuard));
          OCIO_CHECK_ASSERT(OCIO::checkAndMuteAcesInterchangeRoleError(logGuard));
          // If there are any unexpected log messages, print them to the shell.
          logGuard.print();
        }

        // Check the new version.
        OCIO_CHECK_EQUAL(config->getMajorVersion(), 2);

        // Check the 'default' rule. As there is not 'data' or active color space, the default
        // rule is using an inactive color space.

        auto rules = config->getFileRules();
        OCIO_CHECK_EQUAL(rules->getNumEntries(), 2);
        OCIO_CHECK_EQUAL(std::string(rules->getName(0)), OCIO::FileRules::FilePathSearchRuleName);
        OCIO_CHECK_EQUAL(std::string(rules->getName(1)), OCIO::FileRules::DefaultRuleName);
        OCIO_CHECK_EQUAL(std::string(rules->getColorSpace(1)), "cs1");
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

    OCIO_CHECK_NO_THROW(config->validate());
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
    OCIO_CHECK_NO_THROW(fileRules->insertRule(0, OCIO::FileRules::FilePathSearchRuleName, "", "", ""));
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

OCIO_ADD_TEST(FileRules, isDefault)
{
    auto fileRules = OCIO::FileRules::Create();
    OCIO_CHECK_ASSERT(fileRules->isDefault());
    fileRules->setColorSpace(0, "DEFault");
    OCIO_CHECK_ASSERT(fileRules->isDefault());
    fileRules->setColorSpace(0, "raw");
    OCIO_CHECK_ASSERT(!fileRules->isDefault());
    fileRules->setColorSpace(0, "default");
    OCIO_CHECK_ASSERT(fileRules->isDefault());

    OCIO_CHECK_NO_THROW(fileRules->setCustomKey(0, "key", "val"));
    OCIO_CHECK_ASSERT(!fileRules->isDefault());
    OCIO_CHECK_NO_THROW(fileRules->setCustomKey(0, "key", ""));
    OCIO_CHECK_ASSERT(fileRules->isDefault());

    OCIO_CHECK_NO_THROW(fileRules->insertRule(0, "rule", "raw", "*", "a"));
    OCIO_CHECK_ASSERT(!fileRules->isDefault());

    const auto config = OCIO::Config::CreateRaw();
    OCIO_CHECK_ASSERT(config->getFileRules()->isDefault());
}

// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "transforms/builtins/BuiltinTransformRegistry.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(Builtins, basic)
{
    // Create an empty built-in transform registry.

    OCIO::BuiltinTransformRegistryImpl registry;
    OCIO_CHECK_EQUAL(registry.getNumBuiltins(), 0);
    OCIO_CHECK_THROW_WHAT(registry.getBuiltinStyle(0), OCIO::Exception, "Invalid index.");

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_THROW_WHAT(registry.createOps(0, ops),
                          OCIO::Exception,
                          "Invalid index.");

    // Add a built-in transform.

    auto EmptyFunctor = [](OCIO::OpRcPtrVec & /*ops*/) {};
    OCIO_CHECK_NO_THROW(registry.addBuiltin("trans1", nullptr, EmptyFunctor));

    OCIO_CHECK_EQUAL(registry.getNumBuiltins(), 1);
    OCIO_CHECK_EQUAL(OCIO::Platform::Strcasecmp(registry.getBuiltinStyle(0), "trans1"), 0);

    // Add an existing built-in transform i.e. replace the existing one.

    OCIO_CHECK_NO_THROW(registry.addBuiltin("trans1", nullptr, EmptyFunctor));
    OCIO_CHECK_EQUAL(registry.getNumBuiltins(), 1);
    OCIO_CHECK_EQUAL(OCIO::Platform::Strcasecmp(registry.getBuiltinStyle(0), "trans1"), 0);

    OCIO_CHECK_NO_THROW(registry.createOps(0, ops));
}

namespace
{

void CreateOps(const char * name, OCIO::TransformDirection dir, OCIO::OpRcPtrVec & ops, int lineNo)
{
    OCIO::ConstBuiltinTransformRegistryRcPtr reg = OCIO::BuiltinTransformRegistry::Get();

    for (size_t index = 0; index < reg->getNumBuiltins(); ++index)
    {
        if (0 == OCIO::Platform::Strcasecmp(name, reg->getBuiltinStyle(index)))
        {
            OCIO::CreateBuiltinTransformOps(ops, index, dir);
            return;
        }
    }

    std::ostringstream errorMsg;
    errorMsg << "Unknown built-in transform name '" << name << "'.";
    OCIO_CHECK_ASSERT_MESSAGE_FROM(0, errorMsg.str(), lineNo);
}

}

OCIO_ADD_TEST(Builtins, aces)
{
    // Tests only few default built-in transforms.

    OCIO::OpRcPtrVec ops;

    {
        ops.clear();
        CreateOps("IDENTITY", OCIO::TRANSFORM_DIR_FORWARD, ops, __LINE__);
        OCIO_REQUIRE_EQUAL(ops.size(), 1);
        OCIO_REQUIRE_EQUAL(std::string(ops[0]->getInfo()), "<MatrixOffsetOp>");
    }

    {
        ops.clear();
        CreateOps("UTILITY - ACES-AP0_to_CIE-XYZ-D65_BFD", OCIO::TRANSFORM_DIR_FORWARD, ops, __LINE__);
        OCIO_REQUIRE_EQUAL(ops.size(), 1);
        OCIO_REQUIRE_EQUAL(std::string(ops[0]->getInfo()), "<MatrixOffsetOp>");
    }

    {
        ops.clear();
        CreateOps("CURVE - ACEScct-LOG_to_LINEAR", OCIO::TRANSFORM_DIR_FORWARD, ops, __LINE__);
        OCIO_REQUIRE_EQUAL(ops.size(), 1);
        OCIO_REQUIRE_EQUAL(std::string(ops[0]->getInfo()), "<LogOp>");
    }
}

OCIO_ADD_TEST(Builtins, read_write)
{
    // The unit test validates the read/write and the processor creation for all the existing
    // builtin transforms.

    static constexpr char CONFIG_BUILTIN_TRANSFORMS[] {
R"(ocio_profile_version: 2.3

environment:
  {}
search_path: ""
strictparsing: true
luma: [0.2126, 0.7152, 0.0722]

roles:
  aces_interchange: test
  color_timing: test
  compositing_log: test
  default: ref
  scene_linear: test

file_rules:
  - !<Rule> {name: Default, colorspace: default}

displays:
  Disp1:
    - !<View> {name: View1, colorspace: test}

active_displays: []
active_views: []

colorspaces:
  - !<ColorSpace>
    name: ref
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    isdata: false
    allocation: uniform

  - !<ColorSpace>
    name: test
    family: ""
    equalitygroup: ""
    bitdepth: unknown
    isdata: false
    allocation: uniform
    from_scene_reference: !<GroupTransform>
      children:)" };

    // Tests all built-in transforms.

    OCIO::ConstBuiltinTransformRegistryRcPtr reg;
    OCIO_CHECK_NO_THROW(reg = OCIO::BuiltinTransformRegistry::Get());

    std::string configStr;
    configStr += CONFIG_BUILTIN_TRANSFORMS;

    // Add all the existing builtin transforms to one big GroupTransform.

    const size_t numBuiltins = reg->getNumBuiltins();
    for (size_t idx = 0; idx < numBuiltins; ++idx)
    {
        const char * style = reg->getBuiltinStyle(idx);
        configStr += "\n"
                     "        - !<BuiltinTransform> {style: ";
        configStr += style;
        configStr += "}";
    }
    configStr += "\n";

    // Load all the existing builtin transforms.

    std::istringstream iss;
    iss.str(configStr);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(iss));
    OCIO_CHECK_NO_THROW(config->validate());

    // Serialize all the existing builtin transforms.

    {
        std::ostringstream oss;
        OCIO_CHECK_NO_THROW(oss << *config.get());
        OCIO_CHECK_EQUAL(oss.str(), configStr);
    }

    // Create a processor using all the existing builtin transforms.

    {
        OCIO::ConstProcessorRcPtr processor;
        OCIO_CHECK_NO_THROW(processor = config->getProcessor("ref", "test"));
    }
}

OCIO_ADD_TEST(Builtins, version_1_validation)
{
    // The unit test validates that the config reader throws for version 1 configs containing
    // a builtin transform.

    static constexpr char CONFIG[] {
R"(ocio_profile_version: 1

search_path: ""
strictparsing: true
luma: [0.2126, 0.7152, 0.0722]

roles:
  default: ref

displays:
  Disp1:
    - !<View> {name: View1, colorspace: test}

colorspaces:
  - !<ColorSpace>
    name: ref

  - !<ColorSpace>
    name: test
    to_reference: !<BuiltinTransform> {style: ACEScct_to_ACES2065-1})" };

    std::istringstream iss;
    iss.str(CONFIG);

    OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(iss),
                          OCIO::Exception,
                          "Only config version 2 (or higher) can have BuiltinInTransform.");
}

OCIO_ADD_TEST(Builtins, version_2_validation)
{
    // The unit test validates that the config reader throws for version 2 configs containing
    // a builtin transform with the style 'ACES-LMT - ACES 1.3 Reference Gamut Compression'.

    static constexpr char CONFIG[] {
R"(ocio_profile_version: 2

environment:
  {}
search_path: ""
strictparsing: true
luma: [0.2126, 0.7152, 0.0722]

roles:
  default: ref

file_rules:
  - !<Rule> {name: Default, colorspace: default}

displays:
  Disp1:
    - !<View> {name: View1, colorspace: test}

active_displays: []
active_views: []

colorspaces:
  - !<ColorSpace>
    name: ref

  - !<ColorSpace>
    name: test
    from_scene_reference: !<BuiltinTransform> {style: ACES-LMT - ACES 1.3 Reference Gamut Compression})" };

    std::istringstream iss;
    iss.str(CONFIG);

    OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(iss),
                          OCIO::Exception,
                          "Only config version 2.1 (or higher) can have BuiltinTransform style "\
                          "'ACES-LMT - ACES 1.3 Reference Gamut Compression'.");
}

OCIO_ADD_TEST(Builtins, version_2_1_validation)
{
    // The unit test validates that the config reader checkVersionConsistency check throws for
    // version 2.1 configs containing a Builtin Transform with the 2.2 style for ARRI LogC4.

    static constexpr char CONFIG[] {
R"(ocio_profile_version: 2.1

environment:
  {}
search_path: ""
strictparsing: true
luma: [0.2126, 0.7152, 0.0722]

roles:
  default: ref

file_rules:
  - !<Rule> {name: Default, colorspace: default}

displays:
  Disp1:
    - !<View> {name: View1, colorspace: test}

active_displays: []
active_views: []

colorspaces:
  - !<ColorSpace>
    name: ref

  - !<ColorSpace>
    name: test
    from_scene_reference: !<BuiltinTransform> {style: ARRI_LOGC4_to_ACES2065-1})" };

    std::istringstream iss;
    iss.str(CONFIG);

    OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(iss),
                          OCIO::Exception,
                          "Only config version 2.2 (or higher) can have BuiltinTransform style "\
                          "'ARRI_LOGC4_to_ACES2065-1'.");
}

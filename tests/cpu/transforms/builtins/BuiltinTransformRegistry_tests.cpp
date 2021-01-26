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

    static constexpr char CONFIG_BUILTIN_TRANSFORMS[] {
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

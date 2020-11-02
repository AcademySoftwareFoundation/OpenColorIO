// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "NamedTransform.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(NamedTransform, basic)
{
    auto namedTransform = OCIO::NamedTransform::Create();
    OCIO_REQUIRE_ASSERT(namedTransform);
    const std::string name{ namedTransform->getName() };
    OCIO_CHECK_ASSERT(name.empty());
    OCIO_CHECK_ASSERT(!namedTransform->getTransform(OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_ASSERT(!namedTransform->getTransform(OCIO::TRANSFORM_DIR_INVERSE));
    const std::string newName{ "NewName" };
    namedTransform->setName(newName.c_str());
    OCIO_CHECK_EQUAL(newName, namedTransform->getName());

    auto mat = OCIO::MatrixTransform::Create();
    namedTransform->setTransform(mat, OCIO::TRANSFORM_DIR_FORWARD);
    auto fwdTransform = namedTransform->getTransform(OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_ASSERT(OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixTransform>(fwdTransform));
    // Transform is copied when set, so pointers are different.
    OCIO_CHECK_NE(namedTransform->getTransform(OCIO::TRANSFORM_DIR_FORWARD), mat);
    OCIO_CHECK_ASSERT(!namedTransform->getTransform(OCIO::TRANSFORM_DIR_INVERSE));

    auto actualFwdTransform = OCIO::NamedTransformImpl::GetTransform(namedTransform,
                                                                     OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_ASSERT(OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixTransform>(actualFwdTransform));
    auto actualInvTransform = OCIO::NamedTransformImpl::GetTransform(namedTransform,
                                                                     OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_ASSERT(OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixTransform>(actualInvTransform));

    std::ostringstream oss;
    OCIO_CHECK_NO_THROW(oss << *namedTransform);
    OCIO_CHECK_EQUAL(oss.str(), "<NamedTransform name=NewName,\n    forward=\n        "
                                "<MatrixTransform direction=forward, fileindepth=unknown, "
                                "fileoutdepth=unknown, matrix=1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1, "
                                "offset=0 0 0 0>>");

    // Test faulty cases.

    OCIO::ConfigRcPtr config = OCIO::Config::CreateRaw()->createEditableCopy();

    OCIO::NamedTransformRcPtr namedTransformInv;
    OCIO_CHECK_THROW_WHAT(config->addNamedTransform(namedTransformInv), OCIO::Exception,
                          "Named transform is null");

    namedTransformInv = OCIO::NamedTransform::Create();
    OCIO_CHECK_THROW_WHAT(config->addNamedTransform(namedTransformInv), OCIO::Exception,
                          "Named transform must have a non-empty name");

    const std::string name1{ "name" };
    namedTransformInv->setName(name1.c_str());
    OCIO_CHECK_THROW_WHAT(config->addNamedTransform(namedTransformInv), OCIO::Exception,
                          "Named transform must define at least one transform");
}

OCIO_ADD_TEST(Config, named_transform_processor)
{
    // Create a config with color spaces and named transforms.
    
    constexpr const char * Config{ R"(ocio_profile_version: 2

search_path: ""
strictparsing: false
luma: [0.2126, 0.7152, 0.0722]

roles:
  default: raw

file_rules:
  - !<Rule> {name: ColorSpaceNamePathSearch}
  - !<Rule> {name: Default, colorspace: default}

displays:
  sRGB:
    - !<View> {name: Raw, colorspace: raw}

active_displays: []
active_views: []

display_colorspaces:
  - !<ColorSpace>
    name: dcs
    isdata: false
    allocation: uniform
    from_display_reference: !<RangeTransform> {min_in_value: 0, min_out_value: 0}

colorspaces:
  - !<ColorSpace>
    name: raw
    family: raw
    bitdepth: 32f
    description: |
      A raw color space. Conversions to and from this space are no-ops.
    isdata: true
    allocation: uniform

  - !<ColorSpace>
    name: cs
    isdata: false
    allocation: uniform
    to_reference: !<RangeTransform> {max_in_value: 1, max_out_value: 1}

named_transforms:
  - !<NamedTransform>
    name: forward
    transform: !<MatrixTransform> {name: forward, offset: [0.1, 0.2, 0.3, 0.4]}

  - !<NamedTransform>
    name: inverse
    inverse_transform: !<MatrixTransform> {name: inverse, offset: [-0.2, -0.1, -0.1, 0]}

  - !<NamedTransform>
    name: both
    transform: !<MatrixTransform> {name: forward, offset: [0.1, 0.2, 0.3, 0.4]}
    inverse_transform: !<MatrixTransform> {name: inverse, offset: [-0.2, -0.1, -0.1, 0]}
)" };

    std::istringstream iss;
    iss.str(Config);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(iss));

    const std::string forward{ "forward" };
    static constexpr double offsetF[4]{ 0.1, 0.2, 0.3, 0.4 };
    const std::string inverse{ "inverse" };
    static constexpr double offsetI[4]{ -0.2, -0.1, -0.1, 0. };
    const std::string both{ "both" };
    const std::string dcsName{ "dcs" };
    const std::string csName{ "cs" };

    // Basic named transform access.
    {
        auto nt = config->getNamedTransform(forward.c_str());
        OCIO_REQUIRE_ASSERT(nt);
        // Get the transform in the wanted direction and make sure it exists (else use the other
        // transform in the other direction).
        auto tf = nt->getTransform(OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_REQUIRE_ASSERT(tf);

        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor(tf));
        OCIO_REQUIRE_ASSERT(proc);
        OCIO::GroupTransformRcPtr group;
        OCIO_CHECK_NO_THROW(group = proc->createGroupTransform());
        OCIO_REQUIRE_ASSERT(group);
        OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
        OCIO::TransformRcPtr transform;
        OCIO_CHECK_NO_THROW(transform = group->getTransform(0));
        auto matrix = OCIO_DYNAMIC_POINTER_CAST<OCIO::MatrixTransform>(transform);
        OCIO_REQUIRE_ASSERT(matrix);
        OCIO_CHECK_EQUAL(forward, proc->getTransformFormatMetadata(0).getAttributeValue(0));
        double offset[]{ 0., 0., 0., 0. };
        matrix->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], offsetF[0]);
        OCIO_CHECK_EQUAL(offset[1], offsetF[1]);
        OCIO_CHECK_EQUAL(offset[2], offsetF[2]);
    }

    // Diplay color space to named transform.
    {
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor(dcsName.c_str(), forward.c_str()));
        OCIO_REQUIRE_ASSERT(proc);
        OCIO::GroupTransformRcPtr group;
        OCIO_CHECK_NO_THROW(group = proc->createGroupTransform());
        OCIO_REQUIRE_ASSERT(group);
        OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
        OCIO::TransformRcPtr transform;
        OCIO_CHECK_NO_THROW(transform = group->getTransform(0));
        auto matrix = OCIO_DYNAMIC_POINTER_CAST<OCIO::MatrixTransform>(transform);
        OCIO_REQUIRE_ASSERT(matrix);
        // Named transform inverse transform not available, use forward transform inverted.
        OCIO_CHECK_EQUAL(forward, proc->getTransformFormatMetadata(0).getAttributeValue(0));
        double offset[]{ 0., 0., 0., 0. };
        matrix->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], -offsetF[0]);
        OCIO_CHECK_EQUAL(offset[1], -offsetF[1]);
        OCIO_CHECK_EQUAL(offset[2], -offsetF[2]);
    }

    // Color space to named transform.
    {
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor(csName.c_str(), inverse.c_str()));
        OCIO_REQUIRE_ASSERT(proc);
        OCIO::GroupTransformRcPtr group;
        OCIO_CHECK_NO_THROW(group = proc->createGroupTransform());
        OCIO_REQUIRE_ASSERT(group);
        OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
        OCIO::TransformRcPtr transform;
        OCIO_CHECK_NO_THROW(transform = group->getTransform(0));
        auto matrix = OCIO_DYNAMIC_POINTER_CAST<OCIO::MatrixTransform>(transform);
        OCIO_REQUIRE_ASSERT(matrix);
        // Named transform inverse transform.
        OCIO_CHECK_EQUAL(inverse, proc->getTransformFormatMetadata(0).getAttributeValue(0));
        double offset[]{ 0., 0., 0., 0. };
        matrix->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], offsetI[0]);
        OCIO_CHECK_EQUAL(offset[1], offsetI[1]);
        OCIO_CHECK_EQUAL(offset[2], offsetI[2]);
    }

    // Diplay color space to named transform (using ColorSpaceTransform).
    {
        OCIO::ConstProcessorRcPtr proc;
        auto csTransform = OCIO::ColorSpaceTransform::Create();
        csTransform->setSrc(dcsName.c_str());
        csTransform->setDst(both.c_str());
        OCIO_CHECK_NO_THROW(proc = config->getProcessor(csTransform));
        OCIO_REQUIRE_ASSERT(proc);
        OCIO::GroupTransformRcPtr group;
        OCIO_CHECK_NO_THROW(group = proc->createGroupTransform());
        OCIO_REQUIRE_ASSERT(group);
        OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
        OCIO::TransformRcPtr transform;
        OCIO_CHECK_NO_THROW(transform = group->getTransform(0));
        auto matrix = OCIO_DYNAMIC_POINTER_CAST<OCIO::MatrixTransform>(transform);
        OCIO_REQUIRE_ASSERT(matrix);
        // Named transform has both transforms, use inverse transform.
        OCIO_CHECK_EQUAL(inverse, proc->getTransformFormatMetadata(0).getAttributeValue(0));
        double offset[]{ 0., 0., 0., 0. };
        matrix->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], offsetI[0]);
        OCIO_CHECK_EQUAL(offset[1], offsetI[1]);
        OCIO_CHECK_EQUAL(offset[2], offsetI[2]);
    }

    // Named transform to color space.
    {
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor(forward.c_str(), csName.c_str()));
        OCIO_REQUIRE_ASSERT(proc);
        OCIO::GroupTransformRcPtr group;
        OCIO_CHECK_NO_THROW(group = proc->createGroupTransform());
        OCIO_REQUIRE_ASSERT(group);
        OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
        OCIO::TransformRcPtr transform;
        OCIO_CHECK_NO_THROW(transform = group->getTransform(0));
        auto matrix = OCIO_DYNAMIC_POINTER_CAST<OCIO::MatrixTransform>(transform);
        OCIO_REQUIRE_ASSERT(matrix);
        // Named transform forward transform.
        OCIO_CHECK_EQUAL(forward, matrix->getFormatMetadata().getAttributeValue(0));
        double offset[]{ 0., 0., 0., 0. };
        matrix->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], offsetF[0]);
        OCIO_CHECK_EQUAL(offset[1], offsetF[1]);
        OCIO_CHECK_EQUAL(offset[2], offsetF[2]);
    }

    // Named transform to display color space.
    {
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor(inverse.c_str(), dcsName.c_str()));
        OCIO_REQUIRE_ASSERT(proc);
        OCIO::GroupTransformRcPtr group;
        OCIO_CHECK_NO_THROW(group = proc->createGroupTransform());
        OCIO_REQUIRE_ASSERT(group);
        OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
        OCIO::TransformRcPtr transform;
        OCIO_CHECK_NO_THROW(transform = group->getTransform(0));
        auto matrix = OCIO_DYNAMIC_POINTER_CAST<OCIO::MatrixTransform>(transform);
        OCIO_REQUIRE_ASSERT(matrix);
        // Named transform forward transform not available, use inverse transform inverted.
        OCIO_CHECK_EQUAL(inverse, matrix->getFormatMetadata().getAttributeValue(0));
        double offset[]{ 0., 0., 0., 0. };
        matrix->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], -offsetI[0]);
        OCIO_CHECK_EQUAL(offset[1], -offsetI[1]);
        OCIO_CHECK_EQUAL(offset[2], -offsetI[2]);
    }

    // Named transform to color space.
    {
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor(both.c_str(), csName.c_str()));
        OCIO_REQUIRE_ASSERT(proc);
        OCIO::GroupTransformRcPtr group;
        OCIO_CHECK_NO_THROW(group = proc->createGroupTransform());
        OCIO_REQUIRE_ASSERT(group);
        OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
        OCIO::TransformRcPtr transform;
        OCIO_CHECK_NO_THROW(transform = group->getTransform(0));
        auto matrix = OCIO_DYNAMIC_POINTER_CAST<OCIO::MatrixTransform>(transform);
        OCIO_REQUIRE_ASSERT(matrix);
        // Named transform has both transforms, use forward transform.
        OCIO_CHECK_EQUAL(forward, matrix->getFormatMetadata().getAttributeValue(0));
        double offset[]{ 0., 0., 0., 0. };
        matrix->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], offsetF[0]);
        OCIO_CHECK_EQUAL(offset[1], offsetF[1]);
        OCIO_CHECK_EQUAL(offset[2], offsetF[2]);
    }

    // Named transform to named transform.
    {
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor(both.c_str(), both.c_str()));
        OCIO_REQUIRE_ASSERT(proc);
        OCIO::GroupTransformRcPtr group;
        OCIO_CHECK_NO_THROW(group = proc->createGroupTransform());
        OCIO_REQUIRE_ASSERT(group);
        OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 2);
        OCIO::TransformRcPtr transform;
        OCIO_CHECK_NO_THROW(transform = group->getTransform(0));
        auto matrix = OCIO_DYNAMIC_POINTER_CAST<OCIO::MatrixTransform>(transform);
        OCIO_REQUIRE_ASSERT(matrix);
        double offset[]{ 0., 0., 0., 0. };
        matrix->getOffset(offset);
        // Named transform forward transform.
        OCIO_CHECK_EQUAL(forward, proc->getTransformFormatMetadata(0).getAttributeValue(0));
        matrix->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], offsetF[0]);
        OCIO_CHECK_EQUAL(offset[1], offsetF[1]);
        OCIO_CHECK_EQUAL(offset[2], offsetF[2]);

        OCIO_CHECK_NO_THROW(transform = group->getTransform(1));
        matrix = OCIO_DYNAMIC_POINTER_CAST<OCIO::MatrixTransform>(transform);
        OCIO_REQUIRE_ASSERT(matrix);
        // Named transform inverse transform.
        OCIO_CHECK_EQUAL(inverse, proc->getTransformFormatMetadata(1).getAttributeValue(0));
        matrix->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], offsetI[0]);
        OCIO_CHECK_EQUAL(offset[1], offsetI[1]);
        OCIO_CHECK_EQUAL(offset[2], offsetI[2]);
    }
}

OCIO_ADD_TEST(Config, named_transform_validation)
{
    OCIO::ConfigRcPtr config = OCIO::Config::CreateRaw()->createEditableCopy();

    OCIO::NamedTransformRcPtr namedTransform = OCIO::NamedTransform::Create();
    const std::string name1{ "name" };
    namedTransform->setName(name1.c_str());
    auto mat = OCIO::MatrixTransform::Create();
    const double offset[4]{ 0.1, 0.2, 0.3, 0.4 };
    mat->setOffset(offset);
    namedTransform->setTransform(mat, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_CHECK_NO_THROW(config->addNamedTransform(namedTransform));
    OCIO_CHECK_EQUAL(config->getNumNamedTransforms(), 1);

    const std::string name2{ "other_name" };
    namedTransform->setName(name2.c_str());
    namedTransform->setTransform(mat, OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_NO_THROW(config->addNamedTransform(namedTransform));
    OCIO_CHECK_EQUAL(config->getNumNamedTransforms(), 2);

    OCIO_CHECK_NO_THROW(config->validate());

    OCIO_CHECK_EQUAL(name1, config->getNamedTransformNameByIndex(0));
    OCIO_CHECK_EQUAL(name2, config->getNamedTransformNameByIndex(1));
    OCIO_CHECK_EQUAL(std::string(), config->getNamedTransformNameByIndex(2));

    OCIO::ConstNamedTransformRcPtr nt = config->getNamedTransform(name1.c_str());
    OCIO_REQUIRE_ASSERT(nt);
    OCIO_CHECK_ASSERT(nt->getTransform(OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_ASSERT(!nt->getTransform(OCIO::TRANSFORM_DIR_INVERSE));

    nt = config->getNamedTransform(name2.c_str());
    OCIO_REQUIRE_ASSERT(nt);
    OCIO_CHECK_ASSERT(nt->getTransform(OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_ASSERT(nt->getTransform(OCIO::TRANSFORM_DIR_INVERSE));

    OCIO::ConstProcessorRcPtr proc;
    OCIO_CHECK_NO_THROW(proc = config->getProcessor("raw", name1.c_str()));
    OCIO_REQUIRE_ASSERT(proc);

    OCIO_CHECK_NO_THROW(proc = config->getProcessor(name1.c_str(), name1.c_str()));
    OCIO_REQUIRE_ASSERT(proc);

    OCIO_CHECK_THROW_WHAT(proc = config->getProcessor("raw", "missing"), OCIO::Exception,
                          "Color space 'missing' could not be found");

    // NamedTransform can't use a role name.
    config->setRole(name1.c_str(), "raw");
    OCIO_CHECK_THROW_WHAT(config->validate(), OCIO::Exception,
                          "This name is already used for a role");
    config->setRole(name1.c_str(), nullptr);

    // NamedTransform can't use a color space name.
    OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
    cs->setName(name1.c_str());
    config->addColorSpace(cs);
    OCIO_CHECK_THROW_WHAT(config->validate(), OCIO::Exception,
                          "This name is already used for a color space");
    config->removeColorSpace(name1.c_str());

    // NamedTransform can't use a look name.
    OCIO::LookRcPtr look = OCIO::Look::Create();
    look->setName(name1.c_str());
    look->setProcessSpace("raw");

    config->addLook(look);
    OCIO_CHECK_THROW_WHAT(config->validate(), OCIO::Exception,
                          "This name is already used for a look");
    config->clearLooks();

    // NamedTransform can't use a view transform name.
    OCIO::ViewTransformRcPtr vt = OCIO::ViewTransform::Create(OCIO::REFERENCE_SPACE_SCENE);
    vt->setName(name1.c_str());
    vt->setTransform(OCIO::MatrixTransform::Create(), OCIO::VIEWTRANSFORM_DIR_TO_REFERENCE);

    config->addViewTransform(vt);
    OCIO_CHECK_THROW_WHAT(config->validate(), OCIO::Exception,
                          "This name is already used for a view transform");
    config->clearViewTransforms();

    config->setMajorVersion(1);
    config->setFileRules(OCIO::FileRules::Create());
    OCIO_CHECK_THROW_WHAT(config->validate(), OCIO::Exception,
                          "Only version 2 (or higher) can have NamedTransforms");
}

OCIO_ADD_TEST(Config, named_transform_io)
{
    // Validate Config::validate() on config file containing named transforms.

    constexpr const char * OCIO_CONFIG_START{ R"(ocio_profile_version: 2

environment:
  {}
search_path: ""
strictparsing: true
luma: [0.2126, 0.7152, 0.0722]

roles:
  default: raw

file_rules:
  - !<Rule> {name: Default, colorspace: default}

displays:
  Disp1:
    - !<View> {name: View1, colorspace: raw}

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

)" };

    // Test a valid config with 2 named transforms, then using this valid config test that look,
    // role, and file rules can't use named transforms.
    {
        constexpr const char * NT{ R"(named_transforms:
  - !<NamedTransform>
    name: namedTransform1
    family: family
    categories: [input, basic]
    transform: !<ColorSpaceTransform> {src: default, dst: raw}

  - !<NamedTransform>
    name: namedTransform2
    inverse_transform: !<ColorSpaceTransform> {src: default, dst: raw}
)" };

        const std::string configStr = std::string(OCIO_CONFIG_START) + std::string(NT);

        std::istringstream is;
        is.str(configStr);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->validate());

        OCIO_REQUIRE_EQUAL(config->getNumNamedTransforms(), 2);
        OCIO_CHECK_EQUAL(std::string(config->getNamedTransformNameByIndex(0)),
                         "namedTransform1");
        OCIO_CHECK_EQUAL(std::string(config->getNamedTransformNameByIndex(1)),
                         "namedTransform2");
        auto nt = config->getNamedTransform("namedTransform1");
        OCIO_REQUIRE_ASSERT(nt);
        OCIO_CHECK_EQUAL(std::string(nt->getFamily()), "family");
        OCIO_REQUIRE_EQUAL(nt->getNumCategories(), 2);
        OCIO_CHECK_EQUAL(std::string(nt->getCategory(0)), "input");
        OCIO_CHECK_EQUAL(std::string(nt->getCategory(1)), "basic");
        std::ostringstream oss;
        OCIO_CHECK_NO_THROW(oss << *config.get());
        OCIO_CHECK_EQUAL(oss.str(), configStr);

        // Look can't use named transform.
        auto look = OCIO::Look::Create();
        look->setName("look");
        look->setProcessSpace("namedTransform1");
        auto configEdit = config->createEditableCopy();
        configEdit->addLook(look);
        OCIO_CHECK_THROW_WHAT(configEdit->validate(), OCIO::Exception,
                              "process color space, 'namedTransform1', which is not defined");
        configEdit->clearLooks();

        // Role can't use named transform.
        configEdit->setRole("newrole", "namedTransform1");
        OCIO_CHECK_THROW_WHAT(configEdit->validate(), OCIO::Exception,
                              "refers to a color space, 'namedTransform1', which is not defined");
        configEdit->setRole("newrole", nullptr);

        // File rule can use named transform.
        auto rules = configEdit->getFileRules()->createEditableCopy();
        rules->insertRule(0, "newrule", "namedTransform1", "*", "*");
        configEdit->setFileRules(rules);
        OCIO_CHECK_NO_THROW(configEdit->validate());
    }

    // Config can't be read: named transform must define a transform.
    {
        constexpr const char * NT{ R"(named_transforms:
  - !<NamedTransform>
    name: namedTransform1)" };

        const std::string configStr = std::string(OCIO_CONFIG_START) + std::string(NT);

        std::istringstream is;
        is.str(configStr);

        OCIO_CHECK_THROW_WHAT(OCIO::Config::CreateFromStream(is), OCIO::Exception,
                              "Named transform must define at least one transform.");
    }

    // Invalid config, named transform holds an invalid transform.
    {
        constexpr const char * NT{ R"(named_transforms:
  - !<NamedTransform>
    name: namedTransform1
    transform: !<ColorSpaceTransform> {src: default}
)" };

        const std::string configStr = std::string(OCIO_CONFIG_START) + std::string(NT);

        std::istringstream is;
        is.str(configStr);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_THROW_WHAT(config->validate(), OCIO::Exception,
                              "ColorSpaceTransform: empty destination color space name");
    }
}

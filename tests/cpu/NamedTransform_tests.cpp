// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "NamedTransform.cpp"

#include "Platform.h"
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

    auto actualFwdTransform = OCIO::NamedTransform::GetTransform(namedTransform,
                                                                 OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_ASSERT(OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixTransform>(actualFwdTransform));
    auto actualInvTransform = OCIO::NamedTransform::GetTransform(namedTransform,
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

OCIO_ADD_TEST(NamedTransform, alias)
{
    OCIO::NamedTransformRcPtr nt = OCIO::NamedTransform::Create();
    OCIO_REQUIRE_ASSERT(nt);
    OCIO_CHECK_EQUAL(nt->getNumAliases(), 0);
    constexpr char AliasA[]{ "aliasA" };
    constexpr char AliasAAlt[]{ "aLiaSa" };
    constexpr char AliasB[]{ "aliasB" };
    nt->addAlias(AliasA);
    OCIO_CHECK_EQUAL(nt->getNumAliases(), 1);
    nt->addAlias(AliasB);
    OCIO_CHECK_EQUAL(nt->getNumAliases(), 2);
    OCIO_CHECK_EQUAL(std::string(nt->getAlias(0)), AliasA);
    OCIO_CHECK_EQUAL(std::string(nt->getAlias(1)), AliasB);

    // Alias with same name (different case) already exists, do nothing.
    {
        nt->addAlias(AliasAAlt);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 2);
        OCIO_CHECK_EQUAL(std::string(nt->getAlias(0)), AliasA);
        OCIO_CHECK_EQUAL(std::string(nt->getAlias(1)), AliasB);
    }

    // Remove alias (using a different case).
    {
        nt->removeAlias(AliasAAlt);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 1);
        OCIO_CHECK_EQUAL(std::string(nt->getAlias(0)), AliasB);
    }

    // Add with new case.
    {
        nt->addAlias(AliasAAlt);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 2);
        OCIO_CHECK_EQUAL(std::string(nt->getAlias(0)), AliasB);
        OCIO_CHECK_EQUAL(std::string(nt->getAlias(1)), AliasAAlt);
    }

    // Setting the name of the named transform to one of its aliases removes the alias.
    {
        nt->setName(AliasA);
        OCIO_CHECK_EQUAL(std::string(nt->getName()), AliasA);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 1);
        OCIO_CHECK_EQUAL(std::string(nt->getAlias(0)), AliasB);
    }

    // Alias is not added if it is already the named transform name.
    {
        nt->addAlias(AliasAAlt);
        OCIO_CHECK_EQUAL(std::string(nt->getName()), AliasA);
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 1);
        OCIO_CHECK_EQUAL(std::string(nt->getAlias(0)), AliasB);
    }

    // Remove all aliases.
    {
        nt->addAlias("other");
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 2);
        nt->clearAliases();
        OCIO_CHECK_EQUAL(nt->getNumAliases(), 0);
    }

    //
    // Add and access named transforms in a config.
    //

    OCIO::ConfigRcPtr config = OCIO::Config::CreateRaw()->createEditableCopy();
    nt->setName("name");
    nt->setTransform(OCIO::MatrixTransform::Create(), OCIO::TRANSFORM_DIR_FORWARD);

    {
        OCIO_CHECK_NO_THROW(config->addNamedTransform(nt));

        nt->setName("other");
        nt->addAlias(AliasB);
        OCIO_CHECK_NO_THROW(config->addNamedTransform(nt));
        OCIO_CHECK_EQUAL(config->getNumNamedTransforms(), 2);

        auto ntcfg = config->getNamedTransform("name");
        OCIO_REQUIRE_ASSERT(ntcfg);
        OCIO_CHECK_EQUAL(ntcfg->getNumAliases(), 0);
    }

    // Access by alias.
    {
        auto ntcfg = config->getNamedTransform(AliasB);
        OCIO_REQUIRE_ASSERT(ntcfg);
        OCIO_CHECK_EQUAL(std::string(ntcfg->getName()), "other");
        OCIO_CHECK_EQUAL(ntcfg->getNumAliases(), 1);
        OCIO_CHECK_EQUAL(std::string(config->getCanonicalName(AliasB)), "other");
        OCIO_CHECK_EQUAL(std::string(config->getCanonicalName("other")), "other");
        OCIO_CHECK_EQUAL(std::string(config->getCanonicalName("not found")), "");
        OCIO_CHECK_EQUAL(std::string(config->getCanonicalName("")), "");
    }

    // Named transform with same name is replaced.
    {
        nt->setName("name");
        nt->clearAliases();
        nt->addAlias(AliasA);
        OCIO_CHECK_NO_THROW(config->addNamedTransform(nt));
        OCIO_CHECK_EQUAL(config->getNumNamedTransforms(), 2);

        auto ntcfg = config->getNamedTransform("name");
        OCIO_REQUIRE_ASSERT(ntcfg);
        OCIO_CHECK_EQUAL(ntcfg->getNumAliases(), 1);
    }

    // Can't add a named transform if name is used as alias for existing named transform.
    {
        nt->setName(AliasA);
        nt->clearAliases();
        OCIO_CHECK_THROW_WHAT(config->addNamedTransform(nt), OCIO::Exception,
                              "Cannot add 'aliasA' named transform, existing named transform, "
                              "'name' is using this name as an alias");
    }

    // Can't add a named transform if alias is used as alias for existing named transform.
    {
        nt->setName("newName");
        nt->addAlias(AliasB);
        OCIO_CHECK_THROW_WHAT(config->addNamedTransform(nt), OCIO::Exception,
                              "Cannot add 'newName' named transform, it has 'aliasB' alias and "
                              "existing named transform, 'other' is using the same alias");
    }

    // Can't add a named transform if alias is used as name for existing named transform.
    {
        nt->addAlias("other");
        OCIO_CHECK_THROW_WHAT(config->addNamedTransform(nt), OCIO::Exception,
                              "Cannot add 'newName' named transform, it has 'aliasB' alias and "
                              "existing named transform, 'other' is using the same alias");
    }
}

OCIO_ADD_TEST(NamedTransform, static_get_transform)
{
    auto config = OCIO::Config::CreateRaw();

    const double offsetF[4]{ 0.1, 0.2, 0.3, 0.4 };
    const double offsetI[4]{ -offsetF[0], -offsetF[1], -offsetF[2], -offsetF[3] };

    auto mat1 = OCIO::MatrixTransform::Create();
    mat1->setOffset(offsetF);
    OCIO::NamedTransformRcPtr nt1 = OCIO::NamedTransform::Create();
    nt1->setTransform(mat1, OCIO::TRANSFORM_DIR_FORWARD);

    auto mat2 = OCIO::MatrixTransform::Create();
    mat2->setOffset(offsetI);
    OCIO::NamedTransformRcPtr nt2 = OCIO::NamedTransform::Create();
    nt2->setTransform(mat2, OCIO::TRANSFORM_DIR_INVERSE);

    // Forward transform from forward-only named transform
    {
        auto src_tf = OCIO::NamedTransform::GetTransform(nt1,
                                                         OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_ASSERT(src_tf);
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor(src_tf));
        OCIO_REQUIRE_ASSERT(proc);
        OCIO::GroupTransformRcPtr group;
        OCIO_CHECK_NO_THROW(group = proc->createGroupTransform());
        OCIO_REQUIRE_ASSERT(group);
        OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
        OCIO::TransformRcPtr dst_tf;
        OCIO_CHECK_NO_THROW(dst_tf = group->getTransform(0));
        auto mat = OCIO_DYNAMIC_POINTER_CAST<OCIO::MatrixTransform>(dst_tf);
        OCIO_REQUIRE_ASSERT(mat);
        double offset[4];
        mat->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], offsetF[0]);
        OCIO_CHECK_EQUAL(offset[1], offsetF[1]);
        OCIO_CHECK_EQUAL(offset[2], offsetF[2]);
        OCIO_CHECK_EQUAL(offset[3], offsetF[3]);
    }

    // Inverse transform from forward-only named transform
    {
        auto src_tf = OCIO::NamedTransform::GetTransform(nt1,
                                                         OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_ASSERT(src_tf);
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor(src_tf));
        OCIO_REQUIRE_ASSERT(proc);
        OCIO::GroupTransformRcPtr group;
        OCIO_CHECK_NO_THROW(group = proc->createGroupTransform());
        OCIO_REQUIRE_ASSERT(group);
        OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
        OCIO::TransformRcPtr dst_tf;
        OCIO_CHECK_NO_THROW(dst_tf = group->getTransform(0));
        auto mat = OCIO_DYNAMIC_POINTER_CAST<OCIO::MatrixTransform>(dst_tf);
        OCIO_REQUIRE_ASSERT(mat);
        double offset[4];
        mat->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], offsetI[0]);
        OCIO_CHECK_EQUAL(offset[1], offsetI[1]);
        OCIO_CHECK_EQUAL(offset[2], offsetI[2]);
        OCIO_CHECK_EQUAL(offset[3], offsetI[3]);
    }

    // Forward transform from inverse-only named transform
    {
        auto src_tf = OCIO::NamedTransform::GetTransform(nt2,
                                                         OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_ASSERT(src_tf);
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor(src_tf));
        OCIO_REQUIRE_ASSERT(proc);
        OCIO::GroupTransformRcPtr group;
        OCIO_CHECK_NO_THROW(group = proc->createGroupTransform());
        OCIO_REQUIRE_ASSERT(group);
        OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
        OCIO::TransformRcPtr dst_tf;
        OCIO_CHECK_NO_THROW(dst_tf = group->getTransform(0));
        auto mat = OCIO_DYNAMIC_POINTER_CAST<OCIO::MatrixTransform>(dst_tf);
        OCIO_REQUIRE_ASSERT(mat);
        double offset[4];
        mat->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], offsetF[0]);
        OCIO_CHECK_EQUAL(offset[1], offsetF[1]);
        OCIO_CHECK_EQUAL(offset[2], offsetF[2]);
        OCIO_CHECK_EQUAL(offset[3], offsetF[3]);
    }

    // Inverse transform from inverse-only named transform
    {
        auto src_tf = OCIO::NamedTransform::GetTransform(nt2,
                                                         OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_ASSERT(src_tf);
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor(src_tf));
        OCIO_REQUIRE_ASSERT(proc);
        OCIO::GroupTransformRcPtr group;
        OCIO_CHECK_NO_THROW(group = proc->createGroupTransform());
        OCIO_REQUIRE_ASSERT(group);
        OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
        OCIO::TransformRcPtr dst_tf;
        OCIO_CHECK_NO_THROW(dst_tf = group->getTransform(0));
        auto mat = OCIO_DYNAMIC_POINTER_CAST<OCIO::MatrixTransform>(dst_tf);
        OCIO_REQUIRE_ASSERT(mat);
        double offset[4];
        mat->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], offsetI[0]);
        OCIO_CHECK_EQUAL(offset[1], offsetI[1]);
        OCIO_CHECK_EQUAL(offset[2], offsetI[2]);
        OCIO_CHECK_EQUAL(offset[3], offsetI[3]);
    }
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
    - !<View> {name: ntview, colorspace: ntf}

active_displays: []
active_views: []

display_colorspaces:
  - !<ColorSpace>
    name: dcs
    aliases: [display color space]
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
    aliases: [colorspace]
    isdata: false
    allocation: uniform
    to_scene_reference: !<RangeTransform> {max_in_value: 1, max_out_value: 1}

named_transforms:
  - !<NamedTransform>
    name: forward
    aliases: [nt1, ntf]
    encoding: scene-linear
    transform: !<MatrixTransform> {name: forward, offset: [0.1, 0.2, 0.3, 0.4]}

  - !<NamedTransform>
    name: inverse
    aliases: [nt2, nti]
    inverse_transform: !<MatrixTransform> {name: inverse, offset: [-0.2, -0.1, -0.1, 0]}

  - !<NamedTransform>
    name: both
    aliases: [nt3, ntb]
    transform: !<MatrixTransform> {name: forward, offset: [0.1, 0.2, 0.3, 0.4]}
    inverse_transform: !<MatrixTransform> {name: inverse, offset: [-0.2, -0.1, -0.1, 0]}
)" };

    std::istringstream iss;
    iss.str(Config);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(iss));

    OCIO::ConstContextRcPtr context;
    OCIO_CHECK_NO_THROW(context = config->getCurrentContext());
    OCIO_REQUIRE_ASSERT(context);

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
        OCIO_CHECK_EQUAL(std::string(nt->getEncoding()), "scene-linear");
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

    // Basic named transform access using alias.
    {
        auto nt = config->getNamedTransform("nt1"); // Alias being used
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

    // Named transform from NamedTransform object and forward direction.
    {
        auto nt = config->getNamedTransform("forward");
        OCIO_REQUIRE_ASSERT(nt);

        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor(nt, OCIO::TRANSFORM_DIR_FORWARD));
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
        OCIO_CHECK_EQUAL(forward, proc->getTransformFormatMetadata(0).getAttributeValue(0));
        double offset[]{ 0., 0., 0., 0. };
        matrix->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], offsetF[0]);
        OCIO_CHECK_EQUAL(offset[1], offsetF[1]);
        OCIO_CHECK_EQUAL(offset[2], offsetF[2]);
    }

    // Named transform from NamedTransform object and inverse direction.
    {
        auto nt = config->getNamedTransform("forward");
        OCIO_REQUIRE_ASSERT(nt);

        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor(nt, OCIO::TRANSFORM_DIR_INVERSE));
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

    // Named transform from NamedTransform object and forward direction with context.
    {
        auto nt = config->getNamedTransform("inverse");
        OCIO_REQUIRE_ASSERT(nt);

        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor(context, 
                                                        nt, 
                                                        OCIO::TRANSFORM_DIR_FORWARD));
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
        OCIO_CHECK_EQUAL(inverse, proc->getTransformFormatMetadata(0).getAttributeValue(0));
        double offset[]{ 0., 0., 0., 0. };
        matrix->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], -offsetI[0]);
        OCIO_CHECK_EQUAL(offset[1], -offsetI[1]);
        OCIO_CHECK_EQUAL(offset[2], -offsetI[2]);
    }

    // Named transform from NamedTransform object and inverse direction with context.
    {
        auto nt = config->getNamedTransform("inverse");
        OCIO_REQUIRE_ASSERT(nt);

        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor(context, 
                                                        nt, 
                                                        OCIO::TRANSFORM_DIR_INVERSE));
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

    // Named transform from name and forward direction.
    {
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor("inverse", 
                                                        OCIO::TRANSFORM_DIR_FORWARD));
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
        OCIO_CHECK_EQUAL(inverse, proc->getTransformFormatMetadata(0).getAttributeValue(0));
        double offset[]{ 0., 0., 0., 0. };
        matrix->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], -offsetI[0]);
        OCIO_CHECK_EQUAL(offset[1], -offsetI[1]);
        OCIO_CHECK_EQUAL(offset[2], -offsetI[2]);
    }

    // Named transform from name and inverse direction.
    {
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor("inverse", 
                                                        OCIO::TRANSFORM_DIR_INVERSE));
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

    // Named transform from name and forward direction with context.
    {
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor(context, 
                                                        "forward", 
                                                        OCIO::TRANSFORM_DIR_FORWARD));
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
        OCIO_CHECK_EQUAL(forward, proc->getTransformFormatMetadata(0).getAttributeValue(0));
        double offset[]{ 0., 0., 0., 0. };
        matrix->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], offsetF[0]);
        OCIO_CHECK_EQUAL(offset[1], offsetF[1]);
        OCIO_CHECK_EQUAL(offset[2], offsetF[2]);
    }

    // Named transform from name and inverse direction with context.
    {
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor(context, 
                                                        "forward", 
                                                        OCIO::TRANSFORM_DIR_INVERSE));
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

    // Named transform from alias and forward direction.
    {
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor("ntb", OCIO::TRANSFORM_DIR_FORWARD));
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
        OCIO_CHECK_EQUAL(forward, proc->getTransformFormatMetadata(0).getAttributeValue(0));
        double offset[]{ 0., 0., 0., 0. };
        matrix->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], offsetF[0]);
        OCIO_CHECK_EQUAL(offset[1], offsetF[1]);
        OCIO_CHECK_EQUAL(offset[2], offsetF[2]);
    }

    // Named transform from alias and inverse direction.
    {
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor("nt3", OCIO::TRANSFORM_DIR_INVERSE));
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

    // Diplay color space to named transform using aliases.
    {
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor("display color space",
                                                        "ntf")); // Aliases.
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

    // Color space to named transform using aliases.
    {
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor("colorspace", "nt2")); // Aliases
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

    // Named transform to color space using aliases.
    {
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor("ntf", "colorspace")); // Aliases
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

    // Named transform to named transform using aliases.
    {
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor("nt3", "ntb")); // Aliases
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

    {
        OCIO::ConstProcessorRcPtr proc;
        OCIO_CHECK_NO_THROW(proc = config->getProcessor("colorspace", "sRGB", "ntview",
                                                        OCIO::TRANSFORM_DIR_FORWARD));
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

    // See DisplayViewTransform_tests.cpp for additional named transform tests related to their
    // use in displays/views.
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
    OCIO_CHECK_THROW_WHAT(config->setRole(name1.c_str(), "raw"), OCIO::Exception,
                          "Cannot add 'name' role, there is already a named transform using this "
                          "as a name or an alias");
    config->setRole(name1.c_str(), nullptr);

    // NamedTransform can't use a color space name.
    OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
    cs->setName(name1.c_str());
    OCIO_CHECK_THROW_WHAT(config->addColorSpace(cs), OCIO::Exception,
                          "Cannot add 'name' color space, there is already a named transform "
                          "using this name as a name or as an alias: 'name'");
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

    // Test use of named transforms in a role (not allowed), look (not allowed), and file rules
    // (allowed).
    {
        constexpr const char * NT{ R"(named_transforms:
  - !<NamedTransform>
    name: namedTransform1
    aliases: [named1, named2]
    family: family
    categories: [input, basic]
    encoding: data
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
        OCIO_REQUIRE_EQUAL(nt->getNumAliases(), 2);
        OCIO_CHECK_EQUAL(std::string(nt->getAlias(0)), "named1");
        OCIO_CHECK_EQUAL(std::string(nt->getAlias(1)), "named2");
        OCIO_CHECK_EQUAL(std::string(nt->getFamily()), "family");
        OCIO_REQUIRE_EQUAL(nt->getNumCategories(), 2);
        OCIO_CHECK_EQUAL(std::string(nt->getCategory(0)), "input");
        OCIO_CHECK_EQUAL(std::string(nt->getCategory(1)), "basic");
        OCIO_CHECK_EQUAL(std::string(nt->getEncoding()), "data");
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

OCIO_ADD_TEST(Config, colorspace_transform_named_transform)
{
    // Validate Config::validate() on config with ColorSpace or DisplayView Transforms,
    // or ViewTransforms that reference a Named Transform.

    constexpr const char * OCIO_CONFIG{ R"(
ocio_profile_version: 2

file_rules:
  - !<Rule> {name: Default, colorspace: raw}

displays:
  sRGB:
    - !<View> {name: Raw, colorspace: raw}
  Rec.2100-PQ - Display:
    - !<View> {name: test_view, view_transform: vt, display_colorspace: Rec.2100-PQ - Display}

view_transforms:
  - !<ViewTransform>
    name: vt
    from_scene_reference: !<ColorSpaceTransform> {src: nt, dst: cs2}

display_colorspaces:
  - !<ColorSpace>
    name: Rec.2100-PQ - Display
    isdata: false
    from_display_reference: !<BuiltinTransform> {style: DISPLAY - CIE-XYZ-D65_to_REC.2100-PQ}

colorspaces:
  - !<ColorSpace>
    name: raw
    isdata: true

  - !<ColorSpace>
    name: cs2
    isdata: false
    from_scene_reference: !<MatrixTransform> {matrix: [ 2.041587903811, -0.565006974279, -0.344731350778, 0, -0.969243636281, 1.875967501508, 0.041555057407, 0, 0.013444280632, -0.118362392231, 1.015174994391, 0, 0, 0, 0, 1 ]}

  - !<ColorSpace>
    name: cs3
    isdata: false
    from_scene_reference: !<ColorSpaceTransform> {src: nt_alias, dst: cs2}

  - !<ColorSpace>
    name: cs4
    isdata: false
    from_scene_reference: !<DisplayViewTransform> {src: nt_alias, display: Rec.2100-PQ - Display, view: test_view}

named_transforms:
  - !<NamedTransform>
    name: nt
    aliases: [nt_alias]
    transform: !<GroupTransform>
      children:
        - !<MatrixTransform> {matrix: [1.49086870465701, -0.268712979082956, -0.222155725704626, 0, -0.0792372106028327, 1.1793685831111, -0.100131372460806, 0, 0.00277810076707935, -0.0304336146315336, 1.02765551391237, 0, 0, 0, 0, 1]}
)" };

    const std::string configStr = std::string(OCIO_CONFIG);

    std::istringstream is;
    is.str(configStr);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());
}

namespace
{

constexpr char InactiveNTConfigStart[] =
"ocio_profile_version: 2\n"
"\n"
"environment:\n"
"  {}\n"
"search_path: luts\n"
"strictparsing: true\n"
"luma: [0.2126, 0.7152, 0.0722]\n"
"\n"
"roles:\n"
"  default: raw\n"
"  scene_linear: lnh\n"
"\n"
"file_rules:\n"
"  - !<Rule> {name: Default, colorspace: default}\n"
"\n"
"displays:\n"
"  sRGB:\n"
"    - !<View> {name: Raw, colorspace: raw}\n"
"    - !<View> {name: Lnh, colorspace: lnh, looks: beauty}\n"
"\n"
"active_displays: []\n"
"active_views: []\n";

constexpr char InactiveNTConfigEnd[] =
"\n"
"looks:\n"
"  - !<Look>\n"
"    name: beauty\n"
"    process_space: lnh\n"
"    transform: !<CDLTransform> {slope: [1, 2, 1]}\n"
"\n"
"\n"
"colorspaces:\n"
"  - !<ColorSpace>\n"
"    name: raw\n"
"    family: \"\"\n"
"    equalitygroup: \"\"\n"
"    bitdepth: unknown\n"
"    isdata: false\n"
"    allocation: uniform\n"
"\n"
"  - !<ColorSpace>\n"
"    name: lnh\n"
"    family: \"\"\n"
"    equalitygroup: \"\"\n"
"    bitdepth: unknown\n"
"    isdata: false\n"
"    allocation: uniform\n"
"\n"
"named_transforms:\n"
"  - !<NamedTransform>\n"
"    name: nt1\n"
"    aliases: [alias1]\n"
"    categories: [cat1]\n"
"    transform: !<CDLTransform> {offset: [0.1, 0.1, 0.1]}\n"
"\n"
"  - !<NamedTransform>\n"
"    name: nt2\n"
"    categories: [cat2]\n"
"    transform: !<CDLTransform> {offset: [0.2, 0.2, 0.2]}\n"
"\n"
"  - !<NamedTransform>\n"
"    name: nt3\n"
"    categories: [cat3]\n"
"    transform: !<CDLTransform> {offset: [0.3, 0.3, 0.3]}\n";

class InactiveCSGuard
{
public:
    InactiveCSGuard()
    {
        OCIO::Platform::Setenv(OCIO::OCIO_INACTIVE_COLORSPACES_ENVVAR, "nt3, nt1, lnh");
    }
    ~InactiveCSGuard()
    {
        OCIO::Platform::Unsetenv(OCIO::OCIO_INACTIVE_COLORSPACES_ENVVAR);
    }
};

} // anon.

OCIO_ADD_TEST(Config, inactive_named_transforms)
{
    // The unit test validates the inactive named transforms behavior.
    // Tests (Config, inactive_color_space) and (Config, inactive_color_space_precedence) from
    // Config_tests.cpp are testing inactive color spaces without any named transforms.

    std::string configStr;
    configStr += InactiveNTConfigStart;
    configStr += InactiveNTConfigEnd;

    std::istringstream is;
    is.str(configStr);

    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());
    OCIO_REQUIRE_ASSERT(config);
    OCIO_CHECK_NO_THROW(config->validate());

    // Step 1 - No inactive named transforms.

    OCIO_REQUIRE_EQUAL(config->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_INACTIVE), 0);
    OCIO_REQUIRE_EQUAL(config->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ACTIVE), 3);
    OCIO_REQUIRE_EQUAL(config->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 3);

    OCIO_CHECK_EQUAL(std::string("nt1"),
                     config->getNamedTransformNameByIndex(OCIO::NAMEDTRANSFORM_ALL, 0));
    OCIO_CHECK_EQUAL(std::string("nt2"),
                     config->getNamedTransformNameByIndex(OCIO::NAMEDTRANSFORM_ALL, 1));
    OCIO_CHECK_EQUAL(std::string("nt3"),
                     config->getNamedTransformNameByIndex(OCIO::NAMEDTRANSFORM_ALL, 2));
    // Check a faulty call.
    OCIO_CHECK_EQUAL(std::string(""),
                     config->getNamedTransformNameByIndex(OCIO::NAMEDTRANSFORM_ALL, 3));

    OCIO_REQUIRE_EQUAL(config->getNumNamedTransforms(), 3);
    OCIO_CHECK_EQUAL(std::string("nt1"), config->getNamedTransformNameByIndex(0));
    OCIO_CHECK_EQUAL(std::string("nt2"), config->getNamedTransformNameByIndex(1));
    OCIO_CHECK_EQUAL(std::string("nt3"), config->getNamedTransformNameByIndex(2));
    // Check a faulty call.
    OCIO_CHECK_EQUAL(std::string(""), config->getNamedTransformNameByIndex(3));

    // Step 2 - Some inactive color space and named transforms (aliases can be used).

    OCIO_CHECK_NO_THROW(config->setInactiveColorSpaces("lnh, alias1"));
    OCIO_CHECK_EQUAL(config->getInactiveColorSpaces(), std::string("lnh, alias1"));

    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_INACTIVE), 1);
    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_ACTIVE), 1);
    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                                 OCIO::COLORSPACE_ALL), 2);

    OCIO_REQUIRE_EQUAL(config->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_INACTIVE), 1);
    OCIO_REQUIRE_EQUAL(config->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ACTIVE), 2);
    OCIO_REQUIRE_EQUAL(config->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 3);

    // Check methods working with activity flag.
    OCIO_CHECK_EQUAL(std::string("nt1"),
                     config->getNamedTransformNameByIndex(OCIO::NAMEDTRANSFORM_ALL, 0));
    OCIO_CHECK_EQUAL(std::string("nt2"),
                     config->getNamedTransformNameByIndex(OCIO::NAMEDTRANSFORM_ALL, 1));
    OCIO_CHECK_EQUAL(std::string("nt3"),
                     config->getNamedTransformNameByIndex(OCIO::NAMEDTRANSFORM_ALL, 2));
    OCIO_CHECK_EQUAL(std::string("nt2"),
                     config->getNamedTransformNameByIndex(OCIO::NAMEDTRANSFORM_ACTIVE, 0));
    OCIO_CHECK_EQUAL(std::string("nt3"),
                     config->getNamedTransformNameByIndex(OCIO::NAMEDTRANSFORM_ACTIVE, 1));
    OCIO_CHECK_EQUAL(std::string("nt1"),
                     config->getNamedTransformNameByIndex(OCIO::NAMEDTRANSFORM_INACTIVE, 0));

    // Check methods working on only active named transforms.
    OCIO_REQUIRE_EQUAL(config->getNumNamedTransforms(), 2);
    OCIO_CHECK_EQUAL(std::string("nt2"), config->getNamedTransformNameByIndex(0));
    OCIO_CHECK_EQUAL(std::string("nt3"), config->getNamedTransformNameByIndex(1));

    // Request an active named transform.
    OCIO::ConstNamedTransformRcPtr nt;
    OCIO_CHECK_NO_THROW(nt = config->getNamedTransform("nt2"));
    OCIO_CHECK_ASSERT(nt);
    OCIO_CHECK_EQUAL(std::string("nt2"), nt->getName());

    // Request an inactive named transform.
    OCIO_CHECK_NO_THROW(nt = config->getNamedTransform("nt1"));
    OCIO_CHECK_ASSERT(nt);
    OCIO_CHECK_EQUAL(std::string("nt1"), nt->getName());
    OCIO_CHECK_EQUAL(config->getIndexForNamedTransform(nt->getName()), -1);

    // Request an inactive named transform using alias.
    OCIO_CHECK_NO_THROW(nt = config->getNamedTransform("alias1"));
    OCIO_CHECK_ASSERT(nt);
    OCIO_CHECK_EQUAL(std::string("nt1"), nt->getName());
    OCIO_CHECK_EQUAL(config->getIndexForNamedTransform(nt->getName()), -1);

    // Create a processor with one or more inactive color spaces or named transforms.
    OCIO_CHECK_NO_THROW(config->getProcessor("lnh", "nt1"));
    OCIO_CHECK_NO_THROW(config->getProcessor("raw", "nt1"));
    OCIO_CHECK_NO_THROW(config->getProcessor("lnh", "nt2"));
    OCIO_CHECK_NO_THROW(config->getProcessor("nt2", "scene_linear"));

    // Step 3 - No inactive color spaces or named transforms.

    OCIO_CHECK_NO_THROW(config->setInactiveColorSpaces(""));
    OCIO_CHECK_EQUAL(config->getInactiveColorSpaces(), std::string(""));

    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                               OCIO::COLORSPACE_ALL), 2);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 2);
    OCIO_REQUIRE_EQUAL(config->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 3);
    OCIO_REQUIRE_EQUAL(config->getNumNamedTransforms(), 3);

    // Step 4 - No inactive color spaces or named transforms can also use nullptr.

    OCIO_CHECK_NO_THROW(config->setInactiveColorSpaces("lnh, nt1"));
    OCIO_CHECK_EQUAL(config->getInactiveColorSpaces(), std::string("lnh, nt1"));

    OCIO_CHECK_NO_THROW(config->setInactiveColorSpaces(nullptr));
    OCIO_CHECK_EQUAL(config->getInactiveColorSpaces(), std::string(""));

    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                               OCIO::COLORSPACE_ALL), 2);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 2);
    OCIO_REQUIRE_EQUAL(config->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 3);
    OCIO_REQUIRE_EQUAL(config->getNumNamedTransforms(), 3);
}

OCIO_ADD_TEST(Config, inactive_named_transform_precedence)
{
    // The test demonstrates that an API request supersedes the env. variable and the
    // config file contents.

    std::string configStr;
    configStr += InactiveNTConfigStart;
    configStr += "inactive_colorspaces: [nt2]\n";
    configStr += InactiveNTConfigEnd;

    std::istringstream is;
    is.str(configStr);

    OCIO::ConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO_REQUIRE_EQUAL(config->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_INACTIVE), 1);
    OCIO_REQUIRE_EQUAL(config->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ACTIVE), 2);
    OCIO_REQUIRE_EQUAL(config->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 3);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                               OCIO::COLORSPACE_ALL), 2);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 2);

    OCIO_CHECK_EQUAL(config->getNamedTransformNameByIndex(0), std::string("nt1"));
    OCIO_CHECK_EQUAL(config->getNamedTransformNameByIndex(1), std::string("nt3"));

    // Env. variable supersedes the config content.

    InactiveCSGuard guard;

    is.str(configStr);
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is)->createEditableCopy());
    OCIO_CHECK_NO_THROW(config->validate());

    OCIO_REQUIRE_EQUAL(config->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_INACTIVE), 2);
    OCIO_REQUIRE_EQUAL(config->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ACTIVE), 1);
    OCIO_REQUIRE_EQUAL(config->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 3);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                               OCIO::COLORSPACE_ALL), 2);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 1);

    OCIO_CHECK_EQUAL(config->getNamedTransformNameByIndex(0), std::string("nt2"));

    // An API request supersedes the lists from the env. variable and the config file.

    OCIO_CHECK_NO_THROW(config->setInactiveColorSpaces("nt1, lnh"));

    OCIO_REQUIRE_EQUAL(config->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_INACTIVE), 1);
    OCIO_REQUIRE_EQUAL(config->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ACTIVE), 2);
    OCIO_REQUIRE_EQUAL(config->getNumNamedTransforms(OCIO::NAMEDTRANSFORM_ALL), 3);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(OCIO::SEARCH_REFERENCE_SPACE_ALL,
                                               OCIO::COLORSPACE_ALL), 2);
    OCIO_CHECK_EQUAL(config->getNumColorSpaces(), 1);

    OCIO_CHECK_EQUAL(config->getNamedTransformNameByIndex(0), std::string("nt2"));
    OCIO_CHECK_EQUAL(config->getNamedTransformNameByIndex(1), std::string("nt3"));
}

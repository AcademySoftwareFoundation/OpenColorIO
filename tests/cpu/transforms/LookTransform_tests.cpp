// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/fixedfunction/FixedFunctionOpData.h"

#include "transforms/LookTransform.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(LookTransform, basic)
{
    OCIO::LookTransformRcPtr look = OCIO::LookTransform::Create();
    OCIO_CHECK_EQUAL(look->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    look->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(look->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_EQUAL(look->getSrc(), std::string(""));
    OCIO_CHECK_EQUAL(look->getDst(), std::string(""));
    OCIO_CHECK_EQUAL(look->getLooks(), std::string(""));

    OCIO_CHECK_THROW_WHAT(look->validate(), OCIO::Exception, "empty source");

    const std::string src{ "src" };
    const std::string dst{ "dst" };
    const std::string looks{ "look1, look2, look3" };

    look->setSrc(src.c_str());
    OCIO_CHECK_EQUAL(look->getSrc(), src);

    OCIO_CHECK_THROW_WHAT(look->validate(), OCIO::Exception, "empty destination");

    look->setDst(dst.c_str());
    OCIO_CHECK_EQUAL(look->getDst(), dst);

    OCIO_CHECK_NO_THROW(look->validate());

    look->setLooks(looks.c_str());
    OCIO_CHECK_EQUAL(look->getLooks(), looks);

    OCIO_CHECK_ASSERT(!look->getSkipColorSpaceConversion());
    look->setSkipColorSpaceConversion(true);
    OCIO_CHECK_ASSERT(look->getSkipColorSpaceConversion());

    // Copy and check copy has same values.
    auto tr = look->createEditableCopy();
    look = OCIO_DYNAMIC_POINTER_CAST<OCIO::LookTransform>(tr);
    OCIO_CHECK_EQUAL(look->getSrc(), src);
    OCIO_CHECK_EQUAL(look->getDst(), dst);
    OCIO_CHECK_EQUAL(look->getLooks(), looks);
    OCIO_CHECK_ASSERT(look->getSkipColorSpaceConversion());

    // Using null is similar as using an empty string.
    look->setSrc(nullptr);
    OCIO_CHECK_EQUAL(look->getSrc(), std::string(""));

    look->setDst(nullptr);
    OCIO_CHECK_EQUAL(look->getDst(), std::string(""));
}

namespace
{
void ValidateTransform(OCIO::ConstOpRcPtr & op, const std::string & name,
                       OCIO::TransformDirection dir, unsigned line)
{
    OCIO_REQUIRE_EQUAL_FROM(op->data()->getFormatMetadata().getNumAttributes(), 1, line);
    OCIO_CHECK_EQUAL_FROM(op->data()->getFormatMetadata().getAttributeValue(0), name, line);

    auto ff = OCIO_DYNAMIC_POINTER_CAST<const OCIO::FixedFunctionOpData>(op->data());
    OCIO_REQUIRE_ASSERT_FROM(ff, line);
    OCIO_CHECK_EQUAL_FROM(ff->getDirection(), dir, line);
}
}

OCIO_ADD_TEST(LookTransform, build_look_ops)
{
    constexpr const char * OCIO_CONFIG{ R"(
ocio_profile_version: 2

roles:
  default: raw

displays:
  sRGB:
    - !<View> {name: Raw, colorspace: raw}

looks:
  - !<Look>
    name: look1
    process_space: look1_cs
    transform: !<FixedFunctionTransform> {name: look1 trans, style: ACES_RedMod03}

  - !<Look>
    name: look2
    process_space: look2_3_cs
    transform: !<FixedFunctionTransform> {name: look2 trans, style: ACES_RedMod03}
    inverse_transform: !<FixedFunctionTransform> {name: look2 inverse trans, style: ACES_RedMod03}

  - !<Look>
    name: look3
    process_space: look2_3_cs
    inverse_transform: !<FixedFunctionTransform> {name: look3 inverse trans, style: ACES_RedMod03}

colorspaces:
  - !<ColorSpace>
    name: raw
    family: raw
    bitdepth: 32f
    description: |
      A raw color space. Conversions to and from this space are no-ops.
    isdata: true

  - !<ColorSpace>
    name: source
    to_reference: !<FixedFunctionTransform> {name: src, style: ACES_RedMod03}

  - !<ColorSpace>
    name: destination
    from_reference: !<FixedFunctionTransform> {name: dst, style: ACES_RedMod03}

  - !<ColorSpace>
    name: look1_cs
    to_reference: !<FixedFunctionTransform> {name: look1_cs trans, style: ACES_RedMod03}

  - !<ColorSpace>
    name: look2_3_cs
    to_reference: !<FixedFunctionTransform> {name: look2_3_cs trans, style: ACES_RedMod03}
)" };

    std::istringstream is;
    is.str(OCIO_CONFIG);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    // Create look transform with source and destination color spaces, and 3 looks. Each look
    // has its own process space.
    OCIO::LookTransformRcPtr lt = OCIO::LookTransform::Create();
    lt->setSrc("source");
    lt->setDst("destination");
    lt->setLooks("look1, +look2, -look3");

    // Create ops in forward direction.
    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::BuildLookOps(ops, *config, config->getCurrentContext(),
                                           *lt, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(ops.validate());
    OCIO_REQUIRE_EQUAL(ops.size(), 18); // There are many no-ops.

    // Source color space to look1 process color space.
    // No-ops are created at the beginning and at the end of the color space conversion.
    OCIO::ConstOpRcPtr op = ops[0];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    op = ops[1]; // Source to ref.
    ValidateTransform(op, "src", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);
    op = ops[2]; // Ref to look1_cs.
    ValidateTransform(op, "look1_cs trans", OCIO::TRANSFORM_DIR_INVERSE, __LINE__);
    op = ops[3];
    OCIO_CHECK_ASSERT(op->isNoOpType());

    // Look1 transform.
    op = ops[4]; // No-op added before each look.
    OCIO_CHECK_ASSERT(op->isNoOpType());
    op = ops[5];
    ValidateTransform(op, "look1 trans", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // Look1 process color space to look2 process color space.
    op = ops[6];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    op = ops[7]; // Look1 cs to ref.
    ValidateTransform(op, "look1_cs trans", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);
    op = ops[8]; // Ref to look2_3_cs.
    ValidateTransform(op, "look2_3_cs trans", OCIO::TRANSFORM_DIR_INVERSE, __LINE__);
    op = ops[9];
    OCIO_CHECK_ASSERT(op->isNoOpType());

    // Look2 transform.
    op = ops[10];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    // Look2 has both forward and inverse, using forward.
    op = ops[11];
    ValidateTransform(op, "look2 trans", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // No color space conversion: look2 & look3 have the same process color space.

    // Look3 transform.
    op = ops[12];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    // Look3 is applied with a "-" so want to use the inverse_transform direction.
    op = ops[13];
    ValidateTransform(op, "look3 inverse trans", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // Look3 process color space to destination color space.
    op = ops[14];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    op = ops[15]; // Look2_3_cs to ref.
    ValidateTransform(op, "look2_3_cs trans", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);
    op = ops[16]; // Ref to detination.
    ValidateTransform(op, "dst", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);
    op = ops[17];
    OCIO_CHECK_ASSERT(op->isNoOpType());

    // Test in inverse direction.
    ops.clear();
    OCIO_CHECK_NO_THROW(OCIO::BuildLookOps(ops, *config, config->getCurrentContext(),
                                           *lt, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_NO_THROW(ops.validate());
    OCIO_REQUIRE_EQUAL(ops.size(), 18);

    // Destination color space to Look3 process color space.
    op = ops[0];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    op = ops[1];
    ValidateTransform(op, "dst", OCIO::TRANSFORM_DIR_INVERSE, __LINE__);
    op = ops[2];
    ValidateTransform(op, "look2_3_cs trans", OCIO::TRANSFORM_DIR_INVERSE, __LINE__);
    op = ops[3];
    OCIO_CHECK_ASSERT(op->isNoOpType());

    // Look3 transform.
    op = ops[4];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    op = ops[5];
    // Forward transform is not available, so use the inverse of the inverse_transform.
    ValidateTransform(op, "look3 inverse trans", OCIO::TRANSFORM_DIR_INVERSE, __LINE__);

    // No color space conversion: look3 and look2 have the same process color space.

    // Look2 transform.
    op = ops[6];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    // Look2 has both forward and inverse, using inverse.
    op = ops[7];
    ValidateTransform(op, "look2 inverse trans", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);

    // Look2 process color space to look1 process color space.
    op = ops[8];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    op = ops[9];
    ValidateTransform(op, "look2_3_cs trans", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);
    op = ops[10];
    ValidateTransform(op, "look1_cs trans", OCIO::TRANSFORM_DIR_INVERSE, __LINE__);
    op = ops[11];
    OCIO_CHECK_ASSERT(op->isNoOpType());

    // Look1 transform.
    op = ops[12];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    op = ops[13];
    // Inverse_transform not available so use the inverse of the forward transform.
    ValidateTransform(op, "look1 trans", OCIO::TRANSFORM_DIR_INVERSE, __LINE__);

    // Look1 process color space to source color space.
    op = ops[14];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    op = ops[15];
    ValidateTransform(op, "look1_cs trans", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);
    op = ops[16];
    ValidateTransform(op, "src", OCIO::TRANSFORM_DIR_INVERSE, __LINE__);
    op = ops[17];
    OCIO_CHECK_ASSERT(op->isNoOpType());
}

OCIO_ADD_TEST(LookTransform, build_look_options_ops)
{
    constexpr const char * OCIO_CONFIG{ R"(
ocio_profile_version: 2

roles:
  default: raw

displays:
  sRGB:
    - !<View> {name: Raw, colorspace: raw}

looks:
  - !<Look>
    name: look1
    process_space: raw
    transform: !<FileTransform> {src: missingfile}

  - !<Look>
    name: look2
    process_space: look2_cs
    transform: !<FixedFunctionTransform> {name: look2 trans, style: ACES_RedMod03}

  - !<Look>
    name: look3
    process_space: look3_cs
    transform: !<FixedFunctionTransform> {name: look3 trans, style: ACES_RedMod03}

  - !<Look>
    name: look4
    process_space: look4_cs
    transform: !<FixedFunctionTransform> {name: look4 trans, style: ACES_RedMod03}

  - !<Look>
    name: look5
    process_space: raw
    transform: !<FileTransform> {src: missingfile}

colorspaces:
  - !<ColorSpace>
    name: raw
    family: raw
    bitdepth: 32f
    description: |
      A raw color space. Conversions to and from this space are no-ops.
    isdata: true

  - !<ColorSpace>
    name: source
    to_reference: !<FixedFunctionTransform> {name: src, style: ACES_RedMod03}

  - !<ColorSpace>
    name: destination
    from_reference: !<FixedFunctionTransform> {name: dst, style: ACES_RedMod03}

  - !<ColorSpace>
    name: look2_cs
    to_reference: !<FixedFunctionTransform> {name: look2_cs trans, style: ACES_RedMod03}

  - !<ColorSpace>
    name: look3_cs
    to_reference: !<FixedFunctionTransform> {name: look3_cs trans, style: ACES_RedMod03}

  - !<ColorSpace>
    name: look4_cs
    to_reference: !<FixedFunctionTransform> {name: look4_cs trans, style: ACES_RedMod03}
)" };

    std::istringstream is;
    is.str(OCIO_CONFIG);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->validate());

    // LookTransform options test.  First option (1) gets missing file error,
    // second option (2 & 3) works, third option (3 & 4) not needed.

    OCIO::LookTransformRcPtr lt = OCIO::LookTransform::Create();
    const std::string src{ "source" };
    lt->setSrc(src.c_str());
    const std::string dst{ "destination" };
    lt->setDst(dst.c_str());
    lt->setLooks("look1 | look2, look3 | look3, look4");

    // First option fails with a missing file, second option is fine: look2, look3.
    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::BuildLookOps(ops, *config, config->getCurrentContext(),
                                           *lt, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(ops.validate());
    OCIO_REQUIRE_EQUAL(ops.size(), 16);
    OCIO::ConstOpRcPtr op = ops[0];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    op = ops[1]; // Src to ref.
    ValidateTransform(op, "src", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);
    op = ops[2]; // Ref to look2_cs.
    ValidateTransform(op, "look2_cs trans", OCIO::TRANSFORM_DIR_INVERSE, __LINE__);
    op = ops[3];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    op = ops[4];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    op = ops[5]; // Look2 transform.
    ValidateTransform(op, "look2 trans", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);
    op = ops[6];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    op = ops[7]; // Look2_cs to ref.
    ValidateTransform(op, "look2_cs trans", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);
    op = ops[8]; // Ref to look3_cs.
    ValidateTransform(op, "look3_cs trans", OCIO::TRANSFORM_DIR_INVERSE, __LINE__);
    op = ops[9];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    op = ops[10];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    op = ops[11]; // Look3 transform.
    ValidateTransform(op, "look3 trans", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);
    op = ops[12];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    op = ops[13]; // Look3_cs to ref.
    ValidateTransform(op, "look3_cs trans", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);
    op = ops[14]; // Ref to dst.
    ValidateTransform(op, "dst", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);
    op = ops[15];
    OCIO_CHECK_ASSERT(op->isNoOpType());

    // Test in inverse direction.
    // Options are tried in the same order (3rd option is not tried before second one).
    // Looks of the second option are reversed: look3, look2.
    ops.clear();
    OCIO_CHECK_NO_THROW(OCIO::BuildLookOps(ops, *config, config->getCurrentContext(),
                                           *lt, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_NO_THROW(ops.validate());
    OCIO_REQUIRE_EQUAL(ops.size(), 16);
    op = ops[0];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    op = ops[1];
    ValidateTransform(op, "dst", OCIO::TRANSFORM_DIR_INVERSE, __LINE__);
    op = ops[2];
    ValidateTransform(op, "look3_cs trans", OCIO::TRANSFORM_DIR_INVERSE, __LINE__);
    op = ops[3];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    op = ops[4];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    op = ops[5];
    ValidateTransform(op, "look3 trans", OCIO::TRANSFORM_DIR_INVERSE, __LINE__);
    op = ops[6];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    op = ops[7];
    ValidateTransform(op, "look3_cs trans", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);
    op = ops[8];
    ValidateTransform(op, "look2_cs trans", OCIO::TRANSFORM_DIR_INVERSE, __LINE__);
    op = ops[9];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    op = ops[10];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    op = ops[11];
    ValidateTransform(op, "look2 trans", OCIO::TRANSFORM_DIR_INVERSE, __LINE__);
    op = ops[12];
    OCIO_CHECK_ASSERT(op->isNoOpType());
    op = ops[13];
    ValidateTransform(op, "look2_cs trans", OCIO::TRANSFORM_DIR_FORWARD, __LINE__);
    op = ops[14];
    ValidateTransform(op, "src", OCIO::TRANSFORM_DIR_INVERSE, __LINE__);
    op = ops[15];
    OCIO_CHECK_ASSERT(op->isNoOpType());

    // Replace look3 by look5 so that look options will fail, an exception will be thrown.
    // Looks has three options, first one involves look1 and the other two involve look5.

    lt->setLooks("look1 | look2, look5 | look5, look4");

    OCIO_CHECK_THROW_WHAT(OCIO::BuildLookOps(ops, *config, config->getCurrentContext(),
                                             *lt, OCIO::TRANSFORM_DIR_FORWARD),
                          OCIO::Exception,
                          "The specified file reference 'missingfile' could not be located");

}

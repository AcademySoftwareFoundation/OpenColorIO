// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/lut3d/Lut3DOpData.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(Lut3DOpData, empty)
{
    OCIO::Lut3DOpData l(2);
    OCIO_CHECK_NO_THROW(l.validate());
    OCIO_CHECK_ASSERT(!l.isIdentity());
    OCIO_CHECK_ASSERT(!l.isNoOp());
    OCIO_CHECK_EQUAL(l.getType(), OCIO::OpData::Lut3DType);
    OCIO_CHECK_EQUAL(l.getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_ASSERT(l.hasChannelCrosstalk());
}

OCIO_ADD_TEST(Lut3DOpData, accessors)
{
    OCIO::Interpolation interpol = OCIO::INTERP_LINEAR;

    OCIO::Lut3DOpData l(interpol, 33);
    l.getFormatMetadata().addAttribute(OCIO::METADATA_ID, "uid");

    OCIO_CHECK_EQUAL(l.getInterpolation(), interpol);

    l.getArray()[0] = 1.0f;

    OCIO_CHECK_ASSERT(!l.isIdentity());
    OCIO_CHECK_NO_THROW(l.validate());

    interpol = OCIO::INTERP_TETRAHEDRAL;
    l.setInterpolation(interpol);
    OCIO_CHECK_EQUAL(l.getInterpolation(), interpol);

    OCIO_CHECK_EQUAL(l.getArray().getLength(), (long)33);
    OCIO_CHECK_EQUAL(l.getArray().getNumValues(), (long)33 * 33 * 33 * 3);
    OCIO_CHECK_EQUAL(l.getArray().getNumColorComponents(), (long)3);

    l.getArray().resize(17, 3);

    OCIO_CHECK_EQUAL(l.getArray().getLength(), (long)17);
    OCIO_CHECK_EQUAL(l.getArray().getNumValues(), (long)17 * 17 * 17 * 3);
    OCIO_CHECK_EQUAL(l.getArray().getNumColorComponents(), (long)3);
    OCIO_CHECK_NO_THROW(l.validate());
}

OCIO_ADD_TEST(Lut3DOpData, clone)
{
    OCIO::Lut3DOpData ref(33);
    ref.getArray()[1] = 0.1f;

    OCIO::Lut3DOpDataRcPtr pClone = ref.clone();

    OCIO_CHECK_ASSERT(pClone.get());
    OCIO_CHECK_ASSERT(!pClone->isNoOp());
    OCIO_CHECK_ASSERT(!pClone->isIdentity());
    OCIO_CHECK_NO_THROW(pClone->validate());
    OCIO_CHECK_ASSERT(pClone->getArray()==ref.getArray());
}

OCIO_ADD_TEST(Lut3DOpData, not_supported_length)
{
    OCIO_CHECK_NO_THROW(OCIO::Lut3DOpData{ OCIO::Lut3DOpData::maxSupportedLength });
    OCIO_CHECK_THROW_WHAT(OCIO::Lut3DOpData{ OCIO::Lut3DOpData::maxSupportedLength + 1 },
                          OCIO::Exception, "must not be greater");
}

OCIO_ADD_TEST(Lut3DOpData, equality)
{
    OCIO::Lut3DOpData l1(OCIO::INTERP_LINEAR, 33);

    OCIO::Lut3DOpData l2(OCIO::INTERP_BEST, 33);  

    OCIO_CHECK_ASSERT(!(l1 == l2));

    OCIO::Lut3DOpData l3(OCIO::INTERP_LINEAR, 33);  

    OCIO_CHECK_ASSERT(l1 == l3);
}

OCIO_ADD_TEST(Lut3DOpData, interpolation)
{
    OCIO::Lut3DOpData l(2);

    l.setInterpolation(OCIO::INTERP_LINEAR);
    OCIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_LINEAR);
    OCIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OCIO_CHECK_NO_THROW(l.validate());

    l.setInterpolation(OCIO::INTERP_CUBIC);
    OCIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_CUBIC);
    OCIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OCIO_CHECK_THROW_WHAT(l.validate(), OCIO::Exception, "invalid interpolation");

    l.setInterpolation(OCIO::INTERP_TETRAHEDRAL);
    OCIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_TETRAHEDRAL);
    OCIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_TETRAHEDRAL);
    OCIO_CHECK_NO_THROW(l.validate());

    l.setInterpolation(OCIO::INTERP_DEFAULT);
    OCIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_DEFAULT);
    OCIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OCIO_CHECK_NO_THROW(l.validate());

    l.setInterpolation(OCIO::INTERP_BEST);
    OCIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_BEST);
    OCIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_TETRAHEDRAL);
    OCIO_CHECK_NO_THROW(l.validate());

    // NB: INTERP_NEAREST is currently implemented as INTERP_LINEAR.
    l.setInterpolation(OCIO::INTERP_NEAREST);
    OCIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_NEAREST);
    OCIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OCIO_CHECK_NO_THROW(l.validate());

    // Invalid interpolation type are implemented as INTERP_LINEAR
    // but can not be used because validation throws.
    l.setInterpolation(OCIO::INTERP_UNKNOWN);
    OCIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_UNKNOWN);
    OCIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OCIO_CHECK_THROW_WHAT(l.validate(), OCIO::Exception, "invalid interpolation");
}

OCIO_ADD_TEST(Lut3DOpData, is_inverse)
{
    // Create forward LUT.
    OCIO::Lut3DOpDataRcPtr L1NC = std::make_shared<OCIO::Lut3DOpData>(OCIO::INTERP_LINEAR, 5);
    // Set some metadata.
    L1NC->setName("Forward");
    // Make it not an identity.
    OCIO::Array & array = L1NC->getArray();
    OCIO::Array::Values & values = array.getValues();
    values[0] = 20.f;
    OCIO_CHECK_ASSERT(!L1NC->isIdentity());

    // Create an inverse LUT with same basics.
    OCIO::ConstLut3DOpDataRcPtr L1 = L1NC;
    OCIO::Lut3DOpDataRcPtr L2NC = L1->inverse();
    // Change metadata.
    L2NC->setName("Inverse");
    OCIO::ConstLut3DOpDataRcPtr L2 = L2NC;
    // Inverse and forward.
    OCIO_CHECK_ASSERT(!(*L1 == *L2));

    // Back to forward.
    OCIO::Lut3DOpDataRcPtr L3NC = L2->inverse();
    OCIO::ConstLut3DOpDataRcPtr L3 = L3NC;
    OCIO_CHECK_ASSERT(*L3 == *L1);

    // Check isInverse.
    OCIO_CHECK_ASSERT(L1->isInverse(L2));
    OCIO_CHECK_ASSERT(L2->isInverse(L1));
}

OCIO_ADD_TEST(Lut3DOpData, compose)
{
    const std::string spi3dFile("spi_ocio_srgb_test.spi3d");
    OCIO::OpRcPtrVec ops;

    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, spi3dFile, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    // First op is a FileNoOp.
    OCIO_REQUIRE_EQUAL(2, ops.size());

    const std::string spi3dFile1("comp2.spi3d");
    OCIO::OpRcPtrVec ops1;
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops1, spi3dFile1, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(2, ops1.size());

    OCIO_SHARED_PTR<const OCIO::Op> op0 = ops[1];
    OCIO_SHARED_PTR<const OCIO::Op> op1 = ops1[1];

    OCIO::ConstOpDataRcPtr opData0 = op0->data();
    OCIO::ConstOpDataRcPtr opData1 = op1->data();

    OCIO::ConstLut3DOpDataRcPtr lutData0 =
        OCIO::DynamicPtrCast<const OCIO::Lut3DOpData>(opData0);
    OCIO::ConstLut3DOpDataRcPtr lutData1 =
        OCIO::DynamicPtrCast<const OCIO::Lut3DOpData>(opData1);

    OCIO_REQUIRE_ASSERT(lutData0);
    OCIO_REQUIRE_ASSERT(lutData1);

    OCIO::Lut3DOpDataRcPtr lutData0Cloned = lutData0->clone();
    lutData0Cloned->setName("lut1");
    lutData0Cloned->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION,
                                                        "description of lut1");
    OCIO::Lut3DOpDataRcPtr lutData1Cloned = lutData1->clone();
    lutData1Cloned->setName("lut2");
    lutData1Cloned->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION,
                                                        "description of lut2");
    OCIO::ConstLut3DOpDataRcPtr lutData0ClonedConst = lutData0Cloned;
    OCIO::ConstLut3DOpDataRcPtr lutData1ClonedConst = lutData1Cloned;
    OCIO::Lut3DOpDataRcPtr composed;
    OCIO_CHECK_NO_THROW(composed = OCIO::Lut3DOpData::Compose(lutData0ClonedConst,
                                                              lutData1ClonedConst));

    // FormatMetadata composition.
    OCIO_CHECK_EQUAL(composed->getName(), "lut1 + lut2");
    OCIO_REQUIRE_EQUAL(composed->getFormatMetadata().getNumChildrenElements(), 2);
    const auto & desc1 = composed->getFormatMetadata().getChildElement(0);
    OCIO_CHECK_EQUAL(std::string(desc1.getName()), OCIO::METADATA_DESCRIPTION);
    OCIO_CHECK_EQUAL(std::string(desc1.getValue()), "description of lut1");
    const auto & desc2 = composed->getFormatMetadata().getChildElement(1);
    OCIO_CHECK_EQUAL(std::string(desc2.getName()), OCIO::METADATA_DESCRIPTION);
    OCIO_CHECK_EQUAL(std::string(desc2.getValue()), "description of lut2");

    OCIO_CHECK_EQUAL(composed->getArray().getLength(), (unsigned long)32);
    OCIO_CHECK_EQUAL(composed->getArray().getNumColorComponents(),
                     (unsigned long)3);
    OCIO_CHECK_EQUAL(composed->getArray().getNumValues(),
                     (unsigned long)32 * 32 * 32 * 3);

    OCIO_CHECK_CLOSE(composed->getArray().getValues()[0], 0.0288210791f, 1e-7f);
    OCIO_CHECK_CLOSE(composed->getArray().getValues()[1], 0.0280428901f, 1e-7f);
    OCIO_CHECK_CLOSE(composed->getArray().getValues()[2], 0.0262413863f, 1e-7f);

    OCIO_CHECK_CLOSE(composed->getArray().getValues()[666], 0.0f, 1e-7f);
    OCIO_CHECK_CLOSE(composed->getArray().getValues()[667], 0.274289876f, 1e-7f);
    OCIO_CHECK_CLOSE(composed->getArray().getValues()[668], 0.854598403f, 1e-7f);

    OCIO_CHECK_CLOSE(composed->getArray().getValues()[1800], 0.0f, 1e-7f);
    OCIO_CHECK_CLOSE(composed->getArray().getValues()[1801], 0.411249638f, 1e-7f);
    OCIO_CHECK_CLOSE(composed->getArray().getValues()[1802], 0.881694913f, 1e-7f);

    OCIO_CHECK_CLOSE(composed->getArray().getValues()[96903], 1.0f, 1e-7f);
    OCIO_CHECK_CLOSE(composed->getArray().getValues()[96904], 0.588273168f, 1e-7f);
    OCIO_CHECK_CLOSE(composed->getArray().getValues()[96905], 0.0f, 1e-7f);

}

OCIO_ADD_TEST(Lut3DOpData, compose_2)
{
    const std::string clfFile("clf/lut3d_bizarre.clf");
    OCIO::OpRcPtrVec ops;

    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, clfFile, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(2, ops.size());

    const std::string clfFile1("clf/lut3d_17x17x17_10i_12i.clf");
    OCIO::OpRcPtrVec ops1;
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops1, clfFile1, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(2, ops1.size());

    OCIO_SHARED_PTR<const OCIO::Op> op0 = ops[1];
    OCIO_SHARED_PTR<const OCIO::Op> op1 = ops1[1];

    OCIO::ConstOpDataRcPtr opData0 = op0->data();
    OCIO::ConstOpDataRcPtr opData1 = op1->data();

    OCIO::ConstLut3DOpDataRcPtr lutData0 =
        OCIO::DynamicPtrCast<const OCIO::Lut3DOpData>(opData0);
    OCIO::ConstLut3DOpDataRcPtr lutData1 =
        OCIO::DynamicPtrCast<const OCIO::Lut3DOpData>(opData1);

    OCIO_REQUIRE_ASSERT(lutData0);
    OCIO_REQUIRE_ASSERT(lutData1);

    OCIO::Lut3DOpDataRcPtr composed;
    OCIO_CHECK_NO_THROW(composed = OCIO::Lut3DOpData::Compose(lutData0, lutData1));

    OCIO_CHECK_EQUAL(composed->getArray().getLength(), (unsigned long)17);
    const std::vector<float> & a = composed->getArray().getValues();
    OCIO_CHECK_CLOSE(a[6]    ,    2.5942142f  / 4095.0f, 1e-7f);
    OCIO_CHECK_CLOSE(a[7]    ,   29.60961342f / 4095.0f, 1e-7f);
    OCIO_CHECK_CLOSE(a[8]    ,  154.82646179f / 4095.0f, 1e-7f);
    OCIO_CHECK_CLOSE(a[8289] , 1184.69213867f / 4095.0f, 1e-6f);
    OCIO_CHECK_CLOSE(a[8290] , 1854.97229004f / 4095.0f, 1e-7f);
    OCIO_CHECK_CLOSE(a[8291] , 1996.75830078f / 4095.0f, 1e-7f);
    OCIO_CHECK_CLOSE(a[14736], 4094.07617188f / 4095.0f, 1e-7f);
    OCIO_CHECK_CLOSE(a[14737], 4067.37231445f / 4095.0f, 1e-6f);
    OCIO_CHECK_CLOSE(a[14738], 4088.30493164f / 4095.0f, 1e-6f);
}

OCIO_ADD_TEST(Lut3DOpData, inv_lut3d_lut_size)
{
    const std::string fileName("clf/lut3d_17x17x17_10i_12i.clf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, fileName, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(2, ops.size());

    auto op1 = std::dynamic_pointer_cast<const OCIO::Op>(ops[1]);
    OCIO_REQUIRE_ASSERT(op1);
    auto fwdLutData = std::dynamic_pointer_cast<const OCIO::Lut3DOpData>(op1->data());
    OCIO_REQUIRE_ASSERT(fwdLutData);
    OCIO::ConstLut3DOpDataRcPtr invLutData = fwdLutData->inverse();
    OCIO::Lut3DOpDataRcPtr invFastLutData = MakeFastLut3DFromInverse(invLutData);

    OCIO_CHECK_EQUAL(invFastLutData->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);

    OCIO_CHECK_EQUAL(invFastLutData->getArray().getLength(), 48);
}

OCIO_ADD_TEST(Lut3DOpData, compose_inverse_luts)
{
    OCIO::ConstLut3DOpDataRcPtr lutRef = std::make_shared<OCIO::Lut3DOpData>(5);
    OCIO::Lut3DOpDataRcPtr lut = lutRef->clone();

    auto & lutValues = lut->getArray().getValues();
    for (auto & val : lutValues)
    {
        val *= val;
    }

    OCIO::ConstLut3DOpDataRcPtr lutFwd1 = lut->clone();
    OCIO::ConstLut3DOpDataRcPtr lutFwd2 = lutFwd1->clone();

    // Forward + forward.
    auto compLutFwdFwd = OCIO::Lut3DOpData::Compose(lutFwd1, lutFwd2);
    OCIO_CHECK_EQUAL(compLutFwdFwd->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    // Inverse + inverse.
    lut->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO::ConstLut3DOpDataRcPtr lutInv1 = lut->clone();
    OCIO::ConstLut3DOpDataRcPtr lutInv2 = lutInv1->clone();
    auto compLutInvInv = OCIO::Lut3DOpData::Compose(lutInv1, lutInv1);
    OCIO_CHECK_EQUAL(compLutInvInv->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_ASSERT(compLutFwdFwd->getArray().getValues() ==
                      compLutInvInv->getArray().getValues());

    // Forward + inverse.
    auto compLutFwdInv = OCIO::Lut3DOpData::Compose(lutFwd1, lutInv1);
    OCIO_CHECK_EQUAL(compLutFwdInv->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_CHECK_ASSERT(compLutFwdInv->getArray().getValues() == lutRef->getArray().getValues());

    // Inverse + forward.
    auto compLutInvFwd = OCIO::Lut3DOpData::Compose(lutInv1, lutFwd1);
    OCIO_CHECK_EQUAL(compLutInvFwd->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    constexpr float tol = 1e-5f;
    for (size_t i = 0; i < compLutInvFwd->getArray().getValues().size() / 3; ++i)
    {
        OCIO_CHECK_CLOSE(compLutInvFwd->getArray().getValues()[i * 3],
                         lutRef->getArray().getValues()[i * 3], tol);
    }

}

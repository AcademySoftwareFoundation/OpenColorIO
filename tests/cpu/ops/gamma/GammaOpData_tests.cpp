// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/gamma/GammaOpData.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(GammaOpData, accessors)
{
    const OCIO::GammaOpData::Params paramsR = { 2.4, 0.1 };
    const OCIO::GammaOpData::Params paramsG = { 2.2, 0.2 };
    const OCIO::GammaOpData::Params paramsB = { 2.0, 0.4 };
    const OCIO::GammaOpData::Params paramsA = { 1.8, 0.6 };

    OCIO::GammaOpData g1(OCIO::GammaOpData::MONCURVE_FWD,
                         paramsR, paramsG, paramsB, paramsA);

    OCIO_CHECK_EQUAL(g1.getType(), OCIO::OpData::GammaType);

    OCIO_CHECK_ASSERT(g1.getRedParams()   == paramsR);
    OCIO_CHECK_ASSERT(g1.getGreenParams() == paramsG);
    OCIO_CHECK_ASSERT(g1.getBlueParams()  == paramsB);
    OCIO_CHECK_ASSERT(g1.getAlphaParams() == paramsA);

    OCIO_CHECK_EQUAL(g1.getStyle(), OCIO::GammaOpData::MONCURVE_FWD);

    OCIO_CHECK_ASSERT( ! g1.areAllComponentsEqual() );
    OCIO_CHECK_ASSERT( ! g1.isNonChannelDependent() );
    OCIO_CHECK_ASSERT( ! g1.isAlphaComponentIdentity() );


    // Set R, G and B params to paramsR, A set to identity.
    g1.setParams(paramsR);

    OCIO_CHECK_ASSERT(!g1.areAllComponentsEqual());
    OCIO_CHECK_ASSERT(g1.isNonChannelDependent());
    OCIO_CHECK_ASSERT(g1.isAlphaComponentIdentity());

    OCIO_CHECK_ASSERT(g1.getGreenParams() == paramsR);
    OCIO_CHECK_ASSERT(
        OCIO::GammaOpData::isIdentityParameters(g1.getAlphaParams(), 
                                                g1.getStyle()));

    g1.setAlphaParams(paramsR);
    OCIO_CHECK_ASSERT(g1.areAllComponentsEqual());

    g1.setBlueParams(paramsB);
    OCIO_CHECK_ASSERT(g1.getBlueParams() == paramsB);

    OCIO_CHECK_ASSERT(!g1.areAllComponentsEqual());

    g1.setRedParams(paramsB);
    OCIO_CHECK_ASSERT(g1.getRedParams() == paramsB);

    g1.setGreenParams(paramsB);
    OCIO_CHECK_ASSERT(g1.getGreenParams() == paramsB);

    g1.setAlphaParams(paramsA);
    OCIO_CHECK_ASSERT(g1.getAlphaParams() == paramsA);

    g1.setStyle(OCIO::GammaOpData::MONCURVE_REV);
    OCIO_CHECK_EQUAL(g1.getStyle(), OCIO::GammaOpData::MONCURVE_REV);
}

OCIO_ADD_TEST(GammaOpData, identity_style_basic)
{
    const OCIO::GammaOpData::Params IdentityParams
        = OCIO::GammaOpData::getIdentityParameters(OCIO::GammaOpData::BASIC_FWD);

    {
        //
        // Basic identity gamma.
        //
        OCIO::GammaOpData g(OCIO::GammaOpData::BASIC_FWD,
                            IdentityParams, IdentityParams,
                            IdentityParams, IdentityParams);
        OCIO_CHECK_ASSERT(g.isIdentity());
        OCIO_CHECK_ASSERT(!g.isNoOp()); // inBitDepth != outBitDepth
        OCIO_CHECK_ASSERT(g.isChannelIndependent());
    }

    {
        //
        // Default constructor test:
        // gamma op is BASIC_FWD, in/out bit depth 32f.
        //
        OCIO::GammaOpData g;
        g.setParams(IdentityParams);
        g.validate();
        OCIO_CHECK_EQUAL(g.getStyle(), OCIO::GammaOpData::BASIC_FWD);
        OCIO_CHECK_ASSERT(g.isIdentity());
        OCIO_CHECK_ASSERT(!g.isNoOp()); // inBitDepth != outBitDepth
        OCIO_CHECK_ASSERT(g.isChannelIndependent());
    }

    const OCIO::GammaOpData::Params paramsR = { 1.2 };
    const OCIO::GammaOpData::Params paramsG = { 1.6 };
    const OCIO::GammaOpData::Params paramsB = { 2.0 };
    const OCIO::GammaOpData::Params paramsA = { 3.1 };

    {
        //
        // Non-identity check for basic style.
        //
        OCIO::GammaOpData g(OCIO::GammaOpData::BASIC_FWD,
                            paramsR, paramsG, paramsB, paramsA);
        OCIO_CHECK_ASSERT(!g.isIdentity());
        OCIO_CHECK_ASSERT(!g.isNoOp());
        OCIO_CHECK_ASSERT(g.isChannelIndependent());
    }

    {
        //
        // Non-identity check for default constructor.
        // Default gamma op is BASIC_FWD, in/out bitDepth 32f.
        //
        OCIO::GammaOpData g;
        OCIO_CHECK_ASSERT(g.isIdentity());
        OCIO_CHECK_ASSERT(!g.isNoOp()); // basic style clamps, so it isn't a no-op
        OCIO_CHECK_ASSERT(g.isChannelIndependent());

        g.setParams(paramsR);
        g.validate();

        OCIO_CHECK_EQUAL(g.getStyle(), OCIO::GammaOpData::BASIC_FWD);
        OCIO_CHECK_ASSERT(!g.isIdentity());
        OCIO_CHECK_ASSERT(!g.isNoOp());
        OCIO_CHECK_ASSERT(g.isChannelIndependent());
    }
}

OCIO_ADD_TEST(GammaOpData, identity_style_moncurve)
{
    const OCIO::GammaOpData::Params IdentityParams
      = OCIO::GammaOpData::getIdentityParameters(OCIO::GammaOpData::MONCURVE_FWD);

    {
        //
        // Identity test for moncurve.
        //
        OCIO::GammaOpData g(OCIO::GammaOpData::MONCURVE_FWD,
                            IdentityParams, IdentityParams,
                            IdentityParams, IdentityParams);
        OCIO_CHECK_ASSERT(g.isIdentity());
        OCIO_CHECK_ASSERT(g.isNoOp());
        OCIO_CHECK_ASSERT(g.isChannelIndependent());
    }

    {
        //
        // Identity test for forward moncurve with default constructor.
        // Default gamma op is BASIC_FWD, in/out bitDepth 32f.
        //
        OCIO::GammaOpData g;
        g.setStyle(OCIO::GammaOpData::MONCURVE_FWD);
        g.setParams(IdentityParams);
        g.validate();
        OCIO_CHECK_ASSERT(g.isIdentity());
        OCIO_CHECK_ASSERT(g.isNoOp());
        OCIO_CHECK_ASSERT(g.isChannelIndependent());
    }

    const OCIO::GammaOpData::Params paramsR = { 1.2, 0.2 };
    const OCIO::GammaOpData::Params paramsG = { 1.6, 0.7 };
    const OCIO::GammaOpData::Params paramsB = { 2.0, 0.5 };
    const OCIO::GammaOpData::Params paramsA = { 3.1, 0.1 };

    {
        //
        // Non-identity test for moncurve.
        //
        OCIO::GammaOpData g(OCIO::GammaOpData::MONCURVE_FWD,
                            paramsR, paramsG, paramsB, paramsA);
        OCIO_CHECK_ASSERT(!g.isIdentity());
        OCIO_CHECK_ASSERT(!g.isNoOp());
        OCIO_CHECK_ASSERT(g.isChannelIndependent());
    }

    {
        //
        // Non-identity test for moncurve with default constructor.
        // Default gamma op is BASIC_FWD, in/out bitDepth 32f.
        //
        OCIO::GammaOpData g;
        g.setStyle(OCIO::GammaOpData::MONCURVE_FWD);
        g.setParams(paramsR);
        g.validate();

        OCIO_CHECK_ASSERT(!g.isIdentity());
        OCIO_CHECK_ASSERT(!g.isNoOp());
        OCIO_CHECK_ASSERT(g.isChannelIndependent());
    }
}

OCIO_ADD_TEST(GammaOpData, noop_style_basic)
{
    // Test basic gamma
    const OCIO::GammaOpData::Params IdentityParams
        = OCIO::GammaOpData::getIdentityParameters(OCIO::GammaOpData::BASIC_FWD);

    {
        //
        // NoOp test, basic style.
        //
        OCIO::GammaOpData g(OCIO::GammaOpData::BASIC_FWD,
                            IdentityParams, IdentityParams,
                            IdentityParams, IdentityParams);
        OCIO_CHECK_ASSERT(g.isIdentity());
        OCIO_CHECK_ASSERT(!g.isNoOp()); // basic style clamps, so it isn't a no-op
        OCIO_CHECK_ASSERT(g.isChannelIndependent());
    }

    const OCIO::GammaOpData::Params paramsR = { 1.2 };
    const OCIO::GammaOpData::Params paramsG = { 1.6 };
    const OCIO::GammaOpData::Params paramsB = { 2.0 };
    const OCIO::GammaOpData::Params paramsA = { 3.1 };

    {
        //
        // Non-NoOp test, basic style.
        //
        OCIO::GammaOpData g(OCIO::GammaOpData::BASIC_FWD,
                            paramsR, paramsG, paramsB, paramsA);
        OCIO_CHECK_ASSERT(!g.isIdentity());
        OCIO_CHECK_ASSERT(!g.isNoOp());
        OCIO_CHECK_ASSERT(g.isChannelIndependent());
    }

}

OCIO_ADD_TEST(GammaOpData, noop_style_moncurve)
{
    // Test monCurve gamma
    const OCIO::GammaOpData::Params IdentityParams
        = OCIO::GammaOpData::getIdentityParameters(OCIO::GammaOpData::MONCURVE_FWD);

    {
        //
        // NoOp test, moncurve style.
        //
        OCIO::GammaOpData g(OCIO::GammaOpData::MONCURVE_FWD,
                            IdentityParams, IdentityParams,
                            IdentityParams, IdentityParams);
        OCIO_CHECK_ASSERT(g.isIdentity());
        OCIO_CHECK_ASSERT(g.isNoOp());
        OCIO_CHECK_ASSERT(g.isChannelIndependent());
    }

    const OCIO::GammaOpData::Params paramsR = { 1.2, 0.2 };
    const OCIO::GammaOpData::Params paramsG = { 1.6, 0.7 };
    const OCIO::GammaOpData::Params paramsB = { 2.0, 0.5 };
    const OCIO::GammaOpData::Params paramsA = { 3.1, 0.1 };

    {
        //
        // Non-NoOp test, moncurve style.
        //
        OCIO::GammaOpData g(OCIO::GammaOpData::MONCURVE_FWD,
                            paramsR, paramsG, paramsB, paramsA);
        OCIO_CHECK_ASSERT(!g.isIdentity());
        OCIO_CHECK_ASSERT(!g.isNoOp());
        OCIO_CHECK_ASSERT(g.isChannelIndependent());
    }

}

OCIO_ADD_TEST(GammaOpData, validate)
{
    const OCIO::GammaOpData::Params params = { 2.6 };

    const OCIO::GammaOpData::Params paramsR = { 2.4, 0.1 };
    const OCIO::GammaOpData::Params paramsG = { 2.2, 0.2 };
    const OCIO::GammaOpData::Params paramsB = { 2.0, 0.4 };
    const OCIO::GammaOpData::Params paramsA = { 1.8, 0.6 };

    {
        OCIO::GammaOpData g1(OCIO::GammaOpData::MONCURVE_FWD,
                             paramsR, paramsG, params, paramsA);
        OCIO_CHECK_THROW_WHAT(g1.validate(),
                              OCIO::Exception,
                              "GammaOp: Wrong number of parameters");
    }

    {
        OCIO::GammaOpData g1(OCIO::GammaOpData::BASIC_FWD,
                             paramsB, paramsB, paramsB, paramsB);
        OCIO_CHECK_THROW_WHAT(g1.validate(),
                              OCIO::Exception,
                              "GammaOp: Wrong number of parameters");
    }

    {
        const OCIO::GammaOpData::Params params1 = { 0.006 }; // valid range is [0.01, 100]

        OCIO::GammaOpData g1(OCIO::GammaOpData::BASIC_FWD,
                             params1, params1, params1, params1);
        OCIO_CHECK_THROW_WHAT(g1.validate(),
                              OCIO::Exception,
                              "Parameter 0.006 is less than lower bound 0.01");
    }

    {
        const OCIO::GammaOpData::Params params1 = { 110. }; // valid range is [0.01, 100]

        OCIO::GammaOpData g1(OCIO::GammaOpData::BASIC_FWD,
                             params1, params1, params1, params1);
        OCIO_CHECK_THROW_WHAT(g1.validate(),
                              OCIO::Exception,
                              "Parameter 110 is greater than upper bound 100");
    }


    {
        const OCIO::GammaOpData::Params params1 = { 1.,    // valid range is [1, 10]
                                                    11. }; // valid range is [0, 0.9]

        OCIO::GammaOpData g1(OCIO::GammaOpData::MONCURVE_FWD,
                             params1, params1, params1, params1);
        OCIO_CHECK_THROW_WHAT(g1.validate(),
                              OCIO::Exception,
                              "Parameter 11 is greater than upper bound 0.9");
    }

    {
        const OCIO::GammaOpData::Params params1 = { 1.,   // valid range is [1, 10]
                                                    0. }; // valid range is [0, 0.9]

        OCIO::GammaOpData g1(OCIO::GammaOpData::MONCURVE_FWD,
                             params1, params1, params1, params1);

        OCIO_CHECK_NO_THROW( g1.validate() );
    }

    {
        const OCIO::GammaOpData::Params params1 = { 1.,     // valid range is [1, 10]
                                                   -1e-6 }; // valid range is [0, 0.9]

        OCIO::GammaOpData g1(OCIO::GammaOpData::MONCURVE_FWD,
                             params1, params1, params1, params1);
        OCIO_CHECK_THROW_WHAT(g1.validate(),
                              OCIO::Exception,
                              "Parameter -1e-06 is less than lower bound 0");
    }
}

OCIO_ADD_TEST(GammaOpData, equality)
{
    const OCIO::GammaOpData::Params paramsR1 = { 2.4, 0.1 };
    const OCIO::GammaOpData::Params paramsG1 = { 2.2, 0.2 };
    const OCIO::GammaOpData::Params paramsB1 = { 2.0, 0.4 };
    const OCIO::GammaOpData::Params paramsA1 = { 1.8, 0.6 };

    OCIO::GammaOpData g1(OCIO::GammaOpData::MONCURVE_FWD,
                         paramsR1, paramsG1, paramsB1, paramsA1);

    const OCIO::GammaOpData::Params paramsR2 = { 2.6, 0.1 }; // 2.6 != 2.4
    const OCIO::GammaOpData::Params paramsG2 = paramsG1;
    const OCIO::GammaOpData::Params paramsB2 = paramsB1;
    const OCIO::GammaOpData::Params paramsA2 = paramsA1;

    OCIO::GammaOpData g2(OCIO::GammaOpData::MONCURVE_FWD,
                         paramsR2, paramsG2, paramsB2, paramsA2);

    OCIO_CHECK_ASSERT(!(g1 == g2));

    OCIO::GammaOpData g3(OCIO::GammaOpData::MONCURVE_REV,
                         paramsR1, paramsG1, paramsB1, paramsA1);

    OCIO_CHECK_ASSERT(!(g3 == g1));

    g3.setStyle(g1.getStyle());
    g3.validate();

    OCIO_CHECK_ASSERT(g3 == g1);

    OCIO::GammaOpData g4(OCIO::GammaOpData::MONCURVE_FWD,
                         paramsR1, paramsG1, paramsB1, paramsA1);

    OCIO_CHECK_ASSERT(g4 == g1);
}

namespace
{

void CheckGammaInverse(OCIO::GammaOpData::Style refStyle,
                       const OCIO::GammaOpData::Params & refParamsR,
                       const OCIO::GammaOpData::Params & refParamsG,
                       const OCIO::GammaOpData::Params & refParamsB,
                       const OCIO::GammaOpData::Params & refParamsA,
                       OCIO::GammaOpData::Style invStyle,
                       const OCIO::GammaOpData::Params & invParamsR,
                       const OCIO::GammaOpData::Params & invParamsG,
                       const OCIO::GammaOpData::Params & invParamsB,
                       const OCIO::GammaOpData::Params & invParamsA )
{
    OCIO::GammaOpData refGammaOp(refStyle,
                                 refParamsR,
                                 refParamsG,
                                 refParamsB,
                                 refParamsA);

    OCIO::GammaOpDataRcPtr invOp = refGammaOp.inverse();

    OCIO_CHECK_EQUAL(invOp->getStyle(), invStyle);

    OCIO_CHECK_ASSERT(invOp->getRedParams()   == invParamsR);
    OCIO_CHECK_ASSERT(invOp->getGreenParams() == invParamsG);
    OCIO_CHECK_ASSERT(invOp->getBlueParams()  == invParamsB);
    OCIO_CHECK_ASSERT(invOp->getAlphaParams() == invParamsA);

    OCIO_CHECK_ASSERT(refGammaOp.isInverse(*invOp));
    OCIO_CHECK_ASSERT(invOp->isInverse(refGammaOp));
    OCIO_CHECK_ASSERT(!refGammaOp.isInverse(refGammaOp));
    OCIO_CHECK_ASSERT(!invOp->isInverse(*invOp));
}

};

OCIO_ADD_TEST(GammaOpData, basic_inverse)
{
    const OCIO::GammaOpData::Params paramsR = { 2.2 };
    const OCIO::GammaOpData::Params paramsG = { 2.4 };
    const OCIO::GammaOpData::Params paramsB = { 2.6 };
    const OCIO::GammaOpData::Params paramsA = { 2.8 };

    CheckGammaInverse(OCIO::GammaOpData::BASIC_FWD, paramsR, paramsG, paramsB, paramsA,
                      OCIO::GammaOpData::BASIC_REV, paramsR, paramsG, paramsB, paramsA);

    CheckGammaInverse(OCIO::GammaOpData::BASIC_REV, paramsR, paramsG, paramsB, paramsA,
                      OCIO::GammaOpData::BASIC_FWD, paramsR, paramsG, paramsB, paramsA);
}

OCIO_ADD_TEST(GammaOpData, moncurve_inverse)
{
    const OCIO::GammaOpData::Params paramsR = { 2.4, 0.1 };
    const OCIO::GammaOpData::Params paramsG = { 2.2, 0.2 };
    const OCIO::GammaOpData::Params paramsB = { 2.0, 0.4 };
    const OCIO::GammaOpData::Params paramsA = { 1.8, 0.6 };


    CheckGammaInverse(OCIO::GammaOpData::MONCURVE_FWD, paramsR, paramsG, paramsB, paramsA,
                      OCIO::GammaOpData::MONCURVE_REV, paramsR, paramsG, paramsB, paramsA);

    CheckGammaInverse(OCIO::GammaOpData::MONCURVE_REV, paramsR, paramsG, paramsB, paramsA,
                      OCIO::GammaOpData::MONCURVE_FWD, paramsR, paramsG, paramsB, paramsA);
}

OCIO_ADD_TEST(GammaOpData, is_inverse)
{
    // NB: isInverse ignores bit-depth.

    // See also addtl tests in CheckGammaInverse() above.
    // Just need to test that if params are unequal it is not an inverse.
    OCIO::GammaOpData::Params paramsR = { 2.4  };   // gamma
    OCIO::GammaOpData::Params paramsG = { 2.41 };   // gamma

    OCIO::GammaOpData GammaOp1(OCIO::GammaOpData::BASIC_FWD, 
                               paramsR, paramsG, paramsR, paramsR);

    OCIO::GammaOpData GammaOp2(OCIO::GammaOpData::BASIC_REV, 
                               paramsR, paramsG, paramsR, paramsR);

    // Set B param differently.
    OCIO::GammaOpData GammaOp3(OCIO::GammaOpData::BASIC_REV, 
                               paramsR, paramsG, paramsG, paramsR);

    OCIO_CHECK_ASSERT(GammaOp1.isInverse(GammaOp2));
    OCIO_CHECK_ASSERT(!GammaOp1.isInverse(GammaOp3));

    paramsR.push_back(0.1);    // offset
    paramsG.push_back(0.1);    // offset

    OCIO::GammaOpData GammaOp1m(OCIO::GammaOpData::MONCURVE_FWD, 
                                paramsR, paramsG, paramsR, paramsR);

    OCIO::GammaOpData GammaOp2m(OCIO::GammaOpData::MONCURVE_REV, 
                                paramsR, paramsG, paramsR, paramsR);

    // Set blue param differently.
    OCIO::GammaOpData GammaOp3m(OCIO::GammaOpData::MONCURVE_REV, 
                                paramsR, paramsG, paramsG, paramsR);

    OCIO_CHECK_ASSERT(GammaOp1m.isInverse(GammaOp2m));
    OCIO_CHECK_ASSERT(!GammaOp1m.isInverse(GammaOp3m));
}

namespace
{
void TestMayComposeStyle(OCIO::GammaOpData::Style s1, OCIO::GammaOpData::Style s2,
                         bool expected, unsigned line)
{
    OCIO::GammaOpData::Params params = { 2. };
    OCIO::GammaOpData g1(s1, params, params, params, params);
    OCIO::GammaOpData g2(s2, params, params, params, params);
    OCIO_CHECK_EQUAL_FROM(g1.mayCompose(g2), expected, line);
    OCIO_CHECK_EQUAL_FROM(g2.mayCompose(g1), expected, line);
}
}

OCIO_ADD_TEST(GammaOpData, mayCompose)
{
    TestMayComposeStyle(OCIO::GammaOpData::BASIC_FWD,
                        OCIO::GammaOpData::BASIC_FWD, true, __LINE__);
    TestMayComposeStyle(OCIO::GammaOpData::BASIC_FWD,
                        OCIO::GammaOpData::BASIC_REV, true, __LINE__);
    TestMayComposeStyle(OCIO::GammaOpData::BASIC_REV,
                        OCIO::GammaOpData::BASIC_REV, true, __LINE__);
    TestMayComposeStyle(OCIO::GammaOpData::BASIC_FWD,
                        OCIO::GammaOpData::BASIC_MIRROR_FWD, true, __LINE__);
    TestMayComposeStyle(OCIO::GammaOpData::BASIC_FWD,
                        OCIO::GammaOpData::BASIC_MIRROR_REV, true, __LINE__);
    TestMayComposeStyle(OCIO::GammaOpData::BASIC_REV,
                        OCIO::GammaOpData::BASIC_MIRROR_FWD, true, __LINE__);
    TestMayComposeStyle(OCIO::GammaOpData::BASIC_REV,
                        OCIO::GammaOpData::BASIC_MIRROR_REV, true, __LINE__);
    TestMayComposeStyle(OCIO::GammaOpData::BASIC_FWD,
                        OCIO::GammaOpData::BASIC_PASS_THRU_FWD, true, __LINE__);
    TestMayComposeStyle(OCIO::GammaOpData::BASIC_FWD,
                        OCIO::GammaOpData::BASIC_PASS_THRU_REV, true, __LINE__);
    TestMayComposeStyle(OCIO::GammaOpData::BASIC_REV,
                        OCIO::GammaOpData::BASIC_PASS_THRU_FWD, true, __LINE__);
    TestMayComposeStyle(OCIO::GammaOpData::BASIC_REV,
                        OCIO::GammaOpData::BASIC_PASS_THRU_REV, true, __LINE__);
    TestMayComposeStyle(OCIO::GammaOpData::BASIC_MIRROR_FWD,
                        OCIO::GammaOpData::BASIC_MIRROR_FWD, true, __LINE__);
    TestMayComposeStyle(OCIO::GammaOpData::BASIC_MIRROR_REV,
                        OCIO::GammaOpData::BASIC_MIRROR_REV, true, __LINE__);
    TestMayComposeStyle(OCIO::GammaOpData::BASIC_MIRROR_REV,
                        OCIO::GammaOpData::BASIC_MIRROR_FWD, true, __LINE__);
    TestMayComposeStyle(OCIO::GammaOpData::BASIC_PASS_THRU_FWD,
                        OCIO::GammaOpData::BASIC_PASS_THRU_FWD, true, __LINE__);
    TestMayComposeStyle(OCIO::GammaOpData::BASIC_PASS_THRU_REV,
                        OCIO::GammaOpData::BASIC_PASS_THRU_REV, true, __LINE__);
    TestMayComposeStyle(OCIO::GammaOpData::BASIC_PASS_THRU_FWD,
                        OCIO::GammaOpData::BASIC_PASS_THRU_REV, true, __LINE__);
    TestMayComposeStyle(OCIO::GammaOpData::BASIC_MIRROR_FWD,
                        OCIO::GammaOpData::BASIC_PASS_THRU_FWD, false, __LINE__);
    TestMayComposeStyle(OCIO::GammaOpData::BASIC_MIRROR_FWD,
                        OCIO::GammaOpData::BASIC_PASS_THRU_REV, false, __LINE__);
    TestMayComposeStyle(OCIO::GammaOpData::BASIC_MIRROR_REV,
                        OCIO::GammaOpData::BASIC_PASS_THRU_FWD, false, __LINE__);
    TestMayComposeStyle(OCIO::GammaOpData::BASIC_MIRROR_REV,
                        OCIO::GammaOpData::BASIC_PASS_THRU_REV, false, __LINE__);

    OCIO::GammaOpData::Params params1 = { 1.  };
    OCIO::GammaOpData::Params params2 = { 2.2 };
    OCIO::GammaOpData::Params params3 = { 2.6 };

    {
        // R == G != B params.
        OCIO::GammaOpData g1(OCIO::GammaOpData::BASIC_FWD,
                             params2, params2, params1, params1);
        OCIO::GammaOpData g2(OCIO::GammaOpData::BASIC_FWD,
                             params2, params2, params2, params1);
        OCIO_CHECK_ASSERT(g1.mayCompose(g2));
    }

    {
        OCIO::GammaOpData g1(OCIO::GammaOpData::BASIC_FWD,
                             params2, params2, params2, params1);
        params1.push_back(0.0);
        params3.push_back(0.1);
        OCIO::GammaOpData g2(OCIO::GammaOpData::MONCURVE_FWD,
                             params3, params3, params3, params1);
        // Moncurve not allowed.
        OCIO_CHECK_ASSERT(!g1.mayCompose(g2));
    }
}

namespace
{

void CheckGammaCompose(OCIO::GammaOpData::Style style1,
                       const OCIO::GammaOpData::Params & params1,
                       OCIO::GammaOpData::Style style2,
                       const OCIO::GammaOpData::Params & params2,
                       OCIO::GammaOpData::Style refStyle,
                       const OCIO::GammaOpData::Params & refParams)
{
    static const OCIO::GammaOpData::Params paramsA = { 1. };

    const OCIO::GammaOpData g1(style1, 
                               params1, params1, params1, paramsA);

    const OCIO::GammaOpData g2(style2, 
                               params2, params2, params2, paramsA);

    const OCIO::GammaOpDataRcPtr g3 = g1.compose(g2);

    OCIO_CHECK_EQUAL(g3->getStyle(), refStyle);

    OCIO_CHECK_ASSERT(g3->getRedParams()   == refParams);
    OCIO_CHECK_ASSERT(g3->getGreenParams() == refParams);
    OCIO_CHECK_ASSERT(g3->getBlueParams()  == refParams);
    OCIO_CHECK_ASSERT(g3->getAlphaParams() == paramsA);
}

};

OCIO_ADD_TEST(GammaOpData, compose)
{
    {
        const OCIO::GammaOpData::Params params1 = { 2. };
        const OCIO::GammaOpData::Params params2 = { 3. };
        const OCIO::GammaOpData::Params refParams = { 6. };

        CheckGammaCompose(OCIO::GammaOpData::BASIC_FWD, params1,
                          OCIO::GammaOpData::BASIC_FWD, params2,
                          OCIO::GammaOpData::BASIC_FWD, refParams);
    }

    {
        const OCIO::GammaOpData::Params params1 = { 2. };
        const OCIO::GammaOpData::Params params2 = { 4. };
        const OCIO::GammaOpData::Params refParams = { 8. };

        CheckGammaCompose(OCIO::GammaOpData::BASIC_REV, params1,
                          OCIO::GammaOpData::BASIC_REV, params2,
                          OCIO::GammaOpData::BASIC_REV, refParams);
    }

    {
        const OCIO::GammaOpData::Params params1 = { 4. };
        const OCIO::GammaOpData::Params params2 = { 2. };
        const OCIO::GammaOpData::Params refParams = { 2. };

        CheckGammaCompose(OCIO::GammaOpData::BASIC_REV, params1,
                          OCIO::GammaOpData::BASIC_FWD, params2,
                          OCIO::GammaOpData::BASIC_REV, refParams);
    }

    {
        const OCIO::GammaOpData::Params params1 = { 2. };
        const OCIO::GammaOpData::Params params2 = { 4. };
        const OCIO::GammaOpData::Params refParams = { 2. };

        CheckGammaCompose(OCIO::GammaOpData::BASIC_REV, params1,
                          OCIO::GammaOpData::BASIC_FWD, params2,
                          OCIO::GammaOpData::BASIC_FWD, refParams);
    }

    {
        const OCIO::GammaOpData::Params params1 = { 4. };
        OCIO::GammaOpData::Params paramsA = { 1. };
        OCIO::GammaOpData g1(OCIO::GammaOpData::BASIC_REV, 
                             params1, params1, params1, paramsA);

        OCIO::GammaOpData::Params params2 = {2., 0.1};
        paramsA.push_back(0.0);

        OCIO::GammaOpData g2(OCIO::GammaOpData::MONCURVE_REV, 
                             params2, params2, params2, paramsA);

        OCIO_CHECK_THROW_WHAT(g1.compose(g2), 
                              OCIO::Exception, 
                              "GammaOp can only be combined with some GammaOps");
    }
}


// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/fixedfunction/FixedFunctionOpData.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(FixedFunctionOpData, aces_red_mod_style)
{
    OCIO::FixedFunctionOpData func{ OCIO::FixedFunctionOpData::ACES_RED_MOD_03_FWD };
    OCIO_CHECK_EQUAL(func.getStyle(), OCIO::FixedFunctionOpData::ACES_RED_MOD_03_FWD);
    OCIO_CHECK_EQUAL(func.getParams().size(), 0);
    OCIO_CHECK_NO_THROW(func.validate());
    std::string cacheID;
    OCIO_CHECK_NO_THROW(cacheID = func.getCacheID());

    OCIO_CHECK_NO_THROW(func.setStyle(OCIO::FixedFunctionOpData::ACES_RED_MOD_10_FWD));
    OCIO_CHECK_EQUAL(func.getStyle(), OCIO::FixedFunctionOpData::ACES_RED_MOD_10_FWD);
    OCIO_CHECK_NO_THROW(func.validate());

    std::string cacheIDUpdated;
    OCIO_CHECK_NO_THROW(cacheIDUpdated = func.getCacheID());
    OCIO_CHECK_ASSERT(cacheID != cacheIDUpdated);

    OCIO::FixedFunctionOpDataRcPtr inv = func.inverse();
    OCIO_CHECK_EQUAL(inv->getStyle(), OCIO::FixedFunctionOpData::ACES_RED_MOD_10_INV);
    OCIO_CHECK_EQUAL(inv->getParams().size(), 0);
    OCIO_CHECK_NO_THROW(cacheIDUpdated = inv->getCacheID());
    OCIO_CHECK_ASSERT(cacheID != cacheIDUpdated);

    OCIO::FixedFunctionOpData::Params p = func.getParams();
    p.push_back(1.);
    OCIO_CHECK_NO_THROW(func.setParams(p));
    OCIO_CHECK_THROW_WHAT(func.validate(), 
                          OCIO::Exception, 
                          "The style 'ACES_RedMod10 (Forward)' must have zero parameters but 1 found.");
}

OCIO_ADD_TEST(FixedFunctionOpData, aces_dark_to_dim10_style)
{
    OCIO::FixedFunctionOpData func(OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD,
                                   OCIO::FixedFunctionOpData::Params());

    OCIO_CHECK_EQUAL(func.getStyle(), OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD);
    OCIO_CHECK_EQUAL(func.getParams().size(), 0);
    OCIO_CHECK_NO_THROW(func.validate());
    std::string cacheID;
    OCIO_CHECK_NO_THROW(cacheID = func.getCacheID());

    OCIO::FixedFunctionOpDataRcPtr inv = func.inverse();
    OCIO_CHECK_EQUAL(inv->getStyle(), OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_INV);
    OCIO_CHECK_EQUAL(inv->getParams().size(), 0);
    std::string cacheIDUpdated;
    OCIO_CHECK_NO_THROW(cacheIDUpdated = inv->getCacheID());
    OCIO_CHECK_ASSERT(cacheID != cacheIDUpdated);

    OCIO::FixedFunctionOpData::Params p = func.getParams();
    p.push_back(1.);
    OCIO_CHECK_NO_THROW(func.setParams(p));
    OCIO_CHECK_THROW_WHAT(func.validate(), 
                          OCIO::Exception, 
                          "The style 'ACES_DarkToDim10 (Forward)' must have zero parameters but 1 found.");
}

OCIO_ADD_TEST(FixedFunctionOpData, aces_gamut_comp_13_style)
{
    OCIO::FixedFunctionOpData::Params params = { 1.147, 1.264, 1.312, 0.815, 0.803, 0.880, 1.2 };
    OCIO::FixedFunctionOpData func(OCIO::FixedFunctionOpData::ACES_GAMUT_COMP_13_FWD, params);
    OCIO_CHECK_NO_THROW(func.validate());
    std::string cacheID;
    OCIO_CHECK_NO_THROW(cacheID = func.getCacheID());
    OCIO_CHECK_ASSERT(func.getParams() == params);

    OCIO::FixedFunctionOpDataRcPtr inv = func.inverse();
    OCIO_CHECK_EQUAL(inv->getParams()[0], func.getParams()[0]);
    OCIO_CHECK_EQUAL(inv->getStyle(), OCIO::FixedFunctionOpData::ACES_GAMUT_COMP_13_INV);
    std::string cacheIDUpdated;
    OCIO_CHECK_NO_THROW(cacheIDUpdated = inv->getCacheID());
    OCIO_CHECK_ASSERT(cacheID != cacheIDUpdated);

    OCIO_CHECK_ASSERT(func == func);
    OCIO_CHECK_ASSERT(!(func == *inv));

    auto test_params = params;
    test_params.push_back(12);
    OCIO_CHECK_NO_THROW(func.setParams(test_params));
    OCIO_CHECK_THROW_WHAT(func.validate(),
                          OCIO::Exception,
                          "The style 'ACES_GamutComp13 (Forward)' must have "
                          "seven parameters but 8 found.");

    test_params = params;
    test_params.pop_back();
    OCIO_CHECK_NO_THROW(func.setParams(test_params));
    OCIO_CHECK_THROW_WHAT(func.validate(),
                          OCIO::Exception,
                          "The style 'ACES_GamutComp13 (Forward)' must have "
                          "seven parameters but 6 found.");

    test_params = params;
    test_params.clear();
    OCIO_CHECK_NO_THROW(func.setParams(test_params));
    OCIO_CHECK_THROW_WHAT(func.validate(),
                          OCIO::Exception,
                          "The style 'ACES_GamutComp13 (Forward)' must have "
                          "seven parameters but 0 found.");


    test_params = params;
    test_params[0] = 1.0;
    OCIO_CHECK_NO_THROW(func.setParams(test_params));
    OCIO_CHECK_THROW_WHAT(func.validate(), OCIO::Exception, "Parameter 1 (lim_cyan) is outside valid range [1.001,65504]");
    test_params[0] = 65535.0;
    OCIO_CHECK_NO_THROW(func.setParams(test_params));
    OCIO_CHECK_THROW_WHAT(func.validate(), OCIO::Exception, "Parameter 65535 (lim_cyan) is outside valid range [1.001,65504]");
    test_params = params;
    test_params[1] = 1.0;
    OCIO_CHECK_NO_THROW(func.setParams(test_params));
    OCIO_CHECK_THROW_WHAT(func.validate(), OCIO::Exception, "Parameter 1 (lim_magenta) is outside valid range [1.001,65504]");
    test_params[1] = 65535.0;
    OCIO_CHECK_NO_THROW(func.setParams(test_params));
    OCIO_CHECK_THROW_WHAT(func.validate(), OCIO::Exception, "Parameter 65535 (lim_magenta) is outside valid range [1.001,65504]");
    test_params = params;
    test_params[2] = 1.0;
    OCIO_CHECK_NO_THROW(func.setParams(test_params));
    OCIO_CHECK_THROW_WHAT(func.validate(), OCIO::Exception, "Parameter 1 (lim_yellow) is outside valid range [1.001,65504]");
    test_params[2] = 65535.0;
    OCIO_CHECK_NO_THROW(func.setParams(test_params));
    OCIO_CHECK_THROW_WHAT(func.validate(), OCIO::Exception, "Parameter 65535 (lim_yellow) is outside valid range [1.001,65504]");

    test_params = params;
    test_params[3] = -0.1;
    OCIO_CHECK_NO_THROW(func.setParams(test_params));
    OCIO_CHECK_THROW_WHAT(func.validate(), OCIO::Exception, "Parameter -0.1 (thr_cyan) is outside valid range [0,0.9995]");
    test_params[3] = 1.0;
    OCIO_CHECK_NO_THROW(func.setParams(test_params));
    OCIO_CHECK_THROW_WHAT(func.validate(), OCIO::Exception, "Parameter 1 (thr_cyan) is outside valid range [0,0.9995]");
    test_params = params;
    test_params[4] = -0.1;
    OCIO_CHECK_NO_THROW(func.setParams(test_params));
    OCIO_CHECK_THROW_WHAT(func.validate(), OCIO::Exception, "Parameter -0.1 (thr_magenta) is outside valid range [0,0.9995]");
    test_params[4] = 1.0;
    OCIO_CHECK_NO_THROW(func.setParams(test_params));
    OCIO_CHECK_THROW_WHAT(func.validate(), OCIO::Exception, "Parameter 1 (thr_magenta) is outside valid range [0,0.9995]");
    test_params = params;
    test_params[5] = -0.1;
    OCIO_CHECK_NO_THROW(func.setParams(test_params));
    OCIO_CHECK_THROW_WHAT(func.validate(), OCIO::Exception, "Parameter -0.1 (thr_yellow) is outside valid range [0,0.9995]");
    test_params[5] = 1.0;
    OCIO_CHECK_NO_THROW(func.setParams(test_params));
    OCIO_CHECK_THROW_WHAT(func.validate(), OCIO::Exception, "Parameter 1 (thr_yellow) is outside valid range [0,0.9995]");

    test_params = params;
    test_params[6] = 0.0;
    OCIO_CHECK_NO_THROW(func.setParams(test_params));
    OCIO_CHECK_THROW_WHAT(func.validate(), OCIO::Exception, "Parameter 0 (power) is outside valid range [1,65504]");
    test_params = params;
    test_params[6] = 65535.0;
    OCIO_CHECK_NO_THROW(func.setParams(test_params));
    OCIO_CHECK_THROW_WHAT(func.validate(), OCIO::Exception, "Parameter 65535 (power) is outside valid range [1,65504]");
}

OCIO_ADD_TEST(FixedFunctionOpData, rec2100_surround_style)
{
    OCIO::FixedFunctionOpData::Params params = { 2.0 };
    OCIO::FixedFunctionOpData func(OCIO::FixedFunctionOpData::REC2100_SURROUND_FWD, params);
    OCIO_CHECK_NO_THROW(func.validate());
    std::string cacheID;
    OCIO_CHECK_NO_THROW(cacheID = func.getCacheID());
    OCIO_CHECK_ASSERT(func.getParams() == params);

    OCIO::FixedFunctionOpDataRcPtr inv = func.inverse();
    OCIO_CHECK_EQUAL(inv->getParams()[0], func.getParams()[0]);
    OCIO_CHECK_EQUAL(inv->getStyle(), OCIO::FixedFunctionOpData::REC2100_SURROUND_INV);
    std::string cacheIDUpdated;
    OCIO_CHECK_NO_THROW(cacheIDUpdated = inv->getCacheID());
    OCIO_CHECK_ASSERT(cacheID != cacheIDUpdated);

    OCIO_CHECK_ASSERT(func == func);
    OCIO_CHECK_ASSERT(!(func == *inv));

    params = func.getParams();
    params[0] = 120.;
    OCIO_CHECK_NO_THROW(func.setParams(params));
    OCIO_CHECK_THROW_WHAT(func.validate(), 
                          OCIO::Exception,
                          "Parameter 120 is greater than upper bound 100");

    params = func.getParams();
    params[0] = 0.00001;
    OCIO_CHECK_NO_THROW(func.setParams(params));
    OCIO_CHECK_THROW_WHAT(func.validate(),
                          OCIO::Exception,
                          "Parameter 1e-05 is less than lower bound 0.01");

    params = func.getParams();
    params.push_back(12);
    OCIO_CHECK_NO_THROW(func.setParams(params));
    OCIO_CHECK_THROW_WHAT(func.validate(),
                          OCIO::Exception,
                          "The style 'REC2100_Surround (Forward)' must have "
                          "one parameter but 2 found.");

    params = func.getParams();
    params.clear();
    OCIO_CHECK_NO_THROW(func.setParams(params));
    OCIO_CHECK_THROW_WHAT(func.validate(),
                          OCIO::Exception,
                          "The style 'REC2100_Surround (Forward)' must have "
                          "one parameter but 0 found.");
}

OCIO_ADD_TEST(FixedFunctionOpData, is_inverse)
{
    OCIO::FixedFunctionOpData::Params params = { 2.0 };
    auto f_s = std::make_shared<const OCIO::FixedFunctionOpData>(
        OCIO::FixedFunctionOpData::REC2100_SURROUND_FWD, params);
    OCIO::FixedFunctionOpData::Params params_inv = { 0.5 };
    auto f_s_inv1 = std::make_shared<const OCIO::FixedFunctionOpData>(
        OCIO::FixedFunctionOpData::REC2100_SURROUND_FWD, params_inv);
    auto f_s_inv2 = std::make_shared<const OCIO::FixedFunctionOpData>(
        OCIO::FixedFunctionOpData::REC2100_SURROUND_INV, params);

    OCIO_CHECK_ASSERT(f_s->isInverse(f_s_inv1));
    OCIO_CHECK_ASSERT(f_s->isInverse(f_s_inv2));

    OCIO_CHECK_ASSERT(!f_s->isInverse(f_s));
    OCIO_CHECK_ASSERT(!f_s_inv1->isInverse(f_s_inv1));
    OCIO_CHECK_ASSERT(!f_s_inv2->isInverse(f_s_inv2));
    OCIO_CHECK_ASSERT(!f_s_inv1->isInverse(f_s_inv2));

    OCIO::FixedFunctionOpData::Params p0 = {};
    auto f_g = std::make_shared<const OCIO::FixedFunctionOpData>(
        OCIO::FixedFunctionOpData::ACES_GLOW_03_FWD, p0);
    auto f_g_inv = std::make_shared<const OCIO::FixedFunctionOpData>(
        OCIO::FixedFunctionOpData::ACES_GLOW_03_INV, p0);
    OCIO_CHECK_ASSERT(f_g->isInverse(f_g_inv));
    OCIO_CHECK_ASSERT(f_g_inv->isInverse(f_g));
    OCIO_CHECK_ASSERT(!f_g->isInverse(f_g));
    OCIO_CHECK_ASSERT(!f_g_inv->isInverse(f_g_inv));
    OCIO_CHECK_ASSERT(!f_g->isInverse(f_s));

    auto f_r = std::make_shared<const OCIO::FixedFunctionOpData>(
        OCIO::FixedFunctionOpData::ACES_RED_MOD_03_FWD, p0);
    auto f_r_inv = std::make_shared<const OCIO::FixedFunctionOpData>(
        OCIO::FixedFunctionOpData::ACES_RED_MOD_03_INV, p0);
    OCIO_CHECK_ASSERT(f_r->isInverse(f_r_inv));
    OCIO_CHECK_ASSERT(f_r_inv->isInverse(f_r));
    OCIO_CHECK_ASSERT(!f_r->isInverse(f_r));
    OCIO_CHECK_ASSERT(!f_r_inv->isInverse(f_r_inv));
    OCIO_CHECK_ASSERT(!f_r->isInverse(f_g));

    OCIO::FixedFunctionOpData::Params p7 = { 1.147, 1.264, 1.312, 0.815, 0.803, 0.880, 1.2 };
    auto f_gm = std::make_shared<const OCIO::FixedFunctionOpData>(
        OCIO::FixedFunctionOpData::ACES_GAMUT_COMP_13_FWD, p7);
    auto f_gm_inv = std::make_shared<const OCIO::FixedFunctionOpData>(
        OCIO::FixedFunctionOpData::ACES_GAMUT_COMP_13_INV, p7);
    OCIO_CHECK_ASSERT(f_gm->isInverse(f_gm_inv));
    OCIO_CHECK_ASSERT(f_gm_inv->isInverse(f_gm));
    OCIO_CHECK_ASSERT(!f_gm->isInverse(f_gm));
    OCIO_CHECK_ASSERT(!f_gm_inv->isInverse(f_gm_inv));
    OCIO_CHECK_ASSERT(!f_gm->isInverse(f_r));

    p7[6] += 0.01;
    f_gm_inv = std::make_shared<const OCIO::FixedFunctionOpData>(
        OCIO::FixedFunctionOpData::ACES_GAMUT_COMP_13_INV, p7);
    OCIO_CHECK_ASSERT(!f_gm_inv->isInverse(f_gm));
    OCIO_CHECK_ASSERT(!f_gm->isInverse(f_gm_inv));
}

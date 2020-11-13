// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/exposurecontrast/ExposureContrastOpCPU.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


namespace
{
float videoECVal(float in, OCIO::ConstExposureContrastOpDataRcPtr & ec,
                 float inMax = 1., float outMax = 1.)
{
    const float exposure
        = powf(powf(2.f, (float)ec->getExposure()),
               (float)OCIO::EC::VIDEO_OETF_POWER);
    const float contrast
        = (float)std::max(OCIO::EC::MIN_CONTRAST, (ec->getContrast()*ec->getGamma()));
    const float pivot
        = powf((float)std::max(OCIO::EC::MIN_PIVOT, ec->getPivot()),
               (float)OCIO::EC::VIDEO_OETF_POWER);

    if (contrast == 1.f)
    {
        return in * exposure / (pivot*inMax) * pivot * outMax;
    }

    return powf((float)std::max(0.0f, in   * exposure / (pivot*inMax)), contrast) * pivot * outMax;
    //     powf(       std::max(0.0f, t[0] * exposure / m_pivot      ), contrast) * m_pivot,

}

float logECVal(float in, OCIO::ConstExposureContrastOpDataRcPtr & ec,
               float inMax = 1., float outMax = 1.)
{
    const float exposure = (float)(ec->getLogExposureStep() *
                                   ec->getExposure()) * inMax;
    const float contrast
        = (float)std::max(OCIO::EC::MIN_CONTRAST, (ec->getContrast()*ec->getGamma())) * outMax / inMax;

    const float pivot = (float)std::max(OCIO::EC::MIN_PIVOT, ec->getPivot());
    const float logPivot =
        (float)std::max(0., log2(pivot / 0.18) *
                            ec->getLogExposureStep() + ec->getLogMidGray());
    const float offset = (exposure - (logPivot*inMax)) * contrast + (logPivot*outMax);

    return (in * contrast) + offset;
    //     (t[0] * contrast ) + offset,

}

float linECVal(float in, OCIO::ConstExposureContrastOpDataRcPtr & ec,
               float inMax = 1., float outMax = 1.)
{
    const float exposure = powf(2., (float)ec->getExposure());
    const float contrast = (float)std::max(OCIO::EC::MIN_CONTRAST,
                                           (ec->getContrast()*ec->getGamma()));
    const float pivot = (float)std::max(OCIO::EC::MIN_PIVOT, ec->getPivot());

    if (contrast == 1.f)
    {
        return in * exposure / (pivot*inMax) * pivot * outMax;
    }

    return powf((float)std::max(0.0f, in * exposure / (pivot*inMax)),
                contrast) * pivot * outMax;
    //     powf( MAX(0.0f, t[0] * exposure / _iPivot), contrast) * _oPivot,

}

static constexpr float qnan = std::numeric_limits<float>::quiet_NaN();
static constexpr float inf = std::numeric_limits<float>::infinity();

}

OCIO_ADD_TEST(ExposureContrastRenderer, video)
{
    //
    // Video case, no scaling
    //
    const std::vector<float> rgbaImage { 0.0367126f, 0.5f, 1.f,     0.f,
                                         0.2f,       0.f,   .99f, 128.f,
                                         qnan,       qnan, qnan,    0.f,
                                         inf,        inf,  inf,     0.f};

    OCIO::ExposureContrastOpDataRcPtr ec =
        std::make_shared<OCIO::ExposureContrastOpData>(
            OCIO::ExposureContrastOpData::STYLE_VIDEO);

    ec->getExposureProperty()->makeDynamic();
    ec->getContrastProperty()->makeDynamic();
    ec->getGammaProperty()->makeDynamic();

    OCIO::ConstExposureContrastOpDataRcPtr const_ec = ec;
    OCIO::OpCPURcPtr renderer = OCIO::GetExposureContrastCPURenderer(const_ec);
    OCIO_CHECK_ASSERT(OCIO::DynamicPtrCast<OCIO::ECVideoRenderer>(renderer));
    std::vector<float> rgba = rgbaImage;

    renderer->apply(rgba.data(), rgba.data(), 4);

    OCIO_CHECK_EQUAL(rgba[0], videoECVal(rgbaImage[0], const_ec));
    OCIO_CHECK_EQUAL(rgba[1], videoECVal(rgbaImage[1], const_ec));
    OCIO_CHECK_EQUAL(rgba[2], videoECVal(rgbaImage[2], const_ec));
    OCIO_CHECK_EQUAL(rgba[3], rgbaImage[3]);
    OCIO_CHECK_EQUAL(rgba[4], videoECVal(rgbaImage[4], const_ec));
    OCIO_CHECK_EQUAL(rgba[5], videoECVal(rgbaImage[5], const_ec));
    OCIO_CHECK_EQUAL(rgba[6], videoECVal(rgbaImage[6], const_ec));
    OCIO_CHECK_EQUAL(rgba[7], rgbaImage[7]);

    OCIO_CHECK_ASSERT(std::isnan(rgba[8]));
    OCIO_CHECK_ASSERT(std::isnan(rgba[9]));
    OCIO_CHECK_ASSERT(std::isnan(rgba[10]));
    OCIO_CHECK_EQUAL(rgba[12], videoECVal(rgbaImage[12], const_ec));
    OCIO_CHECK_EQUAL(rgba[13], videoECVal(rgbaImage[13], const_ec));
    OCIO_CHECK_EQUAL(rgba[14], videoECVal(rgbaImage[14], const_ec));

    // Re-test with different E/C values.
    //

    // The renderer has a copy of ec, get the dynamic property ptr in order to change the value
    // for the apply.
    OCIO::DynamicPropertyDoubleRcPtr dpc, dpe, dpg;
    OCIO::DynamicPropertyRcPtr dp = renderer->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST);
    OCIO_CHECK_NO_THROW(dpc = OCIO::DynamicPropertyValue::AsDouble(dp));
    dp = renderer->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE);
    OCIO_CHECK_NO_THROW(dpe = OCIO::DynamicPropertyValue::AsDouble(dp));
    dp = renderer->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA);
    OCIO_CHECK_NO_THROW(dpg = OCIO::DynamicPropertyValue::AsDouble(dp));
    dpe->setValue(0.2);
    dpc->setValue(1.0);
    // TODO: When base < 1, ssePower(Inf, base) != Inf.  It returns various
    // large numbers, e.g., 3.6e11 for base=0.3 ranging up to 4.0e36 for base=0.95.
    // dpc->setContrast(0.5);
    dpg->setValue(1.2);

    rgba = rgbaImage;
    renderer->apply(rgba.data(), rgba.data(), 4);

    // The dynamic property ptr only updates the copy in the renderer.  Need to also update the
    // original opData in this case since it is used to check the result of the apply.
    ec->setExposure(0.2);
    ec->setContrast(1.0);
    ec->setGamma(1.2);

    // As the ssePower is an approximation, strict equality is not possible.
    const float error = 1e-5f;
    OCIO_CHECK_CLOSE(rgba[0], videoECVal(rgbaImage[0], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[1], videoECVal(rgbaImage[1], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[2], videoECVal(rgbaImage[2], const_ec), error);
    OCIO_CHECK_EQUAL(rgba[3], rgbaImage[3]);
    OCIO_CHECK_CLOSE(rgba[4], videoECVal(rgbaImage[4], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[5], videoECVal(rgbaImage[5], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[6], videoECVal(rgbaImage[6], const_ec), error);
    OCIO_CHECK_EQUAL(rgba[7], rgbaImage[7]);

    OCIO_CHECK_CLOSE(rgba[8], videoECVal(rgbaImage[8], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[9], videoECVal(rgbaImage[9], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[10], videoECVal(rgbaImage[10], const_ec), error);
    OCIO_CHECK_EQUAL(rgba[12], videoECVal(rgbaImage[12], const_ec));
    OCIO_CHECK_EQUAL(rgba[13], videoECVal(rgbaImage[13], const_ec));
    OCIO_CHECK_EQUAL(rgba[14], videoECVal(rgbaImage[14], const_ec));
}

OCIO_ADD_TEST(ExposureContrastRenderer, log)
{
    //
    // Log case, no scaling
    //
    const std::vector<float> rgbaImage { 0.0367126f, 0.5f, 1.f,    0.f,
                                         0.2f,       0.f,  .99f, 128.f,
                                         qnan,       qnan, qnan,   0.f,
                                          inf,       inf,  inf,    0.f};
    OCIO::ExposureContrastOpDataRcPtr ec =
        std::make_shared<OCIO::ExposureContrastOpData>(
            OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC);

    ec->getExposureProperty()->makeDynamic();
    ec->getContrastProperty()->makeDynamic();
    ec->getGammaProperty()->makeDynamic();
    ec->setExposure(1.2);
    ec->setPivot(0.18);

    OCIO::ConstExposureContrastOpDataRcPtr const_ec = ec;
    OCIO::OpCPURcPtr renderer = OCIO::GetExposureContrastCPURenderer(const_ec);
    OCIO_CHECK_ASSERT(OCIO::DynamicPtrCast<OCIO::ECLogarithmicRenderer>(renderer));

    std::vector<float> rgba = rgbaImage;

    renderer->apply(rgba.data(), rgba.data(), 4);

    const float inMax = (float)OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_F32);
    const float outMax = (float)OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_F32);

    OCIO_CHECK_EQUAL(rgba[0], logECVal(rgbaImage[0], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[1], logECVal(rgbaImage[1], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[2], logECVal(rgbaImage[2], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[3], rgbaImage[3] * outMax / inMax);
    OCIO_CHECK_EQUAL(rgba[4], logECVal(rgbaImage[4], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[5], logECVal(rgbaImage[5], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[6], logECVal(rgbaImage[6], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[7], rgbaImage[7] * outMax / inMax);

    OCIO_CHECK_ASSERT(std::isnan(rgba[8]));
    OCIO_CHECK_ASSERT(std::isnan(rgba[9]));
    OCIO_CHECK_ASSERT(std::isnan(rgba[10]));
    OCIO_CHECK_EQUAL(rgba[12], logECVal(rgbaImage[12], const_ec));
    OCIO_CHECK_EQUAL(rgba[13], logECVal(rgbaImage[13], const_ec));
    OCIO_CHECK_EQUAL(rgba[14], logECVal(rgbaImage[14], const_ec));

    // Re-test with different E/C values.
    //

    OCIO::DynamicPropertyDoubleRcPtr dpc, dpe, dpg;
    OCIO::DynamicPropertyRcPtr dp = renderer->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST);
    OCIO_CHECK_NO_THROW(dpc = OCIO::DynamicPropertyValue::AsDouble(dp));
    dp = renderer->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE);
    OCIO_CHECK_NO_THROW(dpe = OCIO::DynamicPropertyValue::AsDouble(dp));
    dp = renderer->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA);
    OCIO_CHECK_NO_THROW(dpg = OCIO::DynamicPropertyValue::AsDouble(dp));
    dpe->setValue(0.2);
    dpc->setValue(0.5);
    dpg->setValue(1.6);

    rgba = rgbaImage;
    renderer->apply(rgba.data(), rgba.data(), 4);

    ec->setExposure(0.2);
    ec->setContrast(0.5);
    ec->setGamma(1.6);

    OCIO_CHECK_EQUAL(rgba[0], logECVal(rgbaImage[0], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[1], logECVal(rgbaImage[1], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[2], logECVal(rgbaImage[2], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[3], rgbaImage[3] * outMax / inMax);
    OCIO_CHECK_EQUAL(rgba[4], logECVal(rgbaImage[4], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[5], logECVal(rgbaImage[5], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[6], logECVal(rgbaImage[6], const_ec, inMax, outMax));
    OCIO_CHECK_EQUAL(rgba[7], rgbaImage[7] * outMax / inMax);

    OCIO_CHECK_ASSERT(std::isnan(rgba[8]));
    OCIO_CHECK_ASSERT(std::isnan(rgba[9]));
    OCIO_CHECK_ASSERT(std::isnan(rgba[10]));
    OCIO_CHECK_EQUAL(rgba[12], logECVal(rgbaImage[12], const_ec));
    OCIO_CHECK_EQUAL(rgba[13], logECVal(rgbaImage[13], const_ec));
    OCIO_CHECK_EQUAL(rgba[14], logECVal(rgbaImage[14], const_ec));
}

OCIO_ADD_TEST(ExposureContrastRenderer, linear)
{
    //
    // Linear case, no scaling
    //
    const std::vector<float> rgbaImage { 0.0f, 0.5f, 1.f,     0.f,
                                         0.2f, 0.8f,  .99f, 128.f,
                                         qnan, qnan, qnan,    0.f,
                                         inf,  inf,  inf,     0.f};
    OCIO::ExposureContrastOpDataRcPtr ec =
        std::make_shared<OCIO::ExposureContrastOpData>(
            OCIO::ExposureContrastOpData::STYLE_LINEAR);

    ec->getExposureProperty()->makeDynamic();
    ec->getContrastProperty()->makeDynamic();
    ec->getGammaProperty()->makeDynamic();

    OCIO::ConstExposureContrastOpDataRcPtr const_ec = ec;
    OCIO::OpCPURcPtr renderer = OCIO::GetExposureContrastCPURenderer(const_ec);
    OCIO_CHECK_ASSERT(OCIO::DynamicPtrCast<OCIO::ECLinearRenderer>(renderer));

    std::vector<float> rgba = rgbaImage;

    renderer->apply(rgba.data(), rgba.data(), 4);

    OCIO_CHECK_EQUAL(rgba[0], linECVal(rgbaImage[0], const_ec));
    OCIO_CHECK_EQUAL(rgba[1], linECVal(rgbaImage[1], const_ec));
    OCIO_CHECK_EQUAL(rgba[2], linECVal(rgbaImage[2], const_ec));
    OCIO_CHECK_EQUAL(rgba[3], rgbaImage[3]);
    OCIO_CHECK_EQUAL(rgba[4], linECVal(rgbaImage[4], const_ec));
    OCIO_CHECK_EQUAL(rgba[5], linECVal(rgbaImage[5], const_ec));
    OCIO_CHECK_EQUAL(rgba[6], linECVal(rgbaImage[6], const_ec));
    OCIO_CHECK_EQUAL(rgba[7], rgbaImage[7]);

    OCIO_CHECK_ASSERT(std::isnan(rgba[8]));
    OCIO_CHECK_ASSERT(std::isnan(rgba[9]));
    OCIO_CHECK_ASSERT(std::isnan(rgba[10]));
    OCIO_CHECK_EQUAL(rgba[12], linECVal(rgbaImage[12], const_ec));
    OCIO_CHECK_EQUAL(rgba[13], linECVal(rgbaImage[13], const_ec));
    OCIO_CHECK_EQUAL(rgba[14], linECVal(rgbaImage[14], const_ec));

    // Re-test with different E/C values.
    //

    OCIO::DynamicPropertyDoubleRcPtr dpc, dpe, dpg;
    OCIO::DynamicPropertyRcPtr dp = renderer->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST);
    OCIO_CHECK_NO_THROW(dpc = OCIO::DynamicPropertyValue::AsDouble(dp));
    dp = renderer->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE);
    OCIO_CHECK_NO_THROW(dpe = OCIO::DynamicPropertyValue::AsDouble(dp));
    dp = renderer->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA);
    OCIO_CHECK_NO_THROW(dpg = OCIO::DynamicPropertyValue::AsDouble(dp));
    dpe->setValue(0.2);
    dpc->setValue(1.5);
    dpg->setValue(1.2);

    rgba = rgbaImage;
    renderer->apply(rgba.data(), rgba.data(), 4);

    ec->setExposure(0.2);
    ec->setContrast(1.5);
    ec->setGamma(1.2);

    // As the ssePower is an approximation, strict equality is not possible.
    const float error = 5e-5f;

    OCIO_CHECK_CLOSE(rgba[0], linECVal(rgbaImage[0], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[1], linECVal(rgbaImage[1], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[2], linECVal(rgbaImage[2], const_ec), error);
    OCIO_CHECK_EQUAL(rgba[3], rgbaImage[3]);
    OCIO_CHECK_CLOSE(rgba[4], linECVal(rgbaImage[4], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[5], linECVal(rgbaImage[5], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[6], linECVal(rgbaImage[6], const_ec), error);
    OCIO_CHECK_EQUAL(rgba[7], rgbaImage[7]);

    OCIO_CHECK_CLOSE(rgba[8], linECVal(rgbaImage[8], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[9], linECVal(rgbaImage[9], const_ec), error);
    OCIO_CHECK_CLOSE(rgba[10], linECVal(rgbaImage[10], const_ec), error);
    OCIO_CHECK_EQUAL(rgba[12], linECVal(rgbaImage[12], const_ec));
    OCIO_CHECK_EQUAL(rgba[13], linECVal(rgbaImage[13], const_ec));
    OCIO_CHECK_EQUAL(rgba[14], linECVal(rgbaImage[14], const_ec));
}

namespace
{
void TestECInverse(OCIO::ExposureContrastOpData::Style style)
{
    const std::vector<float> rgbaImage { 0.0f, 0.5f, 1.f,     0.f,
                                         0.2f, 0.8f,  .99f, 128.f };

    OCIO::ExposureContrastOpDataRcPtr ec =
        std::make_shared<OCIO::ExposureContrastOpData>(style);

    ec->setExposure(1.5);
    ec->setContrast(0.5);
    ec->setGamma(1.1);
    ec->setPivot(0.18);

    OCIO::ConstExposureContrastOpDataRcPtr const_ec = ec;
    OCIO::OpCPURcPtr renderer = OCIO::GetExposureContrastCPURenderer(const_ec);

    std::vector<float> rgba = rgbaImage;
    renderer->apply(rgba.data(), rgba.data(), 2);

    OCIO::ConstExposureContrastOpDataRcPtr const_eci = ec->inverse();
    OCIO::OpCPURcPtr rendereri = OCIO::GetExposureContrastCPURenderer(const_eci);
    rendereri->apply(rgba.data(), rgba.data(), 2);

    // As the ssePower is an approximation, strict equality is not possible.
    const float error = 1e-5f;

    OCIO_CHECK_CLOSE(rgba[0], rgbaImage[0], error);
    OCIO_CHECK_CLOSE(rgba[1], rgbaImage[1], error);
    OCIO_CHECK_CLOSE(rgba[2], rgbaImage[2], error);
    OCIO_CHECK_EQUAL(rgba[3], rgbaImage[3]);
    OCIO_CHECK_CLOSE(rgba[4], rgbaImage[4], error);
    OCIO_CHECK_CLOSE(rgba[5], rgbaImage[5], error);
    OCIO_CHECK_CLOSE(rgba[6], rgbaImage[6], error);
    OCIO_CHECK_EQUAL(rgba[7], rgbaImage[7]);
}
}

OCIO_ADD_TEST(ExposureContrastRenderer, inverse)
{
    TestECInverse(OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC);
    TestECInverse(OCIO::ExposureContrastOpData::STYLE_LINEAR);
    TestECInverse(OCIO::ExposureContrastOpData::STYLE_VIDEO);
}

void TestLogParamForStyle(OCIO::ExposureContrastOpData::Style style, bool hasEffect)
{
    const std::vector<float> rgbaImage { 0.1f, 0.2f, 0.3f, 0.f,
                                                     0.4f, 0.5f, 0.6f, 0.f,
                                                     0.7f, 0.8f, 0.9f, 0.f };

    OCIO::ExposureContrastOpDataRcPtr ec =
        std::make_shared<OCIO::ExposureContrastOpData>(style);

    ec->setExposure(0.2);
    ec->setContrast(1.0);
    ec->setGamma(1.2);

    std::vector<float> rgbaRef = rgbaImage;

    // Reference.
    {
        OCIO::ConstExposureContrastOpDataRcPtr const_ec = ec;
        OCIO::OpCPURcPtr renderer = OCIO::GetExposureContrastCPURenderer(const_ec);
        renderer->apply(rgbaRef.data(), rgbaRef.data(), 3);
    }

    // Change log parameters.
    ec->setLogExposureStep(0.1);
    ec->setLogMidGray(0.4);
    std::vector<float> rgba = rgbaImage;
    {
        OCIO::ConstExposureContrastOpDataRcPtr const_ec = ec;
        OCIO::OpCPURcPtr renderer = OCIO::GetExposureContrastCPURenderer(const_ec);
        renderer->apply(rgba.data(), rgba.data(), 3);
        for (int i = 0; i < 12; ++i)
        {
            if (!hasEffect || i%4 == 3) // EC does not affect alpha
            {
                OCIO_CHECK_EQUAL(rgba[i], rgbaRef[i]);
            }
            else
            {
                OCIO_CHECK_NE(rgba[i], rgbaRef[i]);
            }
        }
    }
}

OCIO_ADD_TEST(ExposureContrastRenderer, log_params)
{
    TestLogParamForStyle(OCIO::ExposureContrastOpData::STYLE_VIDEO, false);
    TestLogParamForStyle(OCIO::ExposureContrastOpData::STYLE_VIDEO_REV, false);
    TestLogParamForStyle(OCIO::ExposureContrastOpData::STYLE_LINEAR, false);
    TestLogParamForStyle(OCIO::ExposureContrastOpData::STYLE_LINEAR_REV, false);
    TestLogParamForStyle(OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC, true);
    TestLogParamForStyle(OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC_REV, true);
}


// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/exposurecontrast/ExposureContrastOpData.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(ExposureContrastOpData, style)
{
    OCIO::ExposureContrastOpData::Style style = 
        OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC_REV;
    const auto & ConvertToStyle =
        OCIO::ExposureContrastOpData::ConvertStringToStyle;
    OCIO_CHECK_NO_THROW(style =
        ConvertToStyle(OCIO::EC_STYLE_LINEAR));
    OCIO_CHECK_EQUAL(style, OCIO::ExposureContrastOpData::STYLE_LINEAR);
    OCIO_CHECK_NO_THROW(style =
        ConvertToStyle(OCIO::EC_STYLE_LINEAR_REV));
    OCIO_CHECK_EQUAL(style, OCIO::ExposureContrastOpData::STYLE_LINEAR_REV);
    OCIO_CHECK_NO_THROW(style =
        ConvertToStyle(OCIO::EC_STYLE_VIDEO));
    OCIO_CHECK_EQUAL(style, OCIO::ExposureContrastOpData::STYLE_VIDEO);
    OCIO_CHECK_NO_THROW(style =
        ConvertToStyle(OCIO::EC_STYLE_VIDEO_REV));
    OCIO_CHECK_EQUAL(style, OCIO::ExposureContrastOpData::STYLE_VIDEO_REV);
    OCIO_CHECK_NO_THROW(style =
        ConvertToStyle(OCIO::EC_STYLE_LOGARITHMIC));
    OCIO_CHECK_EQUAL(style, OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC);
    OCIO_CHECK_NO_THROW(style =
        ConvertToStyle(OCIO::EC_STYLE_LOGARITHMIC_REV));
    OCIO_CHECK_EQUAL(style, OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC_REV);

    OCIO_CHECK_THROW_WHAT(
        ConvertToStyle("Unknown exposure contrast style"),
        OCIO::Exception, "Unknown exposure contrast style");

    OCIO_CHECK_THROW_WHAT(
        ConvertToStyle(nullptr),
        OCIO::Exception, "Missing exposure contrast style");

    const auto & ConvertToString =
        OCIO::ExposureContrastOpData::ConvertStyleToString;
    std::string styleName;
    OCIO_CHECK_NO_THROW(styleName =
        ConvertToString(OCIO::ExposureContrastOpData::STYLE_LINEAR));
    OCIO_CHECK_EQUAL(styleName, OCIO::EC_STYLE_LINEAR);
    OCIO_CHECK_NO_THROW(styleName =
        ConvertToString(OCIO::ExposureContrastOpData::STYLE_LINEAR_REV));
    OCIO_CHECK_EQUAL(styleName, OCIO::EC_STYLE_LINEAR_REV);
    OCIO_CHECK_NO_THROW(styleName =
        ConvertToString(OCIO::ExposureContrastOpData::STYLE_VIDEO));
    OCIO_CHECK_EQUAL(styleName, OCIO::EC_STYLE_VIDEO);
    OCIO_CHECK_NO_THROW(styleName =
        ConvertToString(OCIO::ExposureContrastOpData::STYLE_VIDEO_REV));
    OCIO_CHECK_EQUAL(styleName, OCIO::EC_STYLE_VIDEO_REV);
    OCIO_CHECK_NO_THROW(styleName =
        ConvertToString(OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC));
    OCIO_CHECK_EQUAL(styleName, OCIO::EC_STYLE_LOGARITHMIC);
    OCIO_CHECK_NO_THROW(styleName =
        ConvertToString(OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC_REV));
    OCIO_CHECK_EQUAL(styleName, OCIO::EC_STYLE_LOGARITHMIC_REV);
}

OCIO_ADD_TEST(ExposureContrastOpData, accessors)
{
    OCIO::ExposureContrastOpData ec0;
    OCIO_CHECK_EQUAL(ec0.getType(), OCIO::OpData::ExposureContrastType);
    OCIO_CHECK_EQUAL(ec0.getStyle(), OCIO::ExposureContrastOpData::STYLE_LINEAR);

    OCIO_CHECK_EQUAL(ec0.getExposure(), 0.0);
    OCIO_CHECK_EQUAL(ec0.getContrast(), 1.0);
    OCIO_CHECK_EQUAL(ec0.getGamma(), 1.0);
    OCIO_CHECK_EQUAL(ec0.getPivot(), 0.18);
    OCIO_CHECK_EQUAL(ec0.getLogExposureStep(),
                     OCIO::ExposureContrastOpData::LOGEXPOSURESTEP_DEFAULT);
    OCIO_CHECK_EQUAL(ec0.getLogMidGray(),
                     OCIO::ExposureContrastOpData::LOGMIDGRAY_DEFAULT);

    OCIO_CHECK_ASSERT(ec0.isIdentity());
    OCIO_CHECK_ASSERT(ec0.isNoOp());
    OCIO_CHECK_ASSERT(!ec0.hasChannelCrosstalk());
    OCIO_CHECK_NO_THROW(ec0.validate());

    std::string cacheID;
    OCIO_CHECK_NO_THROW(cacheID = ec0.getCacheID());
    const std::string expected("linear E: 0 C: 1 G: 1 P: 0.18 LES: 0.088 LMG: 0.435");
    OCIO_CHECK_ASSERT(OCIO::Platform::Strcasecmp(cacheID.c_str(),
                                                 expected.c_str()) == 0);

    ec0.setExposure(0.1);
    OCIO_CHECK_ASSERT(!ec0.isIdentity());
    OCIO_CHECK_ASSERT(!ec0.isNoOp());
    OCIO_CHECK_ASSERT(!ec0.hasChannelCrosstalk());
    std::string cacheIDUpdated;
    OCIO_CHECK_NO_THROW(cacheIDUpdated = ec0.getCacheID());
    OCIO_CHECK_ASSERT(cacheID != cacheIDUpdated);

    OCIO::ExposureContrastOpData ec(OCIO::ExposureContrastOpData::STYLE_VIDEO);
    OCIO_CHECK_EQUAL(ec.getType(), OCIO::OpData::ExposureContrastType);
    OCIO_CHECK_EQUAL(ec.getStyle(), OCIO::ExposureContrastOpData::STYLE_VIDEO);

    OCIO_CHECK_EQUAL(ec.getExposure(), 0.0);
    OCIO_CHECK_EQUAL(ec.getContrast(), 1.0);
    OCIO_CHECK_EQUAL(ec.getGamma(), 1.0);
    OCIO_CHECK_EQUAL(ec.getPivot(), 0.18);
    OCIO_CHECK_EQUAL(ec.getLogExposureStep(),
                     OCIO::ExposureContrastOpData::LOGEXPOSURESTEP_DEFAULT);
    OCIO_CHECK_EQUAL(ec.getLogMidGray(),
                     OCIO::ExposureContrastOpData::LOGMIDGRAY_DEFAULT);

    OCIO_CHECK_ASSERT(ec.isNoOp());

    OCIO_CHECK_ASSERT(!ec.getExposureProperty()->isDynamic());
    OCIO_CHECK_ASSERT(!ec.getContrastProperty()->isDynamic());
    OCIO_CHECK_ASSERT(!ec.getGammaProperty()->isDynamic());
    OCIO_CHECK_ASSERT(!ec.isDynamic());

    // Never treated as NoOp when dynamic.
    ec.getExposureProperty()->makeDynamic();
    OCIO_CHECK_ASSERT(!ec.isNoOp());
    OCIO_CHECK_ASSERT(ec.isDynamic());
    OCIO_CHECK_ASSERT(ec.hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE));

    ec.setExposure(0.1);
    ec.setContrast(0.8);
    ec.setGamma(1.1);
    ec.setPivot(0.2);
    ec.setLogExposureStep(0.07);
    ec.setLogMidGray(0.5);

    OCIO_CHECK_EQUAL(ec.getExposure(), 0.1);
    OCIO_CHECK_EQUAL(ec.getContrast(), 0.8);
    OCIO_CHECK_EQUAL(ec.getGamma(), 1.1);
    OCIO_CHECK_EQUAL(ec.getPivot(), 0.2);
    OCIO_CHECK_EQUAL(ec.getLogExposureStep(), 0.07);
    OCIO_CHECK_EQUAL(ec.getLogMidGray(), 0.5);

    OCIO::DynamicPropertyDoubleImplRcPtr dpExp;
    OCIO::DynamicPropertyDoubleImplRcPtr dpContrast;
    OCIO::DynamicPropertyDoubleImplRcPtr dpGamma;

    // Property must be set as dynamic to accept a dynamic value.
    OCIO_CHECK_ASSERT(!ec.hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST));
    OCIO_CHECK_ASSERT(!ec.hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA));
    OCIO_CHECK_THROW_WHAT(ec.getDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST),
                          OCIO::Exception,
                          "not dynamic")

    OCIO_CHECK_NO_THROW(dpExp = ec.getExposureProperty());
    OCIO_CHECK_EQUAL(dpExp->getValue(), 0.1);
    OCIO_CHECK_ASSERT(!ec.hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST));
    OCIO_CHECK_ASSERT(!ec.hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA));
    dpExp->setValue(1.5);
    OCIO_CHECK_EQUAL(ec.getExposure(), 1.5);
    dpExp->setValue(0.7);
    OCIO_CHECK_EQUAL(ec.getExposure(), 0.7);

    ec.getContrastProperty()->makeDynamic();
    ec.getGammaProperty()->makeDynamic();
    OCIO_CHECK_NO_THROW(dpContrast = ec.getContrastProperty());
    OCIO_CHECK_NO_THROW(dpGamma = ec.getGammaProperty());
    dpContrast->setValue(1.42);
    dpGamma->setValue(0.88);
    OCIO_CHECK_EQUAL(ec.getContrast(), 1.42);
    OCIO_CHECK_EQUAL(ec.getGamma(), 0.88);
}

OCIO_ADD_TEST(ExposureContrastOpData, clone)
{
    OCIO::ExposureContrastOpData ec;
    ec.setExposure(-1.4);
    ec.setContrast(0.8);
    ec.setGamma(1.1);
    ec.setPivot(0.2);

    ec.getExposureProperty()->makeDynamic();
    OCIO::DynamicPropertyDoubleImplRcPtr dpExp;

    OCIO_CHECK_NO_THROW(dpExp = ec.getExposureProperty());
    OCIO_CHECK_EQUAL(dpExp->getValue(), -1.4);
    dpExp->setValue(1.5);
    OCIO::ExposureContrastOpDataRcPtr ecCloned = ec.clone();
    OCIO_REQUIRE_ASSERT(ecCloned);

    OCIO_CHECK_EQUAL(ec.getExposure(), ecCloned->getExposure());
    OCIO_CHECK_EQUAL(ec.getContrast(), ecCloned->getContrast());
    OCIO_CHECK_EQUAL(ec.getGamma(), ecCloned->getGamma());
    OCIO_CHECK_EQUAL(ec.getPivot(), ecCloned->getPivot());
    OCIO_CHECK_EQUAL(ec.getLogExposureStep(), ecCloned->getLogExposureStep());
    OCIO_CHECK_EQUAL(ec.getLogMidGray(), ecCloned->getLogMidGray());

    OCIO_CHECK_EQUAL(ec.getExposureProperty()->isDynamic(),
                     ecCloned->getExposureProperty()->isDynamic());
    OCIO_CHECK_EQUAL(ec.getContrastProperty()->isDynamic(),
                     ecCloned->getContrastProperty()->isDynamic());
    OCIO_CHECK_EQUAL(ec.getGammaProperty()->isDynamic(),
                     ecCloned->getGammaProperty()->isDynamic());

    // Clone makes a copy of the dynamic property rather than sharing the original.
    dpExp->setValue(0.21);
    OCIO_CHECK_NE(ec.getExposure(), ecCloned->getExposure());
    OCIO_CHECK_EQUAL(ec.getExposure(), 0.21);
    OCIO_CHECK_EQUAL(ecCloned->getExposure(), 1.5);
}

OCIO_ADD_TEST(ExposureContrastOpData, inverse)
{
    OCIO::ExposureContrastOpData ec(OCIO::ExposureContrastOpData::STYLE_VIDEO);

    ec.setContrast(0.8);
    ec.setGamma(1.1);
    ec.setPivot(0.2);

    ec.getExposureProperty()->makeDynamic();
    OCIO::DynamicPropertyDoubleImplRcPtr dpExp;

    OCIO_CHECK_NO_THROW(dpExp = ec.getExposureProperty());
    dpExp->setValue(1.5);
    OCIO::ExposureContrastOpDataRcPtr ecInv = ec.inverse();
    OCIO_REQUIRE_ASSERT(ecInv);

    OCIO::ConstExposureContrastOpDataRcPtr ecInvConst = ecInv;
    // Dynamic are not inverse.
    OCIO_CHECK_ASSERT(!ec.isInverse(ecInvConst));

    OCIO_CHECK_EQUAL(ecInv->getStyle(),
                     OCIO::ExposureContrastOpData::STYLE_VIDEO_REV);

    OCIO_CHECK_EQUAL(ec.getExposure(), ecInv->getExposure());
    OCIO_CHECK_EQUAL(ec.getContrast(), ecInv->getContrast());
    OCIO_CHECK_EQUAL(ec.getGamma(), ecInv->getGamma());
    OCIO_CHECK_EQUAL(ec.getPivot(), ecInv->getPivot());
    OCIO_CHECK_EQUAL(ec.getLogExposureStep(), ecInv->getLogExposureStep());
    OCIO_CHECK_EQUAL(ec.getLogMidGray(), ecInv->getLogMidGray());

    OCIO_CHECK_EQUAL(ec.getExposureProperty()->isDynamic(),
                     ecInv->getExposureProperty()->isDynamic());
    OCIO_CHECK_EQUAL(ec.getContrastProperty()->isDynamic(),
                     ecInv->getContrastProperty()->isDynamic());
    OCIO_CHECK_EQUAL(ec.getGammaProperty()->isDynamic(),
                     ecInv->getGammaProperty()->isDynamic());

    // Inverse makes a copy of the dynamic property rather than sharing the original.
    dpExp->setValue(0.21);
    OCIO_CHECK_NE(ec.getExposure(), ecInv->getExposure());
    OCIO_CHECK_EQUAL(ec.getExposure(), 0.21);
    OCIO_CHECK_EQUAL(ecInv->getExposure(), 1.5);

    // Exposure is dynamic in both, never equal.
    OCIO_CHECK_ASSERT(!ec.isInverse(ecInvConst));

    ecInv->getContrastProperty()->makeDynamic();

    // Contrast is dynamic in one and not in the other.
    OCIO_CHECK_ASSERT(!ec.isInverse(ecInvConst));

    ec.getContrastProperty()->makeDynamic();
    OCIO_CHECK_ASSERT(!ec.isInverse(ecInvConst));

    // Gamma values are now different.
    ec.setGamma(1.2);
    OCIO_CHECK_ASSERT(!ec.isInverse(ecInvConst));
}

OCIO_ADD_TEST(ExposureContrastOpData, equality)
{
    OCIO::ExposureContrastOpData ec0;
    OCIO::ExposureContrastOpData ec1;
    OCIO_CHECK_ASSERT(ec0 == ec1);

    // Change style.
    ec0.setStyle(OCIO::ExposureContrastOpData::STYLE_VIDEO);
    OCIO_CHECK_ASSERT(!(ec0 == ec1));
    ec1.setStyle(OCIO::ExposureContrastOpData::STYLE_VIDEO);
    OCIO_CHECK_ASSERT(ec0 == ec1);

    // Change dynamic.
    ec0.getExposureProperty()->makeDynamic();
    OCIO_CHECK_ASSERT(!(ec0 == ec1));
    ec1.getExposureProperty()->makeDynamic();
    OCIO_CHECK_ASSERT(!(ec0 == ec1));

    // Change value of enabled dynamic property.
    ec0.setExposure(0.5);
    OCIO_CHECK_ASSERT(!(ec0 == ec1));
    ec1.setExposure(0.5);
    OCIO_CHECK_ASSERT(!(ec0 == ec1));

    // Change value of dynamic property not enabled.
    ec1.setContrast(0.5);
    OCIO_CHECK_ASSERT(!(ec0 == ec1));

    ec0.setContrast(0.5);
    OCIO_CHECK_ASSERT(!(ec0 == ec1));
}

OCIO_ADD_TEST(ExposureContrastOpData, replace_dynamic_property)
{
    OCIO::ExposureContrastOpData ec0;
    OCIO::ExposureContrastOpData ec1;

    ec0.setExposure(0.0);
    ec1.setExposure(1.0);

    ec0.getExposureProperty()->makeDynamic();
    ec1.getExposureProperty()->makeDynamic();

    auto dpe0 = ec0.getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE);
    auto dpe1 = ec1.getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE);

    // These are 2 different pointers.
    OCIO_CHECK_NE(dpe0.get(), dpe1.get());

    auto dpd0 = ec0.getExposureProperty();
    ec1.replaceDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE, dpd0);
    dpe1 = ec1.getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE);

    // Now, this is the same pointer.
    OCIO_CHECK_EQUAL(dpe0.get(), dpe1.get());

    ec0.getContrastProperty()->makeDynamic();
    // Contrast is not enabled in ec1.
    OCIO_CHECK_THROW(ec1.getDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST), OCIO::Exception);

    // The property is not replaced if dynamic is not enabled.
    dpd0 = ec0.getContrastProperty();
    OCIO_CHECK_THROW(ec1.replaceDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST, dpd0),
                     OCIO::Exception);
    OCIO_CHECK_THROW(ec1.getDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST), OCIO::Exception);
}

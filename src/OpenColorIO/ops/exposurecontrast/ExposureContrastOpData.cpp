/*
Copyright (c) 2019 Autodesk Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/exposurecontrast/ExposureContrastOpData.h"
#include "Platform.h"

OCIO_NAMESPACE_ENTER
{

namespace
{
static constexpr const char * EC_STYLE_LINEAR          = "linear";
static constexpr const char * EC_STYLE_LINEAR_REV      = "linearRev";
static constexpr const char * EC_STYLE_VIDEO           = "video";
static constexpr const char * EC_STYLE_VIDEO_REV       = "videoRev";
static constexpr const char * EC_STYLE_LOGARITHMIC     = "log";
static constexpr const char * EC_STYLE_LOGARITHMIC_REV = "logRev";
}

namespace DefaultValues
{
const int FLOAT_DECIMALS = 7;
}

ExposureContrastOpData::Style ExposureContrastOpData::ConvertStringToStyle(const char * str)
{
    if (str && *str)
    {
        if (0 == Platform::Strcasecmp(str, EC_STYLE_LINEAR))
        {
            return STYLE_LINEAR;
        }
        else if (0 == Platform::Strcasecmp(str, EC_STYLE_LINEAR_REV))
        {
            return STYLE_LINEAR_REV;
        }
        else if (0 == Platform::Strcasecmp(str, EC_STYLE_VIDEO))
        {
            return STYLE_VIDEO;
        }
        else if (0 == Platform::Strcasecmp(str, EC_STYLE_VIDEO_REV))
        {
            return STYLE_VIDEO_REV;
        }
        else if (0 == Platform::Strcasecmp(str, EC_STYLE_LOGARITHMIC))
        {
            return STYLE_LOGARITHMIC;
        }
        else if (0 == Platform::Strcasecmp(str, EC_STYLE_LOGARITHMIC_REV))
        {
            return STYLE_LOGARITHMIC_REV;
        }

        std::ostringstream os;
        os << "Unknown exposure contrast style: '" << str << "'.";

        throw Exception(os.str().c_str());
    }
    throw Exception("Missing exposure contrast style.");
}

const char * ExposureContrastOpData::ConvertStyleToString(ExposureContrastOpData::Style style)
{
    switch (style)
    {
    case STYLE_LINEAR:
    {
        return EC_STYLE_LINEAR;
        break;
    }
    case STYLE_LINEAR_REV:
    {
        return EC_STYLE_LINEAR_REV;
        break;
    }
    case STYLE_VIDEO:
    {
        return EC_STYLE_VIDEO;
        break;
    }
    case STYLE_VIDEO_REV:
    {
        return EC_STYLE_VIDEO_REV;
        break;
    }
    case STYLE_LOGARITHMIC:
    {
        return EC_STYLE_LOGARITHMIC;
        break;
    }
    case STYLE_LOGARITHMIC_REV:
    {
        return EC_STYLE_LOGARITHMIC_REV;
        break;
    }
    }

    throw Exception("Unknown exposure contrast style.");
}

ExposureContrastOpData::ExposureContrastOpData()
    : OpData(BIT_DEPTH_F32, BIT_DEPTH_F32)
    , m_exposure(std::make_shared<DynamicPropertyImpl>(0., false))
    , m_contrast(std::make_shared<DynamicPropertyImpl>(1., false))
    , m_gamma(std::make_shared<DynamicPropertyImpl>(1., false))
{
}

ExposureContrastOpData::ExposureContrastOpData(BitDepth inBitDepth,
                                               BitDepth outBitDepth,
                                               Style style)
    : OpData(inBitDepth, outBitDepth)
    , m_style(style)
    , m_exposure(std::make_shared<DynamicPropertyImpl>(0., false))
    , m_contrast(std::make_shared<DynamicPropertyImpl>(1., false))
    , m_gamma(std::make_shared<DynamicPropertyImpl>(1., false))
{
}

ExposureContrastOpData::~ExposureContrastOpData()
{
}

ExposureContrastOpDataRcPtr ExposureContrastOpData::clone() const
{
    ExposureContrastOpDataRcPtr res =
        std::make_shared<ExposureContrastOpData>(getInputBitDepth(),
                                                 getOutputBitDepth(),
                                                 getStyle());
    *res = *this;

    return res;
}

void ExposureContrastOpData::validate() const
{
    OpData::validate();
}

bool ExposureContrastOpData::isNoOp() const
{
    return isIdentity();
}

bool ExposureContrastOpData::isIdentity() const
{
    return !isDynamic() &&
           (m_exposure->getDoubleValue() == 0.) &&
           (m_contrast->getDoubleValue() == 1.) &&
           (m_gamma->getDoubleValue() == 1.);
}

bool ExposureContrastOpData::isDynamic() const
{
    return m_exposure->isDynamic() ||
           m_contrast->isDynamic() ||
           m_gamma->isDynamic();

}

bool ExposureContrastOpData::isInverse(ConstExposureContrastOpDataRcPtr & r) const
{
    // NB: Please see note in DynamicProperty.h describing how dynamic
    //     properties are compared for equality.
    return *r == *inverse();
}

ExposureContrastOpDataRcPtr ExposureContrastOpData::inverse() const
{
    ExposureContrastOpDataRcPtr ec = clone();
    
    Style invStyle = STYLE_LINEAR;
    switch (getStyle())
    {
    case STYLE_LINEAR:          invStyle = STYLE_LINEAR_REV;          break;
    case STYLE_LINEAR_REV:      invStyle = STYLE_LINEAR;              break;
    case STYLE_VIDEO:           invStyle = STYLE_VIDEO_REV;           break;
    case STYLE_VIDEO_REV:       invStyle = STYLE_VIDEO;               break;
    case STYLE_LOGARITHMIC:     invStyle = STYLE_LOGARITHMIC_REV;     break;
    case STYLE_LOGARITHMIC_REV: invStyle = STYLE_LOGARITHMIC;         break;
    }

    ec->setStyle(invStyle);

    ec->setInputBitDepth(getOutputBitDepth());
    ec->setOutputBitDepth(getInputBitDepth());

    return ec;
}

void ExposureContrastOpData::finalize()
{
    AutoMutex lock(m_mutex);

    std::ostringstream cacheIDStream;
    cacheIDStream << getID();

    cacheIDStream.precision(DefaultValues::FLOAT_DECIMALS);

    cacheIDStream << ConvertStyleToString(m_style) << " ";

    if (!m_exposure->isDynamic())
    {
        cacheIDStream << "E: " << m_exposure->getDoubleValue() << " ";
    }
    if (!m_contrast->isDynamic())
    {
        cacheIDStream << "C: " << m_contrast->getDoubleValue() << " ";
    }
    if (!m_gamma->isDynamic())
    {
        cacheIDStream << "G: " << m_gamma->getDoubleValue() << " ";
    }
    cacheIDStream << "P: " << m_pivot << " ";
    cacheIDStream << "LES: " << m_logExposureStep << " ";
    cacheIDStream << "LMG: " << m_logMidGray;

    m_cacheID = cacheIDStream.str();
}

bool ExposureContrastOpData::operator==(const OpData & other) const
{
    if (this == &other) return true;

    if (!OpData::operator==(other)) return false;

    const ExposureContrastOpData * ec = static_cast<const ExposureContrastOpData *>(&other);

    // NB: Please see note in DynamicProperty.h describing how dynamic
    //     properties are compared for equality.
    return getStyle() == ec->getStyle()
        && getPivot() == ec->getPivot()
        && getLogExposureStep() == ec->getLogExposureStep()
        && getLogMidGray() == ec->getLogMidGray()
        && *m_exposure == *(ec->m_exposure)
        && *m_contrast == *(ec->m_contrast)
        && *m_gamma == *(ec->m_gamma);
}

bool ExposureContrastOpData::hasDynamicProperty(DynamicPropertyType type) const
{
    bool res = false;
    switch (type)
    {
    case DYNAMIC_PROPERTY_EXPOSURE:
        res = m_exposure->isDynamic();
        break;
    case DYNAMIC_PROPERTY_CONTRAST:
        res = m_contrast->isDynamic();
        break;
    case DYNAMIC_PROPERTY_GAMMA:
        res = m_gamma->isDynamic();
        break;
    default:
        break;
    }
    
    return res;
}

DynamicPropertyRcPtr
ExposureContrastOpData::getDynamicProperty(DynamicPropertyType type) const
{
    switch (type)
    {
    case DYNAMIC_PROPERTY_EXPOSURE:
        if (m_exposure->isDynamic())
        {
            return m_exposure;
        }
        break;
    case DYNAMIC_PROPERTY_CONTRAST:
        if (m_contrast->isDynamic())
        {
            return m_contrast;
        }
        break;
    case DYNAMIC_PROPERTY_GAMMA:
        if (m_gamma->isDynamic())
        {
            return m_gamma;
        }
        break;
    default:
        throw Exception("Dynamic property type not supported by ExposureContrast.");
        break;
    }
    throw Exception("ExposureContrast property is not dynamic.");
}

void ExposureContrastOpData::replaceDynamicProperty(DynamicPropertyType type,
                                                    DynamicPropertyImplRcPtr prop)
{
    switch (type)
    {
    case DYNAMIC_PROPERTY_EXPOSURE:
        if (m_exposure->isDynamic())
        {
            m_exposure = prop;
            return;
        }
        break;
    case DYNAMIC_PROPERTY_CONTRAST:
        if (m_contrast->isDynamic())
        {
            m_contrast = prop;
            return;
        }
        break;
    case DYNAMIC_PROPERTY_GAMMA:
        if (m_gamma->isDynamic())
        {
            m_gamma = prop;
            return;
        }
        break;
    default:
        throw Exception("Dynamic property type not supported by ExposureContrast.");
        break;
    }
    throw Exception("ExposureContrast property is not dynamic.");
}

ExposureContrastOpData &
ExposureContrastOpData::operator=(const ExposureContrastOpData & rhs)
{
    if (this == &rhs) return *this;

    OpData::operator=(rhs);

    m_style = rhs.m_style;
    // Copy dynamic properties. Sharing happens when needed, with CPUop for instance.
    m_exposure->setValue(rhs.m_exposure->getDoubleValue());
    m_contrast->setValue(rhs.m_contrast->getDoubleValue());
    m_gamma->setValue(rhs.m_gamma->getDoubleValue());
    if (rhs.m_exposure->isDynamic())
    {
        m_exposure->makeDynamic();
    }
    if (rhs.m_contrast->isDynamic())
    {
        m_contrast->makeDynamic();
    }
    if (rhs.m_gamma->isDynamic())
    {
        m_gamma->makeDynamic();
    }
    m_pivot = rhs.m_pivot;
    m_logExposureStep = rhs.m_logExposureStep;
    m_logMidGray = rhs.m_logMidGray;

    return *this;
}

}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

OIIO_ADD_TEST(ExposureContrastOpData, style)
{
    OCIO::ExposureContrastOpData::Style style;
    const auto & ConvertToStyle =
        OCIO::ExposureContrastOpData::ConvertStringToStyle;
    OIIO_CHECK_NO_THROW(style =
        ConvertToStyle(OCIO::EC_STYLE_LINEAR));
    OIIO_CHECK_EQUAL(style, OCIO::ExposureContrastOpData::STYLE_LINEAR);
    OIIO_CHECK_NO_THROW(style =
        ConvertToStyle(OCIO::EC_STYLE_LINEAR_REV));
    OIIO_CHECK_EQUAL(style, OCIO::ExposureContrastOpData::STYLE_LINEAR_REV);
    OIIO_CHECK_NO_THROW(style =
        ConvertToStyle(OCIO::EC_STYLE_VIDEO));
    OIIO_CHECK_EQUAL(style, OCIO::ExposureContrastOpData::STYLE_VIDEO);
    OIIO_CHECK_NO_THROW(style =
        ConvertToStyle(OCIO::EC_STYLE_VIDEO_REV));
    OIIO_CHECK_EQUAL(style, OCIO::ExposureContrastOpData::STYLE_VIDEO_REV);
    OIIO_CHECK_NO_THROW(style =
        ConvertToStyle(OCIO::EC_STYLE_LOGARITHMIC));
    OIIO_CHECK_EQUAL(style, OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC);
    OIIO_CHECK_NO_THROW(style =
        ConvertToStyle(OCIO::EC_STYLE_LOGARITHMIC_REV));
    OIIO_CHECK_EQUAL(style, OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC_REV);

    OIIO_CHECK_THROW_WHAT(
        ConvertToStyle("Unknown exposure contrast style"),
        OCIO::Exception, "Unknown exposure contrast style");

    OIIO_CHECK_THROW_WHAT(
        ConvertToStyle(nullptr),
        OCIO::Exception, "Missing exposure contrast style");

    const auto & ConvertToString =
        OCIO::ExposureContrastOpData::ConvertStyleToString;
    std::string styleName;
    OIIO_CHECK_NO_THROW(styleName =
        ConvertToString(OCIO::ExposureContrastOpData::STYLE_LINEAR));
    OIIO_CHECK_EQUAL(styleName, OCIO::EC_STYLE_LINEAR);
    OIIO_CHECK_NO_THROW(styleName =
        ConvertToString(OCIO::ExposureContrastOpData::STYLE_LINEAR_REV));
    OIIO_CHECK_EQUAL(styleName, OCIO::EC_STYLE_LINEAR_REV);
    OIIO_CHECK_NO_THROW(styleName =
        ConvertToString(OCIO::ExposureContrastOpData::STYLE_VIDEO));
    OIIO_CHECK_EQUAL(styleName, OCIO::EC_STYLE_VIDEO);
    OIIO_CHECK_NO_THROW(styleName =
        ConvertToString(OCIO::ExposureContrastOpData::STYLE_VIDEO_REV));
    OIIO_CHECK_EQUAL(styleName, OCIO::EC_STYLE_VIDEO_REV);
    OIIO_CHECK_NO_THROW(styleName =
        ConvertToString(OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC));
    OIIO_CHECK_EQUAL(styleName, OCIO::EC_STYLE_LOGARITHMIC);
    OIIO_CHECK_NO_THROW(styleName =
        ConvertToString(OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC_REV));
    OIIO_CHECK_EQUAL(styleName, OCIO::EC_STYLE_LOGARITHMIC_REV);

    OIIO_CHECK_THROW_WHAT(
        ConvertToString((OCIO::ExposureContrastOpData::Style)-1),
        OCIO::Exception, "Unknown exposure contrast style");
}

OIIO_ADD_TEST(ExposureContrastOpData, accessors)
{
    OCIO::ExposureContrastOpData ec0;
    OIIO_CHECK_EQUAL(ec0.getType(), OCIO::OpData::ExposureContrastType);
    OIIO_CHECK_EQUAL(ec0.getStyle(), OCIO::ExposureContrastOpData::STYLE_LINEAR);

    OIIO_CHECK_EQUAL(ec0.getExposure(), 0.0);
    OIIO_CHECK_EQUAL(ec0.getContrast(), 1.0);
    OIIO_CHECK_EQUAL(ec0.getGamma(), 1.0);
    OIIO_CHECK_EQUAL(ec0.getPivot(), 0.18);
    OIIO_CHECK_EQUAL(ec0.getLogExposureStep(), 0.088);
    OIIO_CHECK_EQUAL(ec0.getLogMidGray(), 0.435);

    OIIO_CHECK_ASSERT(ec0.isIdentity());
    OIIO_CHECK_ASSERT(ec0.isNoOp());
    OIIO_CHECK_ASSERT(!ec0.hasChannelCrosstalk());
    OIIO_CHECK_NO_THROW(ec0.validate());
    ec0.finalize();
    const std::string cacheID(ec0.getCacheID());
    const std::string expected("linear E: 0 C: 1 G: 1 P: 0.18 LES: 0.088 LMG: 0.435");
    OIIO_CHECK_ASSERT(OCIO::Platform::Strcasecmp(cacheID.c_str(),
                                                 expected.c_str()) == 0);

    ec0.setExposure(0.1);
    OIIO_CHECK_ASSERT(!ec0.isIdentity());
    OIIO_CHECK_ASSERT(!ec0.isNoOp());
    OIIO_CHECK_ASSERT(!ec0.hasChannelCrosstalk());
    ec0.finalize();
    OIIO_CHECK_ASSERT(cacheID != std::string(ec0.getCacheID()));

    OCIO::ExposureContrastOpData ec(OCIO::BIT_DEPTH_UINT8,
                                    OCIO::BIT_DEPTH_F16,
                                    OCIO::ExposureContrastOpData::STYLE_VIDEO);
    OIIO_CHECK_EQUAL(ec.getType(), OCIO::OpData::ExposureContrastType);
    OIIO_CHECK_EQUAL(ec.getStyle(), OCIO::ExposureContrastOpData::STYLE_VIDEO);

    OIIO_CHECK_EQUAL(ec.getInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ec.getOutputBitDepth(), OCIO::BIT_DEPTH_F16);

    OIIO_CHECK_EQUAL(ec.getExposure(), 0.0);
    OIIO_CHECK_EQUAL(ec.getContrast(), 1.0);
    OIIO_CHECK_EQUAL(ec.getGamma(), 1.0);
    OIIO_CHECK_EQUAL(ec.getPivot(), 0.18);
    OIIO_CHECK_EQUAL(ec.getLogExposureStep(), 0.088);
    OIIO_CHECK_EQUAL(ec.getLogMidGray(), 0.435);

    OIIO_CHECK_ASSERT(ec.isNoOp());

    OIIO_CHECK_ASSERT(!ec.getExposureProperty()->isDynamic());
    OIIO_CHECK_ASSERT(!ec.getContrastProperty()->isDynamic());
    OIIO_CHECK_ASSERT(!ec.getGammaProperty()->isDynamic());
    OIIO_CHECK_ASSERT(!ec.isDynamic());

    // Never treated as NoOp when dynamic.
    ec.getExposureProperty()->makeDynamic();
    OIIO_CHECK_ASSERT(!ec.isNoOp());
    OIIO_CHECK_ASSERT(ec.isDynamic());
    OIIO_CHECK_ASSERT(ec.hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE));

    ec.setExposure(0.1);
    ec.setContrast(0.8);
    ec.setGamma(1.1);
    ec.setPivot(0.2);
    ec.setLogExposureStep(0.07);
    ec.setLogMidGray(0.5);

    OIIO_CHECK_EQUAL(ec.getExposure(), 0.1);
    OIIO_CHECK_EQUAL(ec.getContrast(), 0.8);
    OIIO_CHECK_EQUAL(ec.getGamma(), 1.1);
    OIIO_CHECK_EQUAL(ec.getPivot(), 0.2);
    OIIO_CHECK_EQUAL(ec.getLogExposureStep(), 0.07);
    OIIO_CHECK_EQUAL(ec.getLogMidGray(), 0.5);

    OCIO::DynamicPropertyRcPtr dpExp;
    OCIO::DynamicPropertyRcPtr dpContrast;
    OCIO::DynamicPropertyRcPtr dpGamma;

    // Property must be set as dynamic to accept a dynamic value.
    OIIO_CHECK_ASSERT(!ec.hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST));
    OIIO_CHECK_ASSERT(!ec.hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA));
    OIIO_CHECK_THROW_WHAT(ec.getDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST),
                          OCIO::Exception,
                          "not dynamic")

    OIIO_CHECK_NO_THROW(dpExp = ec.getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE));
    OIIO_CHECK_EQUAL(dpExp->getDoubleValue(), 0.1);
    OIIO_CHECK_ASSERT(!ec.hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST));
    OIIO_CHECK_ASSERT(!ec.hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA));
    dpExp->setValue(1.5);
    OIIO_CHECK_EQUAL(ec.getExposure(), 1.5);
    dpExp->setValue(0.7);
    OIIO_CHECK_EQUAL(ec.getExposure(), 0.7);

    ec.getContrastProperty()->makeDynamic();
    ec.getGammaProperty()->makeDynamic();
    OIIO_CHECK_NO_THROW(dpContrast = ec.getDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST));
    OIIO_CHECK_NO_THROW(dpGamma = ec.getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA));
    dpContrast->setValue(1.42);
    dpGamma->setValue(0.88);
    OIIO_CHECK_EQUAL(ec.getContrast(), 1.42);
    OIIO_CHECK_EQUAL(ec.getGamma(), 0.88);
}

OIIO_ADD_TEST(ExposureContrastOpData, clone)
{
    OCIO::ExposureContrastOpData ec;
    ec.setExposure(-1.4);
    ec.setContrast(0.8);
    ec.setGamma(1.1);
    ec.setPivot(0.2);

    ec.getExposureProperty()->makeDynamic();
    OCIO::DynamicPropertyRcPtr dpExp;

    OIIO_CHECK_NO_THROW(dpExp = ec.getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE));
    OIIO_CHECK_EQUAL(dpExp->getDoubleValue(), -1.4);
    dpExp->setValue(1.5);
    OCIO::ExposureContrastOpDataRcPtr ecCloned = ec.clone();
    OIIO_REQUIRE_ASSERT(ecCloned);

    OIIO_CHECK_EQUAL(ec.getExposure(), ecCloned->getExposure());
    OIIO_CHECK_EQUAL(ec.getContrast(), ecCloned->getContrast());
    OIIO_CHECK_EQUAL(ec.getGamma(), ecCloned->getGamma());
    OIIO_CHECK_EQUAL(ec.getPivot(), ecCloned->getPivot());
    OIIO_CHECK_EQUAL(ec.getLogExposureStep(), ecCloned->getLogExposureStep());
    OIIO_CHECK_EQUAL(ec.getLogMidGray(), ecCloned->getLogMidGray());

    OIIO_CHECK_EQUAL(ec.getExposureProperty()->isDynamic(),
                     ecCloned->getExposureProperty()->isDynamic());
    OIIO_CHECK_EQUAL(ec.getContrastProperty()->isDynamic(),
                     ecCloned->getContrastProperty()->isDynamic());
    OIIO_CHECK_EQUAL(ec.getGammaProperty()->isDynamic(),
                     ecCloned->getGammaProperty()->isDynamic());

    // Clone makes a copy of the dynamic property rather than sharing the original.
    dpExp->setValue(0.21);
    OIIO_CHECK_NE(ec.getExposure(), ecCloned->getExposure());
    OIIO_CHECK_EQUAL(ec.getExposure(), 0.21);
    OIIO_CHECK_EQUAL(ecCloned->getExposure(), 1.5);
}

OIIO_ADD_TEST(ExposureContrastOpData, inverse)
{
    OCIO::ExposureContrastOpData ec(OCIO::BIT_DEPTH_UINT8,
                                    OCIO::BIT_DEPTH_F16,
                                    OCIO::ExposureContrastOpData::STYLE_VIDEO);
    
    ec.setContrast(0.8);
    ec.setGamma(1.1);
    ec.setPivot(0.2);

    ec.getExposureProperty()->makeDynamic();
    OCIO::DynamicPropertyRcPtr dpExp;

    OIIO_CHECK_NO_THROW(dpExp = ec.getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE));
    dpExp->setValue(1.5);
    OCIO::ExposureContrastOpDataRcPtr ecInv = ec.inverse();
    OIIO_REQUIRE_ASSERT(ecInv);

    OCIO::ConstExposureContrastOpDataRcPtr ecInvConst = ecInv;
    OIIO_CHECK_ASSERT(ec.isInverse(ecInvConst));

    OIIO_CHECK_EQUAL(ecInv->getOutputBitDepth(), ec.getInputBitDepth());
    OIIO_CHECK_EQUAL(ecInv->getInputBitDepth(), ec.getOutputBitDepth());

    OIIO_CHECK_EQUAL(ecInv->getStyle(),
                     OCIO::ExposureContrastOpData::STYLE_VIDEO_REV);

    OIIO_CHECK_EQUAL(ec.getExposure(), ecInv->getExposure());
    OIIO_CHECK_EQUAL(ec.getContrast(), ecInv->getContrast());
    OIIO_CHECK_EQUAL(ec.getGamma(), ecInv->getGamma());
    OIIO_CHECK_EQUAL(ec.getPivot(), ecInv->getPivot());
    OIIO_CHECK_EQUAL(ec.getLogExposureStep(), ecInv->getLogExposureStep());
    OIIO_CHECK_EQUAL(ec.getLogMidGray(), ecInv->getLogMidGray());

    OIIO_CHECK_EQUAL(ec.getExposureProperty()->isDynamic(),
                     ecInv->getExposureProperty()->isDynamic());
    OIIO_CHECK_EQUAL(ec.getContrastProperty()->isDynamic(),
                     ecInv->getContrastProperty()->isDynamic());
    OIIO_CHECK_EQUAL(ec.getGammaProperty()->isDynamic(),
                     ecInv->getGammaProperty()->isDynamic());

    // Inverse makes a copy of the dynamic property rather than sharing the original.
    dpExp->setValue(0.21);
    OIIO_CHECK_NE(ec.getExposure(), ecInv->getExposure());
    OIIO_CHECK_EQUAL(ec.getExposure(), 0.21);
    OIIO_CHECK_EQUAL(ecInv->getExposure(), 1.5);
    
    // Exposure is dynamic in both, so value does not matter.
    OIIO_CHECK_ASSERT(ec.isInverse(ecInvConst));

    ecInv->getContrastProperty()->makeDynamic();

    // Contrast is dynamic in one and not in the other.
    OIIO_CHECK_ASSERT(!ec.isInverse(ecInvConst));

    ec.getContrastProperty()->makeDynamic();
    OIIO_CHECK_ASSERT(ec.isInverse(ecInvConst));

    // Gamma values are now different.
    ec.setGamma(1.2);
    OIIO_CHECK_ASSERT(!ec.isInverse(ecInvConst));
}

OIIO_ADD_TEST(ExposureContrastOpData, equality)
{
    OCIO::ExposureContrastOpData ec0;
    OCIO::ExposureContrastOpData ec1;
    OIIO_CHECK_ASSERT(ec0 == ec1);

    // Change style.
    ec0.setStyle(OCIO::ExposureContrastOpData::STYLE_VIDEO);
    OIIO_CHECK_ASSERT(!(ec0 == ec1));
    ec1.setStyle(OCIO::ExposureContrastOpData::STYLE_VIDEO);
    OIIO_CHECK_ASSERT(ec0 == ec1);

    // Change dynamic.
    ec0.getExposureProperty()->makeDynamic();
    OIIO_CHECK_ASSERT(!(ec0 == ec1));
    ec1.getExposureProperty()->makeDynamic();
    OIIO_CHECK_ASSERT(ec0 == ec1);

    // Change value of enabled dynamic property.
    ec0.setExposure(0.5);
    OIIO_CHECK_ASSERT(ec0 == ec1);
    
    // Change value of dynamic property not enabled.
    ec1.setContrast(0.5);
    OIIO_CHECK_ASSERT(!(ec0 == ec1));
    
    ec0.setContrast(0.5);
    OIIO_CHECK_ASSERT(ec0 == ec1);
}

OIIO_ADD_TEST(ExposureContrastOpData, replace_dynamic_property)
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
    OIIO_CHECK_NE(dpe0.get(), dpe1.get());

    ec1.replaceDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE, ec0.getExposureProperty());
    dpe1 = ec1.getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE);

    // Now, this is the same pointer.
    OIIO_CHECK_EQUAL(dpe0.get(), dpe1.get());

    ec0.getContrastProperty()->makeDynamic();
    // Contrast is not enabled in ec1.
    OIIO_CHECK_THROW(ec1.getDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST), OCIO::Exception);

    // The property is not replaced if dynamic is not enabled.
    OIIO_CHECK_THROW(ec1.replaceDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST,
                                                ec0.getContrastProperty()),
                     OCIO::Exception);
    OIIO_CHECK_THROW(ec1.getDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST), OCIO::Exception);
}

#endif
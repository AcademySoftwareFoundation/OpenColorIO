// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/exposurecontrast/ExposureContrastOpData.h"
#include "Platform.h"

namespace OCIO_NAMESPACE
{

namespace
{
// CTF style attribute strings.
static constexpr char EC_STYLE_LINEAR[]          = "linear";
static constexpr char EC_STYLE_LINEAR_REV[]      = "linearRev";
static constexpr char EC_STYLE_VIDEO[]           = "video";
static constexpr char EC_STYLE_VIDEO_REV[]       = "videoRev";
static constexpr char EC_STYLE_LOGARITHMIC[]     = "log";
static constexpr char EC_STYLE_LOGARITHMIC_REV[] = "logRev";
}

namespace DefaultValues
{
const int FLOAT_DECIMALS = 7;
}

// Convert CTF attribute string to OpData style.
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

// Convert internal OpData style enum to CTF attribute string.
const char * ExposureContrastOpData::ConvertStyleToString(ExposureContrastOpData::Style style)
{
    switch (style)
    {
    case STYLE_LINEAR:
    {
        return EC_STYLE_LINEAR;
    }
    case STYLE_LINEAR_REV:
    {
        return EC_STYLE_LINEAR_REV;
    }
    case STYLE_VIDEO:
    {
        return EC_STYLE_VIDEO;
    }
    case STYLE_VIDEO_REV:
    {
        return EC_STYLE_VIDEO_REV;
    }
    case STYLE_LOGARITHMIC:
    {
        return EC_STYLE_LOGARITHMIC;
    }
    case STYLE_LOGARITHMIC_REV:
    {
        return EC_STYLE_LOGARITHMIC_REV;
    }
    }

    throw Exception("Unknown exposure contrast style.");
}

// Combine the Transform style and direction into the internal OpData style.
ExposureContrastOpData::Style ExposureContrastOpData::ConvertStyle(ExposureContrastStyle style,
                                                                   TransformDirection dir)
{
    const bool isForward = dir == TRANSFORM_DIR_FORWARD;

    switch (style)
    {
    case EXPOSURE_CONTRAST_VIDEO:
    {
        return (isForward) ? ExposureContrastOpData::STYLE_VIDEO :
                             ExposureContrastOpData::STYLE_VIDEO_REV;
    }
    case EXPOSURE_CONTRAST_LOGARITHMIC:
    {
        return (isForward) ? ExposureContrastOpData::STYLE_LOGARITHMIC :
                             ExposureContrastOpData::STYLE_LOGARITHMIC_REV;
    }
    case EXPOSURE_CONTRAST_LINEAR:
    {
        return (isForward) ? ExposureContrastOpData::STYLE_LINEAR :
                             ExposureContrastOpData::STYLE_LINEAR_REV;
    }
    }

    std::stringstream ss("Unknown ExposureContrast transform style: ");
    ss << style;

    throw Exception(ss.str().c_str());
}

// Convert internal OpData style to Transform style.
ExposureContrastStyle ExposureContrastOpData::ConvertStyle(ExposureContrastOpData::Style style)
{
    switch (style)
    {
    case ExposureContrastOpData::STYLE_VIDEO:
    case ExposureContrastOpData::STYLE_VIDEO_REV:
        return EXPOSURE_CONTRAST_VIDEO;

    case ExposureContrastOpData::STYLE_LOGARITHMIC:
    case ExposureContrastOpData::STYLE_LOGARITHMIC_REV:
        return EXPOSURE_CONTRAST_LOGARITHMIC;

    case ExposureContrastOpData::STYLE_LINEAR:
    case ExposureContrastOpData::STYLE_LINEAR_REV:
        return EXPOSURE_CONTRAST_LINEAR;
    }

    std::stringstream ss("Unknown ExposureContrast style: ");
    ss << style;

    throw Exception(ss.str().c_str());
}

ExposureContrastOpData::ExposureContrastOpData()
    : OpData()
    , m_exposure(std::make_shared<DynamicPropertyDoubleImpl>(DYNAMIC_PROPERTY_EXPOSURE, 0., false))
    , m_contrast(std::make_shared<DynamicPropertyDoubleImpl>(DYNAMIC_PROPERTY_CONTRAST, 1., false))
    , m_gamma(std::make_shared<DynamicPropertyDoubleImpl>(DYNAMIC_PROPERTY_GAMMA, 1., false))
{
}

ExposureContrastOpData::ExposureContrastOpData(Style style)
    : OpData()
    , m_style(style)
    , m_exposure(std::make_shared<DynamicPropertyDoubleImpl>(DYNAMIC_PROPERTY_EXPOSURE, 0., false))
    , m_contrast(std::make_shared<DynamicPropertyDoubleImpl>(DYNAMIC_PROPERTY_CONTRAST, 1., false))
    , m_gamma(std::make_shared<DynamicPropertyDoubleImpl>(DYNAMIC_PROPERTY_GAMMA, 1., false))
{
}

ExposureContrastOpData::~ExposureContrastOpData()
{
}

ExposureContrastOpDataRcPtr ExposureContrastOpData::clone() const
{
    ExposureContrastOpDataRcPtr res = std::make_shared<ExposureContrastOpData>(getStyle());
    *res = *this;

    return res;
}

void ExposureContrastOpData::validate() const
{
}

bool ExposureContrastOpData::isNoOp() const
{
    return isIdentity();
}

bool ExposureContrastOpData::isIdentity() const
{
    return !isDynamic() &&
           (m_exposure->getValue() == 0.) &&
           (m_contrast->getValue() == 1.) &&
           (m_gamma->getValue() == 1.);
}

bool ExposureContrastOpData::isDynamic() const
{
    return m_exposure->isDynamic() ||
           m_contrast->isDynamic() ||
           m_gamma->isDynamic();
}

bool ExposureContrastOpData::isInverse(ConstExposureContrastOpDataRcPtr & r) const
{
    if (isDynamic() || r->isDynamic())
    {
        return false;
    }
    
    return *r == *inverse();
}

void ExposureContrastOpData::invert() noexcept
{
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
    setStyle(invStyle);
}

ExposureContrastOpDataRcPtr ExposureContrastOpData::inverse() const
{
    ExposureContrastOpDataRcPtr ec = clone();

    ec->invert();

    // Note that any existing metadata could become stale at this point but
    // trying to update it is also challenging since inverse() is sometimes
    // called even during the creation of new ops.

    return ec;
}

std::string ExposureContrastOpData::getCacheID() const
{
    AutoMutex lock(m_mutex);

    std::ostringstream cacheIDStream;
    if (!getID().empty())
    {
        cacheIDStream << getID() << " ";
    }

    cacheIDStream.precision(DefaultValues::FLOAT_DECIMALS);

    cacheIDStream << ConvertStyleToString(m_style) << " ";

    if (!m_exposure->isDynamic())
    {
        cacheIDStream << "E: " << m_exposure->getValue() << " ";
    }
    if (!m_contrast->isDynamic())
    {
        cacheIDStream << "C: " << m_contrast->getValue() << " ";
    }
    if (!m_gamma->isDynamic())
    {
        cacheIDStream << "G: " << m_gamma->getValue() << " ";
    }
    cacheIDStream << "P: " << m_pivot << " ";
    cacheIDStream << "LES: " << m_logExposureStep << " ";
    cacheIDStream << "LMG: " << m_logMidGray;

    return cacheIDStream.str();
}

bool ExposureContrastOpData::operator==(const OpData & other) const
{
    if (!OpData::operator==(other)) return false;

    const ExposureContrastOpData * ec = static_cast<const ExposureContrastOpData *>(&other);

    // NB: Please see note in DynamicProperty.h describing how dynamic
    //     properties are compared for equality.
    return getStyle() == ec->getStyle()
        && getPivot() == ec->getPivot()
        && getLogExposureStep() == ec->getLogExposureStep()
        && getLogMidGray() == ec->getLogMidGray()
        && m_exposure->equals(*(ec->m_exposure))
        && m_contrast->equals(*(ec->m_contrast))
        && m_gamma->equals(*(ec->m_gamma));
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
    case DYNAMIC_PROPERTY_GRADING_PRIMARY:
    case DYNAMIC_PROPERTY_GRADING_RGBCURVE:
    case DYNAMIC_PROPERTY_GRADING_TONE:
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
    case DYNAMIC_PROPERTY_GRADING_PRIMARY:
    case DYNAMIC_PROPERTY_GRADING_RGBCURVE:
    case DYNAMIC_PROPERTY_GRADING_TONE:
    default:
        throw Exception("Dynamic property type not supported by ExposureContrast.");
    }
    throw Exception("ExposureContrast property is not dynamic.");
}

void ExposureContrastOpData::replaceDynamicProperty(DynamicPropertyType type,
                                                    DynamicPropertyDoubleImplRcPtr & prop)
{
    auto propDouble = OCIO_DYNAMIC_POINTER_CAST<DynamicPropertyDoubleImpl>(prop);
    if (propDouble)
    {

        switch (type)
        {
        case DYNAMIC_PROPERTY_EXPOSURE:
            if (m_exposure->isDynamic())
            {
                m_exposure = propDouble;
                return;
            }
            break;
        case DYNAMIC_PROPERTY_CONTRAST:
            if (m_contrast->isDynamic())
            {
                m_contrast = propDouble;
                return;
            }
            break;
        case DYNAMIC_PROPERTY_GAMMA:
            if (m_gamma->isDynamic())
            {
                m_gamma = propDouble;
                return;
            }
            break;
        case DYNAMIC_PROPERTY_GRADING_PRIMARY:
        case DYNAMIC_PROPERTY_GRADING_RGBCURVE:
        case DYNAMIC_PROPERTY_GRADING_TONE:
        default:
            throw Exception("Dynamic property type not supported by ExposureContrast.");
        }
        throw Exception("ExposureContrast property is not dynamic.");
    }
    throw Exception("Dynamic property type not supported by ExposureContrast.");
}

void ExposureContrastOpData::removeDynamicProperties()
{
    m_exposure->makeNonDynamic();
    m_contrast->makeNonDynamic();
    m_gamma->makeNonDynamic();
}

ExposureContrastOpData & ExposureContrastOpData::operator=(const ExposureContrastOpData & rhs)
{
    if (this == &rhs) return *this;

    OpData::operator=(rhs);

    m_style = rhs.m_style;
    // Copy dynamic properties. Sharing happens when needed, with CPUop for instance.
    m_exposure->setValue(rhs.m_exposure->getValue());
    m_contrast->setValue(rhs.m_contrast->getValue());
    m_gamma->setValue(rhs.m_gamma->getValue());
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

// Convert internal OpData style into Transform direction.
TransformDirection ExposureContrastOpData::getDirection() const noexcept
{
    switch (m_style)
    {
    case ExposureContrastOpData::STYLE_LINEAR:
    case ExposureContrastOpData::STYLE_VIDEO:
    case ExposureContrastOpData::STYLE_LOGARITHMIC:
        return TRANSFORM_DIR_FORWARD;

    case ExposureContrastOpData::STYLE_LINEAR_REV:
    case ExposureContrastOpData::STYLE_VIDEO_REV:
    case ExposureContrastOpData::STYLE_LOGARITHMIC_REV:
        return TRANSFORM_DIR_INVERSE;
    }
    return TRANSFORM_DIR_FORWARD;
}

void ExposureContrastOpData::setDirection(TransformDirection dir) noexcept
{
    if (getDirection() != dir)
    {
        invert();
    }
}

} // namespace OCIO_NAMESPACE


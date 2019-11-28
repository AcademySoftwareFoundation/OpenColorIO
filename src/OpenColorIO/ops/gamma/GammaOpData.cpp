// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "ops/gamma/GammaOpData.h"
#include "ops/matrix/MatrixOp.h"
#include "ops/range/RangeOpData.h"
#include "ParseUtils.h"
#include "Platform.h"

namespace OCIO_NAMESPACE
{

namespace
{

const int FLOAT_DECIMALS = 7;

// Declare the values for an identity operation.
const double IdentityScale = 1.;
const double IdentityOffset = 0.;

// Check if params corresponds to a basic identity.
inline bool IsBasicIdentity(const GammaOpData::Params & p)
{
    return (p[0] == IdentityScale);
}

// Check if params corresponds to a moncurve identity.
inline bool IsMonCurveIdentity(const GammaOpData::Params & p)
{
    return (p[0] == IdentityScale && p[1] == IdentityOffset);
}

std::string GetParametersString(const GammaOpData::Params & params)
{
    std::ostringstream oss;
    oss.precision(FLOAT_DECIMALS);
    oss << params[0];
    for(size_t idx=1; idx<params.size(); ++idx)
    {
        oss << ", " << params[idx];
    }
    return oss.str();
}

static constexpr const char * GAMMA_STYLE_BASIC_FWD    = "basicFwd";
static constexpr const char * GAMMA_STYLE_BASIC_REV    = "basicRev";
static constexpr const char * GAMMA_STYLE_MONCURVE_FWD = "moncurveFwd";
static constexpr const char * GAMMA_STYLE_MONCURVE_REV = "moncurveRev";

}; // anon

GammaOpData::Style GammaOpData::ConvertStringToStyle(const char * str)
{
    if (str && *str)
    {
        if (0 == Platform::Strcasecmp(str, GAMMA_STYLE_BASIC_FWD))
        {
            return BASIC_FWD;
        }
        else if (0 == Platform::Strcasecmp(str, GAMMA_STYLE_BASIC_REV))
        {
            return BASIC_REV;
        }
        else if (0 == Platform::Strcasecmp(str, GAMMA_STYLE_MONCURVE_FWD))
        {
            return MONCURVE_FWD;
        }
        else if (0 == Platform::Strcasecmp(str, GAMMA_STYLE_MONCURVE_REV))
        {
            return MONCURVE_REV;
        }

        std::ostringstream os;
        os << "Unknown gamma style: '" << str << "'.";

        throw Exception(os.str().c_str());
    }

    throw Exception("Missing gamma style.");
}

const char * GammaOpData::ConvertStyleToString(Style style)
{
    switch(style)
    {
        case BASIC_FWD:
            return GAMMA_STYLE_BASIC_FWD;
        case BASIC_REV:
            return GAMMA_STYLE_BASIC_REV;
        case MONCURVE_FWD:
            return GAMMA_STYLE_MONCURVE_FWD;
        case MONCURVE_REV:
            return GAMMA_STYLE_MONCURVE_REV;
    }

    std::stringstream ss("Unknown Gamma style: ");
    ss << style;

    throw Exception(ss.str().c_str());
}

GammaOpData::GammaOpData()
    :   OpData()
    ,   m_style(BASIC_FWD)
    ,   m_redParams(getIdentityParameters(m_style))
    ,   m_greenParams(getIdentityParameters(m_style))
    ,   m_blueParams(getIdentityParameters(m_style))
    ,   m_alphaParams(getIdentityParameters(m_style))
{
}

GammaOpData::GammaOpData(const Style & style,
                         const Params & redParams,
                         const Params & greenParams,
                         const Params & blueParams,
                         const Params & alphaParams)
    :   OpData()
    ,   m_style(style)
    ,   m_redParams(redParams)
    ,   m_greenParams(greenParams)
    ,   m_blueParams(blueParams)
    ,   m_alphaParams(alphaParams)
{
}

GammaOpData::~GammaOpData()
{
}

GammaOpDataRcPtr GammaOpData::clone() const
{
    return std::make_shared<GammaOpData>(*this);
}

GammaOpDataRcPtr GammaOpData::inverse() const
{
    GammaOpDataRcPtr gamma = clone();

    Style invStyle = BASIC_FWD;
    switch(getStyle())
    {
      case BASIC_FWD:     invStyle = BASIC_REV;     break;
      case BASIC_REV:     invStyle = BASIC_FWD;     break;
      case MONCURVE_FWD:  invStyle = MONCURVE_REV;  break;
      case MONCURVE_REV:  invStyle = MONCURVE_FWD;  break;
    }
    gamma->setStyle(invStyle);

    // Note that any existing metadata could become stale at this point but
    // trying to update it is also challenging since inverse() is sometimes
    // called even during the creation of new ops.
    return gamma;
}

bool GammaOpData::isInverse(const GammaOpData & B) const
{
    const Style styleA = getStyle();
    const Style styleB = B.getStyle();

    // Note: It's possible that someone could create something where they don't respect our
    // convention of keeping gamma > 1, in which case, there could be two BASIC_FWD that would
    // be an identity. This code does not to try and handle that case yet, however the pair of
    // ops would get combined and removed as an identity in the optimizer.

    if ( (styleA == BASIC_FWD && styleB == BASIC_REV) ||
         (styleA == BASIC_REV && styleB == BASIC_FWD) ||
         (styleA == MONCURVE_FWD && styleB == MONCURVE_REV) ||
         (styleA == MONCURVE_REV && styleB == MONCURVE_FWD) )
    {
        if ( (getRedParams() == B.getRedParams()) &&
             (getGreenParams() == B.getGreenParams()) &&
             (getBlueParams() == B.getBlueParams()) &&
             (getAlphaParams() == B.getAlphaParams()) )
        {
            return true;
        }
    }

    return false;
}

void GammaOpData::setStyle(const Style & style)
{
    // NB: Must call validate after using this method.
    m_style = style;
}

void GammaOpData::setRedParams(const Params & p)
{
    // NB: Must call validate after using this method.
    m_redParams = p;
}

void GammaOpData::setGreenParams(const Params & p)
{
    // NB: Must call validate after using this method.
    m_greenParams = p;
}

void GammaOpData::setBlueParams(const Params & p)
{
    // NB: Must call validate after using this method.
    m_blueParams = p;
}

void GammaOpData::setAlphaParams(const Params & p)
{
    // NB: Must call validate after using this method.
    m_alphaParams = p;
}

void GammaOpData::setParams(const Params & p)
{
    // NB: Must call validate after using this method.
    m_redParams   = p;
    m_greenParams = p;
    m_blueParams  = p;
    m_alphaParams = GammaOpData::getIdentityParameters(getStyle());
}

void validateParams(const GammaOpData::Params & p, 
                    unsigned int reqdSize,
                    const double * lowBounds, 
                    const double * highBounds)
{
    if (p.size() != reqdSize)
    {
        throw Exception("GammaOp: Wrong number of parameters");
    }
    for (unsigned int i=0; i<reqdSize; i++)
    {
        if (p[i] < lowBounds[i])
        {
            std::stringstream ss;
            ss << "Parameter " << p[i] << " is less than lower bound " << lowBounds[i];
            throw Exception(ss.str().c_str());
        }
        if (p[i] > highBounds[i])
        {
            std::stringstream ss;
            ss << "Parameter " << p[i] << " is greater than upper bound " << highBounds[i];
            throw Exception(ss.str().c_str());
        }
    }
}

void GammaOpData::validate() const
{
    OpData::validate();
    GammaOpData::validateParameters();
}

void GammaOpData::validateParameters() const
{
    // Note: When loading from a CTF we want to enforce 
    //       the canonical bounds on the parameters. 

    switch(getStyle())
    {
        case BASIC_FWD:
        case BASIC_REV:
        {
            static const unsigned int reqdSize = 1;
            static const double lowBounds[] = {0.01};
            static const double highBounds[] = {100.};
            validateParams(m_redParams,   reqdSize, lowBounds, highBounds);
            validateParams(m_greenParams, reqdSize, lowBounds, highBounds);
            validateParams(m_blueParams,  reqdSize, lowBounds, highBounds);
            validateParams(m_alphaParams, reqdSize, lowBounds, highBounds);
            break;
        }

        case MONCURVE_FWD:
        case MONCURVE_REV:
        {
            static const unsigned int reqdSize = 2;
            static const double lowBounds[] = {1., 0.};
            static const double highBounds[] = {10., 0.9};
            validateParams(m_redParams,   reqdSize, lowBounds, highBounds);
            validateParams(m_greenParams, reqdSize, lowBounds, highBounds);
            validateParams(m_blueParams,  reqdSize, lowBounds, highBounds);
            validateParams(m_alphaParams, reqdSize, lowBounds, highBounds);
            break;
        }
    }
}

GammaOpData::Params GammaOpData::getIdentityParameters(Style style)
{
    GammaOpData::Params params;
    switch(style)
    {
        case BASIC_FWD:
        case BASIC_REV:
        {
            params.push_back(IdentityScale);
            break;
        }
        case MONCURVE_FWD:
        case MONCURVE_REV:
        {
            params.push_back(IdentityScale);
            params.push_back(IdentityOffset);
            break;
        }
    }

    return params;
}

bool GammaOpData::isIdentityParameters(const Params & parameters, Style style)
{
    switch(style)
    {
        case BASIC_FWD:
        case BASIC_REV:
        {
            return (parameters.size() == 1 && IsBasicIdentity(parameters));
        }
        case MONCURVE_FWD:
        case MONCURVE_REV:
        {
            return (parameters.size() == 2 && IsMonCurveIdentity(parameters));
        }
    }

    return false;
}

bool GammaOpData::isAlphaComponentIdentity() const
{
    return isIdentityParameters(m_alphaParams, getStyle());
}

bool GammaOpData::areAllComponentsEqual() const
{
    // Comparing floats is generally not a good idea, but in this case
    // it is ok to be strict.  Since the same operations are applied to
    // all components, if they started equal, they should remain equal.
    return m_redParams == m_greenParams  &&  m_redParams == m_blueParams 
        && m_redParams == m_alphaParams;
}

bool GammaOpData::isNonChannelDependent() const
{
    return m_redParams == m_greenParams && m_redParams == m_blueParams
        && isAlphaComponentIdentity();
}

bool GammaOpData::isNoOp() const
{
    return isIdentity() && !isClamping();
}

bool GammaOpData::isIdentity() const
{
    switch(getStyle())
    {
        case BASIC_FWD:
        case BASIC_REV:
        {
            if (areAllComponentsEqual() && IsBasicIdentity(m_redParams) )
            {
                return true;
            }
            break;
        }

        case MONCURVE_FWD:
        case MONCURVE_REV:
        {
            if (areAllComponentsEqual() && IsMonCurveIdentity(m_redParams) )
            {
                return true;
            }
            break;
        }
    }
    return false;
}

bool GammaOpData::isClamping() const
{
    return getStyle()==BASIC_FWD || getStyle()==BASIC_REV;
}

bool GammaOpData::mayCompose(const GammaOpData & B) const
{
    // TODO: This hits the most likely scenario, but there are other cases
    //       which technically could be combined (e.g. R & G params unequal).

    // Note: Decided not to make this dependent upon bit depth.

    // NB: This also does not check bypass or dynamic.

    if ( ! isNonChannelDependent() )
        return false;

    // At this point, we have R == G == B, and A == identity.

    if (getStyle() != BASIC_FWD && getStyle() != BASIC_REV)
        return false;

    if (B.getStyle() != BASIC_FWD && B.getStyle() != BASIC_REV)
        return false;

    return true;
}

OpDataRcPtr GammaOpData::getIdentityReplacement() const
{
    OpDataRcPtr op;
    switch(getStyle())
    {
        // These clamp values below 0 -- replace with range.
        // TODO: Gamma processes alpha whereas Range does not.  So the replacement potentially
        // gives somewhat different results since negative alpha values would not be clamped.
        case BASIC_FWD:
        case BASIC_REV:
        {
            op = std::make_shared<RangeOpData>(0.,
                                               RangeOpData::EmptyValue(), // Don't clamp high end.
                                               0.,
                                               RangeOpData::EmptyValue());

            break;
        }

        // These pass through the full range of values -- replace with matrix.
        case MONCURVE_FWD:
        case MONCURVE_REV:
        {
            op = std::make_shared<MatrixOpData>();
            break;
        }
    }
    return op;
}

GammaOpDataRcPtr GammaOpData::compose(const GammaOpData & B) const
{
    if ( ! mayCompose(B) )
    {
        throw Exception("GammaOp can only be combined with some GammaOps");
    }

    // At this point, we have R == G == B, and A == identity
    // and the style is either BASIC FWD or REV.

    double g1 = getRedParams()[0];
    if (getStyle() == BASIC_REV)
    {
        g1 = 1. / g1;
    }

    double g2 = B.getRedParams()[0];
    if (B.getStyle() == BASIC_REV)
    {
        g2 = 1. / g2;
    }

    double gOut = g1 * g2;
    Style style = BASIC_FWD;
    // By convention, we try to keep the gamma parameter > 1.
    if (gOut < 1.0)
    {
        gOut = 1. / gOut;
        style = BASIC_REV;
    }

    // Prevent small rounding errors from not making an identity.
    // E.g., 1/0.45 * 0.45 should have a value exactly 1.
    const double diff = fabs( gOut - 1. );
    if ( diff < 1e-6 )
    {
        gOut = 1.;
    }

    GammaOpData::Params params;
    params.push_back(gOut);
    GammaOpData::Params paramsA;
    paramsA.push_back(1.);

    GammaOpDataRcPtr outOp = std::make_shared<GammaOpData>(style,
                                                           params,
                                                           params,
                                                           params,
                                                           paramsA);

    // TODO: May want to revisit how the metadata is set.
    outOp->getFormatMetadata() = getFormatMetadata();
    outOp->getFormatMetadata().combine(B.getFormatMetadata());

    return outOp;
}

bool GammaOpData::operator==(const OpData & other) const
{
    if(!OpData::operator==(other)) return false;

    const GammaOpData* gop = static_cast<const GammaOpData*>(&other);

    return  m_style == gop->m_style &&
            m_redParams == gop->m_redParams &&
            m_greenParams == gop->m_greenParams &&
            m_blueParams == gop->m_blueParams &&
            m_alphaParams == gop->m_alphaParams;
}

void GammaOpData::finalize()
{
    AutoMutex lock(m_mutex);

    validate();

    std::ostringstream cacheIDStream;
    cacheIDStream << getID() << " ";

    cacheIDStream << GammaOpData::ConvertStyleToString(getStyle()) << " ";

    cacheIDStream << "r:" << GetParametersString(getRedParams())   << " ";
    cacheIDStream << "g:" << GetParametersString(getGreenParams()) << " ";
    cacheIDStream << "b:" << GetParametersString(getBlueParams())  << " ";
    cacheIDStream << "a:" << GetParametersString(getAlphaParams()) << " ";

    m_cacheID = cacheIDStream.str();
}


} // namespace OCIO_NAMESPACE


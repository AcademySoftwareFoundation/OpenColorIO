// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cmath>
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

constexpr char GAMMA_STYLE_BASIC_FWD[]           = "basicFwd";
constexpr char GAMMA_STYLE_BASIC_REV[]           = "basicRev";
constexpr char GAMMA_STYLE_BASIC_MIRROR_FWD[]    = "basicMirrorFwd";
constexpr char GAMMA_STYLE_BASIC_MIRROR_REV[]    = "basicMirrorRev";
constexpr char GAMMA_STYLE_BASIC_PASS_THRU_FWD[] = "basicPassThruFwd";
constexpr char GAMMA_STYLE_BASIC_PASS_THRU_REV[] = "basicPassThruRev";
// Note that CTF before version 2 was using "moncurveFwd". Parsing is case insensitive.
constexpr char GAMMA_STYLE_MONCURVE_FWD[]        = "monCurveFwd";
constexpr char GAMMA_STYLE_MONCURVE_REV[]        = "monCurveRev";
constexpr char GAMMA_STYLE_MONCURVE_MIRROR_FWD[] = "monCurveMirrorFwd";
constexpr char GAMMA_STYLE_MONCURVE_MIRROR_REV[] = "monCurveMirrorRev";

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
        else if (0 == Platform::Strcasecmp(str, GAMMA_STYLE_BASIC_MIRROR_FWD))
        {
            return BASIC_MIRROR_FWD;
        }
        else if (0 == Platform::Strcasecmp(str, GAMMA_STYLE_BASIC_MIRROR_REV))
        {
            return BASIC_MIRROR_REV;
        }
        else if (0 == Platform::Strcasecmp(str, GAMMA_STYLE_BASIC_PASS_THRU_FWD))
        {
            return BASIC_PASS_THRU_FWD;
        }
        else if (0 == Platform::Strcasecmp(str, GAMMA_STYLE_BASIC_PASS_THRU_REV))
        {
            return BASIC_PASS_THRU_REV;
        }
        else if (0 == Platform::Strcasecmp(str, GAMMA_STYLE_MONCURVE_FWD))
        {
            return MONCURVE_FWD;
        }
        else if (0 == Platform::Strcasecmp(str, GAMMA_STYLE_MONCURVE_REV))
        {
            return MONCURVE_REV;
        }
        else if (0 == Platform::Strcasecmp(str, GAMMA_STYLE_MONCURVE_MIRROR_FWD))
        {
            return MONCURVE_MIRROR_FWD;
        }
        else if (0 == Platform::Strcasecmp(str, GAMMA_STYLE_MONCURVE_MIRROR_REV))
        {
            return MONCURVE_MIRROR_REV;
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
    case BASIC_MIRROR_FWD:
        return GAMMA_STYLE_BASIC_MIRROR_FWD;
    case BASIC_MIRROR_REV:
        return GAMMA_STYLE_BASIC_MIRROR_REV;
    case BASIC_PASS_THRU_FWD:
        return GAMMA_STYLE_BASIC_PASS_THRU_FWD;
    case BASIC_PASS_THRU_REV:
        return GAMMA_STYLE_BASIC_PASS_THRU_REV;
    case MONCURVE_FWD:
        return GAMMA_STYLE_MONCURVE_FWD;
    case MONCURVE_REV:
        return GAMMA_STYLE_MONCURVE_REV;
    case MONCURVE_MIRROR_FWD:
        return GAMMA_STYLE_MONCURVE_MIRROR_FWD;
    case MONCURVE_MIRROR_REV:
        return GAMMA_STYLE_MONCURVE_MIRROR_REV;
    }

    std::stringstream ss("Unknown Gamma style: ");
    ss << style;

    throw Exception(ss.str().c_str());
}

NegativeStyle GammaOpData::ConvertStyle(Style style)
{
    switch (style)
    {
    case BASIC_FWD:
    case BASIC_REV:
        return NEGATIVE_CLAMP;
    case MONCURVE_FWD:
    case MONCURVE_REV:
        return NEGATIVE_LINEAR;
    case BASIC_MIRROR_FWD:
    case BASIC_MIRROR_REV:
    case MONCURVE_MIRROR_FWD:
    case MONCURVE_MIRROR_REV:
        return NEGATIVE_MIRROR;
    case BASIC_PASS_THRU_FWD:
    case BASIC_PASS_THRU_REV:
        return NEGATIVE_PASS_THRU;
    }

    std::stringstream ss("Unknown Gamma style: ");
    ss << style;

    throw Exception(ss.str().c_str());
}

GammaOpData::Style GammaOpData::ConvertStyleBasic(NegativeStyle negStyle, TransformDirection dir)
{
    const bool isForward = dir == TRANSFORM_DIR_FORWARD;

    switch (negStyle)
    {
    case NEGATIVE_CLAMP:
    {
        return (isForward) ? GammaOpData::BASIC_FWD :
                             GammaOpData::BASIC_REV;
    }
    case NEGATIVE_MIRROR:
    {
        return (isForward) ? GammaOpData::BASIC_MIRROR_FWD :
                             GammaOpData::BASIC_MIRROR_REV;
    }
    case NEGATIVE_PASS_THRU:
    {
        return (isForward) ? GammaOpData::BASIC_PASS_THRU_FWD :
                             GammaOpData::BASIC_PASS_THRU_REV;
    }
    case NEGATIVE_LINEAR:
    {
        throw Exception("Linear negative extrapolation is not valid for basic exponent style.");
    }
    }

    std::stringstream ss("Unknown negative extrapolation style: ");
    ss << negStyle;

    throw Exception(ss.str().c_str());
}

GammaOpData::Style GammaOpData::ConvertStyleMonCurve(NegativeStyle negStyle, TransformDirection dir)
{
    const bool isForward = dir == TRANSFORM_DIR_FORWARD;

    switch (negStyle)
    {
    case NEGATIVE_LINEAR:
    {
        return (isForward) ? GammaOpData::MONCURVE_FWD :
                             GammaOpData::MONCURVE_REV;
    }
    case NEGATIVE_MIRROR:
    {
        return (isForward) ? GammaOpData::MONCURVE_MIRROR_FWD :
                             GammaOpData::MONCURVE_MIRROR_REV;
    }
    case NEGATIVE_PASS_THRU:
    {
        throw Exception("Pass thru negative extrapolation is not valid "
                        "for MonCurve exponent style.");
    }
    case NEGATIVE_CLAMP:
    {
        throw Exception("Clamp negative extrapolation is not valid for MonCurve exponent style.");
    }
    }

    std::stringstream ss("Unknown negative extrapolation style: ");
    ss << negStyle;

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

    gamma->invert();

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

    // It's possible that some combinations such as BASIC_PASS_THRU_FWD and BASIC_REV will become
    // an identity after being combined and removed later.  We need to do it that way (rather than
    // here) since if isInverse is true, the optimizer calls getIdentityReplacement on only the
    // first of the pair of ops.

    if ( (styleA == BASIC_FWD && styleB == BASIC_REV) ||
         (styleA == BASIC_REV && styleB == BASIC_FWD) ||
         (styleA == MONCURVE_FWD && styleB == MONCURVE_REV) ||
         (styleA == MONCURVE_REV && styleB == MONCURVE_FWD) ||
         (styleA == MONCURVE_MIRROR_FWD && styleB == MONCURVE_MIRROR_REV) ||
         (styleA == MONCURVE_MIRROR_REV && styleB == MONCURVE_MIRROR_FWD) ||
         (styleA == BASIC_MIRROR_FWD && styleB == BASIC_MIRROR_REV) ||
         (styleA == BASIC_MIRROR_REV && styleB == BASIC_MIRROR_FWD) ||
         (styleA == BASIC_PASS_THRU_FWD && styleB == BASIC_PASS_THRU_REV) ||
         (styleA == BASIC_PASS_THRU_REV && styleB == BASIC_PASS_THRU_FWD) )
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

void GammaOpData::setStyle(const Style & style) noexcept
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
        case BASIC_MIRROR_FWD:
        case BASIC_MIRROR_REV:
        case BASIC_PASS_THRU_FWD:
        case BASIC_PASS_THRU_REV:
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
        case MONCURVE_MIRROR_FWD:
        case MONCURVE_MIRROR_REV:
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
        case BASIC_MIRROR_FWD:
        case BASIC_MIRROR_REV:
        case BASIC_PASS_THRU_FWD:
        case BASIC_PASS_THRU_REV:
        {
            params.push_back(IdentityScale);
            break;
        }
        case MONCURVE_FWD:
        case MONCURVE_REV:
        case MONCURVE_MIRROR_FWD:
        case MONCURVE_MIRROR_REV:
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
        case BASIC_MIRROR_FWD:
        case BASIC_MIRROR_REV:
        case BASIC_PASS_THRU_FWD:
        case BASIC_PASS_THRU_REV:
        {
            return (parameters.size() == 1 && IsBasicIdentity(parameters));
        }
        case MONCURVE_FWD:
        case MONCURVE_REV:
        case MONCURVE_MIRROR_FWD:
        case MONCURVE_MIRROR_REV:
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
        case BASIC_MIRROR_FWD:
        case BASIC_MIRROR_REV:
        case BASIC_PASS_THRU_FWD:
        case BASIC_PASS_THRU_REV:
        {
            if (areAllComponentsEqual() && IsBasicIdentity(m_redParams) )
            {
                return true;
            }
            break;
        }

        case MONCURVE_FWD:
        case MONCURVE_REV:
        case MONCURVE_MIRROR_FWD:
        case MONCURVE_MIRROR_REV:
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
    // NB: This also does not check bypass or dynamic.

    const auto styleA = getStyle();
    const auto styleB = B.getStyle();

    if (styleA == BASIC_FWD || styleA == BASIC_REV)
    {
        switch (styleB)
        {
        case BASIC_FWD:
        case BASIC_REV:
        case BASIC_MIRROR_FWD:
        case BASIC_MIRROR_REV:
        case BASIC_PASS_THRU_FWD:
        case BASIC_PASS_THRU_REV:
            return true;
        case MONCURVE_FWD:
        case MONCURVE_REV:
        case MONCURVE_MIRROR_FWD:
        case MONCURVE_MIRROR_REV:
            return false;
        }
    }
    else if (styleA == BASIC_MIRROR_FWD || styleA == BASIC_MIRROR_REV)
    {
        switch (styleB)
        {
        case BASIC_FWD:
        case BASIC_REV:
        case BASIC_MIRROR_FWD:
        case BASIC_MIRROR_REV:
            return true;
        case BASIC_PASS_THRU_FWD:
        case BASIC_PASS_THRU_REV:
        case MONCURVE_FWD:
        case MONCURVE_REV:
        case MONCURVE_MIRROR_FWD:
        case MONCURVE_MIRROR_REV:
            return false;
        }
    }
    else if (styleA == BASIC_PASS_THRU_FWD || styleA == BASIC_PASS_THRU_REV)
    {
        switch (styleB)
        {
        case BASIC_FWD:
        case BASIC_REV:
        case BASIC_PASS_THRU_FWD:
        case BASIC_PASS_THRU_REV:
            return true;
        case BASIC_MIRROR_FWD:
        case BASIC_MIRROR_REV:
        case MONCURVE_FWD:
        case MONCURVE_REV:
        case MONCURVE_MIRROR_FWD:
        case MONCURVE_MIRROR_REV:
            return false;
        }
    }

    return false;
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
        case BASIC_MIRROR_FWD:
        case BASIC_MIRROR_REV:
        case BASIC_PASS_THRU_FWD:
        case BASIC_PASS_THRU_REV:
        case MONCURVE_FWD:
        case MONCURVE_REV:
        case MONCURVE_MIRROR_FWD:
        case MONCURVE_MIRROR_REV:
        {
            op = std::make_shared<MatrixOpData>();
            break;
        }
    }
    return op;
}

namespace
{
GammaOpData::Style CombineBasicStyles(GammaOpData::Style a, GammaOpData::Style b)
{
    // This function assumes that mayCompose was called on the inputs and returned true.
    // The logic here is only valid for that situation.
    if (a == GammaOpData::BASIC_FWD || a == GammaOpData::BASIC_REV ||
        b == GammaOpData::BASIC_FWD || b == GammaOpData::BASIC_REV)
    {
        return GammaOpData::BASIC_FWD;
    }
    else if (a == GammaOpData::BASIC_MIRROR_FWD || a == GammaOpData::BASIC_MIRROR_REV)
    {
        // b BASIC_MIRROR.
        return GammaOpData::BASIC_MIRROR_FWD;
    }
    else // a is BASIC_PASS_THRU (ensured by mayCompose).
    {
        // b BASIC_PASS_THRU.
        return GammaOpData::BASIC_PASS_THRU_FWD;
    }
}

GammaOpData::Style InverseBasicStyle(GammaOpData::Style style)
{
    if (style == GammaOpData::BASIC_PASS_THRU_FWD)
    {
        return GammaOpData::BASIC_PASS_THRU_REV;
    }
    else if (style == GammaOpData::BASIC_MIRROR_FWD)
    {
        return GammaOpData::BASIC_MIRROR_REV;
    }
    return GammaOpData::BASIC_REV;
}

void RoundAround1(double & val)
{
    const double diff = fabs(val - 1.);
    if (diff < 1e-6)
    {
        val = 1.;
    }
}
}

GammaOpDataRcPtr GammaOpData::compose(const GammaOpData & B) const
{
    if ( ! mayCompose(B) )
    {
        throw Exception("GammaOp can only be combined with some GammaOps");
    }

    const auto styleA = getStyle();
    const auto styleB = B.getStyle();

    double r1 = getRedParams()[0];
    double g1 = getGreenParams()[0];
    double b1 = getBlueParams()[0];
    double a1 = getAlphaParams()[0];

    if (styleA == BASIC_REV || styleA == BASIC_MIRROR_REV || styleA == BASIC_PASS_THRU_FWD)
    {
        r1 = 1. / r1;
        g1 = 1. / g1;
        b1 = 1. / b1;
        a1 = 1. / a1;
    }

    double r2 = B.getRedParams()[0];
    double g2 = B.getGreenParams()[0];
    double b2 = B.getBlueParams()[0];
    double a2 = B.getAlphaParams()[0];
    if (styleB == BASIC_REV || styleB == BASIC_MIRROR_REV || styleB == BASIC_PASS_THRU_FWD)
    {
        r2 = 1. / r2;
        g2 = 1. / g2;
        b2 = 1. / b2;
        a2 = 1. / a2;
    }

    double rOut = r1 * r2;
    double gOut = g1 * g2;
    double bOut = b1 * b2;
    double aOut = a1 * a2;

    // Prevent small rounding errors from not making an identity.
    // E.g., 1/0.45 * 0.45 should have a value exactly 1.
    RoundAround1(rOut);
    RoundAround1(gOut);
    RoundAround1(bOut);
    RoundAround1(aOut);

    Style style = CombineBasicStyles(styleA, styleB);

    // By convention, we try to keep the gamma parameter > 1.
    if (rOut < 1.0 && gOut < 1.0 && bOut < 1.0)
    {
        rOut = 1. / rOut;
        gOut = 1. / gOut;
        bOut = 1. / bOut;
        aOut = 1. / aOut;
        style = InverseBasicStyle(style);
    }

    GammaOpData::Params paramsR(1, rOut);
    GammaOpData::Params paramsG(1, gOut);
    GammaOpData::Params paramsB(1, bOut);
    GammaOpData::Params paramsA(1, aOut);

    GammaOpDataRcPtr outOp = std::make_shared<GammaOpData>(style,
                                                           paramsR,
                                                           paramsG,
                                                           paramsB,
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

std::string GammaOpData::getCacheID() const
{
    AutoMutex lock(m_mutex);

    std::ostringstream cacheIDStream;
    if (!getID().empty())
    {
        cacheIDStream << getID() << " ";
    }

    cacheIDStream << GammaOpData::ConvertStyleToString(getStyle()) << " ";

    cacheIDStream << "r:" << GetParametersString(getRedParams())   << " ";
    cacheIDStream << "g:" << GetParametersString(getGreenParams()) << " ";
    cacheIDStream << "b:" << GetParametersString(getBlueParams())  << " ";
    cacheIDStream << "a:" << GetParametersString(getAlphaParams()) << " ";

    return cacheIDStream.str();
}

TransformDirection GammaOpData::getDirection() const noexcept
{
    switch (m_style)
    {
    case BASIC_FWD:
    case BASIC_MIRROR_FWD:
    case BASIC_PASS_THRU_FWD:
    case MONCURVE_FWD:
    case MONCURVE_MIRROR_FWD:
        return TRANSFORM_DIR_FORWARD;

    case BASIC_REV:
    case BASIC_MIRROR_REV:
    case BASIC_PASS_THRU_REV:
    case MONCURVE_REV:
    case MONCURVE_MIRROR_REV:
        return TRANSFORM_DIR_INVERSE;
    }

    return TRANSFORM_DIR_FORWARD;
}

void GammaOpData::setDirection(TransformDirection dir) noexcept
{
    if (getDirection() != dir)
    {
        invert();
    }
}

void GammaOpData::invert() noexcept
{
    Style invStyle = BASIC_FWD;
    switch (getStyle())
    {
    case BASIC_FWD:           invStyle = BASIC_REV;           break;
    case BASIC_REV:           invStyle = BASIC_FWD;           break;
    case BASIC_MIRROR_FWD:    invStyle = BASIC_MIRROR_REV;    break;
    case BASIC_MIRROR_REV:    invStyle = BASIC_MIRROR_FWD;    break;
    case BASIC_PASS_THRU_FWD: invStyle = BASIC_PASS_THRU_REV; break;
    case BASIC_PASS_THRU_REV: invStyle = BASIC_PASS_THRU_FWD; break;
    case MONCURVE_FWD:        invStyle = MONCURVE_REV;        break;
    case MONCURVE_REV:        invStyle = MONCURVE_FWD;        break;
    case MONCURVE_MIRROR_FWD: invStyle = MONCURVE_MIRROR_REV; break;
    case MONCURVE_MIRROR_REV: invStyle = MONCURVE_MIRROR_FWD; break;
    }
    setStyle(invStyle);
}

} // namespace OCIO_NAMESPACE


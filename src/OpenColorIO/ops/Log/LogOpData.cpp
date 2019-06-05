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

#include <algorithm>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "MathUtils.h"
#include "ops/Log/LogOpData.h"
#include "ops/Log/LogUtils.h"
#include "ops/Matrix/MatrixOpData.h"
#include "ops/Range/RangeOpData.h"
#include "Platform.h"

OCIO_NAMESPACE_ENTER
{


namespace DefaultValues
{
static const double logSlope[3]  = { 1.0, 1.0, 1.0 };
static const double linSlope[3]  = { 1.0, 1.0, 1.0 };
static const double linOffset[3]  = { 0.0, 0.0, 0.0 };
static const double logOffset[3] = { 0.0, 0.0, 0.0 };
const std::streamsize FLOAT_DECIMALS = 7;
}

namespace
{
// Validate number of parameters and their respective range and value.
void ValidateParams(const LogOpData::Params & params, TransformDirection direction)
{
    const size_t expectedSize = 4;
    if (params.size() != expectedSize)
    {
        throw Exception("Log: expecting 4 parameters.");
    }

    if (direction != TRANSFORM_DIR_UNKNOWN)
    {

        if (IsScalarEqualToZero(params[LIN_SIDE_SLOPE]))
        {
            std::ostringstream oss;
            oss << "Log: Invalid linear slope value '";
            oss << params[LIN_SIDE_SLOPE];
            oss << "', linear slope cannot be 0.";
            throw Exception(oss.str().c_str());
        }
        if (IsScalarEqualToZero(params[LOG_SIDE_SLOPE]))
        {
            std::ostringstream oss;
            oss << "Log: Invalid log slope value '";
            oss << params[LOG_SIDE_SLOPE];
            oss << "', log slope cannot be 0.";
            throw Exception(oss.str().c_str());
        }
    }
    else
    {
        throw Exception("Log: Invalid direction.");
    }
}
}

LogOpData::LogOpData(double base, TransformDirection direction)
    : OpData(BIT_DEPTH_F32, BIT_DEPTH_F32)
    , m_base(base)
    , m_direction(direction)
{
    if (m_direction == TRANSFORM_DIR_UNKNOWN)
    {
        throw Exception("Cannot create Log op, unspecified transform direction.");
    }

    setParameters(DefaultValues::logSlope, DefaultValues::logOffset,
                  DefaultValues::linSlope, DefaultValues::linOffset);
}

LogOpData::LogOpData(double base,
                     const double(&logSlope)[3],
                     const double(&logOffset)[3],
                     const double(&linSlope)[3],
                     const double(&linOffset)[3],
                     TransformDirection direction)
    : OpData(BIT_DEPTH_F32, BIT_DEPTH_F32)
    , m_base(base)
    , m_direction(direction)
{
    if (m_direction == TRANSFORM_DIR_UNKNOWN)
    {
        throw Exception("Cannot create Log op, unspecified transform direction.");
    }
    setParameters(logSlope, logOffset, linSlope, linOffset);
}

LogOpData::LogOpData(BitDepth inBitDepth,
                     BitDepth outBitDepth,
                     TransformDirection dir,
                     double base,
                     const Params & redParams,
                     const Params & greenParams,
                     const Params & blueParams)
    : OpData(inBitDepth, outBitDepth)
    , m_redParams(redParams)
    , m_greenParams(greenParams)
    , m_blueParams(blueParams)
    , m_base(base)
    , m_direction(dir)
{
    if (m_direction == TRANSFORM_DIR_UNKNOWN)
    {
        throw Exception("Cannot create Log op, unspecified transform direction.");
    }
}

void LogOpData::setBase(double base)
{
    m_base = base;
}

double LogOpData::getBase() const
{
    return m_base;
}

void LogOpData::setValue(LogAffineParameter val, const double(&values)[3])
{
    m_redParams[val]   = values[0];
    m_greenParams[val] = values[1];
    m_blueParams[val]  = values[2];

}

void LogOpData::getValue(LogAffineParameter val, double(&values)[3]) const
{
    values[0] = m_redParams[val];
    values[1] = m_greenParams[val];
    values[2] = m_blueParams[val];
}

void LogOpData::setParameters(const double(&logSlope)[3],
                              const double(&logOffset)[3],
                              const double(&linSlope)[3],
                              const double(&linOffset)[3])
{
    m_redParams.resize(4);
    m_greenParams.resize(4);
    m_blueParams.resize(4);

    setValue(LOG_SIDE_SLOPE, logSlope);
    setValue(LOG_SIDE_OFFSET, logOffset);
    setValue(LIN_SIDE_SLOPE, linSlope);
    setValue(LIN_SIDE_OFFSET, linOffset);
}

void LogOpData::getParameters(double(&logSlope)[3],
                              double(&logOffset)[3],
                              double(&linSlope)[3],
                              double(&linOffset)[3]) const
{
    getValue(LOG_SIDE_SLOPE, logSlope);
    getValue(LOG_SIDE_OFFSET, logOffset);
    getValue(LIN_SIDE_SLOPE, linSlope);
    getValue(LIN_SIDE_OFFSET, linOffset);
}

LogOpData::~LogOpData()
{
}

void LogOpData::validate() const
{
    OpData::validate();

    ValidateParams(m_redParams, m_direction);
    ValidateParams(m_greenParams, m_direction);
    ValidateParams(m_blueParams, m_direction);

    if (m_base == 1.0)
    {
        std::ostringstream oss;
        oss << "Log: Invalid base value '";
        oss << m_base;
        oss << "', base cannot be 1.";
        throw Exception(oss.str().c_str());
    }
    else if (m_base <= 0.0)
    {
        std::ostringstream oss;
        oss << "Log: Invalid base value '";
        oss << m_base;
        oss << "', base must be greater than 0.";
        throw Exception(oss.str().c_str());
    }
}

bool LogOpData::isIdentity() const
{
    return false;
}

OpDataRcPtr LogOpData::getIdentityReplacement() const
{
    OpDataRcPtr resOp;
    if (isLog2() || isLog10())
    {
        if (m_direction == TRANSFORM_DIR_FORWARD)
        {
            // The first op logarithm is not defined for negative values.
            resOp = std::make_shared<RangeOpData>(
                BIT_DEPTH_F32, BIT_DEPTH_F32,
                0.,
                // Don't clamp high end.
                RangeOpData::EmptyValue(),
                0.,
                RangeOpData::EmptyValue());
        }
        else
        {
            // In principle, the power function is defined over the entire domain.
            // However, in practice the input to the following logarithm is clamped
            // to a very small positive number and this imposes a limit.
            // E.g., log10(FLOAT_MIN) = -37.93, but this is so small that it makes
            // more sense to consider it an exact inverse.
            resOp = std::make_shared<MatrixOpData>(
                getInputBitDepth(), getOutputBitDepth());
        }
    }
    else
    {
        if (m_direction == TRANSFORM_DIR_FORWARD)
        {
            // Minimum value allowed is -linOffset/linSlope so that linSlope*x+linOffset > 0.
            const double minValue = -m_redParams[LIN_SIDE_OFFSET] / m_redParams[LIN_SIDE_SLOPE];
            resOp = std::make_shared<RangeOpData>(BIT_DEPTH_F32, BIT_DEPTH_F32,
                                                  minValue,
                                                  // Don't clamp high end.
                                                  RangeOpData::EmptyValue(),
                                                  minValue,
                                                  RangeOpData::EmptyValue());

        }
        else
        {
            resOp = std::make_shared<MatrixOpData>(
                getInputBitDepth(), getOutputBitDepth());
        }
    }

    // The argument is the first half of the identity pair, so set the in-depth.
    // The caller must set the output depth to match the second half.
    resOp->setInputBitDepth(getInputBitDepth());
    return resOp;
}

bool LogOpData::isNoOp() const
{
    return false;
}

void LogOpData::finalize()
{
    AutoMutex lock(m_mutex);

    std::ostringstream cacheIDStream;
    cacheIDStream << getID() << " ";

    cacheIDStream << TransformDirectionToString(m_direction) << " ";

    cacheIDStream << "Base "         << getBaseString(DefaultValues::FLOAT_DECIMALS) << " ";
    cacheIDStream << "LogSlope "     << getLogSlopeString(DefaultValues::FLOAT_DECIMALS) << " ";
    cacheIDStream << "LogOffset "    << getLogOffsetString(DefaultValues::FLOAT_DECIMALS) << " ";
    cacheIDStream << "LinearSlope "  << getLinSlopeString(DefaultValues::FLOAT_DECIMALS) << " ";
    cacheIDStream << "LinearOffset " << getLinOffsetString(DefaultValues::FLOAT_DECIMALS);

    m_cacheID = cacheIDStream.str();
}

bool LogOpData::operator==(const OpData& other) const
{
    if (this == &other) return true;

    if (!OpData::operator==(other)) return false;

    const LogOpData* log = static_cast<const LogOpData*>(&other);

    return (m_direction == log->m_direction
            && m_base == log->m_base
            && m_redParams == log->m_redParams
            && m_greenParams == log->m_greenParams
            && m_blueParams == log->m_blueParams);
}

LogOpDataRcPtr LogOpData::clone() const
{
    return std::make_shared<LogOpData>(getInputBitDepth(),
                                       getOutputBitDepth(),
                                       m_direction,
                                       getBase(),
                                       getRedParams(),
                                       getGreenParams(),
                                       getBlueParams());
}

LogOpDataRcPtr LogOpData::inverse() const
{
    LogOpDataRcPtr invOp = std::make_shared<LogOpData>(
        getOutputBitDepth(),
        getInputBitDepth(),
        GetInverseTransformDirection(m_direction),
        getBase(),
        getRedParams(),
        getGreenParams(),
        getBlueParams());

    invOp->validate();

    return invOp;
}

bool LogOpData::isInverse(ConstLogOpDataRcPtr & r) const
{
    if (GetInverseTransformDirection(m_direction) == r->m_direction
        && allComponentsEqual() && r->allComponentsEqual()
        && getRedParams() == r->getRedParams()
        && getBase() == r->getBase())
    {
        return true;
    }
 
    // Note:  Actually the R/G/B channels would not need to be equal for an
    // inverse, however, the identity replacement would get more complicated if
    // we allowed that case.  Since it is not a typical use-case, we don't
    // consider it an inverse since it is not easy to optimize out.
    return false;
}

bool LogOpData::allComponentsEqual() const
{
    // Comparing doubles is generally not a good idea, but in this case
    // it is ok to be strict.  Since the same operations are applied to
    // all components, if they started equal, they should remain equal.
    return m_redParams == m_greenParams && m_redParams == m_blueParams;
}

template <int index>
std::string getParameterString(const LogOpData & log, std::streamsize precision)
{
    static_assert(index >= 0 && index < 4, "Index has to be in [0..3]");
    std::ostringstream o;
    o.precision(precision);

    if (log.allComponentsEqual())
    {
        o << log.getRedParams()[index];
    }
    else
    {
        o << log.getRedParams()[index] << ", ";
        o << log.getGreenParams()[index] << ", ";
        o << log.getBlueParams()[index];
    }
    return o.str();
}

std::string LogOpData::getBaseString(std::streamsize precision) const
{
    std::ostringstream o;
    o.precision(precision);
    o << getBase();
    return o.str();
}

std::string LogOpData::getLogSlopeString(std::streamsize precision) const
{
    return getParameterString<LOG_SIDE_SLOPE>(*this, precision);
}

std::string LogOpData::getLinSlopeString(std::streamsize precision) const
{
    return getParameterString<LIN_SIDE_SLOPE>(*this, precision);
}

std::string LogOpData::getLinOffsetString(std::streamsize precision) const
{
    return getParameterString<LIN_SIDE_OFFSET>(*this, precision);
}

std::string LogOpData::getLogOffsetString(std::streamsize precision) const
{
    return getParameterString<LOG_SIDE_OFFSET>(*this, precision);
}

bool LogOpData::isLogBase(double base) const
{
    if (allComponentsEqual())
    {
        if (m_redParams[LOG_SIDE_SLOPE] == 1.0
            && m_redParams[LIN_SIDE_SLOPE] == 1.0
            && m_redParams[LIN_SIDE_OFFSET] == 0.0
            && m_redParams[LOG_SIDE_OFFSET] == 0.0
            && m_base == base)
        {
            return true;
        }
    }
    return false;
}

bool LogOpData::isLog2() const
{
    return isLogBase(2.0);
}

bool LogOpData::isLog10() const
{
    return isLogBase(10.0);
}

}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include "unittest.h"

OIIO_ADD_TEST(LogOpData, accessor_test)
{
    OCIO::LogUtil::CTFParams ctfParams;
    auto & redP = ctfParams.get(OCIO::LogUtil::CTFParams::red);
    redP[OCIO::LogUtil::CTFParams::gamma]     = 2.4;
    redP[OCIO::LogUtil::CTFParams::refWhite]  = 410.;
    redP[OCIO::LogUtil::CTFParams::refBlack]  = 256.;
    redP[OCIO::LogUtil::CTFParams::highlight] = 0.2; 
    redP[OCIO::LogUtil::CTFParams::shadow]    = 0.1; 

    auto & greenP = ctfParams.get(OCIO::LogUtil::CTFParams::green);
    greenP[OCIO::LogUtil::CTFParams::gamma]     = 3.5; 
    greenP[OCIO::LogUtil::CTFParams::refWhite]  = 620.;
    greenP[OCIO::LogUtil::CTFParams::refBlack]  = 485.;
    greenP[OCIO::LogUtil::CTFParams::highlight] = 0.7; 
    greenP[OCIO::LogUtil::CTFParams::shadow]    = 0.6; 

    auto & blueP = ctfParams.get(OCIO::LogUtil::CTFParams::blue);
    blueP[OCIO::LogUtil::CTFParams::gamma]     = 4.6; 
    blueP[OCIO::LogUtil::CTFParams::refWhite]  = 730.;
    blueP[OCIO::LogUtil::CTFParams::refBlack]  = 558.;
    blueP[OCIO::LogUtil::CTFParams::highlight] = 0.9; 
    blueP[OCIO::LogUtil::CTFParams::shadow]    = 0.7; 

    ctfParams.m_style = OCIO::LogUtil::LOG_TO_LIN;

    OCIO::LogOpData::Params paramsR, paramsG, paramsB;
    double base = 1.0;
    OCIO::TransformDirection dir;
    OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB, dir);

    OCIO::BitDepth inBitDepth = OCIO::BIT_DEPTH_UINT8;
    OCIO::BitDepth outBitDepth = OCIO::BIT_DEPTH_F16;
    OCIO::LogOpData logOp(inBitDepth, outBitDepth, dir,
                          base, paramsR, paramsG, paramsB);

    OIIO_CHECK_EQUAL(logOp.getType(), OCIO::OpData::LogType);
    OIIO_CHECK_EQUAL(logOp.getInputBitDepth(), inBitDepth);
    OIIO_CHECK_EQUAL(logOp.getOutputBitDepth(), outBitDepth);

    OIIO_CHECK_ASSERT(!logOp.allComponentsEqual());
    OIIO_CHECK_EQUAL(logOp.getBase(), base);
    OIIO_CHECK_ASSERT(logOp.getRedParams() == paramsR);
    OIIO_CHECK_ASSERT(logOp.getGreenParams() == paramsG);
    OIIO_CHECK_ASSERT(logOp.getBlueParams() == paramsB);

    // Update all channels with same parameters.
    greenP = redP;
    blueP = redP;
    OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB, dir);

    OCIO::LogOpData logOp2(inBitDepth, outBitDepth, dir,
                           base, paramsR, paramsG, paramsB);

    OIIO_CHECK_ASSERT(logOp2.allComponentsEqual());
    OIIO_CHECK_ASSERT(logOp2.getRedParams() == paramsR);
    OIIO_CHECK_ASSERT(logOp2.getGreenParams() == paramsR);
    OIIO_CHECK_ASSERT(logOp2.getBlueParams() == paramsR);

    // Update only red channel with new parameters.
    redP[OCIO::LogUtil::CTFParams::gamma]     = 0.6; 
    redP[OCIO::LogUtil::CTFParams::refWhite]  = 358.;
    redP[OCIO::LogUtil::CTFParams::refBlack]  = 115.;
    redP[OCIO::LogUtil::CTFParams::highlight] = 0.7; 
    redP[OCIO::LogUtil::CTFParams::shadow]    = 0.3; 

    OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB, dir);

    OCIO::LogOpData logOp3(inBitDepth, outBitDepth, dir,
                           base, paramsR, paramsG, paramsB);

    OIIO_CHECK_ASSERT(!logOp3.allComponentsEqual());
    OIIO_CHECK_ASSERT(logOp3.getRedParams() == paramsR);
    OIIO_CHECK_ASSERT(logOp3.getGreenParams() == paramsG);
    OIIO_CHECK_ASSERT(logOp3.getBlueParams() == paramsB);

    // Update only green channel with new parameters.
    redP = greenP;
    greenP[OCIO::LogUtil::CTFParams::gamma]     = 0.3;  
    greenP[OCIO::LogUtil::CTFParams::refWhite]  = 333.; 
    greenP[OCIO::LogUtil::CTFParams::refBlack]  = 155.; 
    greenP[OCIO::LogUtil::CTFParams::highlight] = 0.85; 
    greenP[OCIO::LogUtil::CTFParams::shadow]    = 0.111;

    OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB, dir);

    OCIO::LogOpData logOp4(inBitDepth, outBitDepth, dir,
                           base, paramsR, paramsG, paramsB);
    OIIO_CHECK_ASSERT(!logOp4.allComponentsEqual());
    OIIO_CHECK_ASSERT(logOp4.getRedParams() == paramsR);
    OIIO_CHECK_ASSERT(logOp4.getGreenParams() == paramsG);
    OIIO_CHECK_ASSERT(logOp4.getBlueParams() == paramsB);

    // Update only blue channel with new parameters.
    greenP = redP;
    blueP[OCIO::LogUtil::CTFParams::gamma]     = 0.124;
    blueP[OCIO::LogUtil::CTFParams::refWhite]  = 55.;  
    blueP[OCIO::LogUtil::CTFParams::refBlack]  = 33.;  
    blueP[OCIO::LogUtil::CTFParams::highlight] = 0.27; 
    blueP[OCIO::LogUtil::CTFParams::shadow]    = 0.22; 

    OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB, dir);

    OCIO::LogOpData logOp5(inBitDepth, outBitDepth, dir,
                           base, paramsR, paramsG, paramsB);
    OIIO_CHECK_ASSERT(!logOp5.allComponentsEqual());
    OIIO_CHECK_ASSERT(logOp5.getRedParams() == paramsR);
    OIIO_CHECK_ASSERT(logOp5.getGreenParams() == paramsG);
    OIIO_CHECK_ASSERT(logOp5.getBlueParams() == paramsB);

    // Initialize with base.
    const double baseVal = 2.0;
    OCIO::LogOpData logOp6(baseVal, OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_ASSERT(logOp6.allComponentsEqual());
    auto & param = logOp6.getRedParams();
    OIIO_CHECK_EQUAL(logOp6.getBase(), baseVal);
    OIIO_CHECK_EQUAL(param[OCIO::LOG_SIDE_SLOPE], 1.0f);
    OIIO_CHECK_EQUAL(param[OCIO::LIN_SIDE_SLOPE], 1.0f);
    OIIO_CHECK_EQUAL(param[OCIO::LIN_SIDE_OFFSET], 0.0f);
    OIIO_CHECK_EQUAL(param[OCIO::LOG_SIDE_OFFSET], 0.0f);

    // Initialize with OCIO parameters.
    const double logSlope[] = { 1.5, 1.6, 1.7 };
    const double linSlope[] = { 1.1, 1.2, 1.3 };
    const double linOffset[] = { 1.0, 2.0, 3.0 };
    const double logOffset[] = { 10.0, 20.0, 30.0 };

    OCIO::LogOpData logOp7(base, logSlope, logOffset, linSlope, linOffset, OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_ASSERT(!logOp7.allComponentsEqual());
    auto & paramR = logOp7.getRedParams();
    OIIO_CHECK_EQUAL(logOp7.getBase(), base);
    OIIO_CHECK_EQUAL(paramR[OCIO::LOG_SIDE_SLOPE], logSlope[0]);
    OIIO_CHECK_EQUAL(paramR[OCIO::LIN_SIDE_SLOPE], linSlope[0]);
    OIIO_CHECK_EQUAL(paramR[OCIO::LIN_SIDE_OFFSET], linOffset[0]);
    OIIO_CHECK_EQUAL(paramR[OCIO::LOG_SIDE_OFFSET], logOffset[0]);
    auto & paramG = logOp7.getGreenParams();
    OIIO_CHECK_EQUAL(paramG[OCIO::LOG_SIDE_SLOPE], logSlope[1]);
    OIIO_CHECK_EQUAL(paramG[OCIO::LIN_SIDE_SLOPE], linSlope[1]);
    OIIO_CHECK_EQUAL(paramG[OCIO::LIN_SIDE_OFFSET], linOffset[1]);
    OIIO_CHECK_EQUAL(paramG[OCIO::LOG_SIDE_OFFSET], logOffset[1]);
    auto & paramB = logOp7.getBlueParams();
    OIIO_CHECK_EQUAL(paramB[OCIO::LOG_SIDE_SLOPE], logSlope[2]);
    OIIO_CHECK_EQUAL(paramB[OCIO::LIN_SIDE_SLOPE], linSlope[2]);
    OIIO_CHECK_EQUAL(paramB[OCIO::LIN_SIDE_OFFSET], linOffset[2]);
    OIIO_CHECK_EQUAL(paramB[OCIO::LOG_SIDE_OFFSET], logOffset[2]);
}

OIIO_ADD_TEST(LogOpData, validation_fails_test)
{
    double base = 1.0;
    double logSlope[] = { 1.0, 1.0, 1.0 };
    double linSlope[] = { 1.0, 1.0, 1.0 };
    double linOffset[] = { 0.0, 0.0, 0.0 };
    double logOffset[] = { 0.0, 0.0, 0.0 };
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;
    
    // Fail invalid base.
    OCIO::LogOpData logOp1(base, logSlope, logOffset, linSlope, linOffset, direction);
    OIIO_CHECK_THROW_WHAT(logOp1.validate(), OCIO::Exception, "base cannot be 1");
    direction = OCIO::TRANSFORM_DIR_INVERSE;
    OCIO::LogOpData invlogOp1(base, logSlope, logOffset, linSlope, linOffset, direction);
    OIIO_CHECK_THROW_WHAT(invlogOp1.validate(), OCIO::Exception, "base cannot be 1");

    base = 10.0;

    // Fail invalid slope.
    direction = OCIO::TRANSFORM_DIR_FORWARD;
    linSlope[0] = linSlope[1] = linSlope[2] = 0.0;

    OCIO::LogOpData logOp2(base, logSlope, logOffset, linSlope, linOffset, direction);
    OIIO_CHECK_THROW_WHAT(logOp2.validate(), OCIO::Exception, "linear slope cannot be 0");
    direction = OCIO::TRANSFORM_DIR_INVERSE;
    OCIO::LogOpData invlogOp2(base, logSlope, logOffset, linSlope, linOffset, direction);
    OIIO_CHECK_THROW_WHAT(invlogOp2.validate(), OCIO::Exception, "linear slope cannot be 0");

    linSlope[0] = linSlope[1] = linSlope[2] = 1.0;

    // Fail invalid multiplier.
    direction = OCIO::TRANSFORM_DIR_FORWARD;
    logSlope[0] = logSlope[1] = logSlope[2] = 0.0;
    
    OCIO::LogOpData logOp3(base, logSlope, logOffset, linSlope, linOffset, direction);
    OIIO_CHECK_THROW_WHAT(logOp3.validate(), OCIO::Exception, "log slope cannot be 0");
    direction = OCIO::TRANSFORM_DIR_INVERSE;
    OCIO::LogOpData invlogOp3(base, logSlope, logOffset, linSlope, linOffset, direction);
    OIIO_CHECK_THROW_WHAT(invlogOp3.validate(), OCIO::Exception, "log slope cannot be 0");
}

OIIO_ADD_TEST(LogOpData, log_inverse)
{
    OCIO::LogOpData::Params paramR{ 1.5, 10.0, 1.1, 1.0 };
    OCIO::LogOpData::Params paramG{ 1.6, 20.0, 1.2, 2.0 };
    OCIO::LogOpData::Params paramB{ 1.7, 30.0, 1.3, 3.0 };
    const double base = 10.0;

    OCIO::LogOpData logOp0(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_UINT10, OCIO::TRANSFORM_DIR_FORWARD,
                           base, paramR, paramG, paramB);
    OCIO::ConstLogOpDataRcPtr invLogOp0 = logOp0.inverse();

    OIIO_CHECK_ASSERT(logOp0.getRedParams() == invLogOp0->getRedParams());
    OIIO_CHECK_ASSERT(logOp0.getGreenParams() == invLogOp0->getGreenParams());
    OIIO_CHECK_ASSERT(logOp0.getBlueParams() == invLogOp0->getBlueParams());

    OIIO_CHECK_EQUAL(logOp0.getInputBitDepth(), invLogOp0->getOutputBitDepth());
    OIIO_CHECK_EQUAL(logOp0.getOutputBitDepth(), invLogOp0->getInputBitDepth());

    // When components are not equals, ops are not considered inverse.
    OIIO_CHECK_ASSERT(!logOp0.isInverse(invLogOp0));

    // Using equal components.
    OCIO::LogOpData logOp1(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_UINT10, OCIO::TRANSFORM_DIR_FORWARD,
                           base, paramR, paramR, paramR);
    OCIO::ConstLogOpDataRcPtr invLogOp1 = logOp1.inverse();

    OIIO_CHECK_ASSERT(logOp1.isInverse(invLogOp1));

}

OIIO_ADD_TEST(LogOpData, identity_replacement)
{
    OCIO::LogOpData::Params paramsR{ 1.5, 10.0, 2.0, 1.0 };
    const double base = 2.0;
    OCIO::BitDepth inBitDepth = OCIO::BIT_DEPTH_UINT8;
    OCIO::BitDepth outBitDepth = OCIO::BIT_DEPTH_F16;

    {
        OCIO::LogOpData logOp(inBitDepth, outBitDepth,
                              OCIO::TRANSFORM_DIR_INVERSE,
                              base, paramsR, paramsR, paramsR);
        OIIO_CHECK_EQUAL(logOp.getIdentityReplacement()->getType(),
                         OCIO::OpData::MatrixType);
    }
    {
        OCIO::LogOpData logOp(OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_F32,
                              OCIO::TRANSFORM_DIR_FORWARD,
                              base, paramsR, paramsR, paramsR);
        auto op = logOp.getIdentityReplacement();
        OIIO_CHECK_EQUAL(op->getType(),
                         OCIO::OpData::RangeType);
        auto r = std::dynamic_pointer_cast<OCIO::RangeOpData>(op);
        // -32767.5 = -(1.0/2.0) * 65535
        OIIO_CHECK_EQUAL((int)(r->getMinInValue()), (int)-32767.5);
        OIIO_CHECK_ASSERT(r->maxIsEmpty());
    }

    {
        OCIO::LogOpData logOp(2.0f, OCIO::TRANSFORM_DIR_FORWARD);
        OIIO_CHECK_EQUAL(logOp.getIdentityReplacement()->getType(),
                         OCIO::OpData::RangeType);
    }
    {
        OCIO::LogOpData logOp(2.0f, OCIO::TRANSFORM_DIR_INVERSE);
        OIIO_CHECK_EQUAL(logOp.getIdentityReplacement()->getType(),
                         OCIO::OpData::MatrixType);
    }
}

#endif

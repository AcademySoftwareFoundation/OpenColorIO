// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_OPS_LOG_LOGOPDATA_H
#define INCLUDED_OCIO_OPS_LOG_LOGOPDATA_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"

namespace OCIO_NAMESPACE
{

// logSideSlope * log( linSideSlope * color + linSideOffset, base) + logSideOffset
enum LogAffineParameter
{
    LOG_SIDE_SLOPE = 0,
    LOG_SIDE_OFFSET,
    LIN_SIDE_SLOPE,
    LIN_SIDE_OFFSET,
    LIN_SIDE_BREAK,
    LINEAR_SLOPE
};

class LogOpData;
typedef OCIO_SHARED_PTR<LogOpData> LogOpDataRcPtr;
typedef OCIO_SHARED_PTR<const LogOpData> ConstLogOpDataRcPtr;

class LogOpData : public OpData
{
    // This class represents the Log op.
    // 
    // A log op applies one of a family of parametric logarithmic functions.
public:

    // Order of LogParams follows LogAffineParameter:
    // LOG_SIDE_SLOPE, LOG_SIDE_OFFSET,
    // LIN_SIDE_SLOPE, LIN_SIDE_OFFSET.
    // optional: LIN_SIDE_BREAK, LINEAR_SLOPE.
    // By default, the first 4 parameters are there.
    // If LIN_SIDE_BREAK is added, it can not be removed.
    // LIN_SIDE_BREAK has to exist, before adding LINEAR_SLOPE.
    // LINEAR_SLOPE can be removed.
    typedef std::vector<double> Params;

    LogOpData() = delete;

    LogOpData(double base, TransformDirection direction);

    LogOpData(double base,
              const double(&logSlope)[3],
              const double(&logOffset)[3],
              const double(&linSlope)[3],
              const double(&linOffset)[3],
              TransformDirection direction);

    LogOpData(double base,
              const Params & redParams,
              const Params & greenParams,
              const Params & blueParams,
              TransformDirection dir);

    virtual ~LogOpData();

    void validate() const override;

    Type getType() const override { return LogType; }

    bool isIdentity() const override;

    OpDataRcPtr getIdentityReplacement() const override;

    bool isNoOp() const override;

    bool hasChannelCrosstalk() const override { return false; }

    std::string getCacheID() const override;

    bool operator==(const OpData& other) const override;

    LogOpDataRcPtr clone() const;

    LogOpDataRcPtr inverse() const;

    bool isInverse(ConstLogOpDataRcPtr & log) const;

    inline const Params & getRedParams() const { return m_redParams; }
    inline void setRedParams(const Params & p) { m_redParams = p; }

    inline const Params & getGreenParams() const { return m_greenParams; }
    inline void setGreenParams(const Params & p) { m_greenParams = p; }

    inline const Params & getBlueParams() const { return m_blueParams; }
    inline void setBlueParams(const Params & p) { m_blueParams = p; }

    TransformDirection getDirection() const noexcept { return m_direction; }

    void setDirection(TransformDirection dir) noexcept { m_direction = dir; }

    bool allComponentsEqual() const;

    std::string getLogSlopeString(std::streamsize precision) const;

    std::string getLinSlopeString(std::streamsize precision) const;

    std::string getLinOffsetString(std::streamsize precision) const;

    std::string getLogOffsetString(std::streamsize precision) const;

    std::string getBaseString(std::streamsize precision) const;

    std::string getLinBreakString(std::streamsize precision) const;

    std::string getLinearSlopeString(std::streamsize precision) const;

    bool isLog2() const;
    bool isLog10() const;

    bool isSimpleLog() const;

    bool isCamera() const;

    void setBase(double base) noexcept;

    double getBase() const noexcept;

    void setValue(LogAffineParameter val, const double(&values)[3]);

    // Can be used to remove LINEAR_SLOPE
    void unsetLinearSlope();

    // Return false if value is not defined.
    bool getValue(LogAffineParameter val, double(&values)[3]) const noexcept;

    void setParameters(const double(&logSlope)[3],
                       const double(&logOffset)[3],
                       const double(&linSlope)[3],
                       const double(&linOffset)[3]);

    void getParameters(double(&logSlope)[3],
                       double(&logOffset)[3],
                       double(&linSlope)[3],
                       double(&linOffset)[3]) const;

private:
    bool isLogBase(double base) const;

    Params m_redParams;
    Params m_greenParams;
    Params m_blueParams;
    double m_base = 2.0;

    TransformDirection m_direction = TRANSFORM_DIR_FORWARD;
};

} // namespace OCIO_NAMESPACE

#endif

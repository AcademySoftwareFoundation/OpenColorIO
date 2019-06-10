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


#ifndef INCLUDED_OCIO_OPS_LOG_LOGOPDATA_H
#define INCLUDED_OCIO_OPS_LOG_LOGOPDATA_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"

OCIO_NAMESPACE_ENTER
{

// logSideSlope * log( linSideSlope * color + linSideOffset, base) + logSideOffset
enum LogAffineParameter
{
    LOG_SIDE_SLOPE = 0,
    LOG_SIDE_OFFSET,
    LIN_SIDE_SLOPE,
    LIN_SIDE_OFFSET
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
    typedef std::vector<double> Params;

    LogOpData() = delete;

    LogOpData(double base, TransformDirection direction);

    LogOpData(double base,
              const double(&logSlope)[3],
              const double(&logOffset)[3],
              const double(&linSlope)[3],
              const double(&linOffset)[3],
              TransformDirection direction);

    LogOpData(BitDepth inBitDepth,
              BitDepth outBitDepth,
              TransformDirection dir,
              double base,
              const Params & redParams,
              const Params & greenParams,
              const Params & blueParams);

    virtual ~LogOpData();

    void validate() const override;

    Type getType() const override { return LogType; }

    bool isIdentity() const override;

    OpDataRcPtr getIdentityReplacement() const;

    bool isNoOp() const override;

    bool hasChannelCrosstalk() const override { return false; }

    void finalize() override;

    bool operator==(const OpData& other) const override;

    LogOpDataRcPtr clone() const;

    LogOpDataRcPtr inverse() const;

    bool isInverse(ConstLogOpDataRcPtr & r) const;

    inline const Params & getRedParams() const { return m_redParams; }
    inline void setRedParams(const Params & p) { m_redParams = p; }

    inline const Params & getGreenParams() const { return m_greenParams; }
    inline void setGreenParams(const Params & p) { m_greenParams = p; }

    inline const Params & getBlueParams() const { return m_blueParams; }
    inline void setBlueParams(const Params & p) { m_blueParams = p; }

    TransformDirection getDirection() const { return m_direction; }

    void setDirection(TransformDirection dir) { m_direction = dir; }

    bool allComponentsEqual() const;

    std::string getLogSlopeString(std::streamsize precision) const;

    std::string getLinSlopeString(std::streamsize precision) const;

    std::string getLinOffsetString(std::streamsize precision) const;

    std::string getLogOffsetString(std::streamsize precision) const;

    std::string getBaseString(std::streamsize precision) const;

    bool isLog2() const;
    bool isLog10() const;

    void setBase(double base);

    double getBase() const;

    void setValue(LogAffineParameter val, const double(&values)[3]);

    void getValue(LogAffineParameter val, double(&values)[3]) const;

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

}
OCIO_NAMESPACE_EXIT

#endif

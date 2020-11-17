// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_CDLOPDATA_H
#define INCLUDED_OCIO_CDLOPDATA_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "MathUtils.h"


namespace OCIO_NAMESPACE
{

class CDLOpData;
typedef OCIO_SHARED_PTR<CDLOpData> CDLOpDataRcPtr;
typedef OCIO_SHARED_PTR<const CDLOpData> ConstCDLOpDataRcPtr;

class CDLOpData : public OpData
{
public:

    // Enumeration of the CDL styles
    enum Style
    {
        CDL_V1_2_FWD = 0,  // Forward (version 1.2) style
        CDL_V1_2_REV,      // Reverse (version 1.2) style
        CDL_NO_CLAMP_FWD,  // Forward no clamping style
        CDL_NO_CLAMP_REV   // Reverse no clamping style
    };

    static inline Style GetDefaultStyle() { return ConvertStyle(CDL_TRANSFORM_DEFAULT,
                                                                TRANSFORM_DIR_FORWARD); }

    static Style GetStyle(const char * name);
    static const char * GetStyleName(Style style);

    static CDLOpData::Style ConvertStyle(CDLStyle style, TransformDirection dir);
    static CDLStyle ConvertStyle(CDLOpData::Style style);

    // Type definition to hold the values of a SOP paramater
    // (scale, offset and power) for all channels
    struct ChannelParams
    {
        ChannelParams(double r, double g, double b)
        {
            setRGB(r, g, b);
        }

        ChannelParams(double x)
        {
            setRGB(x, x, x);
        }

        ChannelParams()
        {
            setRGB(0.0, 0.0, 0.0);
        }

        void setRGB(double r, double g, double b)
        {
            m_data[0] = r;
            m_data[1] = g;
            m_data[2] = b;
        }

        void getRGB(double * rgb) const
        {
            rgb[0] = m_data[0];
            rgb[1] = m_data[1];
            rgb[2] = m_data[2];
        }

        double operator[](unsigned index) const
        {
            if(index>=3)
            {
                throw Exception("Index is out of range");
            }
            return m_data[index];
        }

        double operator[](unsigned index)
        {
            if(index>=3)
            {
                throw Exception("Index is out of range");
            }
            return m_data[index];
        }

        bool operator==(const ChannelParams& other) const
        {
            return 
                EqualWithAbsError(m_data[0], other.m_data[0], 1e-9) &&
                EqualWithAbsError(m_data[1], other.m_data[1], 1e-9) &&
                EqualWithAbsError(m_data[2], other.m_data[2], 1e-9);
        }

        bool operator!=(const ChannelParams& other) const
        {
            return !(*this == other);
        }

    private:
        double m_data[3];
    };


    CDLOpData();

    CDLOpData(const Style & style,
              const ChannelParams & slopeParams,
              const ChannelParams & offsetParams,
              const ChannelParams & powerParams,
              double saturation);

    virtual ~CDLOpData();

    CDLOpDataRcPtr clone() const;

    bool operator==(const OpData& other) const override;

    Type getType() const override { return CDLType; }

    // Note: Return a boolean status based on the enum stored in the "style" variable.
    bool isReverse() const;

    inline Style getStyle() const { return m_style; }
    void setStyle(Style cdlStyle);

    TransformDirection getDirection() const noexcept;
    void setDirection(TransformDirection dir) noexcept;

    const ChannelParams & getSlopeParams() const { return m_slopeParams; }
    void setSlopeParams(const ChannelParams& slopeParams);

    const ChannelParams & getOffsetParams() const { return m_offsetParams; }
    void setOffsetParams(const ChannelParams& offsetParams);

    const ChannelParams & getPowerParams() const { return m_powerParams; }
    void setPowerParams(const ChannelParams& powerParams);

    double getSaturation() const { return m_saturation; }
    void setSaturation(const double saturation);

    bool isNoOp() const override;
    bool isIdentity() const override;

    OpDataRcPtr getIdentityReplacement() const override;

    void getSimplerReplacement(OpDataVec & tmpops) const override;

    bool hasChannelCrosstalk() const override;

    virtual void validate() const override;

    std::string getSlopeString() const;
    std::string getOffsetString() const;
    std::string getPowerString() const;
    std::string getSaturationString() const;

    bool isInverse(ConstCDLOpDataRcPtr & r) const;

    CDLOpDataRcPtr inverse() const;

    std::string getCacheID() const override;

protected:
    static std::string GetChannelParametersString(ChannelParams params);

    // Note: Return a boolean status based on the enum stored in the "style" variable.
    bool isClamping() const;

    void invert() noexcept;

private:
    Style         m_style;         // CDL style
    ChannelParams m_slopeParams;   // Slope parameters for RGB channels
    ChannelParams m_offsetParams;  // Offset parameters for RGB channels
    ChannelParams m_powerParams;   // Power parameters for RGB channels
    double        m_saturation;    // Saturation parameter
};

} // namespace OCIO_NAMESPACE

#endif

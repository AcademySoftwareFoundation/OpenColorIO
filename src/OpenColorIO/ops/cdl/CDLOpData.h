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
        ChannelParams(double r, double g, double b, double a)
        {
            setRGBA(r, g, b, a);
        }

        ChannelParams(double r, double g, double b)
        {
            setRGBA(r, g, b, 1.0);
        }

        ChannelParams(double x)
        {
            setRGBA(x, x, x, 1.0);
        }

        ChannelParams()
        {
            setRGBA(0.0, 0.0, 0.0, 1.0);
        }

        const double * data() const { return m_data; }
        double * data() { return m_data; }

        void setRGBA(double r, double g, double b, double a)
        {
            setRGB(r, g, b);
            setAlpha(a);
        }

        void setRGB(double r, double g, double b)
        {
            m_data[0] = r;
            m_data[1] = g;
            m_data[2] = b;
        }

        void setAlpha(double a)
        {
            m_data[3] = a;
        }

        void getRGB(double * rgb) const
        {
            rgb[0] = m_data[0];
            rgb[1] = m_data[1];
            rgb[2] = m_data[2];
        }

        // Copy the content of the red, green, blue, and alpha channels 
        //   into the array of floats.
        void getRGBA(double * rgba) const
        {
            getRGB(rgba);
            rgba[3] = m_data[3];
        }

        double operator[](unsigned index) const
        {
            if(index>=4)
            {
                throw Exception("Index is out of range");
            }
            return m_data[index];
        }

        double operator[](unsigned index)
        {
            if(index>=4)
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
                EqualWithAbsError(m_data[2], other.m_data[2], 1e-9) &&
                EqualWithAbsError(m_data[3], other.m_data[3], 1e-9);
        }

        bool operator!=(const ChannelParams& other) const
        {
            return !(*this == other);
        }

    private:
        double m_data[4];
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

    TransformDirection getDirection() const;
    void setDirection(TransformDirection dir);

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

    void invert();

private:
    Style         m_style;         // CDL style
    ChannelParams m_slopeParams;   // Slope parameters for RGB channels
    ChannelParams m_offsetParams;  // Offset parameters for RGB channels
    ChannelParams m_powerParams;   // Power parameters for RGB channels
    double        m_saturation;    // Saturation parameter
};

} // namespace OCIO_NAMESPACE

#endif

// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_FIXEDFUNCTIONOPDATA_H
#define INCLUDED_OCIO_FIXEDFUNCTIONOPDATA_H


#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"



namespace OCIO_NAMESPACE
{

class FixedFunctionOpData;
typedef OCIO_SHARED_PTR<FixedFunctionOpData> FixedFunctionOpDataRcPtr;
typedef OCIO_SHARED_PTR<const FixedFunctionOpData> ConstFixedFunctionOpDataRcPtr;


class FixedFunctionOpData : public OpData
{
public:

    enum Style
    {
        ACES_RED_MOD_03_FWD = 0,  // Red modifier (ACES 0.3/0.7)
        ACES_RED_MOD_03_INV,      // Red modifier inverse (ACES 0.3/0.7)
        ACES_RED_MOD_10_FWD,      // Red modifier (ACES 1.0)
        ACES_RED_MOD_10_INV,      // Red modifier inverse (ACES v1.0)
        ACES_GLOW_03_FWD,         // Glow function (ACES 0.3/0.7)
        ACES_GLOW_03_INV,         // Glow function inverse (ACES 0.3/0.7)
        ACES_GLOW_10_FWD,         // Glow function (ACES 1.0)
        ACES_GLOW_10_INV,         // Glow function inverse (ACES 1.0)
        ACES_DARK_TO_DIM_10_FWD,  // Dark to dim surround correction (ACES 1.0)
        ACES_DARK_TO_DIM_10_INV,  // Dim to dark surround correction (ACES 1.0)
        REC2100_SURROUND_FWD,     // Rec.2100 surround correction (takes one double for the gamma param)
        REC2100_SURROUND_INV,     // Rec.2100 surround correction inverse (takes one gamma param)
        RGB_TO_HSV,               // Classic RGB to HSV function
        HSV_TO_RGB,               // Classic HSV to RGB function
        XYZ_TO_xyY,               // CIE XYZ to 1931 xy chromaticity coordinates
        xyY_TO_XYZ,               // Inverse of above
        XYZ_TO_uvY,               // CIE XYZ to 1976 u'v' chromaticity coordinates
        uvY_TO_XYZ,               // Inverse of above
        XYZ_TO_LUV,               // CIE XYZ to 1976 CIELUV colour space (D65 white)
        LUV_TO_XYZ                // Inverse of above
    };

    static const char * ConvertStyleToString(Style style, bool detailed);
    static Style GetStyle(const char * name);
    static Style ConvertStyle(FixedFunctionStyle style, TransformDirection dir);
    static FixedFunctionStyle ConvertStyle(Style style);

    typedef std::vector<double> Params;

    FixedFunctionOpData() = delete;

    explicit FixedFunctionOpData(Style style);
    FixedFunctionOpData(Style style, const Params & params);
    FixedFunctionOpData(const FixedFunctionOpData &) = default;
    virtual ~FixedFunctionOpData();

    FixedFunctionOpDataRcPtr clone() const;

    void validate() const override;

    Type getType() const override { return FixedFunctionType; }

    bool isNoOp() const override { return false; }
    bool isIdentity() const override { return false; }
    bool hasChannelCrosstalk() const override { return true; }

    bool isInverse(ConstFixedFunctionOpDataRcPtr & r) const;
    FixedFunctionOpDataRcPtr inverse() const;

    std::string getCacheID() const override;

    Style getStyle() const  noexcept { return m_style; }
    void setStyle(Style style)  noexcept { m_style = style; }

    TransformDirection getDirection() const noexcept;
    void setDirection(TransformDirection dir) noexcept;

    void setParams(const Params & params) { m_params = params; }
    const Params & getParams() const { return m_params; }

    bool operator==(const OpData & other) const override;

protected:
    void invert() noexcept;

private:
    Style m_style;
    Params m_params;
};

} // namespace OCIO_NAMESPACE

#endif

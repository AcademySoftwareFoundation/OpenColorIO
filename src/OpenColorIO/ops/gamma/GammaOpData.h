// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_GAMMAOPDATA_H
#define INCLUDED_OCIO_GAMMAOPDATA_H


#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"


namespace OCIO_NAMESPACE
{

class GammaOpData;
typedef OCIO_SHARED_PTR<GammaOpData> GammaOpDataRcPtr;
typedef OCIO_SHARED_PTR<const GammaOpData> ConstGammaOpDataRcPtr;


// This class represents the Gamma op.
// 
// A gamma op applies one of a family of parametric power functions.
// 
// These functions are typically used to model the nonlinearity in a
// display device or camera.
// 
// A style argument is used to distinguish the specific function.
// 
// The BASIC style is simply a power law.
// 
// The MONCURVE style is a power law with the addition of a linear segment
// in the shadows which avoids the fact that the slope of a pure power law
// approaches infinity (for powers > 1) at 0.
// 
// Here are the parameters to use with the MONCURVE style to implement several 
// commonly used functions:
//   sRGB -- gamma: 2.4, offset: 0.055
//   Rec.709 -- gamma: 1/0.45, offset: 0.099
//   L* -- gamma: 3.0, offset: 0.16
// 
// The suffixes FWD and REV are used to distinguish the forward model from
// the reverse (or inverse) model.
// 
//
// By convention, the gamma values should be >= 1 whenever possible.
// These are used as is for the forward direction and the reverse/inverse direction 
// is used to obtain exponents of less than 1. For the MONCURVE style, this is enforced 
// during validation so that the gamma and offset work together properly.
//

class GammaOpData : public OpData
{
public:

    enum Style
    {
        BASIC_FWD = 0,
        BASIC_REV,
        BASIC_MIRROR_FWD,
        BASIC_MIRROR_REV,
        BASIC_PASS_THRU_FWD,
        BASIC_PASS_THRU_REV,
        MONCURVE_FWD,
        MONCURVE_REV,
        MONCURVE_MIRROR_FWD,
        MONCURVE_MIRROR_REV
    };

    static Style ConvertStringToStyle(const char * str);
    static const char * ConvertStyleToString(Style style);

    static NegativeStyle ConvertStyle(Style style);
    static Style ConvertStyleBasic(NegativeStyle style, TransformDirection dir);
    static Style ConvertStyleMonCurve(NegativeStyle style, TransformDirection dir);

    typedef std::vector<double> Params;

    GammaOpData();
    GammaOpData(const GammaOpData &) = default;

    GammaOpData(const Style & style,
                const Params & redParams,
                const Params & greenParams,
                const Params & blueParams,
                const Params & alphaParams);

    virtual ~GammaOpData();

    GammaOpDataRcPtr clone() const;

    inline Style getStyle() const noexcept { return m_style; }

    Type getType() const override { return GammaType; }

    inline const Params & getRedParams()   const { return m_redParams;   }
    inline const Params & getGreenParams() const { return m_greenParams; }
    inline const Params & getBlueParams()  const { return m_blueParams;  }
    inline const Params & getAlphaParams() const { return m_alphaParams; }

    inline Params & getRedParams()   { return m_redParams;   }
    inline Params & getGreenParams() { return m_greenParams; }
    inline Params & getBlueParams()  { return m_blueParams;  }
    inline Params & getAlphaParams() { return m_alphaParams; }

    void setStyle(const Style & style) noexcept;

    void setRedParams  (const Params & parameter);
    void setGreenParams(const Params & parameter);
    void setBlueParams (const Params & parameter);
    void setAlphaParams(const Params & parameter);

    void setParams( const Params & parameter);

    virtual bool isNoOp() const override;
    virtual bool isIdentity() const override;

    virtual OpDataRcPtr getIdentityReplacement() const override;

    virtual bool isChannelIndependent() const { return true; }

    bool mayCompose(const GammaOpData & gamma) const;
    GammaOpDataRcPtr compose(const GammaOpData & gamma) const;

    // Check if red params == green params == blue params == alpha params.
    bool areAllComponentsEqual() const;

    // Check if the red, green and blue params are identical and the alpha is identity.
    bool isNonChannelDependent() const;

    // Check if the alpha channel does nothing except bit depth conversion.
    bool isAlphaComponentIdentity() const;

    GammaOpDataRcPtr inverse() const;
    bool isInverse(const GammaOpData & B) const;

    virtual bool hasChannelCrosstalk() const override { return false; }

    virtual void validate() const override;

    virtual void validateParameters() const;

    bool operator==(const OpData& other) const override;

    std::string getCacheID() const override;

    TransformDirection getDirection() const noexcept;
    void setDirection(TransformDirection dir) noexcept;

    static bool isIdentityParameters(const Params & parameters, Style style);
    static Params getIdentityParameters(Style style);

protected:
    bool isClamping() const;

private:
    void invert() noexcept;

    Style m_style;
    Params m_redParams;
    Params m_greenParams;
    Params m_blueParams;
    Params m_alphaParams;
};

} // namespace OCIO_NAMESPACE


#endif

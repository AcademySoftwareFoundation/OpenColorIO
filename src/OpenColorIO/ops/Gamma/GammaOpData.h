/*
Copyright (c) 2018 Autodesk Inc., et al.
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


#ifndef INCLUDED_OCIO_GAMMAOPDATA_H
#define INCLUDED_OCIO_GAMMAOPDATA_H


#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"


OCIO_NAMESPACE_ENTER
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
        MONCURVE_FWD,
        MONCURVE_REV
    };

    static const char * convertStyleToString(Style style);

    typedef std::vector<double> Params;

    GammaOpData();

    GammaOpData(BitDepth inBitDepth,
                BitDepth outBitDepth,
                const std::string & id,
                const OpData::Descriptions & desc,
                const Style & style,
                const Params & redParams,
                const Params & greenParams,
                const Params & blueParams,
                const Params & alphaParams);

    virtual ~GammaOpData();

    GammaOpDataRcPtr clone() const;

    inline Style getStyle() const { return m_style; }

    Type getType() const override { return GammaType; }

    inline const Params & getRedParams()   const { return m_redParams;   }
    inline const Params & getGreenParams() const { return m_greenParams; }
    inline const Params & getBlueParams()  const { return m_blueParams;  }
    inline const Params & getAlphaParams() const { return m_alphaParams; }

    inline Params & getRedParams()   { return m_redParams;   }
    inline Params & getGreenParams() { return m_greenParams; }
    inline Params & getBlueParams()  { return m_blueParams;  }
    inline Params & getAlphaParams() { return m_alphaParams; }

    void setStyle(const Style & style);

    void setRedParams  (const Params & parameter);
    void setGreenParams(const Params & parameter);
    void setBlueParams (const Params & parameter);
    void setAlphaParams(const Params & parameter);

    void setParams( const Params & parameter);

    virtual bool isNoOp() const override;
    virtual bool isIdentity() const override;
    bool isClamping() const;

    virtual OpDataRcPtr getIdentityReplacement() const;

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

    bool operator==(const GammaOpData& other) const;

    virtual void finalize() override;


    static bool isIdentityParameters(const Params & parameters, Style style);
    static Params getIdentityParameters(Style style);

private:
    Style m_style;
    Params m_redParams;
    Params m_greenParams;
    Params m_blueParams;
    Params m_alphaParams;
};

}
OCIO_NAMESPACE_EXIT


#endif

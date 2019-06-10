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


#ifndef INCLUDED_OCIO_FIXEDFUNCTIONOPDATA_H
#define INCLUDED_OCIO_FIXEDFUNCTIONOPDATA_H


#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"



OCIO_NAMESPACE_ENTER
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
        REC2100_SURROUND          // Rec.2100 surround correction (takes one double for the gamma param)
    };

    static const char * ConvertStyleToString(Style style, bool detailed);
    static Style GetStyle(const char * name);

    typedef std::vector<double> Params;

    FixedFunctionOpData();

    FixedFunctionOpData(BitDepth inBitDepth,
                        BitDepth outBitDepth,
                        const Params & params,
                        Style style);

    virtual ~FixedFunctionOpData();

    FixedFunctionOpDataRcPtr clone() const;

    void validate() const override;

    Type getType() const override { return FixedFunctionType; }

    bool isNoOp() const override { return false; }
    bool isIdentity() const override { return false; }
    bool hasChannelCrosstalk() const override { return true; }

    bool isInverse(ConstFixedFunctionOpDataRcPtr & r) const;
    FixedFunctionOpDataRcPtr inverse() const;

    void finalize() override;

    Style getStyle() const { return m_style; }
    void setStyle(Style style) { m_style = style; }

    void setParams(const Params & params) { m_params = params; }
    const Params & getParams() const { return m_params; }

    bool operator==(const OpData & other) const override;

protected:
    void invert();

private:
    Style m_style;
    Params m_params;
};

}
OCIO_NAMESPACE_EXIT

#endif

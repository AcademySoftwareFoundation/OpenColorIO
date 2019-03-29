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


#ifndef INCLUDED_OCIO_RANGEOPDATA_H
#define INCLUDED_OCIO_RANGEOPDATA_H


#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/Matrix/MatrixOps.h"


OCIO_NAMESPACE_ENTER
{
      
class RangeOpData;
typedef OCIO_SHARED_PTR<RangeOpData> RangeOpDataRcPtr;
typedef OCIO_SHARED_PTR<const RangeOpData> ConstRangeOpDataRcPtr;

class IndexMapping;

// The class represents the Range op data.
// 
// The Range is used to apply an affine transform (scale & offset),
// clamp values to min/max bounds, or apply a simple bit-depth conversion.
// 
// The spec is somewhat ambiguous about the details so we are required
// to make some judgement calls.  The spec allows max/min elements to
// be missing.  This means no clamping is requested.  In order to keep
// the semantics reasonable, we further require that if minIn is set
// then minOut must also be set (but setting minIn doesn't require maxIn).
// 
// The min/max tags serve two purposes, they define the scale and offset
// that will be applied to map in to out.  They also clamp values.
// 
// If no min/max tags are present, the op does bit-depth conversion
// without clamping.  If only min but not max is present then clamping
// is only done at the low end (and vice versa).
// 
// If only min or max is present, the spec doesn't give details so we
// set the scale to whatever is necessary to do bit-depth conversion
// and set the offset to map the in bound to the out bound.
// 
class RangeOpData : public OpData
{
public:
    RangeOpData();

    RangeOpData(BitDepth inBitDepth,  // Requested bit depth for the input
                BitDepth outBitDepth, // Requested bit depth for the output
                double minInValue,    // Lower bound of the domain
                double maxInValue,    // Upper bound of the domain
                double minOutValue,   // Lower bound of the range
                double maxOutValue    // Upper bound of the range
                );

    // Constructor from a 2-entry index map from a Lut1D or Lut3D.
    // Note: Throws if the index map is not appropriate.
    RangeOpData(const IndexMapping& pIM, BitDepth inDepth, unsigned int len);

    virtual ~RangeOpData();

    // Return the value used to set the value of an empty boundary.
    // (May be used to set arguments to the constructor.)
    static double EmptyValue();

    RangeOpDataRcPtr clone() const;

    //
    // Note: The setters below do not call validate and are only for use 
    //       by the file format parser.
    //

    // The lower bound of the op domain
    inline double getMinInValue() const { return m_minInValue; }
    bool hasMinInValue() const;
    void unsetMinInValue();
    void setMinInValue(double value);

    // The upper bound of the op domain
    inline double getMaxInValue() const { return m_maxInValue; }
    bool hasMaxInValue() const;
    void unsetMaxInValue();
    void setMaxInValue(double value);

    // The lower bound of the op range
    inline double getMinOutValue() const { return m_minOutValue; }
    bool hasMinOutValue() const;
    void unsetMinOutValue();
    void setMinOutValue(double value);

    // The upper bound of the op range
    inline double getMaxOutValue() const { return m_maxOutValue; }
    bool hasMaxOutValue() const;
    void unsetMaxOutValue();
    void setMaxOutValue(double value);

    // Get the scale factor used in computation
    inline double getScale() const { return m_scale; }

    // Get the offset used in computation
    inline double getOffset() const { return m_offset; }

    // Get the lower clip used in computation
    inline double getLowBound() const { return m_lowBound; }

    // Get the upper clip used in computation
    inline double getHighBound() const { return m_highBound; }

    // Get the scale factor used in computation for alpha
    inline double getAlphaScale() const { return m_alphaScale; }

    // Validate the state of the instance and initialize private members.
    void validate() const override;

    Type getType() const override { return RangeType; }

    bool isNoOp() const override;
    bool isIdentity() const override;

    // Make an op to replace an identity (or pair identity) of this op type.
    // (Note: For a pair identity, call this on the first half and then set
    // the result's output bit-depth to match the second half.)
    // returns the opData (to be managed by caller)
    OpDataRcPtr getIdentityReplacement() const;

    // True if the op does not scale and does not clamp the normal domain.
    bool isClampIdentity() const;

    // True if the op limits the incoming pixels as least as much as
    // a 1d or 3d LUT would.  I.e., the min/max clamps are at least as narrow
    // as [0, getBitDepthMaxValue()].
    bool clampsToLutDomain() const;

    // True if the op only a clamp on values below 0.
    bool isClampNegs() const;

    bool hasChannelCrosstalk() const override { return false; }

    // Set the output bit depth
    // - out the output bit depth
    // Note: Multiple set operations are lossless.
    void setOutputBitDepth(BitDepth out) override;

    // Set the input bit depth
    // - in the input bit depth
    // Note: Multiple set operations are lossless.
    void setInputBitDepth(BitDepth in) override;

    // True if minIn & minOut do not request clipping
    bool minIsEmpty() const;

    // True if maxIn & maxOut do not request clipping
    bool maxIsEmpty() const;

    // True if the scale and offset are not the identity
    // - ignoreBitDepth to ignore the scaling needed for depth conversion
    bool scales(bool ignoreBitDepth) const;

    // True if the supplied value would be clipped
    bool wouldClip(double val) const;

    // True if low clipping is needed (at the current in & out bit-depths)
    bool minClips() const;

    // True if high clipping is needed (at the current in & out bit-depths)
    bool maxClips() const;

    // Create a MatrixOp that is equivalent to the Range except does not clamp.
    MatrixOpDataRcPtr convertToMatrix() const;

    bool operator==(const OpData& other) const override;

    // True if the op is the inverse
    bool isInverse(ConstRangeOpDataRcPtr & r) const;

    RangeOpDataRcPtr inverse() const;

    // True if the double (i.e. bound values) differs
    static bool FloatsDiffer(double x1, double x2);

    virtual void finalize() override;

private:
    void fillScaleOffset() const;
    double clipOverride(bool isLower) const;
    void fillBounds() const;

private:
    double m_minInValue;            // Minimum for the input value
    double m_maxInValue;            // Maximum for the input value
    double m_minOutValue;           // Minimum for the output value
    double m_maxOutValue;           // Maximum for the output value
    mutable double m_scale;         // Scaling calculated from the limits
    mutable double m_offset;        // Offset calculated from the limits
    mutable double m_lowBound;      // Lower clip point calculated from the limits
    mutable double m_highBound;     // Upper clip point calculated from the limits
    mutable double m_alphaScale;    // Bit-depth scaling for the alpha channel
};

}
OCIO_NAMESPACE_EXIT

#endif

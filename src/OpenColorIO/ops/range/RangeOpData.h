// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_RANGEOPDATA_H
#define INCLUDED_OCIO_RANGEOPDATA_H


#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/matrix/MatrixOp.h"


namespace OCIO_NAMESPACE
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

    RangeOpData(const RangeOpData &) = default;

    RangeOpData(double minInValue,    // Lower bound of the domain
                double maxInValue,    // Upper bound of the domain
                double minOutValue,   // Lower bound of the range
                double maxOutValue    // Upper bound of the range
                );

    RangeOpData(double minInValue,    // Lower bound of the domain
                double maxInValue,    // Upper bound of the domain
                double minOutValue,   // Lower bound of the range
                double maxOutValue,   // Upper bound of the range
                TransformDirection dir);

    // Constructor from a 2-entry index map from a Lut1D or Lut3D.
    // Note: Throws if the index map is not appropriate.
    RangeOpData(const IndexMapping & pIM, unsigned int len, BitDepth bitdepth);

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

    // Validate the state of the instance and initialize private members.
    void validate() const override;
    std::string getCacheID() const override;

    Type getType() const override { return RangeType; }

    bool isNoOp() const override;
    bool isIdentity() const override;

    // True if the op limits the incoming pixels as least as much as
    // a 1d or 3d LUT would.  I.e., the min/max clamps are at least as narrow
    // as [0, getBitDepthMaxValue()].
    bool clampsToLutDomain() const;

    // True if the op only a clamp on values below 0.
    bool isClampNegs() const;

    bool hasChannelCrosstalk() const override { return false; }

    // True if minIn & minOut do not request clipping
    bool minIsEmpty() const;

    // True if maxIn & maxOut do not request clipping
    bool maxIsEmpty() const;

    // True if the scale and offset are not the identity
    bool scales() const;

    // Create a MatrixOp that is equivalent to the Range except does not clamp.
    MatrixOpDataRcPtr convertToMatrix() const;

    bool operator==(const OpData& other) const override;

    RangeOpDataRcPtr getAsForward() const;

    TransformDirection getDirection() const noexcept { return m_direction; }
    void setDirection(TransformDirection dir) noexcept;

    BitDepth getFileInputBitDepth() const { return m_fileInBitDepth; }
    void setFileInputBitDepth(BitDepth in) { m_fileInBitDepth = in; }

    BitDepth getFileOutputBitDepth() const { return m_fileOutBitDepth; }
    void setFileOutputBitDepth(BitDepth out) { m_fileOutBitDepth = out; }

    void normalize();

    RangeOpDataRcPtr compose(ConstRangeOpDataRcPtr & r) const;

private:
    void fillScaleOffset() const;

    // True if the double (i.e. bound values) differs
    static bool FloatsDiffer(double x1, double x2);

private:
    double m_minInValue;            // Minimum for the input value
    double m_maxInValue;            // Maximum for the input value
    double m_minOutValue;           // Minimum for the output value
    double m_maxOutValue;           // Maximum for the output value
    mutable double m_scale;         // Scaling calculated from the limits
    mutable double m_offset;        // Offset calculated from the limits

    BitDepth m_fileInBitDepth  = BIT_DEPTH_UNKNOWN; // In bit-depth to be used for file I/O
    BitDepth m_fileOutBitDepth = BIT_DEPTH_UNKNOWN; // Out bit-depth to be used for file I/O

    TransformDirection m_direction{ TRANSFORM_DIR_FORWARD };
};

} // namespace OCIO_NAMESPACE

#endif

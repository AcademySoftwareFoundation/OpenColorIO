// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_GRADINGBSPLINECURVE_H
#define INCLUDED_OCIO_GRADINGBSPLINECURVE_H

#include <array>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

namespace OCIO_NAMESPACE
{
class GpuShaderText;

class GradingBSplineCurveImpl : public GradingBSplineCurve
{
public:
    explicit GradingBSplineCurveImpl(size_t size);
    GradingBSplineCurveImpl(const std::vector<GradingControlPoint> & controlPoints);
    ~GradingBSplineCurveImpl() = default;

    GradingBSplineCurveRcPtr createEditableCopy() const override;
    size_t getNumControlPoints() const noexcept override;
    void setNumControlPoints(size_t size) override;
    const GradingControlPoint & getControlPoint(size_t index) const override;
    GradingControlPoint & getControlPoint(size_t index) override;
    float getSlope(size_t index) const override;
    void setSlope(size_t index, float slope) override;
    bool slopesAreDefault() const override;
    void validate() const override;

    bool isIdentity() const;

    // The KnotsCoefs struct is used when evaluating the curves.  Unlike the GradingBSplineCurve
    // class, which is for a single curve, the KnotsCoefs struct is something used by the
    // DynamicPropertyGradingRGBCurve to hold the parameters needed to evaluate _all_ the curves.
    //
    // For optimization and reusability purposes, the renderers will expect all of the curve data
    // to be packed in arrays. The curve data will be packed in the order of the curve Type enum.
    //
    // The RGBCurve will pack the curves in RGB_RED, RGB_GREEN, RGB_BLUE, RGB_MASTER order.
    //
    // note: DynamicProperties
    //
    // The curves use dynamic values for all of its data. Since the GPU code only finalizes the
    // GLSL shader once, we must associate each Uniform objects of the GpuShaderCreator used by
    // the renderer to an address in memory that will contain the dynamic value. This is because
    // when dynamic values change, no one is notified of the change (not the GPU renderer nor the
    // GpuShaderCreator). The Uniform object simply uses the function pointer it holds to update
    // the uniform in the shader everytime shader is used.
    //
    // The dynamic property keeps an instance of KnotsCoefs, and exposes its content with a set
    // of accessors. See DynamicPropertyGradingRGBCurveImpl.
    struct KnotsCoefs
    {
        KnotsCoefs() = delete;

        explicit KnotsCoefs(size_t numCurves)
        {
            m_knotsOffsetsArray.resize(2 * numCurves);
            m_coefsOffsetsArray.resize(2 * numCurves);
        }

        // Pre-processing scalar values.

        // Do not apply the op if all curves are identity.
        bool m_localBypass;

        // Pre-processing arrays of length NumCurves*2. Contains offest and count for each curve.
        // When curve is identity, offset is -1 and count is 0.
        std::vector<int> m_knotsOffsetsArray; // Contains offsets info for ALL curves.
        std::vector<int> m_coefsOffsetsArray; // Contains offsets info for ALL curves.

        // The max number of control points must be kept to a minimum, otherwise we may reach the
        // max number of allowed uniforms (1024), resulting in linking errors.
        //
        // On older hardware, even if it links, the responsiveness may become extremely slow if 
        // the number is too large.  It's not clear how to estimate what that limit is but
        // 200 knots & 600 coefs is too many.
        //
        // An alternative would be to use a dynamic size for the uniform arrays, but this would
        // require a finalization of the transform each time the size changes.
        //
        // There are three coefs needed for each polynomial segment, which is the number of knots
        // -1.  The number of knots is chosen dynamically based on what is needed to fit the
        // control points but the number of knots may be, at most, the number of control
        // points * 2 - 1.
        // 
        // There are 4 RGB curves (R, G, B, M) each represented by one RGBCurve.  We want to keep
        // the total for two curves well below the 200 knot, 600 coef limit.
        // (TODO: 6 Hue curves (H/H, H/S, H/L, L/S, L/L, S/S)).
        //
        // A value of 60 knots would allow about 30 control points spread across the 4 or 6 curves.
        // Note that the default RGB curves use 3 control points each and the hue curves may use as
        // many as 6 even for the default.  However, there is an optimization below that does not
        // add knots for curves that are simply identity.
        //
        // Maximum size of the knots array (for ALL curves).
        static constexpr int MAX_NUM_KNOTS = 60;
        // Maximum size of the coefs array (for ALL curves).
        static constexpr int MAX_NUM_COEFS = 180;

        // Pre-processing arrays of length MAX_NUM_KNOTS and MAX_NUM_COEFS.
        std::vector<float> m_coefsArray;  // Contains packed coefs of ALL curves.
        std::vector<float> m_knotsArray;  // Contains packed knots of ALL curves.

        float evalCurve(int curveIdx, float x) const;
        float evalCurveRev(int curveIdx, float x) const;
    };

    // Compute knots and coefs for a curve and add result to knotsCoefs. It has to be called for
    // each curve using a given curve order.
    void computeKnotsAndCoefs(KnotsCoefs & knotsCoefs, int curveIdx) const;

    static void AddShaderEval(GpuShaderText & st,
                              const std::string & knotsOffsets, const std::string & coefsOffsets,
                              const std::string & knots, const std::string & coefs, bool isInv);
private:
    void validateIndex(size_t index) const;

    std::vector<GradingControlPoint> m_controlPoints;
    std::vector<float> m_slopesArray;  // Optional slope values for the control points.
};

bool IsGradingCurveIdentity(const ConstGradingBSplineCurveRcPtr & curve);

bool operator==(const GradingControlPoint & lhs, const GradingControlPoint & rhs);
bool operator!=(const GradingControlPoint & lhs, const GradingControlPoint & rhs);
bool operator==(const GradingBSplineCurve & lhs, const GradingBSplineCurve & rhs);
bool operator!=(const GradingBSplineCurve & lhs, const GradingBSplineCurve & rhs);


} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_GRADINGBSPLINECURVE_H

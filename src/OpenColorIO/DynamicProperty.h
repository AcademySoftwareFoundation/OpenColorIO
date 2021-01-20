// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_DYNAMICPROPERTY_H
#define INCLUDED_OCIO_DYNAMICPROPERTY_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/gradingprimary/GradingPrimary.h"
#include "ops/gradingrgbcurve/GradingBSplineCurve.h"
#include "ops/gradingtone/GradingTone.h"

namespace OCIO_NAMESPACE
{

class DynamicPropertyImpl;
typedef OCIO_SHARED_PTR<DynamicPropertyImpl> DynamicPropertyImplRcPtr;

// Holds type and dynamic state.
class DynamicPropertyImpl : public DynamicProperty
{
public:
    DynamicPropertyImpl() = delete;
    DynamicPropertyImpl(DynamicPropertyType type, bool dynamic);
    virtual ~DynamicPropertyImpl() = default;

    DynamicPropertyType getType() const noexcept override
    {
        return m_type;
    }

    bool isDynamic() const noexcept
    {
        return m_isDynamic;
    }

    void makeDynamic() noexcept
    {
        m_isDynamic = true;
    }

    void makeNonDynamic() noexcept
    {
        m_isDynamic = false;
    }

    // When comparing properties for equality, the following rules apply:
    // - If neither of the objects are dynamic, simply compare the values as usual.
    // - If both objects are dynamic, always return true. Even if the values are
    //   different now, in practice they will be set to the same value since all
    //   properties in a processor of the same type that are dynamic are
    //   synchronized to the same value.
    // - If one of the objects is dynamic and the other is not, always
    //   return false. Even if the values agree now, they may not once in use.
    bool equals(const DynamicPropertyImpl & rhs) const;

protected:
    DynamicPropertyImpl(DynamicPropertyType type);

    DynamicPropertyImpl & operator=(DynamicPropertyImpl &) = delete;
    DynamicPropertyType m_type{ DYNAMIC_PROPERTY_EXPOSURE };

    bool m_isDynamic{ false };
};

bool operator==(const DynamicProperty &, const DynamicProperty &);


class DynamicPropertyDoubleImpl;
typedef OCIO_SHARED_PTR<DynamicPropertyDoubleImpl> DynamicPropertyDoubleImplRcPtr;

class DynamicPropertyDoubleImpl : public DynamicPropertyImpl, public DynamicPropertyDouble
{
public:
    DynamicPropertyDoubleImpl() = delete;
    DynamicPropertyDoubleImpl(DynamicPropertyType type, double val, bool dynamic);
    ~DynamicPropertyDoubleImpl() = default;
    double getValue() const override { return m_value; }
    void setValue(double value) override { m_value = value; }

    DynamicPropertyDoubleImplRcPtr createEditableCopy() const;

private:
    double m_value;
};

class DynamicPropertyGradingPrimaryImpl;
typedef OCIO_SHARED_PTR<DynamicPropertyGradingPrimaryImpl> DynamicPropertyGradingPrimaryImplRcPtr;

class DynamicPropertyGradingPrimaryImpl : public DynamicPropertyImpl,
                                          public DynamicPropertyGradingPrimary
{
public:
    DynamicPropertyGradingPrimaryImpl() = delete;
    DynamicPropertyGradingPrimaryImpl(GradingStyle style,
                                      TransformDirection dir,
                                      const GradingPrimary & val,
                                      bool dynamic);
    // Only to create a copy.
    DynamicPropertyGradingPrimaryImpl(GradingStyle style,
                                      TransformDirection dir,
                                      const GradingPrimary & value,
                                      const GradingPrimaryPreRender & computed,
                                      bool dynamic);
    ~DynamicPropertyGradingPrimaryImpl() = default;

    const GradingPrimary & getValue() const override { return m_value; }
    void setValue(const GradingPrimary & value) override;

    void setStyle(GradingStyle style);
    void setDirection(TransformDirection dir) noexcept;
    TransformDirection getDirection() const noexcept { return m_direction; }
    const GradingPrimaryPreRender & getComputedValue() const { return m_preRenderValues; }

    const Float3 & getBrightness() const { return m_preRenderValues.getBrightness(); }
    const Float3 & getContrast() const { return m_preRenderValues.getContrast(); }
    const Float3 & getGamma() const { return m_preRenderValues.getGamma(); }
    double getPivot() const { return m_preRenderValues.getPivot(); }

    const Float3 & getExposure() const { return m_preRenderValues.getExposure(); }
    const Float3 & getOffset() const { return m_preRenderValues.getOffset(); }

    const Float3 & getSlope() const { return m_preRenderValues.getSlope(); }

    // Do not apply the op if all params are identity.
    bool getLocalBypass() const { return m_preRenderValues.m_localBypass; }

    DynamicPropertyGradingPrimaryImplRcPtr createEditableCopy() const;

private:
    GradingStyle m_style{ GRADING_LOG };
    TransformDirection m_direction{ TRANSFORM_DIR_FORWARD };
    GradingPrimary m_value;
    GradingPrimaryPreRender m_preRenderValues;
};


class DynamicPropertyGradingRGBCurveImpl;
typedef OCIO_SHARED_PTR<DynamicPropertyGradingRGBCurveImpl> DynamicPropertyGradingRGBCurveImplRcPtr;

class DynamicPropertyGradingRGBCurveImpl : public DynamicPropertyImpl,
                                           public DynamicPropertyGradingRGBCurve
{
public:
    DynamicPropertyGradingRGBCurveImpl() = delete;
    DynamicPropertyGradingRGBCurveImpl(const ConstGradingRGBCurveRcPtr & value, bool dynamic);
    ~DynamicPropertyGradingRGBCurveImpl() = default;
    const ConstGradingRGBCurveRcPtr & getValue() const override;
    void setValue(const ConstGradingRGBCurveRcPtr & value) override;

    bool getLocalBypass() const;
    int getNumKnots() const;
    int getNumCoefs() const;
    static int GetNumOffsetValues() { return 8; }
    const int * getKnotsOffsetsArray() const;
    const int * getCoefsOffsetsArray() const;
    const float * getKnotsArray() const;
    const float * getCoefsArray() const;

    const GradingBSplineCurveImpl::KnotsCoefs & getKnotsCoefs() const { return m_knotsCoefs; }

    static unsigned int GetMaxKnots();
    static unsigned int GetMaxCoefs();

    DynamicPropertyGradingRGBCurveImplRcPtr createEditableCopy() const;

private:
    void precompute();

    ConstGradingRGBCurveRcPtr m_gradingRGBCurve;

    // Holds curve data as knots and coefs. There are 4 curves.
    GradingBSplineCurveImpl::KnotsCoefs m_knotsCoefs{ 4 };
};

class DynamicPropertyGradingToneImpl;
typedef OCIO_SHARED_PTR<DynamicPropertyGradingToneImpl> DynamicPropertyGradingToneImplRcPtr;

class DynamicPropertyGradingToneImpl : public DynamicPropertyImpl,
                                       public DynamicPropertyGradingTone
{
public:
    DynamicPropertyGradingToneImpl() = delete;
    DynamicPropertyGradingToneImpl(const GradingTone & value, GradingStyle style, bool dynamic);
    // Only to create a copy.
    DynamicPropertyGradingToneImpl(const GradingTone & value,
                                   const GradingTonePreRender & computed,
                                   bool dynamic);
    ~DynamicPropertyGradingToneImpl() = default;

    const GradingTone & getValue() const override { return m_value; }
    void setValue(const GradingTone & value) override;

    void setStyle(GradingStyle style);
    const GradingTonePreRender & getComputedValue() const { return m_preRenderValues; }

    bool getLocalBypass() const { return m_preRenderValues.m_localBypass; }

    DynamicPropertyGradingToneImplRcPtr createEditableCopy() const;

private:

    GradingTone m_value;
    GradingTonePreRender m_preRenderValues;
};

} // namespace OCIO_NAMESPACE

#endif
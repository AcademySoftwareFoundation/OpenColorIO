// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_PYDYNAMICPROPERTY_H
#define INCLUDED_OCIO_PYDYNAMICPROPERTY_H

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

struct OCIOHIDDEN PyDynamicProperty
{
    PyDynamicProperty(DynamicPropertyRcPtr prop)
        : m_prop(prop)
    {
    }

    virtual ~PyDynamicProperty() = default;

    DynamicPropertyType getType() const
    {
        return m_prop->getType();
    }

    double getDouble()
    {
        auto propDouble = DynamicPropertyValue::AsDouble(m_prop);
        if (propDouble)
        {
            return propDouble->getValue();
        }
        throw OCIO::Exception("Invalid dynamic property type (doesn't hold a double).");
    }

    void setDouble(double v)
    {
        auto propDouble = DynamicPropertyValue::AsDouble(m_prop);
        if (propDouble)
        {
            return propDouble->setValue(v);
        }
        throw OCIO::Exception("Invalid dynamic property type (doesn't accept a double).");
    }

    const GradingPrimary & getGradingPrimary()
    {
        auto propGP = DynamicPropertyValue::AsGradingPrimary(m_prop);
        if (propGP)
        {
            return propGP->getValue();
        }
        throw OCIO::Exception("Invalid dynamic property type (doesn't hold a GradingPrimary).");
    }

    void setGradingPrimary(const GradingPrimary & v)
    {
        auto propGP = DynamicPropertyValue::AsGradingPrimary(m_prop);
        if (propGP)
        {
            return propGP->setValue(v);
        }
        throw OCIO::Exception("Invalid dynamic property type (doesn't accept a GradingPrimary).");
    }

    const ConstGradingRGBCurveRcPtr & getGradingRGBCurve()
    {
        auto propGC = DynamicPropertyValue::AsGradingRGBCurve(m_prop);
        if (propGC)
        {
            return propGC->getValue();
        }
        throw OCIO::Exception("Invalid dynamic property type (doesn't hold a GradingRGBCurve).");
    }

    void setGradingRGBCurve(const ConstGradingRGBCurveRcPtr & v)
    {
        auto propGC = DynamicPropertyValue::AsGradingRGBCurve(m_prop);
        if (propGC)
        {
            return propGC->setValue(v);
        }
        throw OCIO::Exception("Invalid dynamic property type (doesn't accept a GradingRGBCurve).");
    }

    const GradingTone & getGradingTone()
    {
        auto propGT = DynamicPropertyValue::AsGradingTone(m_prop);
        if (propGT)
        {
            return propGT->getValue();
        }
        throw OCIO::Exception("Invalid dynamic property type (doesn't hold a GradingTone).");
    }

    void setGradingTone(const GradingTone & v)
    {
        auto propGT = DynamicPropertyValue::AsGradingTone(m_prop);
        if (propGT)
        {
            return propGT->setValue(v);
        }
        throw OCIO::Exception("Invalid dynamic property type (doesn't accept a GradingTone).");
    }

private:
    DynamicPropertyRcPtr m_prop;
};

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_PYDYNAMICPROPERTY_H

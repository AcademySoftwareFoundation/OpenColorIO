// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_DYNAMICPROPERTY_H
#define INCLUDED_OCIO_DYNAMICPROPERTY_H

#include <OpenColorIO/OpenColorIO.h>

namespace OCIO_NAMESPACE
{

class DynamicPropertyImpl;
typedef OCIO_SHARED_PTR<DynamicPropertyImpl> DynamicPropertyImplRcPtr;

// Holds a value that can be made dynamic.
class DynamicPropertyImpl : public DynamicProperty
{
public:
    DynamicPropertyImpl(DynamicPropertyType type, double value, bool dynamic);
    DynamicPropertyImpl(DynamicPropertyImpl & rhs);
    virtual ~DynamicPropertyImpl() = default;

    double getDoubleValue() const override;
    void setValue(double value) override;

    DynamicPropertyType getType() const override
    {
        return m_type;
    }

    DynamicPropertyValueType getValueType() const override
    {
        return m_valueType;
    }

    bool isDynamic() const override
    {
        return m_isDynamic;
    }

    void makeDynamic()
    {
        m_isDynamic = true;
    }

    void makeNonDynamic()
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

private:
    DynamicPropertyImpl() = delete;
    DynamicPropertyImpl & operator=(DynamicPropertyImpl &) = delete;

    DynamicPropertyType m_type = DYNAMIC_PROPERTY_EXPOSURE;

    DynamicPropertyValueType m_valueType = DYNAMIC_PROPERTY_DOUBLE;
    double m_value = 0;
    bool m_isDynamic = false;
};

bool operator ==(const DynamicProperty &, const DynamicProperty &);

} // namespace OCIO_NAMESPACE

#endif
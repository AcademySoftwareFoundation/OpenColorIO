// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "DynamicProperty.h"

namespace OCIO_NAMESPACE
{

bool operator==(const DynamicProperty &lhs, const DynamicProperty &rhs)
{
    if (DynamicPropertyImpl const * plhs = dynamic_cast<DynamicPropertyImpl const*>(&lhs))
    {
        if (DynamicPropertyImpl const * prhs = dynamic_cast<DynamicPropertyImpl const*>(&rhs))
        {
            return plhs->equals(*prhs);
        }
    }
    return false;
}

DynamicPropertyImpl::DynamicPropertyImpl(DynamicPropertyType type, double value, bool dynamic)
    :   m_type(type)
    ,   m_valueType(DYNAMIC_PROPERTY_DOUBLE)
    ,   m_value(value)
    ,   m_isDynamic(dynamic)
{
}

DynamicPropertyImpl::DynamicPropertyImpl(DynamicPropertyImpl & rhs)
    :   m_type(rhs.m_type)
    ,   m_valueType(rhs.m_valueType)
    ,   m_value(rhs.m_value)
    ,   m_isDynamic(rhs.m_isDynamic)
{   
}

double DynamicPropertyImpl::getDoubleValue() const
{
    if(m_valueType!=DYNAMIC_PROPERTY_DOUBLE)
    {
        throw Exception("The dynamic property does not hold a double precision value.");
    }

    return m_value;
}

void DynamicPropertyImpl::setValue(double value)
{
    if(m_valueType!=DYNAMIC_PROPERTY_DOUBLE)
    {
        throw Exception("The dynamic property does not hold a double precision value.");
    }

    m_value = value;
}

bool DynamicPropertyImpl::equals(const DynamicPropertyImpl & rhs) const
{
    if (this == &rhs) return true;

    if (m_isDynamic == rhs.m_isDynamic && m_type == rhs.m_type)
    {
        if (!m_isDynamic)
        {
            if (m_value == rhs.m_value)
            {
                // Both not dynamic, same value.
                return true;
            }
            else
            {
                // Both not dynamic, different values.
                return false;
            }
        }
        else
        {
            // Both dynamic, will be same.
            return true;
        }
    }

    // One dynamic, not the other.
    return false;
}

DynamicProperty::DynamicProperty()
{

}

DynamicProperty::~DynamicProperty()
{

}

} // namespace OCIO_NAMESPACE


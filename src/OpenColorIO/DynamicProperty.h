/*
Copyright (c) 2019 Autodesk Inc., et al.
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

#ifndef INCLUDED_OCIO_DYNAMICPROPERTY_H
#define INCLUDED_OCIO_DYNAMICPROPERTY_H

#include <OpenColorIO/OpenColorIO.h>

OCIO_NAMESPACE_ENTER
{

class DynamicPropertyImpl;
typedef OCIO_SHARED_PTR<DynamicPropertyImpl> DynamicPropertyImplRcPtr;

// Holds a value that can be made dynamic.
class DynamicPropertyImpl : public DynamicProperty
{
public:
    DynamicPropertyImpl(double value, bool dynamic);

    double getDoubleValue() const override
    {
        return m_value;
    }
    void setValue(double value) override
    {
        m_value = value;
    }

    DynamicPropertyValueType getValueType() const override
    {
        return m_valueType;
    }

    bool isDynamic() const
    {
        return m_isDynamic;
    }
    void makeDynamic()
    {
        m_isDynamic = true;
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
    
    DynamicPropertyValueType m_valueType = DYNAMIC_PROPERTY_DOUBLE;
    double m_value = 0;
    bool m_isDynamic = false;
};

bool operator ==(const DynamicProperty &, const DynamicProperty &);

}
OCIO_NAMESPACE_EXIT

#endif
// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_OPARRAY_H
#define INCLUDED_OCIO_OPARRAY_H

#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "MathUtils.h"

namespace OCIO_NAMESPACE
{

class ArrayBase
{
public:
    ArrayBase() {}
    virtual ~ArrayBase() {}
    virtual void setDoubleValue(unsigned long index, double value) = 0;
    virtual double getDoubleValue(unsigned long index) = 0;
    virtual unsigned long getLength() const = 0;
    virtual unsigned long getNumColorComponents() const = 0;

    // Specialized in the child classes to calculate the expected number of 
    // array values based on the specified length, the interpretation of
    // length and number of components.
    virtual unsigned long getNumValues() const = 0;
};

// The CLF spec defines several ops that all contain an array (LUT1D, LUT3D,
// and Matrix). The Array class is used as a building block to implement those
// other classes. Since the dimensionality of the underlying array of those 
// classes varies, the interpretation of "length" is defined by child classes.
// The class represents the array for a 3by1D LUT and a 3D LUT or a matrix.
template<typename T> class ArrayT : public ArrayBase
{
public:
    typedef std::vector<T> Values;

public:
    ArrayT()
        : m_length(0)
        , m_numColorComponents(0)
    {
    }

    virtual ~ArrayT()
    {
    }

    ArrayT(const ArrayT&) = default;
    ArrayT& operator= (const ArrayT&) = default;

    virtual void resize(unsigned long length, unsigned long numColorComponents)
    {
        m_length = length;
        m_numColorComponents = numColorComponents;
        m_data.resize(getNumValues());
    }

    void setLength(unsigned long length)
    {
        if (m_length != length)
        {
            m_length = length;
            m_data.resize(getNumValues());
        }
    }

    void setDoubleValue(unsigned long index, double value) override
    {
        m_data[index] = (T)value;
    }

    double getDoubleValue(unsigned long index) override
    {
        return double(m_data[index]);
    }

    unsigned long getLength() const override
    {
        return m_length;
    }

    void setMaxColorComponents()
    {
        if (m_numColorComponents != getMaxColorComponents())
        {
            m_numColorComponents = getMaxColorComponents();
            m_data.resize(getNumValues());
        }
    }

    unsigned long getNumColorComponents() const override
    {
        return m_numColorComponents;
    }

    void setNumColorComponents(unsigned long numColorComponents)
    {
        if (m_numColorComponents != numColorComponents)
        {
            m_numColorComponents = numColorComponents;
            m_data.resize(getNumValues());
        }
    }

    void adjustColorComponentNumber()
    {
        if (m_numColorComponents == 3)
        {
            bool sameCoeff = true;
            for (unsigned long idx = 0; idx < m_length && sameCoeff; ++idx)
            {
                if (IsNan(m_data[idx * 3]) &&
                    IsNan(m_data[idx * 3 + 1]) &&
                    IsNan(m_data[idx * 3 + 2]))
                {
                    continue;
                }
                if (m_data[idx * 3] != m_data[idx * 3 + 1]
                    || m_data[idx * 3] != m_data[idx * 3 + 2])
                {
                    sameCoeff = false;
                    break;
                }
            }

            if (sameCoeff)
            {
                m_numColorComponents = 1;  // But keep the three values...
            }
        }
    }

    unsigned long getMaxColorComponents() const
    {
        return 3;
    }

    inline const Values& getValues() const
    {
        return m_data;
    }

    inline Values& getValues()
    {
        return m_data;
    }

    inline const T& operator[](unsigned long index) const
    {
        return m_data[index];
    }

    inline T& operator[](unsigned long index)
    {
        return m_data[index];
    }

    virtual void validate() const
    {
        if (getLength() == 0)
        {
            throw Exception("Array content is empty.");
        }

        // getNumValues is based on the dimensions claimed in the file.  Check
        // that this matches the number of values that were actually set.
        if (m_data.size() != getNumValues())
        {
            std::ostringstream os;
            os << "Array contains: " << m_data.size() << " values, ";
            os << "but " << getNumValues() << " are expected.";
            throw Exception(os.str().c_str());
        }
    }

    bool operator==(const ArrayT & a) const
    {
        if (this == &a) return true;
        return (m_length == a.m_length)
            && (m_numColorComponents == a.m_numColorComponents)
            && (m_data == a.m_data);
    }

    void scale(T scale)
    {
        if (scale != (T)1.)
        {
            const size_t nbVal = m_data.size();
            for (size_t i = 0; i < nbVal; ++i)
            {
                m_data[i] *= scale;
            }
        }
    }

protected:
    unsigned long m_length;
    unsigned long m_numColorComponents;
    Values        m_data;
};

typedef ArrayT<double> ArrayDouble;
typedef ArrayT<float> Array;

} // namespace OCIO_NAMESPACE

#endif

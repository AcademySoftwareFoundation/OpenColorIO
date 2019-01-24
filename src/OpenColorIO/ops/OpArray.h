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

#ifndef INCLUDED_OCIO_OPARRAY_H
#define INCLUDED_OCIO_OPARRAY_H

#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

OCIO_NAMESPACE_ENTER
{

// The CLF spec defines several ops that all contain an array (LUT1D, LUT3D,
// and Matrix). The Array class is used as a building block to implement those
// other classes. Since the dimensionality of the underlying array of those 
// classes varies, the interpretation of "length" is defined by child classes.
// The class represents the array for a 3by1D LUT and a 3D LUT or a matrix.
template<typename T> class ArrayT
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

    void resize(unsigned long length, unsigned long numColorComponents)
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

    void setLength(unsigned length)
    {
        if (m_length != length)
        {
            m_length = length;
            m_data.resize(getNumValues());
        }
    }

    unsigned long getLength() const
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

    unsigned long getNumColorComponents() const
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

    // Specialized in the child classes to calculate the expected number of 
    // array values based on the specified length, the interpretation of
    // length and number of components.
    virtual unsigned long getNumValues() const = 0;

protected:
    unsigned long m_length;
    unsigned long m_numColorComponents;
    Values        m_data;
};

typedef ArrayT<double> ArrayDouble;
typedef ArrayT<float> Array;

}
OCIO_NAMESPACE_EXIT

#endif

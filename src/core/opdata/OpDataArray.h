/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
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

#ifndef INCLUDED_OCIO_ARRAY_H
#define INCLUDED_OCIO_ARRAY_H

#include <OpenColorIO/OpenColorIO.h>


#include <vector>


OCIO_NAMESPACE_ENTER
{
    // Private namespace to the OpData sub-directory
    namespace OpData
    {
        class ArrayBase
        {
        public:
            ArrayBase() {}
            virtual ~ArrayBase() {}
            virtual void setDoubleValue(unsigned index, double value) = 0;
            virtual unsigned getLength() const = 0;
            virtual unsigned getNumColorComponents() const = 0;

            // Get the number of values in the array
            virtual unsigned getNumValues() const = 0;

        };
        // The class represents the array for a 3by1D LUT and a 3D LUT
        // or a matrix.
        template<typename T> class ArrayT : public ArrayBase
        {
        public:
            // Type definition for the list of all of the values of all the entries 
            // that make up the data needed to compute the color operation.
            typedef std::vector<T> Values;

        public:
            // Constructor
            ArrayT()
                : m_length(0)
                , m_numColorComponents(0)
            {
            }

            // Destructor
            virtual ~ArrayT()
            {
            }

            void resize(unsigned length, unsigned numColorComponents)
            {
                m_length = length;
                m_numColorComponents = numColorComponents;
                m_data.resize(getNumValues());
            }

            void setLength(unsigned length)
            {
                if (m_length != length)
                {
                    m_length = length;
                    m_data.resize(getNumValues());
                }
            }

            unsigned getLength() const
            {
                return m_length;
            }

            unsigned getNumColorComponents() const
            {
                return m_numColorComponents;
            }

            void setNumColorComponents(unsigned numColorComponents)
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
                    for (unsigned idx = 0; idx < m_length && sameCoeff; ++idx)
                    {
                        if (m_data[idx * 3] != m_data[idx * 3 + 1]
                            || m_data[idx * 3] != m_data[idx * 3 + 2])
                        {
                            sameCoeff = false;
                        }
                    }

                    if (sameCoeff)
                    {
                        m_numColorComponents = 1;  // But keep the three values...
                    }
                }
            }

            unsigned getMaxColorComponents() const
            {
                return 3;
            }

            const Values& getValues() const
            {
                return m_data;
            }

            Values& getValues()
            {
                return m_data;
            }

            inline const T& operator[](unsigned index) const
            {
                return m_data[index];
            }

            inline T& operator[](unsigned index)
            {
                return m_data[index];
            }

            // Validate the state of the instance
            virtual void validate() const
            {
                if (getLength() == 0)
                {
                    throw Exception("Array content is empty.");
                }

                if (m_data.size() != getNumValues())
                {
                    throw Exception(
                        "Array content does not have the expected number of values.");
                }
            }

            // Check if the array is the same
            bool operator==(const ArrayT& a) const
            {
                return (m_length == a.m_length)
                    && (m_numColorComponents == a.m_numColorComponents)
                    && (m_data == a.m_data);
            }


        protected:
            unsigned m_length;             // The size of the array
            unsigned m_numColorComponents; // The selected number of color components
            Values   m_data;               // All values
        };

        class Array : public ArrayT<float>
        {
        public:

            Array() : ArrayT<float>()
            {
            }

            virtual ~Array()
            {
            }

            virtual void setDoubleValue(unsigned index, double value)
            {
                m_data[index] = (float)value;
            }


        };

        class ArrayDouble : public ArrayT<double>
        {
        public:

            ArrayDouble() : ArrayT<double>()
            {
            }

            virtual ~ArrayDouble()
            {
            }

            virtual void setDoubleValue(unsigned index, double value)
            {
                m_data[index] = value;
            }


        };
    }
}
OCIO_NAMESPACE_EXIT


#endif

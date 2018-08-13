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


#ifndef INCLUDED_OCIO_OPDATADESCRIPTIONS_H
#define INCLUDED_OCIO_OPDATADESCRIPTIONS_H

#include <OpenColorIO/OpenColorIO.h>

#include <string>
#include <vector>

OCIO_NAMESPACE_ENTER
{
    // Private namespace to the OpData sub-directory
    namespace OpData
    {
        // Class containing one or more human readable descriptions
        class Descriptions
        {
        public:
            // Description list type
            typedef std::vector<std::string> List;

        public:
            // Constructor
            Descriptions();

            // Destructor
            ~Descriptions();

            // Assign a Color::Descriptions
            // - desc the description to copy.
            // Return the instance itself
            Descriptions& operator=(const Descriptions& rhs)
            {
                if (this != &rhs)
                {
                    m_descriptions = rhs.m_descriptions;
                }
                return *this;
            }

            // Append descriptions
            // - d is descriptions to append
            // Return itself
            Descriptions& operator +=(const Descriptions& d)
            {
                if (this != &d)
                {
                    m_descriptions.insert(m_descriptions.end(),
                                         d.m_descriptions.begin(),
                                         d.m_descriptions.end());
                }
                return *this;
            }

            // Append a human readable description
            // - d is the description to append
            // Return itself
            Descriptions& operator +=(const std::string& d)
            {
                m_descriptions.push_back(d);
                return *this;
            }

            // Check if a Descriptions is equal to this instance.
            // - other is the other Descriptions to compare with.
            // Return true if they are equal, false otherwise
            bool operator==(const Descriptions& other) const
            {
                if (this == &other) return true;
                return (m_descriptions == other.m_descriptions);
            }

            // Return the list of accumulated descriptions.
            const List& getList() const { return m_descriptions; }

        private:
            List m_descriptions; // Contains all descriptions

            // Copy constructor and assignation operator are fine
        };
    } // exit OpData namespace
}
OCIO_NAMESPACE_EXIT

#endif

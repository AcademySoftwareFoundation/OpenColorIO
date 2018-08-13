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


#ifndef INCLUDED_OCIO_CTFELEMENT_H
#define INCLUDED_OCIO_CTFELEMENT_H

#include <OpenColorIO/OpenColorIO.h>
#include <string>

OCIO_NAMESPACE_ENTER
{

// Private namespace to the CTF sub-directory
namespace CTF
{
// Private namespace for the xml reader utils
namespace Reader
{

    // Base class for all elements possible in the AMPAS common LUT
    // format XML
    class Element
    {
    public:
        // Constructor
        Element(const std::string& name,
                unsigned xmlLineNumber,
                const std::string& xmlFile
        );

        // Destructor
        virtual ~Element();

        // Start the parsing of the element
        virtual void start(const char **atts) = 0;

        // End the parsing of the element
        virtual void end() = 0;

        // Is it a container which means if it can hold other elements
        virtual bool isContainer() const = 0;

        // Get the element's name
        const std::string& getName() const
        {
            return m_name;
        }

        // Get the element's identifier
        virtual const std::string& getIdentifier() const = 0;

        // Get the element's location (line number)
        unsigned getXmLineNumber() const
        {
            return m_xmlLineNumber;
        }

        // Get the xml file name
        const std::string& getXmlFile() const;

        // Is it a dummy element
        virtual bool isDummy() const
        {
            return false;
        }

        // Get the element's type name
        virtual const std::string& getTypeName() const = 0;

        // Set the element context
        // - name is the element name
        // - xmlLineNumber is the line number in the xml file
        // - xmlFile is the xml fiel name
        void setContext(const std::string& name,
                        unsigned xmlLineNumber,
                        const std::string& xmlFile);

    protected:
        void Throw(const std::string & error) const;


    private:
        std::string m_name;       // The name
        unsigned m_xmlLineNumber; // The location
        std::string m_xmlFile;    // The xml file

    private:
        // No default constructor
        Element();

        // Copy constructor is forbidden
        Element(const Element&);

        // Operator = is forbidden
        Element& operator=(const Element&);

    };

} // exit Reader namespace


} // exit CTF namespace
}
OCIO_NAMESPACE_EXIT

#endif

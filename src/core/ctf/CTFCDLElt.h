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


#ifndef INCLUDED_OCIO_CTFCDLELT_H
#define INCLUDED_OCIO_CTFCDLELT_H

#include "CTFOpElt.h"
#include "CTFComplexElt.h"
#include "CTFPlainElt.h"

OCIO_NAMESPACE_ENTER
{

    namespace OpData
    {
        // Forward declare
        class OpData;
        class CDL;
    }


    // Private namespace to the CTF sub-directory
    namespace CTF
    {
        // Private namespace for the xml reader utils
        namespace Reader
        {

            // Class for CDL
            class CDLElt : public OpElt
            {
            public:
                // Constructor
                CDLElt();

                // Destructor
                ~CDLElt();

                // Start the parsing of the element
                virtual void start(const char **atts);

                // End the parsing of the element
                virtual void end();

                // Get the associated OpData
                OpData::OpData* getOp() const;

                // Get the associated CDL
                OpData::CDL* getCDLOp() const
                { 
                    return m_cdlOp;
                }

            protected:
                OpData::CDL* m_cdlOp; // The associated CDL
            };

            // Class for CDL v1.7
            class CDLElt_1_7 : public CDLElt
            {
            public:
                // Constructor
                CDLElt_1_7();

                // Destructor
                ~CDLElt_1_7();

                // Start the parsing of the element
                // - atts The attributes
                virtual void start(const char **atts);
            };

            // Base class for the SatNode element
            class SatNodeBaseElt : public ComplexElt
            {
            public:
                // Constructor
                SatNodeBaseElt(const std::string& name,
                               ContainerElt* pParent,
                               unsigned xmlLineNumber,
                               const std::string& xmlFile
                );

                // Start the parsing of the element
                void start(const char **atts);

                // End the parsing of the element
                void end();

                // Append a description string
                // The description is added directly to the CDLOp
                // in the parent element
                void appendDescription(const std::string& desc);

                // Get the associated CDL
                virtual OpData::CDL* getCDLOp() const = 0;

            private:
                // No default Constructor
                SatNodeBaseElt();
            };

            // Class for the SatNode element in CTF schema
            class SatNodeElt : public SatNodeBaseElt
            {
            public:
                // Constructor
                SatNodeElt(const std::string& name,
                           ContainerElt* pParent,
                           unsigned xmlLineNumber,
                           const std::string& xmlFile
                );

                // Get the associated CDL
                virtual OpData::CDL* getCDLOp() const;

            private:
                // No default Constructor
                SatNodeElt();
            };

            // Class for the CDL Saturation element
            class SaturationElt : public PlainElt
            {
            public:
                // Constructor
                SaturationElt(const std::string& name,
                             ContainerElt* pParent,
                             unsigned xmlLineNumber,
                             const std::string& xmlFile
                );

                // Destructor
                ~SaturationElt();

                // Start the parsing of the element
                void start(const char **atts);

                // End the parsing of the element
                void end();

                // Set the data's element
                // - str the string
                // - len is the string length
                // - xmlLine the location
                void setRawData(const char* str, size_t len, unsigned xmlLine);

            private:
                // No default Constructor
                SaturationElt();

                std::string m_contentData; // The tag content data
            };

            // Base class for the SOPNode element
            class SOPNodeBaseElt : public ComplexElt
            {
            public:
                // Constructor
                SOPNodeBaseElt(const std::string& name,
                               ContainerElt* pParent,
                               unsigned xmlLineNumber,
                               const std::string& xmlFile
                );

                // Start the parsing of the element
                void start(const char **atts);

                // End the parsing of the element
                void end();

                // Append a description string
                // The description is added directly to the CDLOp in
                // the parent element
                void appendDescription(const std::string& desc);

                // Get the associated CDL
                virtual OpData::CDL* getCDLOp() const = 0;

                void setIsSlopeInit(bool status) { m_isSlopeInit = status; }
                void setIsOffsetInit(bool status) { m_isOffsetInit = status; }
                void setIsPowerInit(bool status) { m_isPowerInit = status; }

            private:
                // No default Constructor
                SOPNodeBaseElt();

                bool m_isSlopeInit;
                bool m_isOffsetInit;
                bool m_isPowerInit;
            };

            // Class for the SOPNode element in CTF schema
            class SOPNodeElt : public SOPNodeBaseElt
            {
            public:
                // Constructor
                SOPNodeElt(const std::string& name,   
                           ContainerElt* pParent,     
                           unsigned xmlLineNumber,    
                           const std::string& xmlFile 
                );

                // Get the associated CDL
                virtual OpData::CDL* getCDLOp() const;

            private:
                // No default Constructor
                SOPNodeElt();

            };

            // Class for the slope, offset and power elements
            class SOPValueElt : public PlainElt
            {
            public:
                // Constructor
                SOPValueElt(const std::string& name,   
                            ContainerElt* pParent,     
                            unsigned xmlLineNumber,    
                            const std::string& xmlFile 
                );

                // Destructor
                ~SOPValueElt();

                // Start the parsing of the element
                void start(const char **atts);

                // End the parsing of the element
                void end();

                // Set the data's element
                // - str the string
                // - len is the string length
                // - xmlLine the location
                void setRawData(const char* str, size_t len, unsigned xmlLine);

            private:
                // No default Constructor
                SOPValueElt();

                std::string m_contentData; // The tag content data
            };


        } // exit Reader namespace
    } // exit CTF namespace
}
OCIO_NAMESPACE_EXIT

#endif

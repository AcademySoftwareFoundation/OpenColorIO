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

#include "CTFTransformElt.h"
#include "CTFReaderUtils.h"
#include "CTFReaderVersion.h"
#include "../Platform.h"
#include <sstream>

OCIO_NAMESPACE_ENTER
{
    // Private namespace to the OpData sub-directory
    namespace CTF
    {
        // Private namespace for the xml reader utils
        namespace Reader
        {

            TransformElt::TransformElt(const std::string& name,
                                       unsigned xmlLineNumber,
                                       const std::string& xmlFile,
                                       bool isCLF)
                : ContainerElt(name, xmlLineNumber, xmlFile)
                , m_isCLF(isCLF)
            {
                m_transform = TransformPtr(new Transform);
            }

            TransformElt::~TransformElt()
            {
            }

            const std::string& TransformElt::getIdentifier() const
            {
                return m_transform->getId();
            }

            void TransformElt::start(const char **atts)
            {
                bool isIdFound = false;
                bool isVersionFound = false;
                bool isCLFVersionFound = false;
                Version requestedVersion(0, 0);
                Version requestedCLFVersion(0, 0);

                unsigned i = 0;
                while (atts[i])
                {
                    if (0 == Platform::Strcasecmp(ATTR_ID, atts[i]))
                    {
                        if (!atts[i + 1] || !*atts[i + 1])
                        {
                            Throw("Required attribute 'id' does not have a value. ");
                        }

                        m_transform->setId(atts[i + 1]);
                        isIdFound = true;
                    }
                    else if (0 == Platform::Strcasecmp(ATTR_NAME, atts[i]))
                    {
                        if (!atts[i + 1] || !*atts[i + 1])
                        {
                            Throw("If the attribute 'name' is present, it must have a value. ");
                        }

                        m_transform->setName(atts[i + 1]);
                    }
                    else if (0 == Platform::Strcasecmp(ATTR_INVERSE_OF, atts[i]))
                    {
                        if (!atts[i + 1] || !*atts[i + 1])
                        {
                            Throw("If the attribute 'inverseOf' is present, it must have a value. ");
                        }

                        m_transform->setInverseOfId(atts[i + 1]);
                    }
                    else if (0 == Platform::Strcasecmp(ATTR_VERSION, atts[i]))
                    {
                        if (isCLFVersionFound)
                        {
                            Throw("'compCLFversion' and 'Version' cannot be both present. ");
                        }
                        if (isVersionFound)
                        {
                            Throw("'Version' can only be there once. ");
                        }

                        const char* pVer = atts[i + 1];
                        if (!pVer || !*pVer)
                        {
                            Throw("If the attribute 'version' is present, it must have a value. ");
                        }

                        try
                        {
                            const std::string verString(pVer);
                            Version::ReadVersion(verString, requestedVersion);
                        }
                        catch (Exception& ce)
                        {
                            Throw(ce.what());
                        }

                        isVersionFound = true;
                    }
                    else if (0 == Platform::Strcasecmp(ATTR_COMP_CLF_VERSION, atts[i]))
                    {
                        if (isCLFVersionFound)
                        {
                            Throw("'compCLFversion' can only be there once. ");
                        }
                        if (isVersionFound)
                        {
                            Throw("'compCLFversion' and 'Version' cannot be both present. ");
                        }

                        const char* pVer = atts[i + 1];
                        if (!pVer || !*pVer)
                        {
                            Throw("Required attribute 'compCLFversion' does not have a value. ");
                        }

                        try
                        {
                            std::string verString(pVer);
                            Version::ReadVersion(verString, requestedCLFVersion);
                        }
                        catch (Exception& ce)
                        {
                            Throw(ce.what());
                        }

                        // Translate to CTF
                        // We currently interpret CLF versions <= 2.0 as CTF version 1.7.
                        Version maxCLF(2, 0);
                        if (maxCLF < requestedCLFVersion)
                        {
                            std::ostringstream oss;
                            oss << "Unsupported transform file version '";
                            oss << pVer << "' supplied. ";
                            Throw(oss.str());
                        }

                        requestedVersion = CTF_PROCESS_LIST_VERSION_1_7;

                        isVersionFound = true;
                        isCLFVersionFound = true;
                    }

                    i += 2;
                }

                // Check mandatory elements
                if (!isIdFound)
                {
                    Throw("Required attribute 'id' is missing. ");
                }

                // Transform file format with no version means that
                // the CTF format is 1.2
                if (!isVersionFound)
                {
                    if (m_isCLF && !isCLFVersionFound)
                    {
                        Throw("Required attribute 'compCLFversion' is missing. ");
                    }
                    setVersion(CTF_PROCESS_LIST_VERSION_1_2);
                }
                else
                {
                    setVersion(requestedVersion);
                    if (m_isCLF)
                    {
                        setCLFVersion(requestedCLFVersion);
                    }
                }
            }

            void TransformElt::end()
            {
                try
                {
                    m_transform->validate();
                }
                catch (Exception& e)
                {
                    Throw(e.what());
                }
            }

            void TransformElt::appendDescription(const std::string& desc)
            {
                getTransform()->getDescriptions() += desc;
            }

            const TransformPtr& TransformElt::getTransform() const
            {
                return m_transform;
            }

            const std::string& TransformElt::getTypeName() const
            {
                static const std::string n(TAG_PROCESS_LIST);
                return n;
            }

            void TransformElt::setVersion(const Version& ver)
            {
                if (CTF_PROCESS_LIST_VERSION < ver)
                {
                    std::ostringstream oss;
                    oss << "Unsupported transform file version '";
                    oss << ver << "' supplied. ";
                    Throw(oss.str());
                }
                getTransform()->setCTFVersion(ver);
            }

            void TransformElt::setCLFVersion(const Version& ver)
            {
                getTransform()->setCLFVersion(ver);
            }

            const Version& TransformElt::getVersion() const
            {
                return getTransform()->getCTFVersion();
            }

        } // exit Reader namespace
    } // exit CTF namespace
}
OCIO_NAMESPACE_EXIT

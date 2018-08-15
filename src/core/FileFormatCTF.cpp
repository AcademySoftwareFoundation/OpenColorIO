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

#include <OpenColorIO/OpenColorIO.h>

#include "FileFormatCTF.h"

#include "FileTransform.h"

#include <cstdio>
#include <sstream>
#include <iostream>
#include <fstream>

#include "ctf/CTFReaderUtils.h"
#include "ctf/CTFTransform.h"
#include "ctf/CTFTransformElt.h"
#include "ctf/CTFDummyElt.h"
#include "ctf/CTFDescriptionElt.h"
#include "ctf/CTFCDLElt.h"
#include "ctf/CTFLut1DElt.h"
#include "ctf/CTFLut3DElt.h"
#include "ctf/CTFInfoElt.h"
#include "ctf/CTFArrayElt.h"
#include "ctf/CTFIndexMapElt.h"
#include "ctf/CTFRangeElt.h"

#include "Platform.h"
#include "pystring/pystring.h"

#include "opdata/OpData.h"


// Using eXpat for efficient XML parsing. Using eXap as a static lib
// so we need to define XML_STATIC before including the header file.
#define XML_STATIC
#include <expat.h>

/*
File parser to read CLF files.
TODO: extend to handle CTF files and to write files.
*/


OCIO_NAMESPACE_ENTER
{
    
    namespace
    {

        class LocalCachedFile : public CachedFile
        {
        public:
            LocalCachedFile ()
            {
            };
            ~LocalCachedFile() {};
            
            CTF::Reader::TransformPtr m_transform;

        };
        
        typedef OCIO_SHARED_PTR<LocalCachedFile> LocalCachedFileRcPtr;
        
        class LocalFileFormat : public FileFormat
        {
        public:
            
            ~LocalFileFormat() {};
            
            virtual void GetFormatInfo(FormatInfoVec & formatInfoVec) const;
            
            virtual CachedFileRcPtr Read(
                std::istream & istream,
                const std::string & fileName) const;
            
            virtual void BuildFileOps(OpRcPtrVec & ops,
                                      const Config& config,
                                      const ConstContextRcPtr & context,
                                      CachedFileRcPtr untypedCachedFile,
                                      const FileTransform& fileTransform,
                                      TransformDirection dir) const;
        };
        
        
        void LocalFileFormat::GetFormatInfo(FormatInfoVec & formatInfoVec) const
        {
            FormatInfo info;
            info.name = "Academy/ASC Common LUT Format";
            info.extension = "clf";
            info.capabilities = FORMAT_CAPABILITY_READ;
            formatInfoVec.push_back(info);
        }
    

        class XMLParserHelper
        {
        public:
            XMLParserHelper(const std::string & fileName)
                : m_parser(XML_ParserCreate(NULL))
                , m_fileName(fileName)
            {
                XML_SetUserData(m_parser, this);
                XML_SetElementHandler(m_parser, StartElementHandler, EndElementHandler);
                XML_SetCharacterDataHandler(m_parser, CharacterDataHandler);

                std::string root, extension;
                pystring::os::path::splitext(root, extension, m_fileName);
                m_isCLF = pystring::lower(extension) == ".clf";
            }
            ~XMLParserHelper()
            {
                XML_ParserFree(m_parser);
            }

            void Parse(std::istream & istream)
            {
                std::string line;
                m_lineNumber = 0;
                while (istream.good())
                {
                    std::getline(istream, line);
                    ++m_lineNumber;

                    Parse(line);
                }

                if (!m_elms.empty())
                {
                    std::string error("XML parsing error (no closing tag for '");
                    error += m_elms.back()->getName().c_str();
                    error += "). ";
                    Throw(error);
                }

                const CTF::Reader::TransformPtr& pT = getTransform();
                if (pT.use_count() == 0)
                {
                    static const std::string error("XML parsing error: Invalid transform. ");
                    Throw(error);
                }

                if (pT->getOps().empty())
                {
                    static const std::string error("XML parsing error: No color operator in file. ");
                    Throw(error);
                }


            }
            void Parse(const std::string & buffer)
            {
                int done = 0;

                do
                {
                    if (XML_STATUS_ERROR == XML_Parse(m_parser, buffer.c_str(), (int)buffer.size(), done))
                    {
                        XML_Error eXpatErrorCode = XML_GetErrorCode(m_parser);
                        if (eXpatErrorCode == XML_ERROR_TAG_MISMATCH)
                        {
                            if (!m_elms.empty())
                            {
                                // It could be an Op or an Attribute.
                                std::string error("XML parsing error (no closing tag for '");
                                error += m_elms.back()->getName().c_str();
                                error += "'). ";
                                Throw(error);
                            }
                            else
                            {
                                // Completely lost, something went wrong,
                                // but nothing detected with the stack.
                                static const std::string error("XML parsing error (unbalanced element tags). ");
                                Throw(error);
                            }
                        }
                        else
                        {
                            std::string error("XML parsing error: ");
                            error += XML_ErrorString(XML_GetErrorCode(m_parser));
                            Throw(error);
                        }
                    }
                } while (done);

            }

            CTF::Reader::TransformPtr getTransform()
            {
                return m_transform;
            }

        private:

            void AddOpReader(OpData::OpData::OpType type, const char * xmlTag)
            {
                if (m_elms.size() != 1)
                {
                    std::stringstream ss;
                    ss << ": The " << xmlTag;
                    ss << "'s parent can only be a Transform";

                    m_elms.push_back(new CTF::Reader::DummyElt(xmlTag,
                        (m_elms.empty() ? 0 : m_elms.back()),
                        getXmLineNumber(),
                        getXmlFilename(),
                        ss.str().c_str()));
                }
                else
                {
                    const CTF::Reader::TransformElt* pT
                        = dynamic_cast<const CTF::Reader::TransformElt*>(m_elms.back());

                    CTF::Reader::OpElt* pOp
                        = CTF::Reader::OpElt::getReader(type, pT->getVersion());

                    if (!pOp)
                    {
                        delete pOp;

                        // TODO: Rethink design of versioning in terms of major.minor
                        // For now, returning string version to avoid displaying
                        // '1.200000' instead of '1.2'
                        std::stringstream ss;
                        ss << "Unsupported transform file version '";
                        ss << pT->getVersion();
                        ss << "' for operator '" << xmlTag;
                        Throw(ss.str());
                    }

                    pOp->setContext(xmlTag, m_transform, getXmLineNumber(), getXmlFilename());

                    m_elms.push_back(pOp);
                }
            }

            void Throw(const std::string & error) const
            {
                std::ostringstream os;
                os << "Error parsing .clf file (";
                os << m_fileName.c_str() << "). ";
                os << "Error is: " << error.c_str();
                os << ". At line (" << m_lineNumber << ")";
                throw Exception(os.str().c_str());
            }

            XMLParserHelper() {};

            // Start the parsing of one element
            static void StartElementHandler(void *userData,
                const XML_Char *name,
                const XML_Char **atts)
            {
                XMLParserHelper * pImpl = (XMLParserHelper*)userData;

                if (!pImpl || !name || !*name)
                {
                    if (!pImpl)
                    {
                        throw Exception("Internal CLF parser error.");
                    }
                    else
                    {
                        pImpl->Throw("Internal CLF parser error. ");
                    }
                }

                if (!pImpl->m_elms.empty())
                {
                    // Check if we are still processing a metadata structure
                    CTF::Reader::MetadataElt* pElt 
                        = dynamic_cast<CTF::Reader::MetadataElt*>(pImpl->m_elms.back());
                    if (pElt)
                    {
                        pImpl->m_elms.push_back(
                            new CTF::Reader::MetadataElt(name,
                                pElt,
                                pImpl->m_lineNumber,
                                pImpl->m_fileName));

                        pImpl->m_elms.back()->start(atts);
                        return;
                    }
                }

                // Handle the ProcessList element or its children (the ops).

                if (0 == Platform::Strcasecmp(name, CTF::TAG_PROCESS_LIST))
                {
                    if (pImpl->m_transform.get())
                    {
                        CTF::Reader::TransformElt* pT
                            = dynamic_cast<CTF::Reader::TransformElt*>(pImpl->m_elms.front());

                        pImpl->m_elms.push_back(
                            new CTF::Reader::DummyElt(name, pT,
                                pImpl->getXmLineNumber(),
                                pImpl->getXmlFilename(),
                                ": The Color::Transform already exists"));
                    }
                    else
                    {
                        CTF::Reader::TransformElt* pT
                            = new CTF::Reader::TransformElt(
                                name,
                                pImpl->getXmLineNumber(),
                                pImpl->getXmlFilename(),
                                pImpl->IsCLF());

                        pImpl->m_elms.push_back(pT);

                        pImpl->m_transform = pT->getTransform();
                    }
                }

                // Handle all Ops
                else if (0 == Platform::Strcasecmp(name, CTF::TAG_MATRIX))
                {
                    pImpl->AddOpReader(OpData::OpData::MatrixType, name);
                }
                else if (0 == Platform::Strcasecmp(name, CTF::TAG_LUT1D))
                {
                    pImpl->AddOpReader(OpData::OpData::Lut1DType, name);
                }
                else if (0 == Platform::Strcasecmp(name, CTF::TAG_INVLUT1D))
                {
                    pImpl->AddOpReader(OpData::OpData::InvLut1DType, name);
                }
                else if (0 == Platform::Strcasecmp(name, CTF::TAG_LUT3D))
                {
                    pImpl->AddOpReader(OpData::OpData::Lut3DType, name);
                }
                else if (0 == Platform::Strcasecmp(name, CTF::TAG_INVLUT3D))
                {
                    pImpl->AddOpReader(OpData::OpData::InvLut3DType, name);
                }
                /* TODO: code copied from syncolor for ctf support
                else if (0 == Platform::Strcasecmp(name, CTF::TAG_REFERENCE))
                {
                    pImpl->_addOpReader(Color::Op::Reference, name);
                }
                */
                else if (0 == Platform::Strcasecmp(name, CTF::TAG_RANGE))
                {
                    pImpl->AddOpReader(OpData::OpData::RangeType, name);
                }
                /* TODO: code copied from syncolor for ctf support
                else if (0 == Platform::Strcasecmp(name, CTF::TAG_GAMMA))
                {
                    pImpl->_addOpReader(Color::Op::Gamma, name);
                }
                else if (0 == Platform::Strcasecmp(name, CTF::TAG_EXPOSURE_CONTRAST))
                {
                    pImpl->_addOpReader(Color::Op::ExposureContrast, name);
                }
                else if (0 == Platform::Strcasecmp(name, CTF::TAG_LOG))
                {
                    pImpl->_addOpReader(Color::Op::Log, name);
                }*/
                else if (0 == Platform::Strcasecmp(name, CTF::TAG_CDL))
                {
                    pImpl->AddOpReader(OpData::OpData::CDLType, name);
                }
                /* TODO: code copied from syncolor for ctf support
                else if (0 == Platform::Strcasecmp(name, CTF::TAG_DITHER))
                {
                    pImpl->_addOpReader(Color::Op::Dither, name);
                }
                else if (0 == Platform::Strcasecmp(name, CTF::TAG_GAMUT_MAP))
                {
                    pImpl->_addOpReader(Color::Op::GamutMap, name);
                }
                else if (0 == Platform::Strcasecmp(name, CTF::TAG_HUE_VECTOR))
                {
                    pImpl->_addOpReader(Color::Op::HueVector, name);
                }
                else if (0 == Platform::Strcasecmp(name, CTF::TAG_ACES))
                {
                    pImpl->_addOpReader(Color::Op::ACES, name);
                }
                else if (0 == Platform::Strcasecmp(name, CTF::TAG_FUNCTION))
                {
                    pImpl->_addOpReader(Color::Op::Function, name);
                }
                */
                // Handle other elements that are transform-level metadata or parts of ops.

                else
                {
                    CTF::Reader::Element* pElt = pImpl->m_elms.size() ? pImpl->m_elms.back() : 0x0;

                    CTF::Reader::ContainerElt* pContainer 
                        = dynamic_cast<CTF::Reader::ContainerElt*>(pElt);
                    if (!pContainer)
                    {
                        pImpl->m_elms.push_back(
                            new CTF::Reader::DummyElt(name,
                                pElt,
                                pImpl->getXmLineNumber(),
                                pImpl->getXmlFilename(),
                                0x0));
                    }

                    else if (0 == Platform::Strcasecmp(name, CTF::TAG_INFO))
                    {
                        CTF::Reader::TransformElt* pTransformElt 
                            = dynamic_cast<CTF::Reader::TransformElt*>(pElt);
                        if (pTransformElt)
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::InfoElt(name,
                                    pContainer,
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename()));
                        }
                        else
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::DummyElt(name,
                                (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename(),
                                    ": Info not allowed in this element"));
                        }
                    }

                    else if (0 == Platform::Strcasecmp(name, CTF::TAG_DESCRIPTION))
                    {
                        pImpl->m_elms.push_back(
                            new CTF::Reader::DescriptionElt(name,
                                pContainer,
                                pImpl->getXmLineNumber(),
                                pImpl->getXmlFilename()));
                    }


                    else if (0 == Platform::Strcasecmp(name, CTF::TAG_INPUT_DESCRIPTOR))
                    {
                        CTF::Reader::TransformElt* pTransform
                            = dynamic_cast<CTF::Reader::TransformElt*>(pContainer);

                        if (!pTransform)
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::DummyElt(name,
                                (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename(),
                                    ": InputDescriptor not allowed in this element"));
                        }
                        else
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::InputDescriptorElt(name,
                                    pContainer,
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename()));
                        }
                    }


                    else if (0 == Platform::Strcasecmp(name, CTF::TAG_OUTPUT_DESCRIPTOR))
                    {
                        CTF::Reader::TransformElt* pTransform
                            = dynamic_cast<CTF::Reader::TransformElt*>(pContainer);

                        if (!pTransform)
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::DummyElt(name,
                                (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename(),
                                    ": OutputDescriptor not allowed in this element"));
                        }
                        else
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::OutputDescriptorElt(name,
                                    pContainer,
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename()));
                        }
                    }
                    
                    else if (0 == Platform::Strcasecmp(name, CTF::TAG_ARRAY))
                    {
                        CTF::Reader::ArrayMgt* pA
                            = dynamic_cast<CTF::Reader::ArrayMgt*>(pContainer);
                        if (!pA || pA->isCompleted())
                        {
                            if (!pA)
                            {
                                pImpl->m_elms.push_back(
                                    new CTF::Reader::DummyElt(name,
                                    (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                        pImpl->getXmLineNumber(),
                                        pImpl->getXmlFilename(),
                                        ": Color::Array not allowed in this element"));
                            }
                            else
                            {
                                pImpl->m_elms.push_back(
                                    new CTF::Reader::DummyElt(name,
                                    (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                        pImpl->getXmLineNumber(),
                                        pImpl->getXmlFilename(),
                                        ": Only one Color::Array allowed per op"));
                            }
                        }
                        else
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::ArrayElt(name, pContainer,
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename()));
                        }
                    }

                    
                    else if (0 == Platform::Strcasecmp(name, CTF::TAG_INDEX_MAP))
                    {
                        CTF::Reader::IndexMapMgt* pA
                            = dynamic_cast<CTF::Reader::IndexMapMgt*>(pContainer);
                        if (!pA || pA->isCompletedIM())
                        {
                            if (!pA)
                            {
                                pImpl->m_elms.push_back(
                                    new CTF::Reader::DummyElt(name,
                                    (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                        pImpl->getXmLineNumber(),
                                        pImpl->getXmlFilename(),
                                        ": Color::IndexMap not allowed in this element"));
                            }
                            else
                            {
                                // Currently only support a single IndexMap per LUT.
                                pImpl->Throw("Only one IndexMap allowed per LUT. ");
                            }
                        }
                        else
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::IndexMapElt(name, pContainer,
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename()));
                        }
                    }
                    
                    else if (0 == Platform::Strcasecmp(name, CTF::TAG_MIN_IN_VALUE) ||
                        0 == Platform::Strcasecmp(name, CTF::TAG_MAX_IN_VALUE) ||
                        0 == Platform::Strcasecmp(name, CTF::TAG_MIN_OUT_VALUE) ||
                        0 == Platform::Strcasecmp(name, CTF::TAG_MAX_OUT_VALUE))
                    {
                        CTF::Reader::RangeElt* pRange
                            = dynamic_cast<CTF::Reader::RangeElt*>(pContainer);

                        if (!pRange)
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::DummyElt(name,
                                (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename(),
                                    ": Range Value tags not allowed in this element"));
                        }
                        else
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::RangeValueElt(name,
                                    pContainer,
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename()));
                        }
                    }
                    /* TODO: code copied from syncolor for ctf support
                    else if (0 == Platform::Strcasecmp(name, CTF::TAG_GAMMA_PARAMS))
                    {
                        CTF::Reader::GammaElt* pGamma
                            = dynamic_cast<CTF::Reader::GammaElt*>(pContainer);

                        if (!pGamma)
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::DummyElt(name,
                                (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename(),
                                    ": Gamma Params not allowed in this element"));
                        }
                        else
                        {
                            pImpl->m_elms.push_back(
                                pGamma->createGammaParamsElt(name,
                                    pContainer,
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename()));
                        }
                    }

                    else if (0 == Platform::Strcasecmp(name, CTF::TAG_EC_PARAMS))
                    {
                        CTF::Reader::ExposureContrastElt* pEC
                            = dynamic_cast<CTF::Reader::ExposureContrastElt*>(pContainer);

                        if (!pEC)
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::DummyElt(name,
                                (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename(),
                                    ": E/C Params not allowed in this element"));
                        }
                        else
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::ECParamsElt(name,
                                    pContainer,
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename()));
                        }
                    }

                    else if (0 == Platform::Strcasecmp(name, CTF::TAG_DYNAMIC_PARAMETER))
                    {
                        CTF::Reader::OpElt* pOpElt = dynamic_cast<CTF::Reader::OpElt*>(pContainer);

                        if (!pOpElt)
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::DummyElt(name,
                                (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename(),
                                    ": Dynamic properties not allowed in this element"));
                        }
                        else
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::DynamicParamElt(name,
                                    pContainer,
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename()));
                        }
                    }

                    else if (0 == Platform::Strcasecmp(name, CTF::TAG_LOG_PARAMS))
                    {
                        CTF::Reader::LogElt* pLog
                            = dynamic_cast<CTF::Reader::LogElt*>(pContainer);

                        if (!pLog ||
                            !(pLog->getLog()->getLogStyle() == Color::LogOp::LOG_TO_LIN ||
                                pLog->getLog()->getLogStyle() == Color::LogOp::LIN_TO_LOG))
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::DummyElt(name,
                                (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename(),
                                    ": Log Params not allowed in this element"));
                        }
                        else
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::LogParamsElt(name,
                                    pContainer,
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename()));
                        }
                    }

                    else if (0 == Platform::Strcasecmp(name, CTF::TAG_HUE_VECTOR_PARAMS))
                    {
                        CTF::Reader::HueVectorElt* pHueVector
                            = dynamic_cast<CTF::Reader::HueVectorElt*>(pContainer);

                        if (!pHueVector)
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::DummyElt(name,
                                (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename(),
                                    ": HueVector Params not allowed in this element"));
                        }
                        else
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::HueVectorParamsElt(name,
                                    pContainer,
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename()));
                        }
                    }

                    else if (0 == Platform::Strcasecmp(name, CTF::TAG_ACES_PARAMS))
                    {
                        CTF::Reader::ACESElt* pACES
                            = dynamic_cast<CTF::Reader::ACESElt*>(pContainer);

                        if (!pACES)
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::DummyElt(name,
                                (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename(),
                                    ": ACES Params not allowed in this element"));
                        }
                        else
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::ACESParamsElt(name,
                                    pContainer,
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename()));
                        }
                    }
                    */
                    else if (0 == Platform::Strcasecmp(name, CTF::TAG_SOPNODE))
                    {
                        CTF::Reader::CDLElt* pCDL = dynamic_cast<CTF::Reader::CDLElt*>(pContainer);

                        if (!pCDL)
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::DummyElt(name,
                                (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename(),
                                    ": SOPNode not allowed in this element"));
                        }
                        else
                        {
                            CTF::Reader::SOPNodeElt* sopNodeElt = new CTF::Reader::SOPNodeElt(name,
                                pCDL,
                                pImpl->getXmLineNumber(),
                                pImpl->getXmlFilename());
                            pImpl->m_elms.push_back(sopNodeElt);
                        }
                    }

                    else if (0 == Platform::Strcasecmp(name, CTF::TAG_SLOPE) ||
                        0 == Platform::Strcasecmp(name, CTF::TAG_OFFSET) ||
                        0 == Platform::Strcasecmp(name, CTF::TAG_POWER))
                    {
                        CTF::Reader::SOPNodeElt* pSOPNode = dynamic_cast<CTF::Reader::SOPNodeElt*>(pContainer);

                        if (!pSOPNode)
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::DummyElt(name,
                                (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename(),
                                    ": Slope, Offset or Power tags not allowed in this element"));
                        }
                        else
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::SOPValueElt(name,
                                    pContainer,
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename()));
                        }
                    }

                    else if (0 == Platform::Strcasecmp(name, CTF::TAG_SATNODE))
                    {
                        CTF::Reader::CDLElt* pCDL = dynamic_cast<CTF::Reader::CDLElt*>(pContainer);

                        if (!pCDL)
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::DummyElt(name,
                                (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename(),
                                    ": SatNode not allowed in this element"));
                        }
                        else
                        {
                            CTF::Reader::SatNodeElt* satNodeElt = new CTF::Reader::SatNodeElt(name,
                                pCDL,
                                pImpl->getXmLineNumber(),
                                pImpl->getXmlFilename());
                            pImpl->m_elms.push_back(satNodeElt);
                        }
                    }

                    else if (0 == Platform::Strcasecmp(name, CTF::TAG_SATURATION))
                    {
                        CTF::Reader::SatNodeElt* pSatNode = dynamic_cast<CTF::Reader::SatNodeElt*>(pContainer);

                        if (!pSatNode)
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::DummyElt(name,
                                (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename(),
                                    ": Saturation not allowed in this element"));
                        }
                        else
                        {
                            pImpl->m_elms.push_back(
                                new CTF::Reader::SaturationElt(name,
                                    pContainer,
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename()));
                        }
                    }

                    else
                    {
                        pImpl->m_elms.push_back(
                            new CTF::Reader::DummyElt(name,
                            (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                pImpl->getXmLineNumber(),
                                pImpl->getXmlFilename(),
                                ": Unknown element"));
                    }
                }

                pImpl->m_elms.back()->start(atts);
            }

            // End the parsing of one element
            static void EndElementHandler(void *userData,
                const XML_Char *name)
            {
                XMLParserHelper * pImpl = (XMLParserHelper*)userData;
                if (!pImpl || !name || !*name)
                {
                    throw Exception("XML internal parsing error.");
                }

                // Is the expected element present ?
                std::auto_ptr<CTF::Reader::Element> pElt(pImpl->m_elms.back());
                if (!pElt.get())
                {
                    pImpl->Throw("XML parsing error: Tag is missing. ");
                }

                // Is it the expected element ?
                if (pElt->getName() != name)
                {
                    std::stringstream ss;
                    ss << "XML parsing error: Tag '";
                    ss << (name ? name : "");
                    ss << "' is missing";
                    pImpl->Throw(ss.str());
                }

                if (pElt->isDummy())
                {
                    pImpl->m_elms.pop_back();
                }
                else if (pElt->isContainer())
                {
                    pImpl->m_elms.pop_back();
                }
                else
                {
                    // Is it a plain element ?
                    CTF::Reader::PlainElt* pPlainElt =
                        dynamic_cast<CTF::Reader::PlainElt*>(pElt.get());
                    if (!pPlainElt)
                    {
                        std::stringstream ss;
                        ss << "XML parsing error: Attribute end '";
                        ss << (name ? name : "");
                        ss << "' is illegal. ";
                        pImpl->Throw(ss.str());
                    }

                    pImpl->m_elms.pop_back();

                    CTF::Reader::Element* pParent = pImpl->m_elms.back();

                    // Is it at the right location in the stack ?
                    if (!pParent || !pParent->isContainer() ||
                        pParent != pPlainElt->getParent())
                    {
                        std::stringstream ss;
                        ss << "XML parsing error: Tag '";
                        ss << (name ? name : "");
                        ss << "'.";
                        pImpl->Throw(ss.str());
                    }
                }

                pElt->end();
            }

            // Handle of strings within an element
            static void CharacterDataHandler(void *userData,
                const XML_Char *s, 
                int len)
            {
                XMLParserHelper * pImpl = (XMLParserHelper*)userData;
                if (!pImpl)
                {
                    throw Exception("XML internal parsing error.");
                }

                if (len == 0) return;

                if (len<0 || !s || !*s)
                {
                    pImpl->Throw("XML parsing error: attribute illegal. ");
                }

                CTF::Reader::Element* pElt = pImpl->m_elms.back();
                if (!pElt)
                {
                    std::ostringstream oss;
                    oss << "XML parsing error: missing end tag '";
                    oss << std::string(s, len).c_str();
                    oss << "'.";
                    pImpl->Throw(oss.str());
                }

                CTF::Reader::DescriptionElt* pDescriptionElt = dynamic_cast<CTF::Reader::DescriptionElt*>(pElt);
                if (pDescriptionElt)
                {
                    pDescriptionElt->setRawData(s, len, pImpl->getXmLineNumber());
                }
                else
                {

                    // Strip white spaces
                    size_t start = 0;
                    size_t end = len;
                    CTF::Reader::FindSubString(s, len, start, end);

                    if (end>0)
                    {
                        // Metadata element is a special element processor: It is used to process container
                        // elements, but it is also used to process the terminal/plain elements.
                        CTF::Reader::MetadataElt* pMetadataElt = dynamic_cast<CTF::Reader::MetadataElt*>(pElt);
                        if (pMetadataElt)
                        {
                            pMetadataElt->setRawData(s + start, end - start, pImpl->getXmLineNumber());
                        }
                        else
                        {
                            if (pElt->isContainer())
                            {
                                std::ostringstream oss;
                                oss << "XML parsing error: attribute illegal '";
                                oss << std::string(s, len).c_str();
                                oss << "'.";
                                pImpl->Throw(oss.str());
                            }

                            CTF::Reader::PlainElt* pPlainElt = dynamic_cast<CTF::Reader::PlainElt*>(pElt);
                            if (!pPlainElt)
                            {
                                std::ostringstream oss;
                                oss << "XML parsing error: attribute illegal '";
                                oss << std::string(s, len).c_str();
                                oss << "'.";
                                pImpl->Throw(oss.str());
                            }
                            pPlainElt->setRawData(s + start, end - start, pImpl->getXmLineNumber());
                        }
                    }
                }
            }

            unsigned getXmLineNumber() const
            {
                return m_lineNumber;
            }

            const std::string& getXmlFilename() const
            {
                return m_fileName;
            }

            bool IsCLF() const
            {
                return m_isCLF;
            }

            XML_Parser m_parser;
            unsigned m_lineNumber;
            std::string m_fileName;
            bool m_isCLF;
            CTF::Reader::ElementStack m_elms; // Parsing stack
            CTF::Reader::TransformPtr m_transform;

        };

        // Try and load the format
        // Raise an exception if it can't be loaded.
        
        CachedFileRcPtr LocalFileFormat::Read(
            std::istream & istream,
            const std::string & fileName) const
        {
            XMLParserHelper parser(fileName);
            parser.Parse(istream);

            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());

            // keep transform
            cachedFile->m_transform = parser.getTransform();

            return cachedFile;
        }

        void
        LocalFileFormat::BuildFileOps(OpRcPtrVec & ops,
                                      const Config& /*config*/,
                                      const ConstContextRcPtr & /*context*/,
                                      CachedFileRcPtr untypedCachedFile,
                                      const FileTransform& fileTransform,
                                      TransformDirection dir) const
        {
            LocalCachedFileRcPtr cachedFile = DynamicPtrCast<LocalCachedFile>(untypedCachedFile);
            
            // This should never happen.
            if(!cachedFile)
            {
                std::ostringstream os;
                os << "Cannot build clf Ops. Invalid cache type.";
                throw Exception(os.str().c_str());
            }
            
            const TransformDirection newDir 
                = CombineTransformDirections(dir, fileTransform.getDirection());

            if(newDir == TRANSFORM_DIR_UNKNOWN)
            {
                std::ostringstream os;
                os << "Cannot build file format transform,";
                os << " unspecified transform direction.";
                throw Exception(os.str().c_str());
            }

            const OpData::OpDataVec & allOpData = cachedFile->m_transform->getOps();
            CreateOpVecFromOpDataVec(ops, allOpData, newDir);
        }
    }
    
    FileFormat * CreateFileFormatCLF()
    {
        return new LocalFileFormat();
    }

    LocalCachedFileRcPtr LoadFile(const std::string & filePathName)
    {
        // Open the filePath
        std::ifstream filestream;
        filestream.open(filePathName.c_str(), std::ios_base::in);

        if (!filestream.is_open())
        {
            std::string err("Could not open file: ");
            err += filePathName;

            throw Exception(err.c_str());
        }
        
        std::string root, extension, name;
        pystring::os::path::splitext(root, extension, filePathName);

        name = pystring::os::path::basename(root);
        name += extension;

        // Read file
        LocalFileFormat tester;
        CachedFileRcPtr cachedFile = tester.Read(filestream, name);

        return DynamicPtrCast<LocalCachedFile>(cachedFile);
    }

#ifdef OCIO_UNIT_TEST

#define _STR(x) #x
#define STR(x) _STR(x)

    static const std::string ocioTestFilesDir(STR(OCIO_UNIT_TEST_FILES_DIR));

    CTF::Reader::TransformPtr LoadCTFTestFile(const std::string & fileName)
    {
        std::string filePath(ocioTestFilesDir);
        filePath += "/";
        filePath += fileName;

        return LoadFile(filePath)->m_transform;
    }

#endif
}
OCIO_NAMESPACE_EXIT



#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

#include "opdata/OpDataCDL.h"
#include "opdata/OpDataMatrix.h"
#include "opdata/OpDataRange.h"
#include "opdata/OpDataLut1D.h"
#include "opdata/OpDataInvLut1D.h"
#include "opdata/OpDataInvLut3D.h"

#include "MathUtils.h"

#ifndef OCIO_UNIT_TEST_FILES_DIR
#error Expecting OCIO_UNIT_TEST_FILES_DIR to be defined for tests. Check relevant CMakeLists.txt
#endif

OCIO::LocalCachedFileRcPtr GetFile(const std::string & fileName)
{
    std::string filePath(OCIO::ocioTestFilesDir);
    filePath += "/";
    filePath += fileName;

    return OCIO::LoadFile(filePath);
}

OIIO_ADD_TEST(FileFormatCTF, MissingFile)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        // Test the tests helper function
        const std::string ctfFile("xxxxxxxxxxxxxxxxx.xxxxx");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "Could not open file");
    }
}

OIIO_ADD_TEST(FileFormatCTF, WrongFormat)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("logtolin_8to8.lut");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "syntax error");
        OIIO_CHECK_ASSERT(!(bool)cachedFile);
    }
}

OIIO_ADD_TEST(FileFormatCTF, CLFSpec)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("lut1d_example.clf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getName(), "transform example lut1d");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getId(), "exlut1");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions().getList().size(), 1);
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions().getList()[0],
            " Turn 4 grey levels into 4 inverted codes using a 1D ");
        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::Lut1DType);
        OIIO_CHECK_EQUAL(opList[0]->getName(), "4valueLut");
        OIIO_CHECK_EQUAL(opList[0]->getId(), "lut-23");
        OIIO_CHECK_EQUAL(opList[0]->getInputBitDepth(), OCIO::BIT_DEPTH_UINT12);
        OIIO_CHECK_EQUAL(opList[0]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);
        OIIO_CHECK_EQUAL(opList[0]->getDescriptions().getList().size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getDescriptions().getList()[0],
            " 1D LUT ");
    }

    {
        const std::string ctfFile("lut3d_identity_32f.clf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getName(), "transform example lut3d");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getId(), "exlut2");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions().getList().size(), 1);
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions().getList()[0],
            " 3D LUT example from spec ");
        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::Lut3DType);
        OIIO_CHECK_EQUAL(opList[0]->getName(), "identity");
        OIIO_CHECK_EQUAL(opList[0]->getId(), "lut-24");
        OIIO_CHECK_EQUAL(opList[0]->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_EQUAL(opList[0]->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_EQUAL(opList[0]->getDescriptions().getList().size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getDescriptions().getList()[0],
            " 3D LUT ");
    }

    {
        const std::string ctfFile("matrix_3x4_example.clf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getName(), "transform example matrix");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getId(), "exmat1");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions().getList().size(), 2);
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions().getList()[0],
            " Matrix example from spec ");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions().getList()[1],
            " Used by unit tests ");
        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::MatrixType);
        OIIO_CHECK_EQUAL(opList[0]->getName(), "colorspace conversion");
        OIIO_CHECK_EQUAL(opList[0]->getId(), "mat-25");
        OIIO_CHECK_EQUAL(opList[0]->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_EQUAL(opList[0]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_EQUAL(opList[0]->getDescriptions().getList().size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getDescriptions().getList()[0],
            " 3x4 Matrix , 4th column is offset ");
    }

    {
        // Test two-entries IndexMap support.
        const std::string ctfFile("lut1d_shaper_example.clf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getName(), "transform example lut shaper");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getId(), "exlut3");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions().getList().size(), 1);
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions().getList()[0],
                         " Shaper LUT example from spec ");
        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 2);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::RangeType);
        const OCIO::OpData::Range * pR = dynamic_cast<const OCIO::OpData::Range*>(opList[0]);
        OIIO_CHECK_ASSERT(pR);

        OIIO_CHECK_EQUAL(pR->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_EQUAL(pR->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

        OIIO_CHECK_EQUAL(pR->getMinInValue(), 64.);
        OIIO_CHECK_EQUAL(pR->getMaxInValue(), 940.);
        OIIO_CHECK_EQUAL(pR->getMinOutValue(), 0.);
        OIIO_CHECK_EQUAL(pR->getMaxOutValue(), 1023.);

        OIIO_CHECK_EQUAL(opList[1]->getOpType(), OCIO::OpData::OpData::Lut1DType);
        OIIO_CHECK_EQUAL(opList[1]->getName(), "shaper LUT");
        OIIO_CHECK_EQUAL(opList[1]->getId(), "lut-26");
        OIIO_CHECK_EQUAL(opList[1]->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_EQUAL(opList[1]->getOutputBitDepth(), OCIO::BIT_DEPTH_F16);
        OIIO_CHECK_EQUAL(opList[1]->getDescriptions().getList().size(), 1);
        OIIO_CHECK_EQUAL(opList[1]->getDescriptions().getList()[0],
                         " 1D LUT with shaper ");
    }
}

OIIO_ADD_TEST(FileFormatCTF, Lut1D)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("lut1d_example.ctf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getName(), "1d-lut example");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getId(), "9843a859-e41e-40a8-a51c-840889c3774e");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions().getList().size(), 1);
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions().getList()[0],
            "Apply a 1/2.2 gamma.");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getInputDescriptor(), "RGB");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getOutputDescriptor(), "RGB");
        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 1);

        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::Lut1DType);
        const OCIO::OpData::Lut1D * pLut = dynamic_cast<const OCIO::OpData::Lut1D *>(opList[0]);
        OIIO_CHECK_ASSERT(pLut);

        OIIO_CHECK_EQUAL(pLut->getDescriptions().getList().size(), 1);

        OIIO_CHECK_ASSERT(!pLut->isInputHalfDomain());
        OIIO_CHECK_ASSERT(!pLut->isOutputRawHalfs());
        OIIO_CHECK_ASSERT(pLut->getHueAdjust() == OCIO::OpData::Lut1D::HUE_NONE);

        OIIO_CHECK_ASSERT(pLut->getInputBitDepth() == OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_ASSERT(pLut->getOutputBitDepth() == OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_ASSERT(pLut->getName() == "1d-lut example op");

        // TODO: bypass is for CTF
        // OIIO_CHECK_ASSERT(!pLut->getBypass()->isDynamic());

        const OCIO::OpData::Array& array = pLut->getArray();
        OIIO_CHECK_ASSERT(array.getLength() == 32);
        OIIO_CHECK_ASSERT(array.getNumColorComponents() == 1);
        OIIO_CHECK_ASSERT(array.getNumValues()
            == array.getLength()*pLut->getArray().getMaxColorComponents());

        OIIO_CHECK_ASSERT(array.getValues().size() == 96);
        OIIO_CHECK_EQUAL(array.getValues()[0], 0.0f);
        OIIO_CHECK_EQUAL(array.getValues()[1], 0.0f);
        OIIO_CHECK_EQUAL(array.getValues()[2], 0.0f);
        OIIO_CHECK_EQUAL(array.getValues()[3], 215.0f);
        OIIO_CHECK_EQUAL(array.getValues()[4], 215.0f);
        OIIO_CHECK_EQUAL(array.getValues()[5], 215.0f);
        OIIO_CHECK_EQUAL(array.getValues()[6], 294.0f);
        // and many more
        OIIO_CHECK_EQUAL(array.getValues()[92], 1008.0f);
        OIIO_CHECK_EQUAL(array.getValues()[93], 1023.0f);
        OIIO_CHECK_EQUAL(array.getValues()[94], 1023.0f);
        OIIO_CHECK_EQUAL(array.getValues()[95], 1023.0f);
    }

    // Test the new hue adjust attribute.
    {
        const std::string ctfFile("lut1d_hue_adjust_test.ctf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::Lut1DType);
        const OCIO::OpData::Lut1D * pLut = dynamic_cast<const OCIO::OpData::Lut1D *>(opList[0]);
        OIIO_CHECK_ASSERT(pLut);
        OIIO_CHECK_ASSERT(pLut->getHueAdjust() == OCIO::OpData::Lut1D::HUE_DW3);
    }
}

OIIO_ADD_TEST(FileFormatCTF, Matrix)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("matrix_example.clf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::MatrixType);
        const OCIO::OpData::Matrix * pMatrix = dynamic_cast<const OCIO::OpData::Matrix *>(opList[0]);
        OIIO_CHECK_ASSERT(pMatrix);

        OIIO_CHECK_ASSERT(cachedFile->m_transform->getInputDescriptor() == "XYZ");
        OIIO_CHECK_ASSERT(cachedFile->m_transform->getOutputDescriptor() == "RGB");

        OIIO_CHECK_ASSERT(pMatrix->getInputBitDepth() == OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_ASSERT(pMatrix->getOutputBitDepth() == OCIO::BIT_DEPTH_F32);

        const OCIO::OpData::ArrayDouble& array = pMatrix->getArray();
        OIIO_CHECK_ASSERT(array.getLength() == 4);
        OIIO_CHECK_ASSERT(array.getNumColorComponents() == 4);
        OIIO_CHECK_ASSERT(array.getNumValues() ==
            array.getLength()*array.getLength());

        OIIO_CHECK_EQUAL(array.getValues().size(), array.getNumValues());
        OIIO_CHECK_EQUAL(array.getValues()[0],  3.24);
        OIIO_CHECK_EQUAL(array.getValues()[1], -1.537);
        OIIO_CHECK_EQUAL(array.getValues()[2], -0.49850);
        OIIO_CHECK_EQUAL(array.getValues()[3],  0.0);

        OIIO_CHECK_EQUAL(array.getValues()[4], -0.96930);
        OIIO_CHECK_EQUAL(array.getValues()[5],  1.876);
        OIIO_CHECK_EQUAL(array.getValues()[6],  0.04156);
        OIIO_CHECK_EQUAL(array.getValues()[7],  0.0);

        OIIO_CHECK_EQUAL(array.getValues()[8],  0.05560);
        OIIO_CHECK_EQUAL(array.getValues()[9], -0.204);
        OIIO_CHECK_EQUAL(array.getValues()[10], 1.0573);
        OIIO_CHECK_EQUAL(array.getValues()[11], 0.0);

        OIIO_CHECK_EQUAL(array.getValues()[12], 0.0);
        OIIO_CHECK_EQUAL(array.getValues()[13], 0.0);
        OIIO_CHECK_EQUAL(array.getValues()[14], 0.0);
        OIIO_CHECK_EQUAL(array.getValues()[15], 1.0);

        const OCIO::OpData::Matrix::Offsets& offsets = pMatrix->getOffsets();
        OIIO_CHECK_EQUAL(offsets[0], 0.0);
        OIIO_CHECK_EQUAL(offsets[1], 0.0);
        OIIO_CHECK_EQUAL(offsets[2], 0.0);
        OIIO_CHECK_EQUAL(offsets[3], 0.0);
    }
}

OIIO_ADD_TEST(FileFormatCTF, Matrix4x4)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("matrix_example4x4.ctf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        const OCIO::CTF::Version ctfVersion = cachedFile->m_transform->getCTFVersion();
        OIIO_CHECK_ASSERT(OCIO::CTF::CTF_PROCESS_LIST_VERSION_1_2 == ctfVersion);

        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::MatrixType);
        const OCIO::OpData::Matrix * pMatrix = dynamic_cast<const OCIO::OpData::Matrix *>(opList[0]);
        OIIO_CHECK_ASSERT(pMatrix);

        OIIO_CHECK_ASSERT(cachedFile->m_transform->getInputDescriptor() == "XYZ");
        OIIO_CHECK_ASSERT(cachedFile->m_transform->getOutputDescriptor() == "RGB");

        OIIO_CHECK_ASSERT(pMatrix->getInputBitDepth() == OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_ASSERT(pMatrix->getOutputBitDepth() == OCIO::BIT_DEPTH_F32);

        const OCIO::OpData::ArrayDouble& array = pMatrix->getArray();
        OIIO_CHECK_ASSERT(array.getLength() == 4);
        OIIO_CHECK_ASSERT(array.getNumColorComponents() == 4);
        OIIO_CHECK_ASSERT(array.getNumValues() ==
            array.getLength()*array.getLength());

        OIIO_CHECK_EQUAL(array.getValues().size(), array.getNumValues());
        OIIO_CHECK_EQUAL(array.getValues()[0],  3.24);
        OIIO_CHECK_EQUAL(array.getValues()[1], -1.537);
        OIIO_CHECK_EQUAL(array.getValues()[2], -0.49850);
        OIIO_CHECK_EQUAL(array.getValues()[3],  0.0);

        OIIO_CHECK_EQUAL(array.getValues()[4], -0.96930);
        OIIO_CHECK_EQUAL(array.getValues()[5],  1.876);
        OIIO_CHECK_EQUAL(array.getValues()[6],  0.04156);
        OIIO_CHECK_EQUAL(array.getValues()[7],  0.0);

        OIIO_CHECK_EQUAL(array.getValues()[8],  0.05560);
        OIIO_CHECK_EQUAL(array.getValues()[9], -0.204);
        // Validate double precision can be read both matrix and ...
        OIIO_CHECK_EQUAL(array.getValues()[10], 1.123456789012);
        OIIO_CHECK_EQUAL(array.getValues()[11], 0.0);

        OIIO_CHECK_EQUAL(array.getValues()[12], 0.0);
        OIIO_CHECK_EQUAL(array.getValues()[13], 0.0);
        OIIO_CHECK_EQUAL(array.getValues()[14], 0.0);
        OIIO_CHECK_EQUAL(array.getValues()[15], 1.0);

        const OCIO::OpData::Matrix::Offsets& offsets = pMatrix->getOffsets();
        // ... offset
        OIIO_CHECK_EQUAL(offsets[0], 0.987654321098);
        OIIO_CHECK_EQUAL(offsets[1], 0.2);
        OIIO_CHECK_EQUAL(offsets[2], 0.3);
        OIIO_CHECK_EQUAL(offsets[3], 0.0);
    }
}

OIIO_ADD_TEST(FileFormatCTF, Matrix_1_3_3x3)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("matrix_example_1_3_3x3.ctf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        const OCIO::CTF::Version ctfVersion = cachedFile->m_transform->getCTFVersion();
        OIIO_CHECK_ASSERT(OCIO::CTF::CTF_PROCESS_LIST_VERSION_1_3 == ctfVersion);

        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::MatrixType);
        const OCIO::OpData::Matrix * pMatrix = dynamic_cast<const OCIO::OpData::Matrix *>(opList[0]);
        OIIO_CHECK_ASSERT(pMatrix);

        OIIO_CHECK_ASSERT(cachedFile->m_transform->getInputDescriptor() == "XYZ");
        OIIO_CHECK_ASSERT(cachedFile->m_transform->getOutputDescriptor() == "RGB");

        OIIO_CHECK_ASSERT(pMatrix->getInputBitDepth() == OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_ASSERT(pMatrix->getOutputBitDepth() == OCIO::BIT_DEPTH_F32);

        const OCIO::OpData::ArrayDouble& array = pMatrix->getArray();
        OIIO_CHECK_ASSERT(array.getLength() == 4);
        OIIO_CHECK_ASSERT(array.getNumColorComponents() == 4);
        OIIO_CHECK_ASSERT(array.getNumValues() ==
            array.getLength()*array.getLength());

        OIIO_CHECK_EQUAL(array.getValues().size(), array.getNumValues());
        OIIO_CHECK_EQUAL(array.getValues()[0],  3.24);
        OIIO_CHECK_EQUAL(array.getValues()[1], -1.537);
        OIIO_CHECK_EQUAL(array.getValues()[2], -0.49850);
        OIIO_CHECK_EQUAL(array.getValues()[3],  0.0);

        OIIO_CHECK_EQUAL(array.getValues()[4], -0.96930);
        OIIO_CHECK_EQUAL(array.getValues()[5],  1.876);
        OIIO_CHECK_EQUAL(array.getValues()[6],  0.04156);
        OIIO_CHECK_EQUAL(array.getValues()[7],  0.0);

        OIIO_CHECK_EQUAL(array.getValues()[8],  0.05560);
        OIIO_CHECK_EQUAL(array.getValues()[9], -0.204);
        OIIO_CHECK_EQUAL(array.getValues()[10], 1.0573);
        OIIO_CHECK_EQUAL(array.getValues()[11], 0.0);

        OIIO_CHECK_EQUAL(array.getValues()[12], 0.0);
        OIIO_CHECK_EQUAL(array.getValues()[13], 0.0);
        OIIO_CHECK_EQUAL(array.getValues()[14], 0.0);
        OIIO_CHECK_EQUAL(array.getValues()[15], 1.0);

        const OCIO::OpData::Matrix::Offsets& offsets = pMatrix->getOffsets();
        OIIO_CHECK_EQUAL(offsets[1], 0.0);
        OIIO_CHECK_EQUAL(offsets[2], 0.0);
        OIIO_CHECK_EQUAL(offsets[3], 0.0);
        OIIO_CHECK_EQUAL(offsets[0], 0.0);

    }
}

OIIO_ADD_TEST(FileFormatCTF, Matrix_1_3_4x4)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("matrix_example_1_3_4x4.ctf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        const OCIO::CTF::Version ctfVersion = cachedFile->m_transform->getCTFVersion();
        OIIO_CHECK_ASSERT(OCIO::CTF::CTF_PROCESS_LIST_VERSION_1_3 == ctfVersion);

        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::MatrixType);
        const OCIO::OpData::Matrix * pMatrix = dynamic_cast<const OCIO::OpData::Matrix *>(opList[0]);
        OIIO_CHECK_ASSERT(pMatrix);

        OIIO_CHECK_ASSERT(cachedFile->m_transform->getInputDescriptor() == "XYZ");
        OIIO_CHECK_ASSERT(cachedFile->m_transform->getOutputDescriptor() == "RGB");

        OIIO_CHECK_ASSERT(pMatrix->getInputBitDepth() == OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_ASSERT(pMatrix->getOutputBitDepth() == OCIO::BIT_DEPTH_F32);

        const OCIO::OpData::ArrayDouble& array = pMatrix->getArray();
        OIIO_CHECK_ASSERT(array.getLength() == 4);
        OIIO_CHECK_ASSERT(array.getNumColorComponents() == 4);
        OIIO_CHECK_ASSERT(array.getNumValues() ==
            array.getLength()*array.getLength());

        OIIO_CHECK_EQUAL(array.getValues().size(), array.getNumValues());

        OIIO_CHECK_EQUAL(array.getValues()[0],  3.24);
        OIIO_CHECK_EQUAL(array.getValues()[1], -1.537);
        OIIO_CHECK_EQUAL(array.getValues()[2], -0.49850);
        OIIO_CHECK_EQUAL(array.getValues()[3], -0.1);

        OIIO_CHECK_EQUAL(array.getValues()[4], -0.96930);
        OIIO_CHECK_EQUAL(array.getValues()[5],  1.876);
        OIIO_CHECK_EQUAL(array.getValues()[6],  0.04156);
        OIIO_CHECK_EQUAL(array.getValues()[7], -0.2);

        OIIO_CHECK_EQUAL(array.getValues()[8],   0.05560);
        OIIO_CHECK_EQUAL(array.getValues()[9],  -0.204);
        OIIO_CHECK_EQUAL(array.getValues()[10],  1.0573);
        OIIO_CHECK_EQUAL(array.getValues()[11], -0.3);

        OIIO_CHECK_EQUAL(array.getValues()[12], 0.11);
        OIIO_CHECK_EQUAL(array.getValues()[13], 0.22);
        OIIO_CHECK_EQUAL(array.getValues()[14], 0.33);
        OIIO_CHECK_EQUAL(array.getValues()[15], 0.4);

        const OCIO::OpData::Matrix::Offsets& offsets = pMatrix->getOffsets();
        OIIO_CHECK_EQUAL(offsets[0], 0.0);
        OIIO_CHECK_EQUAL(offsets[1], 0.0);
        OIIO_CHECK_EQUAL(offsets[2], 0.0);
        OIIO_CHECK_EQUAL(offsets[3], 0.0);
    }
}

OIIO_ADD_TEST(FileFormatCTF, Matrix_1_3_offsets)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("matrix_example_1_3_offsets.ctf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        const OCIO::CTF::Version ctfVersion = cachedFile->m_transform->getCTFVersion();
        OIIO_CHECK_ASSERT(OCIO::CTF::CTF_PROCESS_LIST_VERSION_1_3 == ctfVersion);

        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::MatrixType);
        const OCIO::OpData::Matrix * pMatrix = dynamic_cast<const OCIO::OpData::Matrix *>(opList[0]);
        OIIO_CHECK_ASSERT(pMatrix);

        OIIO_CHECK_ASSERT(cachedFile->m_transform->getInputDescriptor() == "XYZ");
        OIIO_CHECK_ASSERT(cachedFile->m_transform->getOutputDescriptor() == "RGB");

        OIIO_CHECK_ASSERT(pMatrix->getInputBitDepth() == OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_ASSERT(pMatrix->getOutputBitDepth() == OCIO::BIT_DEPTH_F32);

        const OCIO::OpData::ArrayDouble& array = pMatrix->getArray();
        OIIO_CHECK_ASSERT(array.getLength() == 4);
        OIIO_CHECK_ASSERT(array.getNumColorComponents() == 4);
        OIIO_CHECK_ASSERT(array.getNumValues() ==
            array.getLength()*array.getLength());

        OIIO_CHECK_EQUAL(array.getValues().size(), array.getNumValues());
        OIIO_CHECK_EQUAL(array.getValues()[0],  3.24);
        OIIO_CHECK_EQUAL(array.getValues()[1], -1.537);
        OIIO_CHECK_EQUAL(array.getValues()[2], -0.49850);
        OIIO_CHECK_EQUAL(array.getValues()[3],  0.0f);

        OIIO_CHECK_EQUAL(array.getValues()[4], -0.96930);
        OIIO_CHECK_EQUAL(array.getValues()[5],  1.876);
        OIIO_CHECK_EQUAL(array.getValues()[6],  0.04156);
        OIIO_CHECK_EQUAL(array.getValues()[7],  0.0);

        OIIO_CHECK_EQUAL(array.getValues()[8],  0.05560);
        OIIO_CHECK_EQUAL(array.getValues()[9], -0.204);
        OIIO_CHECK_EQUAL(array.getValues()[10], 1.0573);
        OIIO_CHECK_EQUAL(array.getValues()[11], 0.0);

        OIIO_CHECK_EQUAL(array.getValues()[12], 0.0);
        OIIO_CHECK_EQUAL(array.getValues()[13], 0.0);
        OIIO_CHECK_EQUAL(array.getValues()[14], 0.0);
        OIIO_CHECK_EQUAL(array.getValues()[15], 1.0);

        const OCIO::OpData::Matrix::Offsets& offsets = pMatrix->getOffsets();
        OIIO_CHECK_EQUAL(offsets[0], 0.1);
        OIIO_CHECK_EQUAL(offsets[1], 0.2);
        OIIO_CHECK_EQUAL(offsets[2], 0.3);
        OIIO_CHECK_EQUAL(offsets[3], 0.0);
    }
}

OIIO_ADD_TEST(FileFormatCTF, Matrix_1_3_alpha_offsets)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("matrix_example_1_3_alpha_offsets.ctf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        const OCIO::CTF::Version ctfVersion = cachedFile->m_transform->getCTFVersion();
        OIIO_CHECK_ASSERT(OCIO::CTF::CTF_PROCESS_LIST_VERSION_1_3 == ctfVersion);

        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::MatrixType);
        const OCIO::OpData::Matrix * pMatrix = dynamic_cast<const OCIO::OpData::Matrix *>(opList[0]);
        OIIO_CHECK_ASSERT(pMatrix);

        OIIO_CHECK_ASSERT(cachedFile->m_transform->getInputDescriptor() == "XYZ");
        OIIO_CHECK_ASSERT(cachedFile->m_transform->getOutputDescriptor() == "RGB");

        OIIO_CHECK_ASSERT(pMatrix->getInputBitDepth() == OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_ASSERT(pMatrix->getOutputBitDepth() == OCIO::BIT_DEPTH_F32);

        const OCIO::OpData::ArrayDouble& array = pMatrix->getArray();
        OIIO_CHECK_ASSERT(array.getLength() == 4);
        OIIO_CHECK_ASSERT(array.getNumColorComponents() == 4);
        OIIO_CHECK_ASSERT(array.getNumValues() ==
            array.getLength()*array.getLength());

        OIIO_CHECK_EQUAL(array.getValues().size(), array.getNumValues());
        OIIO_CHECK_EQUAL(array.getValues()[0],  3.24);
        OIIO_CHECK_EQUAL(array.getValues()[1], -1.537);
        OIIO_CHECK_EQUAL(array.getValues()[2], -0.49850);
        OIIO_CHECK_EQUAL(array.getValues()[3],  0.6);

        OIIO_CHECK_EQUAL(array.getValues()[4], -0.96930);
        OIIO_CHECK_EQUAL(array.getValues()[5],  1.876);
        OIIO_CHECK_EQUAL(array.getValues()[6],  0.04156);
        OIIO_CHECK_EQUAL(array.getValues()[7],  0.7);

        OIIO_CHECK_EQUAL(array.getValues()[8],  0.05560);
        OIIO_CHECK_EQUAL(array.getValues()[9], -0.204);
        OIIO_CHECK_EQUAL(array.getValues()[10], 1.0573);
        OIIO_CHECK_EQUAL(array.getValues()[11], 0.8);

        OIIO_CHECK_EQUAL(array.getValues()[12], 1.2);
        OIIO_CHECK_EQUAL(array.getValues()[13], 1.3);
        OIIO_CHECK_EQUAL(array.getValues()[14], 1.4);
        OIIO_CHECK_EQUAL(array.getValues()[15], 1.5);

        const OCIO::OpData::Matrix::Offsets& offsets = pMatrix->getOffsets();
        OIIO_CHECK_EQUAL(offsets[0], 0.1);
        OIIO_CHECK_EQUAL(offsets[1], 0.2);
        OIIO_CHECK_EQUAL(offsets[2], 0.3);
        OIIO_CHECK_EQUAL(offsets[3], 0.4);
    }
}

OIIO_ADD_TEST(FileFormatCTF, 3by1DLut)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("xyz_to_rgb.clf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 2);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::MatrixType);
        const OCIO::OpData::Matrix * pMatrix = dynamic_cast<const OCIO::OpData::Matrix *>(opList[0]);
        OIIO_CHECK_ASSERT(pMatrix);

        OIIO_CHECK_ASSERT(pMatrix->getInputBitDepth() == OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_ASSERT(pMatrix->getOutputBitDepth() == OCIO::BIT_DEPTH_F32);

        const OCIO::OpData::ArrayDouble& a1 = pMatrix->getArray();
        OIIO_CHECK_ASSERT(a1.getLength() == 4);
        OIIO_CHECK_ASSERT(a1.getNumColorComponents() == 4);
        OIIO_CHECK_ASSERT(a1.getNumValues() == a1.getLength()*a1.getLength());

        OIIO_CHECK_ASSERT(a1.getValues().size() == a1.getNumValues());
        OIIO_CHECK_EQUAL(a1.getValues()[0],  3.24);
        OIIO_CHECK_EQUAL(a1.getValues()[1], -1.537);
        OIIO_CHECK_EQUAL(a1.getValues()[2], -0.49850);
        OIIO_CHECK_EQUAL(a1.getValues()[3],  0.0);

        OIIO_CHECK_EQUAL(a1.getValues()[4], -0.96930);
        OIIO_CHECK_EQUAL(a1.getValues()[5],  1.876);
        OIIO_CHECK_EQUAL(a1.getValues()[6],  0.04156);
        OIIO_CHECK_EQUAL(a1.getValues()[7],  0.0);

        OIIO_CHECK_EQUAL(a1.getValues()[8],  0.05560);
        OIIO_CHECK_EQUAL(a1.getValues()[9], -0.204);
        OIIO_CHECK_EQUAL(a1.getValues()[10], 1.0573);
        OIIO_CHECK_EQUAL(a1.getValues()[11], 0.0);

        OIIO_CHECK_EQUAL(a1.getValues()[12], 0.0);
        OIIO_CHECK_EQUAL(a1.getValues()[13], 0.0);
        OIIO_CHECK_EQUAL(a1.getValues()[14], 0.0);
        OIIO_CHECK_EQUAL(a1.getValues()[15], 1.0);

        const OCIO::OpData::Lut1D* pLut
            = dynamic_cast<const OCIO::OpData::Lut1D*>(opList[1]);
        OIIO_CHECK_ASSERT(pLut);
        OIIO_CHECK_ASSERT(pLut->getInputBitDepth() == OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_ASSERT(pLut->getOutputBitDepth() == OCIO::BIT_DEPTH_F32);

        const OCIO::OpData::Array& a2 = pLut->getArray();
        OIIO_CHECK_ASSERT(a2.getLength() == 17);
        OIIO_CHECK_ASSERT(a2.getNumColorComponents() == 3);
        OIIO_CHECK_EQUAL(a2.getNumValues(), a2.getLength()*pLut->getArray().getMaxColorComponents());

        OIIO_CHECK_ASSERT(a2.getValues().size() == a2.getNumValues());
        OIIO_CHECK_EQUAL(a2.getValues()[0], 0.0f);
        OIIO_CHECK_EQUAL(a2.getValues()[1], 0.0f);
        OIIO_CHECK_EQUAL(a2.getValues()[2], 0.0f);
        OIIO_CHECK_EQUAL(a2.getValues()[3], 0.28358f);

        OIIO_CHECK_EQUAL(a2.getValues()[21], 0.68677f);
        OIIO_CHECK_EQUAL(a2.getValues()[22], 0.68677f);
        OIIO_CHECK_EQUAL(a2.getValues()[23], 0.68677f);

        OIIO_CHECK_EQUAL(a2.getValues()[48], 1.0f);
        OIIO_CHECK_EQUAL(a2.getValues()[49], 1.0f);
        OIIO_CHECK_EQUAL(a2.getValues()[50], 1.0f);
    }
}


OIIO_ADD_TEST(FileFormatCTF, Inv1DLut)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("lut1d_inv.ctf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 2);

        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::MatrixType);
        const OCIO::OpData::Matrix * pMatrix
            = dynamic_cast<const OCIO::OpData::Matrix *>(opList[0]);

        OIIO_CHECK_ASSERT(pMatrix);
        OIIO_CHECK_ASSERT(pMatrix->getInputBitDepth() == OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_ASSERT(pMatrix->getOutputBitDepth() == OCIO::BIT_DEPTH_F32);

        const OCIO::OpData::ArrayDouble& a1 = pMatrix->getArray();
        OIIO_CHECK_EQUAL(a1.getLength(), 4);
        OIIO_CHECK_EQUAL(a1.getNumColorComponents(), 4);
        OIIO_CHECK_EQUAL(a1.getNumValues(), a1.getLength()*a1.getLength());

        OIIO_CHECK_ASSERT(a1.getValues().size() == a1.getNumValues());
        OIIO_CHECK_EQUAL(a1.getValues()[0],  3.24);
        OIIO_CHECK_EQUAL(a1.getValues()[1], -1.537);
        OIIO_CHECK_EQUAL(a1.getValues()[2], -0.49850);
        OIIO_CHECK_EQUAL(a1.getValues()[3],  0.0);

        OIIO_CHECK_EQUAL(a1.getValues()[4], -0.96930);
        OIIO_CHECK_EQUAL(a1.getValues()[5],  1.876);
        OIIO_CHECK_EQUAL(a1.getValues()[6],  0.04156);
        OIIO_CHECK_EQUAL(a1.getValues()[7],  0.0);

        OIIO_CHECK_EQUAL(a1.getValues()[8],  0.05560);
        OIIO_CHECK_EQUAL(a1.getValues()[9], -0.204);
        OIIO_CHECK_EQUAL(a1.getValues()[10], 1.0573);
        OIIO_CHECK_EQUAL(a1.getValues()[11], 0.0);

        OIIO_CHECK_EQUAL(a1.getValues()[12], 0.0);
        OIIO_CHECK_EQUAL(a1.getValues()[13], 0.0);
        OIIO_CHECK_EQUAL(a1.getValues()[14], 0.0);
        OIIO_CHECK_EQUAL(a1.getValues()[15], 1.0);

        OIIO_CHECK_EQUAL(opList[1]->getOpType(), OCIO::OpData::OpData::InvLut1DType);
        const OCIO::OpData::InvLut1D* pLut
            = dynamic_cast<const OCIO::OpData::InvLut1D*>(opList[1]);

        OIIO_CHECK_ASSERT(pLut);
        OIIO_CHECK_EQUAL(pLut->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_EQUAL(pLut->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);

        const OCIO::OpData::Array& a2 = pLut->getArray();
        OIIO_CHECK_EQUAL(a2.getNumColorComponents(), 3);

        OIIO_CHECK_EQUAL(a2.getLength(), 17);
        OIIO_CHECK_EQUAL(a2.getNumValues(), a2.getLength()*a2.getMaxColorComponents());

        const float error = 1e-6f;
        OIIO_CHECK_EQUAL(a2.getValues().size(), a2.getNumValues());

        OIIO_CHECK_CLOSE(a2.getValues()[0], 0.0f, error);
        OIIO_CHECK_CLOSE(a2.getValues()[1], 0.0f, error);
        OIIO_CHECK_CLOSE(a2.getValues()[2], 0.0f, error);
        OIIO_CHECK_CLOSE(a2.getValues()[3], 0.28358f, error);

        OIIO_CHECK_CLOSE(a2.getValues()[21], 0.68677f, error);
        OIIO_CHECK_CLOSE(a2.getValues()[22], 0.68677f, error);
        OIIO_CHECK_CLOSE(a2.getValues()[23], 0.68677f, error);

        OIIO_CHECK_CLOSE(a2.getValues()[48], 1.0f, error);
        OIIO_CHECK_CLOSE(a2.getValues()[49], 1.0f, error);
        OIIO_CHECK_CLOSE(a2.getValues()[50], 1.0f, error);
    }
}

OIIO_ADD_TEST(FileFormatCTF, 3DLut)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("lut3d_example.clf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 1);

        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::Lut3DType);
        const OCIO::OpData::Lut3D * pLut = dynamic_cast<const OCIO::OpData::Lut3D *>(opList[0]);
        OIIO_CHECK_ASSERT(pLut);

        OIIO_CHECK_ASSERT(pLut->getInputBitDepth() == OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_ASSERT(pLut->getOutputBitDepth() == OCIO::BIT_DEPTH_UINT12);

        const OCIO::OpData::Array& a1 = pLut->getArray();

        const OCIO::OpData::Array& array = pLut->getArray();
        OIIO_CHECK_ASSERT(array.getLength() == 17);
        OIIO_CHECK_ASSERT(array.getNumColorComponents() == 3);
        OIIO_CHECK_EQUAL(array.getNumValues(),
            array.getLength()*array.getLength()
            *array.getLength()*pLut->getArray().getMaxColorComponents());

        OIIO_CHECK_ASSERT(array.getValues().size() == array.getNumValues());
        OIIO_CHECK_EQUAL(array.getValues()[0], 10.0f);
        OIIO_CHECK_EQUAL(array.getValues()[1], 0.0f);
        OIIO_CHECK_EQUAL(array.getValues()[2], 5.0f);

        OIIO_CHECK_EQUAL(array.getValues()[18], 26.0f);
        OIIO_CHECK_EQUAL(array.getValues()[19], 308.0f);
        OIIO_CHECK_EQUAL(array.getValues()[20], 580.0f);

        OIIO_CHECK_EQUAL(array.getValues()[30], 0.0f);
        OIIO_CHECK_EQUAL(array.getValues()[31], 586.0f);
        OIIO_CHECK_EQUAL(array.getValues()[32], 1350.0f);
    }
}

OIIO_ADD_TEST(FileFormatCTF, Inv3DLut)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("lut3d_example_Inv.ctf");

        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 1);

        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::InvLut3DType);
        const OCIO::OpData::InvLut3D * pLut = dynamic_cast<const OCIO::OpData::InvLut3D *>(opList[0]);
        OIIO_CHECK_ASSERT(pLut);

        OIIO_CHECK_EQUAL(pLut->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_EQUAL(pLut->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);
        OIIO_CHECK_EQUAL(pLut->getInterpolation(), OCIO::INTERP_TETRAHEDRAL);

        const OCIO::OpData::Array& array = pLut->getArray();
        OIIO_CHECK_EQUAL(array.getNumColorComponents(), 3);
        OIIO_CHECK_EQUAL(array.getNumValues(),
            array.getLength()*array.getLength()
            *array.getLength()*array.getMaxColorComponents());
        OIIO_CHECK_EQUAL(array.getValues().size(), array.getNumValues());

        OIIO_CHECK_EQUAL(array.getLength(), 17);
        OIIO_CHECK_EQUAL(array.getValues()[0], 25.0f);
        OIIO_CHECK_EQUAL(array.getValues()[1], 30.0f);
        OIIO_CHECK_EQUAL(array.getValues()[2], 33.0f);

        OIIO_CHECK_EQUAL(array.getValues()[18],  26.0f);
        OIIO_CHECK_EQUAL(array.getValues()[19], 308.0f);
        OIIO_CHECK_EQUAL(array.getValues()[20], 580.0f);

        OIIO_CHECK_EQUAL(array.getValues()[30],   0.0f);
        OIIO_CHECK_EQUAL(array.getValues()[31], 586.0f);
        OIIO_CHECK_EQUAL(array.getValues()[32],1350.0f);
    }
}


OIIO_ADD_TEST(FileFormatCTF, ReferenceAlias)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("reference_alias.ctf");
        // TODO: will be 1 op when ctf inv LUT is done
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile), OCIO::Exception, "No color operator");

        /*
        const Color::ReferenceOp* pRef
        = dynamic_cast<const Color::ReferenceOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pRef);

        BOOST_CHECK_EQUAL("alias", pRef->getResolvedAlias());
        BOOST_CHECK_EQUAL(false, pRef->isInverted());
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, ReferencePath)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("reference_path.ctf");
        // TODO: will be 1 op when ctf inv LUT is done
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile), OCIO::Exception, "No color operator");

        /*
        const Color::ReferenceOp* pRef
        = dynamic_cast<const Color::ReferenceOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pRef);

        BOOST_CHECK_EQUAL(SYNCOLOR::BASE_PATH_SHARED, pRef->getBasePath());
        BOOST_CHECK_EQUAL("toto/toto.xml", pRef->getPath());
        BOOST_CHECK_EQUAL(true, pRef->isInverted());
        */
    }
}


OIIO_ADD_TEST(FileFormatCTF, ReferenceSequenceInverse)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("three_references_some_inverted.ctf");
        // TODO: will be 3 ops when ctf Reference is done
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile), OCIO::Exception, "No color operator");
        /*
        //
        // Validate the read-in transsform
        //
        const boost::shared_ptr<Color::Transform>& pTransform
        = reader.getTransform();
        BOOST_REQUIRE(pTransform.use_count()==1);

        Color::OpList opList = pTransform->getOps();
        BOOST_CHECK(opList.size() == 3);

        const Color::ReferenceOp* pRef = dynamic_cast<const Color::ReferenceOp*>(opList.get(0));
        BOOST_REQUIRE(pRef);
        BOOST_CHECK(!pRef->isInverted());
        BOOST_CHECK_EQUAL(pRef->getPath(), "xml/test/matrix_example.xml");

        pRef = dynamic_cast<const Color::ReferenceOp*>(opList.get(1));
        BOOST_REQUIRE(pRef);
        BOOST_CHECK(pRef->isInverted());
        BOOST_CHECK_EQUAL(pRef->getPath(), "xml/test/lut_example.ctf");

        pRef = dynamic_cast<const Color::ReferenceOp*>(opList.get(2));
        BOOST_REQUIRE(pRef);
        BOOST_CHECK(!pRef->isInverted());
        BOOST_CHECK_EQUAL(pRef->getPath(), "xml/test/gamma_teset1.xml");

        //
        // Invert the transform
        //
        Color::TransformPtr invertedTransform(new Color::Transform());
        pTransform->inverse(*invertedTransform);

        //
        // Check that the sequence of ops is inverted and that the inversion flags are toggled for
        // each reference.
        //
        opList = invertedTransform->getOps();
        BOOST_CHECK(opList.size() == 3);

        pRef = dynamic_cast<const Color::ReferenceOp*>(opList.get(0));
        BOOST_REQUIRE(pRef);
        BOOST_CHECK(pRef->isInverted());
        BOOST_CHECK_EQUAL(pRef->getPath(), "xml/test/gamma_teset1.xml");

        pRef = dynamic_cast<const Color::ReferenceOp*>(opList.get(1));
        BOOST_REQUIRE(pRef);
        BOOST_CHECK(!pRef->isInverted());
        BOOST_CHECK_EQUAL(pRef->getPath(), "xml/test/lut_example.ctf");

        pRef = dynamic_cast<const Color::ReferenceOp*>(opList.get(2));
        BOOST_REQUIRE(pRef);
        BOOST_CHECK(pRef->isInverted());
        BOOST_CHECK_EQUAL(pRef->getPath(), "xml/test/matrix_example.xml");*/
    }
}

OIIO_ADD_TEST(FileFormatCTF, ReferenceError)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("reference_alias_path.ctf");
        // TODO: will still throw with different error when Reference is done
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile), OCIO::Exception, "No color operator");

        /*try
        {
        reader.parseFile(path+filename);
        }
        catch(Color::Exception& e)
        {
        BOOST_CHECK(e.getErrorCode()
        == SYNCOLOR::ERROR_XML_READER_ALIAS_UNEXPECTED_ATTRIBUTE);
        return;
        }
        BOOST_CHECK( false );*/
    }
}


OIIO_ADD_TEST(FileFormatCTF, ReferenceUTF8)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("reference_utf8.ctf");
        // TODO: will be 1 op when ctf Reference is done
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile), OCIO::Exception, "No color operator");

        /*const Color::ReferenceOp* pRef
        = dynamic_cast<const Color::ReferenceOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pRef);

        const char* utf8Path =
        "\xE6\xA8\x99\xE6\xBA\x96\xE8\x90\xAC\xE5\x9C\x8B\xE7\xA2\xBC";

        BOOST_CHECK_EQUAL(SYNCOLOR::BASE_PATH_SHARED, pRef->getBasePath());
        BOOST_CHECK_EQUAL(utf8Path, pRef->getPath());*/
    }
}

OIIO_ADD_TEST(FileFormatCTF, ErrorChecker)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        // NB: This file has some added unknown elements A, B, and C as a test.
        const std::string ctfFile("unknown_elements.clf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 4);

        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::MatrixType);
        const OCIO::OpData::Matrix * pMatrix = dynamic_cast<const OCIO::OpData::Matrix *>(opList[0]);
        OIIO_CHECK_ASSERT(pMatrix);

        OIIO_CHECK_ASSERT(pMatrix->getInputBitDepth() == OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_ASSERT(pMatrix->getOutputBitDepth() == OCIO::BIT_DEPTH_F32);

        const OCIO::OpData::ArrayDouble& a1 = pMatrix->getArray();
        OIIO_CHECK_ASSERT(a1.getLength() == 4);
        OIIO_CHECK_ASSERT(a1.getNumColorComponents() == 4);
        OIIO_CHECK_ASSERT(a1.getNumValues() == a1.getLength()*a1.getLength());

        OIIO_CHECK_ASSERT(a1.getValues().size() == a1.getNumValues());
        OIIO_CHECK_EQUAL(a1.getValues()[0],  3.24);
        OIIO_CHECK_EQUAL(a1.getValues()[1], -1.537);
        OIIO_CHECK_EQUAL(a1.getValues()[2], -0.49850);
        OIIO_CHECK_EQUAL(a1.getValues()[3],  0.0);

        OIIO_CHECK_EQUAL(a1.getValues()[4], -0.96930);
        OIIO_CHECK_EQUAL(a1.getValues()[5],  1.876);
        OIIO_CHECK_EQUAL(a1.getValues()[6],  0.04156);
        OIIO_CHECK_EQUAL(a1.getValues()[7],  0.0);

        OIIO_CHECK_EQUAL(a1.getValues()[8],  0.05560);
        OIIO_CHECK_EQUAL(a1.getValues()[9], -0.204);
        OIIO_CHECK_EQUAL(a1.getValues()[10], 1.0573);
        OIIO_CHECK_EQUAL(a1.getValues()[11], 0.0);

        OIIO_CHECK_EQUAL(a1.getValues()[12], 0.0);
        OIIO_CHECK_EQUAL(a1.getValues()[13], 0.0);
        OIIO_CHECK_EQUAL(a1.getValues()[14], 0.0);
        OIIO_CHECK_EQUAL(a1.getValues()[15], 1.0);

        OIIO_CHECK_EQUAL(opList[1]->getOpType(), OCIO::OpData::OpData::Lut1DType);
        const OCIO::OpData::Lut1D* pLut1
            = dynamic_cast<const OCIO::OpData::Lut1D*>(opList[1]);
        OIIO_CHECK_ASSERT(pLut1);
        OIIO_CHECK_ASSERT(pLut1->getInputBitDepth() == OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_ASSERT(pLut1->getOutputBitDepth() == OCIO::BIT_DEPTH_F32);

        const OCIO::OpData::Array& a2 = pLut1->getArray();
        OIIO_CHECK_ASSERT(a2.getLength() == 17);
        OIIO_CHECK_ASSERT(a2.getNumColorComponents() == 3);
        OIIO_CHECK_EQUAL(a2.getNumValues(), 
                         a2.getLength()*pLut1->getArray().getMaxColorComponents());

        OIIO_CHECK_ASSERT(a2.getValues().size() == a2.getNumValues());
        OIIO_CHECK_EQUAL(a2.getValues()[0], 0.0f);
        OIIO_CHECK_EQUAL(a2.getValues()[1], 0.0f);
        OIIO_CHECK_EQUAL(a2.getValues()[2], 0.01f);
        OIIO_CHECK_EQUAL(a2.getValues()[3], 0.28358f);
        OIIO_CHECK_EQUAL(a2.getValues()[4], 0.28358f);
        OIIO_CHECK_EQUAL(a2.getValues()[5], 100.0f);
        OIIO_CHECK_EQUAL(a2.getValues()[6], 0.38860f);
        OIIO_CHECK_EQUAL(a2.getValues()[7], 0.38860f);
        OIIO_CHECK_EQUAL(a2.getValues()[8], 127.0f);

        OIIO_CHECK_EQUAL(a2.getValues()[21], 0.68677f);
        OIIO_CHECK_EQUAL(a2.getValues()[22], 0.68677f);
        OIIO_CHECK_EQUAL(a2.getValues()[23], 0.68677f);

        OIIO_CHECK_EQUAL(a2.getValues()[48], 1.0f);
        OIIO_CHECK_EQUAL(a2.getValues()[49], 1.0f);
        OIIO_CHECK_EQUAL(a2.getValues()[50], 1.0f);

        OIIO_CHECK_EQUAL(opList[2]->getOpType(), OCIO::OpData::OpData::Lut1DType);
        const OCIO::OpData::Lut1D* pLut2
            = dynamic_cast<const OCIO::OpData::Lut1D*>(opList[2]);
        OIIO_CHECK_ASSERT(pLut2);
        OIIO_CHECK_ASSERT(pLut2->getInputBitDepth() == OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_ASSERT(pLut2->getOutputBitDepth() == OCIO::BIT_DEPTH_UINT10);

        const OCIO::OpData::Array& array = pLut2->getArray();
        OIIO_CHECK_ASSERT(array.getLength() == 32);
        OIIO_CHECK_ASSERT(array.getNumColorComponents() == 1);
        OIIO_CHECK_EQUAL(array.getNumValues(),
                         array.getLength()*pLut2->getArray().getMaxColorComponents());

        OIIO_CHECK_ASSERT(array.getValues().size() == 96);
        OIIO_CHECK_EQUAL(array.getValues()[0], 0.0f);
        OIIO_CHECK_EQUAL(array.getValues()[1], 0.0f);
        OIIO_CHECK_EQUAL(array.getValues()[2], 0.0f);
        OIIO_CHECK_EQUAL(array.getValues()[3], 215.0f);
        OIIO_CHECK_EQUAL(array.getValues()[4], 215.0f);
        OIIO_CHECK_EQUAL(array.getValues()[5], 215.0f);
        OIIO_CHECK_EQUAL(array.getValues()[6], 294.0f);
        // and many more
        OIIO_CHECK_EQUAL(array.getValues()[92], 1008.0f);
        OIIO_CHECK_EQUAL(array.getValues()[93], 1023.0f);
        OIIO_CHECK_EQUAL(array.getValues()[94], 1023.0f);
        OIIO_CHECK_EQUAL(array.getValues()[95], 1023.0f);

        OIIO_CHECK_EQUAL(opList[3]->getOpType(), OCIO::OpData::OpData::Lut3DType);
        const OCIO::OpData::Lut3D* pLut3
            = dynamic_cast<const OCIO::OpData::Lut3D*>(opList[3]);
        OIIO_CHECK_ASSERT(pLut3);
        OIIO_CHECK_ASSERT(pLut3->getInputBitDepth() == OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_ASSERT(pLut3->getOutputBitDepth() == OCIO::BIT_DEPTH_UINT10);

        const OCIO::OpData::Array& a3 = pLut3->getArray();
        OIIO_CHECK_ASSERT(a3.getLength() == 3);
        OIIO_CHECK_ASSERT(a3.getNumColorComponents() == 3);
        OIIO_CHECK_EQUAL(a3.getNumValues(),
                         a3.getLength()*a3.getLength()
                            *a3.getLength()*pLut3->getArray().getMaxColorComponents());

        OIIO_CHECK_ASSERT(a3.getValues().size() == a3.getNumValues());
        OIIO_CHECK_EQUAL(a3.getValues()[0], 0.0f);
        OIIO_CHECK_EQUAL(a3.getValues()[1], 30.0f);
        OIIO_CHECK_EQUAL(a3.getValues()[2], 33.0f);
        OIIO_CHECK_EQUAL(a3.getValues()[3], 0.0f);
        OIIO_CHECK_EQUAL(a3.getValues()[4], 0.0f);
        OIIO_CHECK_EQUAL(a3.getValues()[5], 133.0f);

        OIIO_CHECK_EQUAL(a3.getValues()[78], 1023.0f);
        OIIO_CHECK_EQUAL(a3.getValues()[79], 1023.0f);
        OIIO_CHECK_EQUAL(a3.getValues()[80], 1023.0f);

        // TODO: check log for parsing warnings
        // DummyElt is logging at debug level
        // Ignore element 'B' (line 34) where its parent is 'ProcessList' (line 2) : Unknown element : test.clf
        // Ignore element 'C' (line 34) where its parent is 'B' (line 34) : test.clf
        // Ignore element 'A' (line 36) where its parent is 'Description' (line 36) : test.clf
    }
}


OIIO_ADD_TEST(FileFormatCTF, BinaryFile)
{
    const std::string ctfFile("image_png.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile), OCIO::Exception, "not well-formed");
}

OIIO_ADD_TEST(FileFormatCTF, ErrorCheckerForDifficultXml)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("difficult_test1_v1.ctf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        // Defaults to 1.2
        const OCIO::CTF::Version ctfVersion = cachedFile->m_transform->getCTFVersion();
        OIIO_CHECK_ASSERT(OCIO::CTF::CTF_PROCESS_LIST_VERSION_1_2 == ctfVersion);

        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 2);

        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::MatrixType);
        const OCIO::OpData::Matrix * pMatrix = dynamic_cast<const OCIO::OpData::Matrix *>(opList[0]);
        OIIO_CHECK_ASSERT(pMatrix);
        OIIO_CHECK_EQUAL(pMatrix->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_EQUAL(pMatrix->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

        const OCIO::OpData::ArrayDouble& array = pMatrix->getArray();
        OIIO_CHECK_EQUAL(array.getLength(), 4u);
        OIIO_CHECK_EQUAL(array.getNumColorComponents(), 4u);
        OIIO_CHECK_EQUAL(array.getNumValues(),
            array.getLength()*array.getLength());

        OIIO_CHECK_ASSERT(array.getValues().size() == array.getNumValues());
        OIIO_CHECK_EQUAL(array.getValues()[0],  3.24);
        OIIO_CHECK_EQUAL(array.getValues()[1], -1.537);
        OIIO_CHECK_EQUAL(array.getValues()[2], -0.4985);
        OIIO_CHECK_EQUAL(array.getValues()[3],  0.0);

        OIIO_CHECK_EQUAL(array.getValues()[4], -0.96930);
        OIIO_CHECK_EQUAL(array.getValues()[5],  1.876);
        OIIO_CHECK_EQUAL(array.getValues()[6],  0.04156);
        OIIO_CHECK_EQUAL(array.getValues()[7],  0.0);

        OIIO_CHECK_EQUAL(array.getValues()[8],  0.0556);
        OIIO_CHECK_EQUAL(array.getValues()[9], -0.204);
        OIIO_CHECK_EQUAL(array.getValues()[10], 0.105730e+1);
        OIIO_CHECK_EQUAL(array.getValues()[11], 0.0);

        OIIO_CHECK_EQUAL(array.getValues()[12], 0.0);
        OIIO_CHECK_EQUAL(array.getValues()[13], 0.0);
        OIIO_CHECK_EQUAL(array.getValues()[14], 0.0);
        OIIO_CHECK_EQUAL(array.getValues()[15], 1.0);

        const OCIO::OpData::Lut1D* pLut
            = dynamic_cast<const OCIO::OpData::Lut1D*>(opList[1]);
        OIIO_CHECK_ASSERT(pLut);
        OIIO_CHECK_ASSERT(pLut->getInputBitDepth() == OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_ASSERT(pLut->getOutputBitDepth() == OCIO::BIT_DEPTH_F32);

        const OCIO::OpData::Array& array2 = pLut->getArray();
        OIIO_CHECK_ASSERT(array2.getLength() == 17);
        OIIO_CHECK_ASSERT(array2.getNumColorComponents() == 3);
        OIIO_CHECK_EQUAL(array2.getNumValues(),
                         array2.getLength()*pLut->getArray().getMaxColorComponents());

        OIIO_CHECK_ASSERT(array2.getValues().size() == 51);
        OIIO_CHECK_EQUAL(array2.getValues()[0], 0.0f);
        OIIO_CHECK_EQUAL(array2.getValues()[1], 0.0f);
        OIIO_CHECK_EQUAL(array2.getValues()[2], 0.0f);
        OIIO_CHECK_EQUAL(array2.getValues()[3], 0.28358f);
        OIIO_CHECK_EQUAL(array2.getValues()[4], 0.28358f);
        OIIO_CHECK_EQUAL(array2.getValues()[5], 0.28358f);
        OIIO_CHECK_EQUAL(array2.getValues()[6], 0.38860f);
        OIIO_CHECK_EQUAL(array2.getValues()[45], 0.97109f);
        OIIO_CHECK_EQUAL(array2.getValues()[46], 0.97109f);
        OIIO_CHECK_EQUAL(array2.getValues()[47], 0.97109f);

        // TODO: check log for parsing warnings
        // DummyElt is logging at debug level
        // Ignore element 'Ignore' (line 10) where its parent is 'ProcessList' (line 8) : Unknown element : difficult_test1_v1.ctf
        // Ignore element 'ProcessList' (line 27) where its parent is 'ProcessList' (line 8) : The Color::Transform already exists : difficult_test1_v1.ctf
        // Ignore element 'Array' (line 30) where its parent is 'Matrix' (line 16) : Only one Color::Array allowed per op : difficult_test1_v1.ctf
        // Ignore element 'just_ignore' (line 37) where its parent is 'ProcessList' (line 8) : Unknown element : difficult_test1_v1.ctf
        // Ignore element 'just_ignore' (line 69) where its parent is 'Description' (line 66) : difficult_test1_v1.ctf
        // Ignore element 'just_ignore' (line 70) where its parent is 'just_ignore' (line 69) : difficult_test1_v1.ctf
        // Ignore element 'Matrix' (line 75) where its parent is 'LUT1D' (line 43) : The Matrix's parent can only be a Transform: difficult_test1_v1.ctf
        // Ignore element 'Description' (line 76) where its parent is 'Matrix' (line 75) : difficult_test1_v1.ctf
        // Ignore element 'Array' (line 77) where its parent is 'Matrix' (line 75) : difficult_test1_v1.ctf
    }
}

OIIO_ADD_TEST(FileFormatCTF, InvalidTransform)
{
    const std::string ctfFile("transform_invalid.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile), OCIO::Exception, "Invalid transform");
}


OIIO_ADD_TEST(FileFormatCTF, MissingAttributeEnd)
{
    const std::string ctfFile("transform_attribute_end_missing.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile), OCIO::Exception, "no closing tag");
}


OIIO_ADD_TEST(FileFormatCTF, MissingTransformId)
{
    const std::string ctfFile("transform_missing_id.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile),
                          OCIO::Exception,
                          "Required attribute 'id'");
}

OIIO_ADD_TEST(FileFormatCTF, MissingInBitDepth)
{
    const std::string ctfFile("transform_missing_inbitdepth.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile),
                          OCIO::Exception,
                          "inBitDepth is missing");
}

OIIO_ADD_TEST(FileFormatCTF, MissingOutBitDepth)
{
    const std::string ctfFile("transform_missing_outbitdepth.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile),
                          OCIO::Exception,
                          "outBitDepth is missing");
}

OIIO_ADD_TEST(FileFormatCTF, ArrayMissingValues)
{
    const std::string ctfFile("array_missing_values.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile),
                          OCIO::Exception,
                          "Expected 3x3x3 Array values");
}

OIIO_ADD_TEST(FileFormatCTF, ArrayIllegalValues)
{
    const std::string ctfFile("array_illegal_values.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile), OCIO::Exception, "Illegal values");
}

OIIO_ADD_TEST(FileFormatCTF, UnknownValue)
{
    const std::string ctfFile("unknown_value.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile),
                          OCIO::Exception,
                          "outBitDepth unknown value");
}

OIIO_ADD_TEST(FileFormatCTF, ArrayCorruptedDimension)
{
    const std::string ctfFile("array_illegal_dimension.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile),
                          OCIO::Exception,
                          "Illegal 'Matrix' dimensions");
}

OIIO_ADD_TEST(FileFormatCTF, ArrayTooManyValues)
{
    const std::string ctfFile("array_too_many_values.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile),
                          OCIO::Exception,
                          "Expected 3x3 Array, found additional values");
}

OIIO_ADD_TEST(FileFormatCTF, MatrixBitdepthIllegal)
{
    const std::string ctfFile("matrix_bitdepth_illegal.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile),
                          OCIO::Exception,
                          "inBitDepth unknown value");
}

OIIO_ADD_TEST(FileFormatCTF, MatrixEndMissing)
{
    const std::string ctfFile("matrix_end_missing.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile),
                          OCIO::Exception,
                          "no closing tag for 'Matrix'");
}

OIIO_ADD_TEST(FileFormatCTF, TransformCorruptedTag)
{
    const std::string ctfFile("transform_corrupted_tag.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile), OCIO::Exception, "no closing tag");
}

OIIO_ADD_TEST(FileFormatCTF, TransformEmpty)
{
    const std::string ctfFile("transform_empty.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile),
                          OCIO::Exception,
                          "No color operator");
}

OIIO_ADD_TEST(FileFormatCTF, TransformIdEmpty)
{
    const std::string ctfFile("transform_id_empty.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile),
                          OCIO::Exception,
                          "Required attribute 'id' does not have a value");
}

OIIO_ADD_TEST(FileFormatCTF, TransformWithBitDepthMismatch)
{
    const std::string ctfFile("transform_bitdepth_mismatch.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile),
                          OCIO::Exception,
                          "Bitdepth missmatch");
}

OIIO_ADD_TEST(FileFormatCTF, checkIndexMap)
{
    const std::string ctfFile("indexMap_test.ctf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile),
                          OCIO::Exception,
                          "IndexMap must have two entries");
}

OIIO_ADD_TEST(FileFormatCTF, MatrixWithOffset)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("matrix_offsets_example.ctf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);
        // Note that the ProcessList does not have a version attribute and
        // therefore defaults to 1.2.
        // The "4 4 3" Array syntax is only allowed in versions 1.2 or earlier.
        const OCIO::CTF::Version ctfVersion = cachedFile->m_transform->getCTFVersion();
        OIIO_CHECK_ASSERT(OCIO::CTF::CTF_PROCESS_LIST_VERSION_1_2 == ctfVersion);

        const OCIO::OpData::OpDataVec & opList =
            cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(),
                         OCIO::OpData::OpData::MatrixType);
        const OCIO::OpData::Matrix * pMatrix =
            dynamic_cast<const OCIO::OpData::Matrix *>(opList[0]);
        OIIO_CHECK_ASSERT(pMatrix);

        OIIO_CHECK_ASSERT(pMatrix->getInputBitDepth() == OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_ASSERT(pMatrix->getOutputBitDepth() == OCIO::BIT_DEPTH_F32);

        const OCIO::OpData::ArrayDouble& array = pMatrix->getArray();
        OIIO_CHECK_ASSERT(array.getLength() == 4);
        OIIO_CHECK_ASSERT(array.getNumColorComponents() == 4);
        OIIO_CHECK_ASSERT(array.getNumValues() ==
            array.getLength()*array.getLength());

        OIIO_CHECK_ASSERT(array.getValues().size() == array.getNumValues());
        OIIO_CHECK_EQUAL(array.getValues()[0],  3.24);
        OIIO_CHECK_EQUAL(array.getValues()[1], -1.537);
        OIIO_CHECK_EQUAL(array.getValues()[2], -0.49850);
        OIIO_CHECK_EQUAL(array.getValues()[3],  0.0);
        
        OIIO_CHECK_EQUAL(array.getValues()[4], -0.96930);
        OIIO_CHECK_EQUAL(array.getValues()[5],  1.876);
        OIIO_CHECK_EQUAL(array.getValues()[6],  0.04156);
        OIIO_CHECK_EQUAL(array.getValues()[7],  0.0);
        
        OIIO_CHECK_EQUAL(array.getValues()[8],  0.05560);
        OIIO_CHECK_EQUAL(array.getValues()[9], -0.204);
        OIIO_CHECK_EQUAL(array.getValues()[10], 1.0573);
        OIIO_CHECK_EQUAL(array.getValues()[11], 0.0);
        
        OIIO_CHECK_EQUAL(array.getValues()[12], 0.0);
        OIIO_CHECK_EQUAL(array.getValues()[13], 0.0);
        OIIO_CHECK_EQUAL(array.getValues()[14], 0.0);
        OIIO_CHECK_EQUAL(array.getValues()[15], 1.0);
        
        OIIO_CHECK_EQUAL(pMatrix->getOffsets()[0], 1.0);
        OIIO_CHECK_EQUAL(pMatrix->getOffsets()[1], 2.0);
        OIIO_CHECK_EQUAL(pMatrix->getOffsets()[2], 3.0);
    }
}

OIIO_ADD_TEST(FileFormatCTF, MatrixWithOffset_1_3)
{
    // Matrix 4 3 3 only valid up to version 1.2
    const std::string ctfFile("matrix_offsets_example_1_3.ctf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile),
                          OCIO::Exception,
                          "Illegal 'Matrix' dimensions 4 4 3");
}

OIIO_ADD_TEST(FileFormatCTF, Lut3by1dWithNanInfinity)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("lut3by1d_nan_infinity_example.clf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        const OCIO::OpData::OpDataVec & opList =
            cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(),
                         OCIO::OpData::OpData::Lut1DType);
        const OCIO::OpData::Lut1D * pLut1d =
            dynamic_cast<const OCIO::OpData::Lut1D*>(opList[0]);
        OIIO_CHECK_ASSERT(pLut1d);

        const OCIO::OpData::Array& array = pLut1d->getArray();

        OIIO_CHECK_ASSERT(array.getValues().size() == array.getNumValues());
        OIIO_CHECK_ASSERT(OCIO::isnan(array.getValues()[0]));
        OIIO_CHECK_ASSERT(OCIO::isnan(array.getValues()[1]));
        OIIO_CHECK_ASSERT(OCIO::isnan(array.getValues()[2]));
        OIIO_CHECK_ASSERT(OCIO::isnan(array.getValues()[3]));
        OIIO_CHECK_ASSERT(OCIO::isnan(array.getValues()[4]));
        OIIO_CHECK_EQUAL(array.getValues()[5],
            std::numeric_limits<float>::infinity());
        OIIO_CHECK_EQUAL(array.getValues()[6],
            std::numeric_limits<float>::infinity());
        OIIO_CHECK_EQUAL(array.getValues()[7],
            std::numeric_limits<float>::infinity());
        OIIO_CHECK_EQUAL(array.getValues()[8],
            -std::numeric_limits<float>::infinity());
        OIIO_CHECK_EQUAL(array.getValues()[9],
            -std::numeric_limits<float>::infinity());
    }
}


OIIO_ADD_TEST(FileFormatCTF, lut1d_half_domain_set_false)
{
    const std::string ctfFile("lut1d_half_domain_set_false.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile),
                          OCIO::Exception,
                          "Illegal 'halfDomain' attribute");
}

OIIO_ADD_TEST(FileFormatCTF, lut1d_raw_half_set_false)
{
    const std::string ctfFile("lut1d_raw_half_set_false.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile),
                          OCIO::Exception,
                          "Illegal 'rawHalfs' attribute");
}

OIIO_ADD_TEST(FileFormatCTF, lut1d_half_domain_raw_half_set)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("lut1d_half_domain_raw_half_set.clf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));

        OIIO_CHECK_ASSERT((bool)cachedFile);

        const OCIO::OpData::OpDataVec & opList =
            cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(),
                         OCIO::OpData::OpData::Lut1DType);
        const OCIO::OpData::Lut1D* pLut1d
            = dynamic_cast<const OCIO::OpData::Lut1D*>(opList[0]);
        OIIO_CHECK_ASSERT(pLut1d);

        OIIO_CHECK_ASSERT(pLut1d->isInputHalfDomain());
        OIIO_CHECK_ASSERT(pLut1d->isOutputRawHalfs());
        
        OIIO_CHECK_EQUAL(pLut1d->getArray().getValues()[0],
            OCIO::ConvertHalfBitsToFloat(0));
        OIIO_CHECK_EQUAL(pLut1d->getArray().getValues()[3],
            OCIO::ConvertHalfBitsToFloat(215));
        OIIO_CHECK_EQUAL(pLut1d->getArray().getValues()[6],
            OCIO::ConvertHalfBitsToFloat(294));
        OIIO_CHECK_EQUAL(pLut1d->getArray().getValues()[9],
            OCIO::ConvertHalfBitsToFloat(354));
        OIIO_CHECK_EQUAL(pLut1d->getArray().getValues()[12],
            OCIO::ConvertHalfBitsToFloat(403));
    }
}

OIIO_ADD_TEST(FileFormatCTF, lut1d_half_domain_invalid_entries)
{
    const std::string ctfFile("lut1d_half_domain_invalid_entries.clf");
    // this should fail with invalid entries exception because the number
    // of entries in the op is not 65536 (required when using half domain)
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile),
                          OCIO::Exception,
                          "65536 required for halfDomain");
}

OIIO_ADD_TEST(FileFormatCTF, inverseOfId_test)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("inverseOfId_test.clf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        OIIO_CHECK_ASSERT(cachedFile->m_transform->getInverseOfId() ==
                          "inverseOfIdTest");
    }
}

OIIO_ADD_TEST(FileFormatCTF, Range1)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("range_test1.clf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        const OCIO::OpData::OpDataVec & opList =
            cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(),
                         OCIO::OpData::OpData::RangeType);
        const OCIO::OpData::Range * pR =
            dynamic_cast<const OCIO::OpData::Range*>(opList[0]);
        OIIO_CHECK_ASSERT(pR);

        OIIO_CHECK_EQUAL(pR->getInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
        OIIO_CHECK_EQUAL(pR->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

        // NB: All exactly representable as flt
        OIIO_CHECK_EQUAL(pR->getMinInValue(), 16.);
        OIIO_CHECK_EQUAL(pR->getMaxInValue(), 235.);
        OIIO_CHECK_EQUAL(pR->getMinOutValue(), -0.5);
        OIIO_CHECK_EQUAL(pR->getMaxOutValue(), 2.);

        OIIO_CHECK_ASSERT(!pR->minIsEmpty());
        OIIO_CHECK_ASSERT(!pR->maxIsEmpty());
    }
}

OIIO_ADD_TEST(FileFormatCTF, Range2)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("range_test2.clf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        const OCIO::OpData::OpDataVec & opList =
            cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(),
                         OCIO::OpData::OpData::RangeType);
        const OCIO::OpData::Range * pR =
            dynamic_cast<const OCIO::OpData::Range*>(opList[0]);
        OIIO_CHECK_ASSERT(pR);
        OIIO_CHECK_EQUAL(pR->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_EQUAL(pR->getOutputBitDepth(), OCIO::BIT_DEPTH_F16);

        OIIO_CHECK_EQUAL((float)pR->getMinInValue(), 0.1f);
        OIIO_CHECK_EQUAL((float)pR->getMinOutValue(), -0.1f);

        OIIO_CHECK_ASSERT(!pR->minIsEmpty());
        OIIO_CHECK_ASSERT(pR->maxIsEmpty());
    }
}

OIIO_ADD_TEST(FileFormatCTF, Range3)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("range_test3.clf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        const OCIO::OpData::OpDataVec & opList =
            cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(),
                         OCIO::OpData::OpData::RangeType);
        const OCIO::OpData::Range * pR =
            dynamic_cast<const OCIO::OpData::Range*>(opList[0]);
        OIIO_CHECK_ASSERT(pR);
        OIIO_CHECK_EQUAL(pR->getInputBitDepth(), OCIO::BIT_DEPTH_F16);
        OIIO_CHECK_EQUAL(pR->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

        OIIO_CHECK_ASSERT(pR->minIsEmpty());
        OIIO_CHECK_ASSERT(pR->maxIsEmpty());
    }
}


OIIO_ADD_TEST(FileFormatCTF, Gamma1)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("gamma_test1.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when Gamma is supported
        // This test is for an old (< 1.5) transform file that contains
        // only a single GammaParams :
        // - R, G and B set to same parameters
        // - A set to identity
        const Color::GammaOp* pG
            = dynamic_cast<const Color::GammaOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pG);
        BOOST_CHECK(pG->getInputBitDepth() == SYNCOLOR::BIT_DEPTH_16f);
        BOOST_CHECK(pG->getOutputBitDepth() == SYNCOLOR::BIT_DEPTH_8i);

        BOOST_CHECK(pG->getGammaStyle() == Color::GammaOp::GAMMA_BASIC_FWD);

        Color::GammaOp::Params params;
        params.push_back(2.4);

        BOOST_CHECK(pG->getRedParams() == params);
        BOOST_CHECK(pG->getGreenParams() == params);
        BOOST_CHECK(pG->getBlueParams() == params);
        BOOST_CHECK(Color::GammaOp::isIdentityParameters(
            pG->getAlphaParams(),
            pG->getGammaStyle()));

        BOOST_CHECK(!pG->areAllComponentsEqual());
        BOOST_CHECK(pG->isNonChannelDependent()); // RGB equals, A identity
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, Gamma2)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("gamma_test2.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when Gamma is supported
        // This test is for an old (< 1.5) transform file that contains
        // a different GammaParams for every channels :
        // - R, G and B set to different parameters
        // - A set to identity

        const Color::GammaOp* pG
            = dynamic_cast<const Color::GammaOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pG);
        BOOST_CHECK(pG->getInputBitDepth() == SYNCOLOR::BIT_DEPTH_32f);
        BOOST_CHECK(pG->getOutputBitDepth() == SYNCOLOR::BIT_DEPTH_10i);

        BOOST_CHECK(pG->getGammaStyle() == Color::GammaOp::GAMMA_BASIC_REV);

        Color::GammaOp::Params paramsR;
        paramsR.push_back(2.4);
        Color::GammaOp::Params paramsG;
        paramsG.push_back(2.35);
        Color::GammaOp::Params paramsB;
        paramsB.push_back(2.2);

        BOOST_CHECK(pG->getRedParams() == paramsR);
        BOOST_CHECK(pG->getGreenParams() == paramsG);
        BOOST_CHECK(pG->getBlueParams() == paramsB);
        BOOST_CHECK(Color::GammaOp::isIdentityParameters(
            pG->getAlphaParams(),
            pG->getGammaStyle()));

        BOOST_CHECK(!pG->areAllComponentsEqual());
        BOOST_CHECK(!pG->isNonChannelDependent());
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, Gamma3)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("gamma_test3.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when Gamma is supported
        // This test is for an old (< 1.5) transform file that contains
        // a single GammaParams :
        // - R, G and B set to same parameters (precision test)
        // - A set to identity

        const Color::GammaOp* pG
            = dynamic_cast<const Color::GammaOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pG);
        BOOST_CHECK(pG->getInputBitDepth() == SYNCOLOR::BIT_DEPTH_16f);
        BOOST_CHECK(pG->getOutputBitDepth() == SYNCOLOR::BIT_DEPTH_8i);

        BOOST_CHECK(pG->getGammaStyle() == Color::GammaOp::GAMMA_MONCURVE_FWD);

        Color::GammaOp::Params params;
        params.push_back(1. / 0.45);
        params.push_back(0.099);

        // This is a precision test to ensure we can recreate a double that is
        // exactly equal to 1/0.45, which is required to implement rec 709 exactly.
        BOOST_CHECK(pG->getRedParams() == params);
        BOOST_CHECK(pG->getGreenParams() == params);
        BOOST_CHECK(pG->getBlueParams() == params);
        BOOST_CHECK(Color::GammaOp::isIdentityParameters(
            pG->getAlphaParams(),
            pG->getGammaStyle()));

        BOOST_CHECK(!pG->areAllComponentsEqual());
        BOOST_CHECK(pG->isNonChannelDependent()); // RGB equals, A identity
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, Gamma4)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("gamma_test4.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when Gamma is supported
        // This test is for an old (< 1.5) transform file that contains
        // a different GammaParams for every channels :
        // - R, G and B set to different parameters (attributes order test)
        // - A set to identity

        const Color::GammaOp* pG
            = dynamic_cast<const Color::GammaOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pG);
        BOOST_CHECK(pG->getInputBitDepth() == SYNCOLOR::BIT_DEPTH_16f);
        BOOST_CHECK(pG->getOutputBitDepth() == SYNCOLOR::BIT_DEPTH_32f);

        BOOST_CHECK(pG->getGammaStyle() == Color::GammaOp::GAMMA_MONCURVE_REV);

        // NB: The file lists the params in reverse order, which should be ok.
        Color::GammaOp::Params paramsR;
        paramsR.push_back(2.2);
        paramsR.push_back(0.001);
        Color::GammaOp::Params paramsG;
        paramsG.push_back(2.4);
        paramsG.push_back(0.01);
        Color::GammaOp::Params paramsB;
        paramsB.push_back(2.6);
        paramsB.push_back(0.1);

        BOOST_CHECK(pG->getRedParams() == paramsR);
        BOOST_CHECK(pG->getGreenParams() == paramsG);
        BOOST_CHECK(pG->getBlueParams() == paramsB);

        BOOST_CHECK(Color::GammaOp::isIdentityParameters(
            pG->getAlphaParams(),
            pG->getGammaStyle()));

        BOOST_CHECK(!pG->areAllComponentsEqual());
        BOOST_CHECK(!pG->isNonChannelDependent());
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, Gamma5)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("gamma_test5.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when Gamma is supported - this test should still throw
        // This test is for an old (< 1.5) transform file that contains
        // an invalid GammaParams for the A channel

        const std::string filename = "/gamma_test5.ctf";
        Color::XMLTransformReader reader;

        try
        {
            reader.parseFile(path + filename);
            BOOST_ERROR("No exception");
        }
        catch (const Color::Exception& e)
        {
            // gamma_test5.xml contains invalid GammaParams for alpha for version < 1.5
            BOOST_CHECK_EQUAL(e.getErrorCode(),
                SYNCOLOR::ERROR_XML_READER_ILLEGAL_CHAN);
        }
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, Gamma6)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("gamma_test6.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when Gamma is supported
        // This test is for an old (< 1.5) transform file that contains
        // a single GammaParams with identity values :
        // - R, G and B set to identity parameters (identity test)
        // - A set to identity

        const Color::GammaOp* pG
            = dynamic_cast<const Color::GammaOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pG);
        BOOST_CHECK(pG->getInputBitDepth() == SYNCOLOR::BIT_DEPTH_16f);
        BOOST_CHECK(pG->getOutputBitDepth() == SYNCOLOR::BIT_DEPTH_8i);

        BOOST_CHECK(pG->getGammaStyle() == Color::GammaOp::GAMMA_MONCURVE_FWD);

        BOOST_CHECK(pG->areAllComponentsEqual());  // RGBA equals
        BOOST_CHECK(pG->isNonChannelDependent()); // RGB equal, A identity
        BOOST_CHECK(pG->isIdentity());          // RGBA identity
        BOOST_CHECK(!pG->isNoOp());             // inputDepth != outputDepth
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, Gamma_wrong_power)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("gamma_wrong_power.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when Gamma is supported - this test should still throw
        try
        {
            reader.parseFile(path + filename);
            BOOST_ERROR("One gamma power value is outside the accepted range, it should have failed");
        }
        catch (Color::Exception& e)
        {
            BOOST_CHECK_EQUAL(e.getErrorCode(), SYNCOLOR::ERROR_TRANSFORM_ALL_PARAM_TOO_LOW);
        }
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, Gamma_alpha1)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("gamma_alpha_test1.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when Gamma is supported
        // This test is for a new (>= 1.5) transform file that contains
        // a single GammaParams :
        // - R, G and B set to same parameters
        // - A set to identity

        const Color::GammaOp* pG
            = dynamic_cast<const Color::GammaOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pG);
        BOOST_CHECK(pG->getInputBitDepth() == SYNCOLOR::BIT_DEPTH_16f);
        BOOST_CHECK(pG->getOutputBitDepth() == SYNCOLOR::BIT_DEPTH_8i);

        BOOST_CHECK(pG->getGammaStyle() == Color::GammaOp::GAMMA_BASIC_FWD);

        Color::GammaOp::Params params;
        params.push_back(2.4);

        BOOST_CHECK(pG->getRedParams() == params);
        BOOST_CHECK(pG->getGreenParams() == params);
        BOOST_CHECK(pG->getBlueParams() == params);
        BOOST_CHECK(Color::GammaOp::isIdentityParameters(
            pG->getAlphaParams(),
            pG->getGammaStyle()));

        BOOST_CHECK(!pG->areAllComponentsEqual());
        BOOST_CHECK(pG->isNonChannelDependent()); // RGB equals, A identity
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, Gamma_alpha2)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("gamma_alpha_test2.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when Gamma is supported
        // This test is for a new (>= 1.5) transform file that contains
        // a different GammaParams for every channels :
        // - R, G, B and A set to different parameters

        const Color::GammaOp* pG
            = dynamic_cast<const Color::GammaOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pG);
        BOOST_CHECK(pG->getInputBitDepth() == SYNCOLOR::BIT_DEPTH_32f);
        BOOST_CHECK(pG->getOutputBitDepth() == SYNCOLOR::BIT_DEPTH_10i);

        BOOST_CHECK(pG->getGammaStyle() == Color::GammaOp::GAMMA_BASIC_REV);

        Color::GammaOp::Params paramsR;
        paramsR.push_back(2.4);
        Color::GammaOp::Params paramsG;
        paramsG.push_back(2.35);
        Color::GammaOp::Params paramsB;
        paramsB.push_back(2.2);
        Color::GammaOp::Params paramsA;
        paramsA.push_back(2.5);

        BOOST_CHECK(pG->getRedParams() == paramsR);
        BOOST_CHECK(pG->getGreenParams() == paramsG);
        BOOST_CHECK(pG->getBlueParams() == paramsB);
        BOOST_CHECK(pG->getAlphaParams() == paramsA);

        BOOST_CHECK(!pG->areAllComponentsEqual());
        BOOST_CHECK(!pG->isNonChannelDependent());
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, Gamma_alpha3)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("gamma_alpha_test3.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when Gamma is supported
        // This test is for a new (>= 1.5) transform file that contains
        // a single GammaParams :
        // - R, G and B set to same parameters (precision test)
        // - A set to identity

        const Color::GammaOp* pG
            = dynamic_cast<const Color::GammaOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pG);
        BOOST_CHECK(pG->getInputBitDepth() == SYNCOLOR::BIT_DEPTH_16f);
        BOOST_CHECK(pG->getOutputBitDepth() == SYNCOLOR::BIT_DEPTH_8i);

        BOOST_CHECK(pG->getGammaStyle() == Color::GammaOp::GAMMA_MONCURVE_FWD);

        Color::GammaOp::Params params;
        params.push_back(1. / 0.45);
        params.push_back(0.099);

        // This is a precision test to ensure we can recreate a double that is
        // exactly equal to 1/0.45, which is required to implement rec 709 exactly.
        BOOST_CHECK(pG->getRedParams() == params);
        BOOST_CHECK(pG->getGreenParams() == params);
        BOOST_CHECK(pG->getBlueParams() == params);
        BOOST_CHECK(Color::GammaOp::isIdentityParameters(
            pG->getAlphaParams(),
            pG->getGammaStyle()));

        BOOST_CHECK(!pG->areAllComponentsEqual());
        BOOST_CHECK(pG->isNonChannelDependent()); // RGB equals, A identity
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, Gamma_alpha4)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("gamma_alpha_test4.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when Gamma is supported
        // This test is for a new (>= 1.5) transform file that contains
        // a different GammaParams for every channels :
        // - R, G, B and A set to different parameters (attributes order test)

        const Color::GammaOp* pG
            = dynamic_cast<const Color::GammaOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pG);
        BOOST_CHECK(pG->getInputBitDepth() == SYNCOLOR::BIT_DEPTH_16f);
        BOOST_CHECK(pG->getOutputBitDepth() == SYNCOLOR::BIT_DEPTH_32f);

        BOOST_CHECK(pG->getGammaStyle() == Color::GammaOp::GAMMA_MONCURVE_REV);

        // NB: The file lists the params in reverse order, which should be ok.
        Color::GammaOp::Params paramsR;
        paramsR.push_back(2.2);
        paramsR.push_back(0.001);
        Color::GammaOp::Params paramsG;
        paramsG.push_back(2.4);
        paramsG.push_back(0.01);
        Color::GammaOp::Params paramsB;
        paramsB.push_back(2.6);
        paramsB.push_back(0.1);
        Color::GammaOp::Params paramsA;
        paramsA.push_back(2.0);
        paramsA.push_back(0.0001);

        BOOST_CHECK(pG->getRedParams() == paramsR);
        BOOST_CHECK(pG->getGreenParams() == paramsG);
        BOOST_CHECK(pG->getBlueParams() == paramsB);
        BOOST_CHECK(pG->getAlphaParams() == paramsA);

        BOOST_CHECK(!pG->areAllComponentsEqual());
        BOOST_CHECK(!pG->isNonChannelDependent());
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, Gamma_alpha5)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("gamma_alpha_test5.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when Gamma is supported
        // This test is for a new (>= 1.5) transform file that contains
        // a GammaParams with no channel specified :
        // - R, G and B set to same parameters
        // and a GammaParams for the A channel :
        // - A set to different parameters

        const Color::GammaOp* pG
            = dynamic_cast<const Color::GammaOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pG);
        BOOST_CHECK(pG->getInputBitDepth() == SYNCOLOR::BIT_DEPTH_16f);
        BOOST_CHECK(pG->getOutputBitDepth() == SYNCOLOR::BIT_DEPTH_8i);

        BOOST_CHECK(pG->getGammaStyle() == Color::GammaOp::GAMMA_MONCURVE_FWD);

        Color::GammaOp::Params params;
        params.push_back(1. / 0.45);
        params.push_back(0.099);

        Color::GammaOp::Params paramsA;
        paramsA.push_back(1.7);
        paramsA.push_back(0.33);

        // This is a precision test to ensure we can recreate a double that is
        // exactly equal to 1/0.45, which is required to implement rec 709 exactly.
        BOOST_CHECK(pG->getRedParams() == params);
        BOOST_CHECK(pG->getGreenParams() == params);
        BOOST_CHECK(pG->getBlueParams() == params);
        BOOST_CHECK(pG->getAlphaParams() == paramsA);

        BOOST_CHECK(!pG->areAllComponentsEqual());
        BOOST_CHECK(!pG->isNonChannelDependent());
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, Gamma_alpha6)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("gamma_alpha_test6.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when Gamma is supported - this test should still throw
        // This test is for an new (>= 1.5) transform file that contains
        // an invalid GammaParams for the A channel (missing offset attribute)

        const std::string filename = "/gamma_alpha_test6.ctf";
        Color::XMLTransformReader reader;

        try
        {
            reader.parseFile(path + filename);
            BOOST_ERROR("No exception");
        }
        catch (const Color::Exception& e)
        {
            BOOST_CHECK_EQUAL(e.getErrorCode(),
                SYNCOLOR::ERROR_XML_READER_MISSING_REQD_PARAM);
        }
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, invalid_version)
{
    const std::string ctfFile("process_list_invalid_version.ctf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile),
                          OCIO::Exception,
                          "is not a valid version");
}

OIIO_ADD_TEST(FileFormatCTF, valid_version)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("process_list_valid_version.ctf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        
        const OCIO::CTF::Version ctfVersion = cachedFile->m_transform->getCTFVersion();
        OIIO_CHECK_EQUAL(ctfVersion, OCIO::CTF::CTF_PROCESS_LIST_VERSION_1_4);
    }
}

OIIO_ADD_TEST(FileFormatCTF, higher_version)
{
    const std::string ctfFile("process_list_higher_version.ctf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile),
                          OCIO::Exception,
                          "Unsupported transform file version");
}

OIIO_ADD_TEST(FileFormatCTF, version_revision)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("process_list_version_revision.ctf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));

        const OCIO::CTF::Version ctfVersion = cachedFile->m_transform->getCTFVersion();
        const OCIO::CTF::Version ver(1, 3, 10);
        OIIO_CHECK_EQUAL(ctfVersion, ver);
        OIIO_CHECK_ASSERT(OCIO::CTF::CTF_PROCESS_LIST_VERSION_1_3 < ctfVersion);
        OIIO_CHECK_ASSERT(ctfVersion < OCIO::CTF::CTF_PROCESS_LIST_VERSION_1_4);
    }
}

OIIO_ADD_TEST(FileFormatCTF, no_version)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("process_list_no_version.ctf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));

        const OCIO::CTF::Version ctfVersion = cachedFile->m_transform->getCTFVersion();
        OIIO_CHECK_EQUAL(ctfVersion, OCIO::CTF::CTF_PROCESS_LIST_VERSION_1_2);
    }
}

OIIO_ADD_TEST(FileFormatCTF, ExposureContrastVideo)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("reader_exposure_contrast_video.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when ExposureContrast is supported

        const Color::ExposureContrastOp* pEC
            = dynamic_cast<const Color::ExposureContrastOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pEC);
        BOOST_CHECK(pEC->getInputBitDepth() == SYNCOLOR::BIT_DEPTH_16f);
        BOOST_CHECK(pEC->getOutputBitDepth() == SYNCOLOR::BIT_DEPTH_16f);

        BOOST_CHECK(pEC->getStyle() == Color::ExposureContrastOp::STYLE_VIDEO);

        BOOST_CHECK_EQUAL(pEC->getExposure()->getValue(), 0.0);
        BOOST_CHECK_EQUAL(pEC->getContrast()->getValue(), 1.0);
        BOOST_CHECK_EQUAL(pEC->getPivot(), 0.18);

        BOOST_CHECK(pEC->isDynamic());
        BOOST_CHECK(pEC->getExposure()->isDynamic());
        BOOST_CHECK(pEC->getContrast()->isDynamic());
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, ExposureContrastLog)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("reader_exposure_contrast_log.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when ExposureContrast is supported

        const Color::ExposureContrastOp* pEC
            = dynamic_cast<const Color::ExposureContrastOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pEC);
        BOOST_CHECK(pEC->getInputBitDepth() == SYNCOLOR::BIT_DEPTH_32f);
        BOOST_CHECK(pEC->getOutputBitDepth() == SYNCOLOR::BIT_DEPTH_32f);

        BOOST_CHECK(pEC->getStyle() == Color::ExposureContrastOp::STYLE_LOGARITHMIC);

        BOOST_CHECK_EQUAL(pEC->getExposure()->getValue(), -1.5);
        BOOST_CHECK_EQUAL(pEC->getContrast()->getValue(), 0.5);
        BOOST_CHECK_EQUAL(pEC->getGamma()->getValue(), 1.2);
        BOOST_CHECK_EQUAL(pEC->getPivot(), 0.18);

        BOOST_CHECK(pEC->isDynamic());
        BOOST_CHECK(pEC->getExposure()->isDynamic());
        BOOST_CHECK(pEC->getContrast()->isDynamic());
        BOOST_CHECK(pEC->getGamma()->isDynamic());
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, ExposureContrastLinear)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("reader_exposure_contrast_linear.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when ExposureContrast is supported

        const Color::ExposureContrastOp* pEC
            = dynamic_cast<const Color::ExposureContrastOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pEC);
        BOOST_CHECK(pEC->getInputBitDepth() == SYNCOLOR::BIT_DEPTH_16f);
        BOOST_CHECK(pEC->getOutputBitDepth() == SYNCOLOR::BIT_DEPTH_32f);

        BOOST_CHECK(pEC->getStyle() == Color::ExposureContrastOp::STYLE_LINEAR);

        BOOST_CHECK_EQUAL(pEC->getExposure()->getValue(), 0.65);
        BOOST_CHECK_EQUAL(pEC->getContrast()->getValue(), 1.2);
        BOOST_CHECK_EQUAL(pEC->getGamma()->getValue(), 0.5);
        BOOST_CHECK_EQUAL(pEC->getPivot(), 1.0);

        BOOST_CHECK(pEC->isDynamic());
        BOOST_CHECK(pEC->getExposure()->isDynamic());
        BOOST_CHECK(pEC->getContrast()->isDynamic());
        BOOST_CHECK(pEC->getGamma()->isDynamic());
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, ExposureContrastBadStyle)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("reader_exposure_contrast_bad_style.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when ExposureContrast is supported - this test should still throw
        try
        {
            reader.parseFile(path + filename);
        }
        catch (Color::Exception& e)
        {
            BOOST_CHECK(e.getErrorCode()
                == SYNCOLOR::ERROR_TRANSFORM_EXPOSURE_CONTRAST_UNKNOWN_STYLE);
            return;
        }
        BOOST_CHECK(false);
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, ExposureContrastMissingParam)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("reader_exposure_contrast_missing_param.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when ExposureContrast is supported - this test should still throw
        try
        {
            reader.parseFile(path + filename);
        }
        catch (Color::Exception& e)
        {
            BOOST_CHECK(e.getErrorCode()
                == SYNCOLOR::ERROR_XML_READER_MISSING_REQD_PARAM);
            return;
        }
        BOOST_CHECK(false);
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, ExposureContrastNoGamma)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("reader_exposure_contrast_no_gamma.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when ExposureContrast is supported
        const Color::ExposureContrastOp* pEC
            = dynamic_cast<const Color::ExposureContrastOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pEC);
        BOOST_CHECK(pEC->getInputBitDepth() == SYNCOLOR::BIT_DEPTH_16f);
        BOOST_CHECK(pEC->getOutputBitDepth() == SYNCOLOR::BIT_DEPTH_16f);

        BOOST_CHECK(pEC->getStyle() == Color::ExposureContrastOp::STYLE_VIDEO);

        BOOST_CHECK_EQUAL(pEC->getExposure()->getValue(), 0.2);
        BOOST_CHECK_EQUAL(pEC->getContrast()->getValue(), 0.65);
        BOOST_CHECK_EQUAL(pEC->getGamma()->getValue(), 1.0);
        BOOST_CHECK_EQUAL(pEC->getPivot(), 0.23);

        BOOST_CHECK(!pEC->isDynamic());
        BOOST_CHECK(!pEC->getExposure()->isDynamic());
        BOOST_CHECK(!pEC->getContrast()->isDynamic());
        BOOST_CHECK(!pEC->getGamma()->isDynamic());
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, ExposureContrastNoGammaDynamic)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("reader_exposure_contrast_no_gamma_dynamic.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when ExposureContrast is supported

        const Color::ExposureContrastOp* pEC
            = dynamic_cast<const Color::ExposureContrastOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pEC);
        BOOST_CHECK(pEC->getInputBitDepth() == SYNCOLOR::BIT_DEPTH_16f);
        BOOST_CHECK(pEC->getOutputBitDepth() == SYNCOLOR::BIT_DEPTH_16f);

        BOOST_CHECK(pEC->getStyle() == Color::ExposureContrastOp::STYLE_VIDEO);

        BOOST_CHECK_EQUAL(pEC->getExposure()->getValue(), 0.42);
        BOOST_CHECK_EQUAL(pEC->getContrast()->getValue(), 1.3);
        BOOST_CHECK_EQUAL(pEC->getGamma()->getValue(), 1.0);
        BOOST_CHECK_EQUAL(pEC->getPivot(), 0.18);

        BOOST_CHECK(pEC->isDynamic());
        BOOST_CHECK(!pEC->getExposure()->isDynamic());
        BOOST_CHECK(!pEC->getContrast()->isDynamic());
        BOOST_CHECK(pEC->getGamma()->isDynamic());
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, ecNotDynamic)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("reader_exposure_contrast_not_dynamic.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when ExposureContrast is supported

        const Color::ExposureContrastOp* pEC
            = dynamic_cast<const Color::ExposureContrastOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pEC);

        BOOST_CHECK(!pEC->isDynamic());
        BOOST_CHECK(!pEC->getExposure()->isDynamic());
        BOOST_CHECK(!pEC->getContrast()->isDynamic());
        BOOST_CHECK(!pEC->getGamma()->isDynamic());
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, ecExposureOnlyDynamic)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("reader_exposure_only_dynamic.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when ExposureContrast is supported
        const boost::shared_ptr<Color::Transform>& pTransform
            = reader.getTransform();
        BOOST_REQUIRE(pTransform.use_count() == 1);

        const Color::ExposureContrastOp* pEC
            = dynamic_cast<const Color::ExposureContrastOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pEC);

        BOOST_CHECK(pEC->isDynamic());
        BOOST_CHECK(pEC->getExposure()->isDynamic());
        BOOST_CHECK(!pEC->getContrast()->isDynamic());
        BOOST_CHECK(!pEC->getGamma()->isDynamic());
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, Log_Log10)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("log_log10.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when Log is supported
        BOOST_CHECK_EQUAL(pTransform->getInputDescriptor(), "inputDesc");
        BOOST_CHECK_EQUAL(pTransform->getOutputDescriptor(), "outputDesc");

        const Color::LogOp* pLog
            = dynamic_cast<const Color::LogOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pLog);
        BOOST_CHECK_EQUAL(pLog->getInputBitDepth(), SYNCOLOR::BIT_DEPTH_32f);
        BOOST_CHECK_EQUAL(pLog->getOutputBitDepth(), SYNCOLOR::BIT_DEPTH_16f);
        BOOST_CHECK_EQUAL(pLog->getLogStyle(), Color::LogOp::LOG10);

        BOOST_CHECK(pLog->allComponentsEqual());

        BOOST_CHECK_EQUAL(pLog->getRedParams().size(), 0u);
        BOOST_CHECK_EQUAL(pLog->getGreenParams().size(), 0u);
        BOOST_CHECK_EQUAL(pLog->getBlueParams().size(), 0u);
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, Log_Log2)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("log_log2.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when Log is supported
        BOOST_CHECK_EQUAL(pTransform->getInputDescriptor(), "inputDesc");
        BOOST_CHECK_EQUAL(pTransform->getOutputDescriptor(), "outputDesc");

        const Color::LogOp* pLog
            = dynamic_cast<const Color::LogOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pLog);
        BOOST_CHECK_EQUAL(pLog->getInputBitDepth(), SYNCOLOR::BIT_DEPTH_32f);
        BOOST_CHECK_EQUAL(pLog->getOutputBitDepth(), SYNCOLOR::BIT_DEPTH_32f);
        BOOST_CHECK_EQUAL(pLog->getLogStyle(), Color::LogOp::LOG2);

        BOOST_CHECK(pLog->allComponentsEqual());

        BOOST_CHECK_EQUAL(pLog->getRedParams().size(), 0u);
        BOOST_CHECK_EQUAL(pLog->getGreenParams().size(), 0u);
        BOOST_CHECK_EQUAL(pLog->getBlueParams().size(), 0u);
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, Log_AntiLog10)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("log_antilog10.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when Log is supported
        BOOST_CHECK_EQUAL(pTransform->getInputDescriptor(), "inputDesc");
        BOOST_CHECK_EQUAL(pTransform->getOutputDescriptor(), "outputDesc");

        const Color::LogOp* pLog
            = dynamic_cast<const Color::LogOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pLog);
        BOOST_CHECK_EQUAL(pLog->getInputBitDepth(), SYNCOLOR::BIT_DEPTH_10i);
        BOOST_CHECK_EQUAL(pLog->getOutputBitDepth(), SYNCOLOR::BIT_DEPTH_16f);
        BOOST_CHECK_EQUAL(pLog->getLogStyle(), Color::LogOp::ANTI_LOG10);

        BOOST_CHECK(pLog->allComponentsEqual());

        BOOST_CHECK_EQUAL(pLog->getRedParams().size(), 0u);
        BOOST_CHECK_EQUAL(pLog->getGreenParams().size(), 0u);
        BOOST_CHECK_EQUAL(pLog->getBlueParams().size(), 0u);
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, Log_AntiLog2)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("log_antilog2.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when Log is supported
        BOOST_CHECK_EQUAL(pTransform->getInputDescriptor(), "inputDesc");
        BOOST_CHECK_EQUAL(pTransform->getOutputDescriptor(), "outputDesc");

        const Color::LogOp* pLog
            = dynamic_cast<const Color::LogOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pLog);
        BOOST_CHECK_EQUAL(pLog->getInputBitDepth(), SYNCOLOR::BIT_DEPTH_16f);
        BOOST_CHECK_EQUAL(pLog->getOutputBitDepth(), SYNCOLOR::BIT_DEPTH_8i);
        BOOST_CHECK_EQUAL(pLog->getLogStyle(), Color::LogOp::ANTI_LOG2);

        BOOST_CHECK(pLog->allComponentsEqual());

        BOOST_CHECK_EQUAL(pLog->getRedParams().size(), 0u);
        BOOST_CHECK_EQUAL(pLog->getGreenParams().size(), 0u);
        BOOST_CHECK_EQUAL(pLog->getBlueParams().size(), 0u);
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, Log_LogToLin)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("log_logtolin.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when Log is supported
        BOOST_CHECK_EQUAL(pTransform->getInputDescriptor(), "inputDesc");
        BOOST_CHECK_EQUAL(pTransform->getOutputDescriptor(), "outputDesc");

        const Color::LogOp* pLog
            = dynamic_cast<const Color::LogOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pLog);
        BOOST_CHECK_EQUAL(pLog->getInputBitDepth(), SYNCOLOR::BIT_DEPTH_32f);
        BOOST_CHECK_EQUAL(pLog->getOutputBitDepth(), SYNCOLOR::BIT_DEPTH_32f);
        BOOST_CHECK_EQUAL(pLog->getLogStyle(), Color::LogOp::LOG_TO_LIN);

        BOOST_CHECK(pLog->allComponentsEqual());

        const Color::LogOp::Params& params = pLog->getRedParams();
        BOOST_REQUIRE_EQUAL(params.size(), 5u);
        BOOST_CHECK_EQUAL(params[0], 0.6);    // gamma
        BOOST_CHECK_EQUAL(params[1], 685.);   // refWhite
        BOOST_CHECK_EQUAL(params[2], 95.);    // refBlack
        BOOST_CHECK_EQUAL(params[3], 1.0);    // highlight
        BOOST_CHECK_EQUAL(params[4], 0.0005); // shadow
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, Log_LintToLog_3Chan)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("log_lintolog_3chan.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile),
                              OCIO::Exception,
                              "No color operator");
        /* TODO: Adjust when Log is supported
        BOOST_CHECK_EQUAL(pTransform->getInputDescriptor(), "inputDesc");
        BOOST_CHECK_EQUAL(pTransform->getOutputDescriptor(), "outputDesc");

        const Color::LogOp* pLog
            = dynamic_cast<const Color::LogOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pLog);
        BOOST_CHECK_EQUAL(pLog->getInputBitDepth(), SYNCOLOR::BIT_DEPTH_32f);
        BOOST_CHECK_EQUAL(pLog->getOutputBitDepth(), SYNCOLOR::BIT_DEPTH_32f);
        BOOST_CHECK_EQUAL(pLog->getLogStyle(), Color::LogOp::LIN_TO_LOG);

        BOOST_CHECK(!pLog->allComponentsEqual());

        const Color::LogOp::Params& paramsR = pLog->getRedParams();
        BOOST_REQUIRE_EQUAL(paramsR.size(), 5u);
        BOOST_CHECK_EQUAL(paramsR[0], 0.5);   // gamma
        BOOST_CHECK_EQUAL(paramsR[1], 681.);  // refWhite
        BOOST_CHECK_EQUAL(paramsR[2], 95.);   // refBlack
        BOOST_CHECK_EQUAL(paramsR[3], 0.9);   // highlight
        BOOST_CHECK_EQUAL(paramsR[4], 0.0045);// shadow

        const Color::LogOp::Params& paramsG = pLog->getGreenParams();
        BOOST_REQUIRE_EQUAL(paramsG.size(), 5u);
        BOOST_CHECK_EQUAL(paramsG[0], 0.6);   // gamma
        BOOST_CHECK_EQUAL(paramsG[1], 682.);  // refWhite
        BOOST_CHECK_EQUAL(paramsG[2], 94.);   // refBlack
        BOOST_CHECK_EQUAL(paramsG[3], 1.);    // highlight
        BOOST_CHECK_EQUAL(paramsG[4], 0.0025);// shadow

        const Color::LogOp::Params& paramsB = pLog->getBlueParams();
        BOOST_REQUIRE_EQUAL(paramsB.size(), 5u);
        BOOST_CHECK_EQUAL(paramsB[0], 0.65);  // gamma
        BOOST_CHECK_EQUAL(paramsB[1], 683.);  // refWhite
        BOOST_CHECK_EQUAL(paramsB[2], 93.);   // refBlack
        BOOST_CHECK_EQUAL(paramsB[3], 0.8);   // highlight
        BOOST_CHECK_EQUAL(paramsB[4], 0.0035);// shadow
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, Log_invalidstyle)
{
    const std::string ctfFile("log_invalidstyle.ctf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile), OCIO::Exception, "No color operator");
    /* TODO: Adjust when Log is supported - this test should still throw
    try
    {
        reader.parseFile(filename);
        BOOST_ERROR("No exception");
    }
    catch (const Color::Exception& e)
    {
        BOOST_CHECK_EQUAL(
            e.getErrorCode(),
            SYNCOLOR::ERROR_TRANSFORM_LOG_UNKNOWN_STYLE);
    }
    */
}

OIIO_ADD_TEST(FileFormatCTF, log_with_faulty_version_test)
{
    const std::string ctfFile("log_log10_faulty_version.ctf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile), OCIO::Exception, "No color operator");
    /* TODO: Adjust when Log is supported - this test should still throw
    try
    {
        reader.parseFile(filename);
        BOOST_ERROR("No exception");
    }
    catch (const Color::Exception& e)
    {
        BOOST_CHECK_EQUAL(
            e.getErrorCode(),
            SYNCOLOR::ERROR_XML_READER_OP_VERSION_MISMATCH);
    }
    */
}

OIIO_ADD_TEST(FileFormatCTF, CDL)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("cdl_clamp_fwd.clf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getInputDescriptor(), "inputDesc");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getOutputDescriptor(), "outputDesc");
        OIIO_CHECK_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::CDLType);
        const OCIO::OpData::CDL * pCDL = dynamic_cast<const OCIO::OpData::CDL*>(opList[0]);
        OIIO_CHECK_ASSERT(pCDL);

        OIIO_CHECK_EQUAL(pCDL->getId(), std::string("look 1"));
        OIIO_CHECK_EQUAL(pCDL->getName(), std::string("cdl"));

        OCIO::OpData::Descriptions descriptions = pCDL->getDescriptions();

        OIIO_CHECK_EQUAL(descriptions.getList().size(), 1u);
        OIIO_CHECK_EQUAL(descriptions.getList()[0], std::string("ASC CDL operation"));

        OIIO_CHECK_EQUAL(pCDL->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_EQUAL(pCDL->getOutputBitDepth(), OCIO::BIT_DEPTH_F16);

        OIIO_CHECK_EQUAL(pCDL->getCDLStyle(), OCIO::OpData::CDL::CDL_V1_2_FWD);
        std::string styleName(OCIO::OpData::CDL::GetCDLStyleName(pCDL->getCDLStyle()));
        OIIO_CHECK_EQUAL(styleName, "v1.2_Fwd");

        OIIO_CHECK_ASSERT(pCDL->getSlopeParams() 
            == OCIO::OpData::CDL::ChannelParams(1.35, 1.1, 0.71));
        OIIO_CHECK_ASSERT(pCDL->getOffsetParams()
            == OCIO::OpData::CDL::ChannelParams(0.05, -0.23, 0.11));
        OIIO_CHECK_ASSERT(pCDL->getPowerParams()
            == OCIO::OpData::CDL::ChannelParams(0.93, 0.81, 1.27));
        OIIO_CHECK_EQUAL(pCDL->getSaturation(), 1.239);
    }
}


OIIO_ADD_TEST(FileFormatCTF, CDL_invalidSOPNode)
{
    const std::string ctfFile("cdl_invalidSOP.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile), OCIO::Exception, 
                          "SOPNode: 3 values required");
}
        
OIIO_ADD_TEST(FileFormatCTF, CDL_invalidSatNode)
{
    const std::string ctfFile("cdl_invalidSat.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile), OCIO::Exception, 
                          "SatNode: non-single value");
}

OIIO_ADD_TEST(FileFormatCTF, CDL_missingSlope)
{
    const std::string ctfFile("cdl_missing_slope.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile), OCIO::Exception, 
                          "Required node 'Slope' is missing");
}

OIIO_ADD_TEST(FileFormatCTF, CDL_missingOffset)
{
    const std::string ctfFile("cdl_missing_offset.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile), OCIO::Exception, 
                          "Required node 'Offset' is missing");
}

OIIO_ADD_TEST(FileFormatCTF, CDL_missingPower)
{
    const std::string ctfFile("cdl_missing_power.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile), OCIO::Exception, 
                          "Required node 'Power' is missing");
}

OIIO_ADD_TEST(FileFormatCTF, CDL_missingStyle)
{
    const std::string ctfFile("cdl_missing_style.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile), OCIO::Exception, 
                          "Required attribute 'style' is missing");
}

OIIO_ADD_TEST(FileFormatCTF, CDL_invalidStyle)
{
    const std::string ctfFile("cdl_invalid_style.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile), OCIO::Exception, 
                          "Unknown style for CDL");
}

OIIO_ADD_TEST(FileFormatCTF, CDL_noSOPNode)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("cdl_noSOP.clf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::CDLType);
        const OCIO::OpData::CDL * pCDL = dynamic_cast<const OCIO::OpData::CDL*>(opList[0]);
        OIIO_CHECK_ASSERT(pCDL);

        OIIO_CHECK_ASSERT(pCDL->getSlopeParams()
            == OCIO::OpData::CDL::ChannelParams(1.0));
        OIIO_CHECK_ASSERT(pCDL->getOffsetParams()
            == OCIO::OpData::CDL::ChannelParams(0.0));
        OIIO_CHECK_ASSERT(pCDL->getPowerParams()
            == OCIO::OpData::CDL::ChannelParams(1.0));
        OIIO_CHECK_EQUAL(pCDL->getSaturation(), 1.239);
    }
}

OIIO_ADD_TEST(FileFormatCTF, CDL_noSatNode)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("cdl_noSat.clf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::CDLType);
        const OCIO::OpData::CDL * pCDL = dynamic_cast<const OCIO::OpData::CDL*>(opList[0]);
        OIIO_CHECK_ASSERT(pCDL);

        OIIO_CHECK_ASSERT(pCDL->getSlopeParams()
            == OCIO::OpData::CDL::ChannelParams(1.35, 1.1, 0.71));
        OIIO_CHECK_ASSERT(pCDL->getOffsetParams()
            == OCIO::OpData::CDL::ChannelParams(0.05, -0.23, 0.11));
        OIIO_CHECK_ASSERT(pCDL->getPowerParams()
            == OCIO::OpData::CDL::ChannelParams(0.93, 0.81, 1.27));
        OIIO_CHECK_EQUAL(pCDL->getSaturation(), 1.0);
    }
}

OIIO_ADD_TEST(FileFormatCTF, Dither)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("dither.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile), OCIO::Exception, "No color operator");
        /* TODO: adjust once Dither support has been added
        const Color::DitherOp* pDither = dynamic_cast<const Color::DitherOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pDither);

        BOOST_CHECK(pDither->getInputBitDepth() == SYNCOLOR::BIT_DEPTH_16f);
        BOOST_CHECK(pDither->getOutputBitDepth() == SYNCOLOR::BIT_DEPTH_12i);
        BOOST_CHECK(pDither->getDitherBitDepth() == SYNCOLOR::BIT_DEPTH_8i);
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, lut1d_hue_adjust_invalid_style)
{
    const std::string ctfFile("lut1d_hue_adjust_invalid_style.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile), OCIO::Exception, "Illegal 'hueAdjust' attribute");
}

OIIO_ADD_TEST(FileFormatCTF, look_test)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("matrix_bypass_true.ctf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);
        /* TODO: bypass is for CTF

        const Color::Op* pOp = pTransform->getOps().get(0);
        BOOST_REQUIRE(pOp);

        BOOST_CHECK(pOp->getBypass()->isDynamic());
        BOOST_CHECK(pOp->getBypass()->getValue());
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, look_test_true)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("matrix_bypass_false.ctf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);
        /* TODO: bypass is for CTF

        const Color::Op* pOp = pTransform->getOps().get(0);
        BOOST_REQUIRE(pOp);

        BOOST_CHECK(pOp->getBypass()->isDynamic());
        BOOST_CHECK(!pOp->getBypass()->getValue());
        */
    }
}


void checkNames(const OCIO::OpData::Metadata::NameList& actualNames,
    const OCIO::OpData::Metadata::NameList& expectedNames)
{
    OCIO::OpData::Metadata::NameList::const_iterator ait, eit, end;
    for(ait = actualNames.begin(), eit = expectedNames.begin(),
        end = actualNames.end(); ait != end; ++ait, ++eit)
    {
        OIIO_CHECK_EQUAL(*ait, *eit);
    }
}


OIIO_ADD_TEST(FileFormatCTF, Metadata)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("metadata.clf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        OIIO_CHECK_EQUAL(cachedFile->m_transform->getInputDescriptor(), "inputDesc");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getOutputDescriptor(), "outputDesc");

        // Ensure ops were not affected by metadata parsing
        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 1);

        const OCIO::OpData::Matrix* pMatrix = dynamic_cast<const OCIO::OpData::Matrix*>(opList[0]);
        OIIO_CHECK_ASSERT(pMatrix);
        OIIO_CHECK_EQUAL(pMatrix->getName(), "identity");

        OIIO_CHECK_EQUAL(pMatrix->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_EQUAL(pMatrix->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);

        const OCIO::OpData::Metadata& info = cachedFile->m_transform->getInfo();

        // Check element values
        //
        OIIO_CHECK_EQUAL(info["Copyright"].getValue(), "Copyright 2013 Autodesk");
        OIIO_CHECK_EQUAL(info["Release"].getValue(), "2015");
        OIIO_CHECK_EQUAL(info["InputColorSpace"]["Description"].getValue(),
                         "Input color space description");
        OIIO_CHECK_EQUAL(info["InputColorSpace"]["Profile"].getValue(),
                         "Input color space profile");
        OIIO_CHECK_EQUAL(info["InputColorSpace"]["Empty"].getValue(), "");
        OIIO_CHECK_EQUAL(info["OutputColorSpace"]["Description"].getValue(),
                         "Output color space description");
        OIIO_CHECK_EQUAL(info["OutputColorSpace"]["Profile"].getValue(),
                         "Output color space profile");
        OIIO_CHECK_EQUAL(info["Category"]["Name"].getValue(), "Category name");

        const OCIO::OpData::Metadata::Attributes& atts = info["Category"]["Name"].getAttributes();
        OIIO_CHECK_ASSERT(atts.size() == 2);
        OIIO_CHECK_EQUAL(atts[0].first, "att1");
        OIIO_CHECK_EQUAL(atts[0].second, "test1");
        OIIO_CHECK_EQUAL(atts[1].first, "att2");
        OIIO_CHECK_EQUAL(atts[1].second, "test2");

        // Check element children count
        //
        OIIO_CHECK_EQUAL(info.getItems().size(), 5u);
        OIIO_CHECK_EQUAL(info["InputColorSpace"].getItems().size(), 3u);
        OIIO_CHECK_EQUAL(info["OutputColorSpace"].getItems().size(), 2u);
        OIIO_CHECK_EQUAL(info["Category"].getItems().size(), 1u);

        // Check element ordering
        //

        // Info element
        {
            OCIO::OpData::Metadata::NameList expectedNames;
            expectedNames.push_back("Copyright");
            expectedNames.push_back("Release");
            expectedNames.push_back("InputColorSpace");
            expectedNames.push_back("OutputColorSpace");
            expectedNames.push_back("Category");

            checkNames(info.getItemsNames(), expectedNames);
        }

        // InputColorSpace element
        {
            OCIO::OpData::Metadata::NameList expectedNames;
            expectedNames.push_back("Description");
            expectedNames.push_back("Profile");
            expectedNames.push_back("Empty");

            checkNames(info["InputColorSpace"].getItemsNames(), expectedNames);
        }

        // OutputColorSpace element
        {
            OCIO::OpData::Metadata::NameList expectedNames;
            expectedNames.push_back("Description");
            expectedNames.push_back("Profile");

            checkNames(info["OutputColorSpace"].getItemsNames(), expectedNames);
        }

        // Category element
        {
            OCIO::OpData::Metadata::NameList expectedNames;
            expectedNames.push_back("Name");

            checkNames(info["Category"].getItemsNames(), expectedNames);
        }
    }
}


OIIO_ADD_TEST(FileFormatCTF, ACES)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("ACES_test1.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile), OCIO::Exception, "No color operator");

        /*
        TODO: ACES is from ctf
        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::ACESType);
        const OCIO::OpData::CDL * pCDL = dynamic_cast<const OCIO::OpData::CDL*>(opList[0]);
        OIIO_CHECK_ASSERT(pCDL);

        const Color::ACESOp* pA
            = dynamic_cast<const Color::ACESOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pA);
        BOOST_CHECK(pA->getInputBitDepth() == SYNCOLOR::BIT_DEPTH_16i);
        BOOST_CHECK(pA->getOutputBitDepth() == SYNCOLOR::BIT_DEPTH_32f);

        BOOST_CHECK(pA->getStyle() == Color::ACESOp::ACES_RED_MOD_03_REV);
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, ACES2)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("ACES_test2.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile), OCIO::Exception, "No color operator");

        /*
        TODO: ACES is from ctf

        const Color::ACESOp* pA
            = dynamic_cast<const Color::ACESOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pA);
        BOOST_CHECK(pA->getInputBitDepth() == SYNCOLOR::BIT_DEPTH_16i);
        BOOST_CHECK(pA->getOutputBitDepth() == SYNCOLOR::BIT_DEPTH_32f);

        BOOST_CHECK(pA->getStyle() == Color::ACESOp::ACES_SURROUND);

        Color::ACESOp::Params params;
        params.push_back(1.2);
        pA->validate();
        BOOST_CHECK(pA->getParams() == params);
        */
    }
}


OIIO_ADD_TEST(FileFormatCTF, Function)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("Function_test1.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile), OCIO::Exception, "No color operator");

        /*
        TODO: Function is from ctf

        const Color::FunctionOp* pA
            = dynamic_cast<const Color::FunctionOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pA);
        BOOST_CHECK(pA->getInputBitDepth() == SYNCOLOR::BIT_DEPTH_16i);
        BOOST_CHECK(pA->getOutputBitDepth() == SYNCOLOR::BIT_DEPTH_32f);

        BOOST_CHECK(pA->getStyle() == Color::FunctionOp::FUNC_XYZ_TO_xyY);
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, IndexMap_1)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("indexMap_test1.clf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 2);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::RangeType);
        const OCIO::OpData::Range * pR 
            = dynamic_cast<const OCIO::OpData::Range*>(opList[0]);
        OIIO_CHECK_ASSERT(pR);

        // Check that the indexMap caused a Range to be inserted.
        OIIO_CHECK_EQUAL(pR->getMinInValue(), 64.5);
        OIIO_CHECK_EQUAL(pR->getMaxInValue(), 940.);
        OIIO_CHECK_EQUAL((int)(pR->getMinOutValue() + 0.5), 132);  // 4*1023/31
        OIIO_CHECK_EQUAL((int)(pR->getMaxOutValue() + 0.5), 1089); // 33*1023/31
        OIIO_CHECK_EQUAL(pR->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_EQUAL(pR->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

        // Check the LUT is ok.
        const OCIO::OpData::Lut1D* pL
            = dynamic_cast<const OCIO::OpData::Lut1D*>(opList[1]);
        OIIO_CHECK_EQUAL(pL->getOpType(), OCIO::OpData::OpData::Lut1DType);
        OIIO_CHECK_EQUAL(pL->getArray().getLength(), 32u);
        OIIO_CHECK_EQUAL(pL->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_EQUAL(pL->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);
    }
}


OIIO_ADD_TEST(FileFormatCTF, IndexMap_2)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("indexMap_test2.clf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        OIIO_CHECK_ASSERT((bool)cachedFile);

        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 2);
        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::RangeType);
        const OCIO::OpData::Range * pR 
            = dynamic_cast<const OCIO::OpData::Range*>(opList[0]);
        OIIO_CHECK_ASSERT(pR);
        OIIO_CHECK_EQUAL(pR->getMinInValue(), -0.1f);
        OIIO_CHECK_EQUAL(pR->getMaxInValue(), 19.f);
        OIIO_CHECK_EQUAL(pR->getMinOutValue(), 0.f);
        OIIO_CHECK_EQUAL(pR->getMaxOutValue(), 1.f);
        OIIO_CHECK_EQUAL(pR->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_EQUAL(pR->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

        // Check the LUT is ok.
        const OCIO::OpData::Lut3D* pL
            = dynamic_cast<const OCIO::OpData::Lut3D*>(opList[1]);
        OIIO_CHECK_EQUAL(pL->getOpType(), OCIO::OpData::OpData::Lut3DType);
        OIIO_CHECK_EQUAL(pL->getArray().getLength(), 2u);
        OIIO_CHECK_EQUAL(pL->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_EQUAL(pL->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    }
}


OIIO_ADD_TEST(FileFormatCTF, IndexMap_3)
{
    const std::string ctfFile("indexMap_test3.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile), OCIO::Exception, 
                          "Only one IndexMap allowed per LUT");
}

OIIO_ADD_TEST(FileFormatCTF, IndexMap_4)
{
    const std::string ctfFile("indexMap_test4.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile), OCIO::Exception, 
                          "IndexMap must have two entries");
}

OIIO_ADD_TEST(FileFormatCTF, CLF_futureVersion)
{
    const std::string ctfFile("info_version_future.clf");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile), OCIO::Exception,
                          "Unsupported transform file version");
}


OIIO_ADD_TEST(FileFormatCTF, CLF_1)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("multiple_ops.clf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(opList.size(), 6);
        // First one is a CDL
        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::CDLType);
        const OCIO::OpData::CDL * cdlOpData 
            = dynamic_cast<const OCIO::OpData::CDL *>(opList[0]);
        OIIO_CHECK_ASSERT(cdlOpData);
        OIIO_CHECK_EQUAL(cdlOpData->getName(), "");
        OIIO_CHECK_EQUAL(cdlOpData->getId(), "cc01234");
        OIIO_CHECK_EQUAL(cdlOpData->getInputBitDepth(), OCIO::BIT_DEPTH_F16);
        OIIO_CHECK_EQUAL(cdlOpData->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_EQUAL(cdlOpData->getDescriptions().getList().size(), 1);
        OIIO_CHECK_EQUAL(cdlOpData->getDescriptions().getList()[0],
            "scene 1 exterior look");
        OIIO_CHECK_EQUAL(cdlOpData->getCDLStyle(), OCIO::OpData::CDL::CDL_V1_2_REV);
        OIIO_CHECK_ASSERT(cdlOpData->getSlopeParams()
            == OCIO::OpData::CDL::ChannelParams(1., 1., 0.8));
        OIIO_CHECK_ASSERT(cdlOpData->getOffsetParams()
            == OCIO::OpData::CDL::ChannelParams(-0.02, 0., 0.15));
        OIIO_CHECK_ASSERT(cdlOpData->getPowerParams()
            == OCIO::OpData::CDL::ChannelParams(1.05, 1.15, 1.4));
        OIIO_CHECK_EQUAL(cdlOpData->getSaturation(), 0.75);

        // Next one in file is a lut1d, but it has an index map,
        // thus a range was inserted before the LUT.
        OIIO_CHECK_EQUAL(opList[1]->getOpType(), OCIO::OpData::OpData::RangeType);
        const OCIO::OpData::Range * rangeOpData 
            = dynamic_cast<const OCIO::OpData::Range *>(opList[1]);
        OIIO_CHECK_ASSERT(rangeOpData);
        OIIO_CHECK_EQUAL(rangeOpData->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_EQUAL(rangeOpData->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_EQUAL(rangeOpData->getMinInValue(), 64.5);
        OIIO_CHECK_EQUAL(rangeOpData->getMaxInValue(), 940.);
        OIIO_CHECK_EQUAL((int)(rangeOpData->getMinOutValue() + 0.5), 132);  // 4*1023/31
        OIIO_CHECK_EQUAL((int)(rangeOpData->getMaxOutValue() + 0.5), 957); // 29*1023/31

        // Lut1D
        OIIO_CHECK_EQUAL(opList[2]->getOpType(), OCIO::OpData::OpData::Lut1DType);
        const OCIO::OpData::Lut1D * l1OpData 
            = dynamic_cast<const OCIO::OpData::Lut1D *>(opList[2]);
        OIIO_CHECK_ASSERT(l1OpData);
        OIIO_CHECK_EQUAL(l1OpData->getName(), "");
        OIIO_CHECK_EQUAL(l1OpData->getId(), "");
        OIIO_CHECK_EQUAL(l1OpData->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_EQUAL(l1OpData->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);
        OIIO_CHECK_EQUAL(l1OpData->getDescriptions().getList().size(), 0);
        OIIO_CHECK_EQUAL(l1OpData->getArray().getLength(), 32u);

        // Check that the noClamp style Range became a Matrix.
        OIIO_CHECK_EQUAL(opList[3]->getOpType(), OCIO::OpData::OpData::MatrixType);
        const OCIO::OpData::Matrix * matOpData
            = dynamic_cast<const OCIO::OpData::Matrix *>(opList[3]);
        OIIO_CHECK_ASSERT(matOpData);
        OIIO_CHECK_EQUAL(matOpData->getInputBitDepth(), OCIO::BIT_DEPTH_UINT12);
        OIIO_CHECK_EQUAL(matOpData->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

        const OCIO::OpData::ArrayDouble& array = matOpData->getArray();
        OIIO_CHECK_EQUAL(array.getLength(), 4u);
        OIIO_CHECK_EQUAL(array.getNumColorComponents(), 4u);
        OIIO_CHECK_EQUAL(array.getNumValues(),
            array.getLength()*array.getLength());

        const float scalef = (900.f - 20.f) / (3760.f - 256.f);
        const float offsetf = 20.f - scalef * 256.f;
        const float prec = 10000.f;
        const int scale = (int)(prec * scalef);
        const int offset = (int)(prec * offsetf);

        OIIO_CHECK_EQUAL(array.getValues().size(), array.getNumValues());
        OIIO_CHECK_EQUAL((int)(prec * array.getValues()[0]), scale);
        OIIO_CHECK_EQUAL(array.getValues()[1], 0.);
        OIIO_CHECK_EQUAL(array.getValues()[2], 0.);
        OIIO_CHECK_EQUAL(array.getValues()[3], 0.);

        OIIO_CHECK_EQUAL(array.getValues()[4], 0.);
        OIIO_CHECK_EQUAL((int)(prec * array.getValues()[5]), scale);
        OIIO_CHECK_EQUAL(array.getValues()[6], 0.);
        OIIO_CHECK_EQUAL(array.getValues()[7], 0.);

        OIIO_CHECK_EQUAL(array.getValues()[8], 0.);
        OIIO_CHECK_EQUAL(array.getValues()[9], 0.);
        OIIO_CHECK_EQUAL((int)(prec * array.getValues()[10]), scale);
        OIIO_CHECK_EQUAL(array.getValues()[11], 0.);

        OIIO_CHECK_EQUAL(array.getValues()[12], 0.);
        OIIO_CHECK_EQUAL(array.getValues()[13], 0.);
        OIIO_CHECK_EQUAL(array.getValues()[14], 0.);
        OIIO_CHECK_EQUAL((int)(prec * array.getValues()[15]),
            (int)(prec * 1023. / 4095.));

        const OCIO::OpData::Matrix::Offsets& offsets = matOpData->getOffsets();
        OIIO_CHECK_EQUAL((int)(prec * offsets[0]), offset);
        OIIO_CHECK_EQUAL((int)(prec * offsets[1]), offset);
        OIIO_CHECK_EQUAL((int)(prec * offsets[2]), offset);
        OIIO_CHECK_EQUAL(offsets[3], 0.f);

        // A range with Clamp
        OIIO_CHECK_EQUAL(opList[4]->getOpType(), OCIO::OpData::OpData::RangeType);
        rangeOpData = dynamic_cast<const OCIO::OpData::Range *>(opList[4]);
        OIIO_CHECK_ASSERT(rangeOpData);
        OIIO_CHECK_EQUAL(rangeOpData->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_EQUAL(rangeOpData->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

        // A range without style defaults to clamp
        OIIO_CHECK_EQUAL(opList[5]->getOpType(), OCIO::OpData::OpData::RangeType);
        rangeOpData = dynamic_cast<const OCIO::OpData::Range *>(opList[5]);
        OIIO_CHECK_ASSERT(rangeOpData);
        OIIO_CHECK_EQUAL(rangeOpData->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_EQUAL(rangeOpData->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    }
}

OIIO_ADD_TEST(FileFormatCTF, maya_31788_tabluation_issue)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        // This clf file contains tabulations used as delimiters for a series of numbers.
        const std::string ctfFile("maya_31788_tabluation.clf");
        OIIO_CHECK_NO_THROW(cachedFile = GetFile(ctfFile));
        const OCIO::OpData::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getId(), "none");
        OIIO_CHECK_EQUAL(opList.size(), 1);

        OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::Lut3DType);

        const OCIO::OpData::Lut3D * pL = static_cast<const OCIO::OpData::Lut3D*>(opList[0]);
        OIIO_CHECK_ASSERT(pL);

        OIIO_CHECK_EQUAL(pL->getInputBitDepth(), OCIO::BIT_DEPTH_UINT12);
        OIIO_CHECK_EQUAL(pL->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);

        const OCIO::OpData::Array& array = pL->getArray();
        OIIO_CHECK_EQUAL(array.getLength(), 33U);
        OIIO_CHECK_EQUAL(array.getNumColorComponents(), 3U);
        OIIO_CHECK_EQUAL(array.getNumValues(), 107811U);
        OIIO_CHECK_EQUAL(array.getValues().size(), 107811U);

        OIIO_CHECK_EQUAL(array.getValues()[0], 0.0f);
        OIIO_CHECK_EQUAL(array.getValues()[1], 0.0f);
        OIIO_CHECK_EQUAL(array.getValues()[2], 0.0f);

        OIIO_CHECK_EQUAL(array.getValues()[3], 0.0f);
        OIIO_CHECK_EQUAL(array.getValues()[4], 0.0f);
        OIIO_CHECK_EQUAL(array.getValues()[5], 13.0f);

        OIIO_CHECK_EQUAL(array.getValues()[6], 1.0f);
        OIIO_CHECK_EQUAL(array.getValues()[7], 0.0f);
        OIIO_CHECK_EQUAL(array.getValues()[8], 44.0f);

        OIIO_CHECK_EQUAL(array.getValues()[9], 0.0f);
        OIIO_CHECK_EQUAL(array.getValues()[10], 1.0f);
        OIIO_CHECK_EQUAL(array.getValues()[11], 94.0f);

        OIIO_CHECK_EQUAL(array.getValues()[3 * 33 + 0], 1.0f);
        OIIO_CHECK_EQUAL(array.getValues()[3 * 33 + 1], 32.0f);
        OIIO_CHECK_EQUAL(array.getValues()[3 * 33 + 2], 0.0f);

        OIIO_CHECK_EQUAL(array.getValues()[3 * 35936 + 0], 4095.0f);
        OIIO_CHECK_EQUAL(array.getValues()[3 * 35936 + 1], 4095.0f);
        OIIO_CHECK_EQUAL(array.getValues()[3 * 35936 + 2], 4095.0f);
    }
}

OIIO_ADD_TEST(FileFormatCTF, GamutMap)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("gamutMap_test1.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile), OCIO::Exception, "No color operator");
        /* TODO: need CTF GamutMap support
        const Color::GamutMapOp* pG
            = dynamic_cast<const Color::GamutMapOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pG);
        BOOST_CHECK(pG->getInputBitDepth() == SYNCOLOR::BIT_DEPTH_16f);
        BOOST_CHECK(pG->getOutputBitDepth() == SYNCOLOR::BIT_DEPTH_12i);

        BOOST_CHECK(pG->getStyle() == Color::GamutMapOp::GAMUT_MAP_DW3);
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, HueVector)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("gamutMap_test1.ctf");
        OIIO_CHECK_THROW_WHAT(cachedFile = GetFile(ctfFile), OCIO::Exception, "No color operator");
        /* TODO: need CTF HueVector support
        const Color::HueVectorOp* pH
            = dynamic_cast<const Color::HueVectorOp*>(pTransform->getOps().get(0));
        BOOST_REQUIRE(pH);
        BOOST_CHECK(pH->getInputBitDepth() == SYNCOLOR::BIT_DEPTH_12i);
        BOOST_CHECK(pH->getOutputBitDepth() == SYNCOLOR::BIT_DEPTH_16f);

        BOOST_CHECK(pH->getStyle() == Color::HueVectorOp::YCH_FWD);
        BOOST_CHECK(pH->getParamStyle() == Color::HueVectorOp::HV_CHROMA);

        Color::HueVectorOp::Params params;
        params.push_back(350.);
        params.push_back(140.);
        params.push_back(0.83);

        BOOST_CHECK(pH->getParams() == params);
        */
    }
}

OIIO_ADD_TEST(FileFormatCTF, LUT3D_file_with_xml_extension)
{
    const std::string ctfFile("ABNorm_CxxxLog10toRec709_Full.xml");
    OIIO_CHECK_THROW_WHAT(GetFile(ctfFile), OCIO::Exception, "Invalid transform");
}


OIIO_ADD_TEST(FileFormatCTF, Info_element_version_test)
{
    // VALID - No Version
    //
    {
        const std::string ctfFile("info_version_without.ctf");
        OIIO_CHECK_NO_THROW(GetFile(ctfFile));
    }

    // VALID - Minor Version
    //
    {
        const std::string ctfFile("info_version_valid_minor.ctf");
        OIIO_CHECK_NO_THROW(GetFile(ctfFile));
    }

    // INVALID - Invalid Version
    //
    {
        const std::string ctfFile("info_version_invalid.ctf");
        OIIO_CHECK_THROW_WHAT(
            GetFile(ctfFile), OCIO::Exception, "Invalid Info element version attribute");
    }

    // INVALID - Unsupported Version
    //
    {
        const std::string ctfFile("info_version_unsupported.ctf");
        OIIO_CHECK_THROW_WHAT(
            GetFile(ctfFile), OCIO::Exception, "Unsupported Info element version attribute");
    }

    // INVALID - Empty Version
    //
    {
        const std::string ctfFile("info_version_empty.ctf");
        OIIO_CHECK_THROW_WHAT(
            GetFile(ctfFile), OCIO::Exception, "Invalid Info element version attribute");
    }
    
}



// TODO: Bring over tests from early 2018 (line 4123 onwards).



#endif // OCIO_UNIT_TEST

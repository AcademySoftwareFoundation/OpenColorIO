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

#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "expat/expat.h"
#include "fileformats/ctf/CTFTransform.h"
#include "fileformats/ctf/CTFReaderHelper.h"
#include "fileformats/ctf/CTFReaderUtils.h"
#include "fileformats/xmlutils/XMLReaderHelper.h"
#include "fileformats/xmlutils/XMLReaderUtils.h"
#include "OpBuilders.h"
#include "ops/NoOp/NoOps.h"
#include "Platform.h"
#include "pystring/pystring.h"
#include "transforms/FileTransform.h"


/*

This file format reader supports the Academy/ASC Common LUT Format (CLF) and
the Autodesk Color Transform Format (CTF).

The Academy/ASC Common LUT format was an initiative to bring vendors together
to agree on a common LUT format for this industry.  Support for CLF is a
requirement in order to obtain ACES Logo Certification from the Academy (in
several product categories).  CLF files are expressed using XML.  The spec,
AMPAS S-2014-006, is available from:
<https://acescentral.com/t/aces-documentation/53>

The Autodesk CTF format is based on the Academy/ASC CLF format and adds several
operators that allow higher quality results by avoiding the need to bake
certain common functions into LUTs.  This ranges from simple power functions
to more complicated operators needed to implement very accurate yet compact
ACES Output Transforms.

Autodesk CTF was also designed to be able to losslessly serialize any OCIO
Processor to a self-contained XML file.  This opens up some useful workflow
options for sharing specific color transformations.  As a result, all native
OCIO ops have a lossless mapping into CTF as XML process nodes.  (This is
sometimes also useful for trouble-shooting.)

The CTF format is a superset of the CLF format, hence the use of a common
parser.  Aside from the file extension, the two formats may be distinguished
based on the version attribute in the root ProcessList element.  A CLF file
uses the attribute "compCLFversion" whereas a CTF file uses "version".

The parser has been carefully designed to assist users in trouble-shooting
problems with files that won't load.  A detailed error message is printed,
along with the line number (similar to a compiler).  There are also extensive
unit tests to ensure robustness.

Note:  One frequent point of confusion regarding the CLF syntax relates to the
inBitDepth and outBitDepth attributes in each process node.  These bit-depths
DO NOT specify the processing precision, nor do they specify the bit-depth of
the images that are input or output from the transform.  The only function of
these bit-depth attributes is to interpret the scaling of the parameter values
in a given process node.  This is helpful since, e.g., it avoids the need for
heuristics to guess whether LUT values are scaled to 10 or 12 bits.  These
attributes must always be present and must match at the interface between
adjacent process nodes.  That said, in some cases, one or both may not actually
affect the results if they are not required to interpret the scaling of the
parameters.  For example, the ASC_CDL parameters are always stored in
normalized form and hence the bit-depths, while required, do not affect their
interpretation.  On the other hand, the interpretation of the parameters in
a Matrix op is affected by both the in and out bit-depths.  It should be noted
that although the bit-depths imply a certain scaling, they never impose a
clamping or quantization, e.g. a LUT array with an outBitDepth of '10i' is free
to contain values outside of [0,1023] and to use fractional values.

*/

// TODO: CTF write support will be done with an upcoming PR.

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
            
    CTFReaderTransformPtr m_transform;
    std::string m_filePath;

};
        
typedef OCIO_SHARED_PTR<LocalCachedFile> LocalCachedFileRcPtr;
        
class LocalFileFormat : public FileFormat
{
public:
            
    ~LocalFileFormat() {}
            
    void GetFormatInfo(FormatInfoVec & formatInfoVec) const override;
            
    CachedFileRcPtr Read(std::istream & istream,
                         const std::string & fileName) const override;
            
    void BuildFileOps(OpRcPtrVec & ops,
                      const Config & config,
                      const ConstContextRcPtr & context,
                      CachedFileRcPtr untypedCachedFile,
                      const FileTransform & fileTransform,
                      TransformDirection dir) const override;
};
        
void LocalFileFormat::GetFormatInfo(FormatInfoVec & formatInfoVec) const
{
    FormatInfo info;
    info.name = "Academy/ASC Common LUT Format";
    info.extension = "clf";
    info.capabilities = FORMAT_CAPABILITY_READ;
    formatInfoVec.push_back(info);

    FormatInfo info2;
    info2.name = "Color Transform Format";
    info2.extension = "ctf";
    info2.capabilities = FORMAT_CAPABILITY_READ;
    formatInfoVec.push_back(info2);
}

class XMLParserHelper
{
public:
    XMLParserHelper() = delete;
    XMLParserHelper(const XMLParserHelper &) = delete;

    XMLParserHelper(const std::string & fileName)
        : m_parser(XML_ParserCreate(nullptr))
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
            // Add back the newline character. Parsing will copy the line in a
            // buffer up to the length of the line without the null termination.
            // Our code will be called back to parse the buffer into a number.
            // Code uses strtod. The buffer has to be delimited so that strtod
            // does not access it after its length.
            line.push_back('\n');
            ++m_lineNumber;

            Parse(line, !istream.good());
        }

        if (!m_elms.empty())
        {
            std::string error("CTF/CLF parsing error (no closing tag for '");
            error += m_elms.back()->getName().c_str();
            error += "). ";
            throwMessage(error);
        }

        const CTFReaderTransformPtr& pT = getTransform();
        if (pT.use_count() == 0)
        {
            static const std::string error(
                "CTF/CLF parsing error: Invalid transform. ");
            throwMessage(error);
        }

        if (pT->getOps().empty())
        {
            static const std::string error(
                "CTF/CLF parsing error: No color operator in file. ");
            throwMessage(error);
        }
    }

    void Parse(const std::string & buffer, bool lastLine)
    {
        const int done = lastLine?1:0;

        if (XML_STATUS_ERROR == XML_Parse(m_parser,
                                          buffer.c_str(),
                                          (int)buffer.size(), done))
        {
            XML_Error eXpatErrorCode = XML_GetErrorCode(m_parser);
            if (eXpatErrorCode == XML_ERROR_TAG_MISMATCH)
            {
                if (!m_elms.empty())
                {
                    // It could be an Op or an Attribute.
                    std::string error(
                        "CTF/CLF parsing error (no closing tag for '");
                    error += m_elms.back()->getName().c_str();
                    error += "'). ";
                    throwMessage(error);
                }
                else
                {
                    // Completely lost, something went wrong,
                    // but nothing detected with the stack.
                    static const std::string error(
                        "CTF/CLF parsing error (unbalanced element tags). ");
                    throwMessage(error);
                }
            }
            else
            {
                std::string error("CTF/CLF parsing error: ");
                error += XML_ErrorString(XML_GetErrorCode(m_parser));
                throwMessage(error);
            }
        }

    }

    CTFReaderTransformPtr getTransform()
    {
        return m_transform;
    }

private:

    void AddOpReader(CTFReaderOpElt::Type type, const char * xmlTag)
    {
        if (m_elms.size() != 1)
        {
            std::stringstream ss;
            ss << ": The " << xmlTag;
            ss << "'s parent can only be a Transform";

            m_elms.push_back(std::make_shared<XmlReaderDummyElt>(
                xmlTag,
                (m_elms.empty() ? 0 : m_elms.back()),
                getXmLineNumber(),
                getXmlFilename(),
                ss.str().c_str()));
        }
        else
        {
            ElementRcPtr pElt = m_elms.back();

            auto pT = std::dynamic_pointer_cast<CTFReaderTransformElt>(pElt);
            CTFReaderOpEltRcPtr pOp =
                CTFReaderOpElt::GetReader(type, pT->getVersion());

            if (!pOp)
            {
                std::stringstream ss;
                ss << "Unsupported transform file version '";
                ss << pT->getVersion() << "' for operator '" << xmlTag;
                throwMessage(ss.str());
            }

            pOp->setContext(xmlTag, m_transform,
                            getXmLineNumber(), getXmlFilename());

            m_elms.push_back(pOp);
        }
    }

    void throwMessage(const std::string & error) const
    {
        std::ostringstream os;
        os << "Error parsing CTF/CLF file (";
        os << m_fileName.c_str() << "). ";
        os << "Error is: " << error.c_str();
        os << ". At line (" << m_lineNumber << ")";
        throw Exception(os.str().c_str());
    }

    // Determines if the element name is supported in the current context.
    static bool SupportedElement(const char * name,
                                 ElementRcPtr & parent,
                                 const char * tag,
                                 const char * parentName,
                                 bool & recognizedName)
    {
        if (name && *name && tag && *tag)
        {
            if (0 == Platform::Strcasecmp(name, tag))
            {
                recognizedName |= true;

                if (!parentName || !strlen(parentName) ||
                    (parent &&
                     0 == Platform::Strcasecmp(parent->getName().c_str(),
                                               parentName)))
                {
                    return true;
                }
            }
        }

        return false;
    }

    static bool SupportedElement(const char * name,
                                 ElementRcPtr & parent,
                                 const std::vector<const char *> & tags,
                                 const char * parentName,
                                 bool & recognizedName)
    {
        if (name && *name)
        {
            const size_t numTags(tags.size());
            size_t i = 0;
            for (; i<numTags; ++i)
            {
                if (0 == Platform::Strcasecmp(name, tags[i]))
                {
                    recognizedName |= true;
                    break;
                }
            }

            // If found name in the tag list, test the parent name.
            if (i < numTags)
            {
                if (!parentName || !strlen(parentName) ||
                    (parent && 0 == Platform::Strcasecmp(parent->getName().c_str(), parentName)))
                {
                    return true;
                }
            }
        }

        return false;
    }

    // Start the parsing of one element.
    static void StartElementHandler(void * userData,
                                    const XML_Char * name,
                                    const XML_Char ** atts)
    {
        static const std::vector<const char *> rangeSubElements = {
            TAG_MIN_IN_VALUE,
            TAG_MAX_IN_VALUE,
            TAG_MIN_OUT_VALUE,
            TAG_MAX_OUT_VALUE
        };

        static const std::vector<const char *> sopSubElements = {
            TAG_SLOPE,
            TAG_OFFSET,
            TAG_POWER
        };

        XMLParserHelper * pImpl = (XMLParserHelper*)userData;

        if (!pImpl || !name || !*name)
        {
            if (!pImpl)
            {
                throw Exception("Internal CTF/CLF parser error.");
            }
            else
            {
                pImpl->throwMessage("Internal CTF/CLF parser error. ");
            }
        }

        if (!pImpl->m_elms.empty())
        {
            // Check if we are still processing a metadata structure.
            ElementRcPtr pElt = pImpl->m_elms.back();

            auto pMD = std::dynamic_pointer_cast<CTFReaderMetadataElt>(pElt);

            if (pMD)
            {
                pImpl->m_elms.push_back(
                    std::make_shared<CTFReaderMetadataElt>(
                        name,
                        pMD,
                        pImpl->m_lineNumber,
                        pImpl->m_fileName));

                pImpl->m_elms.back()->start(atts);
                return;
            }
        }

        // Handle the ProcessList element or its children (the ops).
        if (0 == Platform::Strcasecmp(name, TAG_PROCESS_LIST))
        {
            if (pImpl->m_transform.get())
            {
                ElementRcPtr pElt = pImpl->m_elms.front();
                auto pT = std::dynamic_pointer_cast<CTFReaderTransformElt>(pElt);

                pImpl->m_elms.push_back(
                    std::make_shared<XmlReaderDummyElt>(
                        name, pT,
                        pImpl->getXmLineNumber(),
                        pImpl->getXmlFilename(),
                        ": The Transform already exists"));
            }
            else
            {
                CTFReaderTransformEltRcPtr pT
                    = std::make_shared<CTFReaderTransformElt>(
                        name,
                        pImpl->getXmLineNumber(),
                        pImpl->getXmlFilename(),
                        pImpl->IsCLF());

                pImpl->m_elms.push_back(pT);
                pImpl->m_transform = pT->getTransform();
            }
        }

        // Handle all Ops.
        else
        {
            ElementRcPtr pElt;
            if (pImpl->m_elms.size())
            {
                pElt = pImpl->m_elms.back();
            }

            // Safety check to try and ensure that all new elements will get handled here.
            static_assert(CTFReaderOpElt::NoType == 13, "Need to handle new type here");

            // Will allow to give better error feedback to the user if the
            // element name is not handled. If any case recognizes the name,
            // but the element is not in the correct context (under the correct
            // parent), then the recognizedName boolean will be true.
            bool recognizedName = false;

            // For each possible element name, test against a tag name and a
            // current parent name to determine if the element should be handled.
            if (SupportedElement(name, pElt, TAG_ACES,
                                      TAG_PROCESS_LIST, recognizedName))
            {
                pImpl->AddOpReader(CTFReaderOpElt::ACESType, name);
            }
            else if (SupportedElement(name, pElt, TAG_CDL,
                                      TAG_PROCESS_LIST, recognizedName))
            {
                pImpl->AddOpReader(CTFReaderOpElt::CDLType, name);
            }
            else if (SupportedElement(name, pElt, TAG_EXPOSURE_CONTRAST,
                                      TAG_PROCESS_LIST, recognizedName))
            {
                pImpl->AddOpReader(CTFReaderOpElt::ExposureContrastType, name);
            }
            else if (SupportedElement(name, pElt, TAG_FIXED_FUNCTION,
                                      TAG_PROCESS_LIST, recognizedName))
            {
                pImpl->AddOpReader(CTFReaderOpElt::FixedFunctionType, name);
            }
            else if (SupportedElement(name, pElt, TAG_GAMMA,
                                      TAG_PROCESS_LIST, recognizedName))
            {
                pImpl->AddOpReader(CTFReaderOpElt::GammaType, name);
            }
            else if (SupportedElement(name, pElt, TAG_INVLUT1D,
                                      TAG_PROCESS_LIST, recognizedName))
            {
                pImpl->AddOpReader(CTFReaderOpElt::InvLut1DType, name);
            }
            else if (SupportedElement(name, pElt, TAG_INVLUT3D,
                                      TAG_PROCESS_LIST, recognizedName))
            {
                pImpl->AddOpReader(CTFReaderOpElt::InvLut3DType, name);
            }
            else if (SupportedElement(name, pElt, TAG_LOG,
                                      TAG_PROCESS_LIST, recognizedName))
            {
                pImpl->AddOpReader(CTFReaderOpElt::LogType, name);
            }
            else if (SupportedElement(name, pElt, TAG_LUT1D,
                                      TAG_PROCESS_LIST, recognizedName))
            {
                pImpl->AddOpReader(CTFReaderOpElt::Lut1DType, name);
            }
            else if (SupportedElement(name, pElt, TAG_LUT3D,
                                      TAG_PROCESS_LIST, recognizedName))
            {
                pImpl->AddOpReader(CTFReaderOpElt::Lut3DType, name);
            }
            else if (SupportedElement(name, pElt, TAG_MATRIX,
                                      TAG_PROCESS_LIST, recognizedName))
            {
                pImpl->AddOpReader(CTFReaderOpElt::MatrixType, name);
            }
            else if (SupportedElement(name, pElt, TAG_RANGE,
                                      TAG_PROCESS_LIST, recognizedName))
            {
                pImpl->AddOpReader(CTFReaderOpElt::RangeType, name);
            }
            else if (SupportedElement(name, pElt, TAG_REFERENCE,
                                      TAG_PROCESS_LIST, recognizedName))
            {
                pImpl->AddOpReader(CTFReaderOpElt::ReferenceType, name);
            }
            // TODO: handle other ops from syncolor.

            // Handle other elements that are transform-level metadata or parts of ops.
            else
            {
                auto pT = std::dynamic_pointer_cast<CTFReaderTransformElt>(pElt);

                auto pContainer =
                    std::dynamic_pointer_cast<XmlReaderContainerElt>(pElt);
                if (!pContainer)
                {
                    pImpl->m_elms.push_back(
                        std::make_shared<XmlReaderDummyElt>(
                            name,
                            pElt,
                            pImpl->getXmLineNumber(),
                            pImpl->getXmlFilename(),
                            nullptr));
                }
                else if (SupportedElement(name, pElt, TAG_ACES_PARAMS,
                                          TAG_ACES, recognizedName))
                {
                    pImpl->m_elms.push_back(
                        std::make_shared<CTFReaderACESParamsElt>(
                            name,
                            pContainer,
                            pImpl->getXmLineNumber(),
                            pImpl->getXmlFilename()));
                }
                else if (SupportedElement(name, pElt, TAG_ARRAY, TAG_LUT1D, recognizedName) ||
                         SupportedElement(name, pElt, TAG_ARRAY, TAG_INVLUT1D, recognizedName) ||
                         SupportedElement(name, pElt, TAG_ARRAY, TAG_LUT3D, recognizedName) ||
                         SupportedElement(name, pElt, TAG_ARRAY, TAG_INVLUT3D, recognizedName) ||
                         SupportedElement(name, pElt, TAG_ARRAY, TAG_MATRIX, recognizedName))
                {
                    auto pA = std::dynamic_pointer_cast<CTFArrayMgt>(pContainer);
                    if (!pA || pA->isCompleted())
                    {
                        if (!pA)
                        {
                            pImpl->m_elms.push_back(
                                std::make_shared<XmlReaderDummyElt>(
                                    name,
                                    (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename(),
                                    ": Array not allowed in this element"));
                        }
                        else
                        {
                            pImpl->m_elms.push_back(
                                std::make_shared<XmlReaderDummyElt>(
                                    name,
                                    (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename(),
                                    ": Only one Array allowed per op"));
                        }
                    }
                    else
                    {
                        pImpl->m_elms.push_back(
                            std::make_shared<CTFReaderArrayElt>(
                                name, pContainer,
                                pImpl->getXmLineNumber(),
                                pImpl->getXmlFilename()));
                    }
                }
                else if (SupportedElement(name, pElt, TAG_DESCRIPTION,
                                          "", recognizedName))
                {
                    pImpl->m_elms.push_back(
                        std::make_shared<XmlReaderDescriptionElt>(
                            name,
                            pContainer,
                            pImpl->getXmLineNumber(),
                            pImpl->getXmlFilename()));
                }
                // Dynamic Property is valid under any operator parent. First
                // test if the tag is supported to set the recognizedName 
                // accordingly, without testing for parents. Test for the
                // parent type prior to testing the name.
                else if (SupportedElement(name, pElt, TAG_DYNAMIC_PARAMETER,
                                          "", recognizedName) &&
                         std::dynamic_pointer_cast<CTFReaderOpElt>(pContainer))
                {
                    pImpl->m_elms.push_back(
                        std::make_shared<CTFReaderDynamicParamElt>(
                            name,
                            pContainer,
                            pImpl->getXmLineNumber(),
                            pImpl->getXmlFilename()));
                }
                else if (SupportedElement(name, pElt, TAG_EC_PARAMS,
                                          TAG_EXPOSURE_CONTRAST, recognizedName))
                {
                    pImpl->m_elms.push_back(
                        std::make_shared<CTFReaderECParamsElt>(
                            name,
                            pContainer,
                            pImpl->getXmLineNumber(),
                            pImpl->getXmlFilename()));
                }
                else if (SupportedElement(name, pElt, TAG_GAMMA_PARAMS,
                                          TAG_GAMMA, recognizedName))
                {
                    CTFReaderGammaElt * pGamma = dynamic_cast<CTFReaderGammaElt*>(pContainer.get());
                    pImpl->m_elms.push_back(
                        pGamma->createGammaParamsElt(
                            name,
                            pContainer,
                            pImpl->getXmLineNumber(),
                            pImpl->getXmlFilename()));
                }
                else if (SupportedElement(name, pElt, TAG_INDEX_MAP, TAG_LUT1D, recognizedName) ||
                         SupportedElement(name, pElt, TAG_INDEX_MAP, TAG_LUT3D, recognizedName))
                {
                    auto pA = std::dynamic_pointer_cast<CTFIndexMapMgt>(pContainer);
                    if (!pA || pA->isCompletedIM())
                    {
                        if (!pA)
                        {
                            pImpl->m_elms.push_back(
                                std::make_shared<XmlReaderDummyElt>(
                                    name,
                                    (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                    pImpl->getXmLineNumber(),
                                    pImpl->getXmlFilename(),
                                    ": IndexMap not allowed in this element"));
                        }
                        else
                        {
                            // Currently only support a single IndexMap per LUT.
                            pImpl->throwMessage("Only one IndexMap allowed per LUT. ");
                        }
                    }
                    else
                    {
                        pImpl->m_elms.push_back(
                            std::make_shared<CTFReaderIndexMapElt>(
                                name, pContainer,
                                pImpl->getXmLineNumber(),
                                pImpl->getXmlFilename()));
                    }

                }
                else if (SupportedElement(name, pElt, TAG_INFO, 
                                          TAG_PROCESS_LIST, recognizedName))
                {
                    pImpl->m_elms.push_back(
                        std::make_shared<CTFReaderInfoElt>(
                            name,
                            pContainer,
                            pImpl->getXmLineNumber(),
                            pImpl->getXmlFilename()));
                }
                else if (SupportedElement(name, pElt, TAG_INPUT_DESCRIPTOR,
                                          TAG_PROCESS_LIST, recognizedName))
                {
                    pImpl->m_elms.push_back(
                        std::make_shared<CTFReaderInputDescriptorElt>(
                            name,
                            pContainer,
                            pImpl->getXmLineNumber(),
                            pImpl->getXmlFilename()));
                }
                else if (SupportedElement(name, pElt, TAG_LOG_PARAMS,
                                          TAG_LOG, recognizedName))
                {
                    auto pLog = std::dynamic_pointer_cast<CTFReaderLogElt>(pContainer);
                    const auto style = pLog->getCTFParams().m_style;
                    if (!(style == LogUtil::LOG_TO_LIN ||
                          style == LogUtil::LIN_TO_LOG))
                    {
                        pImpl->m_elms.push_back(
                            std::make_shared<XmlReaderDummyElt>(
                                name,
                                (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                pImpl->getXmLineNumber(),
                                pImpl->getXmlFilename(),
                                ": Log Params not allowed in this element"));
                    }
                    else
                    {
                        pImpl->m_elms.push_back(
                            std::make_shared<CTFReaderLogParamsElt>(name,
                                pContainer,
                                pImpl->getXmLineNumber(),
                                pImpl->getXmlFilename()));
                    }
                }
                else if (SupportedElement(name, pElt, TAG_OUTPUT_DESCRIPTOR,
                    TAG_PROCESS_LIST, recognizedName))
                {
                    pImpl->m_elms.push_back(
                        std::make_shared<CTFReaderOutputDescriptorElt>(
                            name,
                            pContainer,
                            pImpl->getXmLineNumber(),
                            pImpl->getXmlFilename()));
                }
                else if (SupportedElement(name, pElt, rangeSubElements,
                                          TAG_RANGE, recognizedName))
                {
                    pImpl->m_elms.push_back(
                        std::make_shared<CTFReaderRangeValueElt>(
                            name,
                            pContainer,
                            pImpl->getXmLineNumber(),
                            pImpl->getXmlFilename()));
                }
                else if (SupportedElement(name, pElt, TAG_SATNODE,
                                          TAG_CDL, recognizedName) ||
                         SupportedElement(name, pElt, TAG_SATNODEALT,
                                          TAG_CDL, recognizedName))
                {
                    auto pCDL =
                        std::dynamic_pointer_cast<CTFReaderCDLElt>(pContainer);

                    auto satNodeElt = std::make_shared<CTFReaderSatNodeElt>(
                        name,
                        pCDL,
                        pImpl->getXmLineNumber(),
                        pImpl->getXmlFilename());
                    pImpl->m_elms.push_back(satNodeElt);
                }
                else if (SupportedElement(name, pElt, TAG_SATURATION,
                                          TAG_SATNODE, recognizedName))
                {
                    pImpl->m_elms.push_back(
                        std::make_shared<XmlReaderSaturationElt>(
                            name,
                            pContainer,
                            pImpl->getXmLineNumber(),
                            pImpl->getXmlFilename()));
                }
                else if (SupportedElement(name, pElt, TAG_SOPNODE,
                                          TAG_CDL, recognizedName))
                {
                    auto pCDL =
                        std::dynamic_pointer_cast<CTFReaderCDLElt>(pContainer);

                    auto sopNodeElt = std::make_shared<CTFReaderSOPNodeElt>(
                        name,
                        pCDL,
                        pImpl->getXmLineNumber(),
                        pImpl->getXmlFilename());
                    pImpl->m_elms.push_back(sopNodeElt);
                }
                else if (SupportedElement(name, pElt, sopSubElements,
                                          TAG_SOPNODE, recognizedName))
                {
                    pImpl->m_elms.push_back(
                        std::make_shared<XmlReaderSOPValueElt>(
                            name,
                            pContainer,
                            pImpl->getXmLineNumber(),
                            pImpl->getXmlFilename()));
                }
                else
                {
                    if (recognizedName)
                    {
                        std::ostringstream oss;
                        oss << ": '" << name << "' not allowed in this element";

                        pImpl->m_elms.push_back(
                            std::make_shared<XmlReaderDummyElt>(
                                name,
                                (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                pImpl->getXmLineNumber(),
                                pImpl->getXmlFilename(),
                                oss.str().c_str()));
                    }
                    else
                    {
                        pImpl->m_elms.push_back(
                            std::make_shared<XmlReaderDummyElt>(
                                name,
                                (pImpl->m_elms.empty() ? 0 : pImpl->m_elms.back()),
                                pImpl->getXmLineNumber(),
                                pImpl->getXmlFilename(),
                                ": Unknown element"));
                    }
                }
            }
        }

        pImpl->m_elms.back()->start(atts);
    }

    // End the parsing of one element.
    static void EndElementHandler(void * userData,
                                  const XML_Char * name)
    {
        XMLParserHelper * pImpl = (XMLParserHelper*)userData;
        if (!pImpl || !name || !*name)
        {
            throw Exception("CTF/CLF internal parsing error.");
        }

        // Is the expected element present?
        auto pElt(pImpl->m_elms.back());
        if (!pElt.get())
        {
            pImpl->throwMessage("CTF/CLF parsing error: Tag is missing. ");
        }

        // Is it the expected element?
        if (pElt->getName() != name)
        {
            std::stringstream ss;
            ss << "CTF/CLF parsing error: Tag '";
            ss << (name ? name : "");
            ss << "' is missing";
            pImpl->throwMessage(ss.str());
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
            // Is it a plain element?
            auto pPlainElt = std::dynamic_pointer_cast<XmlReaderPlainElt>(pElt);
            if (!pPlainElt)
            {
                std::stringstream ss;
                ss << "CTF/CLF parsing error: Attribute end '";
                ss << (name ? name : "");
                ss << "' is illegal. ";
                pImpl->throwMessage(ss.str());
            }

            pImpl->m_elms.pop_back();

            auto pParent = pImpl->m_elms.back();

            // Is it at the right location in the stack?
            if (!pParent || !pParent->isContainer() ||
                pParent != pPlainElt->getParent())
            {
                std::stringstream ss;
                ss << "CTF/CLF parsing error: Tag '";
                ss << (name ? name : "");
                ss << "'.";
                pImpl->throwMessage(ss.str());
            }
        }

        pElt->end();
    }

    // Handle of strings within an element.
    static void CharacterDataHandler(void * userData,
                                     const XML_Char * s, 
                                     int len)
    {
        XMLParserHelper * pImpl = (XMLParserHelper*)userData;
        if (!pImpl)
        {
            throw Exception("CTF/CLF internal parsing error.");
        }

        if (len == 0) return;
        if (len<0 || !s || !*s)
        {
            pImpl->throwMessage("CTF/CLF parsing error: attribute illegal. ");
        }
        // Parsing a single new line. This is valid.
        if (len == 1 && s[0] == '\n') return;

        auto pElt = pImpl->m_elms.back();
        if (!pElt)
        {
            std::ostringstream oss;
            oss << "CTF/CLF parsing error: missing end tag '";
            oss << std::string(s, len).c_str();
            oss << "'.";
            pImpl->throwMessage(oss.str());
        }

        auto pDescriptionElt =
            std::dynamic_pointer_cast<XmlReaderDescriptionElt>(pElt);
        if (pDescriptionElt)
        {
            pDescriptionElt->setRawData(s, len, pImpl->getXmLineNumber());
        }
        else
        {
            // Strip white spaces.
            size_t start = 0;
            size_t end = len;
            FindSubString(s, len, start, end);

            if (end>0)
            {
                // Metadata element is a special element processor: It is used
                // to process container elements, but it is also used to
                // process the terminal/plain elements.
                auto pMetadataElt =
                    std::dynamic_pointer_cast<CTFReaderMetadataElt>(pElt);
                if (pMetadataElt)
                {
                    pMetadataElt->setRawData(s + start, end - start,
                                             pImpl->getXmLineNumber());
                }
                else
                {
                    if (pElt->isContainer())
                    {
                        std::ostringstream oss;
                        oss << "CTF/CLF parsing error: attribute illegal '";
                        oss << std::string(s, len).c_str();
                        oss << "'.";
                        pImpl->throwMessage(oss.str());
                    }

                    auto pPlainElt = 
                        std::dynamic_pointer_cast<XmlReaderPlainElt>(pElt);
                    if (!pPlainElt)
                    {
                        std::ostringstream oss;
                        oss << "CTF/CLF parsing error: attribute illegal '";
                        oss << std::string(s, len).c_str();
                        oss << "'.";
                        pImpl->throwMessage(oss.str());
                    }
                    pPlainElt->setRawData(s + start, end - start,
                                          pImpl->getXmLineNumber());
                }
            }
        }
    }

    unsigned int getXmLineNumber() const
    {
        return m_lineNumber;
    }

    const std::string & getXmlFilename() const
    {
        return m_fileName;
    }

    bool IsCLF() const
    {
        return m_isCLF;
    }

    XML_Parser m_parser;
    unsigned int m_lineNumber;
    std::string m_fileName;
    bool m_isCLF;
    XmlReaderElementStack m_elms; // Parsing stack
    CTFReaderTransformPtr m_transform;

};

bool isLoadableCTF(std::istream & istream)
{
    std::streampos curPos = istream.tellg();

    const unsigned limit(5 * 1024); // 5 kilobytes.
    const char *pattern = "<ProcessList";
    bool foundPattern = false;
    unsigned sizeProcessed(0);
    char line[limit + 1];

    // Error: file is not found, or cannot read.
    if (istream.good())
    {
        // Find ProcessList tag at beginning of file.
        while (istream.good() && !foundPattern && (sizeProcessed < limit))
        {
            istream.getline(line, limit);
            if (strstr(line, pattern)) foundPattern = true;
            sizeProcessed += (unsigned)strlen(line);
        }
    }

    // Restore pos in stream.
    istream.seekg(curPos);
    return foundPattern;
}

// Try and load the format.
// Raise an exception if it can't be loaded.
CachedFileRcPtr LocalFileFormat::Read(
    std::istream & istream,
    const std::string & filePath) const
{
    if (!isLoadableCTF(istream))
    {
        std::ostringstream oss;
        oss << "Parsing error: '" << filePath << "' is not a CTF/CLF file.";
        throw Exception(oss.str().c_str());
    }

    XMLParserHelper parser(filePath);
    parser.Parse(istream);

    LocalCachedFileRcPtr cachedFile =
        LocalCachedFileRcPtr(new LocalCachedFile());

    // Keep transform.
    cachedFile->m_transform = parser.getTransform();
    cachedFile->m_filePath = filePath;

    return cachedFile;
}

// Helper called by LocalFileFormat::BuildFileOps
void BuildOp(OpRcPtrVec & ops,
             const Config& config,
             const ConstContextRcPtr & context,
             const OpDataRcPtr & opData,
             TransformDirection dir)
{
    if (opData->getType() == OpData::ReferenceType)
    {
        // Recursively resolve the op.
        ReferenceOpDataRcPtr ref = DynamicPtrCast<ReferenceOpData>(opData);
        if (ref->getReferenceStyle() == REF_PATH)
        {
            dir = CombineTransformDirections(dir, ref->getDirection());
            FileTransformRcPtr fileTransform = FileTransform::Create();
            fileTransform->setInterpolation(INTERP_LINEAR);
            fileTransform->setDirection(TRANSFORM_DIR_FORWARD);
            fileTransform->setSrc(ref->getPath().c_str());
            FileTransform * pFileTranform = fileTransform.get();

            size_t sizeBefore = ops.size();
            // This might call LocalFileFormat::BuildFileOps if the file
            // is a CTF. BuildFileTransformOps is making sure there is no
            // cycling recursion.
            BuildFileTransformOps(ops, config, context, *pFileTranform, dir);

            // The original in/out bit-depths of the loaded opvec need
            // to be set to match the depths of the Reference element
            // that they replace.
            size_t sizeAfter = ops.size();
            if (sizeBefore != sizeAfter)
            {
                // Set the input depth of the first op in the loaded opvec
                // to match the Reference.
                while (sizeBefore < sizeAfter)
                {
                    ConstOpRcPtr op = ops[sizeBefore];
                    ConstOpDataRcPtr data = op->data();
                    auto fileData = DynamicPtrCast<const FileNoOpData>(data);
                    // Ignore the FileNoOps that are inserted in order to
                    // properly handle nested References.
                    if (!fileData)
                    {
                        ops[sizeBefore]->setInputBitDepth(
                            ref->getInputBitDepth());
                        break;
                    }
                    ++sizeBefore;
                }
                // Set the output depth of the last op in the loaded opvec
                // to match the Reference.
                while (sizeAfter > sizeBefore)
                {
                    --sizeAfter;
                    ConstOpRcPtr op = ops[sizeAfter];
                    ConstOpDataRcPtr data = op->data();
                    auto fileData = DynamicPtrCast<const FileNoOpData>(data);
                    if (!fileData)
                    {
                        ops[sizeAfter]->setOutputBitDepth(
                            ref->getOutputBitDepth());
                        break;
                    }
                }
            }
        }
    }
    else
    {
        CreateOpVecFromOpData(ops, opData, dir);
    }

}

void
LocalFileFormat::BuildFileOps(OpRcPtrVec & ops,
                              const Config& config,
                              const ConstContextRcPtr & context,
                              CachedFileRcPtr untypedCachedFile,
                              const FileTransform& fileTransform,
                              TransformDirection dir) const
{
    LocalCachedFileRcPtr cachedFile = 
        DynamicPtrCast<LocalCachedFile>(untypedCachedFile);
            
    // This should never happen.
    if(!cachedFile)
    {
        throw Exception("Cannot build clf ops. Invalid cache type.");
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

    // Resolve reference path using context and load referenced files.
    const OpDataVec & opDataVec = cachedFile->m_transform->getOps();
    if (newDir == TRANSFORM_DIR_FORWARD)
    {
        for (auto & opData : opDataVec)
        {
            BuildOp(ops, config, context, opData, newDir);
        }
    }
    else
    {
        for (int idx = (int)opDataVec.size() - 1; idx >= 0; --idx)
        {
            BuildOp(ops, config, context, opDataVec[idx], newDir);
        }
    }
}
    
} // end of anonymous namespace.

FileFormat * CreateFileFormatCLF()
{
    return new LocalFileFormat();
}


}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"
#include "UnitTestUtils.h"

OCIO::LocalCachedFileRcPtr LoadCLFFile(const std::string & fileName)
{
    return OCIO::LoadTestFile<OCIO::LocalFileFormat, OCIO::LocalCachedFile>(
        fileName, std::ios_base::in);
}

OIIO_ADD_TEST(FileFormatCTF, missing_file)
{
    // Test LoadCLFFile helper function with missing file.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("xxxxxxxxxxxxxxxxx.xxxxx");
    OIIO_CHECK_THROW_WHAT(cachedFile = LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Error opening test file.");
}

OIIO_ADD_TEST(FileFormatCTF, wrong_format)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("logtolin_8to8.lut");
        OIIO_CHECK_THROW_WHAT(cachedFile = LoadCLFFile(ctfFile),
                              OCIO::Exception,
                              "not a CTF/CLF file.");
        OIIO_CHECK_ASSERT(!(bool)cachedFile);
    }
}

OIIO_ADD_TEST(FileFormatCTF, clf_spec)
{
    // Parse examples from the specifications document S-2014-006.
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("lut1d_example.clf");
        OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
        OIIO_REQUIRE_ASSERT((bool)cachedFile);
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getName(),
                         "transform example lut1d");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getID(), "exlut1");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions().size(), 1);
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions()[0],
                         " Turn 4 grey levels into 4 inverted codes using a 1D ");
        const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_REQUIRE_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::Lut1DType);
        OIIO_CHECK_EQUAL(opList[0]->getName(), "4valueLut");
        OIIO_CHECK_EQUAL(opList[0]->getID(), "lut-23");
        OIIO_CHECK_EQUAL(opList[0]->getInputBitDepth(), OCIO::BIT_DEPTH_UINT12);
        OIIO_CHECK_EQUAL(opList[0]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);
        OIIO_REQUIRE_EQUAL(opList[0]->getDescriptions().size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getDescriptions()[0], " 1D LUT ");
    }

    {
        const std::string ctfFile("lut3d_identity_12i_16f.clf");
        OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
        OIIO_REQUIRE_ASSERT((bool)cachedFile);
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getName(),
                         "transform example lut3d");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getID(), "exlut2");
        OIIO_REQUIRE_EQUAL(cachedFile->m_transform->getDescriptions().size(), 1);
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions()[0],
                         " 3D LUT example from spec ");
        const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_REQUIRE_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::Lut3DType);
        OIIO_CHECK_EQUAL(opList[0]->getName(), "identity");
        OIIO_CHECK_EQUAL(opList[0]->getID(), "lut-24");
        OIIO_CHECK_EQUAL(opList[0]->getInputBitDepth(), OCIO::BIT_DEPTH_UINT12);
        OIIO_CHECK_EQUAL(opList[0]->getOutputBitDepth(), OCIO::BIT_DEPTH_F16);
        OIIO_REQUIRE_EQUAL(opList[0]->getDescriptions().size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getDescriptions()[0], " 3D LUT ");
    }

    {
        const std::string ctfFile("matrix_3x4_example.clf");
        OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
        OIIO_REQUIRE_ASSERT((bool)cachedFile);
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getName(),
                         "transform example matrix");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getID(), "exmat1");
        OIIO_REQUIRE_EQUAL(cachedFile->m_transform->getDescriptions().size(), 2);
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions()[0],
                         " Matrix example from spec ");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions()[1],
                         " Used by unit tests ");
        const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_REQUIRE_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::MatrixType);
        OIIO_CHECK_EQUAL(opList[0]->getName(), "colorspace conversion");
        OIIO_CHECK_EQUAL(opList[0]->getID(), "mat-25");
        OIIO_CHECK_EQUAL(opList[0]->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_EQUAL(opList[0]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OIIO_REQUIRE_EQUAL(opList[0]->getDescriptions().size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getDescriptions()[0],
                         " 3x4 Matrix , 4th column is offset ");
    }

    {
        // Test two-entries IndexMap support.
        const std::string ctfFile("lut1d_indexmap_example.clf");
        OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
        OIIO_REQUIRE_ASSERT((bool)cachedFile);
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getName(),
                         "transform example lut IndexMap");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getID(), "exlut3");
        OIIO_REQUIRE_EQUAL(cachedFile->m_transform->getDescriptions().size(), 1);
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions()[0],
                         " IndexMap LUT example from spec ");
        const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_REQUIRE_EQUAL(opList.size(), 2);
        OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::RangeType);
        auto pR = std::dynamic_pointer_cast<const OCIO::RangeOpData>(opList[0]);
        OIIO_REQUIRE_ASSERT(pR);

        OIIO_CHECK_EQUAL(pR->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_EQUAL(pR->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

        OIIO_CHECK_EQUAL(pR->getMinInValue(), 64.);
        OIIO_CHECK_EQUAL(pR->getMaxInValue(), 940.);
        OIIO_CHECK_EQUAL(pR->getMinOutValue(), 0.);
        OIIO_CHECK_EQUAL(pR->getMaxOutValue(), 1023.);

        OIIO_CHECK_EQUAL(opList[1]->getType(), OCIO::OpData::Lut1DType);
        OIIO_CHECK_EQUAL(opList[1]->getName(), "IndexMap LUT");
        OIIO_CHECK_EQUAL(opList[1]->getID(), "lut-26");
        OIIO_CHECK_EQUAL(opList[1]->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_EQUAL(opList[1]->getOutputBitDepth(), OCIO::BIT_DEPTH_F16);
        OIIO_REQUIRE_EQUAL(opList[1]->getDescriptions().size(), 1);
        OIIO_CHECK_EQUAL(opList[1]->getDescriptions()[0],
                         " 1D LUT with IndexMap ");
    }
}

OIIO_ADD_TEST(FileFormatCTF, lut_1d)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    {
        const std::string ctfFile("lut1d_32_10i_10i.ctf");
        OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
        OIIO_REQUIRE_ASSERT((bool)cachedFile);
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getName(), "1d-lut example");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getID(),
                         "9843a859-e41e-40a8-a51c-840889c3774e");
        OIIO_REQUIRE_EQUAL(cachedFile->m_transform->getDescriptions().size(), 1);
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions()[0],
                         "Apply a 1/2.2 gamma.");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getInputDescriptor(), "RGB");
        OIIO_CHECK_EQUAL(cachedFile->m_transform->getOutputDescriptor(), "RGB");
        const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_REQUIRE_EQUAL(opList.size(), 1);

        OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::Lut1DType);
        auto pLut = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[0]);
        OIIO_REQUIRE_ASSERT(pLut);

        OIIO_REQUIRE_EQUAL(pLut->getDescriptions().size(), 1);

        OIIO_CHECK_ASSERT(!pLut->isInputHalfDomain());
        OIIO_CHECK_ASSERT(!pLut->isOutputRawHalfs());
        OIIO_CHECK_EQUAL(pLut->getHueAdjust(), OCIO::Lut1DOpData::HUE_NONE);

        OIIO_CHECK_EQUAL(pLut->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_EQUAL(pLut->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_ASSERT(pLut->getName() == "1d-lut example op");

        // TODO: bypass is for CTF
        // OIIO_CHECK_ASSERT(!pLut->getBypass()->isDynamic());

        // LUT is defined with a 32x1 array.
        // Array is extended to 32x3 by duplicating the available component.
        const OCIO::Array & array = pLut->getArray();
        OIIO_CHECK_EQUAL(array.getLength(), 32);
        OIIO_CHECK_EQUAL(array.getNumColorComponents(), 1);
        OIIO_CHECK_EQUAL(array.getNumValues(),
                         array.getLength()
                         *pLut->getArray().getMaxColorComponents());

        OIIO_REQUIRE_EQUAL(array.getValues().size(), 96);
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

    // Test the hue adjust attribute.
    {
        const std::string ctfFile("lut1d_hue_adjust_test.ctf");
        OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
        OIIO_REQUIRE_ASSERT((bool)cachedFile);

        const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
        OIIO_REQUIRE_EQUAL(opList.size(), 1);
        OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::Lut1DType);
        auto pLut = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[0]);
        OIIO_REQUIRE_ASSERT(pLut);
        OIIO_CHECK_EQUAL(pLut->getHueAdjust(), OCIO::Lut1DOpData::HUE_DW3);
    }
}

OIIO_ADD_TEST(FileFormatCTF, matrix4x4)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("matrix_example4x4.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::CTFVersion ctfVersion =
        cachedFile->m_transform->getCTFVersion();
    OIIO_CHECK_ASSERT(OCIO::CTF_PROCESS_LIST_VERSION_1_2 == ctfVersion);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::MatrixType);
    auto pMatrix =
        std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pMatrix);

    OIIO_CHECK_ASSERT(cachedFile->m_transform->getInputDescriptor() == "XYZ");
    OIIO_CHECK_ASSERT(cachedFile->m_transform->getOutputDescriptor() == "RGB");

    OIIO_CHECK_EQUAL(pMatrix->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pMatrix->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    // In file, matrix is defined by a 4x4 array.
    const OCIO::ArrayDouble & array = pMatrix->getArray();
    OIIO_CHECK_EQUAL(array.getLength(), 4);
    OIIO_CHECK_EQUAL(array.getNumColorComponents(), 4);
    OIIO_CHECK_EQUAL(array.getNumValues(),
                     array.getLength()*array.getLength());

    OIIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());
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

    const OCIO::MatrixOpData::Offsets & offsets = pMatrix->getOffsets();
    // ... offset
    OIIO_CHECK_EQUAL(offsets[0], 0.987654321098);
    OIIO_CHECK_EQUAL(offsets[1], 0.2);
    OIIO_CHECK_EQUAL(offsets[2], 0.3);
    OIIO_CHECK_EQUAL(offsets[3], 0.0);
}

OIIO_ADD_TEST(FileFormatCTF, matrix_1_3_3x3)
{
    // Version 1.3, array 3x3x3: matrix with no alpha and no offsets.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("matrix_example_1_3_3x3.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::CTFVersion & ctfVersion =
        cachedFile->m_transform->getCTFVersion();
    OIIO_CHECK_ASSERT(OCIO::CTF_PROCESS_LIST_VERSION_1_3 == ctfVersion);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::MatrixType);
    auto pMatrix =
        std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pMatrix);

    OIIO_CHECK_ASSERT(cachedFile->m_transform->getInputDescriptor() == "XYZ");
    OIIO_CHECK_ASSERT(cachedFile->m_transform->getOutputDescriptor() == "RGB");

    OIIO_CHECK_EQUAL(pMatrix->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    OIIO_CHECK_EQUAL(pMatrix->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

    // 3x3 array gets extended to 4x4.
    const OCIO::ArrayDouble & array = pMatrix->getArray();
    OIIO_CHECK_EQUAL(array.getLength(), 4);
    OIIO_CHECK_EQUAL(array.getNumColorComponents(), 4);
    OIIO_CHECK_EQUAL(array.getNumValues(),
                     array.getLength()*array.getLength());

    OIIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());
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

    const OCIO::MatrixOpData::Offsets & offsets = pMatrix->getOffsets();
    OIIO_CHECK_EQUAL(offsets[1], 0.0);
    OIIO_CHECK_EQUAL(offsets[2], 0.0);
    OIIO_CHECK_EQUAL(offsets[3], 0.0);
    OIIO_CHECK_EQUAL(offsets[0], 0.0);
}

OIIO_ADD_TEST(FileFormatCTF, matrix_1_3_4x4)
{
    // Version 1.3, array 4x4x4, matrix with alpha and no offsets.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("matrix_example_1_3_4x4.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::CTFVersion & ctfVersion =
        cachedFile->m_transform->getCTFVersion();
    OIIO_CHECK_ASSERT(OCIO::CTF_PROCESS_LIST_VERSION_1_3 == ctfVersion);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::MatrixType);
    auto pMatrix =
        std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pMatrix);

    OIIO_CHECK_ASSERT(cachedFile->m_transform->getInputDescriptor() == "XYZ");
    OIIO_CHECK_ASSERT(cachedFile->m_transform->getOutputDescriptor() == "RGB");

    OIIO_CHECK_EQUAL(pMatrix->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pMatrix->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    const OCIO::ArrayDouble & array = pMatrix->getArray();
    OIIO_CHECK_EQUAL(array.getLength(), 4);
    OIIO_CHECK_EQUAL(array.getNumColorComponents(), 4);
    OIIO_CHECK_EQUAL(array.getNumValues(),
                     array.getLength()*array.getLength());

    OIIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());

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

    const OCIO::MatrixOpData::Offsets & offsets = pMatrix->getOffsets();
    OIIO_CHECK_EQUAL(offsets[0], 0.0);
    OIIO_CHECK_EQUAL(offsets[1], 0.0);
    OIIO_CHECK_EQUAL(offsets[2], 0.0);
    OIIO_CHECK_EQUAL(offsets[3], 0.0);
}

OIIO_ADD_TEST(FileFormatCTF, matrix_1_3_offsets)
{
    // Version 1.3, array 3x4x3: matrix only with offsets and no alpha.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("matrix_example_1_3_offsets.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::CTFVersion ctfVersion =
        cachedFile->m_transform->getCTFVersion();
    OIIO_CHECK_ASSERT(OCIO::CTF_PROCESS_LIST_VERSION_1_3 == ctfVersion);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::MatrixType);
    auto pMatrix =
        std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pMatrix);

    OIIO_CHECK_ASSERT(cachedFile->m_transform->getInputDescriptor() == "XYZ");
    OIIO_CHECK_ASSERT(cachedFile->m_transform->getOutputDescriptor() == "RGB");

    OIIO_CHECK_EQUAL(pMatrix->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pMatrix->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    const OCIO::ArrayDouble & array = pMatrix->getArray();
    OIIO_CHECK_EQUAL(array.getLength(), 4);
    OIIO_CHECK_EQUAL(array.getNumColorComponents(), 4);
    OIIO_CHECK_EQUAL(array.getNumValues(),
                     array.getLength()*array.getLength());

    OIIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());
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

    const OCIO::MatrixOpData::Offsets & offsets = pMatrix->getOffsets();
    OIIO_CHECK_EQUAL(offsets[0], 0.1);
    OIIO_CHECK_EQUAL(offsets[1], 0.2);
    OIIO_CHECK_EQUAL(offsets[2], 0.3);
    OIIO_CHECK_EQUAL(offsets[3], 0.0);
}

OIIO_ADD_TEST(FileFormatCTF, matrix_1_3_alpha_offsets)
{
    // Verion 1.3, array 4x5x4: matrix with alpha and offsets.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("matrix_example_1_3_alpha_offsets.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::CTFVersion & ctfVersion =
        cachedFile->m_transform->getCTFVersion();
    OIIO_CHECK_ASSERT(OCIO::CTF_PROCESS_LIST_VERSION_1_3 == ctfVersion);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::MatrixType);
    auto pMatrix =
        std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pMatrix);

    OIIO_CHECK_ASSERT(cachedFile->m_transform->getInputDescriptor() == "XYZ");
    OIIO_CHECK_ASSERT(cachedFile->m_transform->getOutputDescriptor() == "RGB");

    OIIO_CHECK_EQUAL(pMatrix->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pMatrix->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    const OCIO::ArrayDouble & array = pMatrix->getArray();
    OIIO_CHECK_EQUAL(array.getLength(), 4);
    OIIO_CHECK_EQUAL(array.getNumColorComponents(), 4);
    OIIO_CHECK_EQUAL(array.getNumValues(),
                     array.getLength()*array.getLength());

    OIIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());
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

    const OCIO::MatrixOpData::Offsets & offsets = pMatrix->getOffsets();
    OIIO_CHECK_EQUAL(offsets[0], 0.1);
    OIIO_CHECK_EQUAL(offsets[1], 0.2);
    OIIO_CHECK_EQUAL(offsets[2], 0.3);
    OIIO_CHECK_EQUAL(offsets[3], 0.4);
}

OIIO_ADD_TEST(FileFormatCTF, 3by1d_lut)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("xyz_to_rgb.clf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 2);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::MatrixType);
    auto pMatrix =
        std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pMatrix);

    OIIO_CHECK_EQUAL(pMatrix->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pMatrix->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    const OCIO::ArrayDouble & a1 = pMatrix->getArray();
    OIIO_CHECK_EQUAL(a1.getLength(), 4);
    OIIO_CHECK_EQUAL(a1.getNumColorComponents(), 4);
    OIIO_CHECK_EQUAL(a1.getNumValues(), a1.getLength()*a1.getLength());

    OIIO_REQUIRE_EQUAL(a1.getValues().size(), a1.getNumValues());
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

    auto pLut =
        std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[1]);
    OIIO_REQUIRE_ASSERT(pLut);
    OIIO_CHECK_EQUAL(pLut->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(pLut->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pLut->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    const OCIO::Array & a2 = pLut->getArray();
    OIIO_CHECK_EQUAL(a2.getLength(), 17);
    OIIO_CHECK_EQUAL(a2.getNumColorComponents(), 3);
    OIIO_CHECK_EQUAL(a2.getNumValues(),
                     a2.getLength()*pLut->getArray().getMaxColorComponents());

    OIIO_REQUIRE_EQUAL(a2.getValues().size(), a2.getNumValues());
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

OIIO_ADD_TEST(FileFormatCTF, lut1d_inv)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("lut1d_inv.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 2);

    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::MatrixType);
    auto pMatrix =
        std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);

    OIIO_REQUIRE_ASSERT(pMatrix);
    OIIO_CHECK_EQUAL(pMatrix->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pMatrix->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    const OCIO::ArrayDouble & a1 = pMatrix->getArray();
    OIIO_CHECK_EQUAL(a1.getLength(), 4);
    OIIO_CHECK_EQUAL(a1.getNumColorComponents(), 4);
    OIIO_CHECK_EQUAL(a1.getNumValues(), a1.getLength()*a1.getLength());

    OIIO_REQUIRE_EQUAL(a1.getValues().size(), a1.getNumValues());
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

    OIIO_CHECK_EQUAL(opList[1]->getType(), OCIO::OpData::Lut1DType);
    auto pLut =
        std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[1]);

    OIIO_REQUIRE_ASSERT(pLut);
    OIIO_CHECK_EQUAL(pLut->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pLut->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(pLut->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    const OCIO::Array & a2 = pLut->getArray();
    OIIO_CHECK_EQUAL(a2.getNumColorComponents(), 3);

    OIIO_CHECK_EQUAL(a2.getLength(), 17);
    OIIO_CHECK_EQUAL(a2.getNumValues(),
                     a2.getLength()*a2.getMaxColorComponents());

    const float error = 1e-6f;
    OIIO_REQUIRE_EQUAL(a2.getValues().size(), a2.getNumValues());

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

OIIO_ADD_TEST(FileFormatCTF, lut3d)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("lut3d_17x17x17_32f_12i.clf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);

    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::Lut3DType);
    auto pLut =
        std::dynamic_pointer_cast<const OCIO::Lut3DOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pLut);
    OIIO_CHECK_EQUAL(pLut->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(pLut->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pLut->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);

    const OCIO::Array & array = pLut->getArray();
    OIIO_CHECK_EQUAL(array.getLength(), 17);
    OIIO_CHECK_EQUAL(array.getNumColorComponents(), 3);
    OIIO_CHECK_EQUAL(array.getNumValues(),
                     array.getLength()*array.getLength()
                     *array.getLength()
                     *pLut->getArray().getMaxColorComponents());

    OIIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());
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

OIIO_ADD_TEST(FileFormatCTF, lut3d_inv)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("lut3d_example_Inv.ctf");

    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);

    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::Lut3DType);
    auto pLut =
        std::dynamic_pointer_cast<const OCIO::Lut3DOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pLut);

    OIIO_CHECK_EQUAL(pLut->getInputBitDepth(), OCIO::BIT_DEPTH_UINT12);
    OIIO_CHECK_EQUAL(pLut->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pLut->getInterpolation(), OCIO::INTERP_TETRAHEDRAL);
    OIIO_CHECK_EQUAL(pLut->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    const OCIO::Array & array = pLut->getArray();
    OIIO_CHECK_EQUAL(array.getNumColorComponents(), 3);
    OIIO_CHECK_EQUAL(array.getNumValues(),
                     array.getLength()*array.getLength()
                     *array.getLength()*array.getMaxColorComponents());
    OIIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());

    OIIO_CHECK_EQUAL(array.getLength(), 17);
    OIIO_CHECK_EQUAL(array.getValues()[0], 25.0f);
    OIIO_CHECK_EQUAL(array.getValues()[1], 30.0f);
    OIIO_CHECK_EQUAL(array.getValues()[2], 33.0f);

    OIIO_CHECK_EQUAL(array.getValues()[18], 26.0f);
    OIIO_CHECK_EQUAL(array.getValues()[19], 308.0f);
    OIIO_CHECK_EQUAL(array.getValues()[20], 580.0f);

    OIIO_CHECK_EQUAL(array.getValues()[30], 0.0f);
    OIIO_CHECK_EQUAL(array.getValues()[31], 586.0f);
    OIIO_CHECK_EQUAL(array.getValues()[32], 1350.0f);
}

OIIO_ADD_TEST(FileFormatCTF, check_utf8)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("matrix_example_utf8.clf");

    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_REQUIRE_EQUAL(opList[0]->getDescriptions().size(), 1);
    std::string & desc = opList[0]->getDescriptions()[0];
    const std::string utf8Test
        ("\xE6\xA8\x99\xE6\xBA\x96\xE8\x90\xAC\xE5\x9C\x8B\xE7\xA2\xBC");
    OIIO_CHECK_EQUAL(desc, utf8Test);
    const std::string utf8TestWrong
        ("\xE5\xA8\x99\xE6\xBA\x96\xE8\x90\xAC\xE5\x9C\x8B\xE7\xA2\xBC");
    OIIO_CHECK_NE(desc, utf8TestWrong);

}

OIIO_ADD_TEST(FileFormatCTF, error_checker)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    // NB: This file has some added unknown elements A, B, and C as a test.
    const std::string ctfFile("unknown_elements.clf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 4);

    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::MatrixType);
    auto pMatrix =
        std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pMatrix);

    OIIO_CHECK_EQUAL(pMatrix->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pMatrix->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    const OCIO::ArrayDouble & a1 = pMatrix->getArray();
    OIIO_CHECK_EQUAL(a1.getLength(), 4);
    OIIO_CHECK_EQUAL(a1.getNumColorComponents(), 4);
    OIIO_CHECK_EQUAL(a1.getNumValues(), a1.getLength()*a1.getLength());

    OIIO_REQUIRE_EQUAL(a1.getValues().size(), a1.getNumValues());
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

    OIIO_CHECK_EQUAL(opList[1]->getType(), OCIO::OpData::Lut1DType);
    auto pLut1 =
        std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[1]);
    OIIO_REQUIRE_ASSERT(pLut1);
    OIIO_CHECK_EQUAL(pLut1->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pLut1->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    const OCIO::Array & a2 = pLut1->getArray();
    OIIO_CHECK_EQUAL(a2.getLength(), 17);
    OIIO_CHECK_EQUAL(a2.getNumColorComponents(), 3);
    OIIO_CHECK_EQUAL(a2.getNumValues(), 
                     a2.getLength()
                     *pLut1->getArray().getMaxColorComponents());

    OIIO_REQUIRE_EQUAL(a2.getValues().size(), a2.getNumValues());
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

    OIIO_CHECK_EQUAL(opList[2]->getType(), OCIO::OpData::Lut1DType);
    auto pLut2 =
        std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[2]);
    OIIO_REQUIRE_ASSERT(pLut2);
    OIIO_CHECK_EQUAL(pLut2->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pLut2->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

    const OCIO::Array & array = pLut2->getArray();
    OIIO_CHECK_EQUAL(array.getLength(), 32);
    OIIO_CHECK_EQUAL(array.getNumColorComponents(), 1);
    OIIO_CHECK_EQUAL(array.getNumValues(),
                     array.getLength()
                     *pLut2->getArray().getMaxColorComponents());

    OIIO_REQUIRE_EQUAL(array.getValues().size(), 96);
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

    OIIO_CHECK_EQUAL(opList[3]->getType(), OCIO::OpData::Lut3DType);
    auto pLut3 =
        std::dynamic_pointer_cast<const OCIO::Lut3DOpData>(opList[3]);
    OIIO_REQUIRE_ASSERT(pLut3);
    OIIO_CHECK_EQUAL(pLut3->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    OIIO_CHECK_EQUAL(pLut3->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

    const OCIO::Array & a3 = pLut3->getArray();
    OIIO_CHECK_EQUAL(a3.getLength(), 3);
    OIIO_CHECK_EQUAL(a3.getNumColorComponents(), 3);
    OIIO_CHECK_EQUAL(a3.getNumValues(),
                     a3.getLength()*a3.getLength()*a3.getLength()
                     *pLut3->getArray().getMaxColorComponents());

    OIIO_REQUIRE_EQUAL(a3.getValues().size(), a3.getNumValues());
    OIIO_CHECK_EQUAL(a3.getValues()[0], 0.0f);
    OIIO_CHECK_EQUAL(a3.getValues()[1], 30.0f);
    OIIO_CHECK_EQUAL(a3.getValues()[2], 33.0f);
    OIIO_CHECK_EQUAL(a3.getValues()[3], 0.0f);
    OIIO_CHECK_EQUAL(a3.getValues()[4], 0.0f);
    OIIO_CHECK_EQUAL(a3.getValues()[5], 133.0f);

    OIIO_CHECK_EQUAL(a3.getValues()[78], 1023.0f);
    OIIO_CHECK_EQUAL(a3.getValues()[79], 1023.0f);
    OIIO_CHECK_EQUAL(a3.getValues()[80], 1023.0f);

    // TODO: check log for parsing warnings.
    // DummyElt is logging at debug level.
    // Ignore element 'B' (line 34) where its parent is 'ProcessList' (line 2) : Unknown element : unknown_elements.clf
    // Ignore element 'C' (line 34) where its parent is 'B' (line 34) : unknown_elements.clf
    // Ignore element 'A' (line 36) where its parent is 'Description' (line 36) : unknown_elements.clf
}

OIIO_ADD_TEST(FileFormatCTF, binary_file)
{
    const std::string ctfFile("image_png.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception,
                          "is not a CTF/CLF file.");
}

OIIO_ADD_TEST(FileFormatCTF, error_checker_for_difficult_xml)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("difficult_test1_v1.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    // Defaults to 1.2
    const OCIO::CTFVersion ctfVersion =
        cachedFile->m_transform->getCTFVersion();
    OIIO_CHECK_ASSERT(OCIO::CTF_PROCESS_LIST_VERSION_1_2 == ctfVersion);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 2);

    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::MatrixType);
    auto pMatrix =
        std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pMatrix);
    OIIO_CHECK_EQUAL(pMatrix->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pMatrix->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    const OCIO::ArrayDouble & array = pMatrix->getArray();
    OIIO_CHECK_EQUAL(array.getLength(), 4u);
    OIIO_CHECK_EQUAL(array.getNumColorComponents(), 4u);
    OIIO_CHECK_EQUAL(array.getNumValues(),
                     array.getLength()*array.getLength());

    OIIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());
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

    auto pLut = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[1]);
    OIIO_REQUIRE_ASSERT(pLut);
    OIIO_CHECK_EQUAL(pLut->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pLut->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    const OCIO::Array & array2 = pLut->getArray();
    OIIO_CHECK_EQUAL(array2.getLength(), 17);
    OIIO_CHECK_EQUAL(array2.getNumColorComponents(), 3);
    OIIO_CHECK_EQUAL(array2.getNumValues(),
                     array2.getLength()
                     *pLut->getArray().getMaxColorComponents());

    OIIO_REQUIRE_EQUAL(array2.getValues().size(), 51);
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

    // TODO: check log for parsing warnings.
    // DummyElt is logging at debug level.
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

OIIO_ADD_TEST(FileFormatCTF, invalid_transform)
{
    const std::string ctfFile("transform_invalid.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception,
                          "is not a CTF/CLF file.");
}


OIIO_ADD_TEST(FileFormatCTF, missing_element_end)
{
    const std::string ctfFile("transform_element_end_missing.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception, "no element found");
}


OIIO_ADD_TEST(FileFormatCTF, missing_transform_id)
{
    const std::string ctfFile("transform_missing_id.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Required attribute 'id'");
}

OIIO_ADD_TEST(FileFormatCTF, missing_in_bitdepth)
{
    const std::string ctfFile("transform_missing_inbitdepth.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "inBitDepth is missing");
}

OIIO_ADD_TEST(FileFormatCTF, missing_out_bitdepth)
{
    const std::string ctfFile("transform_missing_outbitdepth.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "outBitDepth is missing");
}

OIIO_ADD_TEST(FileFormatCTF, array_missing_values)
{
    const std::string ctfFile("array_missing_values.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Expected 3x3 Array values");
}

OIIO_ADD_TEST(FileFormatCTF, array_illegal_values)
{
    const std::string ctfFile("array_illegal_values.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception,
                          "Illegal values");
}

OIIO_ADD_TEST(FileFormatCTF, unknown_value)
{
    const std::string ctfFile("unknown_outdepth.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "outBitDepth unknown value");
}

OIIO_ADD_TEST(FileFormatCTF, array_corrupted_dimension)
{
    const std::string ctfFile("array_illegal_dimension.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Illegal dimensions");
}

OIIO_ADD_TEST(FileFormatCTF, array_too_many_values)
{
    const std::string ctfFile("array_too_many_values.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Expected 3x3 Array, found too many values");
}

OIIO_ADD_TEST(FileFormatCTF, matrix_bitdepth_illegal)
{
    const std::string ctfFile("matrix_bitdepth_illegal.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "inBitDepth unknown value");
}

OIIO_ADD_TEST(FileFormatCTF, matrix_end_missing)
{
    const std::string ctfFile("matrix_end_missing.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "no closing tag for 'Matrix'");
}

OIIO_ADD_TEST(FileFormatCTF, transform_corrupted_tag)
{
    const std::string ctfFile("transform_corrupted_tag.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception,
                          "no closing tag");
}

OIIO_ADD_TEST(FileFormatCTF, transform_empty)
{
    const std::string ctfFile("transform_empty.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "No color operator");
}

OIIO_ADD_TEST(FileFormatCTF, transform_id_empty)
{
    const std::string ctfFile("transform_id_empty.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Required attribute 'id' does not have a value");
}

OIIO_ADD_TEST(FileFormatCTF, transform_with_bitdepth_mismatch)
{
    const std::string ctfFile("transform_bitdepth_mismatch.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Bitdepth missmatch");
}

OIIO_ADD_TEST(FileFormatCTF, check_index_map)
{
    const std::string ctfFile("indexMap_test.ctf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Only two entry IndexMaps are supported");
}

OIIO_ADD_TEST(FileFormatCTF, matrix_with_offset)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("matrix_offsets_example.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);
    // Note that the ProcessList does not have a version attribute and
    // therefore defaults to 1.2.
    // The "4x4x3" Array syntax is only allowed in versions 1.2 or earlier.
    const OCIO::CTFVersion ctfVersion =
        cachedFile->m_transform->getCTFVersion();
    OIIO_CHECK_ASSERT(OCIO::CTF_PROCESS_LIST_VERSION_1_2 == ctfVersion);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::MatrixType);
    auto pMatrix =
        std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pMatrix);

    OIIO_CHECK_EQUAL(pMatrix->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pMatrix->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    const OCIO::ArrayDouble & array = pMatrix->getArray();
    OIIO_CHECK_EQUAL(array.getLength(), 4);
    OIIO_CHECK_EQUAL(array.getNumColorComponents(), 4);
    OIIO_CHECK_EQUAL(array.getNumValues(),
                     array.getLength()*array.getLength());

    OIIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());
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

OIIO_ADD_TEST(FileFormatCTF, matrix_with_offset_1_3)
{
    // Matrix 4 4 3 only valid up to version 1.2.
    const std::string ctfFile("matrix_offsets_example_1_3.ctf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Illegal dimensions 4 4 3");
}

OIIO_ADD_TEST(FileFormatCTF, lut_3by1d_with_nan_infinity)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("lut3by1d_nan_infinity_example.clf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::Lut1DType);
    auto pLut1d =
        std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pLut1d);

    const OCIO::Array & array = pLut1d->getArray();

    OIIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());
    OIIO_CHECK_ASSERT(OCIO::IsNan(array.getValues()[0]));
    OIIO_CHECK_ASSERT(OCIO::IsNan(array.getValues()[1]));
    OIIO_CHECK_ASSERT(OCIO::IsNan(array.getValues()[2]));
    OIIO_CHECK_ASSERT(OCIO::IsNan(array.getValues()[3]));
    OIIO_CHECK_ASSERT(OCIO::IsNan(array.getValues()[4]));
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

OIIO_ADD_TEST(FileFormatCTF, lut1d_half_domain_set_false)
{
    // Should throw an exception because the 'half_domain' tag
    // was found but set to something other than 'true'.
    const std::string ctfFile("lut1d_half_domain_set_false.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Illegal 'halfDomain' attribute");
}

OIIO_ADD_TEST(FileFormatCTF, lut1d_raw_half_set_false)
{
    // Should throw an exception because the 'raw_halfs' tag
    // was found but set to something other than 'true'.
    const std::string ctfFile("lut1d_raw_half_set_false.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Illegal 'rawHalfs' attribute");
}

OIIO_ADD_TEST(FileFormatCTF, lut1d_half_domain_raw_half_set)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("lut1d_half_domain_raw_half_set.clf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::Lut1DType);
    auto pLut1d =
        std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pLut1d);

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

OIIO_ADD_TEST(FileFormatCTF, lut1d_half_domain_invalid_entries)
{
    const std::string ctfFile("lut1d_half_domain_invalid_entries.clf");
    // This should fail with invalid entries exception because the number
    // of entries in the op is not 65536 (required when using half domain).
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "65536 required for halfDomain");
}

OIIO_ADD_TEST(FileFormatCTF, inverse_of_id_test)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("inverseOfId_test.clf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    OIIO_CHECK_ASSERT(cachedFile->m_transform->getInverseOfId() ==
                      "inverseOfIdTest");
}

OIIO_ADD_TEST(FileFormatCTF, range1)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("range_test1.clf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::RangeType);
    auto pR = std::dynamic_pointer_cast<const OCIO::RangeOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pR);

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

OIIO_ADD_TEST(FileFormatCTF, range2)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("range_test2.clf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::RangeType);
    auto pR = std::dynamic_pointer_cast<const OCIO::RangeOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pR);
    OIIO_CHECK_EQUAL(pR->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pR->getOutputBitDepth(), OCIO::BIT_DEPTH_F16);

    OIIO_CHECK_EQUAL((float)pR->getMinInValue(), 0.1f);
    OIIO_CHECK_EQUAL((float)pR->getMinOutValue(), -0.1f);

    OIIO_CHECK_ASSERT(!pR->minIsEmpty());
    OIIO_CHECK_ASSERT(pR->maxIsEmpty());
}

OIIO_ADD_TEST(FileFormatCTF, range3)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("range_test3.clf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::RangeType);
    auto pR = std::dynamic_pointer_cast<const OCIO::RangeOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pR);
    OIIO_CHECK_EQUAL(pR->getInputBitDepth(), OCIO::BIT_DEPTH_F16);
    OIIO_CHECK_EQUAL(pR->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    OIIO_CHECK_ASSERT(pR->minIsEmpty());
    OIIO_CHECK_ASSERT(pR->maxIsEmpty());
}

OIIO_ADD_TEST(FileFormatCTF, gamma1)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("gamma_test1.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    OIIO_CHECK_EQUAL(cachedFile->m_transform->getID(), "id");

    OIIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions().size(), 1);
    OIIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions()[0], "2.4 gamma");

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::GammaType);
    auto pG = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pG);

    OIIO_CHECK_EQUAL(pG->getInputBitDepth(), OCIO::BIT_DEPTH_F16);
    OIIO_CHECK_EQUAL(pG->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);

    OIIO_CHECK_EQUAL(pG->getStyle(), OCIO::GammaOpData::BASIC_FWD);

    OCIO::GammaOpData::Params params;
    params.push_back(2.4);

    OIIO_CHECK_ASSERT(pG->getRedParams() == params);
    OIIO_CHECK_ASSERT(pG->getGreenParams() == params);
    OIIO_CHECK_ASSERT(pG->getBlueParams() == params);
    // Version of the ctf is less than 1.5, so alpha must be identity.
    OIIO_CHECK_ASSERT(OCIO::GammaOpData::isIdentityParameters(
                          pG->getAlphaParams(),
                          pG->getStyle()));

    OIIO_CHECK_ASSERT(!pG->areAllComponentsEqual());
    OIIO_CHECK_ASSERT(pG->isNonChannelDependent()); // RGB are equal, A is an identity
}

OIIO_ADD_TEST(FileFormatCTF, gamma2)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("gamma_test2.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::GammaType);
    auto pG = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pG);

    OIIO_CHECK_EQUAL(pG->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pG->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

    OIIO_CHECK_EQUAL(pG->getStyle(), OCIO::GammaOpData::BASIC_REV);
    OCIO::GammaOpData::Params paramsR;
    paramsR.push_back(2.4);
    OCIO::GammaOpData::Params paramsG;
    paramsG.push_back(2.35);
    OCIO::GammaOpData::Params paramsB;
    paramsB.push_back(2.2);

    OIIO_CHECK_ASSERT(pG->getRedParams() == paramsR);
    OIIO_CHECK_ASSERT(pG->getGreenParams() == paramsG);
    OIIO_CHECK_ASSERT(pG->getBlueParams() == paramsB);
    OIIO_CHECK_ASSERT(OCIO::GammaOpData::isIdentityParameters(
                          pG->getAlphaParams(),
                          pG->getStyle()));

    OIIO_CHECK_ASSERT(!pG->areAllComponentsEqual());
    OIIO_CHECK_ASSERT(!pG->isNonChannelDependent());
}

OIIO_ADD_TEST(FileFormatCTF, gamma3)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("gamma_test3.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::GammaType);
    auto pG = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pG);

    OIIO_CHECK_EQUAL(pG->getInputBitDepth(), OCIO::BIT_DEPTH_F16);
    OIIO_CHECK_EQUAL(pG->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);

    OIIO_CHECK_EQUAL(pG->getStyle(), OCIO::GammaOpData::MONCURVE_FWD);
    OCIO::GammaOpData::Params params;
    params.push_back(1. / 0.45);
    params.push_back(0.099);

    // This is a precision test to ensure we can recreate a double that is
    // exactly equal to 1/0.45, which is required to implement rec 709 exactly.
    OIIO_CHECK_ASSERT(pG->getRedParams() == params);
    OIIO_CHECK_ASSERT(pG->getGreenParams() == params);
    OIIO_CHECK_ASSERT(pG->getBlueParams() == params);
    OIIO_CHECK_ASSERT(OCIO::GammaOpData::isIdentityParameters(
                          pG->getAlphaParams(),
                          pG->getStyle()));

    OIIO_CHECK_ASSERT(!pG->areAllComponentsEqual());
    OIIO_CHECK_ASSERT(pG->isNonChannelDependent());
}

OIIO_ADD_TEST(FileFormatCTF, gamma4)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("gamma_test4.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::GammaType);
    auto pG = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pG);

    OIIO_CHECK_EQUAL(pG->getInputBitDepth(), OCIO::BIT_DEPTH_F16);
    OIIO_CHECK_EQUAL(pG->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    OIIO_CHECK_EQUAL(pG->getStyle(), OCIO::GammaOpData::MONCURVE_REV);
    OCIO::GammaOpData::Params paramsR;
    paramsR.push_back(2.2);
    paramsR.push_back(0.001);
    OCIO::GammaOpData::Params paramsG;
    paramsG.push_back(2.4);
    paramsG.push_back(0.01);
    OCIO::GammaOpData::Params paramsB;
    paramsB.push_back(2.6);
    paramsB.push_back(0.1);

    OIIO_CHECK_ASSERT(pG->getRedParams() == paramsR);
    OIIO_CHECK_ASSERT(pG->getGreenParams() == paramsG);
    OIIO_CHECK_ASSERT(pG->getBlueParams() == paramsB);
    OIIO_CHECK_ASSERT(OCIO::GammaOpData::isIdentityParameters(
                          pG->getAlphaParams(),
                          pG->getStyle()));

    OIIO_CHECK_ASSERT(!pG->areAllComponentsEqual());
    OIIO_CHECK_ASSERT(!pG->isNonChannelDependent());
}

OIIO_ADD_TEST(FileFormatCTF, gamma5)
{
    // This test is for an old (< 1.5) transform file that contains
    // an invalid GammaParams for the A channel.
    const std::string ctfFile("gamma_test5.ctf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception,
                          "Invalid channel");
}

OIIO_ADD_TEST(FileFormatCTF, gamma6)
{
    // This test is for an old (< 1.5) transform file that contains
    // a single GammaParams with identity values:
    // - R, G and B set to identity parameters (identity test).
    // - A set to identity.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("gamma_test6.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::GammaType);
    auto pG = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pG);

    OIIO_CHECK_EQUAL(pG->getInputBitDepth(), OCIO::BIT_DEPTH_F16);
    OIIO_CHECK_EQUAL(pG->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);

    OIIO_CHECK_EQUAL(pG->getStyle(), OCIO::GammaOpData::MONCURVE_FWD);
    OIIO_CHECK_ASSERT(pG->areAllComponentsEqual());
    OIIO_CHECK_ASSERT(pG->isNonChannelDependent()); 
    OIIO_CHECK_ASSERT(pG->isIdentity());
}

OIIO_ADD_TEST(FileFormatCTF, gamma_wrong_power)
{
    // The moncurve style requires a gamma value >= 1.
    const std::string ctfFile("gamma_wrong_power.ctf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception,
                          "is less than lower bound");
}

OIIO_ADD_TEST(FileFormatCTF, gamma_alpha1)
{
    // This test is for a new (>= 1.5) transform file that contains
    // a single GammaParams:
    // - R, G and B set to same parameters.
    // - A set to identity.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("gamma_alpha_test1.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::GammaType);
    auto pG = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pG);

    OIIO_CHECK_EQUAL(pG->getInputBitDepth(), OCIO::BIT_DEPTH_F16);
    OIIO_CHECK_EQUAL(pG->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(pG->getStyle(), OCIO::GammaOpData::BASIC_FWD);

    OCIO::GammaOpData::Params params;
    params.push_back(2.4);

    OIIO_CHECK_ASSERT(pG->getRedParams() == params);
    OIIO_CHECK_ASSERT(pG->getGreenParams() == params);
    OIIO_CHECK_ASSERT(pG->getBlueParams() == params);
    OIIO_CHECK_ASSERT(OCIO::GammaOpData::isIdentityParameters(
                          pG->getAlphaParams(),
                          pG->getStyle()));

    OIIO_CHECK_ASSERT(!pG->areAllComponentsEqual());
    OIIO_CHECK_ASSERT(pG->isNonChannelDependent());
}

OIIO_ADD_TEST(FileFormatCTF, gamma_alpha2)
{
    // This test is for a new (>= 1.5) transform file that contains
    // a different GammaParams for every channel:
    // - R, G, B and A set to different parameters.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("gamma_alpha_test2.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::GammaType);
    auto pG = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pG);

    OIIO_CHECK_EQUAL(pG->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pG->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    OIIO_CHECK_EQUAL(pG->getStyle(), OCIO::GammaOpData::BASIC_REV);

    OCIO::GammaOpData::Params paramsR;
    paramsR.push_back(2.4);
    OCIO::GammaOpData::Params paramsG;
    paramsG.push_back(2.35);
    OCIO::GammaOpData::Params paramsB;
    paramsB.push_back(2.2);
    OCIO::GammaOpData::Params paramsA;
    paramsA.push_back(2.5);

    OIIO_CHECK_ASSERT(pG->getRedParams() == paramsR);
    OIIO_CHECK_ASSERT(pG->getGreenParams() == paramsG);
    OIIO_CHECK_ASSERT(pG->getBlueParams() == paramsB);
    OIIO_CHECK_ASSERT(pG->getAlphaParams() == paramsA);

    OIIO_CHECK_ASSERT(!pG->areAllComponentsEqual());
    OIIO_CHECK_ASSERT(!pG->isNonChannelDependent());
}

OIIO_ADD_TEST(FileFormatCTF, gamma_alpha3)
{
    // This test is for a new (>= 1.5) transform file that contains
    // a single GammaParams:
    // - R, G and B set to same parameters (precision test).
    // - A set to identity.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("gamma_alpha_test3.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::GammaType);
    auto pG = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pG);

    OIIO_CHECK_EQUAL(pG->getInputBitDepth(), OCIO::BIT_DEPTH_F16);
    OIIO_CHECK_EQUAL(pG->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(pG->getStyle(), OCIO::GammaOpData::MONCURVE_FWD);

    OCIO::GammaOpData::Params params;
    params.push_back(1. / 0.45);
    params.push_back(0.099);

    OIIO_CHECK_ASSERT(pG->getRedParams() == params);
    OIIO_CHECK_ASSERT(pG->getGreenParams() == params);
    OIIO_CHECK_ASSERT(pG->getBlueParams() == params);
    OIIO_CHECK_ASSERT(OCIO::GammaOpData::isIdentityParameters(
                          pG->getAlphaParams(),
                          pG->getStyle()));

    OIIO_CHECK_ASSERT(!pG->areAllComponentsEqual());
    OIIO_CHECK_ASSERT(pG->isNonChannelDependent());
}

OIIO_ADD_TEST(FileFormatCTF, gamma_alpha4)
{
    // This test is for a new (>= 1.5) transform file that contains
    // a different GammaParams for every channel:
    // - R, G, B and A set to different parameters (attributes order test).
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("gamma_alpha_test4.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::GammaType);
    auto pG = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pG);

    OIIO_CHECK_EQUAL(pG->getInputBitDepth(), OCIO::BIT_DEPTH_F16);
    OIIO_CHECK_EQUAL(pG->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pG->getStyle(), OCIO::GammaOpData::MONCURVE_REV);

    OCIO::GammaOpData::Params paramsR;
    paramsR.push_back(2.2);
    paramsR.push_back(0.001);
    OCIO::GammaOpData::Params paramsG;
    paramsG.push_back(2.4);
    paramsG.push_back(0.01);
    OCIO::GammaOpData::Params paramsB;
    paramsB.push_back(2.6);
    paramsB.push_back(0.1);
    OCIO::GammaOpData::Params paramsA;
    paramsA.push_back(2.0);
    paramsA.push_back(0.0001);

    OIIO_CHECK_ASSERT(pG->getRedParams() == paramsR);
    OIIO_CHECK_ASSERT(pG->getGreenParams() == paramsG);
    OIIO_CHECK_ASSERT(pG->getBlueParams() == paramsB);
    OIIO_CHECK_ASSERT(pG->getAlphaParams() == paramsA);

    OIIO_CHECK_ASSERT(!pG->areAllComponentsEqual());
    OIIO_CHECK_ASSERT(!pG->isNonChannelDependent());
}

OIIO_ADD_TEST(FileFormatCTF, gamma_alpha5)
{
    // This test is for a new (>= 1.5) transform file that contains
    // a GammaParams with no channel specified:
    // - R, G and B set to same parameters.
    // and a GammaParams for the A channel:
    // - A set to different parameters.
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("gamma_alpha_test5.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::GammaType);
    auto pG = std::dynamic_pointer_cast<const OCIO::GammaOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pG);

    OIIO_CHECK_EQUAL(pG->getInputBitDepth(), OCIO::BIT_DEPTH_F16);
    OIIO_CHECK_EQUAL(pG->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(pG->getStyle(), OCIO::GammaOpData::MONCURVE_FWD);

    OCIO::GammaOpData::Params params;
    params.push_back(1. / 0.45);
    params.push_back(0.099);
    OCIO::GammaOpData::Params paramsA;
    paramsA.push_back(1.7);
    paramsA.push_back(0.33);

    OIIO_CHECK_ASSERT(pG->getRedParams() == params);
    OIIO_CHECK_ASSERT(pG->getGreenParams() == params);
    OIIO_CHECK_ASSERT(pG->getBlueParams() == params);
    OIIO_CHECK_ASSERT(pG->getAlphaParams() == paramsA);

    OIIO_CHECK_ASSERT(!pG->areAllComponentsEqual());
    OIIO_CHECK_ASSERT(!pG->isNonChannelDependent());
}

OIIO_ADD_TEST(FileFormatCTF, gamma_alpha6)
{
    // This test is for an new (>= 1.5) transform file that contains
    // an invalid GammaParams for the A channel (missing offset attribute).
    const std::string ctfFile("gamma_alpha_test6.ctf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception,
                          "Missing required offset parameter");
}

OIIO_ADD_TEST(FileFormatCTF, invalid_version)
{
    const std::string ctfFile("process_list_invalid_version.ctf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "is not a valid version");
}

OIIO_ADD_TEST(FileFormatCTF, valid_version)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("process_list_valid_version.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    
    const OCIO::CTFVersion ctfVersion =
        cachedFile->m_transform->getCTFVersion();
    OIIO_CHECK_EQUAL(ctfVersion, OCIO::CTF_PROCESS_LIST_VERSION_1_4);
}

OIIO_ADD_TEST(FileFormatCTF, higher_version)
{
    const std::string ctfFile("process_list_higher_version.ctf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile),
                          OCIO::Exception,
                          "Unsupported transform file version");
}

OIIO_ADD_TEST(FileFormatCTF, version_revision)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("process_list_version_revision.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));

    const OCIO::CTFVersion ctfVersion =
        cachedFile->m_transform->getCTFVersion();
    const OCIO::CTFVersion ver(1, 3, 10);
    OIIO_CHECK_EQUAL(ctfVersion, ver);
    OIIO_CHECK_ASSERT(OCIO::CTF_PROCESS_LIST_VERSION_1_3 < ctfVersion);
    OIIO_CHECK_ASSERT(ctfVersion < OCIO::CTF_PROCESS_LIST_VERSION_1_4);
}

OIIO_ADD_TEST(FileFormatCTF, no_version)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("process_list_no_version.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));

    const OCIO::CTFVersion ctfVersion =
        cachedFile->m_transform->getCTFVersion();
    OIIO_CHECK_EQUAL(ctfVersion, OCIO::CTF_PROCESS_LIST_VERSION_1_2);
}

OIIO_ADD_TEST(FileFormatCTF, cdl)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("cdl_clamp_fwd.clf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_CHECK_EQUAL(cachedFile->m_transform->getInputDescriptor(),
                     "inputDesc");
    OIIO_CHECK_EQUAL(cachedFile->m_transform->getOutputDescriptor(),
                     "outputDesc");
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::CDLType);
    auto pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pCDL);

    OIIO_CHECK_EQUAL(pCDL->getID(), std::string("look 1"));
    OIIO_CHECK_EQUAL(pCDL->getName(), std::string("cdl"));

    OCIO::OpData::Descriptions descriptions = pCDL->getDescriptions();

    OIIO_REQUIRE_EQUAL(descriptions.size(), 1u);
    OIIO_CHECK_EQUAL(descriptions[0], std::string("ASC CDL operation"));

    OIIO_CHECK_EQUAL(pCDL->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pCDL->getOutputBitDepth(), OCIO::BIT_DEPTH_F16);

    OIIO_CHECK_EQUAL(pCDL->getStyle(), OCIO::CDLOpData::CDL_V1_2_FWD);
    std::string styleName(OCIO::CDLOpData::GetStyleName(pCDL->getStyle()));
    OIIO_CHECK_EQUAL(styleName, "Fwd");

    OIIO_CHECK_ASSERT(pCDL->getSlopeParams() 
                      == OCIO::CDLOpData::ChannelParams(1.35, 1.1, 0.71));
    OIIO_CHECK_ASSERT(pCDL->getOffsetParams()
                      == OCIO::CDLOpData::ChannelParams(0.05, -0.23, 0.11));
    OIIO_CHECK_ASSERT(pCDL->getPowerParams()
                      == OCIO::CDLOpData::ChannelParams(0.93, 0.81, 1.27));
    OIIO_CHECK_EQUAL(pCDL->getSaturation(), 1.239);
}

OIIO_ADD_TEST(FileFormatCTF, cdl_invalid_sop_node)
{
    const std::string ctfFile("cdl_invalidSOP.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception, 
                          "SOPNode: 3 values required");
}
        
OIIO_ADD_TEST(FileFormatCTF, cdl_invalid_sat_node)
{
    const std::string ctfFile("cdl_invalidSat.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception, 
                          "SatNode: non-single value");
}

OIIO_ADD_TEST(FileFormatCTF, cdl_missing_slope)
{
    const std::string ctfFile("cdl_missing_slope.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception, 
                          "Required node 'Slope' is missing");
}

OIIO_ADD_TEST(FileFormatCTF, cdl_missing_offset)
{
    const std::string ctfFile("cdl_missing_offset.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception, 
                          "Required node 'Offset' is missing");
}

OIIO_ADD_TEST(FileFormatCTF, cdl_missing_power)
{
    const std::string ctfFile("cdl_missing_power.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception, 
                          "Required node 'Power' is missing");
}

OIIO_ADD_TEST(FileFormatCTF, cdl_missing_style)
{
    const std::string ctfFile("cdl_missing_style.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception, 
                          "Required attribute 'style' is missing");
}

OIIO_ADD_TEST(FileFormatCTF, cdl_invalid_style)
{
    const std::string ctfFile("cdl_invalid_style.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception, 
                          "Unknown style for CDL");
}

OIIO_ADD_TEST(FileFormatCTF, cdl_no_sop_node)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("cdl_noSOP.clf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::CDLType);
    auto pCDL =
        std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pCDL);

    OIIO_CHECK_ASSERT(pCDL->getSlopeParams()
                      == OCIO::CDLOpData::ChannelParams(1.0));
    OIIO_CHECK_ASSERT(pCDL->getOffsetParams()
                      == OCIO::CDLOpData::ChannelParams(0.0));
    OIIO_CHECK_ASSERT(pCDL->getPowerParams()
                      == OCIO::CDLOpData::ChannelParams(1.0));
    OIIO_CHECK_EQUAL(pCDL->getSaturation(), 1.239);
}

OIIO_ADD_TEST(FileFormatCTF, cdl_no_sat_node)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("cdl_noSat.clf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::CDLType);
    auto pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pCDL);

    OIIO_CHECK_ASSERT(pCDL->getSlopeParams()
                      == OCIO::CDLOpData::ChannelParams(1.35, 1.1, 0.71));
    OIIO_CHECK_ASSERT(pCDL->getOffsetParams()
                      == OCIO::CDLOpData::ChannelParams(0.05, -0.23, 0.11));
    OIIO_CHECK_ASSERT(pCDL->getPowerParams()
                      == OCIO::CDLOpData::ChannelParams(0.93, 0.81, 1.27));
    OIIO_CHECK_EQUAL(pCDL->getSaturation(), 1.0);
}

OIIO_ADD_TEST(FileFormatCTF, cdl_various_in_ctf)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("cdl_various.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 8);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::CDLType);
    auto pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pCDL);
    OIIO_CHECK_EQUAL(pCDL->getStyle(), OCIO::CDLOpData::CDL_V1_2_FWD);

    pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[1]);
    OIIO_REQUIRE_ASSERT(pCDL);
    OIIO_CHECK_EQUAL(pCDL->getStyle(), OCIO::CDLOpData::CDL_V1_2_FWD);

    pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[2]);
    OIIO_REQUIRE_ASSERT(pCDL);
    OIIO_CHECK_EQUAL(pCDL->getStyle(), OCIO::CDLOpData::CDL_V1_2_REV);

    pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[3]);
    OIIO_REQUIRE_ASSERT(pCDL);
    OIIO_CHECK_EQUAL(pCDL->getStyle(), OCIO::CDLOpData::CDL_V1_2_REV);

    pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[4]);
    OIIO_REQUIRE_ASSERT(pCDL);
    OIIO_CHECK_EQUAL(pCDL->getStyle(), OCIO::CDLOpData::CDL_NO_CLAMP_FWD);

    pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[5]);
    OIIO_REQUIRE_ASSERT(pCDL);
    OIIO_CHECK_EQUAL(pCDL->getStyle(), OCIO::CDLOpData::CDL_NO_CLAMP_FWD);

    pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[6]);
    OIIO_REQUIRE_ASSERT(pCDL);
    OIIO_CHECK_EQUAL(pCDL->getStyle(), OCIO::CDLOpData::CDL_NO_CLAMP_REV);

    pCDL = std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[7]);
    OIIO_REQUIRE_ASSERT(pCDL);
    OIIO_CHECK_EQUAL(pCDL->getStyle(), OCIO::CDLOpData::CDL_NO_CLAMP_REV);
}

OIIO_ADD_TEST(FileFormatCTF, lut1d_hue_adjust_invalid_style)
{
    const std::string ctfFile("lut1d_hue_adjust_invalid_style.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception,
                          "Illegal 'hueAdjust' attribute");
}

void checkNames(const OCIO::Metadata::NameList & actualNames,
                const OCIO::Metadata::NameList & expectedNames)
{
    OCIO::Metadata::NameList::const_iterator ait, eit, end;
    for(ait = actualNames.begin(), eit = expectedNames.begin(),
        end = actualNames.end(); ait != end; ++ait, ++eit)
    {
        OIIO_CHECK_EQUAL(*ait, *eit);
    }
}

OIIO_ADD_TEST(FileFormatCTF, metadata)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("metadata.clf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    OIIO_CHECK_EQUAL(cachedFile->m_transform->getInputDescriptor(),
                     "inputDesc");
    OIIO_CHECK_EQUAL(cachedFile->m_transform->getOutputDescriptor(),
                     "outputDesc");

    // Ensure ops were not affected by metadata parsing.
    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 1);

    auto pMatrix =
        std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pMatrix);
    OIIO_CHECK_EQUAL(pMatrix->getName(), "identity");

    OIIO_CHECK_EQUAL(pMatrix->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pMatrix->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);

    const OCIO::Metadata & info = cachedFile->m_transform->getInfo();

    // Check element values.
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

    const OCIO::Metadata::Attributes & atts =
        info["Category"]["Name"].getAttributes();
    OIIO_REQUIRE_EQUAL(atts.size(), 2);
    OIIO_CHECK_EQUAL(atts[0].first, "att1");
    OIIO_CHECK_EQUAL(atts[0].second, "test1");
    OIIO_CHECK_EQUAL(atts[1].first, "att2");
    OIIO_CHECK_EQUAL(atts[1].second, "test2");

    // Check element children count.
    //
    OIIO_REQUIRE_EQUAL(info.getItems().size(), 5u);
    OIIO_CHECK_EQUAL(info["InputColorSpace"].getItems().size(), 3u);
    OIIO_CHECK_EQUAL(info["OutputColorSpace"].getItems().size(), 2u);
    OIIO_CHECK_EQUAL(info["Category"].getItems().size(), 1u);

    // Check element ordering.
    //

    // Info element.
    {
        OCIO::Metadata::NameList expectedNames;
        expectedNames.push_back("Copyright");
        expectedNames.push_back("Release");
        expectedNames.push_back("InputColorSpace");
        expectedNames.push_back("OutputColorSpace");
        expectedNames.push_back("Category");

        checkNames(info.getItemsNames(), expectedNames);
    }

    // InputColorSpace element.
    {
        OCIO::Metadata::NameList expectedNames;
        expectedNames.push_back("Description");
        expectedNames.push_back("Profile");
        expectedNames.push_back("Empty");

        checkNames(info["InputColorSpace"].getItemsNames(), expectedNames);
    }

    // OutputColorSpace element.
    {
        OCIO::Metadata::NameList expectedNames;
        expectedNames.push_back("Description");
        expectedNames.push_back("Profile");

        checkNames(info["OutputColorSpace"].getItemsNames(), expectedNames);
    }

    // Category element.
    {
        OCIO::Metadata::NameList expectedNames;
        expectedNames.push_back("Name");

        checkNames(info["Category"].getItemsNames(), expectedNames);
    }
}

OIIO_ADD_TEST(FileFormatCTF, index_map_1)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("indexMap_test1.clf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 2);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::RangeType);
    auto pR = std::dynamic_pointer_cast<const OCIO::RangeOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pR);

    // Check that the indexMap caused a Range to be inserted.
    OIIO_CHECK_EQUAL(pR->getMinInValue(), 64.5);
    OIIO_CHECK_EQUAL(pR->getMaxInValue(), 940.);
    OIIO_CHECK_EQUAL((int)(pR->getMinOutValue() + 0.5), 132);  // 4*1023/31
    OIIO_CHECK_EQUAL((int)(pR->getMaxOutValue() + 0.5), 1089); // 33*1023/31
    OIIO_CHECK_EQUAL(pR->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    OIIO_CHECK_EQUAL(pR->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

    // Check the LUT is ok.
    auto pL = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[1]);
    OIIO_REQUIRE_ASSERT(pL);
    OIIO_CHECK_EQUAL(pL->getType(), OCIO::OpData::Lut1DType);
    OIIO_CHECK_EQUAL(pL->getArray().getLength(), 32u);
    OIIO_CHECK_EQUAL(pL->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    OIIO_CHECK_EQUAL(pL->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);
}

OIIO_ADD_TEST(FileFormatCTF, index_map_2)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("indexMap_test2.clf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT((bool)cachedFile);

    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 2);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::RangeType);
    auto pR = std::dynamic_pointer_cast<const OCIO::RangeOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pR);
    OIIO_CHECK_EQUAL(pR->getMinInValue(), -0.1f);
    OIIO_CHECK_EQUAL(pR->getMaxInValue(), 19.f);
    OIIO_CHECK_EQUAL(pR->getMinOutValue(), 0.f);
    OIIO_CHECK_EQUAL(pR->getMaxOutValue(), 1.f);
    OIIO_CHECK_EQUAL(pR->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pR->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    // Check the LUT is ok.
    auto pL = std::dynamic_pointer_cast<const OCIO::Lut3DOpData>(opList[1]);
    OIIO_REQUIRE_ASSERT(pL);
    OIIO_CHECK_EQUAL(pL->getType(), OCIO::OpData::Lut3DType);
    OIIO_CHECK_EQUAL(pL->getArray().getLength(), 2u);
    OIIO_CHECK_EQUAL(pL->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pL->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
}

OIIO_ADD_TEST(FileFormatCTF, index_map_3)
{
    const std::string ctfFile("indexMap_test3.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception, 
                          "Only one IndexMap allowed per LUT");
}

OIIO_ADD_TEST(FileFormatCTF, index_map_4)
{
    const std::string ctfFile("indexMap_test4.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception, 
                          "Only two entry IndexMaps are supported");
}

OIIO_ADD_TEST(FileFormatCTF, clf_future_version)
{
    const std::string ctfFile("clf_version_future.clf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception,
                          "Unsupported transform file version");
}

OIIO_ADD_TEST(FileFormatCTF, clf_1)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("multiple_ops.clf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(opList.size(), 6);
    // First one is a CDL
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::CDLType);
    auto cdlOpData =
        std::dynamic_pointer_cast<const OCIO::CDLOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(cdlOpData);
    OIIO_CHECK_EQUAL(cdlOpData->getName(), "");
    OIIO_CHECK_EQUAL(cdlOpData->getID(), "cc01234");
    OIIO_CHECK_EQUAL(cdlOpData->getInputBitDepth(), OCIO::BIT_DEPTH_F16);
    OIIO_CHECK_EQUAL(cdlOpData->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    OIIO_REQUIRE_EQUAL(cdlOpData->getDescriptions().size(), 1);
    OIIO_CHECK_EQUAL(cdlOpData->getDescriptions()[0],
                     "scene 1 exterior look");
    OIIO_CHECK_EQUAL(cdlOpData->getStyle(), OCIO::CDLOpData::CDL_V1_2_REV);
    OIIO_CHECK_ASSERT(cdlOpData->getSlopeParams()
                      == OCIO::CDLOpData::ChannelParams(1., 1., 0.8));
    OIIO_CHECK_ASSERT(cdlOpData->getOffsetParams()
                      == OCIO::CDLOpData::ChannelParams(-0.02, 0., 0.15));
    OIIO_CHECK_ASSERT(cdlOpData->getPowerParams()
                      == OCIO::CDLOpData::ChannelParams(1.05, 1.15, 1.4));
    OIIO_CHECK_EQUAL(cdlOpData->getSaturation(), 0.75);

    // Next one in file is a lut1d, but it has an index map,
    // thus a range was inserted before the LUT.
    OIIO_CHECK_EQUAL(opList[1]->getType(), OCIO::OpData::RangeType);
    auto rangeOpData =
        std::dynamic_pointer_cast<const OCIO::RangeOpData>(opList[1]);
    OIIO_REQUIRE_ASSERT(rangeOpData);
    OIIO_CHECK_EQUAL(rangeOpData->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    OIIO_CHECK_EQUAL(rangeOpData->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    OIIO_CHECK_EQUAL(rangeOpData->getMinInValue(), 64.5);
    OIIO_CHECK_EQUAL(rangeOpData->getMaxInValue(), 940.);
    OIIO_CHECK_EQUAL((int)(rangeOpData->getMinOutValue() + 0.5), 132);  // 4*1023/31
    OIIO_CHECK_EQUAL((int)(rangeOpData->getMaxOutValue() + 0.5), 957); // 29*1023/31

    // Lut1D.
    OIIO_CHECK_EQUAL(opList[2]->getType(), OCIO::OpData::Lut1DType);
    auto l1OpData =
        std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opList[2]);
    OIIO_REQUIRE_ASSERT(l1OpData);
    OIIO_CHECK_EQUAL(l1OpData->getName(), "");
    OIIO_CHECK_EQUAL(l1OpData->getID(), "");
    OIIO_CHECK_EQUAL(l1OpData->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    OIIO_CHECK_EQUAL(l1OpData->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);
    OIIO_CHECK_EQUAL(l1OpData->getDescriptions().size(), 0);
    OIIO_CHECK_EQUAL(l1OpData->getArray().getLength(), 32u);

    // Check that the noClamp style Range became a Matrix.
    OIIO_CHECK_EQUAL(opList[3]->getType(), OCIO::OpData::MatrixType);
    auto matOpData =
        std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opList[3]);
    OIIO_REQUIRE_ASSERT(matOpData);
    OIIO_CHECK_EQUAL(matOpData->getInputBitDepth(), OCIO::BIT_DEPTH_UINT12);
    OIIO_CHECK_EQUAL(matOpData->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

    const OCIO::ArrayDouble & array = matOpData->getArray();
    OIIO_CHECK_EQUAL(array.getLength(), 4u);
    OIIO_CHECK_EQUAL(array.getNumColorComponents(), 4u);
    OIIO_CHECK_EQUAL(array.getNumValues(),
                     array.getLength()*array.getLength());

    const float scalef = (900.f - 20.f) / (3760.f - 256.f);
    const float offsetf = 20.f - scalef * 256.f;
    const float prec = 10000.f;
    const int scale = (int)(prec * scalef);
    const int offset = (int)(prec * offsetf);

    OIIO_REQUIRE_EQUAL(array.getValues().size(), array.getNumValues());
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

    const OCIO::MatrixOpData::Offsets & offsets = matOpData->getOffsets();
    OIIO_CHECK_EQUAL((int)(prec * offsets[0]), offset);
    OIIO_CHECK_EQUAL((int)(prec * offsets[1]), offset);
    OIIO_CHECK_EQUAL((int)(prec * offsets[2]), offset);
    OIIO_CHECK_EQUAL(offsets[3], 0.f);

    // A range with Clamp.
    OIIO_CHECK_EQUAL(opList[4]->getType(), OCIO::OpData::RangeType);
    rangeOpData =
        std::dynamic_pointer_cast<const OCIO::RangeOpData>(opList[4]);
    OIIO_REQUIRE_ASSERT(rangeOpData);
    OIIO_CHECK_EQUAL(rangeOpData->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    OIIO_CHECK_EQUAL(rangeOpData->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

    // A range without style defaults to clamp.
    OIIO_CHECK_EQUAL(opList[5]->getType(), OCIO::OpData::RangeType);
    rangeOpData =
        std::dynamic_pointer_cast<const OCIO::RangeOpData>(opList[5]);
    OIIO_REQUIRE_ASSERT(rangeOpData);
    OIIO_CHECK_EQUAL(rangeOpData->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    OIIO_CHECK_EQUAL(rangeOpData->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
}

OIIO_ADD_TEST(FileFormatCTF, tabluation_support)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    // This clf file contains tabulations used as delimiters for a
    // series of numbers.
    const std::string ctfFile("tabulation_support.clf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_CHECK_EQUAL(cachedFile->m_transform->getID(), "none");
    OIIO_REQUIRE_EQUAL(opList.size(), 1);

    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::Lut3DType);

    auto pL = std::dynamic_pointer_cast<const OCIO::Lut3DOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pL);

    OIIO_CHECK_EQUAL(pL->getInputBitDepth(), OCIO::BIT_DEPTH_UINT12);
    OIIO_CHECK_EQUAL(pL->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);

    const OCIO::Array & array = pL->getArray();
    OIIO_CHECK_EQUAL(array.getLength(), 33U);
    OIIO_CHECK_EQUAL(array.getNumColorComponents(), 3U);
    OIIO_CHECK_EQUAL(array.getNumValues(), 107811U);
    OIIO_REQUIRE_EQUAL(array.getValues().size(), 107811U);

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

OIIO_ADD_TEST(FileFormatCTF, matrix_windows_eol)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    // This file uses windows end of line character and does not start
    // with the ?xml header.
    const std::string ctfFile("matrix_windows.clf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();
    OIIO_CHECK_EQUAL(cachedFile->m_transform->getID(), "42");
    OIIO_REQUIRE_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::MatrixType);
    OIIO_CHECK_EQUAL(opList[0]->getID(), "mat42");
    OIIO_CHECK_EQUAL(opList[0]->getInputBitDepth(), OCIO::BIT_DEPTH_F16);
    OIIO_CHECK_EQUAL(opList[0]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);
}

OIIO_ADD_TEST(FileFormatCTF, lut_3d_file_with_xml_extension)
{
    const std::string ctfFile("not_a_ctf.xml");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ctfFile), OCIO::Exception,
                          "is not a CTF/CLF file.");
}


OIIO_ADD_TEST(FileFormatCTF, info_element_version_test)
{
    // VALID - No Version.
    //
    {
        const std::string ctfFile("info_version_without.ctf");
        OIIO_CHECK_NO_THROW(LoadCLFFile(ctfFile));
    }

    // VALID - Minor Version.
    //
    {
        const std::string ctfFile("info_version_valid_minor.ctf");
        OIIO_CHECK_NO_THROW(LoadCLFFile(ctfFile));
    }

    // INVALID - Invalid Version.
    //
    {
        const std::string ctfFile("info_version_invalid.ctf");
        OIIO_CHECK_THROW_WHAT(
            LoadCLFFile(ctfFile), OCIO::Exception,
                        "Invalid Info element version attribute");
    }

    // INVALID - Unsupported Version.
    //
    {
        const std::string ctfFile("info_version_unsupported.ctf");
        OIIO_CHECK_THROW_WHAT(
            LoadCLFFile(ctfFile), OCIO::Exception,
            "Unsupported Info element version attribute");
    }

    // INVALID - Empty Version.
    //
    {
        const std::string ctfFile("info_version_empty.ctf");
        OIIO_CHECK_THROW_WHAT(
            LoadCLFFile(ctfFile), OCIO::Exception,
            "Invalid Info element version attribute");
    }
    
}

OIIO_ADD_TEST(Log, load_log10)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    std::string fileName("log_log10.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::OpDataVec & fileOps = cachedFile->m_transform->getOps();
    
    OIIO_CHECK_EQUAL(cachedFile->m_transform->getName(), "log example");
    OIIO_CHECK_EQUAL(cachedFile->m_transform->getID(),
                     "b5cc7aed-d405-4d8b-b64b-382b2341a378");
    OIIO_CHECK_EQUAL(cachedFile->m_transform->getInputDescriptor(), "inputDesc");
    OIIO_CHECK_EQUAL(cachedFile->m_transform->getOutputDescriptor(), "outputDesc");
    OIIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions().size(), 1);
    OIIO_CHECK_EQUAL(cachedFile->m_transform->getDescriptions()[0],
                     "Example of Log10 logarithm operation.");

    OIIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::OpDataRcPtr op = fileOps[0];
    OCIO::LogOpDataRcPtr log = std::dynamic_pointer_cast<OCIO::LogOpData>(op);
    OIIO_REQUIRE_ASSERT(log);
    OIIO_REQUIRE_EQUAL(log->getDescriptions().size(), 1);
    OIIO_CHECK_EQUAL(log->getDescriptions()[0],
                     "Log10 logarithm operation");

    OIIO_CHECK_EQUAL(log->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(log->getOutputBitDepth(), OCIO::BIT_DEPTH_F16);

    OIIO_CHECK_ASSERT(log->isLog10());
    OIIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
}

OIIO_ADD_TEST(Log, load_log2)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    std::string fileName("log_log2.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::OpDataVec & fileOps = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::OpDataRcPtr op = fileOps[0];
    OCIO::LogOpDataRcPtr log = std::dynamic_pointer_cast<OCIO::LogOpData>(op);
    OIIO_REQUIRE_ASSERT(log);

    OIIO_CHECK_EQUAL(log->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(log->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    OIIO_CHECK_ASSERT(log->isLog2());
    OIIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
}

OIIO_ADD_TEST(Log, load_antilog10)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    std::string fileName("log_antilog10.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::OpDataVec & fileOps = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::OpDataRcPtr op = fileOps[0];
    OCIO::LogOpDataRcPtr log = std::dynamic_pointer_cast<OCIO::LogOpData>(op);
    OIIO_REQUIRE_ASSERT(log);

    OIIO_CHECK_EQUAL(log->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    OIIO_CHECK_EQUAL(log->getOutputBitDepth(), OCIO::BIT_DEPTH_F16);

    OIIO_CHECK_ASSERT(log->isLog10());
    OIIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
}

OIIO_ADD_TEST(Log, load_antilog2)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    std::string fileName("log_antilog2.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::OpDataVec & fileOps = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::OpDataRcPtr op = fileOps[0];
    OCIO::LogOpDataRcPtr log = std::dynamic_pointer_cast<OCIO::LogOpData>(op);
    OIIO_REQUIRE_ASSERT(log);

    OIIO_CHECK_EQUAL(log->getInputBitDepth(), OCIO::BIT_DEPTH_F16);
    OIIO_CHECK_EQUAL(log->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);

    OIIO_CHECK_ASSERT(log->isLog2());
    OIIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
}

OIIO_ADD_TEST(Log, load_log_to_lin)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    std::string fileName("log_logtolin.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::OpDataVec & fileOps = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::OpDataRcPtr op = fileOps[0];
    OCIO::LogOpDataRcPtr log = std::dynamic_pointer_cast<OCIO::LogOpData>(op);
    OIIO_REQUIRE_ASSERT(log);

    OIIO_CHECK_EQUAL(log->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(log->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    OIIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_ASSERT(!log->isLog2());
    OIIO_CHECK_ASSERT(!log->isLog10());
    OIIO_CHECK_ASSERT(log->allComponentsEqual());
    auto & param = log->getRedParams();
    OIIO_REQUIRE_EQUAL(param.size(), 4);
    double error = 1e-9;
    OIIO_CHECK_CLOSE(param[OCIO::LOG_SIDE_SLOPE],  0.29325513196, error);
    OIIO_CHECK_CLOSE(param[OCIO::LOG_SIDE_OFFSET], 0.66959921799, error);
    OIIO_CHECK_CLOSE(param[OCIO::LIN_SIDE_SLOPE],  0.98969709693, error);
    OIIO_CHECK_CLOSE(param[OCIO::LIN_SIDE_OFFSET], 0.01030290307, error);
}

OIIO_ADD_TEST(Log, load_lin_to_log)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    std::string fileName("log_lintolog_3chan.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::OpDataVec & fileOps = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::OpDataRcPtr op = fileOps[0];
    OCIO::LogOpDataRcPtr log = std::dynamic_pointer_cast<OCIO::LogOpData>(op);
    OIIO_REQUIRE_ASSERT(log);

    OIIO_CHECK_EQUAL(log->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(log->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    OIIO_CHECK_EQUAL(log->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_ASSERT(!log->allComponentsEqual());

    auto & rParam = log->getRedParams();
    OIIO_REQUIRE_EQUAL(rParam.size(), 4);
    double error = 1e-9;
    OIIO_CHECK_CLOSE(rParam[OCIO::LOG_SIDE_SLOPE],   0.244379276637, error);
    OIIO_CHECK_CLOSE(rParam[OCIO::LOG_SIDE_OFFSET],  0.665689149560, error);
    OIIO_CHECK_CLOSE(rParam[OCIO::LIN_SIDE_SLOPE],   1.111637101285, error);
    OIIO_CHECK_CLOSE(rParam[OCIO::LIN_SIDE_OFFSET], -0.000473391157, error);

    auto & gParam = log->getGreenParams();
    OIIO_REQUIRE_EQUAL(gParam.size(), 4);
    OIIO_CHECK_CLOSE(gParam[OCIO::LOG_SIDE_SLOPE],  0.293255131964, error);
    OIIO_CHECK_CLOSE(gParam[OCIO::LOG_SIDE_OFFSET], 0.666666666667, error);
    OIIO_CHECK_CLOSE(gParam[OCIO::LIN_SIDE_SLOPE],  0.991514003046, error);
    OIIO_CHECK_CLOSE(gParam[OCIO::LIN_SIDE_OFFSET], 0.008485996954, error);

    auto & bParam = log->getBlueParams();
    OIIO_REQUIRE_EQUAL(bParam.size(), 4);
    OIIO_CHECK_CLOSE(bParam[OCIO::LOG_SIDE_SLOPE],  0.317693059628, error);
    OIIO_CHECK_CLOSE(bParam[OCIO::LOG_SIDE_OFFSET], 0.667644183773, error);
    OIIO_CHECK_CLOSE(bParam[OCIO::LIN_SIDE_SLOPE],  1.236287104632, error);
    OIIO_CHECK_CLOSE(bParam[OCIO::LIN_SIDE_OFFSET], 0.010970316295, error);
}

OIIO_ADD_TEST(Log, load_invalid_style)
{
    std::string fileName("log_invalidstyle.ctf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(fileName), OCIO::Exception,
                          "is invalid");
}

OIIO_ADD_TEST(Log, load_faulty_version)
{
    std::string fileName("log_log10_faulty_version.ctf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(fileName), OCIO::Exception,
                          "Unsupported transform file version");
}

//
// NOTE: These tests are on the ReferenceOpData itself, before it gets replaced
// with the ops from the file it is referencing.  Please see RefereceOpData.cpp
// for tests involving the resolved ops.
//
OIIO_ADD_TEST(Reference, load_alias)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    std::string fileName("reference_alias.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::OpDataVec & fileOps = cachedFile->m_transform->getOps();

    OIIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::OpDataRcPtr op = fileOps[0];
    OCIO::ReferenceOpDataRcPtr ref = std::dynamic_pointer_cast<OCIO::ReferenceOpData>(op);
    OIIO_REQUIRE_ASSERT(ref);
    OIIO_CHECK_EQUAL(ref->getName(), "name");
    OIIO_CHECK_EQUAL(ref->getID(), "uuid");
    OIIO_CHECK_EQUAL(ref->getInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ref->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ref->getReferenceStyle(), OCIO::REF_ALIAS);
    OIIO_CHECK_EQUAL(ref->getPath(), "");
    OIIO_CHECK_EQUAL(ref->getAlias(), "alias");
    OIIO_CHECK_EQUAL(ref->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
}

OIIO_ADD_TEST(Reference, load_path)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    std::string fileName("reference_path_missing_file.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::OpDataVec & fileOps = cachedFile->m_transform->getOps();
    
    OIIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::OpDataRcPtr op = fileOps[0];
    OCIO::ReferenceOpDataRcPtr ref = std::dynamic_pointer_cast<OCIO::ReferenceOpData>(op);
    OIIO_REQUIRE_ASSERT(ref);
    OIIO_CHECK_EQUAL(ref->getReferenceStyle(), OCIO::REF_PATH);
    OIIO_CHECK_EQUAL(ref->getPath(), "toto/toto.ctf");
    OIIO_CHECK_EQUAL(ref->getAlias(), "");
    OIIO_CHECK_EQUAL(ref->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
}

OIIO_ADD_TEST(Reference, load_multiple)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    // File contains 2 references, 1 range and 1 reference.
    std::string fileName("references_some_inverted.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::OpDataVec & fileOps = cachedFile->m_transform->getOps();
    
    OIIO_REQUIRE_EQUAL(fileOps.size(), 4);
    OCIO::OpDataRcPtr op0 = fileOps[0];
    OCIO::ReferenceOpDataRcPtr ref0 = std::dynamic_pointer_cast<OCIO::ReferenceOpData>(op0);
    OIIO_REQUIRE_ASSERT(ref0);
    OIIO_CHECK_EQUAL(ref0->getReferenceStyle(), OCIO::REF_PATH);
    OIIO_CHECK_EQUAL(ref0->getPath(), "matrix_example.clf");
    OIIO_CHECK_EQUAL(ref0->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(ref0->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);
    OIIO_CHECK_EQUAL(ref0->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    
    OCIO::OpDataRcPtr op1 = fileOps[1];
    OCIO::ReferenceOpDataRcPtr ref1 = std::dynamic_pointer_cast<OCIO::ReferenceOpData>(op1);
    OIIO_REQUIRE_ASSERT(ref1);
    OIIO_CHECK_EQUAL(ref1->getReferenceStyle(), OCIO::REF_PATH);
    OIIO_CHECK_EQUAL(ref1->getPath(), "xyz_to_rgb.clf");
    OIIO_CHECK_EQUAL(ref1->getInputBitDepth(), OCIO::BIT_DEPTH_UINT12);
    OIIO_CHECK_EQUAL(ref1->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ref1->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::OpDataRcPtr op2 = fileOps[2];
    OCIO::RangeOpDataRcPtr range2 = std::dynamic_pointer_cast<OCIO::RangeOpData>(op2);
    OIIO_REQUIRE_ASSERT(range2);

    OCIO::OpDataRcPtr op3 = fileOps[3];
    OCIO::ReferenceOpDataRcPtr ref3 = std::dynamic_pointer_cast<OCIO::ReferenceOpData>(op3);
    OIIO_REQUIRE_ASSERT(ref3);
    OIIO_CHECK_EQUAL(ref3->getReferenceStyle(), OCIO::REF_PATH);
    OIIO_CHECK_EQUAL(ref3->getPath(), "cdl_clamp_fwd.clf");
    OIIO_CHECK_EQUAL(ref3->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(ref3->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);
    // Note: This tests that the "inverted" attribute set to anything other than
    // true does not result in an inverted transform.
    OIIO_CHECK_EQUAL(ref3->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
}

OIIO_ADD_TEST(Reference, load_path_utf8)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    std::string fileName("reference_utf8.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::OpDataVec & fileOps = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::OpDataRcPtr op = fileOps[0];
    OCIO::ReferenceOpDataRcPtr ref = std::dynamic_pointer_cast<OCIO::ReferenceOpData>(op);
    OIIO_REQUIRE_ASSERT(ref);
    OIIO_CHECK_EQUAL(ref->getReferenceStyle(), OCIO::REF_PATH);
    OIIO_CHECK_EQUAL(ref->getPath(), "\xE6\xA8\x99\xE6\xBA\x96\xE8\x90\xAC\xE5\x9C\x8B\xE7\xA2\xBC");
    OIIO_CHECK_EQUAL(ref->getAlias(), "");
}

OIIO_ADD_TEST(Reference, load_alias_path)
{
    std::string fileName("reference_alias_path.ctf");
    // Can't have alias and path at the same time.
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(fileName), OCIO::Exception,
                          "alias & path attributes for Reference should not be both defined");
}

OIIO_ADD_TEST(FileFormatCTF, exposure_contrast_video)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("exposure_contrast_video.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT(cachedFile);
    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();

    OIIO_REQUIRE_EQUAL(opList.size(), 2);

    OIIO_REQUIRE_ASSERT(opList[0]);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::ExposureContrastType);

    auto pEC = std::dynamic_pointer_cast<const OCIO::ExposureContrastOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pEC);

    OIIO_CHECK_EQUAL(pEC->getInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(pEC->getOutputBitDepth(), OCIO::BIT_DEPTH_F16);

    OIIO_CHECK_EQUAL(pEC->getStyle(), OCIO::ExposureContrastOpData::STYLE_VIDEO);

    OIIO_CHECK_EQUAL(pEC->getExposure(), -1.0);
    OIIO_CHECK_EQUAL(pEC->getContrast(), 1.5);
    OIIO_CHECK_EQUAL(pEC->getPivot(), 0.5);

    OIIO_CHECK_ASSERT(pEC->isDynamic());
    OIIO_CHECK_ASSERT(pEC->getExposureProperty()->isDynamic());
    OIIO_CHECK_ASSERT(pEC->getContrastProperty()->isDynamic());
    OIIO_CHECK_ASSERT(!pEC->getGammaProperty()->isDynamic());

    OIIO_REQUIRE_ASSERT(opList[1]);
    OIIO_CHECK_EQUAL(opList[1]->getType(), OCIO::OpData::ExposureContrastType);

    auto pECRev = std::dynamic_pointer_cast<const OCIO::ExposureContrastOpData>(opList[1]);
    OIIO_REQUIRE_ASSERT(pECRev);
    OIIO_CHECK_ASSERT(!pECRev->isDynamic());

    OIIO_CHECK_EQUAL(pECRev->getStyle(), OCIO::ExposureContrastOpData::STYLE_VIDEO_REV);
}

OIIO_ADD_TEST(FileFormatCTF, exposure_contrast_log)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("exposure_contrast_log.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT(cachedFile);
    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();

    OIIO_REQUIRE_EQUAL(opList.size(), 2);

    OIIO_REQUIRE_ASSERT(opList[0]);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::ExposureContrastType);

    auto pEC = std::dynamic_pointer_cast<const OCIO::ExposureContrastOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pEC);

    OIIO_CHECK_EQUAL(pEC->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(pEC->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    OIIO_CHECK_EQUAL(pEC->getStyle(), OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC);

    OIIO_CHECK_EQUAL(pEC->getExposure(), -1.5);
    OIIO_CHECK_EQUAL(pEC->getContrast(), 0.5);
    OIIO_CHECK_EQUAL(pEC->getGamma(), 1.2);
    OIIO_CHECK_EQUAL(pEC->getPivot(), 0.18);

    OIIO_CHECK_ASSERT(pEC->isDynamic());
    OIIO_CHECK_ASSERT(pEC->getExposureProperty()->isDynamic());
    OIIO_CHECK_ASSERT(pEC->getContrastProperty()->isDynamic());
    OIIO_CHECK_ASSERT(pEC->getGammaProperty()->isDynamic());

    OIIO_REQUIRE_ASSERT(opList[1]);
    OIIO_CHECK_EQUAL(opList[1]->getType(), OCIO::OpData::ExposureContrastType);

    auto pECRev = std::dynamic_pointer_cast<const OCIO::ExposureContrastOpData>(opList[1]);
    OIIO_REQUIRE_ASSERT(pECRev);

    OIIO_CHECK_EQUAL(pECRev->getStyle(), OCIO::ExposureContrastOpData::STYLE_LOGARITHMIC_REV);
    OIIO_CHECK_ASSERT(pECRev->isDynamic());
    OIIO_CHECK_ASSERT(pECRev->getExposureProperty()->isDynamic());
    OIIO_CHECK_ASSERT(!pECRev->getContrastProperty()->isDynamic());
    OIIO_CHECK_ASSERT(!pECRev->getGammaProperty()->isDynamic());
}

OIIO_ADD_TEST(FileFormatCTF, exposure_contrast_linear)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("exposure_contrast_linear.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT(cachedFile);
    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();

    OIIO_REQUIRE_EQUAL(opList.size(), 2);

    OIIO_REQUIRE_ASSERT(opList[0]);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::ExposureContrastType);

    auto pEC = std::dynamic_pointer_cast<const OCIO::ExposureContrastOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pEC);

    OIIO_CHECK_EQUAL(pEC->getInputBitDepth(), OCIO::BIT_DEPTH_F16);
    OIIO_CHECK_EQUAL(pEC->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    OIIO_CHECK_EQUAL(pEC->getStyle(), OCIO::ExposureContrastOpData::STYLE_LINEAR);

    OIIO_CHECK_EQUAL(pEC->getExposure(), 0.65);
    OIIO_CHECK_EQUAL(pEC->getContrast(), 1.2);
    OIIO_CHECK_EQUAL(pEC->getGamma(), 0.5);
    OIIO_CHECK_EQUAL(pEC->getPivot(), 1.0);

    OIIO_CHECK_ASSERT(pEC->isDynamic());
    OIIO_CHECK_ASSERT(pEC->getExposureProperty()->isDynamic());
    OIIO_CHECK_ASSERT(pEC->getContrastProperty()->isDynamic());
    OIIO_CHECK_ASSERT(pEC->getGammaProperty()->isDynamic());

    OIIO_REQUIRE_ASSERT(opList[1]);
    OIIO_CHECK_EQUAL(opList[1]->getType(), OCIO::OpData::ExposureContrastType);

    auto pECRev = std::dynamic_pointer_cast<const OCIO::ExposureContrastOpData>(opList[1]);
    OIIO_REQUIRE_ASSERT(pECRev);

    OIIO_CHECK_EQUAL(pECRev->getStyle(), OCIO::ExposureContrastOpData::STYLE_LINEAR_REV);
    OIIO_CHECK_ASSERT(!pECRev->isDynamic());
    OIIO_CHECK_ASSERT(!pECRev->getExposureProperty()->isDynamic());
    OIIO_CHECK_ASSERT(!pECRev->getContrastProperty()->isDynamic());
    OIIO_CHECK_ASSERT(!pECRev->getGammaProperty()->isDynamic());
}

OIIO_ADD_TEST(FileFormatCTF, exposure_contrast_no_gamma) 
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string ctfFile("exposure_contrast_no_gamma.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(ctfFile));
    OIIO_REQUIRE_ASSERT(cachedFile);
    const OCIO::OpDataVec & opList = cachedFile->m_transform->getOps();

    OIIO_REQUIRE_EQUAL(opList.size(), 1);

    OIIO_REQUIRE_ASSERT(opList[0]);
    OIIO_CHECK_EQUAL(opList[0]->getType(), OCIO::OpData::ExposureContrastType);

    auto pEC = std::dynamic_pointer_cast<const OCIO::ExposureContrastOpData>(opList[0]);
    OIIO_REQUIRE_ASSERT(pEC);

    OIIO_CHECK_EQUAL(pEC->getInputBitDepth(), OCIO::BIT_DEPTH_F16);
    OIIO_CHECK_EQUAL(pEC->getOutputBitDepth(), OCIO::BIT_DEPTH_F16);

    OIIO_CHECK_EQUAL(pEC->getStyle(), OCIO::ExposureContrastOpData::STYLE_VIDEO);

    OIIO_CHECK_EQUAL(pEC->getExposure(), 0.2);
    OIIO_CHECK_EQUAL(pEC->getContrast(), 0.65);
    OIIO_CHECK_EQUAL(pEC->getPivot(), 0.23);

    OIIO_CHECK_EQUAL(pEC->getGamma(), 1.0);

    OIIO_CHECK_ASSERT(!pEC->isDynamic());
    OIIO_CHECK_ASSERT(!pEC->getExposureProperty()->isDynamic());
    OIIO_CHECK_ASSERT(!pEC->getContrastProperty()->isDynamic());
    OIIO_CHECK_ASSERT(!pEC->getGammaProperty()->isDynamic());
}

OIIO_ADD_TEST(FileFormatCTF, exposure_contrast_failures)
{
    const std::string ecBadStyle("exposure_contrast_bad_style.ctf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ecBadStyle), OCIO::Exception,
                          "Unknown exposure contrast style");

    const std::string ecMissingParam("exposure_contrast_missing_param.ctf");
    OIIO_CHECK_THROW_WHAT(LoadCLFFile(ecMissingParam), OCIO::Exception,
                          "exposure missing");
}

OIIO_ADD_TEST(FixedFunction, load_ff_aces_redmod)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    std::string fileName("ff_aces_redmod.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::OpDataVec & fileOps = cachedFile->m_transform->getOps();
    OIIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::OpDataRcPtr op = fileOps[0];
    OCIO::FixedFunctionOpDataRcPtr func = std::dynamic_pointer_cast<OCIO::FixedFunctionOpData>(op);
    OIIO_REQUIRE_ASSERT(func);

    OIIO_CHECK_EQUAL(func->getInputBitDepth(), OCIO::BIT_DEPTH_UINT16);
    OIIO_CHECK_EQUAL(func->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    OIIO_CHECK_EQUAL(func->getStyle(), OCIO::FixedFunctionOpData::ACES_RED_MOD_03_INV);
}

OIIO_ADD_TEST(FixedFunction, load_ff_aces_surround)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    std::string fileName("ff_aces_surround.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::OpDataVec & fileOps = cachedFile->m_transform->getOps();

    OIIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::OpDataRcPtr op = fileOps[0];
    OCIO::FixedFunctionOpDataRcPtr func = std::dynamic_pointer_cast<OCIO::FixedFunctionOpData>(op);
    OIIO_REQUIRE_ASSERT(func);

    OIIO_CHECK_EQUAL(func->getInputBitDepth(), OCIO::BIT_DEPTH_UINT16);
    OIIO_CHECK_EQUAL(func->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    OIIO_CHECK_EQUAL(func->getStyle(), OCIO::FixedFunctionOpData::REC2100_SURROUND);

    OCIO::FixedFunctionOpData::Params params;
    params.push_back(1.2);
    func->validate();
    OIIO_CHECK_ASSERT(func->getParams() == params);
}

void ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::Style style)
{
    std::string styleName{ OCIO::FixedFunctionOpData::ConvertStyleToString(style, false) };
    std::ostringstream strebuf;
    strebuf << "<?xml version='1.0' encoding='UTF-8'?>\n";
    strebuf << "<ProcessList id='none' version='2'>\n";
    strebuf << "    <FixedFunction inBitDepth='8i' outBitDepth='32f' style='";
    strebuf << styleName;
    strebuf << "' />\n";
    strebuf << "</ProcessList>\n";

    std::istringstream ctf;
    ctf.str(strebuf.str());

    // Load file
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr file;
    OIIO_CHECK_NO_THROW(file = tester.Read(ctf, styleName));
    OCIO::LocalCachedFileRcPtr cachedFile = OCIO_DYNAMIC_POINTER_CAST<OCIO::LocalCachedFile>(file);
    const OCIO::OpDataVec & fileOps = cachedFile->m_transform->getOps();

    OIIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::OpDataRcPtr op = fileOps[0];
    OCIO::FixedFunctionOpDataRcPtr func = std::dynamic_pointer_cast<OCIO::FixedFunctionOpData>(op);
    OIIO_REQUIRE_ASSERT(func);

    OIIO_CHECK_EQUAL(func->getStyle(), style);
}

OIIO_ADD_TEST(FixedFunction, load_ff_style)
{
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::ACES_RED_MOD_03_FWD);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::ACES_RED_MOD_03_INV);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::ACES_RED_MOD_10_FWD);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::ACES_RED_MOD_10_INV);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::ACES_GLOW_03_FWD);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::ACES_GLOW_03_INV);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::ACES_GLOW_10_FWD);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::ACES_GLOW_10_INV);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD);
    ValidateFixedFunctionStyleNoParam(OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_INV);
}

OIIO_ADD_TEST(FixedFunction, load_ff_surround)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    std::string fileName("ff_surround.ctf");
    OIIO_CHECK_NO_THROW(cachedFile = LoadCLFFile(fileName));
    const OCIO::OpDataVec & fileOps = cachedFile->m_transform->getOps();

    OIIO_REQUIRE_EQUAL(fileOps.size(), 1);
    OCIO::OpDataRcPtr op = fileOps[0];
    OCIO::FixedFunctionOpDataRcPtr func = std::dynamic_pointer_cast<OCIO::FixedFunctionOpData>(op);
    OIIO_REQUIRE_ASSERT(func);

    OIIO_CHECK_EQUAL(func->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(func->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    OIIO_CHECK_EQUAL(func->getStyle(), OCIO::FixedFunctionOpData::REC2100_SURROUND);

    OCIO::FixedFunctionOpData::Params params;
    params.push_back(0.8);
    func->validate();
    OIIO_CHECK_ASSERT(func->getParams() == params);
}

OIIO_ADD_TEST(FixedFunction, load_ff_fail_version)
{
    std::ostringstream strebuf;
    strebuf << "<?xml version='1.0' encoding='UTF-8'?>\n";
    strebuf << "<ProcessList id='none' version='1.5'>\n";
    strebuf << "    <FixedFunction inBitDepth='8i' outBitDepth='32f' ";
    strebuf << "params = '0.8' ";
    strebuf << "style = 'Rec2100Surround' />\n";
    strebuf << "</ProcessList>\n";

    std::istringstream ctf;
    ctf.str(strebuf.str());

    // Load file
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OIIO_CHECK_THROW_WHAT(tester.Read(ctf, emptyString), OCIO::Exception,
                          "Unsupported transform file version");
}

OIIO_ADD_TEST(FixedFunction, load_ff_fail_params)
{
    std::ostringstream strebuf;
    strebuf << "<?xml version='1.0' encoding='UTF-8'?>\n";
    strebuf << "<ProcessList id='none' version='2'>\n";
    strebuf << "    <FixedFunction inBitDepth='8i' outBitDepth='32f' ";
    strebuf << "params = '0.8 2.0' ";
    strebuf << "style = 'Rec2100Surround' />\n";
    strebuf << "</ProcessList>\n";

    std::istringstream ctf;
    ctf.str(strebuf.str());

    // Load file
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OIIO_CHECK_THROW_WHAT(tester.Read(ctf, emptyString), OCIO::Exception,
                          "must have one parameter but 2 found");
}

OIIO_ADD_TEST(FixedFunction, load_ff_aces_fail_style)
{
    std::ostringstream strebuf;
    strebuf << "<?xml version='1.0' encoding='UTF-8'?>\n";
    strebuf << "<ProcessList id='none' version='1.5'>\n";
    strebuf << "    <ACES inBitDepth='16i' outBitDepth='32f' style='UnknownStyle' />\n";
    strebuf << "</ProcessList>\n";

    std::istringstream ctf;
    ctf.str(strebuf.str());

    // Load file
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OIIO_CHECK_THROW_WHAT(tester.Read(ctf, emptyString), OCIO::Exception,
                          "Unknown FixedFunction style");
}

OIIO_ADD_TEST(FixedFunction, load_ff_aces_fail_gamma_param)
{
    std::ostringstream strebuf;
    strebuf << "<?xml version='1.0' encoding='UTF-8'?>\n";
    strebuf << "<ProcessList id='none' version='1.5'>\n";
    strebuf << "    <ACES inBitDepth='16i' outBitDepth='32f' style='Surround'>\n";
    strebuf << "        <ACESParams wrongParam='1.2' />\n";
    strebuf << "    </ACES>\n";
    strebuf << "</ProcessList>\n";

    std::istringstream ctf;
    ctf.str(strebuf.str());

    // Load file
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OIIO_CHECK_THROW_WHAT(tester.Read(ctf, emptyString), OCIO::Exception,
                          "Missing required parameter");
}

OIIO_ADD_TEST(FixedFunction, load_ff_aces_fail_gamma_twice)
{
    std::ostringstream strebuf;
    strebuf << "<?xml version='1.0' encoding='UTF-8'?>\n";
    strebuf << "<ProcessList id='none' version='1.5'>\n";
    strebuf << "    <ACES inBitDepth='16i' outBitDepth='32f' style='Surround'>\n";
    strebuf << "        <ACESParams gamma='1.2' />\n";
    strebuf << "        <ACESParams gamma='1.4' />\n";
    strebuf << "    </ACES>\n";
    strebuf << "</ProcessList>\n";

    std::istringstream ctf;
    ctf.str(strebuf.str());

    // Load file
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OIIO_CHECK_THROW_WHAT(tester.Read(ctf, emptyString), OCIO::Exception,
                          "only 1 gamma parameter");
}

OIIO_ADD_TEST(FixedFunction, load_ff_aces_fail_missing_param)
{
    std::ostringstream strebuf;
    strebuf << "<?xml version='1.0' encoding='UTF-8'?>\n";
    strebuf << "<ProcessList id='none' version='1.5'>\n";
    strebuf << "    <ACES inBitDepth='16i' outBitDepth='32f' style='Surround'>\n";
    strebuf << "    </ACES>\n";
    strebuf << "</ProcessList>\n";

    std::istringstream ctf;
    ctf.str(strebuf.str());

    // Load file
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OIIO_CHECK_THROW_WHAT(tester.Read(ctf, emptyString), OCIO::Exception,
                          "must have one parameter");
}

// TODO: Bring over tests when adding CTF support.
// synColor: xmlTransformReader_test.cpp

// checkDither
// look_test
// look_test_true
// checkFunction
// checkGamutMap
// checkHueVector
// checkPrimaryLog
// checkPrimaryLin
// checkPrimaryVideo
// checkPrimary_invalidAttr
// checkPrimary_missingStyle
// checkPrimary_styleMismatch
// checkPrimary_invalidGammaValue
// checkPrimary_missing_attribute
// checkPrimary_wrong_attribute
// checkTone
// checkTone_hightlights_only
// checkTone_invalid_attribute_value
// checkRGBCurve
// checkRGBSingleCurve
// checkHUECurve
// checkRGBCurve_decreasingCtrlPnts
// checkRGBCurve_mismatch
// checkRGBCurve_empty
// checkRGBCurve_missing_type
// checkRGBCurve_invalid_ctrl_pnts
// checkRGBCurve_missing_curvelist

#endif // OCIO_UNIT_TEST

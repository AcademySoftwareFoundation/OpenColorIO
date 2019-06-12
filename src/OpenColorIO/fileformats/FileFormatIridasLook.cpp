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

#include <cstdio>
#include <cstring>
#include <iterator>

#include <OpenColorIO/OpenColorIO.h>

#include "expat/expat.h"
#include "ops/Lut1D/Lut1DOp.h"
#include "ops/Lut3D/Lut3DOp.h"
#include "ParseUtils.h"
#include "Platform.h"
#include "pystring/pystring.h"
#include "transforms/FileTransform.h"

/*

Iridas .look format

An XML format containing <shaders>, a series of layers describing the
operations and their parameters (irrelevant to us in this context).

This series of shaders is baked into the <LUT> section.

<?xml version="1.0" ?>
<look>
  <shaders>
    # anything in here is useless to us
  </shaders>
  <LUT>
    <size>"8"</size> # Size of 3D LUT
    <data>"
      0000008000000080000000802CF52E3D2DF52E3D2DF52E3D2CF5AE3D2DF5AE3D
      # ...cut...
      5A216A3F5A216A3FAD10753FAD10753FAD10753F0000803F0000803F0000803F"
    </data>
  </LUT>
</look>

The LUT data contains a 3D LUT, as a hex-encoded series of 32-bit
floats, with little-endian bit-ordering. LUT value ordering is
LUT3DORDER_FAST_RED (red index incrementing fastest, then green, then
blue)

The LUT data is parsed by removing all whitespace and quotes. Taking 8
characters at a time and intepreting as little-endian float, as follows:

Given the string "0000003F0000803FAD10753F":

    >>> import binascii, struct
    >>> struct.unpack("<f", binascii.unhexlify("0000003F"))[0]
    0.5
    >>> struct.unpack("<f", binascii.unhexlify("0000803F"))[0]
    1.0
    >>> struct.unpack("<f", binascii.unhexlify("AD10753F"))[0]
    0.9572857022285461

 */


OCIO_NAMESPACE_ENTER
{
    namespace
    {
        // convert hex ascii to int
        // return true on success, false on failure
        bool hexasciitoint(char& ival, char character)
        {
            if(character>=48 && character<=57) // [0-9]
            {
                ival = static_cast<char>(character-48);
                return true;
            }
            else if(character>=65 && character<=70) // [A-F]
            {
                ival = static_cast<char>(10+character-65);
                return true;
            }
            else if(character>=97 && character<=102) // [a-f]
            {
                ival = static_cast<char>(10+character-97);
                return true;
            }
            
            ival = 0;
            return false;
        }
        
        // convert array of 8 hex ascii to f32
        // The input hexascii is required to be a little-endian representation
        // as used in the iridas file format
        // "AD10753F" -> 0.9572857022285461f on ALL architectures
        
        bool hexasciitofloat(float& fval, const char * ascii)
        {
            // Convert all ASCII numbers to their numerical representations
            char asciinums[8];
            for(unsigned int i=0; i<8; ++i)
            {
                if(!hexasciitoint(asciinums[i], ascii[i]))
                {
                    return false;
                }
            }
            
            unsigned char * fvalbytes = reinterpret_cast<unsigned char *>(&fval);
            
#if OCIO_LITTLE_ENDIAN
            // Since incoming values are little endian, and we're on little endian
            // preserve the byte order
            fvalbytes[0] = (unsigned char) (asciinums[1] | (asciinums[0] << 4));
            fvalbytes[1] = (unsigned char) (asciinums[3] | (asciinums[2] << 4));
            fvalbytes[2] = (unsigned char) (asciinums[5] | (asciinums[4] << 4));
            fvalbytes[3] = (unsigned char) (asciinums[7] | (asciinums[6] << 4));
#else
            // Since incoming values are little endian, and we're on big endian
            // flip the byte order
            fvalbytes[3] = (unsigned char) (asciinums[1] | (asciinums[0] << 4));
            fvalbytes[2] = (unsigned char) (asciinums[3] | (asciinums[2] << 4));
            fvalbytes[1] = (unsigned char) (asciinums[5] | (asciinums[4] << 4));
            fvalbytes[0] = (unsigned char) (asciinums[7] | (asciinums[6] << 4));
#endif
            return true;
        }
    }

    namespace
    {
        class XMLParserHelper
        {
        public:
            XMLParserHelper() = delete;
            XMLParserHelper(const XMLParserHelper &) = delete;
            XMLParserHelper & operator=(const XMLParserHelper &) = delete;

            XMLParserHelper(const std::string & fileName)
                : m_parser(XML_ParserCreate(NULL))
                , m_fileName(fileName)
                , m_ignoring(0)
                , m_inLook(false)
                , m_inLut(false)
                , m_inMask(false)
                , m_size(false)
                , m_data(false)
                , m_lutSize(0)
                , m_lutString("")
            {
                XML_SetUserData(m_parser, this);
                XML_SetElementHandler(m_parser, StartElementHandler, EndElementHandler);
                XML_SetCharacterDataHandler(m_parser, CharacterDataHandler);
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
                    line.push_back('\n');
                    ++m_lineNumber;

                    Parse(line, !istream.good());
                }
            }
            void Parse(const std::string & buffer, bool lastLine)
            {
                const int done = lastLine?1:0;

                if (XML_STATUS_ERROR == XML_Parse(m_parser, buffer.c_str(), (int)buffer.size(), done))
                {
                    XML_Error eXpatErrorCode = XML_GetErrorCode(m_parser);
                    if (eXpatErrorCode == XML_ERROR_TAG_MISMATCH)
                    {
                        Throw("XML parsing error (unbalanced element tags)");
                    }
                    else
                    {
                        std::string error("XML parsing error: ");
                        error += XML_ErrorString(XML_GetErrorCode(m_parser));
                        Throw(error);
                    }
                }
            }

            void getLut(int & lutSize, std::vector<float> & lut) const
            {
                if (m_lutString.size() % 8 != 0)
                {
                    std::ostringstream os;
                    os << "Error parsing Iridas Look file (";
                    os << m_fileName.c_str() << "). ";
                    os << "Number of characters in 'data' must be multiple of 8. ";
                    os << m_lutString.size() << " elements found.";
                    throw Exception(os.str().c_str());
                }

                lutSize = m_lutSize;
                int expactedVectorSize = 3 * (lutSize*lutSize*lutSize);
                lut.reserve(expactedVectorSize);

                const char * ascii = m_lutString.c_str();
                float fval = 0.0f;
                for (unsigned int i = 0; i<m_lutString.size() / 8; ++i)
                {
                    if (!hexasciitofloat(fval, &ascii[8 * i]))
                    {
                        std::ostringstream os;
                        os << "Error parsing Iridas Look file (";
                        os << m_fileName.c_str() << "). ";
                        os << "Non-hex characters found in 'data' block ";
                        os << "at index '" << (8 * i) << "'.";
                        throw Exception(os.str().c_str());
                    }
                    lut.push_back(fval);
                }

                if (expactedVectorSize != static_cast<int>(lut.size()))
                {
                    std::ostringstream os;
                    os << "Error parsing Iridas Look file (";
                    os << m_fileName.c_str() << "). ";
                    os << "Incorrect number of lut3d entries. ";
                    os << "Found " << lut.size() << " values, expected " << expactedVectorSize << ".";
                    throw Exception(os.str().c_str());
                }

            }

        private:

            void Throw(const std::string & error) const
            {
                std::ostringstream os;
                os << "Error parsing Iridas Look file (";
                os << m_fileName.c_str() << "). ";
                os << "Error is: " << error.c_str();
                os << ". At line (" << m_lineNumber << ")";
                throw Exception(os.str().c_str());
            }

            // Start the parsing of one element
            static void StartElementHandler(void *userData,
                const XML_Char *name,
                const XML_Char ** /*atts*/)
            {
                XMLParserHelper * pImpl = (XMLParserHelper*)userData;

                if (!pImpl || !name || !*name)
                {
                    if (!pImpl)
                    {
                        throw Exception("Internal Iridas Look parser error.");
                    }
                    else
                    {
                        pImpl->Throw("Internal error");
                    }
                }

                if (pImpl->m_ignoring > 0)
                {
                    pImpl->m_ignoring += 1;

                    if (pImpl->m_inMask)
                    {
                        // Non-empty mask
                        pImpl->Throw("Cannot load .look LUT containing mask");
                    }
                }
                else
                {
                    if (0 == strcmp(name, "look"))
                    {
                        if (pImpl->m_inLook)
                        {
                            pImpl->Throw("<look> node can not be inside "
                                         "a <look> node");
                        }
                        else
                        {
                            pImpl->m_inLook = true;
                        }
                    }
                    else
                    {
                        if (!pImpl->m_inLook)
                        {
                            pImpl->Throw("Expecting root node to be a look node");
                        }
                        else
                        {
                            if (!pImpl->m_inLut)
                            {

                                if (0 == strcmp(name, "LUT"))
                                {
                                    pImpl->m_inLut = true;
                                }
                                else if (0 == strcmp(name, "mask"))
                                {
                                    pImpl->m_inMask = true;
                                    pImpl->m_ignoring += 1;
                                }
                                else
                                {
                                    pImpl->m_ignoring += 1;
                                }
                            }
                            else
                            {
                                if (0 == strcmp(name, "size"))
                                {
                                    pImpl->m_size = true;
                                }
                                else if (0 == strcmp(name, "data"))
                                {
                                    pImpl->m_data = true;
                                }
                            }
                        }
                    }
                }
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

                if (pImpl->m_ignoring > 0)
                {
                    pImpl->m_ignoring -= 1;
                }
                else
                {
                    if (pImpl->m_size)
                    {
                        if (0 != strcmp(name, "size"))
                        {
                            pImpl->Throw("Expecting <size> end");
                        }
                        // reading size
                        pImpl->m_size = false;
                    }
                    else if (pImpl->m_data)
                    {
                        if (0 != strcmp(name, "data"))
                        {
                            pImpl->Throw("Expecting <data> end");
                        }
                        // reading data
                        pImpl->m_data = false;
                    }
                    else if (pImpl->m_inLut)
                    {
                        if (0 != strcmp(name, "LUT"))
                        {
                            pImpl->Throw("Expecting <LUT> end");
                        }
                        // reading lut finished
                        pImpl->m_inLut = false;
                    }
                    else if (pImpl->m_inLook)
                    {
                        if (0 != strcmp(name, "look"))
                        {
                            pImpl->Throw("Expecting <look> end");
                        }
                        // reading look finished
                        pImpl->m_inLook = false;
                    }
                    else if (pImpl->m_inMask)
                    {
                        if (0 != strcmp(name, "mask"))
                        {
                            pImpl->Throw("Expecting <mask> end");
                        }
                        // reading mask finished
                        pImpl->m_inMask = false;
                    }
                }
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
                    pImpl->Throw("XML parsing error: attribute illegal");
                }
                // Parsing a single new line. This is valid.
                if (len == 1 && s[0] == '\n') return;

                if (pImpl->m_size)
                {
                    std::string size_raw = std::string(s, len);
                    std::string size_clean = pystring::strip(size_raw, "'\" "); // strip quotes and space

                    char* endptr = 0;
                    int size_3d = static_cast<int>(strtol(size_clean.c_str(), &endptr, 10));

                    if (*endptr)
                    {
                        // strtol didn't consume entire string, there was
                        // remaining data, thus did not contain a single integer
                        std::ostringstream os;
                        os << "Invalid LUT size value: '" << size_raw;
                        os << "'. Expected quoted integer";
                        pImpl->Throw(os.str().c_str());
                    }
                    pImpl->m_lutSize = size_3d;
                }
                else if (pImpl->m_data)
                {
                    // TODO: Possible parsing improvement: string copies
                    //       could be avoided.
                    // Remove spaces, quotes and newlines
                    std::string what(s, len);
                    what = pystring::replace(what, " ", "");
                    what = pystring::replace(what, "\"", "");
                    what = pystring::replace(what, "'", "");
                    what = pystring::replace(what, "\n", "");
                    // Append to lut string
                    pImpl->m_lutString += what;
                }
            }

            unsigned getXmlLineNumber() const
            {
                return m_lineNumber;
            }

            const std::string& getXmlFilename() const
            {
                return m_fileName;
            }

            XML_Parser m_parser;
            unsigned m_lineNumber;
            std::string m_fileName;
            int m_ignoring;
            bool m_inLook;
            bool m_inLut;
            bool m_inMask;
            bool m_size;
            bool m_data;
            int m_lutSize;
            std::string m_lutString;
        };

    }

    namespace
    {
        class LocalCachedFile : public CachedFile
        {
        public:
            LocalCachedFile ()
            {
                lut3D = Lut3D::Create();
            };
            ~LocalCachedFile() {};

            // TODO: Switch to the OpData class.
            Lut3DRcPtr lut3D;
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
            info.name = "iridas_look";
            info.extension = "look";
            info.capabilities = FORMAT_CAPABILITY_READ;
            formatInfoVec.push_back(info);
        }

        CachedFileRcPtr
        LocalFileFormat::Read(
            std::istream & istream,
            const std::string & fileName) const
        {
            XMLParserHelper parser(fileName);
            parser.Parse(istream);

            // TODO: There is a LUT1D section in some .look files,
            // which we could use if available. Need to check
            // assumption that it is only written for 1D transforms,
            // and it matches the desired output

            // Validate LUT sizes, and create cached file object
            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());

            parser.getLut(cachedFile->lut3D->size[0], cachedFile->lut3D->lut);

            cachedFile->lut3D->size[1] = cachedFile->lut3D->size[0];
            cachedFile->lut3D->size[2] = cachedFile->lut3D->size[0];

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
                os << "Cannot build Iridas .look Op. Invalid cache type.";
                throw Exception(os.str().c_str());
            }

            TransformDirection newDir = CombineTransformDirections(dir,
                fileTransform.getDirection());
            if(newDir == TRANSFORM_DIR_UNKNOWN)
            {
                std::ostringstream os;
                os << "Cannot build file format transform,";
                os << " unspecified transform direction.";
                throw Exception(os.str().c_str());
            }

            CreateLut3DOp(ops, cachedFile->lut3D,
                          fileTransform.getInterpolation(), newDir);
        }
    }

    FileFormat * CreateFileFormatIridasLook()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

#include "unittest.h"

OCIO_NAMESPACE_USING



OIIO_ADD_TEST(FileFormatIridasLook, hexasciitoint)
{
    {
    char ival = 0;
    bool success = hexasciitoint(ival, 'a');
    OIIO_CHECK_EQUAL(success, true); OIIO_CHECK_EQUAL(ival, 10);
    }
    
    {
    char ival = 0;
    bool success = hexasciitoint(ival, 'A');
    OIIO_CHECK_EQUAL(success, true); OIIO_CHECK_EQUAL(ival, 10);
    }
    
    {
    char ival = 0;
    bool success = hexasciitoint(ival, 'f');
    OIIO_CHECK_EQUAL(success, true); OIIO_CHECK_EQUAL(ival, 15);
    }
    
    {
    char ival = 0;
    bool success = hexasciitoint(ival, 'F');
    OIIO_CHECK_EQUAL(success, true); OIIO_CHECK_EQUAL(ival, 15);
    }
    
    {
    char ival = 0;
    bool success = hexasciitoint(ival, '0');
    OIIO_CHECK_EQUAL(success, true); OIIO_CHECK_EQUAL(ival, 0);
    }
    
    {
    char ival = 0;
    bool success = hexasciitoint(ival, '0');
    OIIO_CHECK_EQUAL(success, true); OIIO_CHECK_EQUAL(ival, 0);
    }
    
    {
    char ival = 0;
    bool success = hexasciitoint(ival, '9');
    OIIO_CHECK_EQUAL(success, true); OIIO_CHECK_EQUAL(ival, 9);
    }
    
    {
    char ival = 0;
    bool success = hexasciitoint(ival, '\n');
    OIIO_CHECK_EQUAL(success, false);
    }
    
    {
    char ival = 0;
    bool success = hexasciitoint(ival, 'j');
    OIIO_CHECK_EQUAL(success, false);
    }
    
    {
    char ival = 0;
    bool success = hexasciitoint(ival, 'x');
    OIIO_CHECK_EQUAL(success, false);
    }
}


OIIO_ADD_TEST(FileFormatIridasLook, hexasciitofloat)
{
    //>>> import binascii, struct
    //>> struct.unpack("<f", binascii.unhexlify("AD10753F"))[0]
    //0.9572857022285461
    
    {
    float fval = 0.0f;
    bool success = hexasciitofloat(fval, "0000003F");
    OIIO_CHECK_EQUAL(success, true); OIIO_CHECK_EQUAL(fval, 0.5f);
    }
    
    {
    float fval = 0.0f;
    bool success = hexasciitofloat(fval, "0000803F");
    OIIO_CHECK_EQUAL(success, true); OIIO_CHECK_EQUAL(fval, 1.0f);
    }
    
    {
    float fval = 0.0f;
    bool success = hexasciitofloat(fval, "AD10753F");
    OIIO_CHECK_EQUAL(success, true); OIIO_CHECK_EQUAL(fval, 0.9572857022285461f);
    }
    
    {
    float fval = 0.0f;
    bool success = hexasciitofloat(fval, "AD10X53F");
    OIIO_CHECK_EQUAL(success, false);
    }
}


OIIO_ADD_TEST(FileFormatIridasLook, simple3d)
{
    std::ostringstream strebuf;
    strebuf << "<?xml version=\"1.0\" ?>" << "\n";
    strebuf << "<look>" << "\n";
    strebuf << "  <shaders>" << "\n";
    strebuf << "    <base>" << "\n";
    strebuf << "      <visible>\"1\"</visible>" << "\n";
    strebuf << "      <sublayer0>" << "\n";
    strebuf << "        <opacity>\"1\"</opacity>" << "\n";
    strebuf << "        <parameters>" << "\n";
    strebuf << "          <Secondary1>\"1\"</Secondary1>" << "\n";
    strebuf << "          <Secondary5>\"0\"</Secondary5>" << "\n";
    strebuf << "          <Secondary4>\"0\"</Secondary4>" << "\n";
    strebuf << "          <Secondary2>\"0\"</Secondary2>" << "\n";
    strebuf << "          <Secondary6>\"0\"</Secondary6>" << "\n";
    strebuf << "          <Secondary3>\"0\"</Secondary3>" << "\n";
    strebuf << "          <Blur>\"0\"</Blur>" << "\n";
    strebuf << "          <saturation>\"0\"</saturation>" << "\n";
    strebuf << "        </parameters>" << "\n";
    strebuf << "      </sublayer0>" << "\n";
    strebuf << "    </base>" << "\n";
    strebuf << "  </shaders>" << "\n";
    strebuf << "  <LUT>" << "\n";
    strebuf << "    <size>\"8\"</size>" << "\n";
    strebuf << "    <data>\"" << "\n";
    strebuf << "      0000008000000080000000802CF52E3D2DF52E3D2DF52E3D2CF5AE3D2DF5AE3D" << "\n";
    strebuf << "      2DF5AE3DE237033EE237033EE237033E2CF52E3E2DF52E3E2DF52E3E78B25A3E" << "\n";
    strebuf << "      78B25A3E78B25A3EE037833EE137833EE137833E8616993E8716993E8716993E" << "\n";
    strebuf << "      4BBDAB3D4BBDAB3D4BBDAB3DF09B013EF09B013EF09B013E3C592D3E3C592D3E" << "\n";
    strebuf << "      3C592D3E8716593E8716593E8716593EE969823EE969823EE969823E8E48983E" << "\n";
    strebuf << "      8E48983E8E48983E3227AE3E3327AE3E3327AE3ED805C43ED905C43ED905C43E" << "\n";
    strebuf << "      4BBD2B3E4BBD2B3E4BBD2B3E967A573E967A573E967A573EF09B813EF09B813E" << "\n";
    strebuf << "      F09B813E967A973E967A973E967A973E3C59AD3E3C59AD3E3C59AD3EE137C33E" << "\n";
    strebuf << "      E137C33EE137C33E8616D93E8616D93E8616D93E2CF5EE3E2CF5EE3E2CF5EE3E" << "\n";
    strebuf << "      F9CD803EF9CD803EF9CD803E9EAC963E9EAC963E9EAC963E448BAC3E448BAC3E" << "\n";
    strebuf << "      448BAC3EEA69C23EEA69C23EEA69C23E8F48D83E8F48D83E8F48D83E3527EE3E" << "\n";
    strebuf << "      3527EE3E3527EE3EED02023FED02023FED02023F40F20C3F40F20C3F40F20C3F" << "\n";
    strebuf << "      4BBDAB3E4BBDAB3E4BBDAB3EF09BC13EF09BC13EF09BC13E967AD73E967AD73E" << "\n";
    strebuf << "      967AD73E3C59ED3E3C59ED3E3C59ED3EF09B013FF09B013FF09B013F438B0C3F" << "\n";
    strebuf << "      438B0C3F438B0C3F967A173F967A173F967A173FE969223FE969223FE969223F" << "\n";
    strebuf << "      9EACD63E9EACD63E9EACD63E428BEC3E438BEC3E438BEC3EF434013FF434013F" << "\n";
    strebuf << "      F434013F47240C3F47240C3F47240C3F9A13173F9A13173F9A13173FED02223F" << "\n";
    strebuf << "      ED02223FED02223F3FF22C3F3FF22C3F3FF22C3F92E1373F92E1373F92E1373F" << "\n";
    strebuf << "      F8CD003FF8CD003FF8CD003F49BD0B3F4ABD0B3F4ABD0B3F9DAC163F9DAC163F" << "\n";
    strebuf << "      9DAC163FF09B213FF09B213FF09B213F438B2C3F438B2C3F438B2C3F967A373F" << "\n";
    strebuf << "      967A373F967A373FE869423FE869423FE869423F3B594D3F3B594D3F3B594D3F" << "\n";
    strebuf << "      A245163FA245163FA245163FF334213FF434213FF434213F47242C3F47242C3F" << "\n";
    strebuf << "      47242C3F9A13373F9A13373F9A13373FED02423FED02423FED02423F40F24C3F" << "\n";
    strebuf << "      40F24C3F40F24C3F92E1573F92E1573F92E1573FE5D0623FE5D0623FE5D0623F" << "\n";
    strebuf << "      9E69853C9E69853C9869853CFCA9713DFCA9713DFCA9713D944FD03D944FD03D" << "\n";
    strebuf << "      944FD03D14E5133E15E5133E15E5133E60A23F3E60A23F3E60A23F3EAA5F6B3E" << "\n";
    strebuf << "      AB5F6B3EAB5F6B3E7A8E8B3E7A8E8B3E7A8E8B3E206DA13E206DA13E206DA13E" << "\n";
    strebuf << "      B217CD3DB217CD3DB217CD3D2449123E2449123E2449123E6F063E3E6F063E3E" << "\n";
    strebuf << "      6F063E3EBAC3693EBAC3693EBAC3693E82C08A3E82C08A3E82C08A3E289FA03E" << "\n";
    strebuf << "      289FA03E289FA03ECC7DB63ECC7DB63ECC7DB63E725CCC3E715CCC3E715CCC3E" << "\n";
    strebuf << "      7E6A3C3E7E6A3C3E7E6A3C3ECA27683ECA27683ECA27683E8AF2893E8AF2893E" << "\n";
    strebuf << "      8AF2893E30D19F3E30D19F3E30D19F3ED5AFB53ED5AFB53ED5AFB53E7B8ECB3E" << "\n";
    strebuf << "      7B8ECB3E7A8ECB3E1F6DE13E1F6DE13E1E6DE13EC44BF73EC54BF73EC44BF73E" << "\n";
    strebuf << "      9224893E9224893E9224893E38039F3E38039F3E38039F3EDEE1B43EDEE1B43E" << "\n";
    strebuf << "      DEE1B43E83C0CA3E83C0CA3E82C0CA3E299FE03E299FE03E289FE03ECE7DF63E" << "\n";
    strebuf << "      CE7DF63ECD7DF63E392E063F392E063F382E063F8C1D113F8C1D113F8B1D113F" << "\n";
    strebuf << "      E413B43EE413B43EE413B43E89F2C93E8AF2C93E89F2C93E30D1DF3E30D1DF3E" << "\n";
    strebuf << "      2FD1DF3ED5AFF53ED5AFF53ED4AFF53E3DC7053F3DC7053F3CC7053F90B6103F" << "\n";
    strebuf << "      90B6103F8FB6103FE2A51B3FE2A51B3FE1A51B3F3595263F3595263F3495263F" << "\n";
    strebuf << "      3703DF3E3703DF3E3603DF3EDCE1F43EDDE1F43EDCE1F43E4160053F4160053F" << "\n";
    strebuf << "      4060053F944F103F944F103F934F103FE73E1B3FE73E1B3FE63E1B3F392E263F" << "\n";
    strebuf << "      392E263F382E263F8C1D313F8C1D313F8B1D313FDF0C3C3FDF0C3C3FDE0C3C3F" << "\n";
    strebuf << "      44F9043F44F9043F43F9043F96E80F3F97E80F3F96E80F3FEAD71A3FEAD71A3F" << "\n";
    strebuf << "      E9D71A3F3DC7253F3DC7253F3CC7253F90B6303F90B6303F8FB6303FE2A53B3F" << "\n";
    strebuf << "      E2A53B3FE1A53B3F3595463F3595463F3495463F8884513F8884513F8784513F" << "\n";
    strebuf << "      EE701A3FEE701A3FED701A3F4060253F4160253F4060253F944F303F944F303F" << "\n";
    strebuf << "      934F303FE73E3B3FE73E3B3FE63E3B3F3A2E463F3A2E463F392E463F8C1D513F" << "\n";
    strebuf << "      8C1D513F8B1D513FDF0C5C3FDF0C5C3FDE0C5C3F32FC663F32FC663F31FC663F" << "\n";
    strebuf << "      9E69053D9E69053D9869053D652F9A3D652F9A3D642F9A3DFCA9F13DFCA9F13D" << "\n";
    strebuf << "      FCA9F13D4892243E4992243E4992243E944F503E944F503E944F503EDE0C7C3E" << "\n";
    strebuf << "      DF0C7C3EDF0C7C3E14E5933E14E5933E14E5933EBAC3A93EBAC3A93EBAC3A93E" << "\n";
    strebuf << "      1A72EE3D1A72EE3D1A72EE3D58F6223E58F6223E58F6223EA3B34E3EA3B34E3E" << "\n";
    strebuf << "      A3B34E3EEE707A3EEE707A3EEE707A3E1C17933E1C17933E1C17933EC2F5A83E" << "\n";
    strebuf << "      C2F5A83EC2F5A83E66D4BE3E66D4BE3E66D4BE3E0CB3D43E0BB3D43E0CB3D43E" << "\n";
    strebuf << "      B2174D3EB2174D3EB2174D3EFDD4783EFDD4783EFDD4783E2449923E2449923E" << "\n";
    strebuf << "      2449923ECA27A83ECA27A83ECA27A83E6F06BE3E6F06BE3E6F06BE3E15E5D33E" << "\n";
    strebuf << "      15E5D33E15E5D33EB9C3E93EB9C3E93EB9C3E93E5EA2FF3E5FA2FF3E5FA2FF3E" << "\n";
    strebuf << "      2C7B913E2C7B913E2C7B913ED259A73ED259A73ED259A73E7838BD3E7838BD3E" << "\n";
    strebuf << "      7838BD3E1D17D33E1D17D33E1D17D33EC3F5E83EC3F5E83EC3F5E83E68D4FE3E" << "\n";
    strebuf << "      68D4FE3E68D4FE3E86590A3F86590A3F86590A3FD948153FD948153FD948153F" << "\n";
    strebuf << "      7E6ABC3E7E6ABC3E7E6ABC3E2349D23E2449D23E2449D23ECA27E83ECA27E83E" << "\n";
    strebuf << "      CA27E83E6F06FE3E6F06FE3E6F06FE3E8AF2093F8AF2093F8AF2093FDDE1143F" << "\n";
    strebuf << "      DDE1143FDDE1143F2FD11F3F2FD11F3F2FD11F3F82C02A3F82C02A3F82C02A3F" << "\n";
    strebuf << "      D159E73ED159E73ED159E73E7638FD3E7738FD3E7738FD3E8E8B093F8E8B093F" << "\n";
    strebuf << "      8E8B093FE17A143FE17A143FE17A143F346A1F3F346A1F3F346A1F3F86592A3F" << "\n";
    strebuf << "      86592A3F86592A3FD948353FD948353FD948353F2C38403F2C38403F2C38403F" << "\n";
    strebuf << "      9124093F9124093F9124093FE313143FE413143FE413143F37031F3F37031F3F" << "\n";
    strebuf << "      37031F3F8AF2293F8AF2293F8AF2293FDDE1343FDDE1343FDDE1343F2FD13F3F" << "\n";
    strebuf << "      2FD13F3F2FD13F3F82C04A3F82C04A3F81C04A3FD5AF553FD5AF553FD4AF553F" << "\n";
    strebuf << "      3B9C1E3F3B9C1E3F3B9C1E3F8D8B293F8E8B293F8E8B293FE17A343FE17A343F" << "\n";
    strebuf << "      E17A343F346A3F3F346A3F3F346A3F3F87594A3F87594A3F86594A3FD948553F" << "\n";
    strebuf << "      D948553FD848553F2C38603F2C38603F2B38603F7F276B3F7F276B3F7E276B3F" << "\n";
    strebuf << "      6E1E483D6E1E483D681E483DCD89BB3DCD89BB3DCC89BB3D3282093E3282093E" << "\n";
    strebuf << "      3282093E7C3F353E7D3F353E7C3F353EC8FC603EC8FC603EC8FC603E095D863E" << "\n";
    strebuf << "      095D863E095D863EAE3B9C3EAE3B9C3EAE3B9C3E541AB23E541AB23E541AB23E" << "\n";
    strebuf << "      41E6073E41E6073E40E6073E8CA3333E8CA3333E8CA3333ED7605F3ED7605F3E" << "\n";
    strebuf << "      D7605F3E118F853E118F853E118F853EB66D9B3EB66D9B3EB66D9B3E5B4CB13E" << "\n";
    strebuf << "      5B4CB13E5B4CB13E002BC73E002BC73E002BC73EA609DD3EA509DD3EA609DD3E" << "\n";
    strebuf << "      E6C45D3EE6C45D3EE6C45D3E18C1843E18C1843E18C1843EBE9F9A3EBE9F9A3E" << "\n";
    strebuf << "      BE9F9A3E647EB03E647EB03E647EB03E095DC63E095DC63E095DC63EAE3BDC3E" << "\n";
    strebuf << "      AE3BDC3EAE3BDC3E531AF23E531AF23E531AF23E7CFC033F7CFC033F7CFC033F" << "\n";
    strebuf << "      C6D1993EC6D1993EC6D1993E6CB0AF3E6CB0AF3E6CB0AF3E128FC53E128FC53E" << "\n";
    strebuf << "      128FC53EB76DDB3EB76DDB3EB76DDB3E5D4CF13E5D4CF13E5D4CF13E8195033F" << "\n";
    strebuf << "      8195033F8195033FD3840E3FD3840E3FD3840E3F2674193F2674193F2674193F" << "\n";
    strebuf << "      18C1C43E18C1C43E18C1C43EBD9FDA3EBE9FDA3EBE9FDA3E647EF03E647EF03E" << "\n";
    strebuf << "      647EF03E842E033F842E033F842E033FD71D0E3FD71D0E3FD71D0E3F2A0D193F" << "\n";
    strebuf << "      2A0D193F2A0D193F7CFC233F7CFC233F7CFC233FCFEB2E3FCFEB2E3FCFEB2E3F" << "\n";
    strebuf << "      6BB0EF3E6BB0EF3E6BB0EF3E87C7023F88C7023F88C7023FDBB60D3FDBB60D3F" << "\n";
    strebuf << "      DBB60D3F2EA6183F2EA6183F2EA6183F8195233F8195233F8195233FD3842E3F" << "\n";
    strebuf << "      D3842E3FD3842E3F2674393F2674393F2674393F7963443F7963443F7963443F" << "\n";
    strebuf << "      DE4F0D3FDE4F0D3FDE4F0D3F303F183F313F183F313F183F842E233F842E233F" << "\n";
    strebuf << "      842E233FD71D2E3FD71D2E3FD71D2E3F2A0D393F2A0D393F2A0D393F7CFC433F" << "\n";
    strebuf << "      7CFC433F7CFC433FCFEB4E3FCFEB4E3FCFEB4E3F22DB593F22DB593F22DB593F" << "\n";
    strebuf << "      88C7223F88C7223F88C7223FDAB62D3FDBB62D3FDBB62D3F2EA6383F2EA6383F" << "\n";
    strebuf << "      2EA6383F8195433F8195433F8195433FD4844E3FD4844E3FD4844E3F2674593F" << "\n";
    strebuf << "      2674593F2674593F7963643F7963643F7963643FCC526F3FCC526F3FCC526F3F" << "\n";
    strebuf << "      9E69853D9E69853D9869853D34E4DC3D34E4DC3D34E4DC3D652F1A3E652F1A3E" << "\n";
    strebuf << "      642F1A3EB1EC453EB1EC453EB0EC453EFCA9713EFCA9713EFCA9713EA3B38E3E" << "\n";
    strebuf << "      A3B38E3EA3B38E3E4892A43E4892A43E4892A43EEE70BA3EEE70BA3EEE70BA3E" << "\n";
    strebuf << "      7493183E7493183E7493183EBF50443EBF50443EBE50443E0A0E703E0A0E703E" << "\n";
    strebuf << "      0A0E703EABE58D3EABE58D3EABE58D3E50C4A33E50C4A33E50C4A33EF5A2B93E" << "\n";
    strebuf << "      F5A2B93EF5A2B93E9A81CF3E9981CF3E9A81CF3E4060E53E3F60E53E4060E53E" << "\n";
    strebuf << "      1A726E3E1A726E3E1A726E3EB2178D3EB2178D3EB2178D3E58F6A23E58F6A23E" << "\n";
    strebuf << "      58F6A23EFED4B83EFED4B83EFED4B83EA3B3CE3EA3B3CE3EA3B3CE3E4892E43E" << "\n";
    strebuf << "      4892E43E4892E43EED70FA3EED70FA3EED70FA3EC927083FC927083FC927083F" << "\n";
    strebuf << "      6028A23E6028A23E6028A23E0607B83E0607B83E0607B83EABE5CD3EABE5CD3E" << "\n";
    strebuf << "      ABE5CD3E51C4E33E51C4E33E51C4E33EF7A2F93EF7A2F93EF7A2F93ECEC0073F" << "\n";
    strebuf << "      CEC0073FCEC0073F20B0123F20B0123F20B0123F739F1D3F739F1D3F739F1D3F" << "\n";
    strebuf << "      B217CD3EB217CD3EB217CD3E57F6E23E58F6E23E58F6E23EFDD4F83EFDD4F83E" << "\n";
    strebuf << "      FDD4F83ED159073FD159073FD159073F2449123F2449123F2449123F77381D3F" << "\n";
    strebuf << "      77381D3F77381D3FC927283FC927283FC927283F1C17333F1C17333F1C17333F" << "\n";
    strebuf << "      0507F83E0507F83E0507F83ED4F2063FD5F2063FD5F2063F28E2113F28E2113F" << "\n";
    strebuf << "      28E2113F7BD11C3F7BD11C3F7BD11C3FCEC0273FCEC0273FCEC0273F20B0323F" << "\n";
    strebuf << "      20B0323F20B0323F739F3D3F739F3D3F739F3D3FC68E483FC68E483FC68E483F" << "\n";
    strebuf << "      2B7B113F2B7B113F2B7B113F7D6A1C3F7E6A1C3F7E6A1C3FD159273FD159273F" << "\n";
    strebuf << "      D159273F2449323F2449323F2449323F77383D3F77383D3F77383D3FC927483F" << "\n";
    strebuf << "      C927483FC927483F1C17533F1C17533F1C17533F6F065E3F6F065E3F6F065E3F" << "\n";
    strebuf << "      D5F2263FD5F2263FD5F2263F27E2313F28E2313F28E2313F7BD13C3F7BD13C3F" << "\n";
    strebuf << "      7BD13C3FCEC0473FCEC0473FCEC0473F21B0523F21B0523F21B0523F739F5D3F" << "\n";
    strebuf << "      739F5D3F739F5D3FC68E683FC68E683FC68E683F197E733F197E733F197E733F" << "\n";
    strebuf << "      06C4A63D06C4A63D00C4A63D9C3EFE3D9C3EFE3D983EFE3D99DC2A3E99DC2A3E" << "\n";
    strebuf << "      98DC2A3EE599563EE599563EE499563E982B813E982B813E982B813E3D0A973E" << "\n";
    strebuf << "      3D0A973E3D0A973EE2E8AC3EE2E8AC3EE2E8AC3E88C7C23E88C7C23E88C7C23E" << "\n";
    strebuf << "      A840293EA840293EA840293EF3FD543EF3FD543EF0FD543E9F5D803E9F5D803E" << "\n";
    strebuf << "      9F5D803E453C963E453C963E453C963EEA1AAC3EEA1AAC3EEA1AAC3E8FF9C13E" << "\n";
    strebuf << "      8FF9C13E8FF9C13E34D8D73E33D8D73E34D8D73EDAB6ED3ED9B6ED3EDAB6ED3E" << "\n";
    strebuf << "      4E1F7F3E4E1F7F3E4E1F7F3E4C6E953E4C6E953E4C6E953EF24CAB3EF24CAB3E" << "\n";
    strebuf << "      F24CAB3E982BC13E982BC13E982BC13E3D0AD73E3D0AD73E3D0AD73EE2E8EC3E" << "\n";
    strebuf << "      E2E8EC3EE2E8EC3EC363013FC363013FC363013F16530C3F16530C3F16530C3F" << "\n";
    strebuf << "      FA7EAA3EFA7EAA3EFA7EAA3EA05DC03EA05DC03EA05DC03E453CD63E453CD63E" << "\n";
    strebuf << "      453CD63EEB1AEC3EEB1AEC3EEB1AEC3EC8FC003FC8FC003FC8FC003F1BEC0B3F" << "\n";
    strebuf << "      1BEC0B3F1BEC0B3F6DDB163F6DDB163F6DDB163FC0CA213FC0CA213FC0CA213F" << "\n";
    strebuf << "      4C6ED53E4C6ED53E4C6ED53EF14CEB3EF24CEB3EF24CEB3ECB95003FCB95003F" << "\n";
    strebuf << "      CB95003F1E850B3F1E850B3F1E850B3F7174163F7174163F7174163FC463213F" << "\n";
    strebuf << "      C463213FC463213F16532C3F16532C3F16532C3F6942373F6942373F6942373F" << "\n";
    strebuf << "      CF2E003FCF2E003FCF2E003F211E0B3F221E0B3F221E0B3F750D163F750D163F" << "\n";
    strebuf << "      750D163FC8FC203FC8FC203FC8FC203F1BEC2B3F1BEC2B3F1BEC2B3F6DDB363F" << "\n";
    strebuf << "      6DDB363F6DDB363FC0CA413FC0CA413FC0CA413F13BA4C3F13BA4C3F13BA4C3F" << "\n";
    strebuf << "      78A6153F78A6153F78A6153FCA95203FCB95203FCB95203F1E852B3F1E852B3F" << "\n";
    strebuf << "      1E852B3F7174363F7174363F7174363FC463413FC463413FC463413F16534C3F" << "\n";
    strebuf << "      16534C3F16534C3F6942573F6942573F6942573FBC31623FBC31623FBC31623F" << "\n";
    strebuf << "      221E2B3F221E2B3F221E2B3F740D363F750D363F750D363FC8FC403FC8FC403F" << "\n";
    strebuf << "      C8FC403F1BEC4B3F1BEC4B3F1BEC4B3F6EDB563F6EDB563F6EDB563FC0CA613F" << "\n";
    strebuf << "      C0CA613FC0CA613F13BA6C3F13BA6C3F13BA6C3F66A9773F66A9773F66A9773F" << "\n";
    strebuf << "      6D1EC83D6D1EC83D681EC83D81CC0F3E81CC0F3E80CC0F3ECD893B3ECD893B3E" << "\n";
    strebuf << "      CC893B3E1847673E1847673E1847673E3182893E3182893E3082893ED7609F3E" << "\n";
    strebuf << "      D7609F3ED6609F3E7C3FB53E7C3FB53E7C3FB53E221ECB3E221ECB3E221ECB3E" << "\n";
    strebuf << "      DCED393EDCED393EDCED393E26AB653E26AB653E24AB653E39B4883E39B4883E" << "\n";
    strebuf << "      38B4883EDE929E3EDE929E3EDE929E3E8371B43E8371B43E8271B43E2950CA3E" << "\n";
    strebuf << "      2850CA3E2950CA3ECE2EE03ECD2EE03ECE2EE03E740DF63E730DF63E740DF63E" << "\n";
    strebuf << "      40E6873E40E6873E40E6873EE6C49D3EE6C49D3EE6C49D3E8CA3B33E8CA3B33E" << "\n";
    strebuf << "      8CA3B33E3182C93E3182C93E3182C93ED660DF3ED660DF3ED660DF3E7C3FF53E" << "\n";
    strebuf << "      7C3FF53E7C3FF53E108F053F108F053F108F053F637E103F637E103F637E103F" << "\n";
    strebuf << "      94D5B23E94D5B23E94D5B23E39B4C83E39B4C83E39B4C83EDF92DE3EDF92DE3E" << "\n";
    strebuf << "      DF92DE3E8571F43E8571F43E8571F43E1528053F1528053F1528053F6817103F" << "\n";
    strebuf << "      6817103F6817103FBA061B3FBA061B3FBA061B3F0DF6253F0DF6253F0DF6253F" << "\n";
    strebuf << "      E6C4DD3EE6C4DD3EE6C4DD3E8AA3F33E8BA3F33E8BA3F33E18C1043F18C1043F" << "\n";
    strebuf << "      18C1043F6BB00F3F6BB00F3F6BB00F3FBE9F1A3FBE9F1A3FBE9F1A3F118F253F" << "\n";
    strebuf << "      118F253F118F253F637E303F637E303F637E303FB66D3B3FB66D3B3FB66D3B3F" << "\n";
    strebuf << "      1C5A043F1C5A043F1C5A043F6E490F3F6F490F3F6F490F3FC2381A3FC2381A3F" << "\n";
    strebuf << "      C2381A3F1528253F1528253F1528253F6717303F6717303F6717303FBA063B3F" << "\n";
    strebuf << "      BA063B3FBA063B3F0DF6453F0DF6453F0DF6453F60E5503F60E5503F60E5503F" << "\n";
    strebuf << "      C5D1193FC5D1193FC5D1193F17C1243F18C1243F18C1243F6BB02F3F6BB02F3F" << "\n";
    strebuf << "      6BB02F3FBE9F3A3FBE9F3A3FBE9F3A3F108F453F108F453F108F453F637E503F" << "\n";
    strebuf << "      637E503F637E503FB66D5B3FB66D5B3FB66D5B3F095D663F095D663F095D663F" << "\n";
    strebuf << "      6F492F3F6F492F3F6F492F3FC1383A3FC2383A3FC2383A3F1528453F1528453F" << "\n";
    strebuf << "      1528453F6817503F6817503F6817503FBA065B3FBA065B3FBA065B3F0DF6653F" << "\n";
    strebuf << "      0DF6653F0DF6653F60E5703F60E5703F60E5703FB3D47B3FB3D47B3FB3D47B3F" << "\n";
    strebuf << "      D578E93DD578E93DD078E93DB579203EB579203EB479203E01374C3E01374C3E" << "\n";
    strebuf << "      00374C3E4CF4773E4CF4773E4CF4773ECBD8913ECBD8913ECAD8913E71B7A73E" << "\n";
    strebuf << "      71B7A73E70B7A73E1696BD3E1696BD3E1696BD3EBC74D33EBC74D33EBC74D33E" << "\n";
    strebuf << "      109B4A3E109B4A3E109B4A3E5A58763E5A58763E5858763ED30A913ED30A913E" << "\n";
    strebuf << "      D20A913E78E9A63E78E9A63E78E9A63E1DC8BC3E1DC8BC3E1CC8BC3EC3A6D23E" << "\n";
    strebuf << "      C2A6D23EC2A6D23E6885E83E6785E83E6885E83E0E64FE3E0D64FE3E0E64FE3E" << "\n";
    strebuf << "      DA3C903EDA3C903EDA3C903E801BA63E801BA63E801BA63E26FABB3E26FABB3E" << "\n";
    strebuf << "      26FABB3ECBD8D13ECBD8D13ECAD8D13E70B7E73E70B7E73E70B7E73E1696FD3E" << "\n";
    strebuf << "      1696FD3E1696FD3E5DBA093F5DBA093F5DBA093FB0A9143FB0A9143FB0A9143F" << "\n";
    strebuf << "      2E2CBB3E2E2CBB3E2E2CBB3ED20AD13ED30AD13ED20AD13E79E9E63E79E9E63E" << "\n";
    strebuf << "      78E9E63E1FC8FC3E1FC8FC3E1EC8FC3E6253093F6253093F6253093FB542143F" << "\n";
    strebuf << "      B542143FB542143F07321F3F07321F3F07321F3F5A212A3F5A212A3F5A212A3F" << "\n";
    strebuf << "      801BE63E801BE63E801BE63E24FAFB3E25FAFB3E24FAFB3E65EC083F65EC083F" << "\n";
    strebuf << "      65EC083FB8DB133FB8DB133FB8DB133F0BCB1E3F0BCB1E3F0BCB1E3F5EBA293F" << "\n";
    strebuf << "      5EBA293F5EBA293FB0A9343FB0A9343FB0A9343F03993F3F03993F3F03993F3F" << "\n";
    strebuf << "      6985083F6985083F6985083FBB74133FBC74133FBC74133F0F641E3F0F641E3F" << "\n";
    strebuf << "      0F641E3F6253293F6253293F6253293FB442343FB442343FB442343F07323F3F" << "\n";
    strebuf << "      07323F3F07323F3F5A214A3F5A214A3F5A214A3FAD10553FAD10553FAD10553F" << "\n";
    strebuf << "      12FD1D3F12FD1D3F12FD1D3F64EC283F65EC283F65EC283FB8DB333FB8DB333F" << "\n";
    strebuf << "      B8DB333F0BCB3E3F0BCB3E3F0BCB3E3F5DBA493F5DBA493F5DBA493FB0A9543F" << "\n";
    strebuf << "      B0A9543FB0A9543F03995F3F03995F3F03995F3F56886A3F56886A3F56886A3F" << "\n";
    strebuf << "      BC74333FBC74333FBC74333F0E643E3F0F643E3F0F643E3F6153493F6253493F" << "\n";
    strebuf << "      6253493FB542543FB542543FB542543F07325F3F07325F3F07325F3F5A216A3F" << "\n";
    strebuf << "      5A216A3F5A216A3FAD10753FAD10753FAD10753F0000803F0000803F0000803F\"" << "\n";
    strebuf << "    </data>" << "\n";
    strebuf << "  </LUT>" << "\n";
    strebuf << "</look>" << "\n";

    std::istringstream simple1D;
    simple1D.str(strebuf.str());

    // Read file
    std::string emptyString;
    LocalFileFormat tester;
    CachedFileRcPtr cachedFile = tester.Read(simple1D, emptyString);
    LocalCachedFileRcPtr looklut = DynamicPtrCast<LocalCachedFile>(cachedFile);

    // Check LUT size is correct
    OIIO_CHECK_EQUAL(looklut->lut3D->size[0], 8);
    OIIO_CHECK_EQUAL(looklut->lut3D->size[1], 8);
    OIIO_CHECK_EQUAL(looklut->lut3D->size[2], 8);

    // Check LUT values
    OIIO_CHECK_EQUAL(looklut->lut3D->lut.size(), 8*8*8*3);

    double cube[8 * 8 * 8 * 3] = {
        -0.00000, -0.00000, -0.00000,
        0.04271, 0.04271, 0.04271,
        0.08543, 0.08543, 0.08543,
        0.12814, 0.12814, 0.12814,
        0.17086, 0.17086, 0.17086,
        0.21357, 0.21357, 0.21357,
        0.25629, 0.25629, 0.25629,
        0.29900, 0.29900, 0.29900,
        0.08386, 0.08386, 0.08386,
        0.12657, 0.12657, 0.12657,
        0.16929, 0.16929, 0.16929,
        0.21200, 0.21200, 0.21200,
        0.25471, 0.25471, 0.25471,
        0.29743, 0.29743, 0.29743,
        0.34014, 0.34014, 0.34014,
        0.38286, 0.38286, 0.38286,
        0.16771, 0.16771, 0.16771,
        0.21043, 0.21043, 0.21043,
        0.25314, 0.25314, 0.25314,
        0.29586, 0.29586, 0.29586,
        0.33857, 0.33857, 0.33857,
        0.38129, 0.38129, 0.38129,
        0.42400, 0.42400, 0.42400,
        0.46671, 0.46671, 0.46671,
        0.25157, 0.25157, 0.25157,
        0.29429, 0.29429, 0.29429,
        0.33700, 0.33700, 0.33700,
        0.37971, 0.37971, 0.37971,
        0.42243, 0.42243, 0.42243,
        0.46514, 0.46514, 0.46514,
        0.50786, 0.50786, 0.50786,
        0.55057, 0.55057, 0.55057,
        0.33543, 0.33543, 0.33543,
        0.37814, 0.37814, 0.37814,
        0.42086, 0.42086, 0.42086,
        0.46357, 0.46357, 0.46357,
        0.50629, 0.50629, 0.50629,
        0.54900, 0.54900, 0.54900,
        0.59171, 0.59171, 0.59171,
        0.63443, 0.63443, 0.63443,
        0.41929, 0.41929, 0.41929,
        0.46200, 0.46200, 0.46200,
        0.50471, 0.50471, 0.50471,
        0.54743, 0.54743, 0.54743,
        0.59014, 0.59014, 0.59014,
        0.63286, 0.63286, 0.63286,
        0.67557, 0.67557, 0.67557,
        0.71829, 0.71829, 0.71829,
        0.50314, 0.50314, 0.50314,
        0.54586, 0.54586, 0.54586,
        0.58857, 0.58857, 0.58857,
        0.63129, 0.63129, 0.63129,
        0.67400, 0.67400, 0.67400,
        0.71671, 0.71671, 0.71671,
        0.75943, 0.75943, 0.75943,
        0.80214, 0.80214, 0.80214,
        0.58700, 0.58700, 0.58700,
        0.62971, 0.62971, 0.62971,
        0.67243, 0.67243, 0.67243,
        0.71514, 0.71514, 0.71514,
        0.75786, 0.75786, 0.75786,
        0.80057, 0.80057, 0.80057,
        0.84329, 0.84329, 0.84329,
        0.88600, 0.88600, 0.88600,
        0.01629, 0.01629, 0.01629,
        0.05900, 0.05900, 0.05900,
        0.10171, 0.10171, 0.10171,
        0.14443, 0.14443, 0.14443,
        0.18714, 0.18714, 0.18714,
        0.22986, 0.22986, 0.22986,
        0.27257, 0.27257, 0.27257,
        0.31529, 0.31529, 0.31529,
        0.10014, 0.10014, 0.10014,
        0.14286, 0.14286, 0.14286,
        0.18557, 0.18557, 0.18557,
        0.22829, 0.22829, 0.22829,
        0.27100, 0.27100, 0.27100,
        0.31371, 0.31371, 0.31371,
        0.35643, 0.35643, 0.35643,
        0.39914, 0.39914, 0.39914,
        0.18400, 0.18400, 0.18400,
        0.22671, 0.22671, 0.22671,
        0.26943, 0.26943, 0.26943,
        0.31214, 0.31214, 0.31214,
        0.35486, 0.35486, 0.35486,
        0.39757, 0.39757, 0.39757,
        0.44029, 0.44029, 0.44029,
        0.48300, 0.48300, 0.48300,
        0.26786, 0.26786, 0.26786,
        0.31057, 0.31057, 0.31057,
        0.35329, 0.35329, 0.35329,
        0.39600, 0.39600, 0.39600,
        0.43871, 0.43871, 0.43871,
        0.48143, 0.48143, 0.48143,
        0.52414, 0.52414, 0.52414,
        0.56686, 0.56686, 0.56686,
        0.35171, 0.35171, 0.35171,
        0.39443, 0.39443, 0.39443,
        0.43714, 0.43714, 0.43714,
        0.47986, 0.47986, 0.47986,
        0.52257, 0.52257, 0.52257,
        0.56529, 0.56529, 0.56529,
        0.60800, 0.60800, 0.60800,
        0.65071, 0.65071, 0.65071,
        0.43557, 0.43557, 0.43557,
        0.47829, 0.47829, 0.47829,
        0.52100, 0.52100, 0.52100,
        0.56371, 0.56371, 0.56371,
        0.60643, 0.60643, 0.60643,
        0.64914, 0.64914, 0.64914,
        0.69186, 0.69186, 0.69186,
        0.73457, 0.73457, 0.73457,
        0.51943, 0.51943, 0.51943,
        0.56214, 0.56214, 0.56214,
        0.60486, 0.60486, 0.60486,
        0.64757, 0.64757, 0.64757,
        0.69029, 0.69029, 0.69029,
        0.73300, 0.73300, 0.73300,
        0.77571, 0.77571, 0.77571,
        0.81843, 0.81843, 0.81843,
        0.60329, 0.60329, 0.60329,
        0.64600, 0.64600, 0.64600,
        0.68871, 0.68871, 0.68871,
        0.73143, 0.73143, 0.73143,
        0.77414, 0.77414, 0.77414,
        0.81686, 0.81686, 0.81686,
        0.85957, 0.85957, 0.85957,
        0.90229, 0.90229, 0.90229,
        0.03257, 0.03257, 0.03257,
        0.07529, 0.07529, 0.07529,
        0.11800, 0.11800, 0.11800,
        0.16071, 0.16071, 0.16071,
        0.20343, 0.20343, 0.20343,
        0.24614, 0.24614, 0.24614,
        0.28886, 0.28886, 0.28886,
        0.33157, 0.33157, 0.33157,
        0.11643, 0.11643, 0.11643,
        0.15914, 0.15914, 0.15914,
        0.20186, 0.20186, 0.20186,
        0.24457, 0.24457, 0.24457,
        0.28729, 0.28729, 0.28729,
        0.33000, 0.33000, 0.33000,
        0.37271, 0.37271, 0.37271,
        0.41543, 0.41543, 0.41543,
        0.20029, 0.20029, 0.20029,
        0.24300, 0.24300, 0.24300,
        0.28571, 0.28571, 0.28571,
        0.32843, 0.32843, 0.32843,
        0.37114, 0.37114, 0.37114,
        0.41386, 0.41386, 0.41386,
        0.45657, 0.45657, 0.45657,
        0.49929, 0.49929, 0.49929,
        0.28414, 0.28414, 0.28414,
        0.32686, 0.32686, 0.32686,
        0.36957, 0.36957, 0.36957,
        0.41229, 0.41229, 0.41229,
        0.45500, 0.45500, 0.45500,
        0.49771, 0.49771, 0.49771,
        0.54043, 0.54043, 0.54043,
        0.58314, 0.58314, 0.58314,
        0.36800, 0.36800, 0.36800,
        0.41071, 0.41071, 0.41071,
        0.45343, 0.45343, 0.45343,
        0.49614, 0.49614, 0.49614,
        0.53886, 0.53886, 0.53886,
        0.58157, 0.58157, 0.58157,
        0.62429, 0.62429, 0.62429,
        0.66700, 0.66700, 0.66700,
        0.45186, 0.45186, 0.45186,
        0.49457, 0.49457, 0.49457,
        0.53729, 0.53729, 0.53729,
        0.58000, 0.58000, 0.58000,
        0.62271, 0.62271, 0.62271,
        0.66543, 0.66543, 0.66543,
        0.70814, 0.70814, 0.70814,
        0.75086, 0.75086, 0.75086,
        0.53571, 0.53571, 0.53571,
        0.57843, 0.57843, 0.57843,
        0.62114, 0.62114, 0.62114,
        0.66386, 0.66386, 0.66386,
        0.70657, 0.70657, 0.70657,
        0.74929, 0.74929, 0.74929,
        0.79200, 0.79200, 0.79200,
        0.83471, 0.83471, 0.83471,
        0.61957, 0.61957, 0.61957,
        0.66229, 0.66229, 0.66229,
        0.70500, 0.70500, 0.70500,
        0.74771, 0.74771, 0.74771,
        0.79043, 0.79043, 0.79043,
        0.83314, 0.83314, 0.83314,
        0.87586, 0.87586, 0.87586,
        0.91857, 0.91857, 0.91857,
        0.04886, 0.04886, 0.04886,
        0.09157, 0.09157, 0.09157,
        0.13429, 0.13429, 0.13429,
        0.17700, 0.17700, 0.17700,
        0.21971, 0.21971, 0.21971,
        0.26243, 0.26243, 0.26243,
        0.30514, 0.30514, 0.30514,
        0.34786, 0.34786, 0.34786,
        0.13271, 0.13271, 0.13271,
        0.17543, 0.17543, 0.17543,
        0.21814, 0.21814, 0.21814,
        0.26086, 0.26086, 0.26086,
        0.30357, 0.30357, 0.30357,
        0.34629, 0.34629, 0.34629,
        0.38900, 0.38900, 0.38900,
        0.43171, 0.43171, 0.43171,
        0.21657, 0.21657, 0.21657,
        0.25929, 0.25929, 0.25929,
        0.30200, 0.30200, 0.30200,
        0.34471, 0.34471, 0.34471,
        0.38743, 0.38743, 0.38743,
        0.43014, 0.43014, 0.43014,
        0.47286, 0.47286, 0.47286,
        0.51557, 0.51557, 0.51557,
        0.30043, 0.30043, 0.30043,
        0.34314, 0.34314, 0.34314,
        0.38586, 0.38586, 0.38586,
        0.42857, 0.42857, 0.42857,
        0.47129, 0.47129, 0.47129,
        0.51400, 0.51400, 0.51400,
        0.55671, 0.55671, 0.55671,
        0.59943, 0.59943, 0.59943,
        0.38429, 0.38429, 0.38429,
        0.42700, 0.42700, 0.42700,
        0.46971, 0.46971, 0.46971,
        0.51243, 0.51243, 0.51243,
        0.55514, 0.55514, 0.55514,
        0.59786, 0.59786, 0.59786,
        0.64057, 0.64057, 0.64057,
        0.68329, 0.68329, 0.68329,
        0.46814, 0.46814, 0.46814,
        0.51086, 0.51086, 0.51086,
        0.55357, 0.55357, 0.55357,
        0.59629, 0.59629, 0.59629,
        0.63900, 0.63900, 0.63900,
        0.68171, 0.68171, 0.68171,
        0.72443, 0.72443, 0.72443,
        0.76714, 0.76714, 0.76714,
        0.55200, 0.55200, 0.55200,
        0.59471, 0.59471, 0.59471,
        0.63743, 0.63743, 0.63743,
        0.68014, 0.68014, 0.68014,
        0.72286, 0.72286, 0.72286,
        0.76557, 0.76557, 0.76557,
        0.80829, 0.80829, 0.80829,
        0.85100, 0.85100, 0.85100,
        0.63586, 0.63586, 0.63586,
        0.67857, 0.67857, 0.67857,
        0.72129, 0.72129, 0.72129,
        0.76400, 0.76400, 0.76400,
        0.80671, 0.80671, 0.80671,
        0.84943, 0.84943, 0.84943,
        0.89214, 0.89214, 0.89214,
        0.93486, 0.93486, 0.93486,
        0.06514, 0.06514, 0.06514,
        0.10786, 0.10786, 0.10786,
        0.15057, 0.15057, 0.15057,
        0.19329, 0.19329, 0.19329,
        0.23600, 0.23600, 0.23600,
        0.27871, 0.27871, 0.27871,
        0.32143, 0.32143, 0.32143,
        0.36414, 0.36414, 0.36414,
        0.14900, 0.14900, 0.14900,
        0.19171, 0.19171, 0.19171,
        0.23443, 0.23443, 0.23443,
        0.27714, 0.27714, 0.27714,
        0.31986, 0.31986, 0.31986,
        0.36257, 0.36257, 0.36257,
        0.40529, 0.40529, 0.40529,
        0.44800, 0.44800, 0.44800,
        0.23286, 0.23286, 0.23286,
        0.27557, 0.27557, 0.27557,
        0.31829, 0.31829, 0.31829,
        0.36100, 0.36100, 0.36100,
        0.40371, 0.40371, 0.40371,
        0.44643, 0.44643, 0.44643,
        0.48914, 0.48914, 0.48914,
        0.53186, 0.53186, 0.53186,
        0.31671, 0.31671, 0.31671,
        0.35943, 0.35943, 0.35943,
        0.40214, 0.40214, 0.40214,
        0.44486, 0.44486, 0.44486,
        0.48757, 0.48757, 0.48757,
        0.53029, 0.53029, 0.53029,
        0.57300, 0.57300, 0.57300,
        0.61571, 0.61571, 0.61571,
        0.40057, 0.40057, 0.40057,
        0.44329, 0.44329, 0.44329,
        0.48600, 0.48600, 0.48600,
        0.52871, 0.52871, 0.52871,
        0.57143, 0.57143, 0.57143,
        0.61414, 0.61414, 0.61414,
        0.65686, 0.65686, 0.65686,
        0.69957, 0.69957, 0.69957,
        0.48443, 0.48443, 0.48443,
        0.52714, 0.52714, 0.52714,
        0.56986, 0.56986, 0.56986,
        0.61257, 0.61257, 0.61257,
        0.65529, 0.65529, 0.65529,
        0.69800, 0.69800, 0.69800,
        0.74071, 0.74071, 0.74071,
        0.78343, 0.78343, 0.78343,
        0.56829, 0.56829, 0.56829,
        0.61100, 0.61100, 0.61100,
        0.65371, 0.65371, 0.65371,
        0.69643, 0.69643, 0.69643,
        0.73914, 0.73914, 0.73914,
        0.78186, 0.78186, 0.78186,
        0.82457, 0.82457, 0.82457,
        0.86729, 0.86729, 0.86729,
        0.65214, 0.65214, 0.65214,
        0.69486, 0.69486, 0.69486,
        0.73757, 0.73757, 0.73757,
        0.78029, 0.78029, 0.78029,
        0.82300, 0.82300, 0.82300,
        0.86571, 0.86571, 0.86571,
        0.90843, 0.90843, 0.90843,
        0.95114, 0.95114, 0.95114,
        0.08143, 0.08143, 0.08143,
        0.12414, 0.12414, 0.12414,
        0.16686, 0.16686, 0.16686,
        0.20957, 0.20957, 0.20957,
        0.25229, 0.25229, 0.25229,
        0.29500, 0.29500, 0.29500,
        0.33771, 0.33771, 0.33771,
        0.38043, 0.38043, 0.38043,
        0.16529, 0.16529, 0.16529,
        0.20800, 0.20800, 0.20800,
        0.25071, 0.25071, 0.25071,
        0.29343, 0.29343, 0.29343,
        0.33614, 0.33614, 0.33614,
        0.37886, 0.37886, 0.37886,
        0.42157, 0.42157, 0.42157,
        0.46429, 0.46429, 0.46429,
        0.24914, 0.24914, 0.24914,
        0.29186, 0.29186, 0.29186,
        0.33457, 0.33457, 0.33457,
        0.37729, 0.37729, 0.37729,
        0.42000, 0.42000, 0.42000,
        0.46271, 0.46271, 0.46271,
        0.50543, 0.50543, 0.50543,
        0.54814, 0.54814, 0.54814,
        0.33300, 0.33300, 0.33300,
        0.37571, 0.37571, 0.37571,
        0.41843, 0.41843, 0.41843,
        0.46114, 0.46114, 0.46114,
        0.50386, 0.50386, 0.50386,
        0.54657, 0.54657, 0.54657,
        0.58929, 0.58929, 0.58929,
        0.63200, 0.63200, 0.63200,
        0.41686, 0.41686, 0.41686,
        0.45957, 0.45957, 0.45957,
        0.50229, 0.50229, 0.50229,
        0.54500, 0.54500, 0.54500,
        0.58771, 0.58771, 0.58771,
        0.63043, 0.63043, 0.63043,
        0.67314, 0.67314, 0.67314,
        0.71586, 0.71586, 0.71586,
        0.50071, 0.50071, 0.50071,
        0.54343, 0.54343, 0.54343,
        0.58614, 0.58614, 0.58614,
        0.62886, 0.62886, 0.62886,
        0.67157, 0.67157, 0.67157,
        0.71429, 0.71429, 0.71429,
        0.75700, 0.75700, 0.75700,
        0.79971, 0.79971, 0.79971,
        0.58457, 0.58457, 0.58457,
        0.62729, 0.62729, 0.62729,
        0.67000, 0.67000, 0.67000,
        0.71271, 0.71271, 0.71271,
        0.75543, 0.75543, 0.75543,
        0.79814, 0.79814, 0.79814,
        0.84086, 0.84086, 0.84086,
        0.88357, 0.88357, 0.88357,
        0.66843, 0.66843, 0.66843,
        0.71114, 0.71114, 0.71114,
        0.75386, 0.75386, 0.75386,
        0.79657, 0.79657, 0.79657,
        0.83929, 0.83929, 0.83929,
        0.88200, 0.88200, 0.88200,
        0.92471, 0.92471, 0.92471,
        0.96743, 0.96743, 0.96743,
        0.09771, 0.09771, 0.09771,
        0.14043, 0.14043, 0.14043,
        0.18314, 0.18314, 0.18314,
        0.22586, 0.22586, 0.22586,
        0.26857, 0.26857, 0.26857,
        0.31129, 0.31129, 0.31129,
        0.35400, 0.35400, 0.35400,
        0.39671, 0.39671, 0.39671,
        0.18157, 0.18157, 0.18157,
        0.22429, 0.22429, 0.22429,
        0.26700, 0.26700, 0.26700,
        0.30971, 0.30971, 0.30971,
        0.35243, 0.35243, 0.35243,
        0.39514, 0.39514, 0.39514,
        0.43786, 0.43786, 0.43786,
        0.48057, 0.48057, 0.48057,
        0.26543, 0.26543, 0.26543,
        0.30814, 0.30814, 0.30814,
        0.35086, 0.35086, 0.35086,
        0.39357, 0.39357, 0.39357,
        0.43629, 0.43629, 0.43629,
        0.47900, 0.47900, 0.47900,
        0.52171, 0.52171, 0.52171,
        0.56443, 0.56443, 0.56443,
        0.34929, 0.34929, 0.34929,
        0.39200, 0.39200, 0.39200,
        0.43471, 0.43471, 0.43471,
        0.47743, 0.47743, 0.47743,
        0.52014, 0.52014, 0.52014,
        0.56286, 0.56286, 0.56286,
        0.60557, 0.60557, 0.60557,
        0.64829, 0.64829, 0.64829,
        0.43314, 0.43314, 0.43314,
        0.47586, 0.47586, 0.47586,
        0.51857, 0.51857, 0.51857,
        0.56129, 0.56129, 0.56129,
        0.60400, 0.60400, 0.60400,
        0.64671, 0.64671, 0.64671,
        0.68943, 0.68943, 0.68943,
        0.73214, 0.73214, 0.73214,
        0.51700, 0.51700, 0.51700,
        0.55971, 0.55971, 0.55971,
        0.60243, 0.60243, 0.60243,
        0.64514, 0.64514, 0.64514,
        0.68786, 0.68786, 0.68786,
        0.73057, 0.73057, 0.73057,
        0.77329, 0.77329, 0.77329,
        0.81600, 0.81600, 0.81600,
        0.60086, 0.60086, 0.60086,
        0.64357, 0.64357, 0.64357,
        0.68629, 0.68629, 0.68629,
        0.72900, 0.72900, 0.72900,
        0.77171, 0.77171, 0.77171,
        0.81443, 0.81443, 0.81443,
        0.85714, 0.85714, 0.85714,
        0.89986, 0.89986, 0.89986,
        0.68471, 0.68471, 0.68471,
        0.72743, 0.72743, 0.72743,
        0.77014, 0.77014, 0.77014,
        0.81286, 0.81286, 0.81286,
        0.85557, 0.85557, 0.85557,
        0.89829, 0.89829, 0.89829,
        0.94100, 0.94100, 0.94100,
        0.98371, 0.98371, 0.98371,
        0.11400, 0.11400, 0.11400,
        0.15671, 0.15671, 0.15671,
        0.19943, 0.19943, 0.19943,
        0.24214, 0.24214, 0.24214,
        0.28486, 0.28486, 0.28486,
        0.32757, 0.32757, 0.32757,
        0.37029, 0.37029, 0.37029,
        0.41300, 0.41300, 0.41300,
        0.19786, 0.19786, 0.19786,
        0.24057, 0.24057, 0.24057,
        0.28329, 0.28329, 0.28329,
        0.32600, 0.32600, 0.32600,
        0.36871, 0.36871, 0.36871,
        0.41143, 0.41143, 0.41143,
        0.45414, 0.45414, 0.45414,
        0.49686, 0.49686, 0.49686,
        0.28171, 0.28171, 0.28171,
        0.32443, 0.32443, 0.32443,
        0.36714, 0.36714, 0.36714,
        0.40986, 0.40986, 0.40986,
        0.45257, 0.45257, 0.45257,
        0.49529, 0.49529, 0.49529,
        0.53800, 0.53800, 0.53800,
        0.58071, 0.58071, 0.58071,
        0.36557, 0.36557, 0.36557,
        0.40829, 0.40829, 0.40829,
        0.45100, 0.45100, 0.45100,
        0.49371, 0.49371, 0.49371,
        0.53643, 0.53643, 0.53643,
        0.57914, 0.57914, 0.57914,
        0.62186, 0.62186, 0.62186,
        0.66457, 0.66457, 0.66457,
        0.44943, 0.44943, 0.44943,
        0.49214, 0.49214, 0.49214,
        0.53486, 0.53486, 0.53486,
        0.57757, 0.57757, 0.57757,
        0.62029, 0.62029, 0.62029,
        0.66300, 0.66300, 0.66300,
        0.70571, 0.70571, 0.70571,
        0.74843, 0.74843, 0.74843,
        0.53329, 0.53329, 0.53329,
        0.57600, 0.57600, 0.57600,
        0.61871, 0.61871, 0.61871,
        0.66143, 0.66143, 0.66143,
        0.70414, 0.70414, 0.70414,
        0.74686, 0.74686, 0.74686,
        0.78957, 0.78957, 0.78957,
        0.83229, 0.83229, 0.83229,
        0.61714, 0.61714, 0.61714,
        0.65986, 0.65986, 0.65986,
        0.70257, 0.70257, 0.70257,
        0.74529, 0.74529, 0.74529,
        0.78800, 0.78800, 0.78800,
        0.83071, 0.83071, 0.83071,
        0.87343, 0.87343, 0.87343,
        0.91614, 0.91614, 0.91614,
        0.70100, 0.70100, 0.70100,
        0.74371, 0.74371, 0.74371,
        0.78643, 0.78643, 0.78643,
        0.82914, 0.82914, 0.82914,
        0.87186, 0.87186, 0.87186,
        0.91457, 0.91457, 0.91457,
        0.95729, 0.95729, 0.95729,
        1.00000, 1.00000, 1.00000
    };

    // check cube data
    for(unsigned int i = 0; i < looklut->lut3D->lut.size(); ++i) {
        OIIO_CHECK_CLOSE(cube[i], looklut->lut3D->lut[i], 1e-4);
    }
}


OIIO_ADD_TEST(FileFormatIridasLook, fail_on_mask)
{
    std::ostringstream strebuf;
    strebuf << "<?xml version=\"1.0\" ?>" << "\n";
    strebuf << "<look>" << "\n";
    strebuf << "  <shaders>" << "\n";
    strebuf << "    <base>" << "\n";
    strebuf << "      <rangeversion>\"2\"</rangeversion>" << "\n";
    strebuf << "      <visible>\"1\"</visible>" << "\n";
    strebuf << "      <sublayer0>" << "\n";
    strebuf << "        <opacity>\"1\"</opacity>" << "\n";
    strebuf << "        <parameters>" << "\n";
    strebuf << "          <LogPrinterLights>\"N1\"</LogPrinterLights>" << "\n";
    strebuf << "        </parameters>" << "\n";
    strebuf << "      </sublayer0>" << "\n";
    strebuf << "      <sublayer3>" << "\n";
    strebuf << "        <opacity>\"1\"</opacity>" << "\n";
    strebuf << "        <parameters>" << "\n";
    strebuf << "          <gamma.Z>\"0.49967\"</gamma.Z>" << "\n";
    strebuf << "          <gain.Z>\"0.28739\"</gain.Z>" << "\n";
    strebuf << "          <gamma.Y>\"0.49179\"</gamma.Y>" << "\n";
    strebuf << "          <gain.Y>\"0.22243\"</gain.Y>" << "\n";
    strebuf << "          <gain.X>\"0.34531\"</gain.X>" << "\n";
    strebuf << "          <gamma.X>\"0.39388\"</gamma.X>" << "\n";
    strebuf << "        </parameters>" << "\n";
    strebuf << "      </sublayer3>" << "\n";
    strebuf << "    </base>" << "\n";
    strebuf << "  </shaders>" << "\n";
    strebuf << "  <mask>" << "\n";
    strebuf << "    <name>\"Untitled00_00_00_00\"</name>" << "\n";
    strebuf << "    <activecontour>\"0\"</activecontour>" << "\n";
    strebuf << "    <width>\"1024\"</width>" << "\n";
    strebuf << "    <height>\"778\"</height>" << "\n";
    strebuf << "    <contour>" << "\n";
    strebuf << "      <positive>\"1\"</positive>" << "\n";
    strebuf << "      <point>" << "\n";
    strebuf << "        <inner>\"catmull-rom,value:317.5,583.5@0\"</inner>" << "\n";
    strebuf << "        <innerprevtangent>\"catmull-rom,value:0,0@0\"</innerprevtangent>" << "\n";
    strebuf << "        <innernexttangent>\"catmull-rom,value:0,0@0\"</innernexttangent>" << "\n";
    strebuf << "        <falloffexponent>\"catmull-rom,value:1@0\"</falloffexponent>" << "\n";
    strebuf << "        <falloffweight>\"catmull-rom,value:0.5@0\"</falloffweight>" << "\n";
    strebuf << "        <detached>linear,value:0@0</detached>" << "\n";
    strebuf << "        <outer>\"catmull-rom,value:317.5,583.5@0\"</outer>" << "\n";
    strebuf << "        <outerprevtangent>\"catmull-rom,value:0,0@0\"</outerprevtangent>" << "\n";
    strebuf << "        <outernexttangent>\"catmull-rom,value:0,0@0\"</outernexttangent>" << "\n";
    strebuf << "        <spline>\"linear,value:0@0\"</spline>" << "\n";
    strebuf << "        <smooth>\"linear,value:0@0\"</smooth>" << "\n";
    strebuf << "      </point>" << "\n";
    strebuf << "    </contour>" << "\n";
    strebuf << "  </mask>" << "\n";
    strebuf << "  <LUT>" << "\n";
    strebuf << "    <size>\"8\"</size>" << "\n";
    strebuf << "    <data>\"" << "\n";
    strebuf << "      000000000000000000000000878B933D000000000000000057BC563E00000000" << "\n";
    strebuf << "      ...truncated, should never be parsed due to mask section\"" << "\n";
    strebuf << "    </data>" << "\n";
    strebuf << "  </LUT>" << "\n";
    strebuf << "</look>" << "\n";

    std::istringstream infile;
    infile.str(strebuf.str());

    // Read file
    LocalFileFormat tester;
    std::string emptyString;

    OIIO_CHECK_THROW_WHAT(
        tester.Read(infile, emptyString),
        Exception, "Cannot load .look LUT containing mask");

}

#endif // OCIO_UNIT_TEST

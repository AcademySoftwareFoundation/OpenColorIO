// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstdio>
#include <cstring>
#include <iterator>

#include <OpenColorIO/OpenColorIO.h>

#include "expat.h"
#include "fileformats/FileFormatUtils.h"
#include "ops/lut1d/Lut1DOp.h"
#include "ops/lut3d/Lut3DOp.h"
#include "ParseUtils.h"
#include "Platform.h"
#include "pystring/pystring.h"
#include "transforms/FileTransform.h"
#include "utils/StringUtils.h"


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
characters at a time and interpreting as little-endian float, as follows:

Given the string "0000003F0000803FAD10753F":

    >>> import binascii, struct
    >>> struct.unpack("<f", binascii.unhexlify("0000003F"))[0]
    0.5
    >>> struct.unpack("<f", binascii.unhexlify("0000803F"))[0]
    1.0
    >>> struct.unpack("<f", binascii.unhexlify("AD10753F"))[0]
    0.9572857022285461

 */

namespace OCIO_NAMESPACE
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

class XMLParserHelper
{
public:
    XMLParserHelper() = delete;
    XMLParserHelper(const XMLParserHelper &) = delete;
    XMLParserHelper & operator=(const XMLParserHelper &) = delete;

    explicit XMLParserHelper(const std::string & fileName)
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
            StringUtils::ReplaceInPlace(what, " ", "");
            StringUtils::ReplaceInPlace(what, "\"", "");
            StringUtils::ReplaceInPlace(what, "'", "");
            StringUtils::ReplaceInPlace(what, "\n", "");
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

class LocalCachedFile : public CachedFile
{
public:
    LocalCachedFile () = default;
    ~LocalCachedFile()  = default;

    Lut3DOpDataRcPtr lut3D;
};

typedef OCIO_SHARED_PTR<LocalCachedFile> LocalCachedFileRcPtr;

class LocalFileFormat : public FileFormat
{
public:
    LocalFileFormat() = default;
    ~LocalFileFormat() = default;

    void getFormatInfo(FormatInfoVec & formatInfoVec) const override;

    CachedFileRcPtr read(std::istream & istream,
                         const std::string & fileName,
                         Interpolation interp) const override;

    void buildFileOps(OpRcPtrVec & ops,
                        const Config & config,
                        const ConstContextRcPtr & context,
                        CachedFileRcPtr untypedCachedFile,
                        const FileTransform & fileTransform,
                        TransformDirection dir) const override;
};

void LocalFileFormat::getFormatInfo(FormatInfoVec & formatInfoVec) const
{
    FormatInfo info;
    info.name = "iridas_look";
    info.extension = "look";
    info.capabilities = FORMAT_CAPABILITY_READ;
    formatInfoVec.push_back(info);
}

CachedFileRcPtr LocalFileFormat::read(std::istream & istream,
                                      const std::string & fileName,
                                      Interpolation interp) const
{
    XMLParserHelper parser(fileName);
    parser.Parse(istream);

    // TODO: There is a LUT1D section in some .look files,
    // which we could use if available. Need to check
    // assumption that it is only written for 1D transforms,
    // and it matches the desired output

    // Validate LUT sizes, and create cached file object
    LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());

    int gridSize = 0;
    std::vector<float> raw;
    parser.getLut(gridSize, raw);

    cachedFile->lut3D = std::make_shared<Lut3DOpData>(gridSize);
    if (Lut3DOpData::IsValidInterpolation(interp))
    {
        cachedFile->lut3D->setInterpolation(interp);
    }
    cachedFile->lut3D->setFileOutputBitDepth(BIT_DEPTH_F32);
    cachedFile->lut3D->setArrayFromRedFastestOrder(raw);

    return cachedFile;
}


void
LocalFileFormat::buildFileOps(OpRcPtrVec & ops,
                                const Config& /*config*/,
                                const ConstContextRcPtr & /*context*/,
                                CachedFileRcPtr untypedCachedFile,
                                const FileTransform& fileTransform,
                                TransformDirection dir) const
{
    LocalCachedFileRcPtr cachedFile = DynamicPtrCast<LocalCachedFile>(untypedCachedFile);

    // This should never happen.
    if(!cachedFile || !cachedFile->lut3D)
    {
        std::ostringstream os;
        os << "Cannot build Iridas .look Op. Invalid cache type.";
        throw Exception(os.str().c_str());
    }

    const auto newDir = CombineTransformDirections(dir, fileTransform.getDirection());

    const auto fileInterp = fileTransform.getInterpolation();

    bool fileInterpUsed = false;
    auto lut3D = HandleLUT3D(cachedFile->lut3D, fileInterp, fileInterpUsed);

    if (!fileInterpUsed)
    {
        LogWarningInterpolationNotUsed(fileInterp, fileTransform);
    }

    CreateLut3DOp(ops, lut3D, newDir);
}
} // namespace

FileFormat * CreateFileFormatIridasLook()
{
    return new LocalFileFormat();
}

} // namespace OCIO_NAMESPACE


// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <iostream>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "fileformats/FileFormatUtils.h"
#include "MathUtils.h"
#include "ops/lut1d/Lut1DOp.h"
#include "ops/lut3d/Lut3DOp.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"
#include "Platform.h"
#include "transforms/FileTransform.h"
#include "utils/StringUtils.h"


// This format is a 1D LUT format that was used by the Discreet (now Autodesk)
// creative finishing products such as Flame and Smoke.
// This format is now deprecated (but still supported) in those products.
// It has been supplanted by the Academy CLF/CTF format.

namespace OCIO_NAMESPACE
{
namespace
{
void ReplaceTabsAndStripSpaces(char * stringToStrip)
{

    short source = -1;
    short destination = 0;

    if (*stringToStrip)
    {
        // Find end
        while (stringToStrip[++source])
        {
            if (stringToStrip[source] == 9) // TAB
                stringToStrip[source] = ' ';
        }

        // Find Last Non Blank
        while (--source >= 0 && stringToStrip[source] == ' ')
            /*do nothing*/;

        // Chop end
        if (stringToStrip[++source] != 0)
            stringToStrip[source] = 0;

        source = -1;

        // Find First Non Blank 
        while (stringToStrip[++source] == ' ') {}

        if (destination != source)
        {
            // Copy 
            while ((stringToStrip[destination++]
                = stringToStrip[source++])) {
            }
        }
    }
}

void StripEndNewLine(char * stringToStrip)
{
    size_t length = std::strlen(stringToStrip);
    if (length)
    {
        length--;
        // Test for line feed and CR (both windows and linux)
        if (stringToStrip[length] == 10 || stringToStrip[length] == 13)
            stringToStrip[length] = 0;
    }
}

class Lut1dUtils
{
public:
    //
    // Defined values of supported LUT formats.
    // this enumerator is mapped onto the values defined
    // in IM_BitsPerChannel
    //
    typedef enum {
        IM_LUT_UNKNOWN_BITS_PERCHANNEL = 0,
        IM_LUT_8BITS_PERCHANNEL = 8,
        IM_LUT_10BITS_PERCHANNEL = 10,
        IM_LUT_12BITS_PERCHANNEL = 12,
        IM_LUT_16BITS_PERCHANNEL = 16,
        IM_LUT_HALFBITS_PERCHANNEL = -16,
        IM_LUT_FLOATBITS_PERCHANNEL = -32
    } IM_LutBitsPerChannel;


    static BitDepth GetBitDepth(IM_LutBitsPerChannel discreetBitDepth)
    {
        switch (discreetBitDepth)
        {
        case IM_LUT_UNKNOWN_BITS_PERCHANNEL:
            return BIT_DEPTH_UNKNOWN;
        case IM_LUT_8BITS_PERCHANNEL:
            return BIT_DEPTH_UINT8;
        case IM_LUT_10BITS_PERCHANNEL:
            return BIT_DEPTH_UINT10;
        case IM_LUT_12BITS_PERCHANNEL:
            return BIT_DEPTH_UINT12;
        case IM_LUT_16BITS_PERCHANNEL:
            return BIT_DEPTH_UINT16;
        case IM_LUT_HALFBITS_PERCHANNEL:
            return BIT_DEPTH_F16;
        case IM_LUT_FLOATBITS_PERCHANNEL:
            return BIT_DEPTH_F32;
        }

        return BIT_DEPTH_UNKNOWN;
    }

    // A look-up table descriptor: 
    class IMLutStruct {
    public:
        int numtables; // Number of tables.
        int length;  // Length of each table.
        IM_LutBitsPerChannel srcBitDepth;
        IM_LutBitsPerChannel targetBitDepth;  // Hint if this is a resizing LUT
        unsigned short ** tables; // Points to an array of "numtables"
                                    // pointers to tables of length "length".

        IMLutStruct() : numtables(0), tables(0) {}
        IMLutStruct(const IMLutStruct &) = delete;
        IMLutStruct& operator=(const IMLutStruct &) = delete;

        ~IMLutStruct();
    };

    // Image LUT library return codes: 
    enum {
        IMLUT_OK = 0,
        IMLUT_ERR_UNEXPECTED_EOF,
        IMLUT_ERR_CANNOT_OPEN,
        IMLUT_ERR_CANNOT_MALLOC,
        IMLUT_ERR_SYNTAX
    };

    // Convert between table size and bit depth
    static IM_LutBitsPerChannel IMLutTableSizeToBitDepth(int tableSize,
                                                            bool isFloat = false);

    // Supply appropriate message string given IMLUT_ERR code.
    static const char * IMLutErrorStr(int errnum);

    // Free an image look-up table descriptor.
    static void IMLutFree(IMLutStruct **lut);

    // Allocate memory for an image look-up table descriptor.
    static bool IMLutAlloc(IMLutStruct **plut, int num, int length);

    // Attempt to open and read a file as an image look-up table.
    // If successful, allocate and return a "LUT", a look-up table
    // descriptor, otherwise return appropriate error status and line
    // of input file at which error occurred.
    static int IMLutGet(
        std::istream & istream,
        const std::string & fileName,
        IMLutStruct **lut, int & line, std::string & errorLine);

    // Determines the bitdepth of a LUT given it's filename Searches for
    // the first occurrence of the "to" sequence of characters in the
    // LutFileName string and then parses the numeric characters.
    // This function is useful for figuring out the target bitdepth
    // of resizing LUT if the filename is an indicator of this
    static IM_LutBitsPerChannel IMLutGetBitDepthFromFileName(const std::string & fileName);

    // Get maximum value in the table based on the bit depth
    static float GetMax(IM_LutBitsPerChannel lutBitDepth);

};


Lut1dUtils::IM_LutBitsPerChannel Lut1dUtils::IMLutTableSizeToBitDepth(
    int tableSize,
    bool isFloat)
{
    switch (tableSize)
    {
    case (256):
        return IM_LUT_8BITS_PERCHANNEL;

    case (1024):
        return IM_LUT_10BITS_PERCHANNEL;

    case (4096):
        return IM_LUT_12BITS_PERCHANNEL;

    case (65536):
        return isFloat ? IM_LUT_HALFBITS_PERCHANNEL : IM_LUT_16BITS_PERCHANNEL;

    default:
        return IM_LUT_UNKNOWN_BITS_PERCHANNEL;
    }
}

// Free an image look-up table descriptor.
Lut1dUtils::IMLutStruct::~IMLutStruct()
{
    if (tables)
    {
        for (int i = 0; i < numtables; i++)
        {
            if (tables[i])
            {
                // in case there was only 1 table in the file
                // the pointer might have been copied.
                int j;
                for (j = 0; j < i; j++)
                {
                    if (tables[j] == tables[i])
                    {
                        break;
                    }
                }
                if (j == i)
                {
                    free(tables[i]);
                    // do not NULL the value (loop above needs it).
                }
            }
        }
        free(tables);
        tables = 0;
    }
}

void Lut1dUtils::IMLutFree(IMLutStruct **lut)
{
    if (*lut) {
        delete *lut;
        *lut = NULL;
    }
}

// Allocate and initialize a look-up table descriptor.
// If successful, return TRUE.
// NOTE:
// *plut must be NULL when this routine is called!
bool Lut1dUtils::IMLutAlloc(IMLutStruct **plut, int num, int length)
{
    int i;
    IMLutStruct *lut;

    if (num < 0 || length < 0)
        return false;

    lut = new IMLutStruct;
    if (lut == NULL)
        return false;
    lut->tables = NULL;
    lut->numtables = num;
    lut->length = length;

    // On import, we never supported LUTs with 16bit integer input.
    // (16bit integer input was interpreted as 12bit.)
    // On export, 16bit input is necessarily float.
    const bool src16bitDepthIsFloat = true;
    lut->srcBitDepth = IMLutTableSizeToBitDepth(length, src16bitDepthIsFloat);

    // targetBitDepth will be set appropriately for conversion LUTs in IMLutGet
    lut->targetBitDepth = IMLutTableSizeToBitDepth(length);

    lut->tables = (unsigned short **)malloc(4 * sizeof(unsigned short *));
    if (lut->tables == NULL)
    {
        delete lut;
        lut = 0;
        return false;
    }
    for (i = 0; i < num; i++)
        lut->tables[i] = NULL;
    for (i = 0; i < num; i++)
    {
        lut->tables[i] = (unsigned short *)malloc(length * sizeof(unsigned short));
        if (lut->tables[i] == NULL)
        {
            delete lut;
            lut = 0;
            return false;
        }
    }
    *plut = lut;
    return true;
}

// Load values from stream into table "ptable".
// Return an error status.
static int tableLoad(
    std::istream & istream,
    unsigned short *ptable, // Destination table.
    int length,             // Length of ptable.
    int ptablestart,        // Start at ptable[ptablestart].
    int & line,             // Last line successfully read.
    std::string & errorLine // Line content in case of syntax err
)
{
    char InString[200];
    int Count = ptablestart;

    while (istream.good())
    {
        line += 1;
        istream.getline(InString, 200);
        if (!istream.good())
            return Lut1dUtils::IMLUT_ERR_UNEXPECTED_EOF;

        ReplaceTabsAndStripSpaces(InString);
        StripEndNewLine(InString);

        if (isdigit(*InString))
        {
            ptable[Count++] = (unsigned short)std::stoi(InString);
            if (Count >= length)
                break;
        }
        else if (*InString != 0)
        {
            errorLine = InString;
            return Lut1dUtils::IMLUT_ERR_SYNTAX;
        }
    }
    return Lut1dUtils::IMLUT_OK;
}

// Find first line that is not blank or a comment:
static bool FindNonComment(
    std::istream & istream,
    int & line,
    char InString[],
    int length // Length of InString.
)
{
    bool readOn = true;
    while (istream.good() && readOn)
    {
        readOn = false;
        istream.getline(InString, length);
        line += 1;

        ReplaceTabsAndStripSpaces(InString);
        StripEndNewLine(InString);
        if (InString[0] == '\0' || InString[0] == '#')
            readOn = true;
    }
    return istream.good();
}

// Attempt to open and read a file as an image look-up table.
// If successful, allocate and return "plut", a look-up table
// descriptor, otherwise return appropriate error status and line
// of input file at which error occurred.
int Lut1dUtils::IMLutGet(
    std::istream & istream,
    const std::string & fileName,
    IMLutStruct **plut,
    int & line,
    std::string & errorLine)
{
    char InString[200];
    IMLutStruct * lut = NULL;
    int numtables;
    int length;
    int tablestart;
    int status = IMLUT_OK;
    int i;
    IM_LutBitsPerChannel depthScaled = IM_LUT_UNKNOWN_BITS_PERCHANNEL;

    line = 0;

    // Find first line that is not blank or a comment:
    if (!FindNonComment(istream, line, InString, 200))
    {
        status = IMLUT_ERR_UNEXPECTED_EOF;
        *plut = 0;
        goto load_abort;
    }

    if (isdigit(*InString))
    {
        // Old format LUT file:  1 table of 256 entries.
        numtables = 1;
        length = 256;

        if (!IMLutAlloc(&lut, numtables, length))
        {
            status = IMLUT_ERR_CANNOT_MALLOC;
            *plut = 0;
            goto load_abort;
        }

        // Load first table value.
        (lut->tables[0])[0] = (unsigned short)std::stoi(InString);
        tablestart = 1;
    }
    else
    {
        char dstDepthS[16] = "";
#ifdef _WIN32
        const int nummatched = sscanf(InString, "%*s %d %d %s", &numtables, &length, dstDepthS, 16);
#else
        const int nummatched = sscanf(InString, "%*s %d %d %s", &numtables, &length, dstDepthS);
#endif
        std::string subStr(InString, 5);
        if (nummatched < 2 ||
            StringUtils::Lower(subStr) != "lut: " ||
            (numtables != 1 && numtables != 3 && numtables != 4) ||
            length <= 0)
        {
            errorLine = InString;
            status = IMLUT_ERR_SYNTAX;
            *plut = 0;
            goto load_abort;
        }

        // New format LUT file:  "numtables" tables, each of
        // "length" entries. Optional dstDepth.
        if (nummatched > 2)
        {
            // Optional dstDepth was specified. Validate it.
            int dstDepth = 0;
            char floatC = ' ';
#ifdef _WIN32
            sscanf(dstDepthS, "%d%c", &dstDepth, &floatC, 1);
#else
            sscanf(dstDepthS, "%d%c", &dstDepth, &floatC);
#endif

            // Currently when Smoke exports a 16f output depth it uses "65536f"
            // as the third token. However it is likely that earlier versions either
            // wrote only two tokens or wrote the third token without the "f". In that
            // case we may wrongly interpret a 16f outDepth as 16i. We may want to
            // investigate this further at some point.

            depthScaled = IMLutTableSizeToBitDepth(dstDepth,
                floatC == 'f' || floatC == 'F');
            if (depthScaled == IM_LUT_UNKNOWN_BITS_PERCHANNEL)
            {
                errorLine = InString;
                status = IMLUT_ERR_SYNTAX;
                *plut = 0;
                goto load_abort;
            }
        }

        if (!IMLutAlloc(&lut, numtables, length))
        {
            status = IMLUT_ERR_CANNOT_MALLOC;
            *plut = 0;
            goto load_abort;
        }
        tablestart = 0;
    }

    for (i = 0; i < numtables; i++)
    {
        status = tableLoad(
            istream,
            lut->tables[i],
            length,
            tablestart, 
            line, errorLine);
        if (status != IMLUT_OK)
        {
            IMLutFree(&lut);
            *plut = 0;
            goto load_abort;
        }
    }

    if (numtables == 1)
    {
        lut->numtables = 3;
        lut->tables[2] = lut->tables[1] = lut->tables[0];
    }

    if (depthScaled == IM_LUT_UNKNOWN_BITS_PERCHANNEL)
    {
        depthScaled = IMLutGetBitDepthFromFileName(fileName.c_str());
    }

    if (IM_LUT_UNKNOWN_BITS_PERCHANNEL != depthScaled)
    {
        lut->targetBitDepth = depthScaled;
    }

    // If there are any more lines in the file that are not blank
    // or comments, it's a syntax error:
    if (FindNonComment(istream, line, InString, 200))
    {
        errorLine = InString;
        status = IMLUT_ERR_SYNTAX;
        IMLutFree(&lut);
        *plut = 0;
        goto load_abort;
    }

    *plut = lut;

load_abort:
    return status;
}

// Parses the filename and attempts to determine the bitdepth
// of the LUT
Lut1dUtils::IM_LutBitsPerChannel Lut1dUtils::IMLutGetBitDepthFromFileName(const std::string & fileName)
{
    if (fileName.empty())
    {
        return IM_LUT_UNKNOWN_BITS_PERCHANNEL;
    }

    std::string lowerFileName(StringUtils::Lower(fileName));

    // Get the export depth from the LUT name.  Look for a bit depth
    // after the "to" string. (ex: 12to10log).
    const char* tokenStr;
    if ((tokenStr = std::strstr(lowerFileName.c_str(), "to")))
    {
        // Skip the "to";
        tokenStr += 2;
        char firstChar = *tokenStr;
        if (firstChar == '8')
        {
            return IM_LUT_8BITS_PERCHANNEL;
        }
        else if (firstChar == '1')
        {
            // Advance one character.
            tokenStr++;
            char secondChar = *tokenStr;
            if (secondChar == '0')
            {
                return IM_LUT_10BITS_PERCHANNEL;
            }
            else if (secondChar == '2')
            {
                return IM_LUT_12BITS_PERCHANNEL;
            }
            else if (secondChar == '6')
            {
                // check for 16fp
                tokenStr++;
                char thirdChar = *tokenStr;
                if (thirdChar == 'f' || thirdChar == 'F')
                {
                    return IM_LUT_HALFBITS_PERCHANNEL;
                }
                else
                {
                    return IM_LUT_16BITS_PERCHANNEL;
                }
            }
        }
        else if (firstChar == '3')
        {
            tokenStr++;
            char secondChar = *tokenStr;

            if (secondChar == '2')
            {
                tokenStr++;
                char thirdChar = *tokenStr;
                if (thirdChar == 'f' || thirdChar == 'F')
                {
                    return IM_LUT_FLOATBITS_PERCHANNEL;
                }
            }
        }
    }

    return IM_LUT_UNKNOWN_BITS_PERCHANNEL;
}

// Supply appropriate message string given IMLUT_ERR status code.
const char * Lut1dUtils::IMLutErrorStr(int errnum)
{
    switch (errnum) 
    {
    case IMLUT_OK:
        return "";
    case IMLUT_ERR_UNEXPECTED_EOF:
        return "Premature EOF reading LUT file";
    case IMLUT_ERR_CANNOT_OPEN:
        return "Cannot open LUT file";
    case IMLUT_ERR_CANNOT_MALLOC:
        return "Cannot allocate memory for LUT";
    case IMLUT_ERR_SYNTAX:
        return "Syntax error reading LUT file";
    default:
        return "Unknown error for LUT";
    }
}

float Lut1dUtils::GetMax(IM_LutBitsPerChannel lutBitDepth)
{
    switch (lutBitDepth)
    {
    case IM_LUT_8BITS_PERCHANNEL:
        return 255.0f;

    case IM_LUT_10BITS_PERCHANNEL:
        return 1023.0f;

    case IM_LUT_12BITS_PERCHANNEL:
        return 4095.0f;

    case IM_LUT_16BITS_PERCHANNEL:
    case IM_LUT_HALFBITS_PERCHANNEL:
        return 65535.0f;

    case IM_LUT_FLOATBITS_PERCHANNEL:
    case IM_LUT_UNKNOWN_BITS_PERCHANNEL:
        return 1.0f;
    }

    return 1.0f;
}

class LocalCachedFile : public CachedFile
{
public:
    LocalCachedFile() = delete;
    LocalCachedFile(BitDepth inBitDepth,
                    BitDepth outBitDepth,
                    unsigned long dimension,
                    Interpolation interp)
    {
        const Lut1DOpData::HalfFlags halfFlags =
            (inBitDepth == BIT_DEPTH_F16)
            ? Lut1DOpData::LUT_INPUT_HALF_CODE
            : Lut1DOpData::LUT_STANDARD;

        lut1D = std::make_shared<Lut1DOpData>(halfFlags, dimension, false);
        if (Lut1DOpData::IsValidInterpolation(interp))
        {
            lut1D->setInterpolation(interp);
        }
        lut1D->setFileOutputBitDepth(outBitDepth);
    };
    ~LocalCachedFile() = default;

    Lut1DOpDataRcPtr lut1D;
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
    info.name = "Discreet 1D LUT";
    info.extension = "lut";
    info.capabilities = FORMAT_CAPABILITY_READ;
    formatInfoVec.push_back(info);
}

// Try and load the format
// Raise an exception if it can't be loaded.

CachedFileRcPtr LocalFileFormat::read(std::istream & istream,
                                      const std::string & filePath,
                                      Interpolation interp) const
{
    Lut1dUtils::IMLutStruct *discreetLut1d = 0x0;
    int errline;
    std::string errorLine;
    std::string root, extension, fileName;
    pystring::os::path::splitext(root, extension, filePath);
    fileName = pystring::os::path::basename(root);

    const int status = Lut1dUtils::IMLutGet(istream,
                                            fileName,
                                            &discreetLut1d,
                                            errline,
                                            errorLine);
    if (status != Lut1dUtils::IMLUT_OK)
    {
        std::ostringstream os;
        os << "Error parsing .lut file (";
        os << filePath.c_str() << ") ";
        os << "using Discreet 1D LUT reader. ";
        os << "Error is: " << Lut1dUtils::IMLutErrorStr(status);
        if (Lut1dUtils::IMLUT_ERR_SYNTAX == status)
        {
            os << " At line (" << errline << "): '";
            os << errorLine << "'.";
        }
        throw Exception(os.str().c_str());
    }

    const BitDepth inputBD = Lut1dUtils::GetBitDepth(discreetLut1d->srcBitDepth);
    const BitDepth outputBD = Lut1dUtils::GetBitDepth(discreetLut1d->targetBitDepth);
    const int lutSize = discreetLut1d->length;

    LocalCachedFileRcPtr cachedFile
        = LocalCachedFileRcPtr(new LocalCachedFile(inputBD,
                                                   outputBD,
                                                   (unsigned long)lutSize,
                                                   interp));

    const float scale = (float)GetBitDepthMaxValue(outputBD);
    Array & array = cachedFile->lut1D->getArray();
    const int srcTableLimit = discreetLut1d->numtables - 1;
    for (int i = 0, p = 0; i< lutSize; ++i)
    {
        for (int j = 0; j<3; ++j, ++p )
        {
            const int srcTable = std::min(j, srcTableLimit);
            if (discreetLut1d->targetBitDepth == Lut1dUtils::IM_LUT_HALFBITS_PERCHANNEL)
            {
                // Convert raw half values to floats.
                half halfObj;
                halfObj.setBits((unsigned short)discreetLut1d->tables[srcTable][i]);
                array[p] = (float)halfObj;
            }
            else
            {
                array[p] = (float)discreetLut1d->tables[srcTable][i] / scale;
            }
        }
    }

    Lut1dUtils::IMLutFree(&discreetLut1d);
    return cachedFile;
}

void
LocalFileFormat::buildFileOps(OpRcPtrVec & ops,
                                const Config & /*config*/,
                                const ConstContextRcPtr & /*context*/,
                                CachedFileRcPtr untypedCachedFile,
                                const FileTransform & fileTransform,
                                TransformDirection dir) const
{
    LocalCachedFileRcPtr cachedFile = DynamicPtrCast<LocalCachedFile>(untypedCachedFile);

    // This should never happen.
    if(!cachedFile || !cachedFile->lut1D)
    {
        std::ostringstream os;
        os << "Cannot build .lut Op. Invalid cache type.";
        throw Exception(os.str().c_str());
    }

    const auto newDir = CombineTransformDirections(dir, fileTransform.getDirection());
    const auto fileInterp = fileTransform.getInterpolation();

    bool fileInterpUsed = false;
    auto lut1D = HandleLUT1D(cachedFile->lut1D, fileInterp, fileInterpUsed);

    if (!fileInterpUsed)
    {
        LogWarningInterpolationNotUsed(fileInterp, fileTransform);
    }

    CreateLut1DOp(ops, lut1D, newDir);
}
}

FileFormat * CreateFileFormatDiscreet1DL()
{
    return new LocalFileFormat();
}

} // namespace OCIO_NAMESPACE


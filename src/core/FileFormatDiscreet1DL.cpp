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

#include "FileTransform.h"
#include "Lut1DOp.h"
#include "opdata/OpDataLut1D.h"
#include "Lut3DOp.h"
#include "MathUtils.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"
#include "Platform.h"
#include "half.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>


#include <iostream>


// This format is a 1D LUT format that was used by the Discreet (now Autodesk)
// creative finishing products such as Flame and Smoke.
// This format is now deprecated (but still supported) in those products.
// It has been supplanted by the Academy CLF/CTF format.

OCIO_NAMESPACE_ENTER
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
            size_t length = strlen(stringToStrip);
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

            // A look-up table descriptor: 
            class IMLutStruct {
            public:
                int numtables; // Number of tables.
                int length;  // Length of each table.
                IM_LutBitsPerChannel srcBitDepth;
                IM_LutBitsPerChannel targetBitDepth;  // Hint if this is a resizing LUT
                unsigned short ** tables; // Points to an array of "numtables"
                                          // pointers to tables of length "length".

                IMLutStruct()
                    : numtables(0)
                    , length(0)
                    , srcBitDepth(IM_LUT_UNKNOWN_BITS_PERCHANNEL)
                    , targetBitDepth(IM_LUT_UNKNOWN_BITS_PERCHANNEL)
                    , tables(0)
                {
                }
                ~IMLutStruct();
            };

            // Image LUT library return codes: 
            enum {
                IMLUT_OK = 0,
                IMLUT_ERR_MISSING_ENTRIES,
                IMLUT_ERR_CANNOT_OPEN,
                IMLUT_ERR_CANNOT_MALLOC,
                IMLUT_ERR_SYNTAX,
                IMLUT_ERR_NO_LUT
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
            // the first occurence of the "to" sequence of characters in the
            // LutFileName string and then parses the numeric characters.
            // This function is usefull for figuring out the target bitdepth
            // of resizing LUT if the filename is an indicator of this
            static IM_LutBitsPerChannel IMLutGetBitDepthFromFileName(const std::string & fileName);

            // Get maximum value in the table based on the bit depth
            static float GetMax(IM_LutBitsPerChannel lutBitDepth);

            // Convert to OCIO bit depth
            static BitDepth ConvertBitDepth(IM_LutBitsPerChannel lutBitDepth);
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

            return IM_LUT_UNKNOWN_BITS_PERCHANNEL;
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
                    return Lut1dUtils::IMLUT_ERR_MISSING_ENTRIES;

                ReplaceTabsAndStripSpaces(InString);
                StripEndNewLine(InString);

                if (isdigit(*InString))
                {
                    int readValue = atoi(InString);
                    unsigned short readUShort = (unsigned short)readValue;
                    if (readValue != (int)readUShort)
                    {
                        return Lut1dUtils::IMLUT_ERR_SYNTAX;
                    }
                    ptable[Count++] = readUShort;
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
                status = IMLUT_ERR_NO_LUT;
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
                (lut->tables[0])[0] = (unsigned short)atoi(InString);
                tablestart = 1;
            }
            else
            {
                char dstDepthS[16] = "";
#ifdef WINDOWS
                const int nummatched = sscanf(InString, "%*s %d %d %s", &numtables, &length, dstDepthS, 16);
#else
                const int nummatched = sscanf(InString, "%*s %d %d %s", &numtables, &length, dstDepthS);
#endif
                std::string subStr(InString, 5);
                if (nummatched < 2 ||
                    pystring::lower(subStr) != "lut: " ||
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
#ifdef WINDOWS
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

            // keep just the file name. Path might include "to"
            std::string root, extension, name;
            pystring::os::path::splitext(root, extension, fileName);

            name = pystring::os::path::basename(root);

            std::string lowerFileName(pystring::lower(name));

            // Get the export depth from the LUT name.  Look for a bit depth
            // after the "to" string. (ex: 12to10log).
            const char* tokenStr;
            if ((tokenStr = strstr(lowerFileName.c_str(), "to")))
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
            case IMLUT_ERR_MISSING_ENTRIES:
                return "Insufficient number of LUT entries";
            case IMLUT_ERR_CANNOT_OPEN:
                return "Cannot open LUT file";
            case IMLUT_ERR_CANNOT_MALLOC:
                return "Cannot allocate memory for LUT";
            case IMLUT_ERR_SYNTAX:
                return "Syntax error reading LUT file";
            case IMLUT_ERR_NO_LUT:
                return "No LUT data found in file";
            default:
                return "Unknown error for LUT";
            }
        }

        float Lut1dUtils::GetMax(IM_LutBitsPerChannel lutBitDepth)
        {
            switch (lutBitDepth)
            {
            case (IM_LUT_8BITS_PERCHANNEL):
                return 255.0f;

            case (IM_LUT_10BITS_PERCHANNEL):
                return 1023.0f;

            case (IM_LUT_12BITS_PERCHANNEL):
                return 4095.0f;

            case (IM_LUT_16BITS_PERCHANNEL):
            case (IM_LUT_HALFBITS_PERCHANNEL):
                return 65535.0f;

            default:
                return 1.0f;
            }
        }

        BitDepth Lut1dUtils::ConvertBitDepth(IM_LutBitsPerChannel lutBitDepth)
        {
            switch (lutBitDepth)
            {
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

            case IM_LUT_UNKNOWN_BITS_PERCHANNEL:
            default:
                return BIT_DEPTH_UNKNOWN;
            }
        }

        class LocalCachedFile : public CachedFile
        {
        public:
            LocalCachedFile ()
            {
            };
            ~LocalCachedFile() {};
            
            OpData::OpDataLut1DRcPtr lut1D;
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
            info.name = "Discreet 1D LUT";
            info.extension = "lut";
            info.capabilities = FORMAT_CAPABILITY_READ;
            formatInfoVec.push_back(info);
        }
        
        // Try and load the format
        // Raise an exception if it can't be loaded.
        
        CachedFileRcPtr LocalFileFormat::Read(
            std::istream & istream,
            const std::string & fileName) const
        {
            Lut1dUtils::IMLutStruct *discreetLut1d = 0x0;
            int errline;
            std::string errorLine;
            const int status = Lut1dUtils::IMLutGet(
                istream,
                fileName,
                &discreetLut1d,
                errline,
                errorLine);
            if (status != Lut1dUtils::IMLUT_OK)
            {
                std::ostringstream os;
                os << "Error parsing .lut file (";
                os << fileName.c_str() << ") ";
                os << "using Discreet 1D LUT reader. ";
                os << "Error is: " << Lut1dUtils::IMLutErrorStr(status);
                if (Lut1dUtils::IMLUT_ERR_SYNTAX == status)
                {
                    os << " At line (" << errline << "): '";
                    os << errorLine << "'.";
                }
                throw Exception(os.str().c_str());
            }

            const BitDepth inputBD = Lut1dUtils::ConvertBitDepth(discreetLut1d->srcBitDepth);
            const BitDepth outputBD = Lut1dUtils::ConvertBitDepth(discreetLut1d->targetBitDepth);
            const unsigned lutSize = discreetLut1d->length;

            OpData::Lut1D::HalfFlags halfFlags = (discreetLut1d->srcBitDepth
                == Lut1dUtils::IM_LUT_HALFBITS_PERCHANNEL)
                ? OpData::Lut1D::LUT_INPUT_HALF_CODE
                : OpData::Lut1D::LUT_STANDARD;
            
            if (discreetLut1d->targetBitDepth
                == Lut1dUtils::IM_LUT_HALFBITS_PERCHANNEL)
            {
                halfFlags = (OpData::Lut1D::HalfFlags)(halfFlags | OpData::Lut1D::LUT_OUTPUT_HALF_CODE);
            }

            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());
            cachedFile->lut1D.reset(new OpData::Lut1D(inputBD, outputBD, halfFlags));
                        
            cachedFile->lut1D->setInterpolation(INTERP_LINEAR);
            OpData::Array& array = cachedFile->lut1D->getArray();
            array.setLength((unsigned)lutSize);
            array.setNumColorComponents(3);

            const int srcTableLimit = discreetLut1d->numtables - 1;
            for (unsigned i = 0, p = 0; i< lutSize; ++i)
            {
                for (int j = 0; j<3; ++j, ++p)
                {
                    int srcTable = std::min(j, srcTableLimit);
                    if (discreetLut1d->targetBitDepth
                        == Lut1dUtils::IM_LUT_HALFBITS_PERCHANNEL)
                    {
                        half halfObj;
                        halfObj.setBits(discreetLut1d->tables[srcTable][i]);
                        array[p] = (float)halfObj;
                    }
                    else
                    {
                        array[p] = (float)discreetLut1d->tables[srcTable][i];
                    }
                }
            }

            Lut1dUtils::IMLutFree(&discreetLut1d);
            return cachedFile;
        }
        
        void
        LocalFileFormat::BuildFileOps(OpRcPtrVec & ops,
                                      const Config & /*config*/,
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
                os << "Cannot build .lut Op. Invalid cache type.";
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
            
            CreateLut1DOp(ops, cachedFile->lut1D, newDir);
        }
    }
    
    FileFormat * CreateFileFormatDiscreet1DL()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"
#include <fstream>

#ifdef WINDOWS
#define stringCopy(to, from, size) strcpy_s(to, size, from);
#else
#define stringCopy(to, from, size) strncpy(to, from, size);
#endif

void TestToolsStripBlank(
    const char * stringToStripChar,
    const std::string & stringResult)
{
    char stringToStrip[200];
    stringCopy(stringToStrip, stringToStripChar, 200);
    OCIO::ReplaceTabsAndStripSpaces(stringToStrip);
    std::string strippedString(stringToStrip);
    OIIO_CHECK_EQUAL(stringResult, strippedString);
}

void TestToolsStripEndNewLine(
    const char * stringToStripChar,
    const std::string & stringResult)
{
    char stringToStrip[200];
    stringCopy(stringToStrip, stringToStripChar, 200);
    OCIO::StripEndNewLine(stringToStrip);
    std::string strippedString(stringToStrip);
    OIIO_CHECK_EQUAL(stringResult, strippedString);
}

OIIO_ADD_TEST(FileFormatD1DL, TestStringUtil)
{
    TestToolsStripBlank("this is a test", "this is a test");
    TestToolsStripBlank("   this is a test      ", "this is a test");
    TestToolsStripBlank(" \t  this\tis a test    \t  ", "this is a test");
    TestToolsStripBlank("\t \t  this is a  test    \t  \t", "this is a  test");
    TestToolsStripBlank("\t \t  this\nis a\t\ttest    \t  \t", "this\nis a  test");
    TestToolsStripBlank("", "");

    TestToolsStripEndNewLine("", "");
    TestToolsStripEndNewLine("\n", "");
    TestToolsStripEndNewLine("\r", "");
    TestToolsStripEndNewLine("a\n", "a");
    TestToolsStripEndNewLine("b\r", "b");
    TestToolsStripEndNewLine("\na", "\na");
    TestToolsStripEndNewLine("\rb", "\rb");
}


#ifndef OCIO_UNIT_TEST_FILES_DIR
#error Expecting OCIO_UNIT_TEST_FILES_DIR to be defined for tests. Check relevant CMakeLists.txt
#endif

#define _STR(x) #x
#define STR(x) _STR(x)

static const std::string ocioTestFilesDir(STR(OCIO_UNIT_TEST_FILES_DIR));


OCIO::LocalCachedFileRcPtr LoadLutFile(const std::string & fileName)
{
    std::string filePath(ocioTestFilesDir + "/" + fileName);
    // Open the filePath
    std::ifstream filestream;
    filestream.open(filePath.c_str(), std::ios_base::in);
    
    std::string root, extension, name;
    OCIO::pystring::os::path::splitext(root, extension, filePath);

    name = OCIO::pystring::os::path::basename(root);

    // Read file
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile = tester.Read(filestream, filePath);

    return OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(cachedFile);
}

OIIO_ADD_TEST(FileFormatD1DL, Test)
{
    OCIO::LocalCachedFileRcPtr lutFile;
    {
        const std::string discreetLut("logtolin_8to8.lut");
        OIIO_CHECK_NO_THROW(lutFile = LoadLutFile(discreetLut));
        OIIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UINT8, lutFile->lut1D->getInputBitDepth());
        OIIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UINT8, lutFile->lut1D->getOutputBitDepth());
        OIIO_CHECK_EQUAL(OCIO::OpData::Lut1D::LUT_STANDARD, lutFile->lut1D->getHalfFlags());
        const OCIO::OpData::Array & array = lutFile->lut1D->getArray();
        OIIO_CHECK_EQUAL(256, array.getLength());

        for (int c = 0; c < 3; ++c)
        {
            // tests some values - all channels are the same
            OIIO_CHECK_EQUAL(0.0f, array[c + 0]);
            OIIO_CHECK_EQUAL(0.0f, array[c + 3 * 22]);
            OIIO_CHECK_EQUAL(5.0f, array[c + 3 * 32]);
            OIIO_CHECK_EQUAL(11.0f, array[c + 3 * 42]);
            OIIO_CHECK_EQUAL(18.0f, array[c + 3 * 52]);
            OIIO_CHECK_EQUAL(27.0f, array[c + 3 * 62]);
            OIIO_CHECK_EQUAL(36.0f, array[c + 3 * 72]);
            OIIO_CHECK_EQUAL(48.0f, array[c + 3 * 82]);
            OIIO_CHECK_EQUAL(159.0f, array[c + 3 * 142]);
            OIIO_CHECK_EQUAL(188.0f, array[c + 3 * 152]);
            OIIO_CHECK_EQUAL(222.0f, array[c + 3 * 162]);
            OIIO_CHECK_EQUAL(255.0f, array[c + 3 * 202]);
        }
    }

    // half out
    {
        const std::string discreetLut("Test_12to16fp.lut");
        OIIO_CHECK_NO_THROW(lutFile = LoadLutFile(discreetLut));
        OIIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UINT12, lutFile->lut1D->getInputBitDepth());
        OIIO_CHECK_EQUAL(OCIO::BIT_DEPTH_F16, lutFile->lut1D->getOutputBitDepth());
        OIIO_CHECK_EQUAL(OCIO::OpData::Lut1D::LUT_OUTPUT_HALF_CODE, lutFile->lut1D->getHalfFlags());
        const OCIO::OpData::Array & array = lutFile->lut1D->getArray();
        OIIO_CHECK_EQUAL(4096, array.getLength());

        // Select some samples to verify the LUT was fully read
        const unsigned sampleInterval = 207u;
        const unsigned short expectedSampleValues[] =
        {
                0, 12546, 13171, 13491, 13705, 13898, 14074, 14238, 14365, 14438,
            14507, 14574, 14638, 14700, 14760, 14818, 14875, 14930, 14983, 15037,
            15094, 15156, 15222, 15294, 15366, 15408, 15453, 15501, 15553, 15609,
            15669, 15733, 15802, 15876, 15954, 16038, 16128, 16224, 16327, 16410,
            16468, 16530, 16596, 16667, 16741, 16821, 16905, 16995, 17090, 17191,
            17298, 17410, 17470, 17534, 17602, 17673, 17749, 17829, 17914, 18003
        };
        for (unsigned i = 0, si = 0; i < array.getLength(); ++si, i += sampleInterval)
        {
            OIIO_CHECK_EQUAL(expectedSampleValues[si], half(array[i]).bits());
        }
    }

    // Half in / out
    {
        const std::string discreetLut16fp16fp("photo_default_16fpto16fp.lut");
        OIIO_CHECK_NO_THROW(lutFile = LoadLutFile(discreetLut16fp16fp));
        OIIO_CHECK_EQUAL(OCIO::BIT_DEPTH_F16, lutFile->lut1D->getInputBitDepth());
        OIIO_CHECK_EQUAL(OCIO::BIT_DEPTH_F16, lutFile->lut1D->getOutputBitDepth());
        OIIO_CHECK_EQUAL(OCIO::OpData::Lut1D::LUT_INPUT_OUTPUT_HALF_CODE,
                         lutFile->lut1D->getHalfFlags());
        const OCIO::OpData::Array & array = lutFile->lut1D->getArray();
        OIIO_CHECK_EQUAL(65536u, array.getLength());

        // Select some samples to verify the LUT was fully read
        const unsigned sampleInterval = 3277u;
        const unsigned short expectedSampleValues[] =
        {
                0,   242,   554,  1265,  2463,  3679,  4918,  6234,  7815,  9945,
            11918, 13222, 14063, 14616, 14958, 15176, 15266, 15349, 15398, 15442,
            15488, 15536, 15586, 15637, 15690, 15745, 15802, 15862, 15923, 15987,
            32770, 33862, 34954, 36047, 37139, 38231, 39324, 40416, 41508, 42601,
            43693, 44785, 45878, 46970, 48062, 49155, 50247, 51339, 52432, 53524,
            54616, 55709, 56801, 57893, 58986, 60078, 61170, 62263, 63355, 64447
        };
        for (unsigned i = 0, si = 0; i < array.getLength(); ++si, i += sampleInterval)
        {
            OIIO_CHECK_EQUAL(expectedSampleValues[si], half(array[i]).bits());
        }
    }

    // Half in
    {
        const std::string discreetLut16fp12("Test_16fpto12.lut");
        OIIO_CHECK_NO_THROW(lutFile = LoadLutFile(discreetLut16fp12));
        OIIO_CHECK_EQUAL(OCIO::BIT_DEPTH_F16, lutFile->lut1D->getInputBitDepth());
        OIIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UINT12, lutFile->lut1D->getOutputBitDepth());
        OIIO_CHECK_EQUAL(OCIO::OpData::Lut1D::LUT_INPUT_HALF_CODE, lutFile->lut1D->getHalfFlags());
        const OCIO::OpData::Array & array = lutFile->lut1D->getArray();
        OIIO_CHECK_EQUAL(65536u, array.getLength());

        // Select some samples to verify the LUT was fully read
        const unsigned sampleInterval = 3277u;
        const float expectedSampleValues[] =
        {
               0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    1.0f,    3.0f,
              10.0f,   36.0f,  130.0f,  466.0f, 1585.0f, 2660.0f, 3595.0f, 4095.0f, 4095.0f, 4095.0f,
            4095.0f, 4095.0f, 4095.0f, 4095.0f, 4095.0f, 4095.0f, 4095.0f, 4095.0f, 4095.0f, 4095.0f,
               0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,
               0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,
               0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f,    0.0f
        };
        for (unsigned i = 0, si = 0; i < array.getLength(); ++si, i += sampleInterval)
        {
            OIIO_CHECK_EQUAL(expectedSampleValues[si], array[i]);
        }
    }

    // Bad file.
    const std::string truncatedLut("error_truncated_file.lut");
    OIIO_CHECK_THROW_WHAT(LoadLutFile(truncatedLut),
                          OCIO::Exception,
                          "Insufficient number of LUT entries");
}

#endif // OCIO_UNIT_TEST

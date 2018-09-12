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
#include "MatrixOps.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"

#include <cstdio>
#include <cstring>
#include <sstream>


OCIO_NAMESPACE_ENTER
{
    ////////////////////////////////////////////////////////////////
    
    namespace
    {
        class LocalCachedFile : public CachedFile
        {
        public:
            LocalCachedFile()
            {
                memset(m44, 0, 16*sizeof(float));
                memset(offset4, 0, 4*sizeof(float));
            };
            ~LocalCachedFile() {};
            
            float m44[16];
            float offset4[4];
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
            info.name = "spimtx";
            info.extension = "spimtx";
            info.capabilities = FORMAT_CAPABILITY_READ;
            formatInfoVec.push_back(info);
        }
        
        CachedFileRcPtr
        LocalFileFormat::Read(
            std::istream & istream,
            const std::string & fileName) const
        {

            // Read the entire file.
            std::ostringstream fileStream;

            {
                const int MAX_LINE_SIZE = 4096;
                char lineBuffer[MAX_LINE_SIZE];

                while (istream.good())
                {
                    istream.getline(lineBuffer, MAX_LINE_SIZE);
                    fileStream << std::string(lineBuffer) << " ";
                }
            }

            // Turn it into parts
            std::vector<std::string> lineParts;
            pystring::split(pystring::strip(fileStream.str()), lineParts);
            if(lineParts.size() != 12)
            {
                std::ostringstream os;
                os << "Error parsing .spimtx file (";
                os << fileName << "). ";
                os << "File must contain 12 float entries. ";
                os << lineParts.size() << " found.";
                throw Exception(os.str().c_str());
            }

            // Turn the parts into floats
            std::vector<float> floatArray;
            if(!StringVecToFloatVec(floatArray, lineParts))
            {
                std::ostringstream os;
                os << "Error parsing .spimtx file (";
                os << fileName << "). ";
                os << "File must contain all float entries. ";
                throw Exception(os.str().c_str());
            }


            // Put the bits in the right place
            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());

            cachedFile->m44[0] = floatArray[0];
            cachedFile->m44[1] = floatArray[1];
            cachedFile->m44[2] = floatArray[2];
            cachedFile->m44[3] = 0.0f;

            cachedFile->m44[4] = floatArray[4];
            cachedFile->m44[5] = floatArray[5];
            cachedFile->m44[6] = floatArray[6];
            cachedFile->m44[7] = 0.0f;

            cachedFile->m44[8] = floatArray[8];
            cachedFile->m44[9] = floatArray[9];
            cachedFile->m44[10] = floatArray[10];
            cachedFile->m44[11] = 0.0f;

            cachedFile->m44[12] = 0.0f;
            cachedFile->m44[13] = 0.0f;
            cachedFile->m44[14] = 0.0f;
            cachedFile->m44[15] = 1.0f;

            cachedFile->offset4[0] = floatArray[3] / 65535.0f;
            cachedFile->offset4[1] = floatArray[7] / 65535.0f;
            cachedFile->offset4[2] = floatArray[11] / 65535.0f;
            cachedFile->offset4[3] = 0.0f;

            return cachedFile;
        }
        
        void LocalFileFormat::BuildFileOps(OpRcPtrVec & ops,
                                      const Config& /*config*/,
                                      const ConstContextRcPtr & /*context*/,
                                      CachedFileRcPtr untypedCachedFile,
                                      const FileTransform& fileTransform,
                                      TransformDirection dir) const
        {
            LocalCachedFileRcPtr cachedFile = DynamicPtrCast<LocalCachedFile>(untypedCachedFile);

            if(!cachedFile) // This should never happen.
            {
                std::ostringstream os;
                os << "Cannot build SpiMtx Ops. Invalid cache type.";
                throw Exception(os.str().c_str());
            }

            TransformDirection newDir = CombineTransformDirections(dir,
                fileTransform.getDirection());

            CreateMatrixOffsetOp(ops,
                                 cachedFile->m44,
                                 cachedFile->offset4,
                                 newDir);
        }
    }
    
    FileFormat * CreateFileFormatSpiMtx()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"
#include <fstream>

OIIO_ADD_TEST(FileFormatSpiMtx, FormatInfo)
{
    OCIO::FormatInfoVec formatInfoVec;
    OCIO::LocalFileFormat tester;
    tester.GetFormatInfo(formatInfoVec);

    OIIO_CHECK_EQUAL(1, formatInfoVec.size());
    OIIO_CHECK_EQUAL("spimtx", formatInfoVec[0].name);
    OIIO_CHECK_EQUAL("spimtx", formatInfoVec[0].extension);
    OIIO_CHECK_EQUAL(OCIO::FORMAT_CAPABILITY_READ,
        formatInfoVec[0].capabilities);
}

OCIO::LocalCachedFileRcPtr LoadLutFile(const std::string & filePath)
{
    // Open the filePath
    std::ifstream filestream;
    filestream.open(filePath.c_str(), std::ios_base::in);

    std::string root, extension, name;
    OCIO::pystring::os::path::splitext(root, extension, filePath);

    name = OCIO::pystring::os::path::basename(root);

    // Read file
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile = tester.Read(filestream, name);

    return OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(cachedFile);
}

#ifndef OCIO_UNIT_TEST_FILES_DIR
#error Expecting OCIO_UNIT_TEST_FILES_DIR to be defined for tests. Check relevant CMakeLists.txt
#endif

#define _STR(x) #x
#define STR(x) _STR(x)

static const std::string ocioTestFilesDir(STR(OCIO_UNIT_TEST_FILES_DIR));

OIIO_ADD_TEST(FileFormatSpiMtx, Test)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string spiMtxFile(ocioTestFilesDir + std::string("/camera_to_aces.spimtx"));
    OIIO_CHECK_NO_THROW(cachedFile = LoadLutFile(spiMtxFile));

    OIIO_CHECK_NE(NULL, cachedFile.get());
    OIIO_CHECK_EQUAL(0.0f, cachedFile->offset4[0]);
    OIIO_CHECK_EQUAL(0.0f, cachedFile->offset4[1]);
    OIIO_CHECK_EQUAL(0.0f, cachedFile->offset4[2]);
    OIIO_CHECK_EQUAL(0.0f, cachedFile->offset4[3]);

    OIIO_CHECK_EQUAL(0.754338638f, cachedFile->m44[0]);
    OIIO_CHECK_EQUAL(0.133697046f, cachedFile->m44[1]);
    OIIO_CHECK_EQUAL(0.111968437f, cachedFile->m44[2]);
    OIIO_CHECK_EQUAL(0.0f, cachedFile->m44[3]);

    OIIO_CHECK_EQUAL(0.021198141f, cachedFile->m44[4]);
    OIIO_CHECK_EQUAL(1.005410934f, cachedFile->m44[5]);
    OIIO_CHECK_EQUAL(-0.026610548f, cachedFile->m44[6]);
    OIIO_CHECK_EQUAL(0.0f, cachedFile->m44[7]);

    OIIO_CHECK_CLOSE(-0.00975699, cachedFile->m44[8], 1e-6f);
    OIIO_CHECK_EQUAL(0.004508563f, cachedFile->m44[9]);
    OIIO_CHECK_EQUAL(1.005253201f, cachedFile->m44[10]);
    OIIO_CHECK_EQUAL(0.0f, cachedFile->m44[11]);

    OIIO_CHECK_EQUAL(0.0f, cachedFile->m44[12]);
    OIIO_CHECK_EQUAL(0.0f, cachedFile->m44[13]);
    OIIO_CHECK_EQUAL(0.0f, cachedFile->m44[14]);
    OIIO_CHECK_EQUAL(1.0f, cachedFile->m44[15]);
}

OCIO::LocalCachedFileRcPtr ReadSpiMtx(const std::string & fileContent)
{
    std::istringstream is;
    is.str(fileContent);

    // Read file
    OCIO::LocalFileFormat tester;
    const std::string SAMPLE_NAME("Memory File");
    OCIO::CachedFileRcPtr cachedFile = tester.Read(is, SAMPLE_NAME);

    return OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(cachedFile);
}

OIIO_ADD_TEST(FileFormatSpiMtx, ReadOffset)
{
    {
        // Validate stream can be read with no error.
        // Then stream will be altered to introduce errors.
        const std::string SAMPLE_FILE =
            "1 0 0 6553.5\n"
            "0 1 0 32767.5\n"
            "0 0 1 65535.0\n";

        OCIO::LocalCachedFileRcPtr cachedFile;
        OIIO_CHECK_NO_THROW(cachedFile = ReadSpiMtx(SAMPLE_FILE));
        OIIO_CHECK_NE(NULL, cachedFile.get());
        OIIO_CHECK_EQUAL(0.1f, cachedFile->offset4[0]);
        OIIO_CHECK_EQUAL(0.5f, cachedFile->offset4[1]);
        OIIO_CHECK_EQUAL(1.0f, cachedFile->offset4[2]);
        OIIO_CHECK_EQUAL(0.0f, cachedFile->offset4[3]);
    }
}

OIIO_ADD_TEST(FileFormatSpiMtx, ReadFailure)
{
    {
        // Validate stream can be read with no error.
        // Then stream will be altered to introduce errors.
        const std::string SAMPLE_NO_ERROR =
            "1.0 0.0 0.0 0.0\n"
            "0.0 1.0 0.0 0.0\n"
            "0.0 0.0 1.0 0.0\n";

        OIIO_CHECK_NO_THROW(ReadSpiMtx(SAMPLE_NO_ERROR));
    }
    {
        // Wrong number of elements
        const std::string SAMPLE_ERROR =
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "0.0 0.0 1.0\n";

        OIIO_CHECK_THROW_WHAT(ReadSpiMtx(SAMPLE_ERROR),
                              OCIO::Exception,
                              "File must contain 12 float entries");
    }
    {
        // Some elements can' t be read as float
        const std::string SAMPLE_ERROR =
            "1.0 0.0 0.0 0.0\n"
            "0.0 error 0.0 0.0\n"
            "0.0 0.0 1.0 0.0\n";

        OIIO_CHECK_THROW_WHAT(ReadSpiMtx(SAMPLE_ERROR),
                              OCIO::Exception,
                              "File must contain all float entries");
    }
}

#endif // OCIO_UNIT_TEST

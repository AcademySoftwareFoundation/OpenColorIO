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
#include "Lut3DOp.h"
#include "pystring/pystring.h"

#include <cstdio>
#include <sstream>

/*
SPILUT 1.0
3 3
32 32 32
0 0 0 0.0132509 0.0158522 0.0156622
0 0 1 0.0136178 0.018843 0.033921
0 0 2 0.0136487 0.0240918 0.0563014
0 0 3 0.015706 0.0303061 0.0774135

... entries can be in any order
... after the expected number of entries is found, file can contain anything
*/


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
                lut = Lut3D::Create();
            };
            ~LocalCachedFile() {};
            
            Lut3DRcPtr lut;
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
            info.name = "spi3d";
            info.extension = "spi3d";
            info.capabilities = FORMAT_CAPABILITY_READ;
            formatInfoVec.push_back(info);
        }
        
        CachedFileRcPtr
        LocalFileFormat::Read(
            std::istream & istream,
            const std::string & fileName) const
        {
            const int MAX_LINE_SIZE = 4096;
            char lineBuffer[MAX_LINE_SIZE];

            Lut3DRcPtr lut3d = Lut3D::Create();

            // Read header information
            istream.getline(lineBuffer, MAX_LINE_SIZE);
            if(!pystring::startswith(pystring::lower(lineBuffer), "spilut"))
            {
                std::ostringstream os;
                os << "Error parsing .spi3d file (";
                os << fileName;
                os << ").  ";
                os << "LUT does not appear to be valid spilut format. ";
                os << "Expected 'SPILUT'.  Found: '" << lineBuffer << "'.";
                throw Exception(os.str().c_str());
            }

            // TODO: Assert 2nd line is 3 3
            istream.getline(lineBuffer, MAX_LINE_SIZE);

            // Get LUT Size
            int rSize = 0, gSize = 0, bSize = 0;
            istream.getline(lineBuffer, MAX_LINE_SIZE);
            if (3 != sscanf(lineBuffer, "%d %d %d", &rSize, &gSize, &bSize))
            {
                std::ostringstream os;
                os << "Error parsing .spi3d file (";
                os << fileName;
                os << "). ";
                os << "Error while reading LUT size. Found: '";
                os << lineBuffer << "'.";
                throw Exception(os.str().c_str());
            }

            lut3d->size[0] = rSize;
            lut3d->size[1] = gSize;
            lut3d->size[2] = bSize;
            lut3d->lut.resize(rSize * gSize * bSize * 3);

            // Parse table
            int index = 0;
            int rIndex, gIndex, bIndex;
            float redValue, greenValue, blueValue;

            int entriesRemaining = rSize * gSize * bSize;

            while (istream.good() && entriesRemaining > 0)
            {
                istream.getline(lineBuffer, MAX_LINE_SIZE);

                if (sscanf(lineBuffer, "%d %d %d %f %f %f",
                    &rIndex, &gIndex, &bIndex,
                    &redValue, &greenValue, &blueValue) == 6)
                {
                    bool invalidIndex = false;
                    if (rIndex < 0 || rIndex >= rSize
                        || gIndex < 0 || gIndex >= gSize
                        || bIndex < 0 || bIndex >= bSize)
                    {
                        invalidIndex = true;
                    }
                    else
                    {
                        index = GetLut3DIndex_RedFast(rIndex, gIndex, bIndex,
                            rSize, gSize, bSize);
                        if (index < 0 || index >= (int)lut3d->lut.size())
                        {
                            invalidIndex = true;
                        }

                    }

                    if (invalidIndex)
                    {
                        std::ostringstream os;
                        os << "Error parsing .spi3d file (";
                        os << fileName;
                        os << "). ";
                        os << "Data is invalid. ";
                        os << "A LUT entry is specified (";
                        os << rIndex << " " << gIndex << " " << bIndex;
                        os << ") that falls outside of the cube.";
                        throw Exception(os.str().c_str());
                    }

                    lut3d->lut[index+0] = redValue;
                    lut3d->lut[index+1] = greenValue;
                    lut3d->lut[index+2] = blueValue;

                    entriesRemaining--;
                }
            }

            // Have we fully populated the table?
            if (entriesRemaining > 0)
            {
                std::ostringstream os;
                os << "Error parsing .spi3d file (";
                os << fileName;
                os << "). ";
                os << "Not enough entries found.";
                throw Exception(os.str().c_str());
            }

            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());
            cachedFile->lut = lut3d;
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
                os << "Cannot build Spi3D Op. Invalid cache type.";
                throw Exception(os.str().c_str());
            }

            TransformDirection newDir = CombineTransformDirections(dir,
                fileTransform.getDirection());

            CreateLut3DOp(ops,
                          cachedFile->lut,
                          fileTransform.getInterpolation(),
                          newDir);
        }
    }
    
    FileFormat * CreateFileFormatSpi3D()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"
#include <fstream>

OIIO_ADD_TEST(FileFormatSpi3D, FormatInfo)
{
    OCIO::FormatInfoVec formatInfoVec;
    OCIO::LocalFileFormat tester;
    tester.GetFormatInfo(formatInfoVec);

    OIIO_CHECK_EQUAL(1, formatInfoVec.size());
    OIIO_CHECK_EQUAL("spi3d", formatInfoVec[0].name);
    OIIO_CHECK_EQUAL("spi3d", formatInfoVec[0].extension);
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

OIIO_ADD_TEST(FileFormatSpi3D, Test)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string spi3dFile(ocioTestFilesDir + std::string("/spi_ocio_srgb_test.spi3d"));
    OIIO_CHECK_NO_THROW(cachedFile = LoadLutFile(spi3dFile));

    OIIO_CHECK_ASSERT((bool)cachedFile);
    OIIO_CHECK_ASSERT((bool)(cachedFile->lut));

    OIIO_CHECK_EQUAL(32, cachedFile->lut->size[0]);
    OIIO_CHECK_EQUAL(32, cachedFile->lut->size[1]);
    OIIO_CHECK_EQUAL(32, cachedFile->lut->size[2]);
    OIIO_CHECK_EQUAL(32*32*32*3, cachedFile->lut->lut.size());

    OIIO_CHECK_EQUAL(0.040157f, cachedFile->lut->lut[0]);
    OIIO_CHECK_EQUAL(0.038904f, cachedFile->lut->lut[1]);
    OIIO_CHECK_EQUAL(0.028316f, cachedFile->lut->lut[2]);
    // 10 2 12
    OIIO_CHECK_EQUAL(0.102161f, cachedFile->lut->lut[37086]);
    OIIO_CHECK_EQUAL(0.032187f, cachedFile->lut->lut[37087]);
    OIIO_CHECK_EQUAL(0.175453f, cachedFile->lut->lut[37088]);
}

void ReadSpi3d(const std::string & fileContent)
{
    std::istringstream is;
    is.str(fileContent);

    // Read file
    OCIO::LocalFileFormat tester;
    const std::string SAMPLE_NAME("Memory File");
    OCIO::CachedFileRcPtr cachedFile = tester.Read(is, SAMPLE_NAME);
}

OIIO_ADD_TEST(FileFormatSpi3D, ReadFailure)
{
    {
        // Validate stream can be read with no error.
        // Then stream will be altered to introduce errors.
        const std::string SAMPLE_NO_ERROR =
            "SPILUT 1.0\n"
            "3 3\n"
            "2 2 2\n"
            "0 0 0 0.0 0.0 0.0\n"
            "0 0 1 0.0 0.0 0.9\n"
            "0 1 0 0.0 0.7 0.0\n"
            "0 1 1 0.0 0.8 0.8\n"
            "1 0 0 0.7 0.0 0.1\n"
            "1 0 1 0.7 0.6 0.1\n"
            "1 1 0 0.6 0.7 0.1\n"
            "1 1 1 0.6 0.7 0.7\n";

        OIIO_CHECK_NO_THROW(ReadSpi3d(SAMPLE_NO_ERROR));
    }
    {
        // Wrong first line
        const std::string SAMPLE_ERROR =
            "SPI LUT 1.0\n"
            "3 3\n"
            "2 2 2\n"
            "0 0 0 0.0 0.0 0.0\n"
            "0 0 1 0.0 0.0 0.9\n"
            "0 1 0 0.0 0.7 0.0\n"
            "0 1 1 0.0 0.8 0.8\n"
            "1 0 0 0.7 0.0 0.1\n"
            "1 0 1 0.7 0.6 0.1\n"
            "1 1 0 0.6 0.7 0.1\n"
            "1 1 1 0.6 0.7 0.7\n";

        OIIO_CHECK_THROW_WHAT(ReadSpi3d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Expected 'SPILUT'");
    }
    {
        // 3 line is not 3 ints
        const std::string SAMPLE_ERROR =
            "SPILUT 1.0\n"
            "3 3\n"
            "42\n"
            "0 0 0 0.0 0.0 0.0\n"
            "0 0 1 0.0 0.0 0.9\n"
            "0 1 0 0.0 0.7 0.0\n"
            "0 1 1 0.0 0.8 0.8\n"
            "1 0 0 0.7 0.0 0.1\n"
            "1 0 1 0.7 0.6 0.1\n"
            "1 1 0 0.6 0.7 0.1\n"
            "1 1 1 0.6 0.7 0.7\n";

        OIIO_CHECK_THROW_WHAT(ReadSpi3d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Error while reading LUT size");
    }
    {
        // index out of range
        const std::string SAMPLE_ERROR =
            "SPILUT 1.0\n"
            "3 3\n"
            "2 2 2\n"
            "0 2 0 0.0 0.0 0.0\n"
            "0 0 1 0.0 0.0 0.9\n"
            "0 1 0 0.0 0.7 0.0\n"
            "0 1 1 0.0 0.8 0.8\n"
            "1 0 0 0.7 0.0 0.1\n"
            "1 0 1 0.7 0.6 0.1\n"
            "1 1 0 0.6 0.7 0.1\n"
            "1 1 1 0.6 0.7 0.7\n";

        OIIO_CHECK_THROW_WHAT(ReadSpi3d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "that falls outside of the cube");
    }
    {
        // Not enough entries
        const std::string SAMPLE_ERROR =
            "SPILUT 1.0\n"
            "3 3\n"
            "2 2 2\n"
            "0 0 0 0.0 0.0 0.0\n"
            "0 0 1 0.0 0.0 0.9\n"
            "0 1 0 0.0 0.7 0.0\n"
            "0 1 1 0.0 0.8 0.8\n"
            "1 0 1 0.7 0.6 0.1\n"
            "1 1 0 0.6 0.7 0.1\n"
            "1 1 1 0.6 0.7 0.7\n";

        OIIO_CHECK_THROW_WHAT(ReadSpi3d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Not enough entries found");
    }
}

#endif // OCIO_UNIT_TEST

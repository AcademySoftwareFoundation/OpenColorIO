// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstdio>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/Lut3D/Lut3DOp.h"
#include "Platform.h"
#include "pystring/pystring.h"
#include "transforms/FileTransform.h"

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
            LocalCachedFile() = default;
            ~LocalCachedFile() = default;
            
            Lut3DOpDataRcPtr lut;
        };
        
        typedef OCIO_SHARED_PTR<LocalCachedFile> LocalCachedFileRcPtr;
        
        
        
        class LocalFileFormat : public FileFormat
        {
        public:
            LocalFileFormat() = default;
            ~LocalFileFormat() = default;
            
            void getFormatInfo(FormatInfoVec & formatInfoVec) const override;
            
            CachedFileRcPtr read(
                std::istream & istream,
                const std::string & fileName) const override;
            
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
            info.name = "spi3d";
            info.extension = "spi3d";
            info.capabilities = FORMAT_CAPABILITY_READ;
            formatInfoVec.push_back(info);
        }
        
        CachedFileRcPtr LocalFileFormat::read(
            std::istream & istream,
            const std::string & fileName) const
        {
            const int MAX_LINE_SIZE = 4096;
            char lineBuffer[MAX_LINE_SIZE];

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

            // TODO: Support nonuniformly sized LUTs.
            if (rSize != gSize || rSize != bSize)
            {
                std::ostringstream os;
                os << "Error parsing .spi3d file (";
                os << fileName;
                os << "). ";
                os << "LUT size should be the same for all components. Found: '";
                os << lineBuffer << "'.";
                throw Exception(os.str().c_str());
            }

            Lut3DOpDataRcPtr lut3d = std::make_shared<Lut3DOpData>((unsigned long)rSize);
            lut3d->setFileOutputBitDepth(BIT_DEPTH_F32);

            // Parse table
            int index = 0;
            int rIndex, gIndex, bIndex;
            float redValue, greenValue, blueValue;

            int entriesRemaining = rSize * gSize * bSize;
            Array & lutArray = lut3d->getArray();
            unsigned long numVal = lutArray.getNumValues();
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
                        index = GetLut3DIndex_BlueFast(rIndex, gIndex, bIndex,
                                                       rSize, gSize, bSize);
                        if (index < 0 || index >= (int)numVal)
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

                    lutArray[index+0] = redValue;
                    lutArray[index+1] = greenValue;
                    lutArray[index+2] = blueValue;

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

        void LocalFileFormat::buildFileOps(OpRcPtrVec & ops,
                                           const Config & /*config*/,
                                           const ConstContextRcPtr & /*context*/,
                                           CachedFileRcPtr untypedCachedFile,
                                           const FileTransform & fileTransform,
                                           TransformDirection dir) const
        {
            LocalCachedFileRcPtr cachedFile = DynamicPtrCast<LocalCachedFile>(untypedCachedFile);

            if(!cachedFile) // This should never happen.
            {
                std::ostringstream os;
                os << "Cannot build Spi3D Op. Invalid cache type.";
                throw Exception(os.str().c_str());
            }

            TransformDirection newDir = fileTransform.getDirection();
            newDir = CombineTransformDirections(dir, newDir);
            
            cachedFile->lut->setInterpolation(fileTransform.getInterpolation());
            CreateLut3DOp(ops,
                          cachedFile->lut,
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
#include "UnitTestUtils.h"

OCIO_ADD_TEST(FileFormatSpi3D, format_info)
{
    OCIO::FormatInfoVec formatInfoVec;
    OCIO::LocalFileFormat tester;
    tester.getFormatInfo(formatInfoVec);

    OCIO_CHECK_EQUAL(1, formatInfoVec.size());
    OCIO_CHECK_EQUAL("spi3d", formatInfoVec[0].name);
    OCIO_CHECK_EQUAL("spi3d", formatInfoVec[0].extension);
    OCIO_CHECK_EQUAL(OCIO::FORMAT_CAPABILITY_READ,
        formatInfoVec[0].capabilities);
}

OCIO::LocalCachedFileRcPtr LoadLutFile(const std::string & fileName)
{
    return OCIO::LoadTestFile<OCIO::LocalFileFormat, OCIO::LocalCachedFile>(
        fileName, std::ios_base::in);
}

OCIO_ADD_TEST(FileFormatSpi3D, test)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string spi3dFile("spi_ocio_srgb_test.spi3d");
    OCIO_CHECK_NO_THROW(cachedFile = LoadLutFile(spi3dFile));

    OCIO_CHECK_ASSERT((bool)cachedFile);
    OCIO_CHECK_ASSERT((bool)(cachedFile->lut));

    const OCIO::Array & lutArray = cachedFile->lut->getArray();
    OCIO_CHECK_EQUAL(32, lutArray.getLength());
    OCIO_CHECK_EQUAL(32*32*32*3, lutArray.getNumValues());

    OCIO_CHECK_EQUAL(0.040157f, lutArray[0]);
    OCIO_CHECK_EQUAL(0.038904f, lutArray[1]);
    OCIO_CHECK_EQUAL(0.028316f, lutArray[2]);
    // 10 2 12
    OCIO_CHECK_EQUAL(0.102161f, lutArray[30948]);
    OCIO_CHECK_EQUAL(0.032187f, lutArray[30949]);
    OCIO_CHECK_EQUAL(0.175453f, lutArray[30950]);
}

void ReadSpi3d(const std::string & fileContent)
{
    std::istringstream is;
    is.str(fileContent);

    // Read file
    OCIO::LocalFileFormat tester;
    const std::string SAMPLE_NAME("Memory File");
    OCIO::CachedFileRcPtr cachedFile = tester.read(is, SAMPLE_NAME);
}

OCIO_ADD_TEST(FileFormatSpi3D, read_failure)
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

        OCIO_CHECK_NO_THROW(ReadSpi3d(SAMPLE_NO_ERROR));
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

        OCIO_CHECK_THROW_WHAT(ReadSpi3d(SAMPLE_ERROR),
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

        OCIO_CHECK_THROW_WHAT(ReadSpi3d(SAMPLE_ERROR),
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

        OCIO_CHECK_THROW_WHAT(ReadSpi3d(SAMPLE_ERROR),
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

        OCIO_CHECK_THROW_WHAT(ReadSpi3d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Not enough entries found");
    }
}

#endif // OCIO_UNIT_TEST

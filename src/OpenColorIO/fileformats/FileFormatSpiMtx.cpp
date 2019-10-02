// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "transforms/FileTransform.h"
#include "ops/Matrix/MatrixOps.h"
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
                memset(m44, 0, 16*sizeof(double));
                memset(offset4, 0, 4*sizeof(double));
            };
            ~LocalCachedFile() {};
            
            double m44[16];
            double offset4[4];
        };
        
        typedef OCIO_SHARED_PTR<LocalCachedFile> LocalCachedFileRcPtr;
        
        
        
        class LocalFileFormat : public FileFormat
        {
        public:
            
            ~LocalFileFormat() {};
            
            void getFormatInfo(FormatInfoVec & formatInfoVec) const override;
            
            CachedFileRcPtr read(
                std::istream & istream,
                const std::string & fileName) const override;
            
            void buildFileOps(OpRcPtrVec & ops,
                              const Config& config,
                              const ConstContextRcPtr & context,
                              CachedFileRcPtr untypedCachedFile,
                              const FileTransform& fileTransform,
                              TransformDirection dir) const override;
        };
        
        void LocalFileFormat::getFormatInfo(FormatInfoVec & formatInfoVec) const
        {
            FormatInfo info;
            info.name = "spimtx";
            info.extension = "spimtx";
            info.capabilities = FORMAT_CAPABILITY_READ;
            formatInfoVec.push_back(info);
        }
        
        CachedFileRcPtr LocalFileFormat::read(
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
            StringVec lineParts;
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

            cachedFile->m44[0] =  floatArray[0];
            cachedFile->m44[1] =  floatArray[1];
            cachedFile->m44[2] =  floatArray[2];
            cachedFile->m44[3] =  0.0;
                                  
            cachedFile->m44[4] =  floatArray[4];
            cachedFile->m44[5] =  floatArray[5];
            cachedFile->m44[6] =  floatArray[6];
            cachedFile->m44[7] =  0.0;
                                  
            cachedFile->m44[8] =  floatArray[8];
            cachedFile->m44[9] =  floatArray[9];
            cachedFile->m44[10] = floatArray[10];
            cachedFile->m44[11] = 0.0;

            cachedFile->m44[12] = 0.0;
            cachedFile->m44[13] = 0.0;
            cachedFile->m44[14] = 0.0;
            cachedFile->m44[15] = 1.0;

            cachedFile->offset4[0] = floatArray[3] / 65535.0;
            cachedFile->offset4[1] = floatArray[7] / 65535.0;
            cachedFile->offset4[2] = floatArray[11] / 65535.0;
            cachedFile->offset4[3] = 0.0;

            return cachedFile;
        }
        
        void LocalFileFormat::buildFileOps(OpRcPtrVec & ops,
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
#include "UnitTestUtils.h"

OCIO_ADD_TEST(FileFormatSpiMtx, FormatInfo)
{
    OCIO::FormatInfoVec formatInfoVec;
    OCIO::LocalFileFormat tester;
    tester.getFormatInfo(formatInfoVec);

    OCIO_CHECK_EQUAL(1, formatInfoVec.size());
    OCIO_CHECK_EQUAL("spimtx", formatInfoVec[0].name);
    OCIO_CHECK_EQUAL("spimtx", formatInfoVec[0].extension);
    OCIO_CHECK_EQUAL(OCIO::FORMAT_CAPABILITY_READ,
        formatInfoVec[0].capabilities);
}

OCIO::LocalCachedFileRcPtr LoadLutFile(const std::string & fileName)
{
    return OCIO::LoadTestFile<OCIO::LocalFileFormat, OCIO::LocalCachedFile>(
        fileName, std::ios_base::in);
}

OCIO_ADD_TEST(FileFormatSpiMtx, Test)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string spiMtxFile("camera_to_aces.spimtx");
    OCIO_CHECK_NO_THROW(cachedFile = LoadLutFile(spiMtxFile));

    OCIO_REQUIRE_ASSERT((bool)cachedFile);
    OCIO_CHECK_EQUAL(0.0, cachedFile->offset4[0]);
    OCIO_CHECK_EQUAL(0.0, cachedFile->offset4[1]);
    OCIO_CHECK_EQUAL(0.0, cachedFile->offset4[2]);
    OCIO_CHECK_EQUAL(0.0, cachedFile->offset4[3]);

    OCIO_CHECK_EQUAL(0.754338638f, (float)cachedFile->m44[0]);
    OCIO_CHECK_EQUAL(0.133697046f, (float)cachedFile->m44[1]);
    OCIO_CHECK_EQUAL(0.111968437f, (float)cachedFile->m44[2]);
    OCIO_CHECK_EQUAL(0.0, cachedFile->m44[3]);

    OCIO_CHECK_EQUAL(0.021198141f, (float)cachedFile->m44[4]);
    OCIO_CHECK_EQUAL(1.005410934f, (float)cachedFile->m44[5]);
    OCIO_CHECK_EQUAL(-0.026610548f, (float)cachedFile->m44[6]);
    OCIO_CHECK_EQUAL(0.0, cachedFile->m44[7]);

    OCIO_CHECK_EQUAL(-0.009756991f, (float)cachedFile->m44[8]);
    OCIO_CHECK_EQUAL(0.004508563f, (float)cachedFile->m44[9]);
    OCIO_CHECK_EQUAL(1.005253201f, (float)cachedFile->m44[10]);
    OCIO_CHECK_EQUAL(0.0, cachedFile->m44[11]);

    OCIO_CHECK_EQUAL(0.0, cachedFile->m44[12]);
    OCIO_CHECK_EQUAL(0.0, cachedFile->m44[13]);
    OCIO_CHECK_EQUAL(0.0, cachedFile->m44[14]);
    OCIO_CHECK_EQUAL(1.0, cachedFile->m44[15]);
}

OCIO::LocalCachedFileRcPtr ReadSpiMtx(const std::string & fileContent)
{
    std::istringstream is;
    is.str(fileContent);

    // Read file
    OCIO::LocalFileFormat tester;
    const std::string SAMPLE_NAME("Memory File");
    OCIO::CachedFileRcPtr cachedFile = tester.read(is, SAMPLE_NAME);

    return OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(cachedFile);
}

OCIO_ADD_TEST(FileFormatSpiMtx, ReadOffset)
{
    {
        // Validate stream can be read with no error.
        // Then stream will be altered to introduce errors.
        const std::string SAMPLE_FILE =
            "1 0 0 6553.5\n"
            "0 1 0 32767.5\n"
            "0 0 1 65535.0\n";

        OCIO::LocalCachedFileRcPtr cachedFile;
        OCIO_CHECK_NO_THROW(cachedFile = ReadSpiMtx(SAMPLE_FILE));
        OCIO_REQUIRE_ASSERT((bool)cachedFile);
        OCIO_CHECK_EQUAL(0.1, cachedFile->offset4[0]);
        OCIO_CHECK_EQUAL(0.5, cachedFile->offset4[1]);
        OCIO_CHECK_EQUAL(1.0, cachedFile->offset4[2]);
        OCIO_CHECK_EQUAL(0.0, cachedFile->offset4[3]);
    }
}

OCIO_ADD_TEST(FileFormatSpiMtx, ReadFailure)
{
    {
        // Validate stream can be read with no error.
        // Then stream will be altered to introduce errors.
        const std::string SAMPLE_NO_ERROR =
            "1.0 0.0 0.0 0.0\n"
            "0.0 1.0 0.0 0.0\n"
            "0.0 0.0 1.0 0.0\n";

        OCIO_CHECK_NO_THROW(ReadSpiMtx(SAMPLE_NO_ERROR));
    }
    {
        // Wrong number of elements
        const std::string SAMPLE_ERROR =
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "0.0 0.0 1.0\n";

        OCIO_CHECK_THROW_WHAT(ReadSpiMtx(SAMPLE_ERROR),
                              OCIO::Exception,
                              "File must contain 12 float entries");
    }
    {
        // Some elements can' t be read as float
        const std::string SAMPLE_ERROR =
            "1.0 0.0 0.0 0.0\n"
            "0.0 error 0.0 0.0\n"
            "0.0 0.0 1.0 0.0\n";

        OCIO_CHECK_THROW_WHAT(ReadSpiMtx(SAMPLE_ERROR),
                              OCIO::Exception,
                              "File must contain all float entries");
    }
}

#endif // OCIO_UNIT_TEST

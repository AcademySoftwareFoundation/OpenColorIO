// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstdio>
#include <iostream>
#include <iterator>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "ops/Lut1D/Lut1DOp.h"
#include "ops/Lut3D/Lut3DOp.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"
#include "transforms/FileTransform.h"

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        class LocalCachedFile : public CachedFile
        {
        public:
            LocalCachedFile () = default;
            ~LocalCachedFile() = default;

            Lut3DOpDataRcPtr lut3D;
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

        private:
            static void ThrowErrorMessage(const std::string & error,
                const std::string & fileName,
                int line,
                const std::string & lineContent);
        };

        void LocalFileFormat::ThrowErrorMessage(const std::string & error,
            const std::string & fileName,
            int line,
            const std::string & lineContent)
        {
            std::ostringstream os;
            os << "Error parsing Pandora LUT file (";
            os << fileName;
            os << ").  ";
            if (-1 != line)
            {
                os << "At line (" << line << "): '";
                os << lineContent << "'.  ";
            }
            os << error;

            throw Exception(os.str().c_str());
        }

        void LocalFileFormat::getFormatInfo(FormatInfoVec & formatInfoVec) const
        {
            FormatInfo info;
            info.name = "pandora_mga";
            info.extension = "mga";
            info.capabilities = FORMAT_CAPABILITY_READ;
            formatInfoVec.push_back(info);

            FormatInfo info2;
            info2.name = "pandora_m3d";
            info2.extension = "m3d";
            info2.capabilities = FORMAT_CAPABILITY_READ;
            formatInfoVec.push_back(info2);
        }
        
        CachedFileRcPtr LocalFileFormat::read(
            std::istream & istream,
            const std::string & fileName) const
        {
            // this shouldn't happen
            if(!istream)
            {
                throw Exception ("File stream empty when trying "
                                 "to read Pandora LUT");
            }

            // Validate the file type
            std::string line;

            // Parse the file
            std::string format;
            int lutEdgeLen = 0;
            int outputBitDepthMaxValue = 0;
            std::vector<int> raw3d;

            {
                StringVec parts;
                std::vector<int> tmpints;
                bool inLut3d = false;
                int lineNumber = 0;

                while(nextline(istream, line))
                {
                    ++lineNumber;

                    // Strip, lowercase, and split the line
                    pystring::split(pystring::lower(pystring::strip(line)),
                                    parts);
                    if(parts.empty()) continue;

                    // Skip all lines starting with '#'
                    if(pystring::startswith(parts[0],"#")) continue;

                    if(parts[0] == "channel")
                    {
                        if(parts.size() != 2 
                            || pystring::lower(parts[1]) != "3d")
                        {
                            ThrowErrorMessage(
                                "Only 3D LUTs are currently supported "
                                "(channel: 3d).",
                                fileName,
                                lineNumber,
                                line);
                        }
                    }
                    else if(parts[0] == "in")
                    {
                        int inval = 0;
                        if(parts.size() != 2
                            || !StringToInt( &inval, parts[1].c_str()))
                        {
                            ThrowErrorMessage(
                                "Malformed 'in' tag.",
                                fileName,
                                lineNumber,
                                line);
                        }
                        raw3d.reserve(inval*3);
                        lutEdgeLen = Get3DLutEdgeLenFromNumPixels(inval);
                    }
                    else if(parts[0] == "out")
                    {
                        if(parts.size() != 2 
                            || !StringToInt(&outputBitDepthMaxValue,
                                            parts[1].c_str()))
                        {
                            ThrowErrorMessage(
                                "Malformed 'out' tag.",
                                fileName,
                                lineNumber,
                                line);
                        }
                    }
                    else if(parts[0] == "format")
                    {
                        if(parts.size() != 2 
                            || pystring::lower(parts[1]) != "lut")
                        {
                            ThrowErrorMessage(
                                "Only LUTs are currently supported "
                                "(format: lut).",
                                fileName,
                                lineNumber,
                                line);
                        }
                    }
                    else if(parts[0] == "values")
                    {
                        if(parts.size() != 4 || 
                           pystring::lower(parts[1]) != "red" ||
                           pystring::lower(parts[2]) != "green" ||
                           pystring::lower(parts[3]) != "blue")
                        {
                            ThrowErrorMessage(
                                "Only rgb LUTs are currently supported "
                                "(values: red green blue).",
                                fileName,
                                lineNumber,
                                line);
                        }

                        inLut3d = true;
                    }
                    else if(inLut3d)
                    {
                        if(!StringVecToIntVec(tmpints, parts) || tmpints.size() != 4)
                        {
                            ThrowErrorMessage(
                                "Expected to find 4 integers.",
                                fileName,
                                lineNumber,
                                line);
                        }

                        raw3d.push_back(tmpints[1]);
                        raw3d.push_back(tmpints[2]);
                        raw3d.push_back(tmpints[3]);
                    }
                }
            }

            // Interpret the parsed data, validate LUT sizes
            if(lutEdgeLen*lutEdgeLen*lutEdgeLen != static_cast<int>(raw3d.size()/3))
            {
                std::ostringstream os;
                os << "Incorrect number of 3D LUT entries. ";
                os << "Found " << raw3d.size() / 3 << ", expected ";
                os << lutEdgeLen*lutEdgeLen*lutEdgeLen << ".";
                ThrowErrorMessage(
                    os.str().c_str(),
                    fileName, -1, "");
            }

            if(lutEdgeLen*lutEdgeLen*lutEdgeLen == 0)
            {
                ThrowErrorMessage(
                    "No 3D LUT entries found.",
                    fileName, -1, "");
            }

            if(outputBitDepthMaxValue <= 0)
            {
                ThrowErrorMessage(
                    "A valid 'out' tag was not found.",
                    fileName, -1, "");
            }

            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());

            // Copy raw3d vector into LutOpData object.
            cachedFile->lut3D = std::make_shared<Lut3DOpData>(lutEdgeLen);

            BitDepth fileBD = GetBitdepthFromMaxValue(outputBitDepthMaxValue);
            cachedFile->lut3D->setFileOutputBitDepth(fileBD);

            auto & lutArray = cachedFile->lut3D->getArray();

            float scale = 1.0f / ((float)outputBitDepthMaxValue - 1.0f);

            // lutArray and LUT in file are blue fastest.
            for (size_t i = 0; i < raw3d.size(); ++i)
            {
                lutArray[(unsigned long)i] = static_cast<float>(raw3d[i]) * scale;
            }

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
            if(!cachedFile)
            {
                std::ostringstream os;
                os << "Cannot build Pandora LUT. Invalid cache type.";
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

            if (cachedFile->lut3D)
            {
                cachedFile->lut3D->setInterpolation(fileTransform.getInterpolation());
                CreateLut3DOp(ops, cachedFile->lut3D, newDir);
            }
        }
    }

    FileFormat * CreateFileFormatPandora()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"
#include "UnitTestUtils.h"

OCIO_ADD_TEST(FileFormatPandora, format_info)
{
    OCIO::FormatInfoVec formatInfoVec;
    OCIO::LocalFileFormat tester;
    tester.getFormatInfo(formatInfoVec);

    OCIO_CHECK_EQUAL(2, formatInfoVec.size());
    OCIO_CHECK_EQUAL("pandora_mga", formatInfoVec[0].name);
    OCIO_CHECK_EQUAL("mga", formatInfoVec[0].extension);
    OCIO_CHECK_EQUAL(OCIO::FORMAT_CAPABILITY_READ,
        formatInfoVec[0].capabilities);
    OCIO_CHECK_EQUAL("pandora_m3d", formatInfoVec[1].name);
    OCIO_CHECK_EQUAL("m3d", formatInfoVec[1].extension);
    OCIO_CHECK_EQUAL(OCIO::FORMAT_CAPABILITY_READ,
        formatInfoVec[1].capabilities);
}

void ReadPandora(const std::string & fileContent)
{
    std::istringstream is;
    is.str(fileContent);

    // Read file
    OCIO::LocalFileFormat tester;
    const std::string SAMPLE_NAME("Memory File");
    OCIO::CachedFileRcPtr cachedFile = tester.read(is, SAMPLE_NAME);
}

OCIO_ADD_TEST(FileFormatPandora, read_failure)
{
    {
        // Validate stream can be read with no error.
        // Then stream will be altered to introduce errors.
        const std::string SAMPLE_NO_ERROR =
            "channel 3d\n"
            "in 8\n"
            "out 256\n"
            "format lut\n"
            "values red green blue\n"
            "0 0     0   0\n"
            "1 0     0 255\n"
            "2 0   255   0\n"
            "3 0   255 255\n"
            "4 255   0   0\n"
            "5 255   0 255\n"
            "6 255 255   0\n"
            "7 255 255 255\n";

        OCIO_CHECK_NO_THROW(ReadPandora(SAMPLE_NO_ERROR));
    }
    {
        // Wrong channel tag
        const std::string SAMPLE_ERROR =
            "channel 2d\n"
            "in 8\n"
            "out 256\n"
            "format lut\n"
            "values red green blue\n"
            "0 0     0   0\n"
            "1 0     0 255\n"
            "2 0   255   0\n"
            "3 0   255 255\n"
            "4 255   0   0\n"
            "5 255   0 255\n"
            "6 255 255   0\n"
            "7 255 255 255\n";

        OCIO_CHECK_THROW_WHAT(ReadPandora(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Only 3D LUTs are currently supported");
    }
    {
        // No value spec (LUT will not be read)
        const std::string SAMPLE_ERROR =
            "channel 3d\n"
            "in 8\n"
            "out 256\n"
            "format lut\n"
            "0 0     0   0\n"
            "1 0     0 255\n"
            "2 0   255   0\n"
            "3 0   255 255\n"
            "4 255   0   0\n"
            "5 255   0 255\n"
            "6 255 255   0\n"
            "7 255 255 255\n";

        OCIO_CHECK_THROW_WHAT(ReadPandora(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Incorrect number of 3D LUT entries");
    }
    {
        // Wrong entry
        const std::string SAMPLE_ERROR =
            "channel 3d\n"
            "in 8\n"
            "out 256\n"
            "format lut\n"
            "values red green blue\n"
            "0 0     0   0\n"
            "1 0     0 255\n"
            "2 0   255   0\n"
            "3 0   255 255\n"
            "4 WRONG 255   0   0\n"
            "5 255   0 255\n"
            "6 255 255   0\n"
            "7 255 255 255\n";

        OCIO_CHECK_THROW_WHAT(ReadPandora(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Expected to find 4 integers");
    }
    {
        // Wrong number of entries
        const std::string SAMPLE_ERROR =
            "channel 3d\n"
            "in 8\n"
            "out 256\n"
            "format lut\n"
            "values red green blue\n"
            "0 0     0   0\n"
            "1 0     0 255\n"
            "2 0   255   0\n"
            "3 0   255 255\n"
            "4 255   0   0\n"
            "5 255   0 255\n"
            "6 255 255   0\n"
            "7 255 255   0\n"
            "8 255 255 255\n";

        OCIO_CHECK_THROW_WHAT(ReadPandora(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Incorrect number of 3D LUT entries");
    }
}

OCIO_ADD_TEST(FileFormatPandora, load_op)
{
    const std::string fileName("pandora_3d.m3d");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, fileName, context,
        OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_EQUAL("<FileNoOp>", ops[0]->getInfo());
    OCIO_CHECK_EQUAL("<Lut3DOp>", ops[1]->getInfo());

    auto op1 = std::const_pointer_cast<const OCIO::Op>(ops[1]);
    auto opData1 = op1->data();
    auto lut = std::dynamic_pointer_cast<const OCIO::Lut3DOpData>(opData1);
    OCIO_REQUIRE_ASSERT(lut);
    OCIO_CHECK_EQUAL(lut->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_EQUAL(lut->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_EQUAL(lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);

    auto & lutArray = lut->getArray();
    OCIO_REQUIRE_EQUAL(lutArray.getNumValues(), 24);
    const float error = 1e-7f;
    OCIO_CHECK_CLOSE(lutArray[0], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[1], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[2], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[3], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[4], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[5], 0.8f, error);
    OCIO_CHECK_CLOSE(lutArray[6], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[7], 0.8f, error);
    OCIO_CHECK_CLOSE(lutArray[8], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[9], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[10], 0.8f, error);
    OCIO_CHECK_CLOSE(lutArray[11], 0.8f, error);
    OCIO_CHECK_CLOSE(lutArray[12], 1.0f, error);
    OCIO_CHECK_CLOSE(lutArray[13], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[14], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[15], 1.0f, error);
    OCIO_CHECK_CLOSE(lutArray[16], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[17], 1.0f, error);
    OCIO_CHECK_CLOSE(lutArray[18], 1.0f, error);
    OCIO_CHECK_CLOSE(lutArray[19], 1.0f, error);
    OCIO_CHECK_CLOSE(lutArray[20], 0.0f, error);
    OCIO_CHECK_CLOSE(lutArray[21], 1.2f, error);
    OCIO_CHECK_CLOSE(lutArray[22], 1.0f, error);
    OCIO_CHECK_CLOSE(lutArray[23], 1.2f, error);
}

#endif // OCIO_UNIT_TEST
// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "transforms/FileTransform.h"
#include "ops/Lut1D/Lut1DOp.h"
#include "ops/Matrix/MatrixOps.h"
#include "ParseUtils.h"
#include "Platform.h"
#include "pystring/pystring.h"

#include <cstdio>
#include <sstream>

/*
Version 1
From -7.5 3.7555555555555555
Components 1
Length 4096
{
        0.031525943963232252
        0.045645604561056156
	...
}

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
            
            Lut1DOpDataRcPtr lut;
            float from_min = 0.0f;
            float from_max = 1.0f;
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
        
        void LocalFileFormat::getFormatInfo(FormatInfoVec & formatInfoVec) const
        {
            FormatInfo info;
            info.name = "spi1d";
            info.extension = "spi1d";
            info.capabilities = FORMAT_CAPABILITY_READ;
            formatInfoVec.push_back(info);
        }
        
        // Try and load the format.
        // Raise an exception if it can't be loaded.

        CachedFileRcPtr LocalFileFormat::read(
            std::istream & istream,
            const std::string & fileName ) const
        {
            // Parse Header Info.
            int lut_size = -1;
            float from_min = 0.0;
            float from_max = 1.0;
            int version = -1;
            int components = -1;

            const int MAX_LINE_SIZE = 4096;
            char lineBuffer[MAX_LINE_SIZE];
            int currentLine = 0;

            // PARSE HEADER INFO
            {
                std::string headerLine("");
                do
                {
                    istream.getline(lineBuffer, MAX_LINE_SIZE);
                    ++currentLine;
                    headerLine = std::string(lineBuffer);

                    if(pystring::startswith(headerLine, "Version"))
                    {
                        // " " in format means any number of spaces (white space,
                        // new line, tab) including 0 of them.
                        // "Version1" is valid.
                        if (sscanf(lineBuffer, "Version %d", &version) != 1)
                        {
                            ThrowErrorMessage("Invalid 'Version' Tag.",
                                              fileName, currentLine, headerLine);
                        }
                        else if (version != 1)
                        {
                            ThrowErrorMessage("Only format version 1 supported.",
                                              fileName, currentLine, headerLine);
                        }
                    }
                    else if(pystring::startswith(headerLine, "From"))
                    {
                        if (sscanf(lineBuffer, "From %f %f", &from_min, &from_max) != 2)
                        {
                            ThrowErrorMessage("Invalid 'From' Tag.",
                                              fileName, currentLine, headerLine);
                        }
                    }
                    else if(pystring::startswith(headerLine, "Components"))
                    {
                        if (sscanf(lineBuffer, "Components %d", &components) != 1)
                        {
                            ThrowErrorMessage("Invalid 'Components' Tag.",
                                              fileName, currentLine, headerLine);
                        }
                    }
                    else if(pystring::startswith(headerLine, "Length"))
                    {
                        if (sscanf(lineBuffer, "Length %d", &lut_size) != 1)
                        {
                            ThrowErrorMessage("Invalid 'Length' Tag.",
                                              fileName, currentLine, headerLine);
                        }
                    }
                }
                while (istream.good() && !pystring::startswith(headerLine,"{"));
            }

            if (version == -1)
            {
                ThrowErrorMessage("Could not find 'Version' Tag.",
                                  fileName, -1, "");
            }
            if (lut_size == -1)
            {
                ThrowErrorMessage("Could not find 'Length' Tag.",
                                  fileName, -1, "");
            }
            if (components == -1)
            {
                ThrowErrorMessage("Could not find 'Components' Tag.",
                                   fileName, -1, "");
            }
            if (components < 0 || components>3)
            {
                ThrowErrorMessage("Components must be [1,2,3].",
                                  fileName, -1, "");
            }

            Lut1DOpDataRcPtr lut1d = std::make_shared<Lut1DOpData>(lut_size);
            lut1d->setFileOutputBitDepth(BIT_DEPTH_F32);
            Array & lutArray = lut1d->getArray();
            unsigned long i = 0;
            {
                istream.getline(lineBuffer, MAX_LINE_SIZE);
                ++currentLine;

                int lineCount=0;
                
                std::vector<std::string> inputLUT;
                std::vector<float> values;

                while (istream.good())
                {
                    const std::string line = pystring::strip(std::string(lineBuffer));
                    if (Platform::Strcasecmp(line.c_str(), "}") == 0)
                    {
                        break;
                    }

                    if (line.length() != 0)
                    {
                        pystring::split(pystring::strip(lineBuffer), inputLUT);
                        values.clear();
                        if (!StringVecToFloatVec(values, inputLUT)
                            || components != (int)values.size())
                        {
                            std::ostringstream os;
                            os << "Malformed LUT line. Expecting a ";
                            os << components << " components entry.";

                            ThrowErrorMessage("Malformed LUT line.",
                                              fileName, currentLine, line);
                        }

                        // If 1 component is specified, use x1 x1 x1.
                        if (components == 1)
                        {
                            lutArray[i]     = values[0];
                            lutArray[i + 1] = values[0];
                            lutArray[i + 2] = values[0];
                            i += 3;
                            ++lineCount;
                        }
                        // If 2 components are specified, use x1 x2 0.0.
                        else if (components == 2)
                        {
                            lutArray[i]     = values[0];
                            lutArray[i + 1] = values[1];
                            lutArray[i + 2] = 0.0f;
                            i += 3;
                            ++lineCount;
                        }
                        // If 3 component is specified, use x1 x2 x3.
                        else if (components == 3)
                        {
                            lutArray[i]     = values[0];
                            lutArray[i + 1] = values[1];
                            lutArray[i + 2] = values[2];
                            i += 3;
                            ++lineCount;
                        }
                        // No other case, components is in [1..3].
                    }

                    istream.getline(lineBuffer, MAX_LINE_SIZE);
                    ++currentLine;
                }

                if (lineCount != lut_size)
                {
                    ThrowErrorMessage("Not enough entries found.",
                                      fileName, -1, "");
                }
            }

            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());
            cachedFile->lut = lut1d;
            cachedFile->from_min = from_min;
            cachedFile->from_max = from_max;
            return cachedFile;
        }

        void LocalFileFormat::buildFileOps(OpRcPtrVec & ops,
                                           const Config & /*config*/,
                                           const ConstContextRcPtr & /*context*/,
                                           CachedFileRcPtr untypedCachedFile,
                                           const FileTransform& fileTransform,
                                           TransformDirection dir) const
        {
            LocalCachedFileRcPtr cachedFile = DynamicPtrCast<LocalCachedFile>(untypedCachedFile);

            if(!cachedFile) // This should never happen.
            {
                std::ostringstream os;
                os << "Cannot build Spi1D Op. Invalid cache type.";
                throw Exception(os.str().c_str());
            }

            TransformDirection newDir = fileTransform.getDirection();
            newDir = CombineTransformDirections(dir, newDir);


            const double min[3] = { cachedFile->from_min,
                                    cachedFile->from_min,
                                    cachedFile->from_min };

            const double max[3] = { cachedFile->from_max,
                                    cachedFile->from_max,
                                    cachedFile->from_max };

            cachedFile->lut->setInterpolation(fileTransform.getInterpolation());

            if (newDir == TRANSFORM_DIR_FORWARD)
            {
                CreateMinMaxOp(ops, min, max, TRANSFORM_DIR_FORWARD);
                CreateLut1DOp(ops, cachedFile->lut, TRANSFORM_DIR_FORWARD);
            }
            else
            {
                CreateLut1DOp(ops, cachedFile->lut, TRANSFORM_DIR_INVERSE);
                CreateMinMaxOp(ops, min, max, TRANSFORM_DIR_INVERSE);
            }
        }

        void LocalFileFormat::ThrowErrorMessage(const std::string & error,
                                                const std::string & fileName,
                                                int line,
                                                const std::string & lineContent)
        {
            std::ostringstream os;
            os << "Error parsing .spi1d file (";
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
    }
    
    FileFormat * CreateFileFormatSpi1D()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"
#include "UnitTestUtils.h"

OCIO_ADD_TEST(FileFormatSpi1D, format_info)
{
    OCIO::FormatInfoVec formatInfoVec;
    OCIO::LocalFileFormat tester;
    tester.getFormatInfo(formatInfoVec);

    OCIO_CHECK_EQUAL(1, formatInfoVec.size());
    OCIO_CHECK_EQUAL("spi1d", formatInfoVec[0].name);
    OCIO_CHECK_EQUAL("spi1d", formatInfoVec[0].extension);
    OCIO_CHECK_EQUAL(OCIO::FORMAT_CAPABILITY_READ,
                     formatInfoVec[0].capabilities);
}

OCIO::LocalCachedFileRcPtr LoadLutFile(const std::string & fileName)
{
    return OCIO::LoadTestFile<OCIO::LocalFileFormat, OCIO::LocalCachedFile>(
        fileName, std::ios_base::in);
}

OCIO_ADD_TEST(FileFormatSpi1D, test)
{
    OCIO::LocalCachedFileRcPtr cachedFile;
    const std::string spi1dFile("cpf.spi1d");
    OCIO_CHECK_NO_THROW(cachedFile = LoadLutFile(spi1dFile));

    OCIO_REQUIRE_ASSERT(cachedFile);
    OCIO_REQUIRE_ASSERT(cachedFile->lut);
    OCIO_CHECK_EQUAL(cachedFile->lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    OCIO_CHECK_EQUAL(0.0f, cachedFile->from_min);
    OCIO_CHECK_EQUAL(1.0f, cachedFile->from_max);

    const OCIO::Array & lutArray = cachedFile->lut->getArray();
    OCIO_CHECK_EQUAL(2048ul, lutArray.getLength());

    OCIO_CHECK_EQUAL(0.0f, lutArray[0]);
    OCIO_CHECK_EQUAL(0.0f, lutArray[1]);
    OCIO_CHECK_EQUAL(0.0f, lutArray[2]);

    OCIO_CHECK_EQUAL(4.511920005404118f, lutArray[1970*3]);
    OCIO_CHECK_EQUAL(4.511920005404118f, lutArray[1970*3 + 1]);
    OCIO_CHECK_EQUAL(4.511920005404118f, lutArray[1970*3 + 2]);
}

OCIO::LocalCachedFileRcPtr ReadSpi1d(const std::string & fileContent)
{
    std::istringstream is;
    is.str(fileContent);

    // Read file.
    OCIO::LocalFileFormat tester;
    const std::string SAMPLE_NAME("Memory File");
    OCIO::CachedFileRcPtr cachedFile = tester.read(is, SAMPLE_NAME);
    return OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(cachedFile);
}

OCIO_ADD_TEST(FileFormatSpi1D, read_failure)
{
    {
        // Validate stream can be read with no error.
        // Then stream will be altered to introduce errors.
        const std::string SAMPLE_NO_ERROR =
            "Version 1\n"
            "From 0.0 1.0\n"
            "Length 2\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "\n"
            "1.0\n"
            "}\n";

        OCIO_CHECK_NO_THROW(ReadSpi1d(SAMPLE_NO_ERROR));
    }
    {
        // Version missing.
        const std::string SAMPLE_ERROR =
            "From 0.0 1.0\n"
            "Length 2\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "1.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Could not find 'Version' Tag");
    }
    {
        // Version is not 1.
        const std::string SAMPLE_ERROR =
            "Version 2\n"
            "From 0.0 1.0\n"
            "Length 2\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "1.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Only format version 1 supported");
    }
    {
        // Version can't be scanned.
        const std::string SAMPLE_ERROR =
            "Version A\n"
            "From 0.0 1.0\n"
            "Length 2\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "1.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Invalid 'Version' Tag");
    }
    {
        // Version case is wrong.
        const std::string SAMPLE_ERROR =
            "VERSION 1\n"
            "From 0.0 1.0\n"
            "Length 2\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "1.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Could not find 'Version' Tag");
    }
    {
        // From does not specify 2 floats.
        const std::string SAMPLE_ERROR =
            "Version 1\n"
            "From 0.0\n"
            "Length 2\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "1.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Invalid 'From' Tag");
    }
    {
        // Length is missing.
        const std::string SAMPLE_ERROR =
            "Version 1\n"
            "From 0.0 1.0\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "1.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Could not find 'Length' Tag");
    }
    {
        // Length can't be read.
        const std::string SAMPLE_ERROR =
            "Version 1\n"
            "From 0.0 1.0\n"
            "Length A\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "1.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Invalid 'Length' Tag");
    }
    {
        // Component is missing.
        const std::string SAMPLE_ERROR =
            "Version 1\n"
            "From 0.0 1.0\n"
            "Length 2\n"
            "{\n"
            "0.0\n"
            "1.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Could not find 'Components' Tag");
    }
    {
        // Component can't be read.
        const std::string SAMPLE_ERROR =
            "Version 1\n"
            "From 0.0 1.0\n"
            "Length 2\n"
            "Components A\n"
            "{\n"
            "0.0\n"
            "1.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Invalid 'Components' Tag");
    }
    {
        // Component not 1 or 2 or 3.
        const std::string SAMPLE_ERROR =
            "Version 1\n"
            "From 0.0 1.0\n"
            "Length 2\n"
            "Components 4\n"
            "{\n"
            "0.0\n"
            "1.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Components must be [1,2,3]");
    }
    {
        // LUT too short.
        const std::string SAMPLE_ERROR =
            "Version 1\n"
            "From 0.0 1.0\n"
            "Length 2\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Not enough entries found");
    }
    {
        // Components==1 but two components specified in LUT.
        const std::string SAMPLE_ERROR =
            "Version 1\n"
            "From 0.0 1.0\n"
            "Length 2\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "1.0 1.0\n"
            "}\n";

        OCIO_CHECK_THROW_WHAT(ReadSpi1d(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Malformed LUT line");
    }
}

OCIO_ADD_TEST(FileFormatSpi1D, identity)
{
    {
        const std::string SAMPLE_LUT =
            "Version 1\n"
            "From 0.0 1.0\n"
            "Length 2\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "1.000007\n"
            "}\n";

        OCIO::LocalCachedFileRcPtr parsedLUT;
        OCIO_CHECK_NO_THROW(parsedLUT = ReadSpi1d(SAMPLE_LUT));
        OCIO_REQUIRE_ASSERT(parsedLUT);
        OCIO_REQUIRE_ASSERT(parsedLUT->lut);
        OCIO_CHECK_ASSERT(parsedLUT->lut->isIdentity());
    }
    {
        const std::string SAMPLE_LUT =
            "Version 1\n"
            "From 0.0 1.0\n"
            "Length 2\n"
            "Components 1\n"
            "{\n"
            "0.0\n"
            "1.00001\n"
            "}\n";

        OCIO::LocalCachedFileRcPtr parsedLUT;
        OCIO_CHECK_NO_THROW(parsedLUT = ReadSpi1d(SAMPLE_LUT));
        OCIO_REQUIRE_ASSERT(parsedLUT);
        OCIO_REQUIRE_ASSERT(parsedLUT->lut);
        OCIO_CHECK_ASSERT(!parsedLUT->lut->isIdentity());
    }

}

#endif // OCIO_UNIT_TEST



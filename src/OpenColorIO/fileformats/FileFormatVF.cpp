// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstdio>
#include <cstring>
#include <iostream>
#include <iterator>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/Lut3D/Lut3DOp.h"
#include "ops/Matrix/MatrixOps.h"
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
            LocalCachedFile() = default;
            ~LocalCachedFile() = default;

            Lut3DOpDataRcPtr lut3D;
            double m44[16]{ 0 };
            bool useMatrix = false;
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
            os << "Error parsing Nuke .vf file (";
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
            info.name = "nukevf";
            info.extension = "vf";
            info.capabilities = FORMAT_CAPABILITY_READ;
            formatInfoVec.push_back(info);
        }
        
        CachedFileRcPtr LocalFileFormat::read(
            std::istream & istream,
            const std::string & fileName) const
        {
            // this shouldn't happen
            if(!istream)
            {
                throw Exception ("File stream empty when trying to read .vf LUT");
            }

            // Validate the file type
            std::string line;
            int lineNumber = 1;
            if(!nextline(istream, line) ||
               !pystring::startswith(pystring::lower(line), "#inventor"))
            {
                ThrowErrorMessage("Expecting '#Inventor V2.1 ascii'.",
                    fileName, lineNumber, line);
            }

            // Parse the file
            std::vector<float> raw3d;
            int size3d[] = { 0, 0, 0 };
            std::vector<float> global_transform;

            {
                StringVec parts;
                std::vector<float> tmpfloats;

                bool in3d = false;

                while(nextline(istream, line))
                {
                    ++lineNumber;

                    // Strip, lowercase, and split the line
                    pystring::split(pystring::lower(pystring::strip(line)), parts);

                    if(parts.empty()) continue;

                    if(pystring::startswith(parts[0],"#")) continue;

                    if(!in3d)
                    {
                        if(parts[0] == "grid_size")
                        {
                            if(parts.size() != 4 || 
                               !StringToInt( &size3d[0], parts[1].c_str()) ||
                               !StringToInt( &size3d[1], parts[2].c_str()) ||
                               !StringToInt( &size3d[2], parts[3].c_str()))
                            {
                                ThrowErrorMessage(
                                    "Malformed grid_size tag.",
                                    fileName, lineNumber, line);
                            }

                            // TODO: Support nununiformly sized LUTs.
                            if (size3d[0] != size3d[1] ||
                                size3d[0] != size3d[2])
                            {
                                std::ostringstream os;
                                os << "Only equal grid size LUTs are supported. Found ";
                                os << "grid size: " << size3d[0] << " x ";
                                os << size3d[1] << " x " << size3d[2] << ".";
                                ThrowErrorMessage(
                                    os.str(),
                                    fileName, lineNumber, line);
                            }

                            raw3d.reserve(3*size3d[0]*size3d[1]*size3d[2]);
                        }
                        else if(parts[0] == "global_transform")
                        {
                            if(parts.size() != 17)
                            {
                                ThrowErrorMessage(
                                    "Malformed global_transform tag. "
                                    "16 floats expected.",
                                    fileName, lineNumber, line);
                            }

                            parts.erase(parts.begin()); // Drop the 1st entry. (the tag)
                            if(!StringVecToFloatVec(global_transform, parts) || global_transform.size() != 16)
                            {
                                ThrowErrorMessage(
                                    "Malformed global_transform tag. "
                                    "Could not convert to float array.",
                                    fileName, lineNumber, line);
                            }
                        }
                        // TODO: element_size (aka scale3)
                        // TODO: world_origin (aka translate3)
                        else if(parts[0] == "data")
                        {
                            in3d = true;
                        }
                    }
                    else // (in3d)
                    {
                        if(StringVecToFloatVec(tmpfloats, parts) && (tmpfloats.size() == 3))
                        {
                            raw3d.push_back(tmpfloats[0]);
                            raw3d.push_back(tmpfloats[1]);
                            raw3d.push_back(tmpfloats[2]);
                        }
                    }
                }
            }

            // Interpret the parsed data, validate LUT sizes
            if(size3d[0]*size3d[1]*size3d[2] != static_cast<int>(raw3d.size()/3))
            {
                std::ostringstream os;
                os << "Incorrect number of 3D LUT entries. ";
                os << "Found " << raw3d.size()/3 << ", expected " << size3d[0]*size3d[1]*size3d[2] << ".";
                ThrowErrorMessage(
                    os.str().c_str(),
                    fileName, -1, "");
            }

            if(size3d[0]*size3d[1]*size3d[2] == 0)
            {
                ThrowErrorMessage(
                    "No 3D LUT entries found.",
                    fileName, -1, "");
            }

            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());

            // Setup the global matrix.
            // (Nuke pre-scales this by the 3D LUT size, so we must undo that here)
            if(global_transform.size() == 16)
            {
                for(int i=0; i<4; ++i)
                {
                    global_transform[4*i+0] *= static_cast<float>(size3d[0]);
                    global_transform[4*i+1] *= static_cast<float>(size3d[1]);
                    global_transform[4*i+2] *= static_cast<float>(size3d[2]);
                    cachedFile->m44[4*i+0] = global_transform[4*i+0];
                    cachedFile->m44[4*i+1] = global_transform[4*i+1];
                    cachedFile->m44[4*i+2] = global_transform[4*i+2];
                    cachedFile->m44[4*i+3] = global_transform[4*i+3];
                }

                cachedFile->useMatrix = true;
            }

            // Copy raw3d vector into LutOpData object.
            cachedFile->lut3D = std::make_shared<Lut3DOpData>(size3d[0]);
            cachedFile->lut3D->setFileOutputBitDepth(BIT_DEPTH_F32);
            // LUT in file is blue fastest.
            cachedFile->lut3D->getArray().getValues() = raw3d;

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
            if(!cachedFile)
            {
                std::ostringstream os;
                os << "Cannot build .vf Op. Invalid cache type.";
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
            }

            if(newDir == TRANSFORM_DIR_FORWARD)
            {
                if(cachedFile->useMatrix)
                {
                    CreateMatrixOp(ops, cachedFile->m44, newDir);
                }

                CreateLut3DOp(ops, cachedFile->lut3D, newDir);
            }
            else if(newDir == TRANSFORM_DIR_INVERSE)
            {
                CreateLut3DOp(ops, cachedFile->lut3D, newDir);

                if(cachedFile->useMatrix)
                {
                    CreateMatrixOp(ops, cachedFile->m44, newDir);
                }
            }
        }
    }

    FileFormat * CreateFileFormatVF()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

#include <fstream>
namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"
#include "UnitTestUtils.h"

OCIO_ADD_TEST(FileFormatVF, format_info)
{
    OCIO::FormatInfoVec formatInfoVec;
    OCIO::LocalFileFormat tester;
    tester.getFormatInfo(formatInfoVec);

    OCIO_CHECK_EQUAL(1, formatInfoVec.size());
    OCIO_CHECK_EQUAL("nukevf", formatInfoVec[0].name);
    OCIO_CHECK_EQUAL("vf", formatInfoVec[0].extension);
    OCIO_CHECK_EQUAL(OCIO::FORMAT_CAPABILITY_READ,
        formatInfoVec[0].capabilities);
}

void ReadVF(const std::string & fileContent)
{
    std::istringstream is;
    is.str(fileContent);

    // Read file.
    OCIO::LocalFileFormat tester;
    const std::string SAMPLE_NAME("Memory File");
    OCIO::CachedFileRcPtr cachedFile = tester.read(is, SAMPLE_NAME);
}

OCIO_ADD_TEST(FileFormatVF, read_failure)
{
    {
        // Validate stream can be read with no error.
        // Then stream will be altered to introduce errors.
        const std::string SAMPLE_NO_ERROR =
            "#Inventor V2.1 ascii\n"
            "grid_size 2 2 2\n"
            "global_transform 1 0 0 0  0 1 0 0  0 0 1 0  0 0 0 1 \n"
            "data\n"
            "0 0 0\n"
            "0 0 1\n"
            "0 1 0\n"
            "0 1 1\n"
            "1 0 0\n"
            "1 0 1\n"
            "1 1 0\n"
            "1 1 1\n";

        OCIO_CHECK_NO_THROW(ReadVF(SAMPLE_NO_ERROR));
    }
    {
        // Too much data
        const std::string SAMPLE_ERROR =
            "#Inventor V2.1 ascii\n"
            "grid_size 2 2 2\n"
            "global_transform 1 0 0 0  0 1 0 0  0 0 1 0  0 0 0 1 \n"
            "data\n"
            "0 0 0\n"
            "0 0 1\n"
            "0 1 0\n"
            "0 1 1\n"
            "1 0 0\n"
            "1 0 1\n"
            "1 1 0\n"
            "1 1 0\n"
            "1 1 1\n";

        OCIO_CHECK_THROW_WHAT(ReadVF(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Incorrect number of 3D LUT entries");
    }
}

OCIO_ADD_TEST(FileFormatVF, load_ops)
{
    const std::string vfFileName("nuke_3d.vf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, vfFileName, context,
        OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO_CHECK_EQUAL("<FileNoOp>", ops[0]->getInfo());
    OCIO_CHECK_EQUAL("<MatrixOffsetOp>", ops[1]->getInfo());
    OCIO_CHECK_EQUAL("<Lut3DOp>", ops[2]->getInfo());

    auto op1 = std::const_pointer_cast<const OCIO::Op>(ops[1]);
    auto opData1 = op1->data();
    auto mat = std::dynamic_pointer_cast<const OCIO::MatrixOpData>(opData1);
    OCIO_REQUIRE_ASSERT(mat);
    auto & matArray = mat->getArray();
    OCIO_CHECK_EQUAL(matArray[0], 2.0f);
    OCIO_CHECK_EQUAL(matArray[1], 0.0f);
    OCIO_CHECK_EQUAL(matArray[2], 0.0f);
    OCIO_CHECK_EQUAL(matArray[3], 0.0f);
    OCIO_CHECK_EQUAL(matArray[4], 0.0f);
    OCIO_CHECK_EQUAL(matArray[5], 2.0f);
    OCIO_CHECK_EQUAL(matArray[6], 0.0f);
    OCIO_CHECK_EQUAL(matArray[7], 0.0f);
    OCIO_CHECK_EQUAL(matArray[8], 0.0f);
    OCIO_CHECK_EQUAL(matArray[9], 0.0f);
    OCIO_CHECK_EQUAL(matArray[10], 2.0f);
    OCIO_CHECK_EQUAL(matArray[11], 0.0f);
    OCIO_CHECK_EQUAL(matArray[12], 0.0f);
    OCIO_CHECK_EQUAL(matArray[13], 0.0f);
    OCIO_CHECK_EQUAL(matArray[14], 0.0f);
    OCIO_CHECK_EQUAL(matArray[15], 1.0f);

    auto op2 = std::const_pointer_cast<const OCIO::Op>(ops[2]);
    auto opData2 = op2->data();
    auto lut = std::dynamic_pointer_cast<const OCIO::Lut3DOpData>(opData2);
    OCIO_REQUIRE_ASSERT(lut);
    OCIO_CHECK_EQUAL(lut->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_EQUAL(lut->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_EQUAL(lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    auto & lutArray = lut->getArray();
    OCIO_REQUIRE_EQUAL(lutArray.getNumValues(), 24);
    OCIO_CHECK_EQUAL(lutArray[0], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[1], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[2], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[3], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[4], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[5], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[6], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[7], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[8], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[9], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[10], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[11], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[12], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[13], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[14], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[15], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[16], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[17], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[18], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[19], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[20], 0.0f);
    OCIO_CHECK_EQUAL(lutArray[21], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[22], 2.0f);
    OCIO_CHECK_EQUAL(lutArray[23], 2.0f);
}

#endif // OCIO_UNIT_TEST

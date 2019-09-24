// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <iterator>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/Lut1D/Lut1DOp.h"
#include "ops/Lut3D/Lut3DOp.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"
#include "transforms/FileTransform.h"

// This implements the spec for:
// Per http://www.filmlight.ltd.uk/resources/documents/truelight/white-papers_tl.php
// FL-TL-TN-0388-TLCubeFormat2.0.pdf
//
// Known deficiency in implementation:
// 1D shaper LUTs (InputLUT) using integer encodings (vs float) are not supported.
// How to we determine if the input is integer? MaxVal?  Or do we look for a decimal-point?
// How about scientific notation? (which is explicitly allowed?)

/*
The input LUT is used to interpolate a higher precision LUT matched to the particular image
format. For integer formats, the range 0-1 is mapped onto the integer range. Floating point
values outside the 0-1 range are allowed but may be truncated for integer formats.
*/


OCIO_NAMESPACE_ENTER
{
    namespace
    {
        class LocalCachedFile : public CachedFile
        {
        public:
            LocalCachedFile() = default;
            ~LocalCachedFile() = default;

            Lut1DOpDataRcPtr lut1D;
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
            
            void bake(const Baker & baker,
                      const std::string & formatName,
                      std::ostream & ostream) const override;
            
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
            info.name = "truelight";
            info.extension = "cub";
            info.capabilities = (FORMAT_CAPABILITY_READ | FORMAT_CAPABILITY_BAKE);
            formatInfoVec.push_back(info);
        }

        CachedFileRcPtr
        LocalFileFormat::read(
            std::istream & istream,
            const std::string & /* fileName unused */) const
        {
            // this shouldn't happen
            if(!istream)
            {
                throw Exception ("File stream empty when trying to read Truelight .cub LUT");
            }

            // Validate the file type
            std::string line;
            if(!nextline(istream, line) || 
               !pystring::startswith(pystring::lower(line), "# truelight cube"))
            {
                throw Exception("LUT doesn't seem to be a Truelight .cub LUT.");
            }

            // Parse the file
            std::vector<float> raw1d;
            std::vector<float> raw3d;
            int size3d[] = { 0, 0, 0 };
            int size1d = 0;
            {
                StringVec parts;
                std::vector<float> tmpfloats;

                bool in1d = false;
                bool in3d = false;

                while(nextline(istream, line))
                {
                    // Strip, lowercase, and split the line
                    pystring::split(pystring::lower(pystring::strip(line)), parts);

                    if(parts.empty()) continue;

                    // Parse header metadata (which starts with #)
                    if(pystring::startswith(parts[0],"#"))
                    {
                        if(parts.size() < 2) continue;

                        if(parts[1] == "width")
                        {
                            if(parts.size() != 5 || 
                               !StringToInt( &size3d[0], parts[2].c_str()) ||
                               !StringToInt( &size3d[1], parts[3].c_str()) ||
                               !StringToInt( &size3d[2], parts[4].c_str()))
                            {
                                throw Exception("Malformed width tag in Truelight .cub LUT.");
                            }

                            if (size3d[0] != size3d[1] ||
                                size3d[0] != size3d[2])
                            {
                                std::ostringstream os;
                                os << "Truelight .cub LUT. ";
                                os << "Only equal grid size LUTs are supported. Found ";
                                os << "grid size: " << size3d[0] << " x ";
                                os << size3d[1] << " x " << size3d[2] << ".";
                                throw Exception(os.str().c_str());
                            }

                            raw3d.reserve(3*size3d[0]*size3d[1]*size3d[2]);
                        }
                        else if(parts[1] == "lutlength")
                        {
                            if(parts.size() != 3 || 
                               !StringToInt( &size1d, parts[2].c_str()))
                            {
                                throw Exception("Malformed lutlength tag in Truelight .cub LUT.");
                            }
                            raw1d.reserve(3*size1d);
                        }
                        else if(parts[1] == "inputlut")
                        {
                            in1d = true;
                            in3d = false;
                        }
                        else if(parts[1] == "cube")
                        {
                            in3d = true;
                            in1d = false;
                        }
                        else if(parts[1] == "end")
                        {
                            in3d = false;
                            in1d = false;

                            // If we hit the end tag, don't bother searching further in the file.
                            break;
                        }
                    }


                    if(in1d || in3d)
                    {
                        if(StringVecToFloatVec(tmpfloats, parts) && (tmpfloats.size() == 3))
                        {
                            if(in1d)
                            {
                                raw1d.push_back(tmpfloats[0]);
                                raw1d.push_back(tmpfloats[1]);
                                raw1d.push_back(tmpfloats[2]);
                            }
                            else if(in3d)
                            {
                                raw3d.push_back(tmpfloats[0]);
                                raw3d.push_back(tmpfloats[1]);
                                raw3d.push_back(tmpfloats[2]);
                            }
                        }
                    }
                }
            }

            // Interpret the parsed data, validate LUT sizes

            if(size1d != static_cast<int>(raw1d.size()/3))
            {
                std::ostringstream os;
                os << "Parse error in Truelight .cub LUT. ";
                os << "Incorrect number of lut1d entries. ";
                os << "Found " << raw1d.size()/3 << ", expected " << size1d << ".";
                throw Exception(os.str().c_str());
            }

            if(size3d[0]*size3d[1]*size3d[2] != static_cast<int>(raw3d.size()/3))
            {
                std::ostringstream os;
                os << "Parse error in Truelight .cub LUT. ";
                os << "Incorrect number of 3D LUT entries. ";
                os << "Found " << raw3d.size()/3 << ", expected " << size3d[0]*size3d[1]*size3d[2] << ".";
                throw Exception(os.str().c_str());
            }


            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());

            const bool has3D = (size3d[0] * size3d[1] * size3d[2] > 0);

            // Reformat 1D data
            if(size1d>0)
            {
                cachedFile->lut1D = std::make_shared<Lut1DOpData>(size1d);
                cachedFile->lut1D->setFileOutputBitDepth(BIT_DEPTH_F32);

                auto & lutArray = cachedFile->lut1D->getArray();

                // Determine the scale factor for the 1D LUT. Example:
                // The inputlut feeding a 6x6x6 3D LUT should be scaled from 0.0-5.0.
                // Beware: Nuke Truelight Writer (at least 6.3 and before) is busted
                // and does this scaling incorrectly.

                float descale = 1.0f;
                if(has3D)
                {
                    descale = 1.0f / static_cast<float>(size3d[0]-1);
                }

                const auto nv = lutArray.getNumValues();
                for(unsigned long i = 0; i < nv; ++i)
                {
                    lutArray[i] = raw1d[i] * descale;
                }
            }

            if (has3D)
            {
                // Reformat 3D data
                cachedFile->lut3D = std::make_shared<Lut3DOpData>(size3d[0]);
                cachedFile->lut3D->setFileOutputBitDepth(BIT_DEPTH_F32);
                cachedFile->lut3D->setArrayFromRedFastestOrder(raw3d);
            }

            return cachedFile;
        }


        void
        LocalFileFormat::bake(const Baker & baker,
                              const std::string & /*formatName*/,
                              std::ostream & ostream) const
        {
            const int DEFAULT_CUBE_SIZE = 32;
            const int DEFAULT_SHAPER_SIZE = 1024;

            ConstConfigRcPtr config = baker.getConfig();
            
            int cubeSize = baker.getCubeSize();
            if (cubeSize==-1) cubeSize = DEFAULT_CUBE_SIZE;
            cubeSize = std::max(2, cubeSize); // smallest cube is 2x2x2

            std::vector<float> cubeData;
            cubeData.resize(cubeSize*cubeSize*cubeSize*3);
            GenerateIdentityLut3D(&cubeData[0], cubeSize, 3, LUT3DORDER_FAST_RED);
            PackedImageDesc cubeImg(&cubeData[0], cubeSize*cubeSize*cubeSize, 1, 3);

            // Apply processor to LUT data
            ConstCPUProcessorRcPtr inputToTarget;
            inputToTarget
                = config->getProcessor(baker.getInputSpace(), 
                                       baker.getTargetSpace())->getDefaultCPUProcessor();
            inputToTarget->apply(cubeImg);

            int shaperSize = baker.getShaperSize();
            if (shaperSize==-1) shaperSize = DEFAULT_SHAPER_SIZE;
            shaperSize = std::max(2, shaperSize); // smallest shaper is 2x2x2


            // Write the header
            ostream << "# Truelight Cube v2.0\n";
            ostream << "# lutLength " << shaperSize << "\n";
            ostream << "# iDims     3\n";
            ostream << "# oDims     3\n";
            ostream << "# width     " << cubeSize << " " << cubeSize << " " << cubeSize << "\n";
            ostream << "\n";


            // Write the shaper LUT
            // (We are just going to use a unity LUT)
            ostream << "# InputLUT\n";
            ostream << std::setprecision(6) << std::fixed;
            float v = 0.0f;
            for (int i=0; i < shaperSize-1; i++)
            {
                v = ((float)i / (float)(shaperSize-1)) * (float)(cubeSize-1);
                ostream << v << " " << v << " " << v << "\n";
            }
            v = (float) (cubeSize-1);
            ostream << v << " " << v << " " << v << "\n"; // ensure that the last value is spot on
            ostream << "\n";

            // Write the cube
            ostream << "# Cube\n";
            for (int i=0; i<cubeSize*cubeSize*cubeSize; ++i)
            {
                ostream << cubeData[3*i+0] << " " << cubeData[3*i+1] << " " << cubeData[3*i+2] << "\n";
            }

            ostream << "# end\n";
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
                os << "Cannot build Truelight .cub Op. Invalid cache type.";
                throw Exception(os.str().c_str());
            }

            TransformDirection newDir = CombineTransformDirections(dir,
                fileTransform.getDirection());
            if (newDir == TRANSFORM_DIR_UNKNOWN)
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
            else if (cachedFile->lut1D)
            {
                cachedFile->lut1D->setInterpolation(fileTransform.getInterpolation());
            }

            if (newDir == TRANSFORM_DIR_FORWARD)
            {
                if (cachedFile->lut1D)
                {
                    CreateLut1DOp(ops, cachedFile->lut1D, newDir);
                }

                if (cachedFile->lut3D)
                {
                    CreateLut3DOp(ops, cachedFile->lut3D, newDir);
                }
            }
            else if (newDir == TRANSFORM_DIR_INVERSE)
            {
                if (cachedFile->lut3D)
                {
                    CreateLut3DOp(ops, cachedFile->lut3D, newDir);
                }

                if (cachedFile->lut1D)
                {
                    CreateLut1DOp(ops, cachedFile->lut1D, newDir);
                }
            }
        }
    }

    FileFormat * CreateFileFormatTruelight()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

OCIO_ADD_TEST(FileFormatTruelight, shaper_and_lut_3d)
{
    // This lowers the red channel by 0.5, other channels are unaffected.
    const char * luttext = "# Truelight Cube v2.0\n"
       "# iDims 3\n"
       "# oDims 3\n"
       "# width 3 3 3\n"
       "# lutLength 5\n"
       "# InputLUT\n"
       " 0.000000 0.000000 0.000000\n"
       " 0.500000 0.500000 0.500000\n"
       " 1.000000 1.000000 1.000000\n"
       " 1.500000 1.500000 1.500000\n"
       " 2.000000 2.000000 2.000000\n"
       "\n"
       "# Cube\n"
       " 0.000000 0.000000 0.000000\n"
       " 0.250000 0.000000 0.000000\n"
       " 0.500000 0.000000 0.000000\n"
       " 0.000000 0.500000 0.000000\n"
       " 0.250000 0.500000 0.000000\n"
       " 0.500000 0.500000 0.000000\n"
       " 0.000000 1.000000 0.000000\n"
       " 0.250000 1.000000 0.000000\n"
       " 0.500000 1.000000 0.000000\n"
       " 0.000000 0.000000 0.500000\n"
       " 0.250000 0.000000 0.500000\n"
       " 0.500000 0.000000 0.500000\n"
       " 0.000000 0.500000 0.500000\n"
       " 0.250000 0.500000 0.500000\n"
       " 0.500000 0.500000 0.500000\n"
       " 0.000000 1.000000 0.500000\n"
       " 0.250000 1.000000 0.500000\n"
       " 0.500000 1.000000 0.500000\n"
       " 0.000000 0.000000 1.000000\n"
       " 0.250000 0.000000 1.000000\n"
       " 0.500000 0.000000 1.000000\n"
       " 0.000000 0.500000 1.000000\n"
       " 0.250000 0.500000 1.000000\n"
       " 0.500000 0.500000 1.000000\n"
       " 0.000000 1.000000 1.000000\n"
       " 0.250000 1.000000 1.000000\n"
       " 0.500000 1.000000 1.000000\n"
       "\n"
       "# end\n"
       "\n"
       "# Truelight profile\n"
       "title{madeup on some display}\n"
       "print{someprint}\n"
       "display{some}\n"
       "cubeFile{madeup.cube}\n"
       "\n"
       " # This last line confirms 'end' tag is obeyed\n"
       " 1.23456 1.23456 1.23456\n";

    std::istringstream lutIStream;
    lutIStream.str(luttext);

    // Read file
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile;
    OCIO_CHECK_NO_THROW(cachedFile = tester.read(lutIStream, emptyString));
    OCIO::LocalCachedFileRcPtr lut = OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(cachedFile);

    OCIO_REQUIRE_ASSERT(lut);

    OCIO_REQUIRE_ASSERT(lut->lut1D);
    OCIO_REQUIRE_ASSERT(lut->lut3D);
    OCIO_CHECK_EQUAL(lut->lut1D->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_EQUAL(lut->lut3D->getFileOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    float data[4*3] = { 0.1f, 0.2f, 0.3f, 0.0f,
                        1.0f, 0.5f, 0.123456f, 0.0f,
                       -1.0f, 1.5f, 0.5f, 0.0f };

    float result[4*3] = { 0.05f, 0.2f, 0.3f, 0.0f,
                          0.50f, 0.5f, 0.123456f, 0.0f,
                          0.0f, 1.0f, 0.5f, 0.0f };

    OCIO::OpRcPtrVec ops;
    if(lut->lut1D)
    {
        CreateLut1DOp(ops, lut->lut1D, OCIO::TRANSFORM_DIR_FORWARD);
    }
    if(lut->lut3D)
    {
        CreateLut3DOp(ops, lut->lut3D, OCIO::TRANSFORM_DIR_FORWARD);
    }
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));


    // Apply the result
    for(OCIO::OpRcPtrVec::size_type i = 0, size = ops.size(); i < size; ++i)
    {
        ops[i]->apply(data, 3);
    }

    for(int i=0; i<4*3; ++i)
    {
        OCIO_CHECK_CLOSE( data[i], result[i], 1.0e-6 );
    }
}

OCIO_ADD_TEST(FileFormatTruelight, shaper)
{
    const char * luttext = "# Truelight Cube v2.0\n"
       "# lutLength 11\n"
       "# iDims 3\n"
       "\n"
       "\n"
       "# InputLUT\n"
       " 0.000 0.000 -0.000\n"
       " 0.200 0.010 -0.100\n"
       " 0.400 0.040 -0.200\n"
       " 0.600 0.090 -0.300\n"
       " 0.800 0.160 -0.400\n"
       " 1.000 0.250 -0.500\n"
       " 1.200 0.360 -0.600\n"
       " 1.400 0.490 -0.700\n"
       " 1.600 0.640 -0.800\n"
       " 1.800 0.820 -0.900\n"
       " 2.000 1.000 -1.000\n"
       "\n\n\n"
       "# end\n";

    std::istringstream lutIStream;
    lutIStream.str(luttext);

    // Read file
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile;
    OCIO_CHECK_NO_THROW(cachedFile = tester.read(lutIStream, emptyString));
    
    OCIO::LocalCachedFileRcPtr lut = OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(cachedFile);

    OCIO_CHECK_ASSERT(lut->lut1D);
    OCIO_CHECK_ASSERT(!lut->lut3D);

    float data[4*3] = { 0.1f, 0.2f, 0.3f, 0.0f,
                        1.0f, 0.5f, 0.123456f, 0.0f,
                       -1.0f, 1.5f, 0.5f, 0.0f };

    float result[4*3] = { 0.2f, 0.04f, -0.3f, 0.0f,
                          2.0f, 0.25f, -0.123456f, 0.0f,
                          0.0f, 1.0f, -0.5f, 0.0f };

    OCIO::OpRcPtrVec ops;
    if(lut->lut1D)
    {
        CreateLut1DOp(ops, lut->lut1D, OCIO::TRANSFORM_DIR_FORWARD);
    }
    if(lut->lut3D)
    {
        CreateLut3DOp(ops, lut->lut3D, OCIO::TRANSFORM_DIR_FORWARD);
    }
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));

    // Apply the result
    for(OCIO::OpRcPtrVec::size_type i = 0, size = ops.size(); i < size; ++i)
    {
        ops[i]->apply(data, 3);
    }

    for(int i=0; i<4*3; ++i)
    {
        OCIO_CHECK_CLOSE( data[i], result[i], 1.0e-6 );
    }
}


OCIO_ADD_TEST(FileFormatTruelight, lut_3d)
{
    // This lowers the red channel by 0.5, other channels are unaffected.
    const char * luttext = "# Truelight Cube v2.0\n"
       "# iDims 3\n"
       "# oDims 3\n"
       "# width 3 3 3\n"
       "\n\n\n"
       "# Cube\n"
       " 0.000000 0.000000 0.000000\n"
       " 0.250000 0.000000 0.000000\n"
       " 0.500000 0.000000 0.000000\n"
       " 0.000000 0.500000 0.000000\n"
       " 0.250000 0.500000 0.000000\n"
       " 0.500000 0.500000 0.000000\n"
       " 0.000000 1.000000 0.000000\n"
       " 0.250000 1.000000 0.000000\n"
       " 0.500000 1.000000 0.000000\n"
       " 0.000000 0.000000 0.500000\n"
       " 0.250000 0.000000 0.500000\n"
       " 0.500000 0.000000 0.500000\n"
       " 0.000000 0.500000 0.500000\n"
       " 0.250000 0.500000 0.500000\n"
       " 0.500000 0.500000 0.500000\n"
       " 0.000000 1.000000 0.500000\n"
       " 0.250000 1.000000 0.500000\n"
       " 0.500000 1.000000 0.500000\n"
       " 0.000000 0.000000 1.000000\n"
       " 0.250000 0.000000 1.000000\n"
       " 0.500000 0.000000 1.000000\n"
       " 0.000000 0.500000 1.000000\n"
       " 0.250000 0.500000 1.000000\n"
       " 0.500000 0.500000 1.000000\n"
       " 0.000000 1.000000 1.000000\n"
       " 0.250000 1.000000 1.000000\n"
       " 0.500000 1.000000 1.000000\n"
       "\n"
       "# end\n";

    std::istringstream lutIStream;
    lutIStream.str(luttext);

    // Read file
    std::string emptyString;
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile;
    OCIO_CHECK_NO_THROW(cachedFile = tester.read(lutIStream, emptyString));
    OCIO::LocalCachedFileRcPtr lut = OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(cachedFile);

    OCIO_CHECK_ASSERT(!lut->lut1D);
    OCIO_CHECK_ASSERT(lut->lut3D);

    float data[4*3] = { 0.1f, 0.2f, 0.3f, 0.0f,
                        1.0f, 0.5f, 0.123456f, 0.0f,
                       -1.0f, 1.5f, 0.5f, 0.0f };

    float result[4*3] = { 0.05f, 0.2f, 0.3f, 0.0f,
                          0.50f, 0.5f, 0.123456f, 0.0f,
                          0.0f, 1.0f, 0.5f, 0.0f };

    OCIO::OpRcPtrVec ops;
    if(lut->lut1D)
    {
        CreateLut1DOp(ops, lut->lut1D, OCIO::TRANSFORM_DIR_FORWARD);
    }
    if(lut->lut3D)
    {
        CreateLut3DOp(ops, lut->lut3D, OCIO::TRANSFORM_DIR_FORWARD);
    }
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));

    // Apply the result
    for(OCIO::OpRcPtrVec::size_type i = 0, size = ops.size(); i < size; ++i)
    {
        ops[i]->apply(data, 3);
    }

    for(int i=0; i<4*3; ++i)
    {
        OCIO_CHECK_CLOSE( data[i], result[i], 1.0e-6 );
    }
}

#endif // OCIO_UNIT_TEST

// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <iterator>

#include <OpenColorIO/OpenColorIO.h>

#include "fileformats/FileFormatUtils.h"
#include "ops/lut1d/Lut1DOp.h"
#include "ops/lut3d/Lut3DOp.h"
#include "ParseUtils.h"
#include "transforms/FileTransform.h"
#include "utils/StringUtils.h"


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


namespace OCIO_NAMESPACE
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

    CachedFileRcPtr read(std::istream & istream,
                         const std::string & fileName,
                         Interpolation interp) const override;

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

CachedFileRcPtr LocalFileFormat::read(std::istream & istream,
                                      const std::string & /* fileName unused */,
                                      Interpolation interp) const
{
    // this shouldn't happen
    if(!istream)
    {
        throw Exception ("File stream empty when trying to read Truelight .cub LUT");
    }

    // Validate the file type
    std::string line;
    if(!nextline(istream, line) || 
        !StringUtils::StartsWith(StringUtils::Lower(line), "# truelight cube"))
    {
        throw Exception("LUT doesn't seem to be a Truelight .cub LUT.");
    }

    // Parse the file
    std::vector<float> raw1d;
    std::vector<float> raw3d;
    int size3d[] = { 0, 0, 0 };
    int size1d = 0;
    {
        StringUtils::StringVec parts;
        std::vector<float> tmpfloats;

        bool in1d = false;
        bool in3d = false;

        while(nextline(istream, line))
        {
            // Strip, lowercase, and split the line
            parts = StringUtils::SplitByWhiteSpaces(StringUtils::Lower(StringUtils::Trim(line)));

            if(parts.empty()) continue;

            // Parse header metadata (which starts with #)
            if(StringUtils::StartsWith(parts[0],"#"))
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
        if (Lut1DOpData::IsValidInterpolation(interp))
        {
            cachedFile->lut1D->setInterpolation(interp);
        }
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
        if (Lut3DOpData::IsValidInterpolation(interp))
        {
            cachedFile->lut3D->setInterpolation(interp);
        }
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
                                baker.getTargetSpace())->getOptimizedCPUProcessor(OPTIMIZATION_LOSSLESS);
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
    if(!cachedFile || (!cachedFile->lut1D && !cachedFile->lut3D))
    {
        std::ostringstream os;
        os << "Cannot build Truelight .cub Op. Invalid cache type.";
        throw Exception(os.str().c_str());
    }

    const auto newDir = CombineTransformDirections(dir, fileTransform.getDirection());

    const auto fileInterp = fileTransform.getInterpolation();

    bool fileInterpUsed = false;
    auto lut1D = HandleLUT1D(cachedFile->lut1D, fileInterp, fileInterpUsed);
    auto lut3D = HandleLUT3D(cachedFile->lut3D, fileInterp, fileInterpUsed);

    if (!fileInterpUsed)
    {
        LogWarningInterpolationNotUsed(fileInterp, fileTransform);
    }

    switch (newDir)
    {
    case TRANSFORM_DIR_FORWARD:
        if (lut1D)
        {
            CreateLut1DOp(ops, lut1D, newDir);
        }

        if (lut3D)
        {
            CreateLut3DOp(ops, lut3D, newDir);
        }
        break;
    case TRANSFORM_DIR_INVERSE:
        if (lut3D)
        {
            CreateLut3DOp(ops, lut3D, newDir);
        }

        if (lut1D)
        {
            CreateLut1DOp(ops, lut1D, newDir);
        }
        break;
    }
}
}

FileFormat * CreateFileFormatTruelight()
{
    return new LocalFileFormat();
}

} // namespace OCIO_NAMESPACE


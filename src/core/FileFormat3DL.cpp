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
#include "Lut3DOp.h"
#include "MathUtils.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

// Discreet's Flame LUT Format
// Use a loose interpretation of the format to allow other 3d luts that look
// similar, but dont strictly adhere to the real definition.

// If line starts with text or # skip it
// If line is a bunch of ints (more than 3) , it's the 1d shaper LUT

// All remaining lines of size 3 int are data
// cube size is determined from num entries
// The bit depth of the shaper LUT and the 3d LUT need not be the same.
/*

Example 1, FLAME
# Comment here
0 64 128 192 256 320 384 448 512 576 640 704 768 832 896 960 1023

0 0 0
0 0 100
0 0 200


Example 2, LUSTRE
#Tokens required by applications - do not edit
3DMESH
Mesh 4 12
0 64 128 192 256 320 384 448 512 576 640 704 768 832 896 960 1023



0 17 17
0 0 88
0 0 157
9 101 197
0 118 308
...

4092 4094 4094

#Tokens required by applications - do not edit

LUT8
gamma 1.0

In this example, the 3D LUT has an input bit depth of 4 bits and an output 
bit depth of 12 bits. You use the input value to calculate the RGB triplet 
to be 17*17*17 (where 17=(2 to the power of 4)+1, and 4 is the input bit 
depth). The first triplet is the output value at (0,0,0);(0,0,1);...;
(0,0,16) r,g,b coordinates; the second triplet is the output value at 
(0,1,0);(0,1,1);...;(0,1,16) r,g,b coordinates; and so on. You use the output 
bit depth to set the output bit depth range (12 bits or 0-4095).
NoteLustre supports an input and output depth of 16 bits for 3D LUTs; however, 
in the processing pipeline, the BLACK_LEVEL to WHITE_LEVEL range is only 14 
bits. This means that even if the 3D LUT is 16 bits, it is normalized to 
fit the BLACK_LEVEL to WHITE_LEVEL range of Lustre.
In Lustre, 3D LUT files can contain grids of 17 cubed, 33 cubed, and 65 cubed; 
however, Lustre converts 17 cubed and 65 cubed grids to 33 cubed for internal 
processing on the output (for rendering and calibration), but not on the input 
3D LUT.
*/


OCIO_NAMESPACE_ENTER
{
    ////////////////////////////////////////////////////////////////
    
    namespace
    {
        class LocalCachedFile : public CachedFile
        {
        public:
            LocalCachedFile () : 
                has1D(false),
                has3D(false)
            {
                lut1D = Lut1D::Create();
                lut3D = Lut3D::Create();
            };
            ~LocalCachedFile() {};
            
            bool has1D;
            bool has3D;
            Lut1DRcPtr lut1D;
            Lut3DRcPtr lut3D;
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
            
            virtual void Write(const Baker & baker,
                               const std::string & formatName,
                               std::ostream & ostream) const;
            
            virtual void BuildFileOps(OpRcPtrVec & ops,
                                      const Config& config,
                                      const ConstContextRcPtr & context,
                                      CachedFileRcPtr untypedCachedFile,
                                      const FileTransform& fileTransform,
                                      TransformDirection dir) const;
        };
        
        
        
        
        
        
        // We use the maximum value found in the LUT to infer
        // the bit depth.  While this is fugly. We dont believe
        // there is a better way, looking at the file, to
        // determine this.
        //
        // Note:  We allow for 2x overshoot in the luts.
        // As we dont allow for odd bit depths, this isnt a big deal.
        // So sizes from 1/2 max - 2x max are valid 
        //
        // FILE      EXPECTED MAX    CORRECTLY DECODED IF MAX IN THIS RANGE 
        // 8-bit     255             [0, 511]      
        // 10-bit    1023            [512, 2047]
        // 12-bit    4095            [2048, 8191]
        // 14-bit    16383           [8192, 32767]
        // 16-bit    65535           [32768, 131071+]
        
        int GetLikelyLutBitDepth(int testval)
        {
            const int MIN_BIT_DEPTH = 8;
            const int MAX_BIT_DEPTH = 16;
            
            if(testval < 0) return -1;
            
            // Only test even bit depths
            for(int bitDepth = MIN_BIT_DEPTH;
                bitDepth <= MAX_BIT_DEPTH; bitDepth+=2)
            {
                int maxcode = static_cast<int>(pow(2.0,bitDepth));
                int adjustedMax = maxcode * 2 - 1;
                if(testval<=adjustedMax) return bitDepth;
            }
            
            return MAX_BIT_DEPTH;
        }
        
        int GetMaxValueFromIntegerBitDepth(int bitDepth)
        {
            return static_cast<int>( pow(2.0, bitDepth) ) - 1;
        }
        
        int GetClampedIntFromNormFloat(float val, float scale)
        {
            val = std::min(std::max(0.0f, val), 1.0f) * scale;
            return static_cast<int>(roundf(val));
        }
        
        void LocalFileFormat::GetFormatInfo(FormatInfoVec & formatInfoVec) const
        {
            FormatInfo info;
            info.name = "flame";
            info.extension = "3dl";
            info.capabilities = (FORMAT_CAPABILITY_READ | FORMAT_CAPABILITY_WRITE);
            formatInfoVec.push_back(info);
            
            FormatInfo info2 = info;
            info2.name = "lustre";
            formatInfoVec.push_back(info2);
        }
        
        // Try and load the format
        // Raise an exception if it can't be loaded.
        
        CachedFileRcPtr LocalFileFormat::Read(
            std::istream & istream,
            const std::string & /* fileName unused */) const
        {
            std::vector<int> rawshaper;
            std::vector<int> raw3d;
            
            int lut3dmax = 0;

            // Parse the file 3d LUT data to an int array
            {
                const int MAX_LINE_SIZE = 4096;
                char lineBuffer[MAX_LINE_SIZE];
                
                std::vector<std::string> lineParts;
                std::vector<int> tmpData;
                
                int lineNumber = 0;

                while(istream.good())
                {
                    istream.getline(lineBuffer, MAX_LINE_SIZE);
                    ++lineNumber;

                    // Strip and split the line
                    pystring::split(pystring::strip(lineBuffer), lineParts);
                    
                    if(lineParts.empty()) continue;
                    if (lineParts.size() > 0)
                    {
                        if (pystring::startswith(lineParts[0], "#"))
                        {
                            continue;
                        }
                        if (pystring::startswith(lineParts[0], "<"))
                        {
                            // format error: reject files that could be
                            // formated as xml
                            std::ostringstream os;
                            os << "Error parsing .3dl file. ";
                            os << "Not expecting a line starting with \"<\".";
                            os << "Line (" << lineNumber << "): '";
                            os << lineBuffer << "'.";
                            throw Exception(os.str().c_str());
                        }
                    }
                    
                    // If we havent found a list of ints, continue
                    if (!StringVecToIntVec(tmpData, lineParts))
                    {
                        // Some keywords are valid (3DMESH, mesh, gamma, LUT*)
                        // but others could be format error.
                        // To preserve v1 behavior, don't reject them.
                        continue;
                    }
                    
                    // If we've found more than 3 ints, and dont have
                    // a shaper LUT yet, we've got it!
                    if(tmpData.size()>3)
                    {
                        if (rawshaper.empty())
                        {
                            for (unsigned int i = 0; i < tmpData.size(); ++i)
                            {
                                rawshaper.push_back(tmpData[i]);
                            }
                        }
                        else
                        {
                            // format error, more than 1 shaper LUT
                            std::ostringstream os;
                            os << "Error parsing .3dl file. ";
                            os << "Appears to contain more than 1 shaper LUT.";
                            os << "Line (" << lineNumber << "): '";
                            os << lineBuffer << "'.";
                            throw Exception(os.str().c_str());
                        }
                    }
                    // If we've found 3 ints, add it to our 3dlut.
                    else if(tmpData.size() == 3)
                    {
                        raw3d.push_back(tmpData[0]);
                        raw3d.push_back(tmpData[1]);
                        raw3d.push_back(tmpData[2]);
                        // Find the maximum shaper LUT value to infer bit-depth
                        lut3dmax = std::max(lut3dmax, tmpData[0]);
                        lut3dmax = std::max(lut3dmax, tmpData[1]);
                        lut3dmax = std::max(lut3dmax, tmpData[2]);
                    }
                    else
                    {
                        // format error, line with 1 or 2 int
                        std::ostringstream os;
                        os << "Error parsing .3dl file. ";
                        os << "Invalid line with less than 3 values.";
                        os << "Line (" << lineNumber << "): '";
                        os << lineBuffer << "'.";
                        throw Exception(os.str().c_str());
                    }
                }
            }
            
            if(raw3d.empty() && rawshaper.empty())
            {
                std::ostringstream os;
                os << "Error parsing .3dl file. ";
                os << "Does not appear to contain a valid shaper LUT or a 3D LUT.";
                throw Exception(os.str().c_str());
            }
            
            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());
            
            // If all we're doing to parse the format is to read in sets of 3 numbers,
            // it's possible that other formats will accidentally be able to be read
            // mistakenly as .3dl files.  We can exclude a huge segement of these mis-reads
            // by screening for files that use float represenations.  I.e., if the MAX
            // value of the LUT is a small number (such as <128.0) it's likely not an integer
            // format, and thus not a likely 3DL file.
            
            const int FORMAT3DL_CODEVALUE_LOWEST_PLAUSIBLE_MAXINT = 128;
            
            // Interpret the shaper LUT
            if(!rawshaper.empty())
            {
                cachedFile->has1D = true;
                
                // Find the maximum shaper LUT value to infer bit-depth
                int shapermax = 0;
                for(unsigned int i=0; i<rawshaper.size(); ++i)
                {
                    shapermax = std::max(shapermax, rawshaper[i]);
                }
                
                if(shapermax<FORMAT3DL_CODEVALUE_LOWEST_PLAUSIBLE_MAXINT)
                {
                    std::ostringstream os;
                    os << "Error parsing .3dl file. ";
                    os << "The maximum shaper LUT value, " << shapermax;
                    os << ", is unreasonably low. This LUT is probably not a .3dl ";
                    os << "file, but instead a related format that shares a similar ";
                    os << "structure.";
                    
                    throw Exception(os.str().c_str());
                }
                
                int shaperbitdepth = GetLikelyLutBitDepth(shapermax);
                if(shaperbitdepth<0)
                {
                    std::ostringstream os;
                    os << "Error parsing .3dl file. ";
                    os << "The maximum shaper LUT value, " << shapermax;
                    os << ", does not correspond to any likely bit depth. ";
                    os << "Please confirm source file is valid.";
                    throw Exception(os.str().c_str());
                }
                
                int bitdepthmax = GetMaxValueFromIntegerBitDepth(shaperbitdepth);
                float scale = 1.0f / static_cast<float>(bitdepthmax);
                
                for(int channel=0; channel<3; ++channel)
                {
                    cachedFile->lut1D->luts[channel].resize(rawshaper.size());
                    
                    for(unsigned int i=0; i<rawshaper.size(); ++i)
                    {
                        cachedFile->lut1D->luts[channel][i] = static_cast<float>(rawshaper[i])*scale;
                    }
                }
                
                // The error threshold will be 2 code values. This will allow
                // shaper luts which use different int conversions (round vs. floor)
                // to both be optimized.
                // Required: Abs Tolerance
                
                const int FORMAT3DL_SHAPER_CODEVALUE_TOLERANCE = 2;
                cachedFile->lut1D->maxerror = FORMAT3DL_SHAPER_CODEVALUE_TOLERANCE*scale;
                cachedFile->lut1D->errortype = ERROR_ABSOLUTE;
            }
            
            
            
            // Interpret the parsed data.
            if(!raw3d.empty())
            {
                cachedFile->has3D = true;
                
                // lut3dmax has been stored while reading values
                if(lut3dmax<FORMAT3DL_CODEVALUE_LOWEST_PLAUSIBLE_MAXINT)
                {
                    std::ostringstream os;
                    os << "Error parsing .3dl file.";
                    os << "The maximum 3d LUT value, " << lut3dmax;
                    os << ", is unreasonably low. This LUT is probably not a .3dl ";
                    os << "file, but instead a related format that shares a similar ";
                    os << "structure.";
                    
                    throw Exception(os.str().c_str());
                }
                
                int lut3dbitdepth = GetLikelyLutBitDepth(lut3dmax);
                if(lut3dbitdepth<0)
                {
                    std::ostringstream os;
                    os << "Error parsing .3dl file.";
                    os << "The maximum 3d LUT value, " << lut3dmax;
                    os << ", does not correspond to any likely bit depth. ";
                    os << "Please confirm source file is valid.";
                    throw Exception(os.str().c_str());
                }
                
                int bitdepthmax = GetMaxValueFromIntegerBitDepth(lut3dbitdepth);
                float scale = 1.0f / static_cast<float>(bitdepthmax);
                
                // Interpret the int array as a 3dlut
                int lutEdgeLen = Get3DLutEdgeLenFromNumPixels((int)raw3d.size()/3);
                
                // Reformat 3D data
                cachedFile->lut3D->size[0] = lutEdgeLen;
                cachedFile->lut3D->size[1] = lutEdgeLen;
                cachedFile->lut3D->size[2] = lutEdgeLen;
                cachedFile->lut3D->lut.reserve(lutEdgeLen * lutEdgeLen * lutEdgeLen * 3);
                
                // lut3D->lut is red fastest (loop on B, G then R to push values)
                for (int bIndex = 0; bIndex<lutEdgeLen; ++bIndex)
                {
                    for (int gIndex = 0; gIndex<lutEdgeLen; ++gIndex)
                    {
                        for (int rIndex = 0; rIndex<lutEdgeLen; ++rIndex)
                        {
                            // In file, LUT is blue fastest
                            int i = GetLut3DIndex_BlueFast(rIndex, gIndex, bIndex,
                                lutEdgeLen, lutEdgeLen, lutEdgeLen);

                            cachedFile->lut3D->lut.push_back(static_cast<float>(raw3d[i + 0]) * scale);
                            cachedFile->lut3D->lut.push_back(static_cast<float>(raw3d[i + 1]) * scale);
                            cachedFile->lut3D->lut.push_back(static_cast<float>(raw3d[i + 2]) * scale);
                        }
                    }
                }

            }
            
            return cachedFile;
        }
        
        // 65 -> 6
        // 33 -> 5
        // 17 -> 4
        
        int CubeDimensionLenToLustreBitDepth(int size)
        {
            float logval = logf(static_cast<float>(size-1)) / logf(2.0);
            return static_cast<int>(logval);
        }
        
        void LocalFileFormat::Write(const Baker & baker,
                                    const std::string & formatName,
                                    std::ostream & ostream) const
        {
            int DEFAULT_CUBE_SIZE = 0;
            int SHAPER_BIT_DEPTH = 10;
            int CUBE_BIT_DEPTH = 12;
            
            if(formatName == "lustre")
            {
                DEFAULT_CUBE_SIZE = 33;
            }
            else if(formatName == "flame")
            {
                DEFAULT_CUBE_SIZE = 17;
            }
            else
            {
                std::ostringstream os;
                os << "Unknown 3dl format name, '";
                os << formatName << "'.";
                throw Exception(os.str().c_str());
            }
            
            ConstConfigRcPtr config = baker.getConfig();
            
            int cubeSize = baker.getCubeSize();
            if(cubeSize==-1) cubeSize = DEFAULT_CUBE_SIZE;
            cubeSize = std::max(2, cubeSize); // smallest cube is 2x2x2
            
            int shaperSize = baker.getShaperSize();
            if(shaperSize==-1) shaperSize = cubeSize;
            
            std::vector<float> cubeData;
            cubeData.resize(cubeSize*cubeSize*cubeSize*3);
            GenerateIdentityLut3D(&cubeData[0], cubeSize, 3, LUT3DORDER_FAST_BLUE);
            PackedImageDesc cubeImg(&cubeData[0], cubeSize*cubeSize*cubeSize, 1, 3);
            
            // Apply our conversion from the input space to the output space.
            ConstProcessorRcPtr inputToTarget;
            std::string looks = baker.getLooks();
            if (!looks.empty())
            {
                LookTransformRcPtr transform = LookTransform::Create();
                transform->setLooks(looks.c_str());
                transform->setSrc(baker.getInputSpace());
                transform->setDst(baker.getTargetSpace());
                inputToTarget = config->getProcessor(transform,
                    TRANSFORM_DIR_FORWARD);
            }
            else
            {
              inputToTarget = config->getProcessor(baker.getInputSpace(),
                  baker.getTargetSpace());
            }
            inputToTarget->apply(cubeImg);
            
            // Write out the file.
            // For for maximum compatibility with other apps, we will
            // not utilize the shaper or output any metadata
            
            if(formatName == "lustre")
            {
                int meshInputBitDepth = CubeDimensionLenToLustreBitDepth(cubeSize);
                ostream << "3DMESH\n";
                ostream << "Mesh " << meshInputBitDepth << " " << CUBE_BIT_DEPTH << "\n";
            }
            
            std::vector<float> shaperData(shaperSize);
            GenerateIdentityLut1D(&shaperData[0], shaperSize, 1);
            
            float shaperScale = static_cast<float>(
                GetMaxValueFromIntegerBitDepth(SHAPER_BIT_DEPTH));
            
            for(unsigned int i=0; i<shaperData.size(); ++i)
            {
                if(i != 0) ostream << " ";
                int val = GetClampedIntFromNormFloat(shaperData[i], shaperScale);
                ostream << val;
            }
            ostream << "\n";
            
            // Write out the 3D Cube
            float cubeScale = static_cast<float>(
                GetMaxValueFromIntegerBitDepth(CUBE_BIT_DEPTH));
            if(cubeSize < 2)
            {
                throw Exception("Internal cube size exception.");
            }
            for(int i=0; i<cubeSize*cubeSize*cubeSize; ++i)
            {
                int r = GetClampedIntFromNormFloat(cubeData[3*i+0], cubeScale);
                int g = GetClampedIntFromNormFloat(cubeData[3*i+1], cubeScale);
                int b = GetClampedIntFromNormFloat(cubeData[3*i+2], cubeScale);
                ostream << r << " " << g << " " << b << "\n";
            }
            ostream << "\n";
            
            if(formatName == "lustre")
            {
                ostream << "LUT8\n";
                ostream << "gamma 1.0\n";
            }
        }
        
        void
        LocalFileFormat::BuildFileOps(OpRcPtrVec & ops,
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
                os << "Cannot build .3dl Op. Invalid cache type.";
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
            
            // TODO: INTERP_LINEAR should not be hard-coded.
            // Instead query 'highest' interpolation?
            // (right now, it's linear). If cubic is added, consider
            // using it
            
            if(newDir == TRANSFORM_DIR_FORWARD)
            {
                if(cachedFile->has1D)
                {
                    CreateLut1DOp(ops, cachedFile->lut1D,
                                  INTERP_LINEAR, newDir);
                }
                if(cachedFile->has3D)
                {
                    CreateLut3DOp(ops, cachedFile->lut3D,
                                  fileTransform.getInterpolation(), newDir);
                }
            }
            else if(newDir == TRANSFORM_DIR_INVERSE)
            {
                if(cachedFile->has3D)
                {
                    CreateLut3DOp(ops, cachedFile->lut3D,
                                  fileTransform.getInterpolation(), newDir);
                }
                if(cachedFile->has1D)
                {
                    CreateLut1DOp(ops, cachedFile->lut1D,
                                  INTERP_LINEAR, newDir);
                }
            }
        }
    }
    
    FileFormat * CreateFileFormat3DL()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"
#include <fstream>

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


OIIO_ADD_TEST(FileFormat3DL, FormatInfo)
{
    OCIO::FormatInfoVec formatInfoVec;
    OCIO::LocalFileFormat tester;
    tester.GetFormatInfo(formatInfoVec);

    OIIO_CHECK_EQUAL(2, formatInfoVec.size());
    OIIO_CHECK_EQUAL("flame", formatInfoVec[0].name);
    OIIO_CHECK_EQUAL("lustre", formatInfoVec[1].name);
    OIIO_CHECK_EQUAL("3dl", formatInfoVec[0].extension);
    OIIO_CHECK_EQUAL("3dl", formatInfoVec[1].extension);
    OIIO_CHECK_EQUAL((OCIO::FORMAT_CAPABILITY_READ
        | OCIO::FORMAT_CAPABILITY_WRITE), formatInfoVec[0].capabilities);
    OIIO_CHECK_EQUAL((OCIO::FORMAT_CAPABILITY_READ
        | OCIO::FORMAT_CAPABILITY_WRITE), formatInfoVec[1].capabilities);
}

OIIO_ADD_TEST(FileFormat3DL, Bake)
{

    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("lnf");
        cs->setFamily("lnf");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("target");
        cs->setFamily("target");
        OCIO::CDLTransformRcPtr transform1 = OCIO::CDLTransform::Create();
        float rgb[3] = { 0.0f, 0.1f, 0.2f };
        transform1->setOffset(rgb);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
        config->addColorSpace(cs);
    }

    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->setMetadata("MetaData not written");
    baker->setFormat("flame");
    baker->setInputSpace("lnf");
    baker->setTargetSpace("target");
    baker->setShaperSize(10);
    baker->setCubeSize(2);
    std::ostringstream outputFlame;
    baker->bake(outputFlame);

    std::vector<std::string> osvecFlame;
    OCIO::pystring::splitlines(outputFlame.str(), osvecFlame);

    std::ostringstream outputLustre;
    baker->setFormat("lustre");
    baker->bake(outputLustre);
    std::vector<std::string> osvecLustre;
    OCIO::pystring::splitlines(outputLustre.str(), osvecLustre);

    std::ostringstream bout;
    bout << "3DMESH" << "\n";
    bout << "Mesh 0 12" << "\n";
    bout << "0 114 227 341 455 568 682 796 909 1023" << "\n";
    bout << "0 410 819" << "\n";
    bout << "0 410 4095" << "\n";
    bout << "0 4095 819" << "\n";
    bout << "0 4095 4095" << "\n";
    bout << "4095 410 819" << "\n";
    bout << "4095 410 4095" << "\n";
    bout << "4095 4095 819" << "\n";
    bout << "4095 4095 4095" << "\n";
    bout << "" << "\n";
    bout << "LUT8" << "\n";
    bout << "gamma 1.0" << "\n";

    std::vector<std::string> resvec;
    OCIO::pystring::splitlines(bout.str(), resvec);
    OIIO_CHECK_EQUAL(resvec.size(), osvecLustre.size());
    OIIO_CHECK_EQUAL(resvec.size() - 4, osvecFlame.size());

    OIIO_CHECK_EQUAL(resvec[0], osvecLustre[0]);
    OIIO_CHECK_EQUAL(resvec[1], osvecLustre[1]);
    for (unsigned int i = 0; i < osvecFlame.size(); ++i)
    {
        OIIO_CHECK_EQUAL(resvec[i+2], osvecFlame[i]);
        OIIO_CHECK_EQUAL(resvec[i+2], osvecLustre[i+2]);
    }
    size_t last = resvec.size() - 2;
    OIIO_CHECK_EQUAL(resvec[last], osvecLustre[last]);
    OIIO_CHECK_EQUAL(resvec[last+1], osvecLustre[last+1]);


}

// FILE      EXPECTED MAX    CORRECTLY DECODED IF MAX IN THIS RANGE 
// 8-bit     255             [0, 511]      
// 10-bit    1023            [512, 2047]
// 12-bit    4095            [2048, 8191]
// 14-bit    16383           [8192, 32767]
// 16-bit    65535           [32768, 131071]

OIIO_ADD_TEST(FileFormat3DL, GetLikelyLutBitDepth)
{
    OIIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(-1), -1);
    
    OIIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(0), 8);
    OIIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(1), 8);
    OIIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(255), 8);
    OIIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(256), 8);
    OIIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(511), 8);
    
    OIIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(512), 10);
    OIIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(1023), 10);
    OIIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(1024), 10);
    OIIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(2047), 10);
    
    OIIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(2048), 12);
    OIIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(4095), 12);
    OIIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(4096), 12);
    OIIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(8191), 12);
    
    OIIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(16383), 14);
    
    OIIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(65535), 16);
    OIIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(65536), 16);
    OIIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(131071), 16);
    
    OIIO_CHECK_EQUAL(OCIO::GetLikelyLutBitDepth(131072), 16);
}


OIIO_ADD_TEST(FileFormat3DL, TestLoad)
{
    // Discreet 3D LUT file
    const std::string discree3DtLut(ocioTestFilesDir
        + std::string("/discreet-3d-lut.3dl"));

    OCIO::LocalCachedFileRcPtr lutFile;
    OIIO_CHECK_NO_THROW(lutFile = LoadLutFile(discree3DtLut));

    OIIO_CHECK_ASSERT(lutFile->has1D);
    OIIO_CHECK_ASSERT(lutFile->has3D);
    OIIO_CHECK_EQUAL(OCIO::ERROR_ABSOLUTE, lutFile->lut1D->errortype);
    OIIO_CHECK_EQUAL(0.00195503421f, lutFile->lut1D->maxerror);
    OIIO_CHECK_EQUAL(0.0f, lutFile->lut1D->from_min[1]);
    OIIO_CHECK_EQUAL(1.0f, lutFile->lut1D->from_max[1]);
    OIIO_CHECK_EQUAL(17, lutFile->lut1D->luts[0].size());
    OIIO_CHECK_EQUAL(0.0f, lutFile->lut1D->luts[0][0]);
    OIIO_CHECK_EQUAL(0.563049853f, lutFile->lut1D->luts[0][9]);
    OIIO_CHECK_EQUAL(1.0f, lutFile->lut1D->luts[0][16]);
    OIIO_CHECK_EQUAL(17, lutFile->lut3D->size[0]);
    OIIO_CHECK_EQUAL(17, lutFile->lut3D->size[1]);
    OIIO_CHECK_EQUAL(17, lutFile->lut3D->size[2]);
    OIIO_CHECK_EQUAL(17*17*17*3, lutFile->lut3D->lut.size());
    // LUT is R fast, file is B fast ([3..5] is [867..869] in file)
    OIIO_CHECK_EQUAL(0.00854700897f, lutFile->lut3D->lut[3]);
    OIIO_CHECK_EQUAL(0.00244200253f, lutFile->lut3D->lut[4]);
    OIIO_CHECK_EQUAL(0.00708180759f, lutFile->lut3D->lut[5]);
    OIIO_CHECK_EQUAL(0.0f, lutFile->lut3D->lut[4335]);
    OIIO_CHECK_EQUAL(0.0368742384f, lutFile->lut3D->lut[4336]);
    OIIO_CHECK_EQUAL(0.0705738738f, lutFile->lut3D->lut[4337]);

    const std::string discree3DtLutFail(ocioTestFilesDir
        + std::string("/error_truncated_file.3dl"));

    OIIO_CHECK_THROW_WHAT(LoadLutFile(discree3DtLutFail),
                          OCIO::Exception,
                          "Cannot infer 3D LUT size");

}



#endif // OCIO_UNIT_TEST

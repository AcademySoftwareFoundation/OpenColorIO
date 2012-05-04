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

/*
// Discreet's Flame Lut Format
// Use a loose interpretation of the format to allow other 3d luts that look
// similar, but dont strictly adhere to the real definition.

// If line starts with text or # skip it
// If line is a bunch of ints (more than 3) , it's the 1d shaper lut

// All remaining lines of size 3 int are data
// cube size is determined from num entries
// The bit depth of the shaper lut and the 3d lut need not be the same.

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
            
            virtual CachedFileRcPtr Read(std::istream & istream) const;
            
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
        
        
        
        
        
        
        // We use the maximum value found in the lut to infer
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
        
        CachedFileRcPtr LocalFileFormat::Read(std::istream & istream) const
        {
            std::vector<int> rawshaper;
            std::vector<int> raw3d;
            
            // Parse the file 3d lut data to an int array
            {
                const int MAX_LINE_SIZE = 4096;
                char lineBuffer[MAX_LINE_SIZE];
                
                std::vector<std::string> lineParts;
                std::vector<int> tmpData;
                
                while(istream.good())
                {
                    istream.getline(lineBuffer, MAX_LINE_SIZE);
                    
                    // Strip and split the line
                    pystring::split(pystring::strip(lineBuffer), lineParts);
                    
                    if(lineParts.empty()) continue;
                    if((lineParts.size() > 0) && pystring::startswith(lineParts[0],"#")) continue;
                    
                    // If we havent found a list of ints, continue
                    if(!StringVecToIntVec(tmpData, lineParts)) continue;
                    
                    // If we've found more than 3 ints, and dont have
                    // a shaper lut yet, we've got it!
                    if(tmpData.size()>3 && rawshaper.empty())
                    {
                        for(unsigned int i=0; i<tmpData.size(); ++i)
                        {
                            rawshaper.push_back(tmpData[i]);
                        }
                    }
                    
                    // If we've found 3 ints, add it to our 3dlut.
                    if(tmpData.size() == 3)
                    {
                        raw3d.push_back(tmpData[0]);
                        raw3d.push_back(tmpData[1]);
                        raw3d.push_back(tmpData[2]);
                    }
                }
            }
            
            if(raw3d.empty() && rawshaper.empty())
            {
                std::ostringstream os;
                os << "Error parsing .3dl file.";
                os << "Does not appear to contain a valid shaper lut or a 3D lut.";
                throw Exception(os.str().c_str());
            }
            
            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());
            
            // If all we're doing to parse the format is to read in sets of 3 numbers,
            // it's possible that other formats will accidentally be able to be read
            // mistakenly as .3dl files.  We can exclude a huge segement of these mis-reads
            // by screening for files that use float represenations.  I.e., if the MAX
            // value of the lut is a small number (such as <128.0) it's likely not an integer
            // format, and thus not a likely 3DL file.
            
            const int FORMAT3DL_CODEVALUE_LOWEST_PLAUSIBLE_MAXINT = 128;
            
            // Interpret the shaper lut
            if(!rawshaper.empty())
            {
                cachedFile->has1D = true;
                
                // Find the maximum shaper lut value to infer bit-depth
                int shapermax = 0;
                for(unsigned int i=0; i<rawshaper.size(); ++i)
                {
                    shapermax = std::max(shapermax, rawshaper[i]);
                }
                
                if(shapermax<FORMAT3DL_CODEVALUE_LOWEST_PLAUSIBLE_MAXINT)
                {
                    std::ostringstream os;
                    os << "Error parsing .3dl file.";
                    os << "The maximum shaper lut value, " << shapermax;
                    os << ", is unreasonably low. This lut is probably not a .3dl ";
                    os << "file, but instead a related format that shares a similar ";
                    os << "structure.";
                    
                    throw Exception(os.str().c_str());
                }
                
                int shaperbitdepth = GetLikelyLutBitDepth(shapermax);
                if(shaperbitdepth<0)
                {
                    std::ostringstream os;
                    os << "Error parsing .3dl file.";
                    os << "The maximum shaper lut value, " << shapermax;
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
                
                // Find the maximum shaper lut value to infer bit-depth
                int lut3dmax = 0;
                for(unsigned int i=0; i<raw3d.size(); ++i)
                {
                    lut3dmax = std::max(lut3dmax, raw3d[i]);
                }
                
                if(lut3dmax<FORMAT3DL_CODEVALUE_LOWEST_PLAUSIBLE_MAXINT)
                {
                    std::ostringstream os;
                    os << "Error parsing .3dl file.";
                    os << "The maximum 3d lut value, " << lut3dmax;
                    os << ", is unreasonably low. This lut is probably not a .3dl ";
                    os << "file, but instead a related format that shares a similar ";
                    os << "structure.";
                    
                    throw Exception(os.str().c_str());
                }
                
                int lut3dbitdepth = GetLikelyLutBitDepth(lut3dmax);
                if(lut3dbitdepth<0)
                {
                    std::ostringstream os;
                    os << "Error parsing .3dl file.";
                    os << "The maximum 3d lut value, " << lut3dmax;
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
                
                for(int rIndex=0; rIndex<lutEdgeLen; ++rIndex)
                {
                    for(int gIndex=0; gIndex<lutEdgeLen; ++gIndex)
                    {
                        for(int bIndex=0; bIndex<lutEdgeLen; ++bIndex)
                        {
                            int i = GetLut3DIndex_B(rIndex, gIndex, bIndex,
                                                    lutEdgeLen, lutEdgeLen, lutEdgeLen);
                            
                            cachedFile->lut3D->lut.push_back(static_cast<float>(raw3d[i+0]) * scale);
                            cachedFile->lut3D->lut.push_back(static_cast<float>(raw3d[i+1]) * scale);
                            cachedFile->lut3D->lut.push_back(static_cast<float>(raw3d[i+2]) * scale);
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

#endif // OCIO_UNIT_TEST

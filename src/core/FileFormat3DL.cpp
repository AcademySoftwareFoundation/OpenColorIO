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

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <pystring.h>

#include <OpenColorIO/OpenColorIO.h>

#include "FileTransform.h"
#include "Lut1DOp.h"
#include "Lut3DOp.h"
#include "ParseUtils.h"

/*
// Discreet's Flame Lut Format
// Use a loose interpretation of the format to allow other 3d luts that look
// similar, but dont strictly adhere to the real definition.

// If line starts with text or # skip it
// If line is a bunch of ints (more than 3) , it's the 1d shaper lut

// All remaining lines of size 3 int are data
// cube size is determined from num entries
// The bit depth of the shaper lut and the 3d lut need not be the same.

Example 1:
# Comment here
0 64 128 192 256 320 384 448 512 576 640 704 768 832 896 960 1023

0 0 0
0 0 100
0 0 200


Example 2:
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
                lut1D = OCIO_SHARED_PTR<Lut1D>(new Lut1D());
                lut3D = OCIO_SHARED_PTR<Lut3D>(new Lut3D());
            };
            ~LocalCachedFile() {};
            
            bool has1D;
            bool has3D;
            OCIO_SHARED_PTR<Lut1D> lut1D;
            OCIO_SHARED_PTR<Lut3D> lut3D;
        };
        
        typedef OCIO_SHARED_PTR<LocalCachedFile> LocalCachedFileRcPtr;
        
        
        
        class LocalFileFormat : public FileFormat
        {
        public:
            
            ~LocalFileFormat() {};
            
            virtual std::string GetName() const;
            virtual std::string GetExtension () const;
            
            virtual bool Supports(const std::string & feature) const;
            
            virtual CachedFileRcPtr Load (std::istream & istream) const;
            
            virtual bool Write(TransformData & /*data*/, std::ostream & /*ostream*/) const;
            
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
        
        int Get3DLutEdgeLenFromNumEntries(int numEntries)
        {
            float fdim = powf((float) numEntries / 3.0f, 1.0f/3.0f);
            int dim = static_cast<int>(roundf(fdim));
            
            if(dim*dim*dim*3 != numEntries)
            {
                std::ostringstream os;
                os << "Cannot infer 3D Lut size. ";
                os << numEntries << " element(s) does not correspond to a ";
                os << "unform cube edge length. (nearest edge length is ";
                os << dim << ").";
                throw Exception(os.str().c_str());
            }
            
            return dim;
        }
        
        
        std::string LocalFileFormat::GetName() const
        {
            return "flame";
        }
        
        std::string LocalFileFormat::GetExtension() const
        {
            return "3dl";
        }
        
        bool LocalFileFormat::Supports(const std::string & feature) const
        {
            if(feature == "load") return true;
            return false;
        }
        
            // Try and load the format
            // Raise an exception if it can't be loaded.
        
        CachedFileRcPtr LocalFileFormat::Load(std::istream & istream) const
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
                float error = FORMAT3DL_SHAPER_CODEVALUE_TOLERANCE*scale;
                
                cachedFile->lut1D->finalize(error, ERROR_ABSOLUTE);
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
                int lutEdgeLen = Get3DLutEdgeLenFromNumEntries((int)raw3d.size());
                
                
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
                
                cachedFile->lut3D->generateCacheID();
            }
            
            return cachedFile;
        }
        
        bool LocalFileFormat::Write(TransformData & /*data*/, std::ostream & /*ostream*/) const
        {
            return false;
        };
        
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
        
        
        struct AutoRegister
        {
            AutoRegister() { RegisterFileFormat(new LocalFileFormat); }
        };
        static AutoRegister registerIt;
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

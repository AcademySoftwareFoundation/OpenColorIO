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
#include "ParseUtils.h"
#include "pystring/pystring.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

#include <iostream>

/*

// Discreet's Flame Lut Format
// Use a loose interpretation of the format to allow other 3d luts that look
// similar, but dont strictly adhere to the real definition.

// If line starts with text or # skip it
// If line is a bunch of ints (more than 3) , it's the 1d shaper lut
// All remaining lines of size 3 int are data
// cube size is determined from num entrie

# Comment here
0 64 128 192 256 320 384 448 512 576 640 704 768 832 896 960 1023

0 0 0
0 0 100
0 0 200

*/


OCIO_NAMESPACE_ENTER
{
    ////////////////////////////////////////////////////////////////
    
    namespace
    {
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
        // 16-bit    65535           [32768, 131071]
        
        int GetLikelyLutBitDepth(int testval)
        {
            // Only test even bit depths
            for(int bitDepth = 8; bitDepth<=16; bitDepth+=2)
            {
                int maxcode = static_cast<int>(pow(2.0,bitDepth));
                int adjustedMax = maxcode * 2 - 1;
                if(testval<=adjustedMax) return bitDepth;
            }
            
            return -1;
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
        
        class LocalCachedFile : public CachedFile
        {
        public:
            LocalCachedFile()
            {
                //useLut1D = false;
                //lut1d = SharedPtr<Lut1D>(new Lut1D());
                lut3d = SharedPtr<Lut3D>(new Lut3D());
            };
            ~LocalCachedFile() {};
            
            //bool useLut1D;
            //SharedPtr<Lut1D> lut1d;
            SharedPtr<Lut3D> lut3d;
        };
        
        typedef SharedPtr<LocalCachedFile> LocalCachedFileRcPtr;
        
        class LocalFormat : public FileFormat
        {
            virtual std::string GetExtension() const
            {
                return "3dl";
            }
            
            // Try and load the format
            // Raise an exception if it can't be loaded.

            virtual CachedFileRcPtr Load(std::istream & istream) const
            {
                std::vector<int> rawLutData;
                int maxLutValue = 0;
                
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
                        
                        // Ignore all data that isnt 3 ints on a line.
                        // This means we are incorrectly ignoring the header data
                        // and the shaper lut
                        // TODO: Load the .3dl shaper lut
                        
                        if(!StringVecToIntVec(tmpData, lineParts)) continue;
                        if(tmpData.empty()) continue;
                        if(tmpData.size() != 3) continue;
                        
                        rawLutData.push_back(tmpData[0]);
                        rawLutData.push_back(tmpData[1]);
                        rawLutData.push_back(tmpData[2]);
                        
                        maxLutValue = std::max(maxLutValue, tmpData[0]);
                        maxLutValue = std::max(maxLutValue, tmpData[1]);
                        maxLutValue = std::max(maxLutValue, tmpData[2]);
                    }
                }
                
                // Interpret the int array as a 3dlut
                
                if(rawLutData.empty())
                {
                    std::ostringstream os;
                    os << "Error parsing .3dl file. ";
                    os << "The contents do not contain any lut entries.";
                    throw Exception(os.str().c_str());
                }
                
                int lutEdgeLen = Get3DLutEdgeLenFromNumEntries((int)rawLutData.size());
                
                // We use the maximum value found in the lut to infer
                // the bit depth.  While this is ugly. We dont believe there is
                // a better way, looking at the file, to determine this.
                
                int likelyBitDepth = GetLikelyLutBitDepth(maxLutValue);
                if(likelyBitDepth < 0)
                {
                    std::ostringstream os;
                    os << "Error parsing .3dl file.";
                    os << "The maximum lut value, " << maxLutValue;
                    os << ", does not correspond to any likely bit depth. ";
                    os << "Please confirm source file is valid.";
                    throw Exception(os.str().c_str());
                }
                
                int bitDepthMaxVal = GetMaxValueFromIntegerBitDepth(likelyBitDepth);
                float scale = 1.0f / static_cast<float>(bitDepthMaxVal);
                
                Lut3DRcPtr lut3d(new Lut3D());
                
                lut3d->size[0] = lutEdgeLen;
                lut3d->size[1] = lutEdgeLen;
                lut3d->size[2] = lutEdgeLen;
                lut3d->lut.resize(lutEdgeLen * lutEdgeLen * lutEdgeLen * 3);
                
                for(int rIndex=0; rIndex<lut3d->size[0]; ++rIndex)
                {
                    for(int gIndex=0; gIndex<lut3d->size[1]; ++gIndex)
                    {
                        for(int bIndex=0; bIndex<lut3d->size[2]; ++bIndex)
                        {
                            int autoDeskLutIndex = GetAutodeskLut3DArrayOffset(rIndex, gIndex, bIndex,
                                                                   lut3d->size[0], lut3d->size[1], lut3d->size[2]);
                            int glLutIndex = GetGLLut3DArrayOffset(rIndex, gIndex, bIndex,
                                                                   lut3d->size[0], lut3d->size[1], lut3d->size[2]);
                            
                            if(autoDeskLutIndex < 0 || autoDeskLutIndex >= (int)lut3d->lut.size() ||
                               glLutIndex < 0 || glLutIndex >= (int)lut3d->lut.size())
                            {
                                std::ostringstream os;
                                os << "Error parsing .3dl file.";
                                os << "A lut entry is specified (";
                                os << rIndex << " " << gIndex << " " << bIndex;
                                os << " that falls outside of the cube.";
                                throw Exception(os.str().c_str());
                            }
                            
                            lut3d->lut[glLutIndex+0] = (float) rawLutData[autoDeskLutIndex+0] * scale;
                            lut3d->lut[glLutIndex+1] = (float) rawLutData[autoDeskLutIndex+1] * scale;
                            lut3d->lut[glLutIndex+2] = (float) rawLutData[autoDeskLutIndex+2] * scale;
                        }
                    }
                }
                
                lut3d->generateCacheID();
                
                LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());
                cachedFile->lut3d = lut3d;
                return cachedFile;
            }

            virtual void BuildFileOps(OpRcPtrVec & ops,
                                      CachedFileRcPtr untypedCachedFile,
                                      const FileTransform& fileTransform,
                                      TransformDirection dir) const
            {
                LocalCachedFileRcPtr cachedFile = DynamicPtrCast<LocalCachedFile>(untypedCachedFile);
                
                if(!cachedFile) // This should never happen.
                {
                    std::ostringstream os;
                    os << "Cannot build 3dl Op. Invalid cache type.";
                    throw Exception(os.str().c_str());
                }
                
                TransformDirection newDir = CombineTransformDirections(dir,
                    fileTransform.getDirection());
                
                if(newDir == TRANSFORM_DIR_FORWARD)
                {
                    // The 1D Shaper lut, if it exists is super low
                    // resolution. use the best interpolation we can.
                    // (right now, it's linear). If cubic is added, consider
                    // using it
                    /*
                    if(cachedFile->useLut1D)
                    {
                        CreateLut1DOp(ops,
                                      cachedFile->lut1d,
                                      INTERP_LINEAR,
                                      TRANSFORM_DIR_FORWARD);
                    }
                    */
                    CreateLut3DOp(ops,
                                  cachedFile->lut3d,
                                  fileTransform.getInterpolation(),
                                  TRANSFORM_DIR_FORWARD);
                }
                else if(newDir == TRANSFORM_DIR_INVERSE)
                {
                    CreateLut3DOp(ops,
                                  cachedFile->lut3d,
                                  fileTransform.getInterpolation(),
                                  TRANSFORM_DIR_INVERSE);
                    /*
                    if(cachedFile->useLut1D)
                    {
                        CreateLut1DOp(ops,
                                      cachedFile->lut1d,
                                      INTERP_LINEAR,
                                      TRANSFORM_DIR_INVERSE);
                    }
                    */
                }
            }
        };
        
        struct AutoRegister
        {
            AutoRegister() { RegisterFileFormat(new LocalFormat); }
        };
        static AutoRegister registerIt;
    }
}
OCIO_NAMESPACE_EXIT

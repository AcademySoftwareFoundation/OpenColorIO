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

#include <OpenColorSpace/OpenColorSpace.h>

#include "FileTransform.h"
#include "Lut1DOp.h"
#include "Lut3DOp.h"
#include "pystring/pystring.h"

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
// cube size is determined from num entrie

# Comment here
0 64 128 192 256 320 384 448 512 576 640 704 768 832 896 960 1023

0 0 0
0 0 100
0 0 200

*/


OCS_NAMESPACE_ENTER
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
        
        
        
        
        class LocalCachedFile : public CachedFile
        {
        public:
            LocalCachedFile()
            {
                useLut1D = false;
                lut1d = SharedPtr<Lut1D>(new Lut1D());
                lut3d = SharedPtr<Lut3D>(new Lut3D());
            };
            ~LocalCachedFile() {};
            
            bool useLut1D;
            SharedPtr<Lut1D> lut1d;
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
                /*
                std::vector<int> rawLutData;
                int maxLutValue=0;
                
                // Parse the file format
                {
                    int lineSize = 4096;
                    char lineBuffer[lineSize];
                    std::string cleanLine;
                    std::vector<std::string> lineParts;
                    std::vector<int> tmpData;
                    
                    while(infile.good())
                    {
                        infile.getline(lineBuffer, lineSize);
                        
                        // Strip and split the line
                        cleanLine = pystring::strip(lineBuffer);
                        pystring::split(cleanLine, lineParts);
                        
                        if(lineParts.empty()) continue;
                        if((lineParts.size() > 0) && pystring::startswith(lineParts[0],"#")) continue;
                        
                        if(!stringVecToIntVec(tmpData, lineParts)) continue;
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
                
                if(rawLutData.empty())
                {
                    throw ColorException("Error parsing .3dl lut contents. Lut is empty: " + fileName);
                }
                
                // Transcode the lut data
                
                unsigned int lutDimensionSize = static_cast<unsigned int>(powf((rawLutData.size() / 3.0), (1/3.0)) + 0.5);
                unsigned int lutTotalSize = lutDimensionSize*lutDimensionSize*lutDimensionSize*3;
                if(lutDimensionSize < 2)
                {
                    throw ColorException("Error parsing .3dl lut contents. Lut is empty: " + fileName);
                }
                
                
                std::auto_ptr<LookupTable> result(new LookupTable);
                
                result->type = LUT_3D;
                result->size_red = lutDimensionSize;
                result->size_green = lutDimensionSize;
                result->size_blue = lutDimensionSize;
                result->size_alpha = 0;
                result->cube_lut.resize(lutTotalSize);
                
                // We use the maximum value found in the lut to infer
                // the bit depth.  While this is ugly. We dont believe there is
                // a better way, looking at the file, to determine this.
                
                int likelyBitDepth = GetLikelyLutBitDepth(maxLutValue);
                int lutMaxVal = static_cast<int>(pow(2.0,likelyBitDepth))-1;
                float scale = 1.0 / static_cast<float>(lutMaxVal);
                
                for(unsigned int rIndex=0;rIndex<result->size_red; rIndex++)
                {
                    for(unsigned int gIndex=0;gIndex<result->size_green; gIndex++)
                    {
                        for(unsigned int bIndex=0;bIndex<result->size_blue; bIndex++)
                        {
                            int lustreIndex = 3 * (bIndex + result->size_blue * (gIndex + result->size_green * rIndex));
                            
                            
                            int spiIndex = lut3DArrayOffset(rIndex, gIndex, bIndex,
                                                            result->size_red,
                                                            result->size_green,
                                                            result->size_blue);
                            result->cube_lut[spiIndex+0] = rawLutData[lustreIndex+0]*scale;
                            result->cube_lut[spiIndex+1] = rawLutData[lustreIndex+1]*scale;
                            result->cube_lut[spiIndex+2] = rawLutData[lustreIndex+2]*scale;
                            
                        }
                    }
                }
                
                */
                /*
                const int MAX_LINE_SIZE = 4096;
                char lineBuffer[MAX_LINE_SIZE];
                
                Lut3DRcPtr lut3d(new Lut3D());
                
                // Read header information
                
                // TODO: Assert 1st line is SPILUT 1.0
                istream.getline(lineBuffer, MAX_LINE_SIZE);
                // TODO: Assert 2nd line is 3 3
                istream.getline(lineBuffer, MAX_LINE_SIZE);
                
                // Get LUT Size
                // TODO: Error handling
                int rSize, gSize, bSize;
                istream.getline(lineBuffer, MAX_LINE_SIZE);
                sscanf(lineBuffer, "%d %d %d", &rSize, &gSize, &bSize);
                
                lut3d->size[0] = rSize;
                lut3d->size[1] = gSize;
                lut3d->size[2] = bSize;
                lut3d->lut.resize(rSize * gSize * bSize * 3);
                
                // Parse table
                int index = 0;
                int rIndex, gIndex, bIndex;
                float redValue, greenValue, blueValue;
                
                int entriesRemaining = rSize * gSize * bSize;
                
                while (istream.good() && entriesRemaining > 0)
                {
                    istream.getline(lineBuffer, MAX_LINE_SIZE);
                    
                    if (sscanf(lineBuffer, "%d %d %d %f %f %f",
                        &rIndex, &gIndex, &bIndex,
                        &redValue, &greenValue, &blueValue) == 6)
                    {
                        index = Lut3DArrayOffset(rIndex, gIndex, bIndex,
                                                 rSize, gSize, bSize);
                        
                        // TODO: confirm index is within bounds
                        lut3d->lut[index+0] = redValue;
                        lut3d->lut[index+1] = greenValue;
                        lut3d->lut[index+2] = blueValue;
                        
                        entriesRemaining--;
                    }
                }
                
                // Have we fully populated the table?
                if (entriesRemaining>0) 
                    throw OCSException("Not enough entries found.");
                */
                
                LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());
                //cachedFile->lut = lut3d;
                return cachedFile;
            }

            virtual void BuildFileOps(OpRcPtrVec * opVec,
                                      CachedFileRcPtr untypedCachedFile,
                                      const FileTransform& fileTransform,
                                      TransformDirection dir) const
            {
                LocalCachedFileRcPtr cachedFile = DynamicPtrCast<LocalCachedFile>(untypedCachedFile);
                
                if(!cachedFile) // This should never happen.
                {
                    std::ostringstream os;
                    os << "Cannot build 3dl Op. Invalid cache type.";
                    throw OCSException(os.str().c_str());
                }
                
                TransformDirection newDir = CombineTransformDirections(dir,
                    fileTransform.getDirection());
                
                if(newDir == TRANSFORM_DIR_INVERSE)
                {
                    // The 1D Shaper lut, if it exists is super low
                    // resolution. use the best interpolation we can.
                    // (right now, it's linear). If cubic is added, consider
                    // using it
                    
                    if(cachedFile->useLut1D)
                    {
                        OpRcPtr op1d = CreateLut1DOp(cachedFile->lut1d,
                                                   INTERP_LINEAR,
                                                   TRANSFORM_DIR_FORWARD);
                        
                        opVec->push_back(op1d);
                    }
                    
                    OpRcPtr op3d = CreateLut3DOp(cachedFile->lut3d,
                                                fileTransform.getInterpolation(),
                                                TRANSFORM_DIR_FORWARD);
                    
                    opVec->push_back(op3d);
                    
                }
                else if(newDir == TRANSFORM_DIR_INVERSE)
                {
                    OpRcPtr op3d = CreateLut3DOp(cachedFile->lut3d,
                                                fileTransform.getInterpolation(),
                                                TRANSFORM_DIR_INVERSE);
                    
                    opVec->push_back(op3d);
                    
                    if(cachedFile->useLut1D)
                    {
                        OpRcPtr op1d = CreateLut1DOp(cachedFile->lut1d,
                                                   INTERP_LINEAR,
                                                   TRANSFORM_DIR_INVERSE);
                        
                        opVec->push_back(op1d);
                    }
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
OCS_NAMESPACE_EXIT

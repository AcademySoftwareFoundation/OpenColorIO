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
#include "Lut3DOp.h"
#include "pystring/pystring.h"

#include <cstdio>
#include <sstream>

/*
SPILUT 1.0
3 3
32 32 32
0 0 0 0.0132509 0.0158522 0.0156622
0 0 1 0.0136178 0.018843 0.033921
0 0 2 0.0136487 0.0240918 0.0563014
0 0 3 0.015706 0.0303061 0.0774135

... entries can be in any order
... after the expected number of entries is found, file can contain anything
*/


OCIO_NAMESPACE_ENTER
{
    ////////////////////////////////////////////////////////////////
    
    namespace
    {
        class LocalCachedFile : public CachedFile
        {
        public:
            LocalCachedFile()
            {
                lut = Lut3D::Create();
            };
            ~LocalCachedFile() {};
            
            Lut3DRcPtr lut;
        };
        
        typedef OCIO_SHARED_PTR<LocalCachedFile> LocalCachedFileRcPtr;
        
        
        
        class LocalFileFormat : public FileFormat
        {
        public:
            
            ~LocalFileFormat() {};
            
            virtual void GetFormatInfo(FormatInfoVec & formatInfoVec) const;
            
            virtual CachedFileRcPtr Read(std::istream & istream) const;
            
            virtual void BuildFileOps(OpRcPtrVec & ops,
                         const Config& config,
                         const ConstContextRcPtr & context,
                         CachedFileRcPtr untypedCachedFile,
                         const FileTransform& fileTransform,
                         TransformDirection dir) const;
        };
        
        
        void LocalFileFormat::GetFormatInfo(FormatInfoVec & formatInfoVec) const
        {
            FormatInfo info;
            info.name = "spi3d";
            info.extension = "spi3d";
            info.capabilities = FORMAT_CAPABILITY_READ;
            formatInfoVec.push_back(info);
        }
        
        CachedFileRcPtr
        LocalFileFormat::Read(std::istream & istream) const
        {
            const int MAX_LINE_SIZE = 4096;
            char lineBuffer[MAX_LINE_SIZE];

            Lut3DRcPtr lut3d = Lut3D::Create();

            // Read header information
            istream.getline(lineBuffer, MAX_LINE_SIZE);
            if(!pystring::startswith(pystring::lower(lineBuffer), "spilut"))
            {
                std::ostringstream os;
                os << "Lut does not appear to be valid spilut format. ";
                os << "Expected 'SPILUT'.  Found, '" << lineBuffer << "'.";
                throw Exception(os.str().c_str());
            }

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
                    index = GetLut3DIndex_B(rIndex, gIndex, bIndex,
                                            rSize, gSize, bSize);
                    if(index < 0 || index >= (int) lut3d->lut.size())
                    {
                        std::ostringstream os;
                        os << "Cannot load .spi3d lut, data is invalid. ";
                        os << "A lut entry is specified (";
                        os << rIndex << " " << gIndex << " " << bIndex;
                        os << " that falls outside of the cube.";
                        throw Exception(os.str().c_str());
                    }

                    lut3d->lut[index+0] = redValue;
                    lut3d->lut[index+1] = greenValue;
                    lut3d->lut[index+2] = blueValue;

                    entriesRemaining--;
                }
            }

            // Have we fully populated the table?
            if (entriesRemaining>0) 
                throw Exception("Not enough entries found.");

            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());
            cachedFile->lut = lut3d;
            return cachedFile;
        }

        void LocalFileFormat::BuildFileOps(OpRcPtrVec & ops,
                                  const Config& /*config*/,
                                  const ConstContextRcPtr & /*context*/,
                                  CachedFileRcPtr untypedCachedFile,
                                  const FileTransform& fileTransform,
                                  TransformDirection dir) const
        {
            LocalCachedFileRcPtr cachedFile = DynamicPtrCast<LocalCachedFile>(untypedCachedFile);

            if(!cachedFile) // This should never happen.
            {
                std::ostringstream os;
                os << "Cannot build Spi3D Op. Invalid cache type.";
                throw Exception(os.str().c_str());
            }

            TransformDirection newDir = CombineTransformDirections(dir,
                fileTransform.getDirection());

            CreateLut3DOp(ops,
                          cachedFile->lut,
                          fileTransform.getInterpolation(),
                          newDir);
        }
    }
    
    FileFormat * CreateFileFormatSpi3D()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT

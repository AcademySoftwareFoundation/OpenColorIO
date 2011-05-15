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
#include "MatrixOps.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"

#include <cstdio>
#include <cstring>
#include <sstream>


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
                memset(m44, 0, 16*sizeof(float));
                memset(offset4, 0, 4*sizeof(float));
            };
            ~LocalCachedFile() {};
            
            float m44[16];
            float offset4[4];
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
            info.name = "spimtx";
            info.extension = "spimtx";
            info.capabilities = FORMAT_CAPABILITY_READ;
            formatInfoVec.push_back(info);
        }
        
        CachedFileRcPtr
        LocalFileFormat::Read(std::istream & istream) const
        {

            // Read the entire file.
            std::ostringstream fileStream;

            {
                const int MAX_LINE_SIZE = 4096;
                char lineBuffer[MAX_LINE_SIZE];

                while (istream.good())
                {
                    istream.getline(lineBuffer, MAX_LINE_SIZE);
                    fileStream << std::string(lineBuffer) << " ";
                }
            }

            // Turn it into parts
            std::vector<std::string> lineParts;
            pystring::split(pystring::strip(fileStream.str()), lineParts);
            if(lineParts.size() != 12)
            {
                std::ostringstream os;
                os << "Error parsing .spimtx file. ";
                os << "File must contain 12 float entries. ";
                os << lineParts.size() << " found.";
                throw Exception(os.str().c_str());
            }

            // Turn the parts into floats
            std::vector<float> floatArray;
            if(!StringVecToFloatVec(floatArray, lineParts))
            {
                std::ostringstream os;
                os << "Error parsing .spimtx file. ";
                os << "File must contain all float entries. ";
                throw Exception(os.str().c_str());
            }


            // Put the bits in the right place
            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());

            cachedFile->m44[0] = floatArray[0];
            cachedFile->m44[1] = floatArray[1];
            cachedFile->m44[2] = floatArray[2];
            cachedFile->m44[3] = 0.0f;

            cachedFile->m44[4] = floatArray[4];
            cachedFile->m44[5] = floatArray[5];
            cachedFile->m44[6] = floatArray[6];
            cachedFile->m44[7] = 0.0f;

            cachedFile->m44[8] = floatArray[8];
            cachedFile->m44[9] = floatArray[9];
            cachedFile->m44[10] = floatArray[10];
            cachedFile->m44[11] = 0.0f;

            cachedFile->m44[12] = 0.0f;
            cachedFile->m44[13] = 0.0f;
            cachedFile->m44[14] = 0.0f;
            cachedFile->m44[15] = 1.0f;

            cachedFile->offset4[0] = floatArray[3] / 65535.0f;
            cachedFile->offset4[1] = floatArray[7] / 65535.0f;
            cachedFile->offset4[2] = floatArray[11] / 65535.0f;
            cachedFile->offset4[3] = 0.0f;

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
                os << "Cannot build SpiMtx Ops. Invalid cache type.";
                throw Exception(os.str().c_str());
            }

            TransformDirection newDir = CombineTransformDirections(dir,
                fileTransform.getDirection());

            CreateMatrixOffsetOp(ops,
                                 cachedFile->m44,
                                 cachedFile->offset4,
                                 newDir);
        }
    }
    
    FileFormat * CreateFileFormatSpiMtx()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT

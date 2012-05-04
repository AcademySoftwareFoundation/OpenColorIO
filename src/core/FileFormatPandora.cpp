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

#include <cstdio>
#include <iostream>
#include <iterator>

#include <OpenColorIO/OpenColorIO.h>

#include "FileTransform.h"
#include "Lut1DOp.h"
#include "Lut3DOp.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        class LocalCachedFile : public CachedFile
        {
        public:
            LocalCachedFile ()
            {
                lut3D = Lut3D::Create();
            };
            ~LocalCachedFile() {};
            
            Lut3DRcPtr lut3D;
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
            info.name = "pandora_mga";
            info.extension = "mga";
            info.capabilities = FORMAT_CAPABILITY_READ;
            formatInfoVec.push_back(info);
            
            FormatInfo info2;
            info2.name = "pandora_m3d";
            info2.extension = "m3d";
            info2.capabilities = FORMAT_CAPABILITY_READ;
            formatInfoVec.push_back(info2);
        }
        
        CachedFileRcPtr
        LocalFileFormat::Read(std::istream & istream) const
        {
            // this shouldn't happen
            if(!istream)
            {
                throw Exception ("File stream empty when trying to read Pandora lut");
            }
            
            // Validate the file type
            std::string line;
            
            // Parse the file
            std::string format;
            int lutEdgeLen = 0;
            int outputBitDepthMaxValue = 0;
            std::vector<int> raw3d;
            
            {
                std::vector<std::string> parts;
                std::vector<int> tmpints;
                bool inLut3d = false;
                
                while(nextline(istream, line))
                {
                    // Strip, lowercase, and split the line
                    pystring::split(pystring::lower(pystring::strip(line)), parts);
                    if(parts.empty()) continue;
                    
                    // Skip all lines starting with '#'
                    if(pystring::startswith(parts[0],"#")) continue;
                    
                    if(parts[0] == "channel")
                    {
                        if(parts.size() != 2 || pystring::lower(parts[1]) != "3d")
                        {
                            throw Exception("Parse error in Pandora lut. Only 3d luts are currently supported. (channel: 3d).");
                        }
                    }
                    else if(parts[0] == "in")
                    {
                        int inval = 0;
                        if(parts.size() != 2 || !StringToInt( &inval, parts[1].c_str()))
                        {
                            throw Exception("Malformed 'in' tag in Pandora lut.");
                        }
                        raw3d.reserve(inval*3);
                        lutEdgeLen = Get3DLutEdgeLenFromNumPixels(inval);
                    }
                    else if(parts[0] == "out")
                    {
                        if(parts.size() != 2 || !StringToInt( &outputBitDepthMaxValue, parts[1].c_str()))
                        {
                            throw Exception("Malformed 'out' tag in Pandora lut.");
                        }
                    }
                    else if(parts[0] == "format")
                    {
                        if(parts.size() != 2 || pystring::lower(parts[1]) != "lut")
                        {
                            throw Exception("Parse error in Pandora lut. Only luts are currently supported. (format: lut).");
                        }
                    }
                    else if(parts[0] == "values")
                    {
                        if(parts.size() != 4 || 
                           pystring::lower(parts[1]) != "red" ||
                           pystring::lower(parts[2]) != "green" ||
                           pystring::lower(parts[3]) != "blue")
                        {
                            throw Exception("Parse error in Pandora lut. Only rgb luts are currently supported. (values: red green blue).");
                        }
                        
                        inLut3d = true;
                    }
                    else if(inLut3d)
                    {
                        if(!StringVecToIntVec(tmpints, parts) || tmpints.size() != 4)
                        {
                            std::ostringstream os;
                            os << "Parse error in Pandora lut. Expected to find 4 integers. Instead found '"
                            << line << "'";
                            throw Exception(os.str().c_str());
                        }
                        
                        raw3d.push_back(tmpints[1]);
                        raw3d.push_back(tmpints[2]);
                        raw3d.push_back(tmpints[3]);
                    }
                }
            }
            
            // Interpret the parsed data, validate lut sizes
            if(lutEdgeLen*lutEdgeLen*lutEdgeLen != static_cast<int>(raw3d.size()/3))
            {
                std::ostringstream os;
                os << "Parse error in Pandora lut. ";
                os << "Incorrect number of lut3d entries. ";
                os << "Found " << raw3d.size()/3 << ", expected " << lutEdgeLen*lutEdgeLen*lutEdgeLen << ".";
                throw Exception(os.str().c_str());
            }
            
            if(lutEdgeLen*lutEdgeLen*lutEdgeLen == 0)
            {
                std::ostringstream os;
                os << "Parse error in Pandora lut. ";
                os << "No 3D Lut entries found.";
                throw Exception(os.str().c_str());
            }
            
            if(outputBitDepthMaxValue <= 0)
            {
                std::ostringstream os;
                os << "Parse error in Pandora lut. ";
                os << "A valid 'out' tag was not found.";
                throw Exception(os.str().c_str());
            }
            
            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());
            
            // Reformat 3D data
            cachedFile->lut3D->size[0] = lutEdgeLen;
            cachedFile->lut3D->size[1] = lutEdgeLen;
            cachedFile->lut3D->size[2] = lutEdgeLen;
            cachedFile->lut3D->lut.reserve(raw3d.size());
            
            float scale = 1.0f / ((float)outputBitDepthMaxValue - 1.0f);
            
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
            
            return cachedFile;
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
                os << "Cannot build Truelight .cub Op. Invalid cache type.";
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
            
            if(newDir == TRANSFORM_DIR_FORWARD)
            {
                CreateLut3DOp(ops, cachedFile->lut3D,
                              fileTransform.getInterpolation(), newDir);
            }
            else if(newDir == TRANSFORM_DIR_INVERSE)
            {
                CreateLut3DOp(ops, cachedFile->lut3D,
                              fileTransform.getInterpolation(), newDir);
            }
        }
    }
    
    FileFormat * CreateFileFormatPandora()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT

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
#include <cstring>
#include <iostream>
#include <iterator>

#include <OpenColorIO/OpenColorIO.h>

#include "FileTransform.h"
#include "Lut3DOp.h"
#include "MatrixOps.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        class LocalCachedFile : public CachedFile
        {
        public:
            LocalCachedFile () :
                useMatrix(false)
            {
                lut3D = Lut3D::Create();
                memset(m44, 0, 16*sizeof(float));
            };
            ~LocalCachedFile() {};
            
            Lut3DRcPtr lut3D;
            float m44[16];
            bool useMatrix;
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
            info.name = "nukevf";
            info.extension = "vf";
            info.capabilities = FORMAT_CAPABILITY_READ;
            formatInfoVec.push_back(info);
        }
        
        CachedFileRcPtr
        LocalFileFormat::Read(std::istream & istream) const
        {
            // this shouldn't happen
            if(!istream)
            {
                throw Exception ("File stream empty when trying to read .vf lut");
            }
            
            // Validate the file type
            std::string line;
            if(!nextline(istream, line) || 
               !pystring::startswith(pystring::lower(line), "#inventor"))
            {
                throw Exception("Lut doesn't seem to be a .vf lut. Expecting '#Inventor V2.1 ascii'.");
            }
            
            // Parse the file
            std::vector<float> raw3d;
            int size3d[] = { 0, 0, 0 };
            std::vector<float> global_transform;
            
            {
                std::vector<std::string> parts;
                std::vector<float> tmpfloats;
                
                bool in3d = false;
                
                while(nextline(istream, line))
                {
                    // Strip, lowercase, and split the line
                    pystring::split(pystring::lower(pystring::strip(line)), parts);
                    
                    if(parts.empty()) continue;
                    
                    if(pystring::startswith(parts[0],"#")) continue;
                    
                    if(!in3d)
                    {
                        if(parts[0] == "grid_size")
                        {
                            if(parts.size() != 4 || 
                               !StringToInt( &size3d[0], parts[1].c_str()) ||
                               !StringToInt( &size3d[1], parts[2].c_str()) ||
                               !StringToInt( &size3d[2], parts[3].c_str()))
                            {
                                throw Exception("Malformed grid_size tag in .vf lut.");
                            }
                            
                            raw3d.reserve(3*size3d[0]*size3d[1]*size3d[2]);
                        }
                        else if(parts[0] == "global_transform")
                        {
                            if(parts.size() != 17)
                            {
                                throw Exception("Malformed global_transform tag. 16 floats expected.");
                            }
                            
                            parts.erase(parts.begin()); // Drop the 1st entry. (the tag)
                            if(!StringVecToFloatVec(global_transform, parts) || global_transform.size() != 16)
                            {
                                throw Exception("Malformed global_transform tag. Could not convert to float array.");
                            }
                        }
                        // TODO: element_size (aka scale3)
                        // TODO: world_origin (aka translate3)
                        else if(parts[0] == "data")
                        {
                            in3d = true;
                        }
                    }
                    else // (in3d)
                    {
                        if(StringVecToFloatVec(tmpfloats, parts) && (tmpfloats.size() == 3))
                        {
                            raw3d.push_back(tmpfloats[0]);
                            raw3d.push_back(tmpfloats[1]);
                            raw3d.push_back(tmpfloats[2]);
                        }
                    }
                }
            }
            
            // Interpret the parsed data, validate lut sizes
            if(size3d[0]*size3d[1]*size3d[2] != static_cast<int>(raw3d.size()/3))
            {
                std::ostringstream os;
                os << "Parse error in .vf lut. ";
                os << "Incorrect number of lut3d entries. ";
                os << "Found " << raw3d.size()/3 << ", expected " << size3d[0]*size3d[1]*size3d[2] << ".";
                throw Exception(os.str().c_str());
            }
            
            if(size3d[0]*size3d[1]*size3d[2] == 0)
            {
                std::ostringstream os;
                os << "Parse error in .vf lut. ";
                os << "No 3D Lut entries found.";
                throw Exception(os.str().c_str());
            }
            
            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());
            
            // Setup the global matrix.
            // (Nuke pre-scales this by the 3dlut size, so we must undo that here)
            if(global_transform.size() == 16)
            {
                for(int i=0; i<4; ++i)
                {
                    global_transform[4*i+0] *= static_cast<float>(size3d[0]);
                    global_transform[4*i+1] *= static_cast<float>(size3d[1]);
                    global_transform[4*i+2] *= static_cast<float>(size3d[2]);
                }
                
                memcpy(cachedFile->m44, &global_transform[0], 16*sizeof(float));
                cachedFile->useMatrix = true;
            }
            
            // Reformat 3D data
            cachedFile->lut3D->size[0] = size3d[0];
            cachedFile->lut3D->size[1] = size3d[1];
            cachedFile->lut3D->size[2] = size3d[2];
            cachedFile->lut3D->lut.reserve(raw3d.size());
            
            for(int rIndex=0; rIndex<size3d[0]; ++rIndex)
            {
                for(int gIndex=0; gIndex<size3d[1]; ++gIndex)
                {
                    for(int bIndex=0; bIndex<size3d[2]; ++bIndex)
                    {
                        int i = GetLut3DIndex_B(rIndex, gIndex, bIndex,
                                                size3d[0], size3d[1], size3d[2]);
                        
                        cachedFile->lut3D->lut.push_back(static_cast<float>(raw3d[i+0]));
                        cachedFile->lut3D->lut.push_back(static_cast<float>(raw3d[i+1]));
                        cachedFile->lut3D->lut.push_back(static_cast<float>(raw3d[i+2]));
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
                os << "Cannot build .vf Op. Invalid cache type.";
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
                if(cachedFile->useMatrix)
                {
                    CreateMatrixOp(ops, cachedFile->m44, newDir);
                }
                
                CreateLut3DOp(ops, cachedFile->lut3D,
                              fileTransform.getInterpolation(), newDir);
            }
            else if(newDir == TRANSFORM_DIR_INVERSE)
            {
                CreateLut3DOp(ops, cachedFile->lut3D,
                              fileTransform.getInterpolation(), newDir);
                
                if(cachedFile->useMatrix)
                {
                    CreateMatrixOp(ops, cachedFile->m44, newDir);
                }
            }
        }
    }
    
    FileFormat * CreateFileFormatVF()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

// TODO: Unit test

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
#include <iterator>

#include <OpenColorIO/OpenColorIO.h>

#include "FileTransform.h"
#include "Lut1DOp.h"
#include "Lut3DOp.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"

/*

http://doc.iridas.com/index.php/LUT_Formats

#comments start with '#'
#title is currently ignored, but it's not an error to enter one
TITLE "title"

#LUT_1D_SIZE M or
#LUT_3D_SIZE M
#where M is the size of the texture
#a 3D texture has the size M x M x M
#e.g. LUT_3D_SIZE 16 creates a 16 x 16 x 16 3D texture
LUT_3D_SIZE 2 

#Default input value range (domain) is 0.0 (black) to 1.0 (white)
#Specify other min/max values to map the cube to any custom input
#range you wish to use, for example if you're working with HDR data
DOMAIN_MIN 0.0 0.0 0.0
DOMAIN_MAX 1.0 1.0 1.0

#for 1D textures, the data is simply a list of floating point values,
#three per line, in RGB order
#for 3D textures, the data is also RGB, and ordered in such a way
#that the red coordinate changes fastest, then the green coordinate,
#and finally, the blue coordinate changes slowest:
0.0 0.0 0.0
1.0 0.0 0.0
0.0 1.0 0.0
1.0 1.0 0.0
0.0 0.0 1.0
1.0 0.0 1.0
0.0 1.0 1.0
1.0 1.0 1.0

#Note that the LUT data is not limited to any particular range
#and can contain values under 0.0 and over 1.0
#The processing application might however still clip the
#output values to the 0.0 - 1.0 range, depending on the internal
#precision of that application's pipeline
#IRIDAS applications generally use a floating point pipeline
#with little or no clipping


*/


OCIO_NAMESPACE_ENTER
{
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
            info.name = "iridas_cube";
            info.extension = "cube";
            info.capabilities = FORMAT_CAPABILITY_READ;
            formatInfoVec.push_back(info);
        }
        
        CachedFileRcPtr
        LocalFileFormat::Read(std::istream & istream) const
        {
            // this shouldn't happen
            if(!istream)
            {
                throw Exception ("File stream empty when trying to read Iridas .cube lut");
            }
            
            // Parse the file
            std::vector<float> raw;
            
            int size3d[] = { 0, 0, 0 };
            int size1d = 0;
            
            bool in1d = false;
            bool in3d = false;
            
            float domain_min[] = { 0.0f, 0.0f, 0.0f };
            float domain_max[] = { 1.0f, 1.0f, 1.0f };
            
            {
                std::string line;
                std::vector<std::string> parts;
                std::vector<float> tmpfloats;
                
                while(nextline(istream, line))
                {
                    // All lines starting with '#' are comments
                    if(pystring::startswith(line,"#")) continue;
                    
                    // Strip, lowercase, and split the line
                    pystring::split(pystring::lower(pystring::strip(line)), parts);
                    if(parts.empty()) continue;
                    
                    if(pystring::lower(parts[0]) == "title")
                    {
                        // Optional, and currently unhandled
                    }
                    else if(pystring::lower(parts[0]) == "lut_1d_size")
                    {
                        if(parts.size() != 2 || !StringToInt( &size1d, parts[1].c_str()))
                        {
                            throw Exception("Malformed LUT_1D_SIZE tag in Iridas .cube lut.");
                        }
                        
                        raw.reserve(3*size1d);
                        in1d = true;
                    }
                    else if(pystring::lower(parts[0]) == "lut_2d_size")
                    {
                        throw Exception("Unsupported Iridas .cube lut tag: 'LUT_2D_SIZE'.");
                    }
                    else if(pystring::lower(parts[0]) == "lut_3d_size")
                    {
                        int size = 0;
                        
                        if(parts.size() != 2 || !StringToInt( &size, parts[1].c_str()))
                        {
                            throw Exception("Malformed LUT_3D_SIZE tag in Iridas .cube lut.");
                        }
                        size3d[0] = size;
                        size3d[1] = size;
                        size3d[2] = size;
                        
                        raw.reserve(3*size3d[0]*size3d[1]*size3d[2]);
                        in3d = true;
                    }
                    else if(pystring::lower(parts[0]) == "domain_min")
                    {
                        if(parts.size() != 4 || 
                            !StringToFloat( &domain_min[0], parts[1].c_str()) ||
                            !StringToFloat( &domain_min[1], parts[2].c_str()) ||
                            !StringToFloat( &domain_min[2], parts[3].c_str()))
                        {
                            throw Exception("Malformed DOMAIN_MIN tag in Iridas .cube lut.");
                        }
                    }
                    else if(pystring::lower(parts[0]) == "domain_max")
                    {
                        if(parts.size() != 4 || 
                            !StringToFloat( &domain_max[0], parts[1].c_str()) ||
                            !StringToFloat( &domain_max[1], parts[2].c_str()) ||
                            !StringToFloat( &domain_max[2], parts[3].c_str()))
                        {
                            throw Exception("Malformed DOMAIN_MAX tag in Iridas .cube lut.");
                        }
                    }
                    else
                    {
                        // It must be a float triple!
                        
                        if(!StringVecToFloatVec(tmpfloats, parts) || tmpfloats.size() != 3)
                        {
                            std::ostringstream os;
                            os << "Malformed color triples specified in Iridas .cube lut:";
                            os << "'" << line << "'.";
                            throw Exception(os.str().c_str());
                        }
                        
                        for(int i=0; i<3; ++i)
                        {
                            raw.push_back(tmpfloats[i]);
                        }
                    }
                }
            }
            
            // Interpret the parsed data, validate lut sizes
            
            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());
            
            if(in1d)
            {
                if(size1d != static_cast<int>(raw.size()/3))
                {
                    std::ostringstream os;
                    os << "Parse error in Iridas .cube lut. ";
                    os << "Incorrect number of lut1d entries. ";
                    os << "Found " << raw.size()/3 << ", expected " << size1d << ".";
                    throw Exception(os.str().c_str());
                }
                
                // Reformat 1D data
                if(size1d>0)
                {
                    cachedFile->has1D = true;
                    memcpy(cachedFile->lut1D->from_min, domain_min, 3*sizeof(float));
                    memcpy(cachedFile->lut1D->from_max, domain_max, 3*sizeof(float));
                    
                    for(int channel=0; channel<3; ++channel)
                    {
                        cachedFile->lut1D->luts[channel].resize(size1d);
                        for(int i=0; i<size1d; ++i)
                        {
                            cachedFile->lut1D->luts[channel][i] = raw[3*i+channel];
                        }
                    }
                    
                    // 1e-5 rel error is a good threshold when float numbers near 0
                    // are written out with 6 decimal places of precision.  This is
                    // a bit aggressive, I.e., changes in the 6th decimal place will
                    // be considered roundoff error, but changes in the 5th decimal
                    // will be considered lut 'intent'.
                    // 1.0
                    // 1.000005 equal to 1.0
                    // 1.000007 equal to 1.0
                    // 1.000010 not equal
                    // 0.0
                    // 0.000001 not equal
                    
                    cachedFile->lut1D->maxerror = 1e-5f;
                    cachedFile->lut1D->errortype = ERROR_RELATIVE;
                }
            }
            else if(in3d)
            {
                cachedFile->has3D = true;
                
                if(size3d[0]*size3d[1]*size3d[2] != static_cast<int>(raw.size()/3))
                {
                    std::ostringstream os;
                    os << "Parse error in Iridas .cube lut. ";
                    os << "Incorrect number of lut3d entries. ";
                    os << "Found " << raw.size()/3 << ", expected " << size3d[0]*size3d[1]*size3d[2] << ".";
                    throw Exception(os.str().c_str());
                }
                
                // Reformat 3D data
                memcpy(cachedFile->lut3D->from_min, domain_min, 3*sizeof(float));
                memcpy(cachedFile->lut3D->from_max, domain_max, 3*sizeof(float));
                cachedFile->lut3D->size[0] = size3d[0];
                cachedFile->lut3D->size[1] = size3d[1];
                cachedFile->lut3D->size[2] = size3d[2];
                cachedFile->lut3D->lut = raw;
            }
            else
            {
                std::ostringstream os;
                os << "Parse error in Iridas .cube lut. ";
                os << "Lut type (1D/3D) unspecified.";
                throw Exception(os.str().c_str());
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
                os << "Cannot build Iridas .cube Op. Invalid cache type.";
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
    
    FileFormat * CreateFileFormatIridasCube()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

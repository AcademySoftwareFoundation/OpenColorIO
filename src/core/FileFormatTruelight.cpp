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
            LocalCachedFile () : 
                has1D(false)
            {
                lut1D = OCIO_SHARED_PTR<Lut1D>(new Lut1D());
                lut3D = OCIO_SHARED_PTR<Lut3D>(new Lut3D());
            };
            ~LocalCachedFile() {};
            
            bool has1D;
            OCIO_SHARED_PTR<Lut1D> lut1D;
            OCIO_SHARED_PTR<Lut3D> lut3D;
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
            info.name = "truelight";
            info.extension = "cub";
            info.capabilities = FORMAT_CAPABILITY_READ;
            formatInfoVec.push_back(info);
        }
        
        CachedFileRcPtr
        LocalFileFormat::Read(std::istream & istream) const
        {
            // this shouldn't happen
            if(!istream)
            {
                throw Exception ("File stream empty when trying to read Truelight .cub lut");
            }
            
            // Validate the file type
            std::string line;
            if(!nextline(istream, line) || 
               !pystring::startswith(pystring::lower(line), "# truelight cube"))
            {
                throw Exception("Lut doesn't seem to be a Truelight .cub lut.");
            }
            
            // Parse the file
            std::vector<float> raw1d;
            std::vector<float> raw3d;
            int size3d[] = { 0, 0, 0 };
            int size1d = 0;
            {
                std::vector<std::string> parts;
                std::vector<float> tmpfloats;
                
                bool in1d = false;
                bool in3d = false;
                
                while(nextline(istream, line))
                {
                    // Strip, lowercase, and split the line
                    pystring::split(pystring::lower(pystring::strip(line)), parts);
                    
                    if(parts.empty()) continue;
                    
                    if(pystring::startswith(parts[0],"#"))
                    {
                        if(parts.size() < 2) continue;
                        
                        if(parts[1] == "width")
                        {
                            if(parts.size() != 5 || 
                               !StringToInt( &size3d[0], parts[2].c_str()) ||
                               !StringToInt( &size3d[1], parts[3].c_str()) ||
                               !StringToInt( &size3d[2], parts[4].c_str()))
                            {
                                throw Exception("Malformed width tag in Truelight .cub lut.");
                            }
                            
                            raw3d.reserve(3*size3d[0]*size3d[1]*size3d[2]);
                        }
                        else if(parts[1] == "lutlength")
                        {
                            if(parts.size() != 3 || 
                               !StringToInt( &size1d, parts[2].c_str()))
                            {
                                throw Exception("Malformed lutlength tag in Truelight .cub lut.");
                            }
                            raw1d.reserve(3*size1d);
                        }
                        else if(parts[1] == "inputlut")
                        {
                            in1d = true;
                            in3d = false;
                        }
                        else if(parts[1] == "cube")
                        {
                            in3d = true;
                            in1d = false;
                        }
                    }
                    
                    if(StringVecToFloatVec(tmpfloats, parts) && (tmpfloats.size() == 3))
                    {
                        if(in1d)
                        {
                            raw1d.push_back(tmpfloats[0]);
                            raw1d.push_back(tmpfloats[1]);
                            raw1d.push_back(tmpfloats[2]);
                        }
                        else if(in3d)
                        {
                            raw3d.push_back(tmpfloats[0]);
                            raw3d.push_back(tmpfloats[1]);
                            raw3d.push_back(tmpfloats[2]);
                        }
                        else
                        {
                            throw Exception("Parse error in Truelight .cub lut. Numbers outside of lut block.");
                        }
                    }
                }
            }
            
            // Interpret the parsed data, validate lut sizes
            
            if(size1d != static_cast<int>(raw1d.size()/3))
            {
                std::ostringstream os;
                os << "Parse error in Truelight .cub lut. ";
                os << "Incorrect number of lut1d entries. ";
                os << "Found " << raw1d.size()/3 << ", expected " << size1d << ".";
                throw Exception(os.str().c_str());
            }
            
            if(size3d[0]*size3d[1]*size3d[2] != static_cast<int>(raw3d.size()/3))
            {
                std::ostringstream os;
                os << "Parse error in Truelight .cub lut. ";
                os << "Incorrect number of lut3d entries. ";
                os << "Found " << raw3d.size()/3 << ", expected " << size3d[0]*size3d[1]*size3d[2] << ".";
                throw Exception(os.str().c_str());
            }
            
            if(size3d[0]*size3d[1]*size3d[2] == 0)
            {
                std::ostringstream os;
                os << "Parse error in Truelight .cub lut. ";
                os << "No 3D Lut entries found.";
                throw Exception(os.str().c_str());
            }
            
            
            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());
            
            // Reformat 1D data
            if(size1d>0)
            {
                cachedFile->has1D = true;
                
                for(int channel=0; channel<3; ++channel)
                {
                    float descale = 1.0f / static_cast<float>(size3d[channel]);
                    cachedFile->lut1D->luts[channel].resize(size1d);
                    for(int i=0; i<size1d; ++i)
                    {
                        cachedFile->lut1D->luts[channel][i] = raw1d[3*i+channel] * descale;
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
                
                cachedFile->lut1D->finalize(1e-5f, ERROR_RELATIVE);
            }
            
            // Reformat 3D data
            cachedFile->lut3D->size[0] = size3d[0];
            cachedFile->lut3D->size[1] = size3d[1];
            cachedFile->lut3D->size[2] = size3d[2];
            cachedFile->lut3D->lut = raw3d;
            cachedFile->lut3D->generateCacheID();
            
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
                
                CreateLut3DOp(ops, cachedFile->lut3D,
                              fileTransform.getInterpolation(), newDir);
            }
            else if(newDir == TRANSFORM_DIR_INVERSE)
            {
                CreateLut3DOp(ops, cachedFile->lut3D,
                              fileTransform.getInterpolation(), newDir);
                
                if(cachedFile->has1D)
                {
                    CreateLut1DOp(ops, cachedFile->lut1D,
                                  INTERP_LINEAR, newDir);
                }
            }
        }
    }
    
    FileFormat * CreateFileFormatTruelight()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

OIIO_ADD_TEST(FileFormatTruelight, simpletest3D)
{
    // This lowers the red channel by 0.5, other channels are unaffected.
    const char * luttext = "# Truelight Cube v2.0\n"
       "# iDims 3\n"
       "# oDims 3\n"
       "# width 3 3 3\n"
       "# lutLength 4\n"
       "# InputLUT\n"
       " 0.000000 0.000000 0.000000\n"
       " 1.000000 1.000000 1.000000\n"
       " 2.000000 2.000000 2.000000\n"
       " 3.000000 3.000000 3.000000\n"
       "\n"
       "# Cube\n"
       " 0.000000 0.000000 0.000000\n"
       " 0.250000 0.000000 0.000000\n"
       " 0.500000 0.000000 0.000000\n"
       " 0.000000 0.500000 0.000000\n"
       " 0.250000 0.500000 0.000000\n"
       " 0.500000 0.500000 0.000000\n"
       " 0.000000 1.000000 0.000000\n"
       " 0.250000 1.000000 0.000000\n"
       " 0.500000 1.000000 0.000000\n"
       " 0.000000 0.000000 0.500000\n"
       " 0.250000 0.000000 0.500000\n"
       " 0.500000 0.000000 0.500000\n"
       " 0.000000 0.500000 0.500000\n"
       " 0.250000 0.500000 0.500000\n"
       " 0.500000 0.500000 0.500000\n"
       " 0.000000 1.000000 0.500000\n"
       " 0.250000 1.000000 0.500000\n"
       " 0.500000 1.000000 0.500000\n"
       " 0.000000 0.000000 1.000000\n"
       " 0.250000 0.000000 1.000000\n"
       " 0.500000 0.000000 1.000000\n"
       " 0.000000 0.500000 1.000000\n"
       " 0.250000 0.500000 1.000000\n"
       " 0.500000 0.500000 1.000000\n"
       " 0.000000 1.000000 1.000000\n"
       " 0.250000 1.000000 1.000000\n"
       " 0.500000 1.000000 1.000000\n"
       "\n"
       "# end\n"
       "\n"
       "# Truelight profile\n"
       "title{madeup on some display}\n"
       "print{someprint}\n"
       "display{some}\n"
       "cubeFile{madeup.cube}\n"
       "\n"
       "# Date Tue Jan 25 17:57:57 2011\n";
    
    std::istringstream simple3D;
    simple3D.str(luttext);
    
    // Read file
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile = tester.Read(simple3D);
    OCIO::LocalCachedFileRcPtr csplut = OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(cachedFile);
    
    // TODO: Test the results of truelight loading.
}

#endif // OCIO_UNIT_TEST

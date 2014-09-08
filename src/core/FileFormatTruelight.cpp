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
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <iterator>

#include <OpenColorIO/OpenColorIO.h>

#include "FileTransform.h"
#include "Lut1DOp.h"
#include "Lut3DOp.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"

// This implements the spec for:
// Per http://www.filmlight.ltd.uk/resources/documents/truelight/white-papers_tl.php
// FL-TL-TN-0388-TLCubeFormat2.0.pdf
//
// Known deficiency in implementation:
// 1D shaper luts (InputLUT) using integer encodings (vs float) are not supported.
// How to we determine if the input is integer? MaxVal?  Or do we look for a decimal-point?
// How about scientific notation? (which is explicitly allowed?)

/*
The input LUT is used to interpolate a higher precision LUT matched to the particular image
format. For integer formats, the range 0-1 is mapped onto the integer range. Floating point
values outside the 0-1 range are allowed but may be truncated for integer formats.
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
        
        void LocalFileFormat::GetFormatInfo(FormatInfoVec & formatInfoVec) const
        {
            FormatInfo info;
            info.name = "truelight";
            info.extension = "cub";
            info.capabilities = (FORMAT_CAPABILITY_READ | FORMAT_CAPABILITY_WRITE);
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
                    
                    // Parse header metadata (which starts with #)
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
                        else if(parts[1] == "end")
                        {
                            in3d = false;
                            in1d = false;
                            
                            // If we hit the end tag, don't bother searching further in the file.
                            break;
                        }
                    }
                    
                    
                    if(in1d || in3d)
                    {
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
            
            
            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());
            
            cachedFile->has1D = (size1d>0);
            cachedFile->has3D = (size3d[0]*size3d[1]*size3d[2]>0);
            
            // Reformat 1D data
            if(cachedFile->has1D)
            {
                for(int channel=0; channel<3; ++channel)
                {
                    // Determine the scale factor for the 1d lut. Example:
                    // The inputlut feeding a 6x6x6 3dlut should be scaled from 0.0-5.0.
                    // Beware: Nuke Truelight Writer (at least 6.3 and before) is busted
                    // and does this scaling incorrectly.
                    
                    float descale = 1.0f;
                    if(cachedFile->has3D)
                    {
                        descale = 1.0f / static_cast<float>(size3d[channel]-1);
                    }
                    
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
                
                cachedFile->lut1D->maxerror = 1e-5f;
                cachedFile->lut1D->errortype = ERROR_RELATIVE;
            }
            
            // Reformat 3D data
            if(cachedFile->has3D)
            {
                cachedFile->lut3D->size[0] = size3d[0];
                cachedFile->lut3D->size[1] = size3d[1];
                cachedFile->lut3D->size[2] = size3d[2];
                cachedFile->lut3D->lut = raw3d;
            }
            
            return cachedFile;
        }


        void
        LocalFileFormat::Write(const Baker & baker,
                               const std::string & /*formatName*/,
                               std::ostream & ostream) const
        {
            const int DEFAULT_CUBE_SIZE = 32;
            const int DEFAULT_SHAPER_SIZE = 1024;

            ConstConfigRcPtr config = baker.getConfig();
            
            int cubeSize = baker.getCubeSize();
            if (cubeSize==-1) cubeSize = DEFAULT_CUBE_SIZE;
            cubeSize = std::max(2, cubeSize); // smallest cube is 2x2x2

            std::vector<float> cubeData;
            cubeData.resize(cubeSize*cubeSize*cubeSize*3);
            GenerateIdentityLut3D(&cubeData[0], cubeSize, 3, LUT3DORDER_FAST_RED);
            PackedImageDesc cubeImg(&cubeData[0], cubeSize*cubeSize*cubeSize, 1, 3);
            
            // Apply processor to lut data
            ConstProcessorRcPtr inputToTarget;
            inputToTarget = config->getProcessor(baker.getInputSpace(), baker.getTargetSpace());
            inputToTarget->apply(cubeImg);
            
            int shaperSize = baker.getShaperSize();
            if (shaperSize==-1) shaperSize = DEFAULT_SHAPER_SIZE;
            shaperSize = std::max(2, shaperSize); // smallest shaper is 2x2x2


            // Write the header
            ostream << "# Truelight Cube v2.0\n";
            ostream << "# lutLength " << shaperSize << "\n";
            ostream << "# iDims     3\n";
            ostream << "# oDims     3\n";
            ostream << "# width     " << cubeSize << " " << cubeSize << " " << cubeSize << "\n";
            ostream << "\n";


            // Write the shaper lut
            // (We are just going to use a unity lut)
            ostream << "# InputLUT\n";
            ostream << std::setprecision(6) << std::fixed;
            float v = 0.0f;
            for (int i=0; i < shaperSize-1; i++)
            {
                v = ((float)i / (float)(shaperSize-1)) * (float)(cubeSize-1);
                ostream << v << " " << v << " " << v << "\n";
            }
            v = (float) (cubeSize-1);
            ostream << v << " " << v << " " << v << "\n"; // ensure that the last value is spot on
            ostream << "\n";

            // Write the cube
            ostream << "# Cube\n";
            for (int i=0; i<cubeSize*cubeSize*cubeSize; ++i)
            {
                ostream << cubeData[3*i+0] << " " << cubeData[3*i+1] << " " << cubeData[3*i+2] << "\n";
            }
            
            ostream << "# end\n";
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

OIIO_ADD_TEST(FileFormatTruelight, ShaperAndLut3D)
{
    // This lowers the red channel by 0.5, other channels are unaffected.
    const char * luttext = "# Truelight Cube v2.0\n"
       "# iDims 3\n"
       "# oDims 3\n"
       "# width 3 3 3\n"
       "# lutLength 5\n"
       "# InputLUT\n"
       " 0.000000 0.000000 0.000000\n"
       " 0.500000 0.500000 0.500000\n"
       " 1.000000 1.000000 1.000000\n"
       " 1.500000 1.500000 1.500000\n"
       " 2.000000 2.000000 2.000000\n"
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
       " # This last line confirms 'end' tag is obeyed\n"
       " 1.23456 1.23456 1.23456\n";
    
    std::istringstream lutIStream;
    lutIStream.str(luttext);
    
    // Read file
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile;
    OIIO_CHECK_NO_THOW(cachedFile = tester.Read(lutIStream));
    OCIO::LocalCachedFileRcPtr lut = OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(cachedFile);
    
    OIIO_CHECK_ASSERT(lut->has1D);
    OIIO_CHECK_ASSERT(lut->has3D);
    
    float data[4*3] = { 0.1f, 0.2f, 0.3f, 0.0f,
                        1.0f, 0.5f, 0.123456f, 0.0f,
                       -1.0f, 1.5f, 0.5f, 0.0f };
    
    float result[4*3] = { 0.05f, 0.2f, 0.3f, 0.0f,
                          0.50f, 0.5f, 0.123456f, 0.0f,
                          0.0f, 1.5f, 0.5f, 0.0f };
    
    OCIO::OpRcPtrVec ops;
    if(lut->has1D)
    {
        CreateLut1DOp(ops, lut->lut1D,
                      OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_FORWARD);
    }
    if(lut->has3D)
    {
        CreateLut3DOp(ops, lut->lut3D,
                      OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_FORWARD);
    }
    FinalizeOpVec(ops);
    
    
    // Apply the result
    for(OCIO::OpRcPtrVec::size_type i = 0, size = ops.size(); i < size; ++i)
    {
        ops[i]->apply(data, 3);
    }
    
    for(int i=0; i<3; ++i)
    {
        OIIO_CHECK_CLOSE( data[i], result[i], 1.0e-6 );
    }
}

OIIO_ADD_TEST(FileFormatTruelight, Shaper)
{
    const char * luttext = "# Truelight Cube v2.0\n"
       "# lutLength 11\n"
       "# iDims 3\n"
       "\n"
       "\n"
       "# InputLUT\n"
       " 0.000 0.000 -0.000\n"
       " 0.200 0.010 -0.100\n"
       " 0.400 0.040 -0.200\n"
       " 0.600 0.090 -0.300\n"
       " 0.800 0.160 -0.400\n"
       " 1.000 0.250 -0.500\n"
       " 1.200 0.360 -0.600\n"
       " 1.400 0.490 -0.700\n"
       " 1.600 0.640 -0.800\n"
       " 1.800 0.820 -0.900\n"
       " 2.000 1.000 -1.000\n"
       "\n\n\n"
       "# end\n";
    
    std::istringstream lutIStream;
    lutIStream.str(luttext);
    
    // Read file
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile;
    OIIO_CHECK_NO_THOW(cachedFile = tester.Read(lutIStream));
    
    OCIO::LocalCachedFileRcPtr lut = OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(cachedFile);
    
    OIIO_CHECK_ASSERT(lut->has1D);
    OIIO_CHECK_ASSERT(!lut->has3D);
    
    float data[4*3] = { 0.1f, 0.2f, 0.3f, 0.0f,
                        1.0f, 0.5f, 0.123456f, 0.0f,
                       -1.0f, 1.5f, 0.5f, 0.0f };
    
    float result[4*3] = { 0.2f, 0.04f, -0.3f, 0.0f,
                          2.0f, 0.25f, -0.123456f, 0.0f,
                          0.0f, 1.0f, -0.5f, 0.0f };
    
    OCIO::OpRcPtrVec ops;
    if(lut->has1D)
    {
        CreateLut1DOp(ops, lut->lut1D,
                      OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_FORWARD);
    }
    if(lut->has3D)
    {
        CreateLut3DOp(ops, lut->lut3D,
                      OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_FORWARD);
    }
    FinalizeOpVec(ops);
    
    
    // Apply the result
    for(OCIO::OpRcPtrVec::size_type i = 0, size = ops.size(); i < size; ++i)
    {
        ops[i]->apply(data, 3);
    }
    
    for(int i=0; i<3; ++i)
    {
        OIIO_CHECK_CLOSE( data[i], result[i], 1.0e-6 );
    }
}


OIIO_ADD_TEST(FileFormatTruelight, Lut3D)
{
    // This lowers the red channel by 0.5, other channels are unaffected.
    const char * luttext = "# Truelight Cube v2.0\n"
       "# iDims 3\n"
       "# oDims 3\n"
       "# width 3 3 3\n"
       "\n\n\n"
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
       "# end\n";
    
    std::istringstream lutIStream;
    lutIStream.str(luttext);
    
    // Read file
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile;
    OIIO_CHECK_NO_THOW(cachedFile = tester.Read(lutIStream));
    OCIO::LocalCachedFileRcPtr lut = OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(cachedFile);
    
    OIIO_CHECK_ASSERT(!lut->has1D);
    OIIO_CHECK_ASSERT(lut->has3D);
    
    float data[4*3] = { 0.1f, 0.2f, 0.3f, 0.0f,
                        1.0f, 0.5f, 0.123456f, 0.0f,
                       -1.0f, 1.5f, 0.5f, 0.0f };
    
    float result[4*3] = { 0.05f, 0.2f, 0.3f, 0.0f,
                          0.50f, 0.5f, 0.123456f, 0.0f,
                          0.0f, 1.5f, 0.5f, 0.0f };
    
    OCIO::OpRcPtrVec ops;
    if(lut->has1D)
    {
        CreateLut1DOp(ops, lut->lut1D,
                      OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_FORWARD);
    }
    if(lut->has3D)
    {
        CreateLut3DOp(ops, lut->lut3D,
                      OCIO::INTERP_LINEAR, OCIO::TRANSFORM_DIR_FORWARD);
    }
    FinalizeOpVec(ops);
    
    
    // Apply the result
    for(OCIO::OpRcPtrVec::size_type i = 0, size = ops.size(); i < size; ++i)
    {
        ops[i]->apply(data, 3);
    }
    
    for(int i=0; i<3; ++i)
    {
        OIIO_CHECK_CLOSE( data[i], result[i], 1.0e-6 );
    }
}

#endif // OCIO_UNIT_TEST

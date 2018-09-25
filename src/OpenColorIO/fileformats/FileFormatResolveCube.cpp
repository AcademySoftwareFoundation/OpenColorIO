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
#include <cstring>
#include <iterator>

#include <OpenColorIO/OpenColorIO.h>

#include "transforms/FileTransform.h"
#include "ops/Lut1D/Lut1DOp.h"
#include "ops/Lut3D/Lut3DOp.h"
#include "ParseUtils.h"
#include "MathUtils.h"
#include "Logging.h"
#include "pystring/pystring.h"

/*

Peter Chamberlain
https://forum.blackmagicdesign.com/viewtopic.php?f=21&t=40284#p232952

Resolve LUT Info

3D Format (.cube)
The LUT file needs to have an extension of .cube
While described as a 3D LUT format the .cube file could contain ...
- 3D LUT data (only)
- 1D LUT data (only)
- Both a 3D LUT and a 1D 'shaper' LUT.

Irrespective of what data a .cube file contains (1D, 3D or both), it is always
displayed by Resolve in the 3D LUT section.

Lines beginning with # are considered comments. All comment lines need to be 
placed before the header lines.

3D LUT data (only)
There is a header of 2 lines:
LUT_3D_SIZE N (where N is the number of points along each axis of the LUT.)
LUT_3D_INPUT_RANGE MIN MAX (where MIN is the minimum input value expected to be
passed through the 3D LUT. Like wise for MAX.)

Followed by this are NxNxN rows of RGB values between 0.0 and 1.0. Each row
has 3 floating point numbers separated by a space (space delimited). The data
is ordered as red major (red fastest)

Top few lines of a sample .cube 3D LUT file which would contain 33x33x33 rows
of 3D LUT data and expects input in the range 0.0 1.0:

LUT_3D_SIZE 33
LUT_3D_INPUT_RANGE 0.0 1.0
0.000000 0.000000 0.000000
0.047059 0.000000 0.000000
0.101961 0.000000 0.000000
0.156863 0.000000 0.000000
0.207843 0.000000 0.000000
0.262745 0.000000 0.000000
0.321569 0.000000 0.000000
0.376471 0.000000 0.000000
0.427451 0.000000 0.000000
0.482353 0.000000 0.000000
0.537255 0.000000 0.000000
0.592157 0.000000 0.000000


1D LUT data (only)

The 1D LUT data requires a header with the following two fields
LUT_1D_SIZE N (where N is the number of points in each channel of the 1D LUT)
LUT_1D_INPUT_RANGE MIN MAX (where MIN is the minimum input value expected to be
passed through the 1D LUT. Like wise for MAX.)

This is followed by N data lines with 3 floating point values per line with a
space separating them (first is R, second is G, third is B)

Top few lines of a sample .cube file containing 1D LUT data only. It contains
4096 rows of 1D LUT data and expects input in the range 0.0 1.0:

LUT_1D_SIZE 4096
LUT_1D_INPUT_RANGE 0.0 1.0
0.000000 0.000000 0.000000
0.047059 0.047059 0.047059
0.101961 0.101961 0.101961
0.156863 0.156863 0.156863
0.207843 0.207843 0.207843
0.262745 0.262745 0.262745
0.321569 0.321569 0.321569
0.376471 0.376471 0.376471
0.427451 0.427451 0.427451
0.482353 0.482353 0.482353
0.537255 0.537255 0.537255
0.592157 0.592157 0.592157


1D 'shaper' LUT and 3D LUT data

When a .cube file contains both 1D and 3D LUT data the 1D LUT data is treated
as a 'shaper' LUT and is applied first with the output from the 1D 'shaper' LUT
section then being fed into the 3D LUT section.

A .cube file containing a 1D 'shaper' LUT and 3D LUT data requires a header
with the following fields:
LUT_1D_SIZE N1D (where N1D is the number of points in each channel of the
1D 'shaper' LUT)
LUT_1D_INPUT_RANGE MIN1D MAX1D (where MIN1D is the minimum input value
expected to be passed through the 'shaper' 1D LUT. Like wise for MAX1D.)
LUT_3D_SIZE N3D (where N3D is the number of points along each axis of
the LUT.)
LUT_3D_INPUT_RANGE MIN3D MAX 3D (where MIN3D is the minimum input value
expected to be passed through the 3D LUT. Like wise for MAX3D.)

This is followed the 1D 'shaper' data: N1D data lines with 3 floating point
values per line with a space separating them (first is R, second is G,
third is B).
Followed by the 3D data: N3DxN3DxN3D rows of RGB values between 0.0 and 1.0.
Each row has 3 floating point numbers separated by a space (space delimited).
The data is ordered as red major (red fastest).

Below is an example of a sample .cube file containing 1D 'shaper' LUT and
3D LUT data. It contains 6 rows of 1D LUT data with input in the range 0.0 1.0
and 3x3x3 (27) rows of 3D LUT data with input in the range 0.0 1.0

# Sample 3D cube file containing 1D shaper LUT and 3D LUT.
# 1. The size and range of both the LUTs should be specified first.
# 2. LUT_*D_INPUT_RANGE is an optional field.
# 3. The 1D shaper LUT below inverts the signal
# 4. The 3D LUT which follows inverts the signal again
LUT_1D_SIZE 6
LUT_1D_INPUT_RANGE 0.0 1.0
LUT_3D_SIZE 3
LUT_3D_INPUT_RANGE 0.0 1.0
1.0 1.0 1.0
0.8 0.8 0.8
0.6 0.6 0.6
0.4 0.4 0.4
0.2 0.2 0.2
0.0 0.0 0.0
1.0 1.0 1.0
0.5 1.0 1.0
0.0 1.0 1.0
1.0 0.5 1.0
0.5 0.5 1.0
0.0 0.5 1.0
1.0 0.0 1.0
0.5 0.0 1.0
0.0 0.0 1.0
1.0 1.0 0.5
0.5 1.0 0.5
0.0 1.0 0.5
1.0 0.5 0.5
0.5 0.5 0.5
0.0 0.5 0.5
1.0 0.0 0.5
0.5 0.0 0.5
0.0 0.0 0.5
1.0 1.0 0.0
0.5 1.0 0.0
0.0 1.0 0.0
1.0 0.5 0.0
0.5 0.5 0.0
0.0 0.5 0.0
1.0 0.0 0.0
0.5 0.0 0.0
0.0 0.0 0.0

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
            
            virtual CachedFileRcPtr Read(
                std::istream & istream,
                const std::string & fileName) const;
            
            virtual void Write(const Baker & baker,
                               const std::string & formatName,
                               std::ostream & ostream) const;
            
            virtual void BuildFileOps(OpRcPtrVec & ops,
                         const Config& config,
                         const ConstContextRcPtr & context,
                         CachedFileRcPtr untypedCachedFile,
                         const FileTransform& fileTransform,
                         TransformDirection dir) const;
        private:
            static void ThrowErrorMessage(const std::string & error,
                const std::string & fileName,
                int line,
                const std::string & lineContent);
        };
        
        void LocalFileFormat::ThrowErrorMessage(const std::string & error,
            const std::string & fileName,
            int line,
            const std::string & lineContent)
        {
            std::ostringstream os;
            os << "Error parsing Resolve .cube file (";
            os << fileName;
            os << ").  ";
            if (-1 != line)
            {
                os << "At line (" << line << "): '";
                os << lineContent << "'.  ";
            }
            os << error;

            throw Exception(os.str().c_str());
        }

        void LocalFileFormat::GetFormatInfo(FormatInfoVec & formatInfoVec) const
        {
            FormatInfo info;
            info.name = "resolve_cube";
            info.extension = "cube";
            info.capabilities = FORMAT_CAPABILITY_ALL;
            formatInfoVec.push_back(info);
        }
        
        CachedFileRcPtr
        LocalFileFormat::Read(
            std::istream & istream,
            const std::string & fileName) const
        {
            
            // this shouldn't happen
            if(!istream)
            {
                throw Exception ("File stream empty when trying to read Resolve .cube lut");
            }
            
            // Parse the file
            std::vector<float> raw1d;
            std::vector<float> raw3d;
            
            int size3d = 0;
            int size1d = 0;
            
            bool has1d = false;
            bool has3d = false;
            
            float range1d_min = 0.0f;
            float range1d_max = 1.0f;
            
            float range3d_min = 0.0f;
            float range3d_max = 1.0f;
            
            {
                std::string line;
                std::vector<std::string> parts;
                std::vector<float> tmpfloats;
                int lineNumber = 0;
                bool headerComplete = false;
                int tripletNumber = 0;
                
                while(nextline(istream, line))
                {
                    ++lineNumber;
                    
                    // All lines starting with '#' are comments
                    if(pystring::startswith(line,"#"))
                    {
                        if(headerComplete)
                        {
                            ThrowErrorMessage(
                                "Comments not allowed after header.",
                                fileName,
                                lineNumber,
                                line);
                        }
                        else
                        {
                            continue;
                        }
                    }
                    
                    // Strip, lowercase, and split the line
                    pystring::split(pystring::lower(pystring::strip(line)), parts);
                    if(parts.empty()) continue;
                    
                    if(pystring::lower(parts[0]) == "title")
                    {
                        ThrowErrorMessage(
                            "Unsupported tag: 'TITLE'.",
                            fileName,
                            lineNumber,
                            line);
                    }
                    else if(pystring::lower(parts[0]) == "lut_1d_size")
                    {
                        if(parts.size() != 2
                            || !StringToInt( &size1d, parts[1].c_str()))
                        {
                            ThrowErrorMessage(
                                "Malformed LUT_1D_SIZE tag.",
                                fileName,
                                lineNumber,
                                line);
                        }
                        
                        raw1d.reserve(3*size1d);
                        has1d = true;
                    }
                    else if(pystring::lower(parts[0]) == "lut_2d_size")
                    {
                        ThrowErrorMessage(
                            "Unsupported tag: 'LUT_2D_SIZE'.",
                            fileName,
                            lineNumber,
                            line);
                    }
                    else if(pystring::lower(parts[0]) == "lut_3d_size")
                    {
                        if(parts.size() != 2
                            || !StringToInt( &size3d, parts[1].c_str()))
                        {
                            ThrowErrorMessage(
                                "Malformed LUT_3D_SIZE tag.",
                                fileName,
                                lineNumber,
                                line);
                        }
                        
                        raw3d.reserve(3*size3d*size3d*size3d);
                        has3d = true;
                    }
                    else if(pystring::lower(parts[0]) == "lut_1d_input_range")
                    {
                        if(parts.size() != 3 || 
                            !StringToFloat( &range1d_min, parts[1].c_str()) ||
                            !StringToFloat( &range1d_max, parts[2].c_str()))
                        {
                            ThrowErrorMessage(
                                "Malformed LUT_1D_INPUT_RANGE tag.",
                                fileName,
                                lineNumber,
                                line);
                        }
                    }
                    else if(pystring::lower(parts[0]) == "lut_3d_input_range")
                    {
                        if(parts.size() != 3 || 
                            !StringToFloat( &range3d_min, parts[1].c_str()) ||
                            !StringToFloat( &range3d_max, parts[2].c_str()))
                        {
                            ThrowErrorMessage(
                                "Malformed LUT_3D_INPUT_RANGE tag.",
                                fileName,
                                lineNumber,
                                line);
                        }
                    }
                    else
                    {
                        headerComplete = true;
                        
                        // It must be a float triple!
                        if(!StringVecToFloatVec(tmpfloats, parts) || tmpfloats.size() != 3)
                        {
                            ThrowErrorMessage(
                                "Malformed color triples specified.",
                                fileName,
                                lineNumber,
                                line);
                        }
                        
                        for(int i=0; i<3; ++i)
                        {
                            if(has1d && tripletNumber < size1d)
                            {
                                raw1d.push_back(tmpfloats[i]);
                            }
                            else
                            {
                                raw3d.push_back(tmpfloats[i]);
                            }
                        }
                        
                        ++tripletNumber;
                    }
                }
            }
            
            // Interpret the parsed data, validate lut sizes
            
            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());
            
            if(has1d)
            {
                if(size1d != static_cast<int>(raw1d.size()/3))
                {
                    std::ostringstream os;
                    os << "Incorrect number of lut1d entries. ";
                    os << "Found " << raw1d.size() / 3;
                    os << ", expected " << size1d << ".";
                    ThrowErrorMessage(
                        os.str().c_str(),
                        fileName, -1, "");
                }
                
                // Reformat 1D data
                if(size1d>0)
                {
                    cachedFile->has1D = true;
                    
                    for(size_t i=0; i<3; ++i)
                    {
                        cachedFile->lut1D->from_min[i] = range1d_min;
                        cachedFile->lut1D->from_max[i] = range1d_max;
                    }
                    
                    for(int channel=0; channel<3; ++channel)
                    {
                        cachedFile->lut1D->luts[channel].resize(size1d);
                        for(int i=0; i<size1d; ++i)
                        {
                            cachedFile->lut1D->luts[channel][i] = raw1d[3*i+channel];
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
            if(has3d)
            {
                cachedFile->has3D = true;
                
                if(size3d*size3d*size3d 
                    != static_cast<int>(raw3d.size()/3))
                {
                    std::ostringstream os;
                    os << "Incorrect number of lut3d entries. ";
                    os << "Found " << raw3d.size() / 3 << ", expected ";
                    os << size3d * size3d * size3d << ".";
                    ThrowErrorMessage(
                        os.str().c_str(),
                        fileName, -1, "");
                }
                
                // Reformat 3D data
                for(size_t i=0; i<3; ++i)
                {
                    cachedFile->lut3D->from_min[i] = range3d_min;
                    cachedFile->lut3D->from_max[i] = range3d_max;
                    cachedFile->lut3D->size[i] = size3d;
                }
                cachedFile->lut3D->lut = raw3d;
            }
            if(!has1d && !has3d)
            {
                ThrowErrorMessage(
                    "Lut type (1D/3D) unspecified.",
                    fileName, -1, "");
            }
            
            return cachedFile;
        }
        
        void LocalFileFormat::Write(const Baker & baker,
                                    const std::string & formatName,
                                    std::ostream & ostream) const
        {
            
            const int DEFAULT_1D_SIZE = 4096;
            const int DEFAULT_SHAPER_SIZE = 4096;
            const int DEFAULT_3D_SIZE = 64;
            
            if(formatName != "resolve_cube")
            {
                std::ostringstream os;
                os << "Unknown cube format name, '";
                os << formatName << "'.";
                throw Exception(os.str().c_str());
            }
            
            //
            // Initialize config and data
            //
            
            ConstConfigRcPtr config = baker.getConfig();
            
            int onedSize = baker.getCubeSize();
            if(onedSize==-1) onedSize = DEFAULT_1D_SIZE;
            if(onedSize<2)
            {
                std::ostringstream os;
                os << "1D LUT size must be higher than 2 (was " << onedSize << ")";
                throw Exception(os.str().c_str());
            }
            
            int cubeSize = baker.getCubeSize();
            if(cubeSize==-1) cubeSize = DEFAULT_3D_SIZE;
            cubeSize = std::max(2, cubeSize); // smallest cube is 2x2x2
            
            int shaperSize = baker.getShaperSize();
            if(shaperSize<0) shaperSize = DEFAULT_SHAPER_SIZE;
            if(shaperSize<2)
            {
                std::ostringstream os;
                os << "A shaper space ('" << baker.getShaperSpace() << "') has";
                os << " been specified, so the shaper size must be 2 or larger";
                throw Exception(os.str().c_str());
            }
            
            // Get spaces from baker
            const std::string shaperSpace = baker.getShaperSpace();
            const std::string inputSpace = baker.getInputSpace();
            const std::string targetSpace = baker.getTargetSpace();
            const std::string looks = baker.getLooks();
            
            //
            // Determine required LUT type
            //
            
            const int CUBE_1D = 1; // 1D LUT version number
            const int CUBE_3D = 2; // 3D LUT version number
            const int CUBE_1D_3D = 3; // 3D LUT with 1D prelut
            
            ConstProcessorRcPtr inputToTargetProc;
            if (!looks.empty())
            {
                LookTransformRcPtr transform = LookTransform::Create();
                transform->setLooks(looks.c_str());
                transform->setSrc(inputSpace.c_str());
                transform->setDst(targetSpace.c_str());
                inputToTargetProc = config->getProcessor(transform,
                    TRANSFORM_DIR_FORWARD);
            }
            else
            {
                inputToTargetProc = config->getProcessor(
                    inputSpace.c_str(),
                    targetSpace.c_str());
            }
            
            int required_lut = -1;
            
            if(inputToTargetProc->hasChannelCrosstalk())
            {
                if(shaperSpace.empty())
                {
                    // Has crosstalk, but no shaper, so need 3D LUT
                    required_lut = CUBE_3D;
                }
                else
                {
                    // Crosstalk with shaper-space
                    required_lut = CUBE_1D_3D;
                }
            }
            else
            {
                // No crosstalk
                required_lut = CUBE_1D;
            }

            if(required_lut == -1)
            {
                // Unnecessary paranoia
                throw Exception(
                    "Internal logic error, LUT type was not determined");
            }
            
            //
            // Generate Shaper
            //
            
            std::vector<float> shaperData;
            
            float fromInStart = 0;
            float fromInEnd = 1;
            
            if(required_lut == CUBE_1D_3D)
            {
                // TODO: Later we only grab the green channel for the prelut,
                // should ensure the prelut is monochromatic somehow?
                
                ConstProcessorRcPtr inputToShaperProc = config->getProcessor(
                    inputSpace.c_str(),
                    shaperSpace.c_str());
                
                if(inputToShaperProc->hasChannelCrosstalk())
                {
                    // TODO: Automatically turn shaper into
                    // non-crosstalked version?
                    std::ostringstream os;
                    os << "The specified shaperSpace, '" << baker.getShaperSpace();
                    os << "' has channel crosstalk, which is not appropriate for";
                    os << " shapers. Please select an alternate shaper space or";
                    os << " omit this option.";
                    throw Exception(os.str().c_str());
                }
                
                // Calculate min/max value
                {
                    // Get input value of 1.0 in shaper space, as this
                    // is the higest value that is transformed by the
                    // cube (e.g for a generic lin-to-log transform,
                    // what the log value 1.0 is in linear).
                    ConstProcessorRcPtr shaperToInputProc = config->getProcessor(
                        shaperSpace.c_str(),
                        inputSpace.c_str());

                    float minval[3] = {0.0f, 0.0f, 0.0f};
                    float maxval[3] = {1.0f, 1.0f, 1.0f};

                    shaperToInputProc->applyRGB(minval);
                    shaperToInputProc->applyRGB(maxval);
                    
                    // Grab green channel, as this is the one used later
                    fromInStart = minval[1];
                    fromInEnd = maxval[1];
                }

                // Generate the identity shaper values, then apply the transform.
                // Shaper is linearly sampled from fromInStart to fromInEnd
                shaperData.resize(shaperSize*3);
                
                for (int i = 0; i < shaperSize; ++i)
                {
                    const float x = (float)(double(i) / double(shaperSize - 1));
                    float cur_value = lerpf(fromInStart, fromInEnd, x);

                    shaperData[3*i+0] = cur_value;
                    shaperData[3*i+1] = cur_value;
                    shaperData[3*i+2] = cur_value;
                }
                
                PackedImageDesc shaperImg(&shaperData[0], shaperSize, 1, 3);
                inputToShaperProc->apply(shaperImg);
            }
            
            //
            // Generate 3DLUT
            //
            
            std::vector<float> cubeData;
            if(required_lut == CUBE_3D || required_lut == CUBE_1D_3D)
            {
                cubeData.resize(cubeSize*cubeSize*cubeSize*3);
                GenerateIdentityLut3D(&cubeData[0], cubeSize, 3, LUT3DORDER_FAST_RED);
                PackedImageDesc cubeImg(&cubeData[0], cubeSize*cubeSize*cubeSize, 1, 3);
                
                ConstProcessorRcPtr cubeProc;
                if(required_lut == CUBE_1D_3D)
                {
                    // Shaper goes from input-to-shaper, so cube goes from shaper-to-target
                    if (!looks.empty())
                    {
                        LookTransformRcPtr transform = LookTransform::Create();
                        transform->setLooks(looks.c_str());
                        transform->setSrc(shaperSpace.c_str());
                        transform->setDst(targetSpace.c_str());
                        cubeProc = config->getProcessor(transform,
                            TRANSFORM_DIR_FORWARD);
                    }
                    else
                    {
                        cubeProc = config->getProcessor(shaperSpace.c_str(),
                                                        targetSpace.c_str());
                    }
                }
                else
                {
                    // No shaper, so cube goes from input-to-target
                    cubeProc = inputToTargetProc;
                }

                
                cubeProc->apply(cubeImg);
            }
            
            //
            // Generate 1DLUT
            //
            
            std::vector<float> onedData;
            if(required_lut == CUBE_1D)
            {
                onedData.resize(onedSize * 3);
                GenerateIdentityLut1D(&onedData[0], onedSize, 3);
                PackedImageDesc onedImg(&onedData[0], onedSize, 1, 3);
                
                inputToTargetProc->apply(onedImg);
            }
            
            //
            // Write LUT
            //
            
            // Set to a fixed 6 decimal precision
            ostream.setf(std::ios::fixed, std::ios::floatfield);
            ostream.precision(6);
            
            // Comments
            if(baker.getMetadata() != NULL)
            {
                std::string metadata = baker.getMetadata();
                std::vector<std::string> metadatavec;
                pystring::split(pystring::strip(metadata), metadatavec, "\n");
                if(metadatavec.size() > 0)
                {
                    for(size_t i = 0; i < metadatavec.size(); ++i)
                    {
                        ostream << "# " << metadatavec[i] << "\n";
                    }
                    ostream << "\n";
                }
            }
            
            // Header
            // Note about LUT_ND_INPUT_RANGE tags :
            // These tags are optional and will default to the 0..1 range,
            // not wrting them explicitly allow for wider compatibility
            // with parser based on other cube specification (eg. Iridas_Itx)
            if(required_lut == CUBE_1D)
            {
                ostream << "LUT_1D_SIZE " << onedSize << "\n";
                //ostream << "LUT_1D_INPUT_RANGE 0.0 1.0\n";
            }
            else if(required_lut == CUBE_1D_3D)
            {
                ostream << "LUT_1D_SIZE " << shaperSize << "\n";
                ostream << "LUT_1D_INPUT_RANGE " << fromInStart << " " << fromInEnd << "\n";
            }
            if(required_lut == CUBE_3D || required_lut == CUBE_1D_3D)
            {
                ostream << "LUT_3D_SIZE " << cubeSize << "\n";
                //ostream << "LUT_3D_INPUT_RANGE 0.0 1.0\n";
            }
            
            // Write 1D data
            if(required_lut == CUBE_1D)
            {
                for(int i=0; i<onedSize; ++i)
                {
                    ostream << onedData[3*i+0] << " "
                            << onedData[3*i+1] << " "
                            << onedData[3*i+2] << "\n";
                }
            }
            else if(required_lut == CUBE_1D_3D)
            {
                for(int i=0; i<shaperSize; ++i)
                {
                    ostream << shaperData[3*i+0] << " "
                            << shaperData[3*i+1] << " "
                            << shaperData[3*i+2] << "\n";
                }
            }
            
            // Write 3D data
            if(required_lut == CUBE_3D || required_lut == CUBE_1D_3D)
            {
                for(int i=0; i<cubeSize*cubeSize*cubeSize; ++i)
                {
                    ostream << cubeData[3*i+0] << " "
                            << cubeData[3*i+1] << " "
                            << cubeData[3*i+2] << "\n";
                }
            }
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
                os << "Cannot build Resolve .cube Op. Invalid cache type.";
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
    
    FileFormat * CreateFileFormatResolveCube()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"
#include <fstream>

OCIO::LocalCachedFileRcPtr ReadResolveCube(const std::string & fileContent)
{
    std::istringstream is;
    is.str(fileContent);

    // Read file
    OCIO::LocalFileFormat tester;
    const std::string SAMPLE_NAME("Memory File");
    OCIO::CachedFileRcPtr cachedFile = tester.Read(is, SAMPLE_NAME);

    return OCIO::DynamicPtrCast<OCIO::LocalCachedFile>(cachedFile);
}

OIIO_ADD_TEST(FileFormatResolveCube, FormatInfo)
{
    OCIO::FormatInfoVec formatInfoVec;
    OCIO::LocalFileFormat tester;
    tester.GetFormatInfo(formatInfoVec);

    OIIO_CHECK_EQUAL(1, formatInfoVec.size());
    OIIO_CHECK_EQUAL("resolve_cube", formatInfoVec[0].name);
    OIIO_CHECK_EQUAL("cube", formatInfoVec[0].extension);
    OIIO_CHECK_EQUAL(OCIO::FORMAT_CAPABILITY_READ | OCIO::FORMAT_CAPABILITY_WRITE,
        formatInfoVec[0].capabilities);
}

OIIO_ADD_TEST(FileFormatResolveCube, Read1D)
{
    const std::string SAMPLE =
        "LUT_1D_SIZE 2\n"
        "LUT_1D_INPUT_RANGE 0.0 1.0\n"

        "0.0 0.0 0.0\n"
        "1.0 0.0 0.0\n";

    OIIO_CHECK_NO_THROW(ReadResolveCube(SAMPLE));
}

OIIO_ADD_TEST(FileFormatResolveCube, Read3D)
{
    const std::string SAMPLE =
        "LUT_3D_SIZE 2\n"
        "LUT_3D_INPUT_RANGE 0.0 1.0\n"

        "0.0 0.0 0.0\n"
        "1.0 0.0 0.0\n"
        "0.0 1.0 0.0\n"
        "1.0 1.0 0.0\n"
        "0.0 0.0 1.0\n"
        "1.0 0.0 1.0\n"
        "0.0 1.0 1.0\n"
        "1.0 1.0 1.0\n";

    OIIO_CHECK_NO_THROW(ReadResolveCube(SAMPLE));
}

OIIO_ADD_TEST(FileFormatResolveCube, Read1D3D)
{
    const std::string SAMPLE =
        "LUT_1D_SIZE 6\n"
        "LUT_1D_INPUT_RANGE 0.0 1.0\n"
        "LUT_3D_SIZE 3\n"
        "LUT_3D_INPUT_RANGE 0.0 1.0\n"
        
        "1.0 1.0 1.0\n"
        "0.8 0.8 0.8\n"
        "0.6 0.6 0.6\n"
        "0.4 0.4 0.4\n"
        "0.2 0.2 0.2\n"
        "0.0 0.0 0.0\n"
        "1.0 1.0 1.0\n"
        "0.5 1.0 1.0\n"
        "0.0 1.0 1.0\n"
        "1.0 0.5 1.0\n"
        "0.5 0.5 1.0\n"
        "0.0 0.5 1.0\n"
        "1.0 0.0 1.0\n"
        "0.5 0.0 1.0\n"
        "0.0 0.0 1.0\n"
        "1.0 1.0 0.5\n"
        "0.5 1.0 0.5\n"
        "0.0 1.0 0.5\n"
        "1.0 0.5 0.5\n"
        "0.5 0.5 0.5\n"
        "0.0 0.5 0.5\n"
        "1.0 0.0 0.5\n"
        "0.5 0.0 0.5\n"
        "0.0 0.0 0.5\n"
        "1.0 1.0 0.0\n"
        "0.5 1.0 0.0\n"
        "0.0 1.0 0.0\n"
        "1.0 0.5 0.0\n"
        "0.5 0.5 0.0\n"
        "0.0 0.5 0.0\n"
        "1.0 0.0 0.0\n"
        "0.5 0.0 0.0\n"
        "0.0 0.0 0.0\n";

    OIIO_CHECK_NO_THROW(ReadResolveCube(SAMPLE));
}

OIIO_ADD_TEST(FileFormatResolveCube, ReadDefaultRange)
{
    const std::string SAMPLE_1D =
        "LUT_1D_SIZE 2\n"

        "0.0 0.0 0.0\n"
        "1.0 0.0 0.0\n";

    OIIO_CHECK_NO_THROW(ReadResolveCube(SAMPLE_1D));
    
    const std::string SAMPLE_3D =
        "LUT_3D_SIZE 2\n"

        "0.0 0.0 0.0\n"
        "1.0 0.0 0.0\n"
        "0.0 1.0 0.0\n"
        "1.0 1.0 0.0\n"
        "0.0 0.0 1.0\n"
        "1.0 0.0 1.0\n"
        "0.0 1.0 1.0\n"
        "1.0 1.0 1.0\n";

    OIIO_CHECK_NO_THROW(ReadResolveCube(SAMPLE_3D));
    
    const std::string SAMPLE_1D3D =
        "LUT_1D_SIZE 2\n"
        "LUT_3D_SIZE 2\n"
        
        "0.0 0.0 0.0\n"
        "1.0 1.0 1.0\n"
        "0.0 0.0 0.0\n"
        "1.0 0.0 0.0\n"
        "0.0 1.0 0.0\n"
        "1.0 1.0 0.0\n"
        "0.0 0.0 1.0\n"
        "1.0 0.0 1.0\n"
        "0.0 1.0 1.0\n"
        "1.0 1.0 1.0\n";

    OIIO_CHECK_NO_THROW(ReadResolveCube(SAMPLE_1D3D));
}


OIIO_ADD_TEST(FileFormatResolveCube, ReadFailure)
{
    {
        // Wrong LUT_3D_SIZE tag
        const std::string SAMPLE_ERROR =
            "LUT_3D_SIZE 2 2\n"
            "LUT_3D_INPUT_RANGE 0.0 1.0\n"

            "0.0 0.0 0.0\n"
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "1.0 1.0 0.0\n"
            "0.0 0.0 1.0\n"
            "1.0 0.0 1.0\n"
            "0.0 1.0 1.0\n"
            "1.0 1.0 1.0\n";

        OIIO_CHECK_THROW(ReadResolveCube(SAMPLE_ERROR), OCIO::Exception);
    }
    {
        // Wrong LUT_3D_INPUT_RANGE tag
        const std::string SAMPLE_ERROR =
            "LUT_3D_SIZE 2\n"
            "LUT_3D_INPUT_RANGE 0.0 1.0 2.0\n"

            "0.0 0.0 0.0\n"
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "1.0 1.0 0.0\n"
            "0.0 0.0 1.0\n"
            "1.0 0.0 1.0\n"
            "0.0 1.0 1.0\n"
            "1.0 1.0 1.0\n";

        OIIO_CHECK_THROW(ReadResolveCube(SAMPLE_ERROR), OCIO::Exception);
    }
    {
        // Comment after header
        const std::string SAMPLE_ERROR =
            "LUT_3D_SIZE 2\n"
            "LUT_3D_INPUT_RANGE 0.0 1.0 2.0\n"
            "# Malformed comment\n"
            "0.0 0.0 0.0\n"
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "1.0 1.0 0.0\n"
            "0.0 0.0 1.0\n"
            "1.0 0.0 1.0\n"
            "0.0 1.0 1.0\n"
            "1.0 1.0 1.0\n";

        OIIO_CHECK_THROW(ReadResolveCube(SAMPLE_ERROR), OCIO::Exception);
    }
    {
        // Unexpected tag
        const std::string SAMPLE_ERROR =
            "LUT_3D_SIZE 2\n"
            "LUT_3D_INPUT_RANGE 0.0 1.0 2.0\n"
            "WRONG_TAG\n"
            "0.0 0.0 0.0\n"
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "1.0 1.0 0.0\n"
            "0.0 0.0 1.0\n"
            "1.0 0.0 1.0\n"
            "0.0 1.0 1.0\n"
            "1.0 1.0 1.0\n";

        OIIO_CHECK_THROW(ReadResolveCube(SAMPLE_ERROR), OCIO::Exception);
    }
    {
        // Wrong number of entries
        const std::string SAMPLE_ERROR =
            "LUT_3D_SIZE 2\n"
            "LUT_3D_INPUT_RANGE 0.0 1.0 2.0\n"

            "0.0 0.0 0.0\n"
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "1.0 1.0 0.0\n"
            "0.0 0.0 1.0\n"
            "1.0 0.0 1.0\n"
            "0.0 1.0 1.0\n"
            "0.0 1.0 1.0\n"
            "0.0 1.0 1.0\n"
            "1.0 1.0 1.0\n";

        OIIO_CHECK_THROW(ReadResolveCube(SAMPLE_ERROR), OCIO::Exception);
    }
}

OIIO_ADD_TEST(FileFormatResolveCube, Bake1D)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("input");
        cs->setFamily("input");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("target");
        cs->setFamily("target");
        config->addColorSpace(cs);
    }
    
    std::ostringstream bout;
    bout << "# "                                                    << "\n";
    bout << ""                                                      << "\n";
    bout << "LUT_1D_SIZE 2"                                         << "\n";
    bout << "0.000000 0.000000 0.000000"                            << "\n";
    bout << "1.000000 1.000000 1.000000"                            << "\n";
    
    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->setFormat("resolve_cube");
    baker->setInputSpace("input");
    baker->setTargetSpace("target");
    baker->setCubeSize(2);
    std::ostringstream output;
    baker->bake(output);
    
    //
    std::vector<std::string> osvec;
    OCIO::pystring::splitlines(output.str(), osvec);
    std::vector<std::string> resvec;
    OCIO::pystring::splitlines(bout.str(), resvec);
    OIIO_CHECK_EQUAL(osvec.size(), resvec.size());
    for(unsigned int i = 0; i < resvec.size(); ++i)
    {
        OIIO_CHECK_EQUAL(osvec[i], resvec[i]);
    }
}

OIIO_ADD_TEST(FileFormatResolveCube, Bake3D)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("input");
        cs->setFamily("input");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("target");
        cs->setFamily("target");
        
        // Set saturation to cause channel crosstalk, making a 3D LUT
        OCIO::CDLTransformRcPtr transform1 = OCIO::CDLTransform::Create();
        transform1->setSat(0.5f);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
        
        config->addColorSpace(cs);
    }
    
    std::ostringstream bout;
    bout << "# OpenColorIO Test Line 1"                             << "\n";
    bout << "# OpenColorIO Test Line 2"                             << "\n";
    bout << ""                                                      << "\n";
    bout << "LUT_3D_SIZE 2"                                         << "\n";
    bout << "0.000000 0.000000 0.000000"                            << "\n";
    bout << "0.606300 0.106300 0.106300"                            << "\n";
    bout << "0.357600 0.857600 0.357600"                            << "\n";
    bout << "0.963900 0.963900 0.463900"                            << "\n";
    bout << "0.036100 0.036100 0.536100"                            << "\n";
    bout << "0.642400 0.142400 0.642400"                            << "\n";
    bout << "0.393700 0.893700 0.893700"                            << "\n";
    bout << "1.000000 1.000000 1.000000"                            << "\n";
    
    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    std::ostringstream metadata;
    metadata << "OpenColorIO Test Line 1\n";
    metadata << "OpenColorIO Test Line 2\n";
    baker->setMetadata(metadata.str().c_str());
    baker->setFormat("resolve_cube");
    baker->setInputSpace("input");
    baker->setTargetSpace("target");
    baker->setCubeSize(2);
    std::ostringstream output;
    baker->bake(output);
    
    //
    std::vector<std::string> osvec;
    OCIO::pystring::splitlines(output.str(), osvec);
    std::vector<std::string> resvec;
    OCIO::pystring::splitlines(bout.str(), resvec);
    OIIO_CHECK_EQUAL(osvec.size(), resvec.size());
    for(unsigned int i = 0; i < resvec.size(); ++i)
    {
        OIIO_CHECK_EQUAL(osvec[i], resvec[i]);
    }
}

OIIO_ADD_TEST(FileFormatResolveCube, Bake1D3D)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("input");
        cs->setFamily("input");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("shaper");
        cs->setFamily("shaper");
        OCIO::ExponentTransformRcPtr transform1 = OCIO::ExponentTransform::Create();
        float test[4] = {2.2f, 2.2f, 2.2f, 1.0f};
        transform1->setValue(test);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_TO_REFERENCE);
        config->addColorSpace(cs);
    }
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("target");
        cs->setFamily("target");
        
        // Set saturation to cause channel crosstalk, making a 3D LUT
        OCIO::CDLTransformRcPtr transform1 = OCIO::CDLTransform::Create();
        transform1->setSat(0.5f);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
        
        config->addColorSpace(cs);
    }
    
    std::ostringstream bout;
    bout << "# "                                                    << "\n";
    bout << ""                                                      << "\n";
    bout << "LUT_1D_SIZE 10"                                        << "\n";
    bout << "LUT_1D_INPUT_RANGE 0.000000 1.000000"                  << "\n";
    bout << "LUT_3D_SIZE 2"                                         << "\n";
    bout << "0.000000 0.000000 0.000000"                            << "\n";
    bout << "0.368344 0.368344 0.368344"                            << "\n";
    bout << "0.504760 0.504760 0.504760"                            << "\n";
    bout << "0.606913 0.606913 0.606913"                            << "\n";
    bout << "0.691699 0.691699 0.691699"                            << "\n";
    bout << "0.765539 0.765539 0.765539"                            << "\n";
    bout << "0.831684 0.831684 0.831684"                            << "\n";
    bout << "0.892049 0.892049 0.892049"                            << "\n";
    bout << "0.947870 0.947870 0.947870"                            << "\n";
    bout << "1.000000 1.000000 1.000000"                            << "\n";
    bout << "0.000000 0.000000 0.000000"                            << "\n";
    bout << "0.606300 0.106300 0.106300"                            << "\n";
    bout << "0.357600 0.857600 0.357600"                            << "\n";
    bout << "0.963900 0.963900 0.463900"                            << "\n";
    bout << "0.036100 0.036100 0.536100"                            << "\n";
    bout << "0.642400 0.142400 0.642400"                            << "\n";
    bout << "0.393700 0.893700 0.893700"                            << "\n";
    bout << "1.000000 1.000000 1.000000"                            << "\n";
    
    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->setFormat("resolve_cube");
    baker->setInputSpace("input");
    baker->setShaperSpace("shaper");
    baker->setTargetSpace("target");
    baker->setShaperSize(10);
    baker->setCubeSize(2);
    std::ostringstream output;
    baker->bake(output);
    
    //
    std::vector<std::string> osvec;
    OCIO::pystring::splitlines(output.str(), osvec);
    std::vector<std::string> resvec;
    OCIO::pystring::splitlines(bout.str(), resvec);
    OIIO_CHECK_EQUAL(osvec.size(), resvec.size());
    for(unsigned int i = 0; i < resvec.size(); ++i)
    {
        OIIO_CHECK_EQUAL(osvec[i], resvec[i]);
    }
}

#endif // OCIO_UNIT_TEST

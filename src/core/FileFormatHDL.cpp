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

/*
    
    Houdini LUTs
    http://www.sidefx.com/docs/hdk11.0/hdk_io_lut.html
    
    Types:
      - 1D Lut (partial support)
      - 3D Lut
      - 3D Lut with 1D Prelut
    
    TODO:
        - Add support for other 1D types (R, G, B, A, RGB, RGBA, All)
          we only support type 'C' atm.
        - Add support for 'Sampling' tag
    
*/

#include <cstdio>
#include <iostream>
#include <iterator>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <map>

#include <OpenColorIO/OpenColorIO.h>

#include "FileTransform.h"
#include "Lut1DOp.h"
#include "Lut3DOp.h"
#include "ParseUtils.h"
#include "MathUtils.h"
#include "pystring/pystring.h"

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        // HDL parser helpers

        // HDL headers/LUT's are shoved into these datatypes
        typedef std::map<std::string, std::vector<std::string> > StringToStringVecMap;
        typedef std::map<std::string, std::vector<float> > StringToFloatVecMap;

        void
        readHeaders(StringToStringVecMap& headers,
                    std::istream& istream)
        {
            std::string line;
            while(nextline(istream, line))
            {
                std::vector<std::string> chunks;

                // Remove trailing/leading whitespace, lower-case and
                // split into words
                pystring::split(pystring::lower(pystring::strip(line)), chunks);

                // Skip empty lines
                if(chunks.empty()) continue;

                // Stop looking for headers at the "LUT:" line
                if(chunks[0] == "lut:") break;

                // Use first index as key, and remove it from the value
                std::string key = chunks[0];
                chunks.erase(chunks.begin());

                headers[key] = chunks;
            }
        }

        // Try to grab key (e.g "version") from headers. Throws
        // exception if not found, or if number of chunks in value is
        // not between min_vals and max_vals (e.g the "length" key
        // must exist, and must have either 1 or 2 values)
        std::vector<std::string>
        findHeaderItem(StringToStringVecMap& headers,
                       const std::string key,
                       const unsigned int min_vals,
                       const unsigned int max_vals)
        {
            StringToStringVecMap::iterator iter;
            iter = headers.find(key);

            // Error if key is not found
            if(iter == headers.end())
            {
                std::ostringstream os;
                os << "'" << key << "' line not found";
                throw Exception(os.str().c_str());
            }

            // Error if incorrect number of values is found
            if(iter->second.size() < min_vals ||
               iter->second.size() > max_vals)
            {
                std::ostringstream os;
                os << "Incorrect number of chunks (" << iter->second.size() << ")";
                os << " after '" << key << "' line, expected ";

                if(min_vals == max_vals)
                {
                    os << min_vals;
                }
                else
                {
                    os << "between " << min_vals << " and " << max_vals;
                }

                throw Exception(os.str().c_str());
            }

            return iter->second;
        }

        // Simple wrapper to call findHeaderItem with a fixed number
        // of values (e.g "version" should have a single value)
        std::vector<std::string>
        findHeaderItem(StringToStringVecMap& chunks,
                       const std::string key,
                       const unsigned int numvals)
        {
            return findHeaderItem(chunks, key, numvals, numvals);
        }

        // Crudely parse LUT's - doesn't do any length checking etc,
        // just grabs a series of floats for Pre{...}, 3d{...} etc
        // Does some basic error checking, but there are situations
        // were it could incorrectly accept broken data (like
        // "Pre{0.0\n1.0}blah"), but hopefully none where it misses
        // data
        void
        readLuts(std::istream& istream,
                 StringToFloatVecMap& lutValues)
        {
            // State variables
            bool inlut = false;
            std::string lutname;

            std::string word;

            while(istream >> word)
            {
                if(!inlut)
                {
                    if(word == "{")
                    {
                        // Lone "{" is for a 3D
                        inlut = true;
                        lutname = "3d";
                    }
                    else
                    {
                        // Named lut, e.g "Pre {"
                        inlut = true;
                        lutname = pystring::lower(word);

                        // Ensure next word is "{"
                        std::string nextword;
                        istream >> nextword;
                        if(nextword != "{")
                        {
                            std::ostringstream os;
                            os << "Malformed LUT - Unknown word '";
                            os << word << "' after LUT name '";
                            os << nextword << "'";
                            throw Exception(os.str().c_str());
                        }
                    }
                }
                else if(word == "}")
                {
                    // end of LUT
                    inlut = false;
                    lutname = "";
                }
                else if(inlut)
                {
                    // StringToFloat was far slower, for 787456 values:
                    // - StringToFloat took 3879 (avg nanoseconds per value)
                    // - stdtod took 169 nanoseconds
                    char* endptr = 0;
                    float v = static_cast<float>(strtod(word.c_str(), &endptr));

                    if(!*endptr)
                    {
                        // Since each word should contain a single
                        // float value, the pointer should be null
                        lutValues[lutname].push_back(v);
                    }
                    else
                    {
                        // stdtod endptr still contained stuff,
                        // meaning an invalid float value
                        std::ostringstream os;
                        os << "Invalid float value in " << lutname;
                        os << " LUT, '" << word << "'";
                        throw Exception(os.str().c_str());
                    }
                }
                else
                {
                    std::ostringstream os;
                    os << "Unexpected word, possibly a value outside";
                    os <<" a LUT {} block. Word was '" << word << "'";
                    throw Exception(os.str().c_str());

                }
            }
        }

    } // end anonymous "HDL parser helpers" namespace

    namespace
    {
        class CachedFileHDL : public CachedFile
        {
        public:
            CachedFileHDL ()
            {
                hdlversion = "unknown";
                hdlformat = "unknown";
                hdltype = "unknown";
                hdlblack = 0.0;
                hdlwhite = 1.0;
                lut1D = Lut1D::Create();
                lut3D = Lut3D::Create();
            };
            ~CachedFileHDL() {};
            std::string hdlversion;
            std::string hdlformat;
            std::string hdltype;
            float to_min; // TODO: maybe add this to Lut1DOp?
            float to_max; // TODO: maybe add this to Lut1DOp?
            float hdlblack;
            float hdlwhite;
            Lut1DRcPtr lut1D;
            Lut3DRcPtr lut3D;
        };
        typedef OCIO_SHARED_PTR<CachedFileHDL> CachedFileHDLRcPtr;
        
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
            info.name = "houdini";
            info.extension = "lut";
            info.capabilities = FORMAT_CAPABILITY_READ | FORMAT_CAPABILITY_WRITE;
            formatInfoVec.push_back(info);
        }
        
        CachedFileRcPtr
        LocalFileFormat::Read(std::istream & istream) const
        {
            
            // this shouldn't happen
            if (!istream)
                throw Exception ("file stream empty when trying to read Houdini lut");

            //
            CachedFileHDLRcPtr cachedFile = CachedFileHDLRcPtr (new CachedFileHDL ());
            Lut1DRcPtr lut1d_ptr = Lut1D::Create();
            Lut3DRcPtr lut3d_ptr = Lut3D::Create();

            // Parse headers into key-value pairs
            StringToStringVecMap header_chunks;
            StringToStringVecMap::iterator iter;

            // Read headers, ending after the "LUT:" line
            readHeaders(header_chunks, istream);

            // Grab useful values from headers
            std::vector<std::string> value;

            // "Version 3" - format version (currently one version
            // number per LUT type)
            value = findHeaderItem(header_chunks, "version", 1);
            cachedFile->hdlversion = value[0];

            // "Format any" - bit depth of image the LUT should be
            // applied to (this is basically ignored)
            value = findHeaderItem(header_chunks, "format", 1);
            cachedFile->hdlformat = value[0];

            // "Type 3d" - type of LUT
            {
                value = findHeaderItem(header_chunks, "type", 1);

                cachedFile->hdltype = value[0];
            }

            // "From 0.0 1.0" - range of input values
            {
                float from_min, from_max;

                value = findHeaderItem(header_chunks, "from", 2);

                if(!StringToFloat(&from_min, value[0].c_str()) ||
                   !StringToFloat(&from_max, value[1].c_str()))
                {
                    std::ostringstream os;
                    os << "Invalid float value(s) on 'From' line, '";
                    os << value[0] << "' and '"  << value[1] << "'";
                    throw Exception(os.str().c_str());
                }

                for(int i = 0; i < 3; ++i)
                {
                    lut1d_ptr->from_min[i] = from_min;
                    lut1d_ptr->from_max[i] = from_max;
                }
            }


            // "To 0.0 1.0" - range of values in LUT (e.g "0 255"
            // to specify values as 8-bit numbers, usually "0 1")
            {
                float to_min, to_max;

                value = findHeaderItem(header_chunks, "to", 2);

                if(!StringToFloat(&to_min, value[0].c_str()) ||
                   !StringToFloat(&to_max, value[1].c_str()))
                {
                    std::ostringstream os;
                    os << "Invalid float value(s) on 'To' line, '";
                    os << value[0] << "' and '"  << value[1] << "'";
                    throw Exception(os.str().c_str());
                }
                cachedFile->to_min = to_min;
                cachedFile->to_max = to_max;
            }

            // "Black 0" and "White 1" - obsolete options, should be 0
            // and 1

            {
                value = findHeaderItem(header_chunks, "black", 1);

                float black;

                if(!StringToFloat(&black, value[0].c_str()))
                {
                    std::ostringstream os;
                    os << "Invalid float value on 'Black' line, '";
                    os << value[0] << "'";
                    throw Exception(os.str().c_str());
                }
                cachedFile->hdlblack = black;
            }

            {
                value = findHeaderItem(header_chunks, "white", 1);

                float white;

                if(!StringToFloat(&white, value[0].c_str()))
                {
                    std::ostringstream os;
                    os << "Invalid float value on 'White' line, '";
                    os << value[0] << "'";
                    throw Exception(os.str().c_str());
                }
                cachedFile->hdlwhite = white;
            }


            // Verify type is valid and supported - used to handle
            // length sensibly, and checking the LUT later
            {
                std::string ltype = cachedFile->hdltype;
                if(ltype != "3d" && ltype != "3d+1d" && ltype != "c")
                {
                    std::ostringstream os;
                    os << "Unsupported Houdini LUT type: '" << ltype << "'";
                    throw Exception(os.str().c_str());
                }
            }


            // "Length 2" or "Length 2 5" - either "[cube size]", or "[cube
            // size] [prelut size]"
            int size_3d = -1;
            int size_prelut = -1;
            int size_1d = -1;

            {
                std::vector<int> lut_sizes;

                value = findHeaderItem(header_chunks, "length", 1, 2);
                for(unsigned int i = 0; i < value.size(); ++i)
                {
                    int tmpsize = -1;
                    if(!StringToInt(&tmpsize, value[i].c_str()))
                    {
                        std::ostringstream os;
                        os << "Invalid integer on 'Length' line: ";
                        os << "'" << value[0] << "'";
                        throw Exception(os.str().c_str());
                    }
                    lut_sizes.push_back(tmpsize);
                }

                if(cachedFile->hdltype == "3d" || cachedFile->hdltype == "3d+1d")
                {
                    // Set cube size
                    size_3d = lut_sizes[0];

                    lut3d_ptr->size[0] = lut_sizes[0];
                    lut3d_ptr->size[1] = lut_sizes[0];
                    lut3d_ptr->size[2] = lut_sizes[0];
                }

                if(cachedFile->hdltype == "c")
                {
                    size_1d = lut_sizes[0];
                }

                if(cachedFile->hdltype == "3d+1d")
                {
                    size_prelut = lut_sizes[1];
                }
            }

            // Read stuff after "LUT:"
            StringToFloatVecMap lut_data;
            readLuts(istream, lut_data);

            //
            StringToFloatVecMap::iterator lut_iter;

            if(cachedFile->hdltype == "3d+1d")
            {
                // Read prelut, and bind onto cachedFile
                lut_iter = lut_data.find("pre");
                if(lut_iter == lut_data.end())
                {
                    std::ostringstream os;
                    os << "3D+1D LUT should contain Pre{} LUT section";
                    throw Exception(os.str().c_str());
                }

                if(size_prelut != static_cast<int>(lut_iter->second.size()))
                {
                    std::ostringstream os;
                    os << "Pre{} LUT was " << lut_iter->second.size();
                    os << " values long, expected " << size_prelut << " values";
                    throw Exception(os.str().c_str());
                }

                lut1d_ptr->luts[0] = lut_iter->second;
                lut1d_ptr->luts[1] = lut_iter->second;
                lut1d_ptr->luts[2] = lut_iter->second;
                lut1d_ptr->maxerror = 0.0f;
                lut1d_ptr->errortype = ERROR_RELATIVE;
                cachedFile->lut1D = lut1d_ptr;
            }

            if(cachedFile->hdltype == "3d" ||
               cachedFile->hdltype == "3d+1d")
            {
                // Bind 3D LUT to lut3d_ptr, along with some
                // slightly-elabourate error messages

                lut_iter = lut_data.find("3d");
                if(lut_iter == lut_data.end())
                {
                    std::ostringstream os;
                    os << "3D LUT section not found";
                    throw Exception(os.str().c_str());
                }

                int size_3d_cubed = size_3d * size_3d * size_3d;

                if(size_3d_cubed * 3 != static_cast<int>(lut_iter->second.size()))
                {
                    int foundsize = static_cast<int>(lut_iter->second.size());
                    int foundlines = foundsize / 3;

                    std::ostringstream os;
                    os << "3D LUT contains incorrect number of values. ";
                    os << "Contained " << foundsize << " values ";
                    os << "(" << foundlines << " lines), ";
                    os << "expected " << (size_3d_cubed*3) << " values ";
                    os << "(" << size_3d_cubed << " lines)";
                    throw Exception(os.str().c_str());
                }

                lut3d_ptr->lut = lut_iter->second;

                // Bind to cachedFile
                cachedFile->lut3D = lut3d_ptr;
            }

            if(cachedFile->hdltype == "c")
            {
                // Bind simple 1D RGB LUT
                lut_iter = lut_data.find("rgb");
                if(lut_iter == lut_data.end())
                {
                    std::ostringstream os;
                    os << "3D+1D LUT should contain Pre{} LUT section";
                    throw Exception(os.str().c_str());
                }

                if(size_1d != static_cast<int>(lut_iter->second.size()))
                {
                    std::ostringstream os;
                    os << "RGB{} LUT was " << lut_iter->second.size();
                    os << " values long, expected " << size_1d << " values";
                    throw Exception(os.str().c_str());
                }

                lut1d_ptr->luts[0] = lut_iter->second;
                lut1d_ptr->luts[1] = lut_iter->second;
                lut1d_ptr->luts[2] = lut_iter->second;
                lut1d_ptr->maxerror = 0.0f;
                lut1d_ptr->errortype = ERROR_RELATIVE;
                cachedFile->lut1D = lut1d_ptr;
            }

            return cachedFile;
        }
        
        void LocalFileFormat::Write(const Baker & baker,
                                    const std::string & formatName,
                                    std::ostream & ostream) const
        {

            if(formatName != "houdini")
            {
                std::ostringstream os;
                os << "Unknown hdl format name, '";
                os << formatName << "'.";
                throw Exception(os.str().c_str());
            }

            // Get config
            ConstConfigRcPtr config = baker.getConfig();

            // setup the floating point precision
            ostream.setf(std::ios::fixed, std::ios::floatfield);
            ostream.precision(6);

            // Default sizes
            const int DEFAULT_SHAPER_SIZE = 1024;
            // MPlay produces bad results with 32^3 cube (in a way
            // that looks more quantised than even "nearest"
            // interpolation in OCIOFileTransform)
            const int DEFAULT_CUBE_SIZE = 64;
            const int DEFAULT_1D_SIZE = 1024;

            // Get configured sizes
            int cubeSize = baker.getCubeSize();
            int shaperSize = baker.getShaperSize();
            // FIXME: Misusing cube size to set 1D LUT size, as it seemed
            // slightly less confusing than using the shaper LUT size
            int onedSize = baker.getCubeSize();
            
            // Defaults and sanity check on cube size
            if(cubeSize == -1) cubeSize = DEFAULT_CUBE_SIZE;
            if(cubeSize < 0) cubeSize = DEFAULT_CUBE_SIZE;
            if(cubeSize<2)
            {
                std::ostringstream os;
                os << "Cube size must be 2 or larger (was " << cubeSize << ")";
                throw Exception(os.str().c_str());
            }
            
            // ..and same for shaper size
            if(shaperSize<0) shaperSize = DEFAULT_SHAPER_SIZE;
            if(shaperSize<2)
            {
                std::ostringstream os;
                os << "A shaper space ('" << baker.getShaperSpace() << "') has";
                os << " been specified, so the shaper size must be 2 or larger";
                throw Exception(os.str().c_str());
            }
            
            // ..and finally, for the 1D LUT size
            if(onedSize == -1) onedSize = DEFAULT_1D_SIZE;
            if(onedSize < 2)
            {
                std::ostringstream os;
                os << "1D LUT size must be higher than 2 (was " << onedSize << ")";
                throw Exception(os.str().c_str());
            }
            
            // Version numbers
            const int HDL_1D = 1; // 1D LUT version number
            const int HDL_3D = 2; // 3D LUT version number
            const int HDL_3D1D = 3; // 3D LUT with 1D prelut
            
            // Get spaces from baker
            const std::string shaperSpace = baker.getShaperSpace();
            const std::string inputSpace = baker.getInputSpace();
            const std::string targetSpace = baker.getTargetSpace();
            const std::string looks = baker.getLooks();

            // Determine required LUT type
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
                    // Has crosstalk, but no prelut, so need 3D LUT
                    required_lut = HDL_3D;
                }
                else
                {
                    // Crosstalk with shaper-space
                    required_lut = HDL_3D1D;
                }
            }
            else
            {
                // No crosstalk
                required_lut = HDL_1D;
            }

            if(required_lut == -1)
            {
                // Unnecessary paranoia
                throw Exception(
                    "Internal logic error, LUT type was not determined");
            }
            
            // Make prelut
            std::vector<float> prelutData;
            
            float fromInStart = 0; // for "From:" part of header
            float fromInEnd = 1;
            
            if(required_lut == HDL_3D1D)
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
                    // cube (e.g for a generic lin-to-log trasnform,
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

                // Generate the identity prelut values, then apply the transform.
                // Prelut is linearly sampled from fromInStart to fromInEnd
                prelutData.resize(shaperSize*3);
                
                for (int i = 0; i < shaperSize; ++i)
                {
                    const float x = (float)(double(i) / double(shaperSize - 1));
                    float cur_value = lerpf(fromInStart, fromInEnd, x);

                    prelutData[3*i+0] = cur_value;
                    prelutData[3*i+1] = cur_value;
                    prelutData[3*i+2] = cur_value;
                }
                
                PackedImageDesc prelutImg(&prelutData[0], shaperSize, 1, 3);
                inputToShaperProc->apply(prelutImg);
            }
            
            // TODO: Do same "auto prelut" input-space allocation as FileFormatCSP?
            
            // Make 3D LUT
            std::vector<float> cubeData;
            if(required_lut == HDL_3D || required_lut == HDL_3D1D)
            {
                cubeData.resize(cubeSize*cubeSize*cubeSize*3);
                
                GenerateIdentityLut3D(&cubeData[0], cubeSize, 3, LUT3DORDER_FAST_RED);
                PackedImageDesc cubeImg(&cubeData[0], cubeSize*cubeSize*cubeSize, 1, 3);
                
                ConstProcessorRcPtr cubeProc;
                if(required_lut == HDL_3D1D)
                {
                    // Prelut goes from input-to-shaper, so cube goes from shaper-to-target
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
                    // No prelut, so cube goes from input-to-target
                  cubeProc = inputToTargetProc;
                }

                
                cubeProc->apply(cubeImg);
            }
            
            
            // Make 1D LUT
            std::vector<float> onedData;
            if(required_lut == HDL_1D)
            {
                onedData.resize(onedSize * 3);
                
                GenerateIdentityLut1D(&onedData[0], onedSize, 3);
                PackedImageDesc onedImg(&onedData[0], onedSize, 1, 3);
                
                inputToTargetProc->apply(onedImg);
            }
            
            
            // Write the file contents
            ostream << "Version\t\t" << required_lut << "\n";
            ostream << "Format\t\t" << "any" << "\n";
            
            ostream << "Type\t\t";
            if(required_lut == HDL_1D)
                ostream << "RGB";
            if(required_lut == HDL_3D)
                ostream << "3D";
            if(required_lut == HDL_3D1D)
                ostream << "3D+1D";
            ostream << "\n";
            
            ostream << "From\t\t" << fromInStart << " " << fromInEnd << "\n";
            ostream << "To\t\t" << 0.0f << " " << 1.0f << "\n";
            ostream << "Black\t\t" << 0.0f << "\n";
            ostream << "White\t\t" << 1.0f << "\n";
            
            if(required_lut == HDL_3D1D)
                ostream << "Length\t\t" << cubeSize << " " << shaperSize << "\n";
            if(required_lut == HDL_3D)
                ostream << "Length\t\t" << cubeSize << "\n";
            if(required_lut == HDL_1D)
                ostream << "Length\t\t" << onedSize << "\n";
            
            ostream << "LUT:\n";
            
            // Write prelut
            if(required_lut == HDL_3D1D)
            {
                ostream << "Pre {\n";
                for(int i=0; i < shaperSize; ++i)
                {
                    // Grab green channel from RGB prelut
                    ostream << "\t" << prelutData[i*3+1] << "\n";
                }
                ostream << "}\n";
            }
            
            // Write "3D {" part of output of 3D+1D LUT
            if(required_lut == HDL_3D1D)
            {
                ostream << "3D {\n";
            }
            
            // Write the slightly-different "{" without line for the 3D-only LUT
            if(required_lut == HDL_3D)
            {
                ostream << " {\n";
            }
            
            // Write the cube data after the "{"
            if(required_lut == HDL_3D || required_lut == HDL_3D1D)
            {
                for(int i=0; i < cubeSize*cubeSize*cubeSize; ++i)
                {
                    // TODO: Original baker code clamped values to
                    // 1.0, was this necessary/desirable?

                    ostream << "\t" << cubeData[3*i+0];
                    ostream << " "  << cubeData[3*i+1];
                    ostream << " "  << cubeData[3*i+2] << "\n";
                }
                
                // Write closing "}"
                ostream << " }\n";
            }
            
            // Write out channels for 1D LUT
            if(required_lut == HDL_1D)
            {
                ostream << "R {\n";
                for(int i=0; i < onedSize; ++i)
                    ostream << "\t" << onedData[i*3+0] << "\n";
                ostream << "}\n";

                ostream << "G {\n";
                for(int i=0; i < onedSize; ++i)
                    ostream << "\t" << onedData[i*3+1] << "\n";
                ostream << "}\n";

                ostream << "B {\n";
                for(int i=0; i < onedSize; ++i)
                    ostream << "\t" << onedData[i*3+2] << "\n";
                ostream << "}\n";
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
            
            CachedFileHDLRcPtr cachedFile = DynamicPtrCast<CachedFileHDL>(untypedCachedFile);
            
            // This should never happen.
            if(!cachedFile)
            {
                std::ostringstream os;
                os << "Cannot build Houdini Op. Invalid cache type.";
                throw Exception(os.str().c_str());
            }
            
            TransformDirection newDir = CombineTransformDirections(dir,
                fileTransform.getDirection());
            
            if(newDir == TRANSFORM_DIR_FORWARD) {
                if(cachedFile->hdltype == "c")
                {
                    CreateLut1DOp(ops, cachedFile->lut1D,
                                  fileTransform.getInterpolation(), newDir);
                }
                else if(cachedFile->hdltype == "3d")
                {
                    CreateLut3DOp(ops, cachedFile->lut3D,
                                  fileTransform.getInterpolation(), newDir);
                }
                else if(cachedFile->hdltype == "3d+1d")
                {
                    CreateLut1DOp(ops, cachedFile->lut1D,
                                  fileTransform.getInterpolation(), newDir);
                    CreateLut3DOp(ops, cachedFile->lut3D,
                                  fileTransform.getInterpolation(), newDir);
                }
                else
                {
                    throw Exception("Unhandled hdltype while creating forward ops");
                }
            } else if(newDir == TRANSFORM_DIR_INVERSE) {
                if(cachedFile->hdltype == "c")
                {
                    CreateLut1DOp(ops, cachedFile->lut1D,
                                  fileTransform.getInterpolation(), newDir);
                }
                else if(cachedFile->hdltype == "3d")
                {
                    CreateLut3DOp(ops, cachedFile->lut3D,
                                  fileTransform.getInterpolation(), newDir);
                }
                else if(cachedFile->hdltype == "3d+1d")
                {
                    CreateLut3DOp(ops, cachedFile->lut3D,
                                  fileTransform.getInterpolation(), newDir);
                    CreateLut1DOp(ops, cachedFile->lut1D,
                                  fileTransform.getInterpolation(), newDir);
                }
                else
                {
                    throw Exception("Unhandled hdltype while creating reverse ops");
                }
            }
            return;
        }
    }
    
    FileFormat * CreateFileFormatHDL()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

OIIO_ADD_TEST(FileFormatHDL, Read1D)
{
    std::ostringstream strebuf;
    strebuf << "Version\t\t1" << "\n";
    strebuf << "Format\t\tany" << "\n";
    strebuf << "Type\t\tC" << "\n";
    strebuf << "From\t\t0.1 3.2" << "\n";
    strebuf << "To\t\t0 1" << "\n";
    strebuf << "Black\t\t0" << "\n";
    strebuf << "White\t\t0.99" << "\n";
    strebuf << "Length\t\t9" << "\n";
    strebuf << "LUT:" << "\n";
    strebuf << "RGB {" << "\n";
    strebuf << "\t0" << "\n";
    strebuf << "\t0.000977517" << "\n";
    strebuf << "\t0.00195503" << "\n";
    strebuf << "\t0.00293255" << "\n";
    strebuf << "\t0.00391007" << "\n";
    strebuf << "\t0.00488759" << "\n";
    strebuf << "\t0.0058651" << "\n";
    strebuf << "\t0.999022" << "\n";
    strebuf << "\t1.67 }" << "\n";
    
    //
    float from_min = 0.1f;
    float from_max = 3.2f;
    float to_min = 0.0f;
    float to_max = 1.0f;
    float black = 0.0f;
    float white = 0.99f;
    float lut1d[9] = { 0.0f, 0.000977517f, 0.00195503f, 0.00293255f,
        0.00391007f, 0.00488759f, 0.0058651f, 0.999022f, 1.67f };
    
    std::istringstream simple3D1D;
    simple3D1D.str(strebuf.str());
    
    // Load file
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile = tester.Read(simple3D1D);
    OCIO::CachedFileHDLRcPtr lut = OCIO::DynamicPtrCast<OCIO::CachedFileHDL>(cachedFile);
    
    //
    OIIO_CHECK_EQUAL(to_min, lut->to_min);
    OIIO_CHECK_EQUAL(to_max, lut->to_max);
    OIIO_CHECK_EQUAL(black, lut->hdlblack);
    OIIO_CHECK_EQUAL(white, lut->hdlwhite);
    
    // check 1D data (each channel has the same data)
    for(int c = 0; c < 3; ++c) {
        OIIO_CHECK_EQUAL(from_min, lut->lut1D->from_min[c]);
        OIIO_CHECK_EQUAL(from_max, lut->lut1D->from_max[c]);

        OIIO_CHECK_EQUAL(9, lut->lut1D->luts[c].size());

        for(unsigned int i = 0; i < lut->lut1D->luts[c].size(); ++i) {
            OIIO_CHECK_EQUAL(lut1d[i], lut->lut1D->luts[c][i]);
        }
    }
}

OIIO_ADD_TEST(FileFormatHDL, Bake1D)
{
    
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    
    // Add lnf space
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("lnf");
        cs->setFamily("lnf");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }
    
    // Add target space
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("target");
        cs->setFamily("target");
        OCIO::CDLTransformRcPtr transform1 = OCIO::CDLTransform::Create();
        
        float rgb[3] = {0.1f, 0.1f, 0.1f};
        transform1->setOffset(rgb);
        
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
        config->addColorSpace(cs);
    }
        
    std::string bout =
    "Version\t\t1\n"
    "Format\t\tany\n"
    "Type\t\tRGB\n"
    "From\t\t0.000000 1.000000\n"
    "To\t\t0.000000 1.000000\n"
    "Black\t\t0.000000\n"
    "White\t\t1.000000\n"
    "Length\t\t10\n"
    "LUT:\n"
    "R {\n"
    "\t0.100000\n"
    "\t0.211111\n"
    "\t0.322222\n"
    "\t0.433333\n"
    "\t0.544444\n"
    "\t0.655556\n"
    "\t0.766667\n"
    "\t0.877778\n"
    "\t0.988889\n"
    "\t1.100000\n"
    " }\n"
    "G {\n"
    "\t0.100000\n"
    "\t0.211111\n"
    "\t0.322222\n"
    "\t0.433333\n"
    "\t0.544444\n"
    "\t0.655556\n"
    "\t0.766667\n"
    "\t0.877778\n"
    "\t0.988889\n"
    "\t1.100000\n"
    " }\n"
    "B {\n"
    "\t0.100000\n"
    "\t0.211111\n"
    "\t0.322222\n"
    "\t0.433333\n"
    "\t0.544444\n"
    "\t0.655556\n"
    "\t0.766667\n"
    "\t0.877778\n"
    "\t0.988889\n"
    "\t1.100000\n"
    " }\n";
    
    //
    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->setFormat("houdini");
    baker->setInputSpace("lnf");
    baker->setTargetSpace("target");
    baker->setCubeSize(10); // FIXME: Misusing the cube size to set the 1D LUT size
    std::ostringstream output;
    baker->bake(output);
    
    //std::cerr << "The LUT: " << std::endl << output.str() << std::endl;
    //std::cerr << "Expected:" << std::endl << bout << std::endl;
    
    //
    std::vector<std::string> osvec;
    OCIO::pystring::splitlines(output.str(), osvec);
    std::vector<std::string> resvec;
    OCIO::pystring::splitlines(bout, resvec);
    OIIO_CHECK_EQUAL(osvec.size(), resvec.size());
    for(unsigned int i = 0; i < std::min(osvec.size(), resvec.size()); ++i)
        OIIO_CHECK_EQUAL(OCIO::pystring::strip(osvec[i]), OCIO::pystring::strip(resvec[i]));
    
}

OIIO_ADD_TEST(FileFormatHDL, Read3D)
{
    std::ostringstream strebuf;
    strebuf << "Version         2" << "\n";
    strebuf << "Format      any" << "\n";
    strebuf << "Type        3D" << "\n";
    strebuf << "From        0.2 0.9" << "\n";
    strebuf << "To      0.001 0.999" << "\n";
    strebuf << "Black       0.002" << "\n";
    strebuf << "White       0.98" << "\n";
    strebuf << "Length      2" << "\n";
    strebuf << "LUT:" << "\n";
    strebuf << " {" << "\n";
    strebuf << " 0 0 0" << "\n";
    strebuf << " 0 0 0" << "\n";
    strebuf << " 0 0.390735 2.68116e-28" << "\n";
    strebuf << " 0 0.390735 0" << "\n";
    strebuf << " 0 0 0" << "\n";
    strebuf << " 0 0 0.599397" << "\n";
    strebuf << " 0 0.601016 0" << "\n";
    strebuf << " 0 0.601016 0.917034" << "\n";
    strebuf << " }" << "\n";
    
    std::istringstream simple3D1D;
    simple3D1D.str(strebuf.str());
    // Load file
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile = tester.Read(simple3D1D);
    OCIO::CachedFileHDLRcPtr lut = OCIO::DynamicPtrCast<OCIO::CachedFileHDL>(cachedFile);
    
    //
    //float from_min = 0.2;
    //float from_max = 0.9;
    float to_min = 0.001f;
    float to_max = 0.999f;
    float black = 0.002f;
    float white = 0.98f;
    float cube[2 * 2 * 2 * 3 ] = {
        0.f, 0.f, 0.f,
        0.f, 0.f, 0.f,
        0.f, 0.390735f, 2.68116e-28f,
        0.f, 0.390735f, 0.f,
        0.f, 0.f, 0.f,
        0.f, 0.f, 0.599397f,
        0.f, 0.601016f, 0.f,
        0.f, 0.601016f, 0.917034f };
    
    //
    OIIO_CHECK_EQUAL(to_min, lut->to_min);
    OIIO_CHECK_EQUAL(to_max, lut->to_max);
    OIIO_CHECK_EQUAL(black, lut->hdlblack);
    OIIO_CHECK_EQUAL(white, lut->hdlwhite);
    
    // check cube data
    OIIO_CHECK_EQUAL(2*2*2*3, lut->lut3D->lut.size());

    for(unsigned int i = 0; i < lut->lut3D->lut.size(); ++i) {
        OIIO_CHECK_EQUAL(cube[i], lut->lut3D->lut[i]);
    }
}

OIIO_ADD_TEST(FileFormatHDL, Bake3D)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    
    // Set luma coef's to simple values
    {
        float lumaCoef[3] = {0.333f, 0.333f, 0.333f};
        config->setDefaultLumaCoefs(lumaCoef);
    }
    
    // Add lnf space
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("lnf");
        cs->setFamily("lnf");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }
    
    // Add target space
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("target");
        cs->setFamily("target");
        OCIO::CDLTransformRcPtr transform1 = OCIO::CDLTransform::Create();
        
        // Set saturation to cause channel crosstalk, making a 3D LUT
        transform1->setSat(0.5f);

        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
        config->addColorSpace(cs);
    }
        
    std::string bout = 
    "Version\t\t2\n"
    "Format\t\tany\n"
    "Type\t\t3D\n"
    "From\t\t0.000000 1.000000\n"
    "To\t\t0.000000 1.000000\n"
    "Black\t\t0.000000\n"
    "White\t\t1.000000\n"
    "Length\t\t2\n"
    "LUT:\n"
    " {\n"
    "\t0.000000 0.000000 0.000000\n"
    "\t0.606300 0.106300 0.106300\n"
    "\t0.357600 0.857600 0.357600\n"
    "\t0.963900 0.963900 0.463900\n"
    "\t0.036100 0.036100 0.536100\n"
    "\t0.642400 0.142400 0.642400\n"
    "\t0.393700 0.893700 0.893700\n"
    "\t1.000000 1.000000 1.000000\n"
    " }\n";
    
    //
    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->setFormat("houdini");
    baker->setInputSpace("lnf");
    baker->setTargetSpace("target");
    baker->setCubeSize(2);
    std::ostringstream output;
    baker->bake(output);
    
    //std::cerr << "The LUT: " << std::endl << output.str() << std::endl;
    //std::cerr << "Expected:" << std::endl << bout << std::endl;
    
    //
    std::vector<std::string> osvec;
    OCIO::pystring::splitlines(output.str(), osvec);
    std::vector<std::string> resvec;
    OCIO::pystring::splitlines(bout, resvec);
    OIIO_CHECK_EQUAL(osvec.size(), resvec.size());
    for(unsigned int i = 0; i < std::min(osvec.size(), resvec.size()); ++i)
        OIIO_CHECK_EQUAL(OCIO::pystring::strip(osvec[i]), OCIO::pystring::strip(resvec[i]));
}

OIIO_ADD_TEST(FileFormatHDL, Read3D1D)
{
    std::ostringstream strebuf;
    strebuf << "Version         3" << "\n";
    strebuf << "Format      any" << "\n";
    strebuf << "Type        3D+1D" << "\n";
    strebuf << "From        0.005478 14.080103" << "\n";
    strebuf << "To      0 1" << "\n";
    strebuf << "Black       0" << "\n";
    strebuf << "White       1" << "\n";
    strebuf << "Length      2 10" << "\n";
    strebuf << "LUT:" << "\n";
    strebuf << "Pre {" << "\n";
    strebuf << "    0.994922" << "\n";
    strebuf << "    0.995052" << "\n";
    strebuf << "    0.995181" << "\n";
    strebuf << "    0.995310" << "\n";
    strebuf << "    0.995439" << "\n";
    strebuf << "    0.995568" << "\n";
    strebuf << "    0.995697" << "\n";
    strebuf << "    0.995826" << "\n";
    strebuf << "    0.995954" << "\n";
    strebuf << "    0.996082" << "\n";
    strebuf << "}" << "\n";
    strebuf << "3D {" << "\n";
    strebuf << "    0.093776 0.093776 0.093776" << "\n";
    strebuf << "    0.105219 0.093776 0.093776" << "\n";
    strebuf << "    0.118058 0.093776 0.093776" << "\n";
    strebuf << "    0.132463 0.093776 0.093776" << "\n";
    strebuf << "    0.148626 0.093776 0.093776" << "\n";
    strebuf << "    0.166761 0.093776 0.093776" << "\n";
    strebuf << "    0.187109 0.093776 0.093776" << "\n";
    strebuf << "    0.209939 0.093776 0.093776" << "\n";
    strebuf << "}" << "\n";
    
    //
    float from_min = 0.005478f;
    float from_max = 14.080103f;
    float to_min = 0.0f;
    float to_max = 1.0f;
    float black = 0.0f;
    float white = 1.0f;
    float prelut[10] = { 0.994922f, 0.995052f, 0.995181f, 0.995310f, 0.995439f,
        0.995568f, 0.995697f, 0.995826f, 0.995954f, 0.996082f };
    float cube[2 * 2 * 2 * 3 ] = {
        0.093776f, 0.093776f, 0.093776f,
        0.105219f, 0.093776f, 0.093776f,
        0.118058f, 0.093776f, 0.093776f,
        0.132463f, 0.093776f, 0.093776f,
        0.148626f, 0.093776f, 0.093776f,
        0.166761f, 0.093776f, 0.093776f,
        0.187109f, 0.093776f, 0.093776f,
        0.209939f, 0.093776f, 0.093776f };
    
    std::istringstream simple3D1D;
    simple3D1D.str(strebuf.str());
    
    // Load file
    OCIO::LocalFileFormat tester;
    OCIO::CachedFileRcPtr cachedFile = tester.Read(simple3D1D);
    OCIO::CachedFileHDLRcPtr lut = OCIO::DynamicPtrCast<OCIO::CachedFileHDL>(cachedFile);
    
    //
    OIIO_CHECK_EQUAL(to_min, lut->to_min);
    OIIO_CHECK_EQUAL(to_max, lut->to_max);
    OIIO_CHECK_EQUAL(black, lut->hdlblack);
    OIIO_CHECK_EQUAL(white, lut->hdlwhite);
    
    // check prelut data (each channel has the same data)
    for(int c = 0; c < 3; ++c) {
        OIIO_CHECK_EQUAL(from_min, lut->lut1D->from_min[c]);
        OIIO_CHECK_EQUAL(from_max, lut->lut1D->from_max[c]);
        OIIO_CHECK_EQUAL(10, lut->lut1D->luts[c].size());

        for(unsigned int i = 0; i < lut->lut1D->luts[c].size(); ++i) {
            OIIO_CHECK_EQUAL(prelut[i], lut->lut1D->luts[c][i]);
        }
    }

    OIIO_CHECK_EQUAL(2*2*2*3, lut->lut3D->lut.size());
    
    // check cube data
    for(unsigned int i = 0; i < lut->lut3D->lut.size(); ++i) {
        OIIO_CHECK_EQUAL(cube[i], lut->lut3D->lut[i]);
    }
}

OIIO_ADD_TEST(FileFormatHDL, Bake3D1D)
{
    // check baker output
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    
    // Set luma coef's to simple values
    {
        float lumaCoef[3] = {0.333f, 0.333f, 0.333f};
        config->setDefaultLumaCoefs(lumaCoef);
    }

    // Add lnf space
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("lnf");
        cs->setFamily("lnf");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }
    
    // Add shaper space
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("shaper");
        cs->setFamily("shaper");
        OCIO::ExponentTransformRcPtr transform1 = OCIO::ExponentTransform::Create();
        float test[4] = {2.6f, 2.6f, 2.6f, 1.0f};
        transform1->setValue(test);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_TO_REFERENCE);
        config->addColorSpace(cs);
    }
    
    // Add target space
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("target");
        cs->setFamily("target");
        OCIO::CDLTransformRcPtr transform1 = OCIO::CDLTransform::Create();

        // Set saturation to cause channel crosstalk, making a 3D LUT
        transform1->setSat(0.5f);

        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_FROM_REFERENCE);
        config->addColorSpace(cs);
    }
    
    std::string bout = 
    "Version\t\t3\n"
    "Format\t\tany\n"
    "Type\t\t3D+1D\n"
    "From\t\t0.000000 1.000000\n"
    "To\t\t0.000000 1.000000\n"
    "Black\t\t0.000000\n"
    "White\t\t1.000000\n"
    "Length\t\t2 10\n"
    "LUT:\n"
    "Pre {\n"
    "\t0.000000\n"
    "\t0.429520\n"
    "\t0.560744\n"
    "\t0.655378\n"
    "\t0.732057\n"
    "\t0.797661\n"
    "\t0.855604\n"
    "\t0.907865\n"
    "\t0.955710\n"
    "\t1.000000\n"
    "}\n"
    "3D {\n"
    "\t0.000000 0.000000 0.000000\n"
    "\t0.606300 0.106300 0.106300\n"
    "\t0.357600 0.857600 0.357600\n"
    "\t0.963900 0.963900 0.463900\n"
    "\t0.036100 0.036100 0.536100\n"
    "\t0.642400 0.142400 0.642400\n"
    "\t0.393700 0.893700 0.893700\n"
    "\t1.000000 1.000000 1.000000\n"
    "}\n";
    
    //
    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->setFormat("houdini");
    baker->setInputSpace("lnf");
    baker->setShaperSpace("shaper");
    baker->setTargetSpace("target");
    baker->setShaperSize(10);
    baker->setCubeSize(2);
    std::ostringstream output;
    baker->bake(output);

    //std::cerr << "The LUT: " << std::endl << output.str() << std::endl;
    //std::cerr << "Expected:" << std::endl << bout << std::endl;

    //
    std::vector<std::string> osvec;
    OCIO::pystring::splitlines(output.str(), osvec);
    std::vector<std::string> resvec;
    OCIO::pystring::splitlines(bout, resvec);
    OIIO_CHECK_EQUAL(osvec.size(), resvec.size());
    
    // TODO: Get this working on osx
    /*
    for(unsigned int i = 0; i < std::min(osvec.size(), resvec.size()); ++i)
        OIIO_CHECK_EQUAL(OCIO::pystring::strip(osvec[i]), OCIO::pystring::strip(resvec[i]));
    */
}


OIIO_ADD_TEST(FileFormatHDL, LookTest)
{
    // Note this sets up a Look with the same parameters as the Bake3D1D test
    // however it uses a different shaper space, to ensure we catch that case.
    // Also ensure we detect the effects of the desaturation by using a 3 cubed
    // LUT, which will thus test colour values other than the corner points of
    // the cube

    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    
    // Add lnf space
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("lnf");
        cs->setFamily("lnf");
        config->addColorSpace(cs);
        config->setRole(OCIO::ROLE_REFERENCE, cs->getName());
    }
    
    // Add shaper space
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
    
    // Add Look process space
    {
        OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
        cs->setName("look_process");
        cs->setFamily("look_process");
        OCIO::ExponentTransformRcPtr transform1 = OCIO::ExponentTransform::Create();
        float test[4] = {2.6f, 2.6f, 2.6f, 1.0f};
        transform1->setValue(test);
        cs->setTransform(transform1, OCIO::COLORSPACE_DIR_TO_REFERENCE);
        config->addColorSpace(cs);
    }

    // Add Look process space
    {
        OCIO::LookRcPtr look = OCIO::Look::Create();
        look->setName("look");
        look->setProcessSpace("look_process");
        OCIO::CDLTransformRcPtr transform1 = OCIO::CDLTransform::Create();

        // Set saturation to cause channel crosstalk, making a 3D LUT
        transform1->setSat(0.5f);

        look->setTransform(transform1);
        config->addLook(look);
    }
    
    
    std::string bout = 
    "Version\t\t3\n"
    "Format\t\tany\n"
    "Type\t\t3D+1D\n"
    "From\t\t0.000000 1.000000\n"
    "To\t\t0.000000 1.000000\n"
    "Black\t\t0.000000\n"
    "White\t\t1.000000\n"
    "Length\t\t3 10\n"
    "LUT:\n"
    "Pre {\n"
    "\t0.000000\n"
    "\t0.368344\n"
    "\t0.504760\n"
    "\t0.606913\n"
    "\t0.691699\n"
    "\t0.765539\n"
    "\t0.831684\n"
    "\t0.892049\n"
    "\t0.947870\n"
    "\t1.000000\n"
    "}\n"
    "3D {\n"
    "\t0.000000 0.000000 0.000000\n"
    "\t0.276787 0.035360 0.035360\n"
    "\t0.553575 0.070720 0.070720\n"
    "\t0.148309 0.416989 0.148309\n"
    "\t0.478739 0.478739 0.201718\n"
    "\t0.774120 0.528900 0.245984\n"
    "\t0.296618 0.833978 0.296618\n"
    "\t0.650361 0.902354 0.355417\n"
    "\t0.957478 0.957478 0.403436\n"
    "\t0.009867 0.009867 0.239325\n"
    "\t0.296368 0.049954 0.296368\n"
    "\t0.575308 0.086766 0.343137\n"
    "\t0.166161 0.437812 0.437812\n"
    "\t0.500000 0.500000 0.500000\n"
    "\t0.796987 0.550484 0.550484\n"
    "\t0.316402 0.857106 0.607391\n"
    "\t0.672631 0.925760 0.672631\n"
    "\t0.981096 0.981096 0.725386\n"
    "\t0.019735 0.019735 0.478650\n"
    "\t0.312132 0.062101 0.541651\n"
    "\t0.592736 0.099909 0.592736\n"
    "\t0.180618 0.454533 0.695009\n"
    "\t0.517061 0.517061 0.761560\n"
    "\t0.815301 0.567796 0.815301\n"
    "\t0.332322 0.875624 0.875624\n"
    "\t0.690478 0.944497 0.944497\n"
    "\t1.000000 1.000000 1.000000\n"
    "}\n";
    
    //
    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    baker->setConfig(config);
    baker->setFormat("houdini");
    baker->setInputSpace("lnf");
    baker->setShaperSpace("shaper");
    baker->setTargetSpace("shaper");
    baker->setLooks("look");
    baker->setShaperSize(10);
    baker->setCubeSize(3);
    std::ostringstream output;
    baker->bake(output);

    std::cerr << "The LUT: " << std::endl << output.str() << std::endl;
    std::cerr << "Expected:" << std::endl << bout << std::endl;

    //
    std::vector<std::string> osvec;
    OCIO::pystring::splitlines(output.str(), osvec);
    std::vector<std::string> resvec;
    OCIO::pystring::splitlines(bout, resvec);
    OIIO_CHECK_EQUAL(osvec.size(), resvec.size());
    
    for(unsigned int i = 0; i < std::min(osvec.size(), resvec.size()); ++i)
        OIIO_CHECK_EQUAL(OCIO::pystring::strip(osvec[i]), OCIO::pystring::strip(resvec[i]));
}

#endif // OCIO_BUILD_TESTS

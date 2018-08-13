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
#include <algorithm>

#include <OpenColorIO/OpenColorIO.h>

#include "FileTransform.h"
#include "Lut1DOp.h"
#include "Lut3DOp.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"

/*

Iridas itx format
LUT_3D_SIZE M

#LUT_3D_SIZE M
#where M is the size of the texture
#a 3D texture has the size M x M x M
#e.g. LUT_3D_SIZE 16 creates a 16 x 16 x 16 3D texture

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
*/


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
            os << "Error parsing Iridas .itx file (";
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
            info.name = "iridas_itx";
            info.extension = "itx";
            info.capabilities = (FORMAT_CAPABILITY_READ | FORMAT_CAPABILITY_WRITE);
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
                throw Exception ("File stream empty when trying to read Iridas .itx LUT");
            }
            
            // Parse the file
            std::vector<float> raw;
            
            int size3d[] = { 0, 0, 0 };
            bool in3d = false;
            
            {
                std::string line;
                std::vector<std::string> parts;
                std::vector<float> tmpfloats;
                int lineNumber = 0;

                while(nextline(istream, line))
                {
                    ++lineNumber;
                    // All lines starting with '#' are comments
                    if(pystring::startswith(line,"#")) continue;
                    
                    // Strip, lowercase, and split the line
                    pystring::split(pystring::lower(pystring::strip(line)), parts);
                    if(parts.empty()) continue;
                    
                    if(pystring::lower(parts[0]) == "lut_3d_size")
                    {
                        int size = 0;
                        
                        if(parts.size() != 2 
                            || !StringToInt( &size, parts[1].c_str()))
                        {
                            ThrowErrorMessage(
                                "Malformed LUT_3D_SIZE tag.",
                                fileName,
                                lineNumber,
                                line);
                        }
                        size3d[0] = size;
                        size3d[1] = size;
                        size3d[2] = size;
                        
                        raw.reserve(3*size3d[0]*size3d[1]*size3d[2]);
                        in3d = true;
                    }
                    else if(in3d)
                    {
                        // It must be a float triple!
                        
                        if(!StringVecToFloatVec(tmpfloats, parts)
                            || tmpfloats.size() != 3)
                        {
                            ThrowErrorMessage(
                                "Malformed color triples specified.",
                                fileName,
                                lineNumber,
                                line);
                        }
                        
                        for(int i=0; i<3; ++i)
                        {
                            raw.push_back(tmpfloats[i]);
                        }
                    }
                }
            }
            
            // Interpret the parsed data, validate LUT sizes
            LocalCachedFileRcPtr cachedFile 
                = LocalCachedFileRcPtr(new LocalCachedFile());
            
            if(in3d)
            {
                if(size3d[0]*size3d[1]*size3d[2] 
                    != static_cast<int>(raw.size()/3))
                {
                    std::ostringstream os;
                    os << "Incorrect number of 3D LUT entries. ";
                    os << "Found " << raw.size() / 3 << ", expected ";
                    os << size3d[0] * size3d[1] * size3d[2] << ".";
                    ThrowErrorMessage(
                        os.str().c_str(),
                        fileName, -1, "");
                }
                
                // Reformat 3D data
                cachedFile->lut3D->size[0] = size3d[0];
                cachedFile->lut3D->size[1] = size3d[1];
                cachedFile->lut3D->size[2] = size3d[2];
                cachedFile->lut3D->lut = raw;
            }
            else
            {
                ThrowErrorMessage(
                    "No 3D LUT found.",
                    fileName, -1, "");
            }
            
            return cachedFile;
        }
        
        void LocalFileFormat::Write(const Baker & baker,
                                    const std::string & formatName,
                                    std::ostream & ostream) const
        {
            int DEFAULT_CUBE_SIZE = 64;
            
            if(formatName != "iridas_itx")
            {
                std::ostringstream os;
                os << "Unknown 3dl format name, '";
                os << formatName << "'.";
                throw Exception(os.str().c_str());
            }
            
            ConstConfigRcPtr config = baker.getConfig();
            
            int cubeSize = baker.getCubeSize();
            if(cubeSize==-1) cubeSize = DEFAULT_CUBE_SIZE;
            cubeSize = std::max(2, cubeSize); // smallest cube is 2x2x2
            
            std::vector<float> cubeData;
            cubeData.resize(cubeSize*cubeSize*cubeSize*3);
            GenerateIdentityLut3D(&cubeData[0], cubeSize, 3, LUT3DORDER_FAST_RED);
            PackedImageDesc cubeImg(&cubeData[0], cubeSize*cubeSize*cubeSize, 1, 3);
            
            // Apply our conversion from the input space to the output space.
            ConstProcessorRcPtr inputToTarget;
            std::string looks = baker.getLooks();
            if (!looks.empty())
            {
                LookTransformRcPtr transform = LookTransform::Create();
                transform->setLooks(looks.c_str());
                transform->setSrc(baker.getInputSpace());
                transform->setDst(baker.getTargetSpace());
                inputToTarget = config->getProcessor(transform,
                    TRANSFORM_DIR_FORWARD);
            }
            else
            {
                inputToTarget = config->getProcessor(baker.getInputSpace(),
                    baker.getTargetSpace());
            }
            inputToTarget->apply(cubeImg);
            
            // Write out the file.
            // For for maximum compatibility with other apps, we will
            // not utilize the shaper or output any metadata
            
            ostream << "LUT_3D_SIZE " << cubeSize << "\n";
            if(cubeSize < 2)
            {
                throw Exception("Internal cube size exception.");
            }
            
            // Set to a fixed 6 decimal precision
            ostream.setf(std::ios::fixed, std::ios::floatfield);
            ostream.precision(6);
            for(int i=0; i<cubeSize*cubeSize*cubeSize; ++i)
            {
                float r = cubeData[3*i+0];
                float g = cubeData[3*i+1];
                float b = cubeData[3*i+2];
                ostream << r << " " << g << " " << b << "\n";
            }
            ostream << "\n";
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
                os << "Cannot build Iridas .itx Op. Invalid cache type.";
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
    
    FileFormat * CreateFileFormatIridasItx()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////
#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

void ReadIridasItx(const std::string & fileContent)
{
    std::istringstream is;
    is.str(fileContent);

    // Read file
    OCIO::LocalFileFormat tester;
    const std::string SAMPLE_NAME("Memory File");
    OCIO::CachedFileRcPtr cachedFile = tester.Read(is, SAMPLE_NAME);
}

OIIO_ADD_TEST(FileFormatIridasItx, ReadFailure)
{
    {
        // Validate stream can be read with no error.
        // Then stream will be altered to introduce errors.
        const std::string SAMPLE_NO_ERROR =
            "LUT_3D_SIZE 2\n"

            "0.0 0.0 0.0\n"
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "1.0 1.0 0.0\n"
            "0.0 0.0 1.0\n"
            "1.0 0.0 1.0\n"
            "0.0 1.0 1.0\n"
            "1.0 1.0 1.0\n";

        OIIO_CHECK_NO_THROW(ReadIridasItx(SAMPLE_NO_ERROR));
    }
    {
        // Wrong LUT_3D_SIZE tag
        const std::string SAMPLE_ERROR =
            "LUT_3D_SIZE 2 2\n"

            "0.0 0.0 0.0\n"
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "1.0 1.0 0.0\n"
            "0.0 0.0 1.0\n"
            "1.0 0.0 1.0\n"
            "0.0 1.0 1.0\n"
            "1.0 1.0 1.0\n";

        OIIO_CHECK_THROW_WHAT(ReadIridasItx(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Malformed LUT_3D_SIZE tag");
    }
    {
        // Unexpected tag
        const std::string SAMPLE_ERROR =
            "LUT_3D_SIZE 2\n"

            "WRONG_TAG\n"
            "0.0 0.0 0.0\n"
            "1.0 0.0 0.0\n"
            "0.0 1.0 0.0\n"
            "1.0 1.0 0.0\n"
            "0.0 0.0 1.0\n"
            "1.0 0.0 1.0\n"
            "0.0 1.0 1.0\n"
            "1.0 1.0 1.0\n";

        OIIO_CHECK_THROW_WHAT(ReadIridasItx(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Malformed color triples specified");
    }
    {
        // Wrong number of entries
        const std::string SAMPLE_ERROR =
            "LUT_3D_SIZE 2\n"

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

        OIIO_CHECK_THROW_WHAT(ReadIridasItx(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Incorrect number of 3D LUT entries");
    }
}

#endif // OCIO_UNIT_TEST

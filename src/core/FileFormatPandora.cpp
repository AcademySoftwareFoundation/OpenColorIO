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
            
            virtual CachedFileRcPtr Read(
                std::istream & istream,
                const std::string & fileName) const;
            
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
            os << "Error parsing Pandora LUT file (";
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
        LocalFileFormat::Read(
            std::istream & istream,
            const std::string & fileName) const
        {
            // this shouldn't happen
            if(!istream)
            {
                throw Exception ("File stream empty when trying "
                                 "to read Pandora LUT");
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
                int lineNumber = 0;

                while(nextline(istream, line))
                {
                    ++lineNumber;

                    // Strip, lowercase, and split the line
                    pystring::split(pystring::lower(pystring::strip(line)),
                                    parts);
                    if(parts.empty()) continue;
                    
                    // Skip all lines starting with '#'
                    if(pystring::startswith(parts[0],"#")) continue;
                    
                    if(parts[0] == "channel")
                    {
                        if(parts.size() != 2 
                            || pystring::lower(parts[1]) != "3d")
                        {
                            ThrowErrorMessage(
                                "Only 3D LUTs are currently supported "
                                "(channel: 3d).",
                                fileName,
                                lineNumber,
                                line);
                        }
                    }
                    else if(parts[0] == "in")
                    {
                        int inval = 0;
                        if(parts.size() != 2
                            || !StringToInt( &inval, parts[1].c_str()))
                        {
                            ThrowErrorMessage(
                                "Malformed 'in' tag.",
                                fileName,
                                lineNumber,
                                line);
                        }
                        raw3d.reserve(inval*3);
                        lutEdgeLen = Get3DLutEdgeLenFromNumPixels(inval);
                    }
                    else if(parts[0] == "out")
                    {
                        if(parts.size() != 2 
                            || !StringToInt(&outputBitDepthMaxValue,
                                            parts[1].c_str()))
                        {
                            ThrowErrorMessage(
                                "Malformed 'out' tag.",
                                fileName,
                                lineNumber,
                                line);
                        }
                    }
                    else if(parts[0] == "format")
                    {
                        if(parts.size() != 2 
                            || pystring::lower(parts[1]) != "lut")
                        {
                            ThrowErrorMessage(
                                "Only LUTs are currently supported "
                                "(format: lut).",
                                fileName,
                                lineNumber,
                                line);
                        }
                    }
                    else if(parts[0] == "values")
                    {
                        if(parts.size() != 4 || 
                           pystring::lower(parts[1]) != "red" ||
                           pystring::lower(parts[2]) != "green" ||
                           pystring::lower(parts[3]) != "blue")
                        {
                            ThrowErrorMessage(
                                "Only rgb LUTs are currently supported "
                                "(values: red green blue).",
                                fileName,
                                lineNumber,
                                line);
                        }
                        
                        inLut3d = true;
                    }
                    else if(inLut3d)
                    {
                        if(!StringVecToIntVec(tmpints, parts) || tmpints.size() != 4)
                        {
                            ThrowErrorMessage(
                                "Expected to find 4 integers.",
                                fileName,
                                lineNumber,
                                line);
                        }
                        
                        raw3d.push_back(tmpints[1]);
                        raw3d.push_back(tmpints[2]);
                        raw3d.push_back(tmpints[3]);
                    }
                }
            }
            
            // Interpret the parsed data, validate LUT sizes
            if(lutEdgeLen*lutEdgeLen*lutEdgeLen != static_cast<int>(raw3d.size()/3))
            {
                std::ostringstream os;
                os << "Incorrect number of 3D LUT entries. ";
                os << "Found " << raw3d.size() / 3 << ", expected ";
                os << lutEdgeLen*lutEdgeLen*lutEdgeLen << ".";
                ThrowErrorMessage(
                    os.str().c_str(),
                    fileName, -1, "");
            }
            
            if(lutEdgeLen*lutEdgeLen*lutEdgeLen == 0)
            {
                ThrowErrorMessage(
                    "No 3D LUT entries found.",
                    fileName, -1, "");
            }
            
            if(outputBitDepthMaxValue <= 0)
            {
                ThrowErrorMessage(
                    "A valid 'out' tag was not found.",
                    fileName, -1, "");
            }
            
            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());
            
            // Reformat 3D data
            cachedFile->lut3D->size[0] = lutEdgeLen;
            cachedFile->lut3D->size[1] = lutEdgeLen;
            cachedFile->lut3D->size[2] = lutEdgeLen;
            cachedFile->lut3D->lut.reserve(raw3d.size());
            
            float scale = 1.0f / ((float)outputBitDepthMaxValue - 1.0f);
            
            // lut3D->lut is red fastest (loop on B, G then R to push values)
            for (int bIndex = 0; bIndex<lutEdgeLen; ++bIndex)
            {
                for(int gIndex=0; gIndex<lutEdgeLen; ++gIndex)
                {
                    for (int rIndex = 0; rIndex<lutEdgeLen; ++rIndex)
                    {
                        // In file, LUT is blue fastest
                        int i = GetLut3DIndex_BlueFast(rIndex, gIndex, bIndex,
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
                os << "Cannot build Pandora LUT. Invalid cache type.";
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

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"
#include <fstream>

OIIO_ADD_TEST(FileFormatPandora, FormatInfo)
{
    OCIO::FormatInfoVec formatInfoVec;
    OCIO::LocalFileFormat tester;
    tester.GetFormatInfo(formatInfoVec);

    OIIO_CHECK_EQUAL(2, formatInfoVec.size());
    OIIO_CHECK_EQUAL("pandora_mga", formatInfoVec[0].name);
    OIIO_CHECK_EQUAL("mga", formatInfoVec[0].extension);
    OIIO_CHECK_EQUAL(OCIO::FORMAT_CAPABILITY_READ,
        formatInfoVec[0].capabilities);
    OIIO_CHECK_EQUAL("pandora_m3d", formatInfoVec[1].name);
    OIIO_CHECK_EQUAL("m3d", formatInfoVec[1].extension);
    OIIO_CHECK_EQUAL(OCIO::FORMAT_CAPABILITY_READ,
        formatInfoVec[1].capabilities);
}

void ReadPandora(const std::string & fileContent)
{
    std::istringstream is;
    is.str(fileContent);

    // Read file
    OCIO::LocalFileFormat tester;
    const std::string SAMPLE_NAME("Memory File");
    OCIO::CachedFileRcPtr cachedFile = tester.Read(is, SAMPLE_NAME);
}

OIIO_ADD_TEST(FileFormatPandora, ReadFailure)
{
    {
        // Validate stream can be read with no error.
        // Then stream will be altered to introduce errors.
        const std::string SAMPLE_NO_ERROR =
            "channel 3d\n"
            "in 8\n"
            "out 256\n"
            "format lut\n"
            "values red green blue\n"
            "0 0     0   0\n"
            "1 0     0 255\n"
            "2 0   255   0\n"
            "3 0   255 255\n"
            "4 255   0   0\n"
            "5 255   0 255\n"
            "6 255 255   0\n"
            "7 255 255 255\n";

        OIIO_CHECK_NO_THROW(ReadPandora(SAMPLE_NO_ERROR));
    }
    {
        // Wrong channel tag
        const std::string SAMPLE_ERROR =
            "channel 2d\n"
            "in 8\n"
            "out 256\n"
            "format lut\n"
            "values red green blue\n"
            "0 0     0   0\n"
            "1 0     0 255\n"
            "2 0   255   0\n"
            "3 0   255 255\n"
            "4 255   0   0\n"
            "5 255   0 255\n"
            "6 255 255   0\n"
            "7 255 255 255\n";

        OIIO_CHECK_THROW_WHAT(ReadPandora(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Only 3D LUTs are currently supported");
    }
    {
        // No value spec (LUT will not be read)
        const std::string SAMPLE_ERROR =
            "channel 3d\n"
            "in 8\n"
            "out 256\n"
            "format lut\n"
            "0 0     0   0\n"
            "1 0     0 255\n"
            "2 0   255   0\n"
            "3 0   255 255\n"
            "4 255   0   0\n"
            "5 255   0 255\n"
            "6 255 255   0\n"
            "7 255 255 255\n";

        OIIO_CHECK_THROW_WHAT(ReadPandora(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Incorrect number of 3D LUT entries");
    }
    {
        // Wrong entry
        const std::string SAMPLE_ERROR =
            "channel 3d\n"
            "in 8\n"
            "out 256\n"
            "format lut\n"
            "values red green blue\n"
            "0 0     0   0\n"
            "1 0     0 255\n"
            "2 0   255   0\n"
            "3 0   255 255\n"
            "4 WRONG 255   0   0\n"
            "5 255   0 255\n"
            "6 255 255   0\n"
            "7 255 255 255\n";

        OIIO_CHECK_THROW_WHAT(ReadPandora(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Expected to find 4 integers");
    }
    {
        // Wrong number of entries
        const std::string SAMPLE_ERROR =
            "channel 3d\n"
            "in 8\n"
            "out 256\n"
            "format lut\n"
            "values red green blue\n"
            "0 0     0   0\n"
            "1 0     0 255\n"
            "2 0   255   0\n"
            "3 0   255 255\n"
            "4 255   0   0\n"
            "5 255   0 255\n"
            "6 255 255   0\n"
            "7 255 255   0\n"
            "8 255 255 255\n";

        OIIO_CHECK_THROW_WHAT(ReadPandora(SAMPLE_ERROR),
                              OCIO::Exception,
                              "Incorrect number of 3D LUT entries");
    }
}

#endif // OCIO_UNIT_TEST
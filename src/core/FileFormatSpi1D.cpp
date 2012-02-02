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
#include "Lut1DOp.h"
#include "pystring/pystring.h"

#include <cstdio>
#include <sstream>

/*
Version 1
From -7.5 3.7555555555555555
Components 1
Length 4096
{
        0.031525943963232252
        0.045645604561056156
	...
}

*/


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
                lut = Lut1D::Create();
            };
            ~LocalCachedFile() {};
            
            Lut1DRcPtr lut;
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
            info.name = "spi1d";
            info.extension = "spi1d";
            info.capabilities = FORMAT_CAPABILITY_READ;
            formatInfoVec.push_back(info);
        }
        
        // Try and load the format
        // Raise an exception if it can't be loaded.

        CachedFileRcPtr LocalFileFormat::Read(std::istream & istream) const
        {
            Lut1DRcPtr lut1d = Lut1D::Create();

            // Parse Header Info
            int lut_size = -1;
            float from_min = 0.0;
            float from_max = 1.0;
            int version = -1;
            int components = -1;

            const int MAX_LINE_SIZE = 4096;
            char lineBuffer[MAX_LINE_SIZE];

            // PARSE HEADER INFO
            {
                std::string headerLine("");
                do
                {
                    istream.getline(lineBuffer, MAX_LINE_SIZE);
                    headerLine = std::string(lineBuffer);
                    if(pystring::startswith(headerLine, "Version"))
                    {
                        if(sscanf(lineBuffer, "Version %d", &version)!=1)
                            throw Exception("Invalid 'Version' Tag");
                    }
                    else if(pystring::startswith(headerLine, "From"))
                    {
                        if(sscanf(lineBuffer, "From %f %f", &from_min, &from_max)!=2)
                            throw Exception("Invalid 'From' Tag");
                    }
                    else if(pystring::startswith(headerLine, "Components"))
                    {
                        if(sscanf(lineBuffer, "Components %d", &components)!=1)
                            throw Exception("Invalid 'Components' Tag");
                    }
                    else if(pystring::startswith(headerLine, "Length"))
                    {
                        if(sscanf(lineBuffer, "Length %d", &lut_size)!=1)
                            throw Exception("Invalid 'Length' Tag");
                    }
                }
                while (istream.good() && !pystring::startswith(headerLine,"{"));
            }

            if(version == -1)
                throw Exception("Could not find 'Version' Tag");
            if(version != 1)
                throw Exception("Only format version 1 supported.");
            if (lut_size == -1)
                throw Exception("Could not find 'Length' Tag");
            if (components == -1)
                throw Exception("Could not find 'Components' Tag");
            if (components<0 || components>3)
                throw Exception("Components must be [1,2,3]");



            for(int i=0; i<3; ++i)
            {
                lut1d->from_min[i] = from_min;
                lut1d->from_max[i] = from_max;
                lut1d->luts[i].clear();
                lut1d->luts[i].reserve(lut_size);
            }

            {
                istream.getline(lineBuffer, MAX_LINE_SIZE);

                int lineCount=0;
                float values[4];

                while (istream.good())
                {
                    // If 1 component is specificed, use x1 x1 x1 defaultA
                    if(components==1 && sscanf(lineBuffer,"%f",&values[0])==1)
                    {
                        lut1d->luts[0].push_back(values[0]);
                        lut1d->luts[1].push_back(values[0]);
                        lut1d->luts[2].push_back(values[0]);
                        ++lineCount;
                    }
                    // If 2 components are specificed, use x1 x2 0.0
                    else if(components==2 && sscanf(lineBuffer,"%f %f",&values[0],&values[1])==2)
                    {
                        lut1d->luts[0].push_back(values[0]);
                        lut1d->luts[1].push_back(values[1]);
                        lut1d->luts[2].push_back(0.0);
                        ++lineCount;
                    }
                    // If 3 component is specificed, use x1 x2 x3 defaultA
                    else if(components==3 && sscanf(lineBuffer,"%f %f %f",&values[0],&values[1],&values[2])==3)
                    {
                        lut1d->luts[0].push_back(values[0]);
                        lut1d->luts[1].push_back(values[1]);
                        lut1d->luts[2].push_back(values[2]);
                        ++lineCount;
                    }

                    if(lineCount == lut_size) break;

                    istream.getline(lineBuffer, MAX_LINE_SIZE);
                }

                if(lineCount!=lut_size)
                    throw Exception("Not enough entries found.");
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
            lut1d->maxerror = 1e-5f;
            lut1d->errortype = ERROR_RELATIVE;

            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());
            cachedFile->lut = lut1d;
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
                os << "Cannot build Spi1D Op. Invalid cache type.";
                throw Exception(os.str().c_str());
            }

            TransformDirection newDir = CombineTransformDirections(dir,
                fileTransform.getDirection());

            CreateLut1DOp(ops,
                          cachedFile->lut,
                          fileTransform.getInterpolation(),
                          newDir);
        }
    }
    
    FileFormat * CreateFileFormatSpi1D()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT

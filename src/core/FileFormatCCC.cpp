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

#include <map>
#include <tinyxml.h>

#include <OpenColorIO/OpenColorIO.h>

#include "CDLTransform.h"
#include "FileTransform.h"
#include "OpBuilders.h"
#include "pystring/pystring.h"

OCIO_NAMESPACE_ENTER
{
    ////////////////////////////////////////////////////////////////
    
    namespace
    {
        typedef std::map<std::string,CDLTransformRcPtr> CDLMap;
        
        class LocalCachedFile : public CachedFile
        {
        public:
            LocalCachedFile () {};
            
            ~LocalCachedFile() {};
            
            CDLMap transforms;
        };
        
        typedef OCIO_SHARED_PTR<LocalCachedFile> LocalCachedFileRcPtr;
        typedef OCIO_SHARED_PTR<TiXmlDocument> TiXmlDocumentRcPtr;
        
        
        
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
            info.name = "ColorCorrectionCollection";
            info.extension = "ccc";
            info.capabilities = FORMAT_CAPABILITY_READ;
            formatInfoVec.push_back(info);
        }
        
        // Try and load the format
        // Raise an exception if it can't be loaded.
        
        CachedFileRcPtr LocalFileFormat::Read(std::istream & istream) const
        {
            std::ostringstream rawdata;
            rawdata << istream.rdbuf();
            
            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());
            
            TiXmlDocumentRcPtr doc = TiXmlDocumentRcPtr(new TiXmlDocument());
            doc->Parse(rawdata.str().c_str());
            
            if(doc->Error())
            {
                std::ostringstream os;
                os << "XML Parse Error. ";
                os << doc->ErrorDesc() << " (line ";
                os << doc->ErrorRow() << ", character ";
                os << doc->ErrorCol() << ")";
                throw Exception(os.str().c_str());
            }
            
            TiXmlElement* rootElement = doc->RootElement();
            if(!rootElement)
            {
                std::ostringstream os;
                os << "Error loading xml. Null root element.";
                throw Exception(os.str().c_str());
            }
            
            if(std::string(rootElement->Value()) != "ColorCorrectionCollection")
            {
                std::ostringstream os;
                os << "Error loading ccc xml. ";
                os << "Root element is type '" << rootElement->Value() << "', ";
                os << "ColorCorrectionCollection expected.";
                throw Exception(os.str().c_str());
            }
            
            GetCDLTransforms(cachedFile->transforms, rootElement);
            
            if(cachedFile->transforms.empty())
            {
                std::ostringstream os;
                os << "Error loading ccc xml. ";
                os << "No ColorCorrection elements found.";
                throw Exception(os.str().c_str());
            }
            
            return cachedFile;
        }
        
        void
        LocalFileFormat::BuildFileOps(OpRcPtrVec & ops,
                                      const Config& config,
                                      const ConstContextRcPtr & context,
                                      CachedFileRcPtr untypedCachedFile,
                                      const FileTransform& fileTransform,
                                      TransformDirection dir) const
        {
            LocalCachedFileRcPtr cachedFile = DynamicPtrCast<LocalCachedFile>(untypedCachedFile);
            
            // This should never happen.
            if(!cachedFile)
            {
                std::ostringstream os;
                os << "Cannot build .ccc Op. Invalid cache type.";
                throw Exception(os.str().c_str());
            }
            
            TransformDirection newDir = CombineTransformDirections(dir,
                fileTransform.getDirection());
            if(newDir == TRANSFORM_DIR_UNKNOWN)
            {
                std::ostringstream os;
                os << "Cannot build ASC FileTransform,";
                os << " unspecified transform direction.";
                throw Exception(os.str().c_str());
            }
            
            std::string cccid = fileTransform.getCCCId();
            cccid = context->resolveStringVar(cccid.c_str());
            
            CDLMap::const_iterator iter = cachedFile->transforms.find(cccid);
            if(iter == cachedFile->transforms.end())
            {
                std::ostringstream os;
                os << "Cannot build ASC FileTransform, specified cccid '";
                os << cccid << "' not found in " << fileTransform.getSrc();
                throw Exception(os.str().c_str());
            }
            
            BuildCDLOps(ops,
                        config,
                        *(iter->second),
                        newDir);
        }
    }
    
    FileFormat * CreateFileFormatCCC()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT


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

#include <tinyxml.h>
#include <stdio.h>

#include <OpenColorIO/OpenColorIO.h>

#include "FileTransform.h"
#include "OpBuilders.h"
#include "MatrixOps.h"

OCIO_NAMESPACE_ENTER
{
    ////////////////////////////////////////////////////////////////
    
    namespace
    {
        /// An internal Op cache that has been read from a CTF file.
        class CachedOp
        {
        };

        typedef OCIO_SHARED_PTR<CachedOp> CachedOpRcPtr;
        typedef std::vector<CachedOpRcPtr> CachedOpRcPtrVec;

        ////////////////////////////////////////////////////////////////

        class XMLTagHandler;
        typedef OCIO_SHARED_PTR<XMLTagHandler> XMLTagHandlerRcPtr;

        class XMLTagHandler
        {
        public:
            static XMLTagHandlerRcPtr CreateHandlerForTagName(std::string text);

            virtual CachedOpRcPtr handleXMLTag() = 0;
        };

        class MatrixTagHandler : public XMLTagHandler
        {
            class MatrixCachedOp : public CachedOp
            {

            };

            virtual CachedOpRcPtr handleXMLTag() {
                return CachedOpRcPtr(new MatrixCachedOp);
            }
        };

        /// A factory method to instantiate an appropriate XMLTagHandler for a given
        /// tag name. For example, passing "matrix" to this function should instantiate
        /// and return a MatrixTagHandler.
        XMLTagHandlerRcPtr XMLTagHandler::CreateHandlerForTagName(std::string text)
        {
            return XMLTagHandlerRcPtr(new MatrixTagHandler());
        }

        ////////////////////////////////////////////////////////////////

        class LocalCachedFile : public CachedFile
        {
        public:
            LocalCachedFile ()
            {

            };
            
            ~LocalCachedFile() {};
            
            CachedOpRcPtrVec m_cachedOps;

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
            info.name = "Color Transform Format";
            info.extension = "ctf";
            info.capabilities = FORMAT_CAPABILITY_READ;
            formatInfoVec.push_back(info);
        }
        
        // Try and load the format
        // Raise an exception if it can't be loaded.
        
        CachedFileRcPtr LocalFileFormat::Read(std::istream & istream) const
        {
            std::ostringstream rawdata;
            rawdata << istream.rdbuf();

            // We were asked to Read, so we should create a new cached file and fill it with
            // data from the file.
            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());
            
            // Create a TinyXML representation of the raw data we were given
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
            printf("text: %s\n", rootElement->GetText());

            // Create an XMLTagHandler to handle this specific tag
            XMLTagHandlerRcPtr tagHandler = XMLTagHandler::CreateHandlerForTagName("matrix");
            tagHandler->handleXMLTag();

            /*
            Read XML data into cachedFile
            */

            //cachedFile->m_cachedOps.push_back(CachedOpRcPtr(new CachedOp(69)));
            
            return cachedFile;
        }
        
        void
        LocalFileFormat::BuildFileOps(OpRcPtrVec & ops,
                                      const Config& config,
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
                os << "Cannot build .ctf Op. Invalid cache type.";
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
            
            float * scale = new float[4];
            scale[0] = -2.0;
            scale[1] = 2.0;
            scale[2] = 2.0;
            scale[3] = 3.0;

            CreateScaleOp(ops, scale, dir);


            /*
            class Op;
            typedef OCIO_SHARED_PTR<Op> OpRcPtr;
            typedef std::vector<OpRcPtr> OpRcPtrVec;

            ops.push_back( MatrixOffsetOpRcPtr(new MatrixOffsetOp(m44,
            offset4, direction)) );
            */
        }
    }
    
    FileFormat * CreateFileFormatCTF()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT


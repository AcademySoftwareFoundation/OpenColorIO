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
        public:
            virtual void buildFinalOp(OpRcPtrVec &ops, TransformDirection dir) = 0;
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

            virtual CachedOpRcPtr handleXMLTag(TiXmlElement * element) = 0;
        };

        /// To add support for a new CTF XML tag:
        ///
        /// 1. Copy the template FooTagHandler below and replace Foo with an
        /// appropriate name.
        ///
        /// 2. Modify the CreateHandlerForTagName factory method to return
        /// your new handler subclass when passed an appropriate XML tag string.

        /*
        class FooTagHandler : public XMLTagHandler
        {
            class FooCachedOp : public CachedOp
            {
            public:
                /// Data structures representing the operator go here.
                /// For example, the MatrixCachedOp contains just a float m_m44[16],
                /// to store the elements of the matrix.

                virtual void buildFinalOp(OpRcPtrVec &ops, TransformDirection dir) {
                    /// Turn the cached data into actual ops by passing the 
                    /// ops variable to a Create___Op function.

                    /// For example, the matrix operator just called
                    /// CreateMatrixOp(ops, &m_m44[0], dir);
                }
            };

            virtual CachedOpRcPtr handleXMLTag(TiXmlElement * element) {
                /// FooCachedOp * cachedOp = new FooCachedOp;

                /// Read the ASCII data from the Tiny XML element representation
                /// into the intermediate cached format.
                /// For example, the MatrixTagHandler just converts the ASCII
                /// values to floats and stores them in a MatrixCachedOp.               

                /// return CachedOpRcPtr(cachedOp);
            }
        };
        */

        /// Handles <matrix> tags in CTF files.
        class MatrixTagHandler : public XMLTagHandler
        {
            class MatrixCachedOp : public CachedOp
            {
            public:
                float m_m44[16];

                /// Turns the cached data into an Op and adds it to the vector of
                /// Ops
                virtual void buildFinalOp(OpRcPtrVec &ops, TransformDirection dir) {
                    CreateMatrixOp(ops, &m_m44[0], dir);
                }
            };

            /// Parses the raw ASCII data and returns a CachedOp
            /// containing the data.
            virtual CachedOpRcPtr handleXMLTag(TiXmlElement * element) {
                MatrixCachedOp * cachedOp = new MatrixCachedOp;

                // The first child should be an <array> tag
                TiXmlElement *arrayElement = element->FirstChildElement();

                // Read the matrix tokens into our matrix array
                // TODO: if the matrix specified is only 3x3, extrapolate to 4x4
                std::istringstream is(arrayElement->GetText());
                int i = 0;
                while (!is.eof()) {
                    double token;
                    is >> token;

                    cachedOp->m_m44[i++] = token;
                }

                return CachedOpRcPtr(cachedOp);
            }
        };

        /// A factory method to instantiate an appropriate XMLTagHandler for a given
        /// tag name. For example, passing "matrix" to this function should instantiate
        /// and return a MatrixTagHandler.
        XMLTagHandlerRcPtr XMLTagHandler::CreateHandlerForTagName(std::string text)
        {
            if (text.compare("Matrix") == 0) {
                return XMLTagHandlerRcPtr(new MatrixTagHandler());
            }
            return XMLTagHandlerRcPtr(NULL);
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
            info.name = "Color Transform File";
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
            // data from the .ctf file.
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
            
            // Get the root element of the CTF file. It should be "ProcessList"
            // TODO: should probably verify this!
            TiXmlElement* rootElement = doc->RootElement();

            // Iterate through the children nodes
            TiXmlElement* currentElement = rootElement->FirstChildElement();
            while (currentElement) {
                std::string tagName = currentElement->Value();

                // Create an XMLTagHandler to handle this specific tag
                XMLTagHandlerRcPtr tagHandler = XMLTagHandler::CreateHandlerForTagName(tagName);
                if (tagHandler) {
                    CachedOpRcPtr cachedOp = tagHandler->handleXMLTag(currentElement);

                    // Store the CachedOp in our cached file, as we'll need it later
                    cachedFile->m_cachedOps.push_back(cachedOp);
                }
                
                // On to the next one
                currentElement = currentElement->NextSiblingElement();
            }
            
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

            // Iterate through the cached file ops
            for (int i = 0; i < cachedFile->m_cachedOps.size(); i++) {
                CachedOpRcPtr thisOp = cachedFile->m_cachedOps[i];
                thisOp->buildFinalOp(ops, dir);
            }
        }
    }
    
    FileFormat * CreateFileFormatCTF()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT


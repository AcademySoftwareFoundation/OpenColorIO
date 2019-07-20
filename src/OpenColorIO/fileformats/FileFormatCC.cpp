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

#include "fileformats/cdl/CDLParser.h"
#include "transforms/FileTransform.h"
#include "OpBuilders.h"

OCIO_NAMESPACE_ENTER
{
    ////////////////////////////////////////////////////////////////
    
    namespace
    {
        class LocalCachedFile : public CachedFile
        {
        public:
            LocalCachedFile ()
            {
                transform = CDLTransform::Create();
            };
            
            ~LocalCachedFile() {};
            
            CDLTransformRcPtr transform;
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
        };
        
        void LocalFileFormat::GetFormatInfo(FormatInfoVec & formatInfoVec) const
        {
            FormatInfo info;
            info.name = "ColorCorrection";
            info.extension = "cc";
            info.capabilities = FORMAT_CAPABILITY_READ;
            formatInfoVec.push_back(info);
        }
        
        // Try and load the format
        // Raise an exception if it can't be loaded.
        
        CachedFileRcPtr LocalFileFormat::Read(
            std::istream & istream,
            const std::string & fileName) const
        {
            LocalCachedFileRcPtr cachedFile = LocalCachedFileRcPtr(new LocalCachedFile());
            
            try
            {
                CDLParser parser(fileName);
                parser.parse(istream);
                parser.getCDLTransform(cachedFile->transform);
            }
            catch(Exception & e)
            {
                std::ostringstream os;
                os << "Error parsing .cc file. ";
                os << "Does not appear to contain a valid ASC CDL XML:";
                os << e.what();
                throw Exception(os.str().c_str());
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
                os << "Cannot build .cc Op. Invalid cache type.";
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
            
            BuildCDLOps(ops,
                        config,
                        *cachedFile->transform,
                        newDir);
        }
    }
    
    FileFormat * CreateFileFormatCC()
    {
        return new LocalFileFormat();
    }
}
OCIO_NAMESPACE_EXIT

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include "UnitTest.h"
#include "UnitTestUtils.h"

OCIO::LocalCachedFileRcPtr LoadCCFile(const std::string & fileName)
{
    return OCIO::LoadTestFile<OCIO::LocalFileFormat, OCIO::LocalCachedFile>(
        fileName, std::ios_base::in);
}

OCIO_ADD_TEST(FileFormatCC, TestCC1)
{
    // CC file
    const std::string fileName("cdl_test1.cc");

    OCIO::LocalCachedFileRcPtr ccFile;
    OCIO_CHECK_NO_THROW(ccFile = LoadCCFile(fileName));

    std::string idStr(ccFile->transform->getID());
    OCIO_CHECK_EQUAL("foo", idStr);
    std::string descStr(ccFile->transform->getDescription());
    OCIO_CHECK_EQUAL("this is a description", descStr);
    float slope[3] = { 0.f, 0.f, 0.f };
    OCIO_CHECK_NO_THROW(ccFile->transform->getSlope(slope));
    OCIO_CHECK_EQUAL(1.1f, slope[0]);
    OCIO_CHECK_EQUAL(1.2f, slope[1]);
    OCIO_CHECK_EQUAL(1.3f, slope[2]);
    float offset[3] = { 0.f, 0.f, 0.f };
    OCIO_CHECK_NO_THROW(ccFile->transform->getOffset(offset));
    OCIO_CHECK_EQUAL(2.1f, offset[0]);
    OCIO_CHECK_EQUAL(2.2f, offset[1]);
    OCIO_CHECK_EQUAL(2.3f, offset[2]);
    float power[3] = { 0.f, 0.f, 0.f };
    OCIO_CHECK_NO_THROW(ccFile->transform->getPower(power));
    OCIO_CHECK_EQUAL(3.1f, power[0]);
    OCIO_CHECK_EQUAL(3.2f, power[1]);
    OCIO_CHECK_EQUAL(3.3f, power[2]);
    OCIO_CHECK_EQUAL(0.7f, ccFile->transform->getSat());
}

OCIO_ADD_TEST(FileFormatCC, TestCC2)
{
    // CC file using windows eol.
    const std::string fileName("cdl_test2.cc");

    OCIO::LocalCachedFileRcPtr ccFile;
    OCIO_CHECK_NO_THROW(ccFile = LoadCCFile(fileName));

    std::string idStr(ccFile->transform->getID());
    OCIO_CHECK_EQUAL("cc0001", idStr);
    // OCIO keeps only the first SOPNode description
    std::string descStr(ccFile->transform->getDescription());
    OCIO_CHECK_EQUAL("Example look", descStr);
    float slope[3] = { 0.f, 0.f, 0.f };
    OCIO_CHECK_NO_THROW(ccFile->transform->getSlope(slope));
    OCIO_CHECK_EQUAL(1.0f, slope[0]);
    OCIO_CHECK_EQUAL(1.0f, slope[1]);
    OCIO_CHECK_EQUAL(0.9f, slope[2]);
    float offset[3] = { 0.f, 0.f, 0.f };
    OCIO_CHECK_NO_THROW(ccFile->transform->getOffset(offset));
    OCIO_CHECK_EQUAL(-0.03f, offset[0]);
    OCIO_CHECK_EQUAL(-0.02f, offset[1]);
    OCIO_CHECK_EQUAL(0.0f, offset[2]);
    float power[3] = { 0.f, 0.f, 0.f };
    OCIO_CHECK_NO_THROW(ccFile->transform->getPower(power));
    OCIO_CHECK_EQUAL(1.25f, power[0]);
    OCIO_CHECK_EQUAL(1.0f, power[1]);
    OCIO_CHECK_EQUAL(1.0f, power[2]);
    OCIO_CHECK_EQUAL(1.7f, ccFile->transform->getSat());
}

OCIO_ADD_TEST(FileFormatCC, TestCC_SATNode)
{
    // CC file
    const std::string fileName("cdl_test_SATNode.cc");

    OCIO::LocalCachedFileRcPtr ccFile;
    OCIO_CHECK_NO_THROW(ccFile = LoadCCFile(fileName));

    // "SATNode" is recognized.
    OCIO_CHECK_EQUAL(0.42f, ccFile->transform->getSat());
}

OCIO_ADD_TEST(FileFormatCC, TestCC_ASC_SAT)
{
    // CC file
    const std::string fileName("cdl_test_ASC_SAT.cc");

    OCIO::LocalCachedFileRcPtr ccFile;
    OCIO_CHECK_NO_THROW(ccFile = LoadCCFile(fileName));

    // "ASC_SAT" is not recognized. Default value is returned.
    OCIO_CHECK_EQUAL(1.0f, ccFile->transform->getSat());
}

OCIO_ADD_TEST(FileFormatCC, TestCC_ASC_SOP)
{
    // CC file
    const std::string fileName("cdl_test_ASC_SOP.cc");

    OCIO::LocalCachedFileRcPtr ccFile;
    OCIO_CHECK_NO_THROW(ccFile = LoadCCFile(fileName));

    // "ASC_SOP" is not recognized. Default values are used.
    std::string descStr(ccFile->transform->getDescription());
    OCIO_CHECK_EQUAL("", descStr);
    float slope[3] = { 0.f, 0.f, 0.f };
    OCIO_CHECK_NO_THROW(ccFile->transform->getSlope(slope));
    OCIO_CHECK_EQUAL(1.0f, slope[0]);
    float offset[3] = { 1.f, 1.f, 1.f };
    OCIO_CHECK_NO_THROW(ccFile->transform->getOffset(offset));
    OCIO_CHECK_EQUAL(0.0f, offset[0]);
    float power[3] = { 0.f, 0.f, 0.f };
    OCIO_CHECK_NO_THROW(ccFile->transform->getPower(power));
    OCIO_CHECK_EQUAL(1.0f, power[0]);
}

#endif
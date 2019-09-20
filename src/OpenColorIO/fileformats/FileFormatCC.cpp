// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

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
            
            ~LocalCachedFile() = default;
            
            CDLTransformRcPtr transform;
        };
        
        typedef OCIO_SHARED_PTR<LocalCachedFile> LocalCachedFileRcPtr;
        
        
        
        class LocalFileFormat : public FileFormat
        {
        public:
            LocalFileFormat() = default;
            ~LocalFileFormat() = default;
            
            void getFormatInfo(FormatInfoVec & formatInfoVec) const override;
            
            CachedFileRcPtr read(
                std::istream & istream,
                const std::string & fileName) const override;
            
            void buildFileOps(OpRcPtrVec & ops,
                              const Config& config,
                              const ConstContextRcPtr & context,
                              CachedFileRcPtr untypedCachedFile,
                              const FileTransform& fileTransform,
                              TransformDirection dir) const override;
        };
        
        void LocalFileFormat::getFormatInfo(FormatInfoVec & formatInfoVec) const
        {
            FormatInfo info;
            info.name = "ColorCorrection";
            info.extension = "cc";
            info.capabilities = FORMAT_CAPABILITY_READ;
            formatInfoVec.push_back(info);
        }
        
        // Try and load the format
        // Raise an exception if it can't be loaded.
        
        CachedFileRcPtr LocalFileFormat::read(
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
        LocalFileFormat::buildFileOps(OpRcPtrVec & ops,
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

    OCIO_REQUIRE_ASSERT(ccFile);

    auto & formatMetadata = ccFile->transform->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(formatMetadata.getNumChildrenElements(), 1);
    OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getName()), "SOPDescription");
    OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getValue()), "this is a description");

    std::string idStr(ccFile->transform->getID());
    OCIO_CHECK_EQUAL("foo", idStr);

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
    OCIO_CHECK_EQUAL(0.7, ccFile->transform->getSat());
}

OCIO_ADD_TEST(FileFormatCC, TestCC2)
{
    // CC file using windows eol.
    const std::string fileName("cdl_test2.cc");

    OCIO::LocalCachedFileRcPtr ccFile;
    OCIO_CHECK_NO_THROW(ccFile = LoadCCFile(fileName));

    OCIO_REQUIRE_ASSERT(ccFile);

    auto & formatMetadata = ccFile->transform->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(formatMetadata.getNumChildrenElements(), 2);
    OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getName()), "SOPDescription");
    OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(0).getValue()), "Example look");
    OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(1).getName()), "SATDescription");
    OCIO_CHECK_EQUAL(std::string(formatMetadata.getChildElement(1).getValue()), "boosting sat");

    std::string idStr(ccFile->transform->getID());
    OCIO_CHECK_EQUAL("cc0001", idStr);

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
    OCIO_CHECK_EQUAL(1.7, ccFile->transform->getSat());
}

OCIO_ADD_TEST(FileFormatCC, TestCC_SATNode)
{
    // CC file
    const std::string fileName("cdl_test_SATNode.cc");

    OCIO::LocalCachedFileRcPtr ccFile;
    OCIO_CHECK_NO_THROW(ccFile = LoadCCFile(fileName));

    OCIO_REQUIRE_ASSERT(ccFile);

    // "SATNode" is recognized.
    OCIO_CHECK_EQUAL(0.42, ccFile->transform->getSat());
}

OCIO_ADD_TEST(FileFormatCC, TestCC_ASC_SAT)
{
    // CC file
    const std::string fileName("cdl_test_ASC_SAT.cc");

    OCIO::LocalCachedFileRcPtr ccFile;
    OCIO_CHECK_NO_THROW(ccFile = LoadCCFile(fileName));

    OCIO_REQUIRE_ASSERT(ccFile);

    // "ASC_SAT" is not recognized. Default value is returned.
    OCIO_CHECK_EQUAL(1.0, ccFile->transform->getSat());
}

OCIO_ADD_TEST(FileFormatCC, TestCC_ASC_SOP)
{
    // CC file
    const std::string fileName("cdl_test_ASC_SOP.cc");

    OCIO::LocalCachedFileRcPtr ccFile;
    OCIO_CHECK_NO_THROW(ccFile = LoadCCFile(fileName));

    OCIO_REQUIRE_ASSERT(ccFile);

    // "ASC_SOP" is not recognized. Default values are used.
    auto & formatMetadata = ccFile->transform->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(formatMetadata.getNumChildrenElements(), 0);

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
// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <vector>
#include <iostream>

#include <OpenColorIO/OpenColorIO.h>

#include "transforms/FileTransform.h"
#include "MathUtils.h"
#include "pystring/pystring.h"

OCIO_NAMESPACE_ENTER
{
    
    BakerRcPtr Baker::Create()
    {
        return BakerRcPtr(new Baker(), &deleter);
    }
    
    void Baker::deleter(Baker* c)
    {
        delete c;
    }
    
    class Baker::Impl
    {
    public:
        
        ConfigRcPtr config_;
        std::string formatName_;
        FormatMetadataImpl formatMetadata_{ METADATA_ROOT, "" };
        std::string inputSpace_;
        std::string shaperSpace_;
        std::string looks_;
        std::string targetSpace_;
        int shapersize_;
        int cubesize_;
        
        Impl() :
            shapersize_(-1),
            cubesize_(-1)
        {
        }

        Impl(const Impl &) = delete;
        
        ~Impl()
        {
        }
        
        Impl& operator= (const Impl & rhs)
        {
            if (this != &rhs)
            {
                config_ = rhs.config_;
                formatName_ = rhs.formatName_;
                formatMetadata_ = rhs.formatMetadata_;
                inputSpace_ = rhs.inputSpace_;
                shaperSpace_ = rhs.shaperSpace_;
                looks_ = rhs.looks_;
                targetSpace_ = rhs.targetSpace_;
                shapersize_ = rhs.shapersize_;
                cubesize_ = rhs.cubesize_;
            }
            return *this;
        }
    };
    
    Baker::Baker()
    : m_impl(new Baker::Impl)
    {
    }
    
    Baker::~Baker()
    {
        delete m_impl;
        m_impl = NULL;
    }
    
    BakerRcPtr Baker::createEditableCopy() const
    {
        BakerRcPtr oven = Baker::Create();
        *oven->m_impl = *m_impl;
        return oven;
    }
    
    void Baker::setConfig(const ConstConfigRcPtr & config)
    {
        getImpl()->config_ = config->createEditableCopy();
    }
    
    ConstConfigRcPtr Baker::getConfig() const
    {
        return getImpl()->config_;
    }
    
    int Baker::getNumFormats()
    {
        return FormatRegistry::GetInstance().getNumFormats(FORMAT_CAPABILITY_BAKE);
    }
    
    const char * Baker::getFormatNameByIndex(int index)
    {
        return FormatRegistry::GetInstance().getFormatNameByIndex(FORMAT_CAPABILITY_BAKE, index);
    }
    
    const char * Baker::getFormatExtensionByIndex(int index)
    {
        return FormatRegistry::GetInstance().getFormatExtensionByIndex(FORMAT_CAPABILITY_BAKE, index);
    }
    
    void Baker::setFormat(const char * formatName)
    {
        FileFormat* fmt = FormatRegistry::GetInstance().getFileFormatByName(formatName);
        FormatInfoVec formatInfoVec;
        fmt->getFormatInfo(formatInfoVec);
        for(unsigned int i=0; i<formatInfoVec.size(); ++i)
        {
            if(formatInfoVec[i].capabilities & FORMAT_CAPABILITY_BAKE)
            {
                getImpl()->formatName_ = formatName;
                return;
            }
        }

        std::ostringstream os;
        os << "File format " << formatName;
        os << " does not support baking.";
        throw Exception(os.str().c_str());
    }
    
    const char * Baker::getFormat() const
    {
        return getImpl()->formatName_.c_str();
    }

    const FormatMetadata & Baker::getFormatMetadata() const
    {
        return getImpl()->formatMetadata_;
    }

    FormatMetadata & Baker::getFormatMetadata()
    {
        return getImpl()->formatMetadata_;
    }

    void Baker::setInputSpace(const char * inputSpace)
    {
        getImpl()->inputSpace_ = inputSpace;
    }
    
    const char * Baker::getInputSpace() const
    {
        return getImpl()->inputSpace_.c_str();
    }
    
    void Baker::setShaperSpace(const char * shaperSpace)
    {
        getImpl()->shaperSpace_ = shaperSpace;
    }
    
    const char * Baker::getShaperSpace() const
    {
        return getImpl()->shaperSpace_.c_str();
    }

    void Baker::setLooks(const char * looks)
    {
        getImpl()->looks_ = looks;
    }
    
    const char * Baker::getLooks() const
    {
        return getImpl()->looks_.c_str();
    }

    void Baker::setTargetSpace(const char * targetSpace)
    {
        getImpl()->targetSpace_ = targetSpace;
    }
    
    const char * Baker::getTargetSpace() const
    {
        return getImpl()->targetSpace_.c_str();
    }
    
    void Baker::setShaperSize(int shapersize)
    {
        getImpl()->shapersize_ = shapersize;
    }
    
    int Baker::getShaperSize() const
    {
        return getImpl()->shapersize_;
    }
    
    void Baker::setCubeSize(int cubesize)
    {
        getImpl()->cubesize_ = cubesize;
    }
    
    int Baker::getCubeSize() const
    {
        return getImpl()->cubesize_;
    }
    
    void Baker::bake(std::ostream & os) const
    {
        FileFormat* fmt = FormatRegistry::GetInstance().getFileFormatByName(getImpl()->formatName_);
        
        if(!fmt)
        {
            std::ostringstream err;
            err << "The format named '" << getImpl()->formatName_;
            err << "' could not be found. ";
            throw Exception(err.str().c_str());
        }

        if(!getConfig())
        {
            throw Exception("No OCIO config has been set");
        }
   
        try
        {
            fmt->bake(*this, getImpl()->formatName_, os);
        }
        catch(std::exception & e)
        {
            std::ostringstream err;
            err << "Error baking " << getImpl()->formatName_ << ":";
            err << e.what();
            throw Exception(err.str().c_str());
        }
        
        // 
        // TODO:
        // 
        // - throw exception when we don't have inputSpace and targetSpace
        //   at least set
        // - check limits of shaper and target, throw exception if we can't
        //   write that much data in x format
        // - check that the shaper is 1D transform only, throw exception
        // - check the file format supports shapers, 1D and 3D
        // - add some checks to make sure we are monotonic
        // - deal with the case of writing out non cube formats (1D only)
        // - do a compare between OCIO transform and output LUT transform
        //   throw error if we going beyond tolerance
        //
    }
    
}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

/*
OCIO_ADD_TEST(Baker_Unit_Tests, test_listlutwriters)
{
    
    std::vector<std::string> current_writers;
    current_writers.push_back("cinespace");
    current_writers.push_back("houdini");
    
    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    
    OCIO_CHECK_EQUAL(baker->getNumFormats(), (int)current_writers.size());
    
    std::vector<std::string> test;
    for(int i = 0; i < baker->getNumFormats(); ++i)
        test.push_back(baker->getFormatNameByIndex(i));
    
    for(unsigned int i = 0; i < current_writers.size(); ++i)
        OCIO_CHECK_EQUAL(current_writers[i], test[i]);
    
}
*/

OCIO_ADD_TEST(Baker, bake)
{
    // SSE aware test, similar to python test.
    OCIO::BakerRcPtr bake = OCIO::Baker::Create();

    static const std::string myProfile =
        "ocio_profile_version: 1\n"
        "\n"
        "strictparsing: false\n"
        "\n"
        "colorspaces :\n"
        "  - !<ColorSpace>\n"
        "    name : lnh\n"
        "    bitdepth : 16f\n"
        "    isdata : false\n"
        "    allocation : lg2\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name : test\n"
        "    bitdepth : 8ui\n"
        "    isdata : false\n"
        "    allocation : uniform\n"
        "    to_reference : !<ExponentTransform> {value: [2.2, 2.2, 2.2, 1]}\n";

    static const std::string expectedLut =
        "CSPLUTV100\n"
        "3D\n"
        "\n"
        "BEGIN METADATA\n"
        "this is some metadata!\n"
        "END METADATA\n"
        "\n"
#ifdef USE_SSE
        "4\n"
        "0.000977 0.039373 1.587398 64.000168\n"
        "0.000000 0.333333 0.666667 1.000000\n"
        "4\n"
        "0.000977 0.039373 1.587398 64.000168\n"
        "0.000000 0.333333 0.666667 1.000000\n"
        "4\n"
        "0.000977 0.039373 1.587398 64.000168\n"
        "0.000000 0.333333 0.666667 1.000000\n"
        "\n"
        "2 2 2\n"
        "0.042823 0.042823 0.042823\n"
        "6.622035 0.042823 0.042823\n"
        "0.042823 6.622035 0.042823\n"
        "6.622035 6.622035 0.042823\n"
        "0.042823 0.042823 6.622035\n"
        "6.622035 0.042823 6.622035\n"
        "0.042823 6.622035 6.622035\n"
        "6.622035 6.622035 6.622035\n"
#else
        "4\n"
        "0.000977 0.039373 1.587401 64.000000\n"
        "0.000000 0.333333 0.666667 1.000000\n"
        "4\n"
        "0.000977 0.039373 1.587401 64.000000\n"
        "0.000000 0.333333 0.666667 1.000000\n"
        "4\n"
        "0.000977 0.039373 1.587401 64.000000\n"
        "0.000000 0.333333 0.666667 1.000000\n"
        "\n"
        "2 2 2\n"
        "0.042823 0.042823 0.042823\n"
        "6.622026 0.042823 0.042823\n"
        "0.042823 6.622026 0.042823\n"
        "6.622026 6.622026 0.042823\n"
        "0.042823 0.042823 6.622026\n"
        "6.622026 0.042823 6.622026\n"
        "0.042823 6.622026 6.622026\n"
        "6.622026 6.622026 6.622026\n"
#endif // USE_SSE
        "\n";
    std::istringstream is(myProfile);
    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_REQUIRE_EQUAL(config->getNumColorSpaces(), 2);
    bake->setConfig(config);
    auto cfg2 = bake->getConfig();
    OCIO_REQUIRE_EQUAL(cfg2->getNumColorSpaces(), 2);

    const std::string testString{ "this is some metadata!" };
    bake->getFormatMetadata().addChildElement("Desc", testString.c_str());
    const auto & data = bake->getFormatMetadata();
    OCIO_CHECK_EQUAL(data.getNumChildrenElements(), 1);
    OCIO_CHECK_EQUAL(testString, data.getChildElement(0).getValue());

    bake->setFormat("cinespace");
    OCIO_CHECK_EQUAL("cinespace", std::string(bake->getFormat()));
    bake->setInputSpace("lnh");
    OCIO_CHECK_EQUAL("lnh", std::string(bake->getInputSpace()));
    bake->setLooks("foo, +bar");
    OCIO_CHECK_EQUAL("foo, +bar", std::string(bake->getLooks()));
    bake->setLooks("");
    bake->setTargetSpace("test");
    OCIO_CHECK_EQUAL("test", std::string(bake->getTargetSpace()));
    bake->setShaperSize(4);
    OCIO_CHECK_EQUAL(4, bake->getShaperSize());
    bake->setCubeSize(2);
    OCIO_CHECK_EQUAL(2, bake->getCubeSize());
    std::ostringstream os;
    OCIO_CHECK_NO_THROW(bake->bake(os));
    OCIO_CHECK_EQUAL(expectedLut, os.str());
    OCIO_CHECK_EQUAL(10, bake->getNumFormats());
    OCIO_CHECK_EQUAL("cinespace", std::string(bake->getFormatNameByIndex(4)));
    OCIO_CHECK_EQUAL("3dl", std::string(bake->getFormatExtensionByIndex(1)));
}

OCIO_ADD_TEST(Baker, empty_config)
{
    // Verify that running bake with an empty configuration
    // throws an exception and does not segfault.
    OCIO::BakerRcPtr bake = OCIO::Baker::Create();
    bake->setFormat("cinespace");
    std::ostringstream os;
    OCIO_CHECK_THROW_WHAT(bake->bake(os), OCIO::Exception, "No OCIO config has been set");
}

#endif // OCIO_BUILD_TESTS

    

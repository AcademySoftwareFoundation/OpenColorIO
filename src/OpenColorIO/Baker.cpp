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
        std::string type_;
        std::string metadata_;
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
        
        ~Impl()
        {
        }
        
        Impl& operator= (const Impl & rhs)
        {
            config_      = rhs.config_;
            formatName_  = rhs.formatName_;
            inputSpace_  = rhs.inputSpace_;
            shaperSpace_ = rhs.shaperSpace_;
            looks_        = rhs.looks_;
            targetSpace_ = rhs.targetSpace_;
            shapersize_  = rhs.shapersize_;
            cubesize_    = rhs.cubesize_;
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
        return FormatRegistry::GetInstance().getNumFormats(FORMAT_CAPABILITY_WRITE);
    }
    
    const char * Baker::getFormatNameByIndex(int index)
    {
        return FormatRegistry::GetInstance().getFormatNameByIndex(FORMAT_CAPABILITY_WRITE, index);
    }
    
    const char * Baker::getFormatExtensionByIndex(int index)
    {
        return FormatRegistry::GetInstance().getFormatExtensionByIndex(FORMAT_CAPABILITY_WRITE, index);
    }
    
    void Baker::setFormat(const char * formatName)
    {
        getImpl()->formatName_ = formatName;
    }
    
    const char * Baker::getFormat() const
    {
        return getImpl()->formatName_.c_str();
    }
    
    void Baker::setType(const char * type)
    {
        getImpl()->type_ = type;
    }
    
    const char * Baker::getType() const
    {
        return getImpl()->type_.c_str();
    }
    
    void Baker::setMetadata(const char * metadata)
    {
        getImpl()->metadata_ = metadata;
    }
    
    const char * Baker::getMetadata() const
    {
        return getImpl()->metadata_.c_str();
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
        
        try
        {
            fmt->Write(*this, getImpl()->formatName_, os);
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
        // - check that the shaper is 1D transform only, throw excpetion
        // - check the file format supports shapers, 1D and 3D
        // - add some checks to make sure we are monotonic
        // - deal with the case of writing out non cube formats (1D only)
        // - do a compare between ocio transform and output lut transform
        //   throw error if we going beyond tolerance
        //
    }
    
}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

/*
OIIO_ADD_TEST(Baker_Unit_Tests, test_listlutwriters)
{
    
    std::vector<std::string> current_writers;
    current_writers.push_back("cinespace");
    current_writers.push_back("houdini");
    
    OCIO::BakerRcPtr baker = OCIO::Baker::Create();
    
    OIIO_CHECK_EQUAL(baker->getNumFormats(), (int)current_writers.size());
    
    std::vector<std::string> test;
    for(int i = 0; i < baker->getNumFormats(); ++i)
        test.push_back(baker->getFormatNameByIndex(i));
    
    for(unsigned int i = 0; i < current_writers.size(); ++i)
        OIIO_CHECK_EQUAL(current_writers[i], test[i]);
    
}
*/

#endif // OCIO_BUILD_TESTS

    

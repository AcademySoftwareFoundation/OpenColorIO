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

#include "FileTransform.h"
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
        std::string targetSpace_;
        int shapersize_;
        int cubesize_;
        
        Impl()
        {
            shapersize_ = 1024;
            cubesize_ = 32;
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
    
    int Baker::getNumFormats() const
    {
        int count = 0;
        FormatRegistry & formats = GetFormatRegistry();
        for(unsigned int findex=0; findex<formats.size(); ++findex)
        {
            FileFormat* format = formats[findex];
            if(format->Supports("write"))
                count++;
        }
        return count;
    }
    
    const char * Baker::getFormatNameByIndex(int index) const
    {
        int count = 0;
        FormatRegistry & formats = GetFormatRegistry();
        for(unsigned int findex=0; findex<formats.size(); ++findex)
        {
            FileFormat* format = formats[findex];
            if(format->Supports("write")) {
                if(count == index)
                    return pystring::lower(format->GetName()).c_str();
                count++;
            }
        }
        return "";
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
    
    void Baker::setCubeSize(int cubesize)
    {
        getImpl()->cubesize_ = cubesize;
    }
    
    void Baker::bake(std::ostream & os) const
    {
        
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
        
        ConstProcessorRcPtr shaper_enc;
        ConstProcessorRcPtr shaper_dec;
        ConstProcessorRcPtr target;
        
        if(getImpl()->shaperSpace_ != "")
        {
            shaper_enc = getImpl()->config_->getProcessor(getImpl()->inputSpace_.c_str(),
                                                          getImpl()->shaperSpace_.c_str());
            shaper_dec = getImpl()->config_->getProcessor(getImpl()->shaperSpace_.c_str(),
                                                          getImpl()->inputSpace_.c_str());
            target     = getImpl()->config_->getProcessor(getImpl()->shaperSpace_.c_str(),
                                                          getImpl()->targetSpace_.c_str());
        }
        else
        {
            target     = getImpl()->config_->getProcessor(getImpl()->inputSpace_.c_str(),
                                                          getImpl()->targetSpace_.c_str());
        }
        
        TransformData data;
        
        // cubesize
        data.lookup3DSize = getImpl()->cubesize_;
        
        // shaper should be atleast cubsize^2
        data.shaperSize = getImpl()->shapersize_;
        
        //
        data.minlum[0] = 0.f; data.minlum[1] = 0.f; data.minlum[2] = 0.f;
        data.maxlum[0] = 1.f; data.maxlum[1] = 1.f; data.maxlum[2] = 1.f;
        
        if(shaper_enc && shaper_dec)
        {
            // find the min and max luminance we can encode
            shaper_dec->applyRGB(data.minlum);
            shaper_dec->applyRGB(data.maxlum);
            
            // build an identitiy ramp and setup the encode ramp based
            // on the max and min luminance
            
            data.shaper_ident.resize(data.shaperSize * 3);
            data.shaper_encode.resize(data.shaperSize * 3);
            data.shaper_decode.resize(data.shaperSize * 3);
            
            for (size_t i = 0; i < data.shaperSize; ++i)
            {
                const float x = (float)(double(i) / double(data.shaperSize - 1));
                data.shaper_ident[3*i+0] = x;
                data.shaper_ident[3*i+1] = x;
                data.shaper_ident[3*i+2] = x;
                data.shaper_encode[3*i+0] = lerpf(data.minlum[0], data.maxlum[0], x);
                data.shaper_encode[3*i+1] = lerpf(data.minlum[1], data.maxlum[1], x);
                data.shaper_encode[3*i+2] = lerpf(data.minlum[2], data.maxlum[2], x);
            }
            data.shaper_decode = data.shaper_ident;
            
            // uniform shaper to decoded
            PackedImageDesc _tmp1(&data.shaper_decode[0], data.shaperSize, 1, 3);
            shaper_dec->apply(_tmp1);
            
            // uniform shaper to encoded
            PackedImageDesc _tmp2(&data.shaper_encode[0], data.shaperSize, 1, 3);
            shaper_enc->apply(_tmp2);
            
        }
        
        if(target)
        {
            for (size_t ib = 0; ib < data.lookup3DSize; ++ib) {
                const float bx = (float)(double(ib) / double(data.lookup3DSize - 1));
                for (size_t ig = 0; ig < data.lookup3DSize; ++ig) {
                    const float gx = (float)(double(ig) / double(data.lookup3DSize - 1));
                    for (size_t ir = 0; ir < data.lookup3DSize; ++ir) {
                        const float rx = (float)(double(ir) / double(data.lookup3DSize - 1));
                        float _tmp[3] = { rx, gx, bx };
                        // run the cube through the shaper first
                        if(shaper_enc && shaper_dec)
                        {
                            shaper_dec->applyRGB((float *)&_tmp);
                            shaper_enc->applyRGB((float *)&_tmp);
                        }
                        // apply the cube
                        target->applyRGB((float *)&_tmp);
                        data.lookup3D.push_back(_tmp[0]);
                        data.lookup3D.push_back(_tmp[1]);
                        data.lookup3D.push_back(_tmp[2]);
                    }
                }
            }
        }
        
        //
        bool supported = false;
        FileFormat* fmt = GetFileFormat(getImpl()->formatName_);
        if(fmt)
        {
            try
            {
                supported = fmt->Write(data, os);
            }
            catch(std::exception & e)
            {
                std::ostringstream err;
                err << e.what();
                throw Exception(err.str().c_str());
            }
        }
        
        if(!supported || !fmt)
        {
            std::ostringstream err;
            err << "We don't support the '" << getImpl()->formatName_;
            err << "' lut format for baking";
            throw Exception(err.str().c_str());
        }
        
    }
    
}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

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

#endif // OCIO_BUILD_TESTS

    
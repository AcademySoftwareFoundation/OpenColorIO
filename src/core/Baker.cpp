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

#define LERP(a, b, x) ((a) * (1 - (x)) + (b) * (x))

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
    
    void Baker::setFormat(const char * formatName)
    {
        getImpl()->formatName_ = formatName;
    }
    
    void Baker::setInput(const char * inputSpace)
    {
        getImpl()->inputSpace_ = inputSpace;
    }
    
    void Baker::setShaper(const char * shaperSpace)
    {
        getImpl()->shaperSpace_ = shaperSpace;
    }
    
    void Baker::setTarget(const char * targetSpace)
    {
        getImpl()->targetSpace_ = targetSpace;
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
            
            // build a ramp ident ramp
            for (size_t i = 0; i < data.shaperSize; ++i)
            {
                const float x = (float)(double(i) / double(data.shaperSize - 1));
                data.shaper_ident.push_back(x);
            }
            
            // uniform shaper to decoded
            for (size_t i = 0; i < data.shaper_ident.size(); ++i)
            {
                RGB _tmp;
                _tmp.r = data.shaper_ident[i];
                _tmp.g = data.shaper_ident[i];
                _tmp.b = data.shaper_ident[i];
                shaper_dec->applyRGB((float *)&_tmp);
                data.shaper_decode.push_back(_tmp);
            }
            
            // uniform shaper to encoded
            for (size_t i = 0; i < data.shaper_ident.size(); ++i)
            {
                RGB _tmp;
                _tmp.r = LERP(data.minlum[0], data.maxlum[0], data.shaper_ident[i]);
                _tmp.g = LERP(data.minlum[1], data.maxlum[1], data.shaper_ident[i]);
                _tmp.b = LERP(data.minlum[2], data.maxlum[2], data.shaper_ident[i]);
                shaper_enc->applyRGB((float *)&_tmp);
                data.shaper_encode.push_back(_tmp);
            }
            
        }
        
        if(target)
        {
            for (size_t ib = 0; ib < data.lookup3DSize; ++ib) {
                const float bx = (float)(double(ib) / double(data.lookup3DSize - 1));
                for (size_t ig = 0; ig < data.lookup3DSize; ++ig) {
                    const float gx = (float)(double(ig) / double(data.lookup3DSize - 1));
                    for (size_t ir = 0; ir < data.lookup3DSize; ++ir) {
                        const float rx = (float)(double(ir) / double(data.lookup3DSize - 1));
                        RGB _tmp;
                        _tmp.r = rx;
                        _tmp.g = gx;
                        _tmp.b = bx;
                        // run the cube through the shaper first
                        if(shaper_enc && shaper_dec)
                        {
                            shaper_dec->applyRGB((float *)&_tmp);
                            shaper_enc->applyRGB((float *)&_tmp);
                        }
                        // apply the cube
                        target->applyRGB((float *)&_tmp);
                        data.lookup3D.push_back(_tmp);
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

    
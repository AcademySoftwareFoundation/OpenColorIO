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

#include <algorithm>

#include "OpBuilders.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"


OCIO_NAMESPACE_ENTER
{
    LookTransformRcPtr LookTransform::Create()
    {
        return LookTransformRcPtr(new LookTransform(), &deleter);
    }
    
    void LookTransform::deleter(LookTransform* t)
    {
        delete t;
    }
    
    
    class LookTransform::Impl
    {
    public:
        TransformDirection dir_;
        std::string src_;
        std::string dst_;
        std::string looks_;
        
        Impl() :
            dir_(TRANSFORM_DIR_FORWARD)
        { }
        
        ~Impl()
        { }
        
        Impl& operator= (const Impl & rhs)
        {
            dir_ = rhs.dir_;
            src_ = rhs.src_;
            dst_ = rhs.dst_;
            looks_ = rhs.looks_;
            return *this;
        }
    };
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    LookTransform::LookTransform()
        : m_impl(new LookTransform::Impl)
    {
    }
    
    TransformRcPtr LookTransform::createEditableCopy() const
    {
        LookTransformRcPtr transform = LookTransform::Create();
        *(transform->m_impl) = *m_impl;
        return transform;
    }
    
    LookTransform::~LookTransform()
    {
        delete m_impl;
        m_impl = NULL;
    }
    
    LookTransform& LookTransform::operator= (const LookTransform & rhs)
    {
        *m_impl = *rhs.m_impl;
        return *this;
    }
    
    TransformDirection LookTransform::getDirection() const
    {
        return getImpl()->dir_;
    }
    
    void LookTransform::setDirection(TransformDirection dir)
    {
        getImpl()->dir_ = dir;
    }
    
    const char * LookTransform::getSrc() const
    {
        return getImpl()->src_.c_str();
    }
    
    void LookTransform::setSrc(const char * src)
    {
        getImpl()->src_ = src;
    }
    
    const char * LookTransform::getDst() const
    {
        return getImpl()->dst_.c_str();
    }
    
    void LookTransform::setDst(const char * dst)
    {
        getImpl()->dst_ = dst;
    }
    
    void LookTransform::setLooks(const char * looks)
    {
        getImpl()->looks_ = looks;
    }
    
    const char * LookTransform::getLooks() const
    {
        return getImpl()->looks_.c_str();
    }
    
    std::ostream& operator<< (std::ostream& os, const LookTransform& t)
    {
        os << "<LookTransform ";
        os << "src=" << t.getSrc() << ", ";
        os << "dst=" << t.getDst() << ", ";
        os << "looks=" << t.getLooks() << ", ";
        os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
        os << ">\n";
        return os;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    
    void SplitLooks(StringVec & lookVec, TransformDirectionVec & directionVec,
                    const std::string & looks)
    {
        SplitStringEnvStyle(lookVec, looks.c_str());
        
        for(unsigned int i=0; i<lookVec.size(); ++i)
        {
            if(pystring::startswith(lookVec[i], "+"))
            {
                directionVec.push_back(TRANSFORM_DIR_FORWARD);
                lookVec[i] = pystring::lstrip(lookVec[i], "+");
            }
            else if(pystring::startswith(lookVec[i], "-"))
            {
                directionVec.push_back(TRANSFORM_DIR_INVERSE);
                lookVec[i] = pystring::lstrip(lookVec[i], "-");
            }
            else
            {
                directionVec.push_back(TRANSFORM_DIR_FORWARD);
            }
        }
    }
    
    void BuildLookOps(OpRcPtrVec & ops,
                      const Config& config,
                      const ConstContextRcPtr & context,
                      const LookTransform & lookTransform,
                      TransformDirection dir)
    {
        ConstColorSpaceRcPtr src, dst;
        src = config.getColorSpace( lookTransform.getSrc() );
        dst = config.getColorSpace( lookTransform.getDst() );
        
        if(!src)
        {
            std::ostringstream os;
            os << "BuildLookOps error.";
            os << "The specified lookTransform specifies a src colorspace, '";
            os <<  lookTransform.getSrc() << "', which is not defined.";
            throw Exception(os.str().c_str());
        }
        
        if(!dst)
        {
            std::ostringstream os;
            os << "BuildLookOps error.";
            os << "The specified lookTransform specifies a dst colorspace, '";
            os <<  lookTransform.getDst() << "', which is not defined.";
            throw Exception(os.str().c_str());
        }
        
        // We must handle the inverse src/dst colorspace transformation explicitly.
        if(dir == TRANSFORM_DIR_INVERSE)
        {
            std::swap(src, dst);
        }
        
        ConstColorSpaceRcPtr currentColorSpace = src;
        BuildLookOps(ops,
                     currentColorSpace,
                     false,
                     config,
                     context,
                     lookTransform.getLooks(),
                     dir);
        
        BuildColorSpaceOps(ops, config, context,
                           currentColorSpace,
                           dst);
    }
    
    void BuildLookOps(OpRcPtrVec & ops,
                      ConstColorSpaceRcPtr & currentColorSpace,
                      bool skipColorSpaceConversions,
                      const Config& config,
                      const ConstContextRcPtr & context,
                      const std::string & looks,
                      TransformDirection dir)
    {
        if(looks.empty()) return;
        
        // Turn the look string into a set of ordered look operations
        StringVec lookVec;
        TransformDirectionVec directionVec;
        SplitLooks(lookVec, directionVec, looks);
        
        // We account for application in the inverse direction
        // by pre-reversing the order (and flipping direction) of
        // the look vectors.
        if(dir == TRANSFORM_DIR_INVERSE)
        {
            std::reverse(lookVec.begin(), lookVec.end());
            std::reverse(directionVec.begin(), directionVec.end());
            for(unsigned int i=0; i<directionVec.size(); ++i)
            {
                directionVec[i] = GetInverseTransformDirection(directionVec[i]);
            }
        }
        else if(dir == TRANSFORM_DIR_UNKNOWN)
        {
            std::ostringstream os;
            os << "BuildLookOps error. A valid transform direction must be specified.";
            throw Exception(os.str().c_str());
        }
        
        for(unsigned int i=0; i<lookVec.size(); ++i)
        {
            ConstLookRcPtr look = config.getLook(lookVec[i].c_str());
            if(!look)
            {
                std::ostringstream os;
                os << "BuildLookOps error. ";
                os << "The specified look, '" << lookVec[i];
                os << "', cannot be found in this OCIO configuration.";
                if(config.getNumLooks() == 0)
                {
                    os << " (No looks defined in config)";
                }
                else
                {
                    os << " (looks: ";
                    for(int ii=0; ii<config.getNumLooks(); ++ii)
                    {
                        if(ii != 0) os << ", ";
                        os << config.getLookNameByIndex(ii);
                    }
                    os << ")";
                }
                
                throw Exception(os.str().c_str());
            }
            
            ConstColorSpaceRcPtr processColorSpace = config.getColorSpace(look->getProcessSpace());
            if(!processColorSpace)
            {
                std::ostringstream os;
                os << "BuildLookOps error. ";
                os << "The specified look, '" << lookVec[i];
                os << "', requires processing in the ColorSpace, '";
                os << look->getProcessSpace() << "' which is not defined.";
                throw Exception(os.str().c_str());
            }
            
            // Put the new ops into a temp array, to see if it's a no-op
            // If it is a no-op, dont bother doing the colorspace conversion.
            OpRcPtrVec tmpOps;
            
            if(directionVec[i] == TRANSFORM_DIR_FORWARD)
            {
                if(look->getTransform())
                {
                    BuildOps(tmpOps, config, context, look->getTransform(), TRANSFORM_DIR_FORWARD);
                }
                else if(look->getInverseTransform())
                {
                    BuildOps(tmpOps, config, context, look->getInverseTransform(), TRANSFORM_DIR_INVERSE);
                }
            }
            else if(directionVec[i] == TRANSFORM_DIR_INVERSE)
            {
                if(look->getInverseTransform())
                {
                    BuildOps(tmpOps, config, context, look->getInverseTransform(), TRANSFORM_DIR_FORWARD);
                }
                else if(look->getTransform())
                {
                    BuildOps(tmpOps, config, context, look->getTransform(), TRANSFORM_DIR_INVERSE);
                }
            }
            else
            {
                std::ostringstream os;
                os << "BuildLookOps error. ";
                os << "The specified look, '" << lookVec[i];
                os << "', processing in the ColorSpace, '";
                os << look->getProcessSpace() << "' has an ill-defined transform direction.";
                throw Exception(os.str().c_str());
            }
            
            if(!IsOpVecNoOp(tmpOps))
            {
                if(!skipColorSpaceConversions)
                {
                    BuildColorSpaceOps(ops, config, context,
                                       currentColorSpace,
                                       processColorSpace);
                    currentColorSpace = processColorSpace;
                }
                
                std::copy(tmpOps.begin(), tmpOps.end(), std::back_inserter(ops));
            }
        }
    }
}
OCIO_NAMESPACE_EXIT

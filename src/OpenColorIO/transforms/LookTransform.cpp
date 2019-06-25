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
#include <iterator>

#include "LookParse.h"
#include "ops/NoOp/NoOps.h"
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

        Impl(const Impl &) = delete;

        ~Impl()
        { }
        
        Impl& operator= (const Impl & rhs)
        {
            if (this != &rhs)
            {
                dir_ = rhs.dir_;
                src_ = rhs.src_;
                dst_ = rhs.dst_;
                looks_ = rhs.looks_;
            }
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
        if (this != &rhs)
        {
            *m_impl = *rhs.m_impl;
        }
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

    void LookTransform::validate() const
    {
        Transform::validate();

        if (getImpl()->src_.empty())
        {
            throw Exception("LookTransform: empty source color space name");
        }

        if (getImpl()->dst_.empty())
        {
            throw Exception("LookTransform: empty destination color space name");
        }
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
        os << "<LookTransform";
        os << " src=" << t.getSrc();
        os << ", dst=" << t.getDst();
        os << ", looks=" << t.getLooks();
        os << ", direction=" << TransformDirectionToString(t.getDirection());
        os << ">";
        return os;
    }
    
    ////////////////////////////////////////////////////////////////////////////
    
    
    
    
    namespace
    {
    
    void RunLookTokens(OpRcPtrVec & ops,
                       ConstColorSpaceRcPtr & currentColorSpace,
                       bool skipColorSpaceConversions,
                       const Config& config,
                       const ConstContextRcPtr & context,
                       const LookParseResult::Tokens & lookTokens)
    {
        if(lookTokens.empty()) return;
        
        for(unsigned int i=0; i<lookTokens.size(); ++i)
        {
            const std::string & lookName = lookTokens[i].name;
            
            if(lookName.empty()) continue;
            
            ConstLookRcPtr look = config.getLook(lookName.c_str());
            if(!look)
            {
                std::ostringstream os;
                os << "RunLookTokens error. ";
                os << "The specified look, '" << lookName;
                os << "', cannot be found. ";
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
            
            // Put the new ops into a temp array, to see if it's a no-op
            // If it is a no-op, dont bother doing the colorspace conversion.
            OpRcPtrVec tmpOps;
            
            if(lookTokens[i].dir == TRANSFORM_DIR_FORWARD)
            {
                CreateLookNoOp(tmpOps, lookName);
                if(look->getTransform())
                {
                    BuildOps(tmpOps, config, context, look->getTransform(), TRANSFORM_DIR_FORWARD);
                }
                else if(look->getInverseTransform())
                {
                    BuildOps(tmpOps, config, context, look->getInverseTransform(), TRANSFORM_DIR_INVERSE);
                }
            }
            else if(lookTokens[i].dir == TRANSFORM_DIR_INVERSE)
            {
                CreateLookNoOp(tmpOps, std::string("-") + lookName);
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
                os << "The specified look, '" << lookTokens[i].name;
                os << "' has an ill-defined transform direction.";
                throw Exception(os.str().c_str());
            }
            
            if(!IsOpVecNoOp(tmpOps))
            {
                if(!skipColorSpaceConversions)
                {
                    ConstColorSpaceRcPtr processColorSpace = config.getColorSpace(look->getProcessSpace());
                    if(!processColorSpace)
                    {
                        std::ostringstream os;
                        os << "RunLookTokens error. ";
                        os << "The specified look, '" << lookTokens[i].name;
                        os << "', requires processing in the ColorSpace, '";
                        os << look->getProcessSpace() << "' which is not defined.";
                        throw Exception(os.str().c_str());
                    }
                    
                    BuildColorSpaceOps(ops, config, context,
                                       currentColorSpace,
                                       processColorSpace);
                    currentColorSpace = processColorSpace;
                }
                
                ops += tmpOps;
            }
        }
    }
    
    } // anon namespace
    
    ////////////////////////////////////////////////////////////////////////////
    
    
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
        
        LookParseResult looks;
        looks.parse(lookTransform.getLooks());
        
        // We must handle the inverse src/dst colorspace transformation explicitly.
        if(dir == TRANSFORM_DIR_INVERSE)
        {
            std::swap(src, dst);
            looks.reverse();
        }
        else if(dir == TRANSFORM_DIR_UNKNOWN)
        {
            std::ostringstream os;
            os << "BuildLookOps error. A valid transform direction must be specified.";
            throw Exception(os.str().c_str());
        }
        
        ConstColorSpaceRcPtr currentColorSpace = src;
        BuildLookOps(ops,
                     currentColorSpace,
                     false,
                     config,
                     context,
                     looks);
        
        BuildColorSpaceOps(ops, config, context,
                           currentColorSpace,
                           dst);
    }
    
    void BuildLookOps(OpRcPtrVec & ops,
                      ConstColorSpaceRcPtr & currentColorSpace,
                      bool skipColorSpaceConversions,
                      const Config& config,
                      const ConstContextRcPtr & context,
                      const LookParseResult & looks)
    {
        const LookParseResult::Options & options = looks.getOptions();
        
        if(options.empty())
        {
            // Do nothing
        }
        else if(options.size() == 1)
        {
            // As an optimization, if we only have a single look option,
            // just push back onto the final location
            RunLookTokens(ops,
                          currentColorSpace,
                          skipColorSpaceConversions,
                          config,
                          context,
                          options[0]);
        }
        else
        {
            // If we have multiple look options, try each one in order,
            // and if we can create the ops without a missing file exception,
            // push back it's results and return
            
            bool success = false;
            std::ostringstream os;
            
            OpRcPtrVec tmpOps;
            ConstColorSpaceRcPtr cs;
            
            for(unsigned int i=0; i<options.size(); ++i)
            {
                cs = currentColorSpace;
                tmpOps.clear();
                
                try
                {
                    RunLookTokens(tmpOps,
                                  cs,
                                  skipColorSpaceConversions,
                                  config,
                                  context,
                                  options[i]);
                    success = true;
                    break;
                }
                catch(ExceptionMissingFile & e)
                {
                    if(i != 0) os << "  ...  ";
                    
                    os << "(";
                    LookParseResult::serialize(os, options[i]);
                    os << ") " << e.what();
                }
            }
            
            if(success)
            {
                currentColorSpace = cs;
                ops += tmpOps;
            }
            else
            {
                throw ExceptionMissingFile(os.str().c_str());
            }
        }
    }
}
OCIO_NAMESPACE_EXIT

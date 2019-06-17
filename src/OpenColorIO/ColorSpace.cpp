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

#include <cstring>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "pystring/pystring.h"


OCIO_NAMESPACE_ENTER
{
    ColorSpaceRcPtr ColorSpace::Create()
    {
        return ColorSpaceRcPtr(new ColorSpace(), &deleter);
    }
    
    void ColorSpace::deleter(ColorSpace* c)
    {
        delete c;
    }
    
    
    class ColorSpace::Impl
    {
    public:
        std::string name_;
        std::string family_;
        std::string equalityGroup_;
        std::string description_;
        
        BitDepth bitDepth_;
        bool isData_;
        
        Allocation allocation_;
        std::vector<float> allocationVars_;
        
        TransformRcPtr toRefTransform_;
        TransformRcPtr fromRefTransform_;
        
        bool toRefSpecified_;
        bool fromRefSpecified_;

        typedef std::vector<std::string> Categories;
        Categories categories_;
        
        Impl() :
            bitDepth_(BIT_DEPTH_UNKNOWN),
            isData_(false),
            allocation_(ALLOCATION_UNIFORM),
            toRefSpecified_(false),
            fromRefSpecified_(false)
        { }

        Impl(const Impl &) = delete;
        
        ~Impl()
        { }
        
        Impl& operator= (const Impl & rhs)
        {
            if (this != &rhs)
            {
                name_ = rhs.name_;
                family_ = rhs.family_;
                equalityGroup_ = rhs.equalityGroup_;
                description_ = rhs.description_;
                bitDepth_ = rhs.bitDepth_;
                isData_ = rhs.isData_;
                allocation_ = rhs.allocation_;
                allocationVars_ = rhs.allocationVars_;

                toRefTransform_ = rhs.toRefTransform_?
                    rhs.toRefTransform_->createEditableCopy()
                    : rhs.toRefTransform_;

                fromRefTransform_ = rhs.fromRefTransform_?
                    rhs.fromRefTransform_->createEditableCopy()
                    : rhs.fromRefTransform_;

                toRefSpecified_ = rhs.toRefSpecified_;
                fromRefSpecified_ = rhs.fromRefSpecified_;

                categories_ = rhs.categories_;
            }
            return *this;
        }

        Categories::const_iterator findCategory(const char * category) const
        {
            if(!category || !*category) return categories_.end();

            // NB: Categories are not case-sensitive and whitespace is stripped.
            const std::string ref(pystring::strip(pystring::lower(category)));

            for(auto itr = categories_.begin(); itr!=categories_.end(); ++itr)
            {
                if(pystring::strip(pystring::lower(*itr))==ref)
                {
                    return itr;
                }
            }

            return categories_.end();
        }

        void removeCategory(const char * category)
        {
            if(!category || !*category) return;

            // NB: Categories are not case-sensitive and whitespace is stripped.
            const std::string ref(pystring::strip(pystring::lower(category)));

            for(auto itr = categories_.begin(); itr!=categories_.end(); ++itr)
            {
                if(pystring::strip(pystring::lower(*itr))==ref)
                {
                    categories_.erase(itr);
                    return;
                }
            }

            return;
        }
    };
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    ColorSpace::ColorSpace()
    : m_impl(new ColorSpace::Impl)
    {
    }
    
    ColorSpace::~ColorSpace()
    {
        delete m_impl;
        m_impl = NULL;
    }
    
    ColorSpaceRcPtr ColorSpace::createEditableCopy() const
    {
        ColorSpaceRcPtr cs = ColorSpace::Create();
        *cs->m_impl = *m_impl;
        return cs;
    }
    
    const char * ColorSpace::getName() const
    {
        return getImpl()->name_.c_str();
    }
    
    void ColorSpace::setName(const char * name)
    {
        getImpl()->name_ = name;
    }
    const char * ColorSpace::getFamily() const
    {
        return getImpl()->family_.c_str();
    }
    
    void ColorSpace::setFamily(const char * family)
    {
        getImpl()->family_ = family;
    }
    
    const char * ColorSpace::getEqualityGroup() const
    {
        return getImpl()->equalityGroup_.c_str();
    }
    
    void ColorSpace::setEqualityGroup(const char * equalityGroup)
    {
        getImpl()->equalityGroup_ = equalityGroup;
    }
    
    const char * ColorSpace::getDescription() const
    {
        return getImpl()->description_.c_str();
    }
    
    void ColorSpace::setDescription(const char * description)
    {
        getImpl()->description_ = description;
    }
    
    BitDepth ColorSpace::getBitDepth() const
    {
        return getImpl()->bitDepth_;
    }
    
    void ColorSpace::setBitDepth(BitDepth bitDepth)
    {
        getImpl()->bitDepth_ = bitDepth;
    }

    bool ColorSpace::hasCategory(const char * category) const
    {
        return getImpl()->findCategory(category)
            != getImpl()->categories_.end();
    }

    void ColorSpace::addCategory(const char * category)
    {
        if(getImpl()->findCategory(category) 
            == getImpl()->categories_.end())
        {
            getImpl()->categories_.push_back(pystring::strip(category));
        }
    }

    void ColorSpace::removeCategory(const char * category)
    {
        getImpl()->removeCategory(category);
    }

    int ColorSpace::getNumCategories() const
    {
        return static_cast<int>(getImpl()->categories_.size());
    }

    const char * ColorSpace::getCategory(int index) const
    {
        if(index<0 || index>=(int)getImpl()->categories_.size()) return nullptr;

        return getImpl()->categories_[index].c_str();
    }

    void ColorSpace::clearCategories()
    {
        getImpl()->categories_.clear();
    }

    bool ColorSpace::isData() const
    {
        return getImpl()->isData_;
    }
    
    void ColorSpace::setIsData(bool val)
    {
        getImpl()->isData_ = val;
    }
    
    Allocation ColorSpace::getAllocation() const
    {
        return getImpl()->allocation_;
    }
    
    void ColorSpace::setAllocation(Allocation allocation)
    {
        getImpl()->allocation_ = allocation;
    }
    
    int ColorSpace::getAllocationNumVars() const
    {
        return static_cast<int>(getImpl()->allocationVars_.size());
    }
    
    void ColorSpace::getAllocationVars(float * vars) const
    {
        if(!getImpl()->allocationVars_.empty())
        {
            memcpy(vars,
                &getImpl()->allocationVars_[0],
                getImpl()->allocationVars_.size()*sizeof(float));
        }
    }
    
    void ColorSpace::setAllocationVars(int numvars, const float * vars)
    {
        getImpl()->allocationVars_.resize(numvars);
        
        if(!getImpl()->allocationVars_.empty())
        {
            memcpy(&getImpl()->allocationVars_[0],
                vars,
                numvars*sizeof(float));
        }
    }
    
    ConstTransformRcPtr ColorSpace::getTransform(ColorSpaceDirection dir) const
    {
        if(dir == COLORSPACE_DIR_TO_REFERENCE)
            return getImpl()->toRefTransform_;
        else if(dir == COLORSPACE_DIR_FROM_REFERENCE)
            return getImpl()->fromRefTransform_;
        
        throw Exception("Unspecified ColorSpaceDirection");
    }
    
    void ColorSpace::setTransform(const ConstTransformRcPtr & transform,
                                  ColorSpaceDirection dir)
    {
        TransformRcPtr transformCopy;
        if(transform) transformCopy = transform->createEditableCopy();
        
        if(dir == COLORSPACE_DIR_TO_REFERENCE)
            getImpl()->toRefTransform_ = transformCopy;
        else if(dir == COLORSPACE_DIR_FROM_REFERENCE)
            getImpl()->fromRefTransform_ = transformCopy;
        else
            throw Exception("Unspecified ColorSpaceDirection");
    }
    
    std::ostream& operator<< (std::ostream& os, const ColorSpace& cs)
    {
        int numVars(cs.getAllocationNumVars());
        std::vector<float> vars(numVars);
        cs.getAllocationVars(&vars[0]);

        os << "<ColorSpace ";
        os << "name=" << cs.getName() << ", ";
        os << "family=" << cs.getFamily() << ", ";
        os << "equalityGroup=" << cs.getEqualityGroup() << ", ";
        os << "bitDepth=" << BitDepthToString(cs.getBitDepth()) << ", ";
        os << "isData=" << BoolToString(cs.isData());
        if (numVars)
        {
            os << ", allocation=" << AllocationToString(cs.getAllocation()) << ", ";
            os << "vars=" << vars[0];
            for (int i = 1; i < numVars; ++i)
            {
                os << " " << vars[i];
            }
        }
        os << ">";
        
        if(cs.getTransform(COLORSPACE_DIR_TO_REFERENCE))
        {
            os << "\n    " << cs.getName() << " --> Reference";
            os << "\n\t" << *cs.getTransform(COLORSPACE_DIR_TO_REFERENCE);
        }
        
        if(cs.getTransform(COLORSPACE_DIR_FROM_REFERENCE))
        {
            os << "\n    Reference --> " << cs.getName();
            os << "\n\t" << *cs.getTransform(COLORSPACE_DIR_FROM_REFERENCE);
        }
        return os;
    }
}
OCIO_NAMESPACE_EXIT



///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

OIIO_ADD_TEST(ColorSpace, category)
{
    OCIO::ColorSpaceRcPtr cs = OCIO::ColorSpace::Create();
    OIIO_CHECK_EQUAL(cs->getNumCategories(), 0);

    OIIO_CHECK_ASSERT(!cs->hasCategory("linear"));
    OIIO_CHECK_ASSERT(!cs->hasCategory("rendering"));
    OIIO_CHECK_ASSERT(!cs->hasCategory("log"));

    OIIO_CHECK_NO_THROW(cs->addCategory("linear"));
    OIIO_CHECK_NO_THROW(cs->addCategory("rendering"));
    OIIO_CHECK_EQUAL(cs->getNumCategories(), 2);

    OIIO_CHECK_ASSERT(cs->hasCategory("linear"));
    OIIO_CHECK_ASSERT(cs->hasCategory("rendering"));
    OIIO_CHECK_ASSERT(!cs->hasCategory("log"));

    OIIO_CHECK_EQUAL(std::string(cs->getCategory(0)), std::string("linear"));
    OIIO_CHECK_EQUAL(std::string(cs->getCategory(1)), std::string("rendering"));
    // Check with an invalid index.
    OIIO_CHECK_NO_THROW(cs->getCategory(2));
    OIIO_CHECK_ASSERT(cs->getCategory(2) == nullptr);

    OIIO_CHECK_NO_THROW(cs->removeCategory("linear"));
    OIIO_CHECK_EQUAL(cs->getNumCategories(), 1);
    OIIO_CHECK_ASSERT(!cs->hasCategory("linear"));
    OIIO_CHECK_ASSERT(cs->hasCategory("rendering"));
    OIIO_CHECK_ASSERT(!cs->hasCategory("log"));

    // Remove a category not in the color space.
    OIIO_CHECK_NO_THROW(cs->removeCategory("log"));
    OIIO_CHECK_EQUAL(cs->getNumCategories(), 1);
    OIIO_CHECK_ASSERT(cs->hasCategory("rendering"));

    OIIO_CHECK_NO_THROW(cs->clearCategories());
    OIIO_CHECK_EQUAL(cs->getNumCategories(), 0);
}

#endif // OCIO_UNIT_TEST

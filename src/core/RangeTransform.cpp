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

#include <OpenColorIO/OpenColorIO.h>

#include "OpBuilders.h"
#include "RangeOps.h"
#include "MathUtils.h"
#include "opdata/OpDataRange.h"


OCIO_NAMESPACE_ENTER
{
    RangeTransformRcPtr RangeTransform::Create()
    {
        return RangeTransformRcPtr(new RangeTransform(), &deleter);
    }
    
    void RangeTransform::deleter(RangeTransform* t)
    {
        delete t;
    }
    
    class RangeTransform::Impl
    {
    public:
        Impl() 
            :   m_data(new OpData::Range)
            ,   m_direction(TRANSFORM_DIR_FORWARD) 
        {
        }

        ~Impl() {}

        Impl& operator= (const Impl & rhs)
        {
            if(this!=&rhs)
            {
                *m_data     = *rhs.m_data;
                m_direction = rhs.m_direction;
            }
            return *this;
        }

        OpData::OpDataRangeRcPtr m_data;
        TransformDirection m_direction;

    private:
        Impl(const Impl&);
    };

    ///////////////////////////////////////////////////////////////////////////
    
    
    
    RangeTransform::RangeTransform()
        : m_impl(new RangeTransform::Impl)
    {
    }
    
    TransformRcPtr RangeTransform::createEditableCopy() const
    {
        RangeTransformRcPtr transform = RangeTransform::Create();
        *transform->m_impl = *m_impl;
        return transform;
    }
    
    RangeTransform::~RangeTransform()
    {
        delete m_impl;
        m_impl = NULL;
    }
    
    RangeTransform& RangeTransform::operator= (const RangeTransform & rhs)
    {
        if (this != &rhs)
        {
            *m_impl = *rhs.m_impl;
        }
        return *this;
    }
    
    TransformDirection RangeTransform::getDirection() const
    {
        return getImpl()->m_direction;
    }
    
    void RangeTransform::setDirection(TransformDirection dir)
    {
        getImpl()->m_direction = dir;
    }
    
    void RangeTransform::validate() const
    {
        if (getImpl()->m_direction != TRANSFORM_DIR_FORWARD
            && getImpl()->m_direction != TRANSFORM_DIR_INVERSE)
        {
            throw Exception("RangeTransform: invalid direction");
        }
        getImpl()->m_data->validate();
    }

    bool RangeTransform::equals(const RangeTransform & other) const
    {
        return *(getImpl()->m_data)   == *(other.getImpl()->m_data)
            && getImpl()->m_direction == other.getImpl()->m_direction;
    }

    void RangeTransform::setMinInValue(double val)
    {
        getImpl()->m_data->setMinInValue(val);
    }

    double RangeTransform::getMinInValue() const
    {
        return getImpl()->m_data->getMinInValue();
    }

    bool RangeTransform::hasMinInValue() const
    {
        return getImpl()->m_data->hasMinInValue();
    }

    void RangeTransform::unsetMinInValue()
    {
        getImpl()->m_data->unsetMinInValue();
    }


    void RangeTransform::setMaxInValue(double val)
    {
        getImpl()->m_data->setMaxInValue(val);
    }

    double RangeTransform::getMaxInValue() const
    {
        return getImpl()->m_data->getMaxInValue();
    }

    bool RangeTransform::hasMaxInValue() const
    {
        return getImpl()->m_data->hasMaxInValue();
    }

    void RangeTransform::unsetMaxInValue()
    {
        getImpl()->m_data->unsetMaxInValue();
    }


    void RangeTransform::setMinOutValue(double val)
    {
        getImpl()->m_data->setMinOutValue(val);
    }

    double RangeTransform::getMinOutValue() const
    {
        return getImpl()->m_data->getMinOutValue();
    }

    bool RangeTransform::hasMinOutValue() const
    {
        return getImpl()->m_data->hasMinOutValue();
    }

    void RangeTransform::unsetMinOutValue()
    {
        getImpl()->m_data->unsetMinOutValue();
    }


    void RangeTransform::setMaxOutValue(double val)
    {
        getImpl()->m_data->setMaxOutValue(val);
    }

    double RangeTransform::getMaxOutValue() const
    {
        return getImpl()->m_data->getMaxOutValue();
    }

    bool RangeTransform::hasMaxOutValue() const
    {
        return getImpl()->m_data->hasMaxOutValue();
    }

    void RangeTransform::unsetMaxOutValue()
    {
        getImpl()->m_data->unsetMaxOutValue();
    }


    std::ostream& operator<< (std::ostream& os, const RangeTransform& t)
    {
        os << "<RangeTransform ";
        os << "direction=" << TransformDirectionToString(t.getDirection());
        if(t.hasMinInValue())  os << ", " << "minInValue="  << t.getMinInValue();
        if(t.hasMaxInValue())  os << ", " << "maxInValue="  << t.getMaxInValue();
        if(t.hasMinOutValue()) os << ", " << "minOutValue=" << t.getMinOutValue();
        if(t.hasMaxOutValue()) os << ", " << "maxOutValue=" << t.getMaxOutValue();
        os << ">";
        return os;
    }



    ///////////////////////////////////////////////////////////////////////////

    void BuildRangeOps(OpRcPtrVec & ops,
                       const Config& /*config*/,
                       const RangeTransform & transform,
                       TransformDirection dir)
    {
        const TransformDirection combinedDir 
            = CombineTransformDirections(dir, transform.getDirection());

        CreateRangeOp(ops, 
                      transform.getMinInValue(), transform.getMaxInValue(), 
                      transform.getMinOutValue(), transform.getMaxOutValue(),
                      combinedDir);
    }
    
}
OCIO_NAMESPACE_EXIT


////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"


OCIO_NAMESPACE_USING


OIIO_ADD_TEST(RangeTransform, Basic)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    OIIO_CHECK_EQUAL(range->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_ASSERT(!range->hasMinInValue());
    OIIO_CHECK_ASSERT(!range->hasMaxInValue());
    OIIO_CHECK_ASSERT(!range->hasMinOutValue());
    OIIO_CHECK_ASSERT(!range->hasMaxOutValue());

    range->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(range->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    range->setMinInValue(-0.5f);
    OIIO_CHECK_EQUAL(range->getMinInValue(), -0.5f);
    OIIO_CHECK_ASSERT(range->hasMinInValue());

    OCIO::RangeTransformRcPtr range2 = OCIO::RangeTransform::Create();
    range2->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    range2->setMinInValue(-0.5f);
    OIIO_CHECK_ASSERT(range2->equals(*range));

    range2->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    range2->unsetMinInValue();
    range2->setMaxInValue(-0.5f);
    range2->setMinOutValue(1.5f);
    range2->setMaxOutValue(4.5f);

    // (Note that the transform would not validate at this point.)

    OIIO_CHECK_ASSERT(!range2->hasMinInValue());
    OIIO_CHECK_EQUAL(range2->getMaxInValue(), -0.5f);
    OIIO_CHECK_EQUAL(range2->getMinOutValue(), 1.5f);
    OIIO_CHECK_EQUAL(range2->getMaxOutValue(), 4.5f);

    range2->setMinInValue(-1.5f);
    OIIO_CHECK_EQUAL(range2->getMinInValue(), -1.5f);
    OIIO_CHECK_EQUAL(range2->getMaxInValue(), -0.5f);
    OIIO_CHECK_EQUAL(range2->getMinOutValue(), 1.5f);
    OIIO_CHECK_EQUAL(range2->getMaxOutValue(), 4.5f);

    OIIO_CHECK_ASSERT(range2->hasMinInValue());
    OIIO_CHECK_ASSERT(range2->hasMaxInValue());
    OIIO_CHECK_ASSERT(range2->hasMinOutValue());
    OIIO_CHECK_ASSERT(range2->hasMaxOutValue());

    range2->unsetMinInValue();
    OIIO_CHECK_ASSERT(!range2->hasMinInValue());
    OIIO_CHECK_ASSERT(range2->hasMaxInValue());
    OIIO_CHECK_ASSERT(range2->hasMinOutValue());
    OIIO_CHECK_ASSERT(range2->hasMaxOutValue());

    range2->unsetMaxInValue();
    OIIO_CHECK_ASSERT(!range2->hasMinInValue());
    OIIO_CHECK_ASSERT(!range2->hasMaxInValue());
    OIIO_CHECK_ASSERT(range2->hasMinOutValue());
    OIIO_CHECK_ASSERT(range2->hasMaxOutValue());

    range2->unsetMinOutValue();
    OIIO_CHECK_ASSERT(!range2->hasMinInValue());
    OIIO_CHECK_ASSERT(!range2->hasMaxInValue());
    OIIO_CHECK_ASSERT(!range2->hasMinOutValue());
    OIIO_CHECK_ASSERT(range2->hasMaxOutValue());

    range2->unsetMaxOutValue();
    OIIO_CHECK_ASSERT(!range2->hasMinInValue());
    OIIO_CHECK_ASSERT(!range2->hasMaxInValue());
    OIIO_CHECK_ASSERT(!range2->hasMinOutValue());
    OIIO_CHECK_ASSERT(!range2->hasMaxOutValue());
}

#endif

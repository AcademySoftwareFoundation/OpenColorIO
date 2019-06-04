/*
Copyright (c) 2018 Autodesk Inc., et al.
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
#include "ops/FixedFunction/FixedFunctionOpData.h"
#include "ops/FixedFunction/FixedFunctionOps.h"


OCIO_NAMESPACE_ENTER
{

namespace
{

FixedFunctionOpData::Style ConvertStyle(FixedFunctionStyle style, TransformDirection dir)
{
    if(dir==TRANSFORM_DIR_UNKNOWN)
    {
        throw Exception(
            "Cannot create FixedFunctionOp with unspecified transform direction.");
    }

    const bool isForward = dir==TRANSFORM_DIR_FORWARD;

    switch(style)
    {
        case FIXED_FUNCTION_ACES_RED_MOD_03:
        {
            if(isForward) return FixedFunctionOpData::ACES_RED_MOD_03_FWD;
            else          return FixedFunctionOpData::ACES_RED_MOD_03_INV;
            break;
        }
        case FIXED_FUNCTION_ACES_RED_MOD_10:
        {
            if(isForward) return FixedFunctionOpData::ACES_RED_MOD_10_FWD;
            else          return FixedFunctionOpData::ACES_RED_MOD_10_INV;
            break;
        }
        case FIXED_FUNCTION_ACES_GLOW_03:
        {
            if(isForward) return FixedFunctionOpData::ACES_GLOW_03_FWD;
            else          return FixedFunctionOpData::ACES_GLOW_03_INV;
            break;
        }
        case FIXED_FUNCTION_ACES_GLOW_10:
        {
            if(isForward) return FixedFunctionOpData::ACES_GLOW_10_FWD;
            else          return FixedFunctionOpData::ACES_GLOW_10_INV;
            break;
        }
        case FIXED_FUNCTION_ACES_DARK_TO_DIM_10:
        {
            if(isForward) return FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD;
            else          return FixedFunctionOpData::ACES_DARK_TO_DIM_10_INV;
            break;
        }
        case FIXED_FUNCTION_REC2100_SURROUND:
        {
            return FixedFunctionOpData::REC2100_SURROUND;
            break;
        }
    }

    std::stringstream ss("Unknown FixedFunction transform style: ");
    ss << style;

    throw Exception(ss.str().c_str());

    return FixedFunctionOpData::ACES_RED_MOD_03_FWD;
}

FixedFunctionStyle ConvertStyle(FixedFunctionOpData::Style style)
{
    switch(style)
    {
        case FixedFunctionOpData::ACES_RED_MOD_03_FWD:
        case FixedFunctionOpData::ACES_RED_MOD_03_INV:
            return FIXED_FUNCTION_ACES_RED_MOD_03;

        case FixedFunctionOpData::ACES_RED_MOD_10_FWD:
        case FixedFunctionOpData::ACES_RED_MOD_10_INV:
            return FIXED_FUNCTION_ACES_RED_MOD_10;

        case FixedFunctionOpData::ACES_GLOW_03_FWD:
        case FixedFunctionOpData::ACES_GLOW_03_INV:
            return FIXED_FUNCTION_ACES_GLOW_03;

        case FixedFunctionOpData::ACES_GLOW_10_FWD:
        case FixedFunctionOpData::ACES_GLOW_10_INV:
            return FIXED_FUNCTION_ACES_GLOW_10;

        case FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD:
        case FixedFunctionOpData::ACES_DARK_TO_DIM_10_INV:
            return FIXED_FUNCTION_ACES_DARK_TO_DIM_10;

        case FixedFunctionOpData::REC2100_SURROUND:
            return FIXED_FUNCTION_REC2100_SURROUND;
    }

    std::stringstream ss("Unknown FixedFunction style: ");
    ss << style;

    throw Exception(ss.str().c_str());

    return FIXED_FUNCTION_ACES_RED_MOD_03;
}

}; // anon


FixedFunctionTransformRcPtr FixedFunctionTransform::Create()
{
    return FixedFunctionTransformRcPtr(new FixedFunctionTransform(), &deleter);
}

void FixedFunctionTransform::deleter(FixedFunctionTransform* t)
{
    delete t;
}

class FixedFunctionTransform::Impl : public FixedFunctionOpData
{
public:
    Impl() 
        :   FixedFunctionOpData()
        ,   m_direction(TRANSFORM_DIR_FORWARD)
    {
    }

    ~Impl() {}

    Impl(const Impl &) = delete;

    Impl& operator=(const Impl & rhs)
    {
        if(this!=&rhs)
        {
            FixedFunctionOpData::operator=(rhs);
            m_direction  = rhs.m_direction;
        }
        return *this;
    }

    bool equals(const Impl & rhs) const
    {
        if(this==&rhs) return true;

        return FixedFunctionOpData::operator==(rhs)
               && m_direction == rhs.m_direction;
    }

    TransformDirection m_direction;
};

///////////////////////////////////////////////////////////////////////////



FixedFunctionTransform::FixedFunctionTransform()
    : m_impl(new FixedFunctionTransform::Impl)
{
}

TransformRcPtr FixedFunctionTransform::createEditableCopy() const
{
    FixedFunctionTransformRcPtr transform = FixedFunctionTransform::Create();
    *transform->m_impl = *m_impl;
    return transform;
}

FixedFunctionTransform::~FixedFunctionTransform()
{
    delete m_impl;
    m_impl = nullptr;
}

FixedFunctionTransform & FixedFunctionTransform::operator= (const FixedFunctionTransform & rhs)
{
    if (this != &rhs)
    {
        *m_impl = *rhs.m_impl;
    }
    return *this;
}

TransformDirection FixedFunctionTransform::getDirection() const
{
    return getImpl()->m_direction;
}

void FixedFunctionTransform::setDirection(TransformDirection dir)
{
    getImpl()->m_direction = dir;
}

void FixedFunctionTransform::validate() const
{
    Transform::validate();
    getImpl()->validate();
}

FixedFunctionStyle FixedFunctionTransform::getStyle() const
{
    return ConvertStyle(getImpl()->getStyle());
}

void FixedFunctionTransform::setStyle(FixedFunctionStyle style)
{
    getImpl()->setStyle(ConvertStyle(style, TRANSFORM_DIR_FORWARD));
}

size_t FixedFunctionTransform::getNumParams() const
{
    return getImpl()->getParams().size();
}

void FixedFunctionTransform::setParams(const double * params, size_t num)
{
    FixedFunctionOpData::Params p(num);
    std::copy(params, params+num, p.begin());
    getImpl()->setParams(p);
}

void FixedFunctionTransform::getParams(double * params) const
{
    const FixedFunctionOpData::Params & p = getImpl()->getParams();
    std::copy(p.cbegin(), p.cend(), params);
}



std::ostream& operator<< (std::ostream & os, const FixedFunctionTransform & t)
{
    os << "<FixedFunction ";
    os << "direction=" << TransformDirectionToString(t.getDirection());
    os << ", style=" << FixedFunctionStyleToString(t.getStyle());

    const size_t numParams = t.getNumParams();
    if(numParams>0)
    {
        FixedFunctionOpData::Params params(numParams, 0.);
        t.getParams(&params[0]);

        os << ", params=" << params[0];
        for (size_t i = 1; i < numParams; ++i)
        {
          os << " " << params[i];
        }
    }

    os << ">";
    return os;
}


///////////////////////////////////////////////////////////////////////////

void BuildFixedFunctionOps(OpRcPtrVec & ops,
                           const Config & /*config*/,
                           const ConstContextRcPtr & /*context*/,
                           const FixedFunctionTransform & transform,
                           TransformDirection dir)
{
    const TransformDirection combinedDir 
        = CombineTransformDirections(dir, transform.getDirection());

    const size_t numParams = transform.getNumParams();
    FixedFunctionOpData::Params params(numParams, 0.);
    if(numParams>0) transform.getParams(&params[0]);

    CreateFixedFunctionOp(ops, params, ConvertStyle(transform.getStyle(), combinedDir));
}


}
OCIO_NAMESPACE_EXIT


////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"


OCIO_NAMESPACE_USING


OIIO_ADD_TEST(FixedFunctionTransform, basic)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    OIIO_CHECK_EQUAL(func->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(func->getStyle(), OCIO::FIXED_FUNCTION_ACES_RED_MOD_03);
    OIIO_CHECK_EQUAL(func->getNumParams(), 0);
    OIIO_CHECK_NO_THROW(func->validate());

    OIIO_CHECK_NO_THROW(func->setDirection(OCIO::TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(func->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(func->getNumParams(), 0);
    OIIO_CHECK_NO_THROW(func->validate());

    OIIO_CHECK_NO_THROW(func->setStyle(OCIO::FIXED_FUNCTION_ACES_RED_MOD_10));
    OIIO_CHECK_EQUAL(func->getStyle(), OCIO::FIXED_FUNCTION_ACES_RED_MOD_10);
    OIIO_CHECK_EQUAL(func->getNumParams(), 0);
    OIIO_CHECK_NO_THROW(func->validate());

    OIIO_CHECK_NO_THROW(func->setStyle(OCIO::FIXED_FUNCTION_REC2100_SURROUND));
    OIIO_CHECK_THROW_WHAT(func->validate(), OCIO::Exception, 
                          "The style 'REC2100_Surround' must have "
                          "one parameter but 0 found.");

    OIIO_CHECK_EQUAL(func->getNumParams(), 0);
    const double values[1] = { 1. };
    OIIO_CHECK_NO_THROW(func->setParams(&values[0], 1));
    OIIO_CHECK_EQUAL(func->getNumParams(), 1);
    double results[1] = { 0. };
    OIIO_CHECK_NO_THROW(func->getParams(&results[0]));
    OIIO_CHECK_EQUAL(results[0], values[0]);

    OIIO_CHECK_NO_THROW(func->validate());

    OIIO_CHECK_NO_THROW(func->setStyle(OCIO::FIXED_FUNCTION_ACES_DARK_TO_DIM_10));
    OIIO_CHECK_THROW_WHAT(func->validate(), OCIO::Exception, 
                          "The style 'ACES_DarkToDim10 (Forward)' must have "
                          "zero parameters but 1 found.");
}

#endif

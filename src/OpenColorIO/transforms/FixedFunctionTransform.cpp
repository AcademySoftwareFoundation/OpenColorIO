// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cstring>

#include <OpenColorIO/OpenColorIO.h>

#include "OpBuilders.h"
#include "ops/FixedFunction/FixedFunctionOpData.h"


OCIO_NAMESPACE_ENTER
{

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

FormatMetadata & FixedFunctionTransform::getFormatMetadata()
{
    return m_impl->getFormatMetadata();
}

const FormatMetadata & FixedFunctionTransform::getFormatMetadata() const
{
    return m_impl->getFormatMetadata();
}

FixedFunctionStyle FixedFunctionTransform::getStyle() const
{
    return FixedFunctionOpData::ConvertStyle(getImpl()->getStyle());
}

void FixedFunctionTransform::setStyle(FixedFunctionStyle style)
{
    getImpl()->setStyle(FixedFunctionOpData::ConvertStyle(style));
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

}
OCIO_NAMESPACE_EXIT


////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

#include "UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(FixedFunctionTransform, basic)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    OCIO_CHECK_EQUAL(func->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(func->getStyle(), OCIO::FIXED_FUNCTION_ACES_RED_MOD_03);
    OCIO_CHECK_EQUAL(func->getNumParams(), 0);
    OCIO_CHECK_NO_THROW(func->validate());

    OCIO_CHECK_NO_THROW(func->setDirection(OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_EQUAL(func->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(func->getNumParams(), 0);
    OCIO_CHECK_NO_THROW(func->validate());

    OCIO_CHECK_NO_THROW(func->setStyle(OCIO::FIXED_FUNCTION_ACES_RED_MOD_10));
    OCIO_CHECK_EQUAL(func->getStyle(), OCIO::FIXED_FUNCTION_ACES_RED_MOD_10);
    OCIO_CHECK_EQUAL(func->getNumParams(), 0);
    OCIO_CHECK_NO_THROW(func->validate());

    OCIO_CHECK_NO_THROW(func->setStyle(OCIO::FIXED_FUNCTION_REC2100_SURROUND));
    OCIO_CHECK_THROW_WHAT(func->validate(), OCIO::Exception,
                          "The style 'REC2100_Surround' must have "
                          "one parameter but 0 found.");

    OCIO_CHECK_EQUAL(func->getNumParams(), 0);
    const double values[1] = { 1. };
    OCIO_CHECK_NO_THROW(func->setParams(&values[0], 1));
    OCIO_CHECK_EQUAL(func->getNumParams(), 1);
    double results[1] = { 0. };
    OCIO_CHECK_NO_THROW(func->getParams(&results[0]));
    OCIO_CHECK_EQUAL(results[0], values[0]);

    OCIO_CHECK_NO_THROW(func->validate());

    OCIO_CHECK_NO_THROW(func->setStyle(OCIO::FIXED_FUNCTION_ACES_DARK_TO_DIM_10));
    OCIO_CHECK_THROW_WHAT(func->validate(), OCIO::Exception,
                          "The style 'ACES_DarkToDim10 (Forward)' must have "
                          "zero parameters but 1 found.");
}

#endif

// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>

#include <OpenColorIO/OpenColorIO.h>

#include "OpBuilders.h"
#include "ops/gamma/GammaOpData.h"

namespace OCIO_NAMESPACE
{
ExponentTransformRcPtr ExponentTransform::Create()
{
    return ExponentTransformRcPtr(new ExponentTransform(), &deleter);
}

void ExponentTransform::deleter(ExponentTransform* t)
{
    delete t;
}


class ExponentTransform::Impl : public GammaOpData
{
public:
    TransformDirection m_dir;

    Impl()
        :   GammaOpData()
        ,   m_dir(TRANSFORM_DIR_FORWARD)
    {
        setRedParams  ( {1.} );
        setGreenParams( {1.} );
        setBlueParams ( {1.} );
        setAlphaParams( {1.} );
    }

    Impl(const Impl &) = delete;

    ~Impl()
    { }

    Impl& operator= (const Impl & rhs)
    {
        if (this != &rhs)
        {
            GammaOpData::operator=(rhs);
            m_dir = rhs.m_dir;
        }
        return *this;
    }
};

///////////////////////////////////////////////////////////////////////////



ExponentTransform::ExponentTransform()
    : m_impl(new ExponentTransform::Impl)
{
}

TransformRcPtr ExponentTransform::createEditableCopy() const
{
    ExponentTransformRcPtr transform = ExponentTransform::Create();
    *(transform->m_impl) = *m_impl;
    return transform;
}

ExponentTransform::~ExponentTransform()
{
    delete m_impl;
    m_impl = nullptr;
}

ExponentTransform& ExponentTransform::operator= (const ExponentTransform & rhs)
{
    if (this != &rhs)
    {
        *m_impl = *rhs.m_impl;
    }
    return *this;
}

TransformDirection ExponentTransform::getDirection() const
{
    return getImpl()->m_dir;
}

void ExponentTransform::setDirection(TransformDirection dir)
{
    getImpl()->m_dir = dir;
}

void ExponentTransform::validate() const
{
    try
    {
        Transform::validate();
        getImpl()->validate();
    }
    catch(Exception & ex)
    {
        std::string errMsg("ExponentTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }
}

FormatMetadata & ExponentTransform::getFormatMetadata()
{
    return m_impl->getFormatMetadata();
}

const FormatMetadata & ExponentTransform::getFormatMetadata() const
{
    return m_impl->getFormatMetadata();
}

void ExponentTransform::setValue(const double(&vec4)[4])
{
    getImpl()->getRedParams()  [0] = vec4[0];
    getImpl()->getGreenParams()[0] = vec4[1];
    getImpl()->getBlueParams() [0] = vec4[2];
    getImpl()->getAlphaParams()[0] = vec4[3];
}

void ExponentTransform::getValue(double(&vec4)[4]) const
{
    vec4[0] = getImpl()->getRedParams()  [0];
    vec4[1] = getImpl()->getGreenParams()[0];
    vec4[2] = getImpl()->getBlueParams() [0];
    vec4[3] = getImpl()->getAlphaParams()[0];
}

std::ostream& operator<< (std::ostream& os, const ExponentTransform & t)
{
    double value[4];
    t.getValue(value);

    os << "<ExponentTransform ";
    os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";

    os << "value=" << value[0];
    for (int i = 1; i < 4; ++i)
    {
      os << " " << value[i];
    }

    os << ">";
    return os;
}


} // namespace OCIO_NAMESPACE


// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>

#include <OpenColorIO/OpenColorIO.h>

#include "OpBuilders.h"
#include "transforms/ExponentTransform.h"

namespace OCIO_NAMESPACE
{
ExponentTransformRcPtr ExponentTransform::Create()
{
    return ExponentTransformRcPtr(new ExponentTransformImpl(), &ExponentTransformImpl::deleter);
}

void ExponentTransformImpl::deleter(ExponentTransform * t)
{
    delete static_cast<ExponentTransformImpl *>(t);
}

TransformRcPtr ExponentTransformImpl::createEditableCopy() const
{
    TransformRcPtr transform = ExponentTransform::Create();
    dynamic_cast<ExponentTransformImpl*>(transform.get())->data() = data();
    return transform;
}

TransformDirection ExponentTransformImpl::getDirection() const noexcept
{
    if (GammaOpData::BASIC_FWD == data().getStyle())
    {
        return TRANSFORM_DIR_FORWARD;
    }
    else
    {
        return TRANSFORM_DIR_INVERSE;
    }
}

void ExponentTransformImpl::setDirection(TransformDirection dir) noexcept
{
    if (TRANSFORM_DIR_FORWARD == dir)
    {
        data().setStyle(GammaOpData::BASIC_FWD);
    }
    else
    {
        data().setStyle(GammaOpData::BASIC_REV);
    }
}

void ExponentTransformImpl::validate() const
{
    try
    {
        Transform::validate();
        data().validate();
    }
    catch(Exception & ex)
    {
        std::string errMsg("ExponentTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }
}

FormatMetadata & ExponentTransformImpl::getFormatMetadata() noexcept
{
    return data().getFormatMetadata();
}

const FormatMetadata & ExponentTransformImpl::getFormatMetadata() const noexcept
{
    return data().getFormatMetadata();
}

bool ExponentTransformImpl::equals(const ExponentTransform & other) const noexcept
{
    if (this == &other) return true;
    return data() == dynamic_cast<const ExponentTransformImpl*>(&other)->data();
}

void ExponentTransformImpl::setValue(const double(&vec4)[4]) noexcept
{
    data().getRedParams()  [0] = vec4[0];
    data().getGreenParams()[0] = vec4[1];
    data().getBlueParams() [0] = vec4[2];
    data().getAlphaParams()[0] = vec4[3];
}

void ExponentTransformImpl::getValue(double(&vec4)[4]) const noexcept
{
    vec4[0] = data().getRedParams()  [0];
    vec4[1] = data().getGreenParams()[0];
    vec4[2] = data().getBlueParams() [0];
    vec4[3] = data().getAlphaParams()[0];
}

std::ostream & operator<< (std::ostream & os, const ExponentTransform & t)
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


// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>

#include <OpenColorIO/OpenColorIO.h>

#include "transforms/GradingPrimaryTransform.h"

namespace OCIO_NAMESPACE
{

GradingPrimaryTransformRcPtr GradingPrimaryTransform::Create(GradingStyle style)
{
    return GradingPrimaryTransformRcPtr(new GradingPrimaryTransformImpl(style),
                                        &GradingPrimaryTransformImpl::deleter);
}

GradingPrimaryTransformImpl::GradingPrimaryTransformImpl(GradingStyle style)
    : m_data(style)
{
}

void GradingPrimaryTransformImpl::deleter(GradingPrimaryTransform * t)
{
    delete static_cast<GradingPrimaryTransformImpl *>(t);
}

TransformRcPtr GradingPrimaryTransformImpl::createEditableCopy() const
{
    GradingPrimaryTransformRcPtr transform = GradingPrimaryTransform::Create(getStyle());
    dynamic_cast<GradingPrimaryTransformImpl*>(transform.get())->data() = data();
    return transform;
}

TransformDirection GradingPrimaryTransformImpl::getDirection() const noexcept
{
    return data().getDirection();
}

void GradingPrimaryTransformImpl::setDirection(TransformDirection dir) noexcept
{
    data().setDirection(dir);
}

void GradingPrimaryTransformImpl::validate() const
{
    try
    {
        Transform::validate();
        data().validate();
    }
    catch(Exception & ex)
    {
        std::string errMsg("GradingPrimaryTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }
}

FormatMetadata & GradingPrimaryTransformImpl::getFormatMetadata() noexcept
{
    return data().getFormatMetadata();
}

const FormatMetadata & GradingPrimaryTransformImpl::getFormatMetadata() const noexcept
{
    return data().getFormatMetadata();
}


bool GradingPrimaryTransformImpl::equals(const GradingPrimaryTransform & other) const noexcept
{
    if (this == &other) return true;
    return data() == dynamic_cast<const GradingPrimaryTransformImpl*>(&other)->data();
}

GradingStyle GradingPrimaryTransformImpl::getStyle() const noexcept
{
    return data().getStyle();
}

void GradingPrimaryTransformImpl::setStyle(GradingStyle style) noexcept
{
    data().setStyle(style);
}

const GradingPrimary & GradingPrimaryTransformImpl::getValue() const
{
    return data().getValue();
}

void GradingPrimaryTransformImpl::setValue(const GradingPrimary & values)
{
    data().setValue(values);
}

bool GradingPrimaryTransformImpl::isDynamic() const noexcept
{
    return data().isDynamic();
}

void GradingPrimaryTransformImpl::makeDynamic() noexcept
{
    data().getDynamicPropertyInternal()->makeDynamic();
}

void GradingPrimaryTransformImpl::makeNonDynamic() noexcept
{
    data().getDynamicPropertyInternal()->makeNonDynamic();
}

std::ostream& operator<< (std::ostream & os, const GradingPrimaryTransform & t) noexcept
{
    os << "<GradingPrimaryTransform ";
    os << "direction=" << TransformDirectionToString(t.getDirection());
    os << ", style=" << GradingStyleToString(t.getStyle());
    os << ", values=" << t.getValue();
    if (t.isDynamic())
    {
        os << ", dynamic";
    }
    os << ">";
    return os;
}

std::ostream & operator<<(std::ostream & os, const GradingRGBM & rgbm)
{
    os << "<r=" << rgbm.m_red << ", g=" << rgbm.m_green
       << ", b=" << rgbm.m_blue << ", m=" << rgbm.m_master << ">";
    return os;
}

std::ostream & operator<<(std::ostream & os, const GradingPrimary & prim)
{
    os << "<brightness="   << prim.m_brightness;
    os << ", contrast="     << prim.m_contrast  ;
    os << ", gamma="        << prim.m_gamma     ;
    os << ", offset="       << prim.m_offset    ;
    os << ", exposure="     << prim.m_exposure  ;
    os << ", lift="         << prim.m_lift      ;
    os << ", gain="         << prim.m_gain      ;
    os << ", saturation="   << prim.m_saturation;
    os << ", pivot=<contrast=" << prim.m_pivot     ;
    os <<         ", black="   << prim.m_pivotBlack;
    os <<         ", white="   << prim.m_pivotWhite;
    os <<         ">";
    if (prim.m_clampBlack != GradingPrimary::NoClampBlack())
    {
        os << ", clampBlack=" << prim.m_clampBlack;
    }
    if (prim.m_clampWhite != GradingPrimary::NoClampWhite())
    {
        os << ", clampWhite=" << prim.m_clampWhite;
    }
    os << ">";

    return os;
}

} // namespace OCIO_NAMESPACE

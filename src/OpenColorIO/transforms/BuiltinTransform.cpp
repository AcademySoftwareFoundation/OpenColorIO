// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "transforms/builtins/BuiltinTransformRegistry.h"
#include "transforms/BuiltinTransform.h"
#include "Platform.h"


namespace OCIO_NAMESPACE
{

BuiltinTransformRcPtr BuiltinTransform::Create()
{
    return BuiltinTransformRcPtr(new BuiltinTransformImpl(), &BuiltinTransformImpl::deleter);
}

void BuiltinTransformImpl::deleter(BuiltinTransform * t)
{
    delete static_cast<BuiltinTransformImpl *>(t);
}

TransformRcPtr BuiltinTransformImpl::createEditableCopy() const
{
    BuiltinTransformRcPtr transform = BuiltinTransform::Create();

    transform->setDirection(getDirection());
    transform->setStyle(getStyle());

    return transform;
}

TransformDirection BuiltinTransformImpl::getDirection() const noexcept
{
    return m_direction;
}

void BuiltinTransformImpl::setDirection(TransformDirection dir) noexcept
{
    m_direction = dir;
}

const char * BuiltinTransformImpl::getStyle() const noexcept
{
    return BuiltinTransformRegistry::Get()->getBuiltinStyle(m_transformIndex);
}

const char * BuiltinTransformImpl::getDescription() const noexcept
{
    return BuiltinTransformRegistry::Get()->getBuiltinDescription(m_transformIndex);
}

size_t BuiltinTransformImpl::getTransformIndex() const noexcept
{
    return m_transformIndex;
}

void BuiltinTransformImpl::setStyle(const char * style)
{
    for (size_t index = 0; index < BuiltinTransformRegistry::Get()->getNumBuiltins(); ++index)
    {
        if (0 == Platform::Strcasecmp(style, BuiltinTransformRegistry::Get()->getBuiltinStyle(index)))
        {
            m_transformIndex = index;
            return;
        }
    }

    std::ostringstream oss;
    oss << "BuiltinTransform: invalid built-in transform style '" << style << "'.";

    throw Exception(oss.str().c_str());
}


void BuildBuiltinOps(OpRcPtrVec & ops,
                     const BuiltinTransform & transform,
                     TransformDirection dir)
{
    const TransformDirection combinedDir = CombineTransformDirections(dir, transform.getDirection());

    const BuiltinTransformImpl * pImpl = dynamic_cast<const BuiltinTransformImpl*>(&transform);

    CreateBuiltinTransformOps(ops, pImpl->getTransformIndex(), combinedDir);
}


std::ostream & operator<< (std::ostream & os, const BuiltinTransform & t) noexcept
{
    os << "<BuiltinTransform"
       << " direction = " << TransformDirectionToString(t.getDirection())
       << ", style = " << t.getStyle()
       << ">";
    return os;
}



} // namespace OCIO_NAMESPACE

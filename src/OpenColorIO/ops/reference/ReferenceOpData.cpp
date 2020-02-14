// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "ops/reference/ReferenceOpData.h"
#include "Platform.h"
#include "pystring/pystring.h"
#include "transforms/FileTransform.h"

namespace OCIO_NAMESPACE
{

ReferenceOpData::ReferenceOpData()
    : OpData()
{
}

ReferenceOpData::~ReferenceOpData()
{
}

void ReferenceOpData::validate() const
{
}

bool ReferenceOpData::isNoOp() const
{
    return false;
}

bool ReferenceOpData::isIdentity() const
{
    return false;
}

bool ReferenceOpData::hasChannelCrosstalk() const
{
    return true;
}

bool ReferenceOpData::operator==(const OpData& other) const
{
    if (!OpData::operator==(other)) return false;

    const ReferenceOpData* rop = static_cast<const ReferenceOpData*>(&other);

    if (m_referenceStyle != rop->m_referenceStyle) return false;
    if (m_direction != rop->m_direction) return false;
    if (m_referenceStyle == REF_PATH)
    {
        if (m_path != rop->m_path) return false;
    }
    else
    {
        if (m_alias != rop->m_alias) return false;
    }

    return true;
}

void ReferenceOpData::finalize()
{
    AutoMutex lock(m_mutex);

    std::ostringstream cacheIDStream;
    if (!getID().empty())
    {
        cacheIDStream << getID() << " ";
    }

    cacheIDStream << m_referenceStyle << " ";
    cacheIDStream << TransformDirectionToString(m_direction) << " ";
    if (m_referenceStyle == REF_PATH)
    {
        cacheIDStream << m_path << " ";
    }
    else
    {
        cacheIDStream << m_alias << " ";
    }

    m_cacheID = cacheIDStream.str();
}

} // namespace OCIO_NAMESPACE


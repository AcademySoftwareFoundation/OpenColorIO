// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <memory>

#include <OpenColorIO/OpenColorIO.h>

#include "Mutex.h"
#include "ops/matrix/MatrixOp.h"
#include "OpBuilders.h"
#include "Platform.h"
#include "transforms/builtins/ACES.h"
#include "transforms/builtins/ArriCameras.h"
#include "transforms/builtins/BuiltinTransformRegistry.h"
#include "transforms/builtins/CanonCameras.h"
#include "transforms/builtins/Displays.h"
#include "transforms/builtins/PanasonicCameras.h"
#include "transforms/builtins/RedCameras.h"
#include "transforms/builtins/SonyCameras.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{

namespace
{

static BuiltinTransformRegistryRcPtr globalRegistry;
static Mutex globalRegistryMutex;

} // anon.

ConstBuiltinTransformRegistryRcPtr BuiltinTransformRegistry::Get() noexcept
{
    AutoMutex guard(globalRegistryMutex);

    if (!globalRegistry)
    {
        globalRegistry = std::make_shared<BuiltinTransformRegistryImpl>();
        DynamicPtrCast<BuiltinTransformRegistryImpl>(globalRegistry)->registerAll();
    }

    return globalRegistry;
}

void BuiltinTransformRegistryImpl::addBuiltin(const char * style, const char * description, OpCreator creator)
{
    BuiltinData data{ style, description ? description : "", creator };

    for (auto & builtin : m_builtins)
    {
        if (0==Platform::Strcasecmp(data.m_style.c_str(), builtin.m_style.c_str()))
        {
            builtin = data;
            return;
        }
    }

    m_builtins.push_back(data);
}

size_t BuiltinTransformRegistryImpl::getNumBuiltins() const noexcept
{
    return m_builtins.size();
}

const char * BuiltinTransformRegistryImpl::getBuiltinStyle(size_t index) const
{
    if (index >= m_builtins.size())
    {
        throw Exception("Invalid index.");
    }

    return m_builtins[index].m_style.c_str();
}

const char * BuiltinTransformRegistryImpl::getBuiltinDescription(size_t index) const
{
    if (index >= m_builtins.size())
    {
        throw Exception("Invalid index.");
    }

    return m_builtins[index].m_description.c_str();
}

void BuiltinTransformRegistryImpl::createOps(size_t index, OpRcPtrVec & ops) const
{
    if (index >= m_builtins.size())
    {
        throw Exception("Invalid index.");
    }
    
    m_builtins[index].m_creator(ops);
}

void BuiltinTransformRegistryImpl::registerAll() noexcept
{
    m_builtins.clear();

    m_builtins.push_back({"IDENTITY", "", [](OpRcPtrVec & ops) -> void
                                            {
                                                CreateIdentityMatrixOp(ops);
                                            } } );

    // ACES support.
    ACES::RegisterAll(*this);

    // Camera support.
    CAMERA::ARRI::RegisterAll(*this);
    CAMERA::CANON::RegisterAll(*this);
    CAMERA::PANASONIC::RegisterAll(*this);
    CAMERA::RED::RegisterAll(*this);
    CAMERA::SONY::RegisterAll(*this);

    // Display support.
    DISPLAY::RegisterAll(*this);
}


void CreateBuiltinTransformOps(OpRcPtrVec & ops, size_t nameIndex, TransformDirection direction)
{
    if (nameIndex > BuiltinTransformRegistry::Get()->getNumBuiltins())
    {
        throw Exception("Invalid built-in transform name.");
    }

    const BuiltinTransformRegistryImpl * registry
        = dynamic_cast<const BuiltinTransformRegistryImpl *>(BuiltinTransformRegistry::Get().get());

    switch (direction)
    {
    case TRANSFORM_DIR_FORWARD:
    {
        registry->createOps(nameIndex, ops);
        break;
    }
    case TRANSFORM_DIR_INVERSE:
    {
        OpRcPtrVec tmp;
        registry->createOps(nameIndex, tmp);

        OpRcPtrVec t = tmp.invert();
        ops.insert(ops.end(), t.begin(), t.end());
        break;
    }
    }
}


} // namespace OCIO_NAMESPACE

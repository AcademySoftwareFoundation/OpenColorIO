// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_BUILTIN_TRANSFORM_REGISTRY_H
#define INCLUDED_OCIO_BUILTIN_TRANSFORM_REGISTRY_H


#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"


namespace OCIO_NAMESPACE
{

class BuiltinTransformRegistryImpl : public BuiltinTransformRegistry
{
    using OpCreator = std::function<void(OpRcPtrVec & ops)>;

    struct BuiltinData
    {
        BuiltinData(const char * style, const char * description, OpCreator creator)
            : m_style(style ? style : "")
            , m_description(description ? description : "")
            , m_creator(creator)
        {
        }
        BuiltinData() = delete;
        BuiltinData(const BuiltinData & o) = default;
        BuiltinData(BuiltinData && o) noexcept
            : m_style(std::move(o.m_style))
            , m_description(std::move(o.m_description))
            , m_creator(std::move(o.m_creator))
        {
        }
        BuiltinData & operator= (const BuiltinData & o)
        {
            m_style = o.m_style;
            m_description = o.m_description;
            m_creator = o.m_creator;
            return *this;
        }
        std::string m_style;       // The built-in transform style.
        std::string m_description; // The optional built-in transform description.
        OpCreator   m_creator;     // Functor to create the op(s).
    };
    using Builtins = std::vector<BuiltinData>;

public:
    BuiltinTransformRegistryImpl() = default;
    BuiltinTransformRegistryImpl(const BuiltinTransformRegistryImpl &) = delete;
    BuiltinTransformRegistryImpl & operator=(const BuiltinTransformRegistryImpl &) = delete;
    ~BuiltinTransformRegistryImpl() = default;

    size_t getNumBuiltins() const noexcept override;
    const char * getBuiltinStyle(size_t index) const override;
    const char * getBuiltinDescription(size_t index) const override;

    void addBuiltin(const char * style, const char * description, OpCreator creator);

    void createOps(size_t index, OpRcPtrVec & ops) const;

    void registerAll() noexcept;

private:
    Builtins m_builtins;
};

void CreateBuiltinTransformOps(OpRcPtrVec & ops, size_t nameIndex, TransformDirection direction);


} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_BUILTIN_TRANSFORM_REGISTRY_H

// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_NAMEDTRANSFORM_H
#define INCLUDED_OCIO_NAMEDTRANSFORM_H

#include <string>

#include <OpenColorIO/OpenColorIO.h>

#include "TokensManager.h"

namespace OCIO_NAMESPACE
{

class NamedTransformImpl : public NamedTransform
{
public:
    NamedTransformImpl() = default;
    ~NamedTransformImpl() = default;

    NamedTransformRcPtr createEditableCopy() const override;

    const char * getName() const noexcept override;
    void setName(const char * name) noexcept override;

    size_t getNumAliases() const noexcept override;
    const char * getAlias(size_t idx) const noexcept override;
    void addAlias(const char * alias) noexcept override;
    void removeAlias(const char * alias) noexcept override;
    void clearAliases() noexcept override;

    const char * getFamily() const noexcept override;
    void setFamily(const char * family) noexcept override;

    const char * getDescription() const noexcept override;
    void setDescription(const char * description) noexcept override;

    bool hasCategory(const char * category) const noexcept override;
    void addCategory(const char * category) noexcept override;
    void removeCategory(const char * category) noexcept override;
    int getNumCategories() const noexcept override;
    const char * getCategory(int index) const noexcept override;
    void clearCategories() noexcept override;

    const char * getEncoding() const noexcept override;
    void setEncoding(const char * encoding) noexcept override;

    ConstTransformRcPtr getTransform(TransformDirection dir) const override;
    void setTransform(const ConstTransformRcPtr & transform, TransformDirection dir) override;

    // Will create the transform from the inverse direction if the transform for requested
    // direction is missing.
    static ConstTransformRcPtr GetTransform(const ConstNamedTransformRcPtr & nt,
                                            TransformDirection dir);

    static void Deleter(NamedTransform * nt);

private:
    std::string m_name;
    StringUtils::StringVec m_aliases;
    ConstTransformRcPtr m_forwardTransform;
    ConstTransformRcPtr m_inverseTransform;

    std::string m_family;
    std::string m_description;
    TokensManager m_categories;
    std::string m_encoding;
};

ConstTransformRcPtr GetTransform(const ConstNamedTransformRcPtr & src,
                                 const ConstNamedTransformRcPtr & dst);

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_NAMEDTRANSFORM_H

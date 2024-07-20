// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "BuiltinConfigUtils.h"

namespace OCIO_NAMESPACE
{
    namespace BUILTIN_CONFIG_UTILS
    {

    void AddColorSpace(ConfigRcPtr& cfg
        , const char* name
        , ReferenceSpaceType type
        , const char** aliases
        , BitDepth bitDepth
        , const char** categories
        , const char* encoding
        , const char* eqGroup
        , const char* family
        , bool isData
        , ConstTransformRcPtr trFrom
        , ConstTransformRcPtr trTo
        , const char* desc)
    {
        auto out = ColorSpace::Create(type);

        if (aliases)
        {
            for (int i = 0;; ++i)
            {
                const char* alias = aliases[i];
                if (!alias || !*alias)
                    break;
                out->addAlias(alias);
            }
        }

        if (categories)
        {
            for (int i = 0;; ++i)
            {
                const char* cat = categories[i];
                if (!cat || !*cat)
                    break;
                out->addCategory(cat);
            }
        }

        out->setBitDepth(bitDepth);
        out->setDescription(desc);
        out->setEncoding(encoding);
        out->setEqualityGroup(eqGroup);
        out->setFamily(family);
        out->setName(name);
        out->setIsData(isData);
        if (trFrom)
            out->setTransform(trFrom, COLORSPACE_DIR_FROM_REFERENCE);
        if (trTo)
            out->setTransform(trTo, COLORSPACE_DIR_TO_REFERENCE);

        cfg->addColorSpace(out);
    }

    void AddNamedTramsform(ConfigRcPtr& cfg
        , const char* name
        , const char** aliases
        , const char** categories
        , const char* encoding
        , const char* family
        , ConstTransformRcPtr trFwd
        , ConstTransformRcPtr trInv
        , const char* desc)    
    {
        auto out = NamedTransform::Create();

        out->setName(name);
        out->setDescription(desc);
        out->setEncoding(encoding);
        out->setFamily(family);
        if (trFwd)
            out->setTransform(trFwd, TRANSFORM_DIR_FORWARD);
        if (trInv)
            out->setTransform(trInv, TRANSFORM_DIR_INVERSE);

        if (aliases)
        {
            for (int i = 0;; ++i)
            {
                const char* alias = aliases[i];
                if (!alias || !*alias)
                    break;
                out->addAlias(alias);
            }
        }

        if (categories)
        {
            for (int i = 0;; ++i)
            {
                const char* cat = categories[i];
                if (!cat || !*cat)
                    break;
                out->addCategory(cat);
            }
        }

        cfg->addNamedTransform(out);
    }

    }; // namespace OCIO_BUILTIN_CONFIG_UTILS
}; // namespace

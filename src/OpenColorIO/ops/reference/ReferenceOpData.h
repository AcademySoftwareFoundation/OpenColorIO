// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_REFERENCEOPDATA_H
#define INCLUDED_OCIO_REFERENCEOPDATA_H

#include <list>
#include <string>
#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"

namespace OCIO_NAMESPACE
{

enum ReferenceStyle
{
    // Reference is either a full path or relative path.
    REF_PATH,
    // An alias is a way of referring to a transform defined in the
    // synColorConfig.xml file. This feature is not fully implemented in OCIO.
    REF_ALIAS
};

class ReferenceOpData;
typedef OCIO_SHARED_PTR<ReferenceOpData> ReferenceOpDataRcPtr;
typedef OCIO_SHARED_PTR<const ReferenceOpData> ConstReferenceOpDataRcPtr;

class ReferenceOpData : public OpData
{
public:
    ReferenceOpData();
    ~ReferenceOpData();

    // Validate the state of the instance and initialize private members.
    void validate() const override;

    Type getType() const override { return ReferenceType; }

    bool isNoOp() const override;
    bool isIdentity() const override;

    bool hasChannelCrosstalk() const override;

    bool operator==(const OpData& other) const override;

    std::string getCacheID() const override;

    ReferenceStyle getReferenceStyle() const
    {
        return m_referenceStyle;
    }

    const std::string & getPath() const
    {
        return m_path;
    }

    void setPath(const std::string & path)
    {
        m_referenceStyle = REF_PATH;
        m_path = path;
    }

    const std::string & getAlias() const
    {
        return m_alias;
    }

    void setAlias(const std::string & alias)
    {
        m_referenceStyle = REF_ALIAS;
        m_alias = alias;
    }

    TransformDirection getDirection() const
    {
        return m_direction;
    }

    void setDirection(TransformDirection dir)
    {
        m_direction = dir;
    }

private:
    ReferenceStyle m_referenceStyle = REF_PATH;

    std::string m_path;
    std::string m_alias;

    TransformDirection m_direction = TRANSFORM_DIR_FORWARD;
};


} // namespace OCIO_NAMESPACE


#endif
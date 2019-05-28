/*
Copyright (c) 2019 Autodesk Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef INCLUDED_OCIO_REFERENCEOPDATA_H
#define INCLUDED_OCIO_REFERENCEOPDATA_H

#include <list>
#include <string>
#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"

OCIO_NAMESPACE_ENTER
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

    virtual void finalize() override;

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


}
OCIO_NAMESPACE_EXIT


#endif
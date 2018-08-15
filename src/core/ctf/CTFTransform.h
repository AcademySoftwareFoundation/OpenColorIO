/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
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


#ifndef INCLUDED_OCIO_CTFTRANSFORM_H
#define INCLUDED_OCIO_CTFTRANSFORM_H

#include <OpenColorIO/OpenColorIO.h>

#include <string>

#include "../opdata/OpDataVec.h"
#include "../opdata/OpDataMetadata.h"
#include "../opdata/OpDataDescriptions.h"


OCIO_NAMESPACE_ENTER
{

// Private namespace to the CTF sub-directory
namespace CTF
{
// Private namespace for the CTF reader utils
namespace Reader
{
class Transform
{
public:
    Transform();

    ~Transform()
    {
    }
    const std::string& getId() const
    {
        return m_id;
    }
    void setId(const char* id)
    {
        m_id = id;
    }
    const std::string& getName() const
    {
        return m_name;
    }
    void setName(const char* name)
    {
        m_name = name;
    }
    const std::string& getInverseOfId() const
    {
        return m_inverseOfId;
    }
    void setInverseOfId(const char* id)
    {
        m_inverseOfId = id;
    }
    OpData::Metadata& getInfo()
    {
        return m_info;
    }
    const OpData::OpDataVec & getOps() const
    {
        return m_ops;
    }
    OpData::OpDataVec & getOps()
    {
        return m_ops;
    }
    const OpData::Descriptions& getDescriptions() const
    {
        return m_descriptions;
    }
    OpData::Descriptions& getDescriptions()
    {
        return m_descriptions;
    }

    const std::string& getInputDescriptor() const
    {
        return m_inDescriptor;
    }

    void setInputDescriptor(const std::string& in)
    {
        m_inDescriptor = in;
    }

    const std::string& getOutputDescriptor() const
    {
        return m_outDescriptor;
    }

    void setOutputDescriptor(const std::string& out)
    {
        m_outDescriptor = out;
    }

    void setCTFVersion(const Version & ver);
    void setCLFVersion(const Version & ver);

    const Version & getCTFVersion() const;

    void validate();

private:
    std::string m_id;
    std::string m_name;
    std::string m_inverseOfId;
    std::string m_inDescriptor;
    std::string m_outDescriptor;
    OpData::Metadata m_info;
    OpData::OpDataVec m_ops;
    OpData::Descriptions m_descriptions;
                
    // CTF version used even for CLF files.
    // CLF versions <= 2.0 are interpreted as CTF version 1.7
    Version m_version;

    // Orginal CLF version (for reference)
    Version m_versionCLF;

};

typedef OCIO_SHARED_PTR<Transform> TransformPtr;

} // exit Reader namespace
} // exit CTF namespace
}
OCIO_NAMESPACE_EXIT

#endif

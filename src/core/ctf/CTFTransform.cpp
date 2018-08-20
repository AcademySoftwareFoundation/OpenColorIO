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

#include "CTFTransform.h"
#include "../opdata/OpData.h"
#include "../Platform.h"
#include <sstream>

OCIO_NAMESPACE_ENTER
{
// Private namespace to the CTF sub-directory
namespace CTF
{
void Version::ReadVersion(const std::string & versionString, Version & versionOut)
{
    int numDot = 0;
    int numInt = 0;
    bool canBeDot = false;
    std::string::const_iterator it = versionString.begin();
    while (it != versionString.end())
    {
        if (::isdigit(*it))
        {
            numInt = numDot + 1;
            canBeDot = true;
            ++it;
        }
        else if (it[0] == '.' && canBeDot)
        {
            canBeDot = false;
            numDot += 1;
            ++it;
        }
        else
        {
            break;
        }
    }
    if (versionString.empty()
        || it != versionString.end()
        || numInt == 0
        || numInt > 3
        || numInt == numDot)
    {
        std::ostringstream os;
        os << "'";
        os << versionString;
        os << "' is not a valid version. ";
        os << "Expecting MAJOR[.MINOR[.REVISION]] ";
        throw Exception(os.str().c_str());
    }

    versionOut.m_major = 0;
    versionOut.m_minor = 0;
    versionOut.m_revision = 0;

    sscanf(versionString.c_str(), "%d.%d.%d",
            &versionOut.m_major,
            &versionOut.m_minor,
            &versionOut.m_revision);
}

Version & Version::operator=(const Version &rhs)
{
    if (this != &rhs)
    {
        m_major = rhs.m_major;
        m_minor = rhs.m_minor;
        m_revision = rhs.m_revision;
    }
    return *this;
}

bool Version::operator==(const Version &rhs) const
{
    if (this == &rhs) return true;

    return m_major == rhs.m_major
        && m_minor == rhs.m_minor
        && m_revision == rhs.m_revision;
}

bool Version::operator<=(const Version &rhs) const
{
    if (*this == rhs)
    {
        return true;
    }
    return *this < rhs;
}

bool Version::operator<(const Version &rhs) const
{
    if (this == &rhs) return false;

    if (m_major < rhs.m_major)
    {
        return true;
    }
    else if (m_major > rhs.m_major)
    {
        return false;
    }
    else
    {
        if (m_minor < rhs.m_minor)
        {
            return true;
        }
        else if (m_minor > rhs.m_minor)
        {
            return false;
        }
        else
        {
            if (m_revision < rhs.m_revision)
            {
                return true;
            }
            return false;
        }
    }
}

// Private namespace for the xml reader utils
namespace Reader
{
Transform::Transform()
    : m_info("Info")
    , m_version(CTF_PROCESS_LIST_VERSION)
    , m_versionCLF(0, 0)
{
}

void Transform::setCTFVersion(const Version& ver)
{
    m_version = ver;
}

void Transform::setCLFVersion(const Version& ver)
{
    m_versionCLF = ver;
}

const Version& Transform::getCTFVersion() const
{
    return m_version;
}

void Transform::validate()
{
    m_ops.validate();
}

} // exit Reader namespace
} // exit CTF namespace
}
OCIO_NAMESPACE_EXIT

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "../UnitTest.h"
#include "../opdata/OpDataMatrix.h"

OIIO_ADD_TEST(CTFTransform, TestVersionRead)
{
    {
        const OCIO::CTF::Version version1(1, 2, 3);
        const OCIO::CTF::Version version2(1, 2, 3);
        OIIO_CHECK_EQUAL(version1, version2);
        {
            const OCIO::CTF::Version version3(0, 0, 1);
            OIIO_CHECK_ASSERT(false == (version1 == version3));
            OIIO_CHECK_ASSERT(version3 < version1);
        }
        {
            const OCIO::CTF::Version version3(0, 1, 0);
            OIIO_CHECK_ASSERT(false == (version1 == version3));
            OIIO_CHECK_ASSERT(version3 < version1);
        }
        {
            const OCIO::CTF::Version version3(1, 0, 0);
            OIIO_CHECK_ASSERT(false == (version1 == version3));
            OIIO_CHECK_ASSERT(version3 < version1);
        }
        {
            const OCIO::CTF::Version version3(1, 2, 0);
            OIIO_CHECK_ASSERT(false == (version1 == version3));
            OIIO_CHECK_ASSERT(version3 < version1);
        }
        {
            const OCIO::CTF::Version version3(1, 2, 2);
            OIIO_CHECK_ASSERT(false == (version1 == version3));
            OIIO_CHECK_ASSERT(version3 < version1);
        }
    }

    OCIO::CTF::Version versionRead;
    {
        OIIO_CHECK_NO_THROW(OCIO::CTF::Version::ReadVersion("1.2.3", versionRead));
        const OCIO::CTF::Version version(1, 2, 3);
        OIIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OIIO_CHECK_NO_THROW(OCIO::CTF::Version::ReadVersion("1.2", versionRead));
        const OCIO::CTF::Version version(1, 2, 0);
        OIIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OIIO_CHECK_NO_THROW(OCIO::CTF::Version::ReadVersion("1", versionRead));
        const OCIO::CTF::Version version(1, 0, 0);
        OIIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OIIO_CHECK_NO_THROW(OCIO::CTF::Version::ReadVersion("1.10", versionRead));
        const OCIO::CTF::Version version(1, 10, 0);
        OIIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OIIO_CHECK_NO_THROW(OCIO::CTF::Version::ReadVersion("1.1.0", versionRead));
        const OCIO::CTF::Version version(1, 1, 0);
        OIIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OIIO_CHECK_NO_THROW(OCIO::CTF::Version::ReadVersion("1.01", versionRead));
        const OCIO::CTF::Version version(1, 1, 0);
        OIIO_CHECK_EQUAL(version, versionRead);
    }

    OIIO_CHECK_THROW_WHAT(OCIO::CTF::Version::ReadVersion("", versionRead),
                          OCIO::Exception, 
                          "is not a valid version");
    OIIO_CHECK_THROW_WHAT(OCIO::CTF::Version::ReadVersion("1 2", versionRead),
                          OCIO::Exception, 
                          "is not a valid version");
    OIIO_CHECK_THROW_WHAT(OCIO::CTF::Version::ReadVersion("1-2", versionRead),
                          OCIO::Exception, 
                          "is not a valid version");
    OIIO_CHECK_THROW_WHAT(OCIO::CTF::Version::ReadVersion("a", versionRead),
                          OCIO::Exception, 
                          "is not a valid version");
    OIIO_CHECK_THROW_WHAT(OCIO::CTF::Version::ReadVersion("1.", versionRead),
                          OCIO::Exception, 
                          "is not a valid version");
    OIIO_CHECK_THROW_WHAT(OCIO::CTF::Version::ReadVersion(".2", versionRead),
                          OCIO::Exception, 
                          "is not a valid version");
    OIIO_CHECK_THROW_WHAT(OCIO::CTF::Version::ReadVersion("1.0 2", versionRead),
                          OCIO::Exception,
                          "is not a valid version");
}

OIIO_ADD_TEST(CTFTransform, TestVersionWrite)
{
    {
        const OCIO::CTF::Version version(1, 2, 3);
        std::ostringstream ostream;
        ostream << version;
        OIIO_CHECK_EQUAL(ostream.str(), "1.2.3");
    }
    {
        const OCIO::CTF::Version version(1, 0, 3);
        std::ostringstream ostream;
        ostream << version;
        OIIO_CHECK_EQUAL(ostream.str(), "1.0.3");
    }
    {
        const OCIO::CTF::Version version(1, 2, 0);
        std::ostringstream ostream;
        ostream << version;
        OIIO_CHECK_EQUAL(ostream.str(), "1.2");
    }
    {
        const OCIO::CTF::Version version(1, 20, 0);
        std::ostringstream ostream;
        ostream << version;
        OIIO_CHECK_EQUAL(ostream.str(), "1.20");
    }
    {
        const OCIO::CTF::Version version(1, 0, 0);
        std::ostringstream ostream;
        ostream << version;
        OIIO_CHECK_EQUAL(ostream.str(), "1");
    }
    {
        const OCIO::CTF::Version version(0, 0, 0);
        std::ostringstream ostream;
        ostream << version;
        OIIO_CHECK_EQUAL(ostream.str(), "0");
    }
}

OIIO_ADD_TEST(CTFTransform, TestAccessors)
{
    OCIO::CTF::Reader::Transform t;
    {
        const OCIO::CTF::Reader::Transform & ct = t;

        OCIO::OpData::Metadata & info = t.getInfo();
        const OCIO::OpData::Metadata & cinfo = t.getInfo();

        OIIO_CHECK_EQUAL(info.getName(), "Info");
        OIIO_CHECK_EQUAL(cinfo.getName(), "Info");

        OIIO_CHECK_EQUAL(t.getId(), "");
        OIIO_CHECK_EQUAL(ct.getId(), "");
        OIIO_CHECK_EQUAL(t.getName(), "");
        OIIO_CHECK_EQUAL(ct.getName(), "");
        OIIO_CHECK_EQUAL(t.getInverseOfId(), "");
        OIIO_CHECK_EQUAL(ct.getInverseOfId(), "");
        OIIO_CHECK_EQUAL(t.getInputDescriptor(), "");
        OIIO_CHECK_EQUAL(ct.getInputDescriptor(), "");
        OIIO_CHECK_EQUAL(t.getOutputDescriptor(), "");
        OIIO_CHECK_EQUAL(ct.getOutputDescriptor(), "");

        OIIO_CHECK_ASSERT(t.getOps().empty());
        OIIO_CHECK_ASSERT(ct.getOps().empty());

        OIIO_CHECK_ASSERT(t.getDescriptions().getList().empty());
        OIIO_CHECK_ASSERT(ct.getDescriptions().getList().empty());
    }
    t.setName("Name");
    t.setId("123");
    t.setInverseOfId("654");
    t.setInputDescriptor("input");
    t.setOutputDescriptor("output");
    
    OCIO::OpData::Matrix * matrixOp = new OCIO::OpData::Matrix;
    t.getOps().append(matrixOp);

    t.getDescriptions() += "One";
    t.getDescriptions() += "Two";

    {
        const OCIO::CTF::Reader::Transform & ct = t;

        OIIO_CHECK_EQUAL(t.getId(), "123");
        OIIO_CHECK_EQUAL(ct.getId(), "123");
        OIIO_CHECK_EQUAL(t.getName(), "Name");
        OIIO_CHECK_EQUAL(ct.getName(), "Name");
        OIIO_CHECK_EQUAL(t.getInverseOfId(), "654");
        OIIO_CHECK_EQUAL(ct.getInverseOfId(), "654");
        OIIO_CHECK_EQUAL(t.getInputDescriptor(), "input");
        OIIO_CHECK_EQUAL(ct.getInputDescriptor(), "input");
        OIIO_CHECK_EQUAL(t.getOutputDescriptor(), "output");
        OIIO_CHECK_EQUAL(ct.getOutputDescriptor(), "output");

        OIIO_CHECK_EQUAL(t.getOps().size(), 1);
        OIIO_CHECK_EQUAL(ct.getOps().size(), 1);

        OIIO_CHECK_EQUAL(t.getDescriptions().getList().size(), 2);
        OIIO_CHECK_EQUAL(ct.getDescriptions().getList().size(), 2);
        OIIO_CHECK_EQUAL(t.getDescriptions().getList()[0], "One");
        OIIO_CHECK_EQUAL(ct.getDescriptions().getList()[0], "One");
        OIIO_CHECK_EQUAL(t.getDescriptions().getList()[1], "Two");
        OIIO_CHECK_EQUAL(ct.getDescriptions().getList()[1], "Two");
    }
}

// TODO: Port additional tests from syncolor transform_test.cpp.

#endif

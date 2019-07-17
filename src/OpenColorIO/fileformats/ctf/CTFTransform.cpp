/*
Copyright (c) 2018 Autodesk Inc., et al.
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

#include <sstream>

#include "fileformats/ctf/CTFTransform.h"
#include "Platform.h"

OCIO_NAMESPACE_ENTER
{

void CTFVersion::ReadVersion(const std::string & versionString,
                             CTFVersion & versionOut)
{
    unsigned int numDot = 0;
    unsigned int numInt = 0;
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

CTFVersion & CTFVersion::operator=(const CTFVersion & rhs)
{
    if (this != &rhs)
    {
        m_major = rhs.m_major;
        m_minor = rhs.m_minor;
        m_revision = rhs.m_revision;
    }
    return *this;
}

bool CTFVersion::operator==(const CTFVersion & rhs) const
{
    if (this == &rhs) return true;

    return m_major == rhs.m_major
        && m_minor == rhs.m_minor
        && m_revision == rhs.m_revision;
}

bool CTFVersion::operator<=(const CTFVersion & rhs) const
{
    if (*this == rhs)
    {
        return true;
    }
    return *this < rhs;
}

bool CTFVersion::operator>=(const CTFVersion & rhs) const
{
    return rhs <= *this;
}

bool CTFVersion::operator<(const CTFVersion & rhs) const
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

bool CTFVersion::operator>(const CTFVersion & rhs) const
{
    return rhs < *this;
}

CTFReaderTransform::CTFReaderTransform()
    : m_info("Info")
    , m_version(CTF_PROCESS_LIST_VERSION)
    , m_versionCLF(0, 0)
{
}

void CTFReaderTransform::setCTFVersion(const CTFVersion & ver)
{
    m_version = ver;
}

void CTFReaderTransform::setCLFVersion(const CTFVersion & ver)
{
    m_versionCLF = ver;
}

const CTFVersion & CTFReaderTransform::getCTFVersion() const
{
    return m_version;
}

void CTFReaderTransform::validate()
{
    BitDepth bitdepth = BIT_DEPTH_UNKNOWN;

    const size_t max = m_ops.size();
    for (size_t i = 0; i<max; ++i)
    {
        OpDataRcPtr & op = m_ops[i];

        op->validate();

        if (i > 0 && bitdepth != op->getInputBitDepth())
        {
            std::ostringstream os;
            os << "Bitdepth missmatch between ops";
            os << "'. Op " << i - 1;
            os << " (" << m_ops[i - 1]->getID();
            os << ") output bitdepth is ";
            os << bitdepth << ". Op " << i;
            os << " (" << op->getID();
            os << ") intput bitdepth is ";
            os << op->getInputBitDepth();
            throw Exception(os.str().c_str());
        }

        bitdepth = op->getOutputBitDepth();
    }
}

}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "ops/Matrix/MatrixOpData.h"
#include "UnitTest.h"

OCIO_ADD_TEST(CTFVersion, read_version)
{
    {
        const OCIO::CTFVersion version1(1, 2, 3);
        const OCIO::CTFVersion version2(1, 2, 3);
        OCIO_CHECK_EQUAL(version1, version2);
        {
            const OCIO::CTFVersion version3(0, 0, 1);
            OCIO_CHECK_ASSERT(false == (version1 == version3));
            OCIO_CHECK_ASSERT(version3 < version1);
        }
        {
            const OCIO::CTFVersion version3(0, 1, 0);
            OCIO_CHECK_ASSERT(false == (version1 == version3));
            OCIO_CHECK_ASSERT(version3 < version1);
        }
        {
            const OCIO::CTFVersion version3(1, 0, 0);
            OCIO_CHECK_ASSERT(false == (version1 == version3));
            OCIO_CHECK_ASSERT(version3 < version1);
        }
        {
            const OCIO::CTFVersion version3(1, 2, 0);
            OCIO_CHECK_ASSERT(false == (version1 == version3));
            OCIO_CHECK_ASSERT(version3 < version1);
        }
        {
            const OCIO::CTFVersion version3(1, 2, 2);
            OCIO_CHECK_ASSERT(false == (version1 == version3));
            OCIO_CHECK_ASSERT(version3 < version1);
        }
    }

    OCIO::CTFVersion versionRead;
    {
        OCIO_CHECK_NO_THROW(OCIO::CTFVersion::ReadVersion("1.2.3", versionRead));
        const OCIO::CTFVersion version(1, 2, 3);
        OCIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OCIO_CHECK_NO_THROW(OCIO::CTFVersion::ReadVersion("1.2", versionRead));
        const OCIO::CTFVersion version(1, 2, 0);
        OCIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OCIO_CHECK_NO_THROW(OCIO::CTFVersion::ReadVersion("1", versionRead));
        const OCIO::CTFVersion version(1, 0, 0);
        OCIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OCIO_CHECK_NO_THROW(OCIO::CTFVersion::ReadVersion("1.10", versionRead));
        const OCIO::CTFVersion version(1, 10, 0);
        OCIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OCIO_CHECK_NO_THROW(OCIO::CTFVersion::ReadVersion("1.1.0", versionRead));
        const OCIO::CTFVersion version(1, 1, 0);
        OCIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OCIO_CHECK_NO_THROW(OCIO::CTFVersion::ReadVersion("1.01", versionRead));
        const OCIO::CTFVersion version(1, 1, 0);
        OCIO_CHECK_EQUAL(version, versionRead);
    }

    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion("", versionRead),
                          OCIO::Exception, 
                          "is not a valid version");
    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion("1 2", versionRead),
                          OCIO::Exception, 
                          "is not a valid version");
    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion("1-2", versionRead),
                          OCIO::Exception, 
                          "is not a valid version");
    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion("a", versionRead),
                          OCIO::Exception, 
                          "is not a valid version");
    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion("1.", versionRead),
                          OCIO::Exception, 
                          "is not a valid version");
    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion(".2", versionRead),
                          OCIO::Exception, 
                          "is not a valid version");
    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion("1.0 2", versionRead),
                          OCIO::Exception,
                          "is not a valid version");
    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion("-1", versionRead),
                          OCIO::Exception,
                          "is not a valid version");
}

OCIO_ADD_TEST(CTFVersion, version_write)
{
    {
        const OCIO::CTFVersion version(1, 2, 3);
        std::ostringstream ostream;
        ostream << version;
        OCIO_CHECK_EQUAL(ostream.str(), "1.2.3");
    }
    {
        const OCIO::CTFVersion version(1, 0, 3);
        std::ostringstream ostream;
        ostream << version;
        OCIO_CHECK_EQUAL(ostream.str(), "1.0.3");
    }
    {
        const OCIO::CTFVersion version(1, 2, 0);
        std::ostringstream ostream;
        ostream << version;
        OCIO_CHECK_EQUAL(ostream.str(), "1.2");
    }
    {
        const OCIO::CTFVersion version(1, 20, 0);
        std::ostringstream ostream;
        ostream << version;
        OCIO_CHECK_EQUAL(ostream.str(), "1.20");
    }
    {
        const OCIO::CTFVersion version(1, 0, 0);
        std::ostringstream ostream;
        ostream << version;
        OCIO_CHECK_EQUAL(ostream.str(), "1");
    }
    {
        const OCIO::CTFVersion version(0, 0, 0);
        std::ostringstream ostream;
        ostream << version;
        OCIO_CHECK_EQUAL(ostream.str(), "0");
    }
}

OCIO_ADD_TEST(CTFReaderTransform, accessors)
{
    OCIO::CTFReaderTransform t;
    {
        const OCIO::CTFReaderTransform & ct = t;

        OCIO::Metadata & info = t.getInfo();
        const OCIO::Metadata & cinfo = t.getInfo();

        OCIO_CHECK_EQUAL(info.getName(), "Info");
        OCIO_CHECK_EQUAL(cinfo.getName(), "Info");

        OCIO_CHECK_EQUAL(t.getID(), "");
        OCIO_CHECK_EQUAL(ct.getID(), "");
        OCIO_CHECK_EQUAL(t.getName(), "");
        OCIO_CHECK_EQUAL(ct.getName(), "");
        OCIO_CHECK_EQUAL(t.getInverseOfId(), "");
        OCIO_CHECK_EQUAL(ct.getInverseOfId(), "");
        OCIO_CHECK_EQUAL(t.getInputDescriptor(), "");
        OCIO_CHECK_EQUAL(ct.getInputDescriptor(), "");
        OCIO_CHECK_EQUAL(t.getOutputDescriptor(), "");
        OCIO_CHECK_EQUAL(ct.getOutputDescriptor(), "");

        OCIO_CHECK_ASSERT(t.getOps().empty());
        OCIO_CHECK_ASSERT(ct.getOps().empty());

        OCIO_CHECK_ASSERT(t.getDescriptions().empty());
        OCIO_CHECK_ASSERT(ct.getDescriptions().empty());
    }
    t.setName("Name");
    t.setID("123");
    t.setInverseOfId("654");
    t.setInputDescriptor("input");
    t.setOutputDescriptor("output");
    
    auto matrixOp = std::make_shared<OCIO::MatrixOpData>();
    t.getOps().push_back(matrixOp);

    t.getDescriptions() += "One";
    t.getDescriptions() += "Two";

    {
        const OCIO::CTFReaderTransform & ct = t;

        OCIO_CHECK_EQUAL(t.getID(), "123");
        OCIO_CHECK_EQUAL(ct.getID(), "123");
        OCIO_CHECK_EQUAL(t.getName(), "Name");
        OCIO_CHECK_EQUAL(ct.getName(), "Name");
        OCIO_CHECK_EQUAL(t.getInverseOfId(), "654");
        OCIO_CHECK_EQUAL(ct.getInverseOfId(), "654");
        OCIO_CHECK_EQUAL(t.getInputDescriptor(), "input");
        OCIO_CHECK_EQUAL(ct.getInputDescriptor(), "input");
        OCIO_CHECK_EQUAL(t.getOutputDescriptor(), "output");
        OCIO_CHECK_EQUAL(ct.getOutputDescriptor(), "output");

        OCIO_CHECK_EQUAL(t.getOps().size(), 1);
        OCIO_CHECK_EQUAL(ct.getOps().size(), 1);

        OCIO_CHECK_EQUAL(t.getDescriptions().size(), 2);
        OCIO_CHECK_EQUAL(ct.getDescriptions().size(), 2);
        OCIO_CHECK_EQUAL(t.getDescriptions()[0], "One");
        OCIO_CHECK_EQUAL(ct.getDescriptions()[0], "One");
        OCIO_CHECK_EQUAL(t.getDescriptions()[1], "Two");
        OCIO_CHECK_EQUAL(ct.getDescriptions()[1], "Two");
    }
}

OCIO_ADD_TEST(CTFReaderTransform, validate)
{
    OCIO::CTFReaderTransform t;
    auto matrix = std::make_shared<OCIO::MatrixOpData>(OCIO::BIT_DEPTH_UINT10,
                                                       OCIO::BIT_DEPTH_F32);
    t.getOps().push_back(matrix);

    matrix = std::make_shared<OCIO::MatrixOpData>(OCIO::BIT_DEPTH_F32,
                                                  OCIO::BIT_DEPTH_F32);

    t.getOps().push_back(matrix);

    OCIO_CHECK_NO_THROW(t.validate());

    matrix = std::make_shared<OCIO::MatrixOpData>(OCIO::BIT_DEPTH_F16,
                                                  OCIO::BIT_DEPTH_F32);
    t.getOps().push_back(matrix);

    OCIO_CHECK_THROW_WHAT(t.validate(), OCIO::Exception,
                          "Bitdepth missmatch between ops");

    matrix->setInputBitDepth(OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_NO_THROW(t.validate());
}


#endif

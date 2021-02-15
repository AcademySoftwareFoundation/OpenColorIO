// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "fileformats/ctf/CTFTransform.cpp"

#include "ops/matrix/MatrixOpData.h"
#include "testutils/UnitTest.h"
#include "transforms/FileTransform.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


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

        OCIO::FormatMetadataImpl & info = t.getInfoMetadata();
        const OCIO::FormatMetadataImpl & cinfo = t.getInfoMetadata();

        OCIO_CHECK_EQUAL(std::string(info.getElementName()), OCIO::METADATA_INFO);
        OCIO_CHECK_EQUAL(std::string(cinfo.getElementName()), OCIO::METADATA_INFO);

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

    t.getDescriptions().push_back("One");
    t.getDescriptions().push_back("Two");

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

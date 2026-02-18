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
        OCIO_CHECK_NO_THROW(versionRead = OCIO::CTFVersion("1.2.3"));
        const OCIO::CTFVersion version(1, 2, 3);
        OCIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OCIO_CHECK_NO_THROW(versionRead = OCIO::CTFVersion("1.2"));
        const OCIO::CTFVersion version(1, 2, 0);
        OCIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OCIO_CHECK_NO_THROW(versionRead = OCIO::CTFVersion("1"));
        const OCIO::CTFVersion version(1, 0, 0);
        OCIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OCIO_CHECK_NO_THROW(versionRead = OCIO::CTFVersion("1.10"));
        const OCIO::CTFVersion version(1, 10, 0);
        OCIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OCIO_CHECK_NO_THROW(versionRead = OCIO::CTFVersion("1.1.0"));
        const OCIO::CTFVersion version(1, 1, 0);
        OCIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OCIO_CHECK_NO_THROW(versionRead = OCIO::CTFVersion("1.01"));
        const OCIO::CTFVersion version(1, 1, 0);
        OCIO_CHECK_EQUAL(version, versionRead);
    }

    {
        // Numeric format is always accepted.
        OCIO_CHECK_NO_THROW(versionRead = OCIO::CTFVersion("2.0.0", OCIO::CTFVersion::VERSION_SMPTE_CLF));
        const OCIO::CTFVersion version(2, 0, 0);
        OCIO_CHECK_EQUAL(version, versionRead);
    }

    {
        // Short SMPTE should be accepted only when the format is allowed.
        OCIO_CHECK_THROW_WHAT(versionRead = OCIO::CTFVersion(
            "ST2136-1:2024"), 
            OCIO::Exception,
            "is not a valid version. Expecting MAJOR[.MINOR[.REVISION]]");
    }

    {
        // Long SMPTE should be accepted only when the format is allowed.
        OCIO_CHECK_THROW_WHAT(versionRead = OCIO::CTFVersion(
            "http://www.smpte-ra.org/ns/2136-1/2024"), 
            OCIO::Exception,
            "is not a valid version. Expecting MAJOR[.MINOR[.REVISION]]");
    }

    {
        // SMPTE version is regarded as v3.0.0.
        OCIO_CHECK_NO_THROW(versionRead = OCIO::CTFVersion(
            "ST2136-1:2024", OCIO::CTFVersion::VERSION_SMPTE_CLF));
        const OCIO::CTFVersion version(3, 0, 0);
        OCIO_CHECK_EQUAL(version, versionRead);
    }

    {
        // Short SMPTE string is not allowed when only the long format is accepted.
        OCIO_CHECK_THROW_WHAT(versionRead = OCIO::CTFVersion(
            "ST2136-1:2024", OCIO::CTFVersion::VERSION_SMPTE_XMLNS),
            OCIO::Exception,
            "is not a valid version. Expecting 'http://www.smpte-ra.org/ns/2136-1/2024' or MAJOR[.MINOR[.REVISION]]");
    }

    {
        // Long SMPTE should be accepted only when the format is allowed and
        // should be regarded as v3.0.0.
        OCIO_CHECK_NO_THROW(versionRead = OCIO::CTFVersion(
            "http://www.smpte-ra.org/ns/2136-1/2024", OCIO::CTFVersion::VERSION_SMPTE_XMLNS));
        const OCIO::CTFVersion version(3, 0, 0);
        OCIO_CHECK_EQUAL(version, versionRead);
    }

    {
        // Long SMPTE string is not allowed when only the short format is accepted.
        OCIO_CHECK_THROW_WHAT(versionRead = OCIO::CTFVersion(
            "http://www.smpte-ra.org/ns/2136-1/2024", OCIO::CTFVersion::VERSION_SMPTE_CLF),
            OCIO::Exception,
            "is not a valid version. Expecting 'ST2136-1:2024' or MAJOR[.MINOR[.REVISION]]");
    }

    OCIO_CHECK_THROW_WHAT(versionRead = OCIO::CTFVersion(""),
        OCIO::Exception,
        "is not a valid version");
    OCIO_CHECK_THROW_WHAT(versionRead = OCIO::CTFVersion("1 2"),
        OCIO::Exception,
        "is not a valid version");
    OCIO_CHECK_THROW_WHAT(versionRead = OCIO::CTFVersion("1-2"),
        OCIO::Exception,
        "is not a valid version");
    OCIO_CHECK_THROW_WHAT(versionRead = OCIO::CTFVersion("a"),
        OCIO::Exception,
        "is not a valid version");
    OCIO_CHECK_THROW_WHAT(versionRead = OCIO::CTFVersion("1."),
        OCIO::Exception,
        "is not a valid version");
    OCIO_CHECK_THROW_WHAT(versionRead = OCIO::CTFVersion(".2"),
        OCIO::Exception,
        "is not a valid version");
    OCIO_CHECK_THROW_WHAT(versionRead = OCIO::CTFVersion("1.0 2"),
        OCIO::Exception,
        "is not a valid version");
    OCIO_CHECK_THROW_WHAT(versionRead = OCIO::CTFVersion("-1"),
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
    {
        const OCIO::CTFVersion version(
            "ST2136-1:2024", 
            OCIO::CTFVersion::VERSION_SMPTE_CLF);
        std::ostringstream ostream;
        ostream << version;
        OCIO_CHECK_EQUAL(ostream.str(), "ST2136-1:2024");
    }
    {
        const OCIO::CTFVersion version(
            "http://www.smpte-ra.org/ns/2136-1/2024", 
            OCIO::CTFVersion::VERSION_SMPTE_XMLNS);
        std::ostringstream ostream;
        ostream << version;
        OCIO_CHECK_EQUAL(ostream.str(), "http://www.smpte-ra.org/ns/2136-1/2024");
    }

}

OCIO_ADD_TEST(CTFReaderTransform, accessors)
{
    OCIO::CTFReaderTransform t;
    OCIO::FormatMetadataImpl & meta = t.getInfoMetadata();
    {
        const OCIO::CTFReaderTransform & ct = t;
        const OCIO::FormatMetadataImpl & cmeta = t.getInfoMetadata();

        OCIO::FormatMetadataImpl & info = t.getInfoMetadata();
        const OCIO::FormatMetadataImpl & cinfo = t.getInfoMetadata();

        OCIO_CHECK_EQUAL(std::string(info.getElementName()), OCIO::METADATA_INFO);
        OCIO_CHECK_EQUAL(std::string(cinfo.getElementName()), OCIO::METADATA_INFO);

        OCIO_CHECK_EQUAL(t.getID(), "");
        OCIO_CHECK_EQUAL(ct.getID(), "");
        OCIO_CHECK_EQUAL(t.getName(), "");
        OCIO_CHECK_EQUAL(ct.getName(), "");
        OCIO_CHECK_EQUAL_STR(meta.getAttributeValue("inverseOf"), "");
        OCIO_CHECK_EQUAL_STR(cmeta.getAttributeValue("inverseOf"), "");

        OCIO_CHECK_ASSERT(t.getOpDataVec().empty());
        OCIO_CHECK_ASSERT(ct.getOpDataVec().empty());

        OCIO_REQUIRE_EQUAL(meta.getNumChildrenElements(), 0);
        OCIO_REQUIRE_EQUAL(cmeta.getNumChildrenElements(), 0);
    }
    t.setName("Name");
    t.setID("123");
    meta.addAttribute(OCIO::ATTR_INVERSE_OF, "654");
    meta.addChildElement(OCIO::METADATA_ID_ELEMENT, "urn:uuid:123e4567-e89b-12d3-a456-426655440000");

    auto matrixOp = std::make_shared<OCIO::MatrixOpData>();
    t.getOpDataVec().push_back(matrixOp);

    meta.addChildElement(OCIO::METADATA_DESCRIPTION, "One");
    meta.addChildElement(OCIO::METADATA_DESCRIPTION, "Two");
    meta.addChildElement(OCIO::METADATA_INPUT_DESCRIPTOR, "input 1");
    meta.addChildElement(OCIO::METADATA_INPUT_DESCRIPTOR, "input 2");
    meta.addChildElement(OCIO::METADATA_OUTPUT_DESCRIPTOR, "output 1");
    meta.addChildElement(OCIO::METADATA_OUTPUT_DESCRIPTOR, "output 2");
    auto & outDesc2 = meta.getChildElement(meta.getNumChildrenElements()-1);
    outDesc2.addAttribute(OCIO::ATTR_LANGUAGE, "tr");

    {
        const OCIO::CTFReaderTransform & ct = t;
        const OCIO::FormatMetadataImpl & cmeta = t.getInfoMetadata();

        OCIO_CHECK_EQUAL(t.getID(), "123");
        OCIO_CHECK_EQUAL(ct.getID(), "123");
        OCIO_CHECK_EQUAL(t.getName(), "Name");
        OCIO_CHECK_EQUAL(ct.getName(), "Name");
        OCIO_CHECK_EQUAL_STR(meta.getAttributeValue("inverseOf"), "654");
        OCIO_CHECK_EQUAL_STR(cmeta.getAttributeValue("inverseOf"), "654");

        OCIO_CHECK_EQUAL(t.getOpDataVec().size(), 1);
        OCIO_CHECK_EQUAL(ct.getOpDataVec().size(), 1);

        OCIO_REQUIRE_EQUAL(meta.getNumChildrenElements(), 7);
        OCIO_CHECK_EQUAL_STR(meta.getChildElement(0).getElementName(), "Id");
        OCIO_CHECK_EQUAL_STR(meta.getChildElement(0).getElementValue(), "urn:uuid:123e4567-e89b-12d3-a456-426655440000");
        OCIO_CHECK_EQUAL_STR(meta.getChildElement(1).getElementName(), "Description");
        OCIO_CHECK_EQUAL_STR(meta.getChildElement(1).getElementValue(), "One");
        OCIO_CHECK_EQUAL_STR(meta.getChildElement(2).getElementName(), "Description");
        OCIO_CHECK_EQUAL_STR(meta.getChildElement(2).getElementValue(), "Two");
        OCIO_CHECK_EQUAL_STR(meta.getChildElement(3).getElementName(), "InputDescriptor");
        OCIO_CHECK_EQUAL_STR(meta.getChildElement(3).getElementValue(), "input 1");
        OCIO_CHECK_EQUAL_STR(meta.getChildElement(4).getElementName(), "InputDescriptor");
        OCIO_CHECK_EQUAL_STR(meta.getChildElement(4).getElementValue(), "input 2");
        OCIO_CHECK_EQUAL_STR(meta.getChildElement(5).getElementName(), "OutputDescriptor");
        OCIO_CHECK_EQUAL_STR(meta.getChildElement(5).getElementValue(), "output 1");
        OCIO_CHECK_EQUAL_STR(meta.getChildElement(6).getElementName(), "OutputDescriptor");
        OCIO_CHECK_EQUAL_STR(meta.getChildElement(6).getElementValue(), "output 2");
        OCIO_CHECK_EQUAL_STR(meta.getChildElement(6).getAttributeValue("language"), "tr");
    }
}

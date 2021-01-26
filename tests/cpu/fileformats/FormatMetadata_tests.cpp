// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "fileformats/FormatMetadata.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(FormatMetadataImpl, test_accessors)
{
    OCIO::FormatMetadataImpl info(OCIO::METADATA_INFO, "");
    OCIO_CHECK_EQUAL(std::string(info.getElementName()), OCIO::METADATA_INFO);

    // Make sure that we can add attributes and that existing attributes will get
    // overwritten.
    info.addAttribute("version", "1.0");

    const OCIO::FormatMetadataImpl::Attributes & atts1 = info.getAttributes();
    OCIO_CHECK_EQUAL(atts1.size(), 1);
    OCIO_CHECK_EQUAL(atts1[0].first, "version");
    OCIO_CHECK_EQUAL(atts1[0].second, "1.0");

    info.addAttribute("version", "2.0");

    const OCIO::FormatMetadataImpl::Attributes& atts2 = info.getAttributes();
    OCIO_CHECK_EQUAL(atts2.size(), 1);
    OCIO_CHECK_EQUAL(atts2[0].first, "version");
    OCIO_CHECK_EQUAL(atts2[0].second, "2.0");

    info.getChildrenElements().emplace_back("Copyright", "Copyright 2013 Autodesk");
    info.getChildrenElements().emplace_back("Release", "2015");
    OCIO_REQUIRE_EQUAL(info.getChildrenElements().size(), 2);
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[0].getElementName()),
                     "Copyright");
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[0].getElementValue()),
                     "Copyright 2013 Autodesk");
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[1].getElementName()), "Release");
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[1].getElementValue()), "2015");

    // Add input color space metadata.
    info.getChildrenElements().emplace_back("InputColorSpace", "");
    OCIO::FormatMetadataImpl & inCS = info.getChildrenElements().back();
    // 2 elements can have the same name.
    inCS.getChildrenElements().emplace_back(OCIO::METADATA_DESCRIPTION,
                                            "Input color space description");
    inCS.getChildrenElements().emplace_back(OCIO::METADATA_DESCRIPTION, "Other description");
    inCS.getChildrenElements().emplace_back("Profile", "Input color space profile");
    OCIO_REQUIRE_EQUAL(info.getChildrenElements().size(), 3);
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[2].getElementName()),
                     "InputColorSpace");
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[2].getElementValue()), "");
    OCIO_REQUIRE_EQUAL(info.getChildrenElements()[2].getChildrenElements().size(), 3);
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[2].getChildrenElements()[0].getElementName()),
                     OCIO::METADATA_DESCRIPTION);
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[2].getChildrenElements()[0].getElementValue()),
                     "Input color space description");
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[2].getChildrenElements()[1].getElementName()),
                     OCIO::METADATA_DESCRIPTION);
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[2].getChildrenElements()[1].getElementValue()),
                     "Other description");
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[2].getChildrenElements()[2].getElementName()),
                     "Profile");
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[2].getChildrenElements()[2].getElementValue()),
                     "Input color space profile");

    // Add output color space metadata.
    info.getChildrenElements().emplace_back("OutputColorSpace", "Output Colors Space");
    OCIO::FormatMetadataImpl & outCS = info.getChildrenElements().back();
    outCS.getChildrenElements().emplace_back(OCIO::METADATA_DESCRIPTION, "Output color space description");
    outCS.getChildrenElements().emplace_back("Profile", "Output color space profile");
    OCIO_REQUIRE_EQUAL(info.getChildrenElements().size(), 4);
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[3].getElementName()),
                     "OutputColorSpace");
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[3].getElementValue()),
                     "Output Colors Space");
    OCIO_REQUIRE_EQUAL(info.getChildrenElements()[3].getChildrenElements().size(), 2);
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[3].getChildrenElements()[0].getElementName()),
                     OCIO::METADATA_DESCRIPTION);
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[3].getChildrenElements()[0].getElementValue()),
                     "Output color space description");
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[3].getChildrenElements()[1].getElementName()),
                     "Profile");
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[3].getChildrenElements()[1].getElementValue()),
                     "Output color space profile");

    // Add category.
    // Assign value directly to the metadata element.
    info.getChildrenElements().emplace_back("Category", "");
    OCIO::FormatMetadataImpl & cat = info.getChildrenElements().back();
    cat.getChildrenElements().emplace_back("Name", "Color space category name");
    cat.getChildrenElements().emplace_back("Importance", "High");

    // Note:  This is a hypothetical example to test the class, it doesn't correspond to any actual file format.
    /*
    <Info version="2.0">
        <Copyright>Copyright 2013 Autodesk</Copyright>
        <Release>2015</Release>
        <InputColorSpace>
            <Description>Input color space description</Description>
            <Description>Other description</Description>
            <Profile>Input color space profile</Profile>
        </InputColorSpace>
        <OutputColorSpace>
            Output Colors Space
            <Description>Output color space description</Description>
            <Profile>Output color space profile</Profile>
        </OutputColorSpace>
        <Category>
            <Name>Color space category name</Name>
            <Importance>High</Importance>
        </Category>
    </Info>
    */

    OCIO_REQUIRE_EQUAL(info.getChildrenElements().size(), 5);
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[4].getElementName()), "Category");
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[4].getElementValue()), "");
    OCIO_REQUIRE_EQUAL(info.getChildrenElements()[4].getChildrenElements().size(), 2);
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[4].getChildrenElements()[0].getElementName()),
                     "Name");
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[4].getChildrenElements()[0].getElementValue()),
                     "Color space category name");
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[4].getChildrenElements()[1].getElementName()),
                     "Importance");
    OCIO_CHECK_EQUAL(std::string(info.getChildrenElements()[4].getChildrenElements()[1].getElementValue()),
                     "High");

    //
    // Do similar tests using only FormatMetadata public API interface.
    //
    info.clear();
    OCIO_REQUIRE_EQUAL(std::string(info.getElementName()), "Info");
    OCIO_REQUIRE_EQUAL(std::string(info.getElementValue()), "");
    OCIO_REQUIRE_EQUAL(info.getNumAttributes(), 0);
    OCIO_REQUIRE_EQUAL(info.getNumChildrenElements(), 0);

    info.addAttribute("version", "1.0");
    OCIO_REQUIRE_EQUAL(info.getNumAttributes(), 1);
    OCIO_CHECK_EQUAL(std::string(info.getAttributeName(0)), "version");
    OCIO_CHECK_EQUAL(std::string(info.getAttributeValue(0)), "1.0");

    info.addAttribute("version", "2.0");
    OCIO_REQUIRE_EQUAL(info.getNumAttributes(), 1);
    OCIO_CHECK_EQUAL(std::string(info.getAttributeName(0)), "version");
    OCIO_CHECK_EQUAL(std::string(info.getAttributeValue(0)), "2.0");

    info.addChildElement("Copyright", "Copyright 2013 Autodesk");
    info.addChildElement("Release", "2015");

    OCIO_REQUIRE_EQUAL(info.getNumChildrenElements(), 2);
    const auto & info0 = info.getChildElement(0);
    OCIO_CHECK_EQUAL(std::string(info0.getElementName()),
                     "Copyright");
    OCIO_CHECK_EQUAL(std::string(info0.getElementValue()),
                     "Copyright 2013 Autodesk");
    const auto & info1 = info.getChildElement(1);
    OCIO_CHECK_EQUAL(std::string(info1.getElementName()), "Release");
    OCIO_CHECK_EQUAL(std::string(info1.getElementValue()), "2015");

    info.addChildElement("InputColorSpace", "" );
    OCIO_REQUIRE_EQUAL(info.getNumChildrenElements(), 3);
    // 2 elements can have the same name.
    auto & icInfo = info.getChildElement(2);
    icInfo.addChildElement(OCIO::METADATA_DESCRIPTION, "Input color space description");
    icInfo.addChildElement(OCIO::METADATA_DESCRIPTION, "Other description");
    icInfo.addChildElement("Profile", "Input color space profile");

    OCIO_CHECK_EQUAL(std::string(icInfo.getElementName()), "InputColorSpace");
    OCIO_CHECK_EQUAL(std::string(icInfo.getElementValue()), "");

    OCIO_REQUIRE_EQUAL(icInfo.getNumChildrenElements(), 3);
    OCIO_CHECK_EQUAL(std::string(icInfo.getChildElement(0).getElementName()),
                     OCIO::METADATA_DESCRIPTION);
    OCIO_CHECK_EQUAL(std::string(icInfo.getChildElement(0).getElementValue()),
                     "Input color space description");
    OCIO_CHECK_EQUAL(std::string(icInfo.getChildElement(1).getElementName()),
                     OCIO::METADATA_DESCRIPTION);
    OCIO_CHECK_EQUAL(std::string(icInfo.getChildElement(1).getElementValue()),
                     "Other description");
    OCIO_CHECK_EQUAL(std::string(icInfo.getChildElement(2).getElementName()),
                     "Profile");
    OCIO_CHECK_EQUAL(std::string(icInfo.getChildElement(2).getElementValue()),
                     "Input color space profile");

    std::ostringstream oss;
    oss << info;
    const std::string expectedRes{ "<Info version=\"2.0\">"
                                   "<Copyright>Copyright 2013 Autodesk</Copyright>"
                                   "<Release>2015</Release>"
                                   "<InputColorSpace>"
                                   "<Description>Input color space description</Description>"
                                   "<Description>Other description</Description>"
                                   "<Profile>Input color space profile</Profile>"
                                   "</InputColorSpace></Info>" };
    OCIO_CHECK_EQUAL(expectedRes, oss.str());

    // Rename tests.

    // Valid new name.
    OCIO_CHECK_NO_THROW(info.setElementName("TEST"));
    OCIO_CHECK_EQUAL(std::string(info.getElementName()), "TEST");

    // Name can't be empty.
    OCIO_CHECK_THROW_WHAT(info.setElementName(""), OCIO::Exception, "has to have a non-empty name");
    OCIO_CHECK_THROW_WHAT(info.setElementName(nullptr), OCIO::Exception,
                          "FormatMetadata has to have a non-empty name");

    // ROOT is reserved.
    OCIO_CHECK_THROW_WHAT(info.setElementName(OCIO::METADATA_ROOT), OCIO::Exception,
                          "'ROOT' is reversed for root FormatMetadata elements");

    // Similar exceptions when adding a child element.

    OCIO_CHECK_THROW_WHAT(info.addChildElement("", ""), OCIO::Exception, "has to have a non-empty name");
    OCIO_CHECK_THROW_WHAT(info.addChildElement(nullptr, ""), OCIO::Exception,
                          "FormatMetadata has to have a non-empty name");
    OCIO_CHECK_THROW_WHAT(info.addChildElement(OCIO::METADATA_ROOT, ""), OCIO::Exception,
                          "'ROOT' is reversed for root FormatMetadata elements");

    // Root element can't be renamed.
    OCIO::FormatMetadataImpl root;
    OCIO_CHECK_EQUAL(std::string(root.getElementName()), OCIO::METADATA_ROOT);
    OCIO_CHECK_THROW_WHAT(root.setElementName("test"), OCIO::Exception,
                          "FormatMetadata 'ROOT' element can't be renamed");

    // Attribute name must be non-empty.

    OCIO_CHECK_THROW_WHAT(root.addAttribute("", "test"), OCIO::Exception,
                          "Attribute must have a non-empty name");
    OCIO_CHECK_THROW_WHAT(root.addAttribute(nullptr, "test"), OCIO::Exception,
                          "Attribute must have a non-empty name");
}

OCIO_ADD_TEST(FormatMetadataImpl, helpers)
{
    OCIO::FormatMetadataImpl root0;
    OCIO_CHECK_EQUAL(std::string(root0.getName()), "");
    OCIO_CHECK_EQUAL(std::string(root0.getID()), "");

    root0.addAttribute(OCIO::METADATA_NAME, "root0");
    root0.addAttribute(OCIO::METADATA_ID, "ID0");

    OCIO_CHECK_EQUAL(std::string(root0.getName()), "root0");
    OCIO_CHECK_EQUAL(std::string(root0.getID()), "ID0");

    root0.setName("root1");
    root0.setID("ID1");

    OCIO_CHECK_EQUAL(std::string(root0.getName()), "root1");
    OCIO_CHECK_EQUAL(std::string(root0.getID()), "ID1");

    root0.setName("");
    root0.setID("");

    OCIO_CHECK_EQUAL(std::string(root0.getName()), "");
    OCIO_CHECK_EQUAL(std::string(root0.getID()), "");
}

OCIO_ADD_TEST(FormatMetadataImpl, combine)
{
    OCIO::FormatMetadataImpl root0;
    root0.addAttribute(OCIO::METADATA_NAME, "root0");
    root0.addAttribute(OCIO::METADATA_ID, "ID0");
    root0.addChildElement("test0", "val0");
    OCIO::FormatMetadataImpl root1;
    root1.addAttribute(OCIO::METADATA_NAME, "root1");
    root1.addAttribute(OCIO::METADATA_ID, "ID1");
    root1.addChildElement("test1", "val1");
    auto & sub1 = root1.getChildElement(0);
    sub1.addChildElement("sub1-test", "subval");

    root0.addAttribute("att0", "attval0");
    root0.addAttribute("att1", "attval1");
    root1.addAttribute("att1", "otherval");
    root1.addAttribute("att2", "attval2");
    /*
    root0 is:
    <ROOT name="root0" id="ID0" att0="attval0" att1="attval1">
        <test0>val0</test0>
    </ROOT>

    root1 is:
    <ROOT name="root1" id="ID1" att1="otherval" att2="attval2">
        <test1>val1
            <sub1-test>subval
            </sub1-test>
        </test1>
    </ROOT>
    */

    root0.combine(root1);

    /*
    Now root0 is:
    <ROOT name="root0 + root1" id="ID0 + ID1" att0="attval0" att1="attval1 + otherval" att2="attval2">
        <test0>val0</test0>
        <test1>val1
            <sub1-test>subval
            </sub1-test>
        </test1>
    </ROOT>
    */

    OCIO_REQUIRE_EQUAL(root0.getNumAttributes(), 5);
    OCIO_REQUIRE_EQUAL(root0.getNumChildrenElements(), 2);

    OCIO_CHECK_EQUAL(std::string("test0"), root0.getChildrenElements()[0].getElementName());
    OCIO_CHECK_EQUAL(std::string("val0"), root0.getChildrenElements()[0].getElementValue());

    OCIO_CHECK_EQUAL(std::string("test1"), root0.getChildrenElements()[1].getElementName());
    OCIO_CHECK_EQUAL(std::string("val1"), root0.getChildrenElements()[1].getElementValue());
    // Sub elements are copied.
    OCIO_CHECK_EQUAL(root0.getChildrenElements()[1].getNumChildrenElements(), 1);

    OCIO_CHECK_EQUAL(std::string(OCIO::METADATA_NAME), root0.getAttributeName(0));
    // Name attributes are is combined.
    OCIO_CHECK_EQUAL(std::string("root0 + root1"), root0.getAttributeValue(0));

    OCIO_CHECK_EQUAL(std::string(OCIO::METADATA_ID), root0.getAttributeName(1));
    // Id attributes are is combined.
    OCIO_CHECK_EQUAL(std::string("ID0 + ID1"), root0.getAttributeValue(1));

    // Other attributes are added.
    OCIO_CHECK_EQUAL(std::string("att0"), root0.getAttributeName(2));
    OCIO_CHECK_EQUAL(std::string("attval0"), root0.getAttributeValue(2));
    OCIO_CHECK_EQUAL(std::string("att1"), root0.getAttributeName(3));
    // Existing attribute values are combined.
    OCIO_CHECK_EQUAL(std::string("attval1 + otherval"), root0.getAttributeValue(3));
    OCIO_CHECK_EQUAL(std::string("att2"), root0.getAttributeName(4));
    OCIO_CHECK_EQUAL(std::string("attval2"), root0.getAttributeValue(4));

    OCIO::FormatMetadataImpl root2;
    root2.addAttribute(OCIO::METADATA_NAME, "root2");
    root2.addChildElement("test", "val2");
    OCIO::FormatMetadataImpl root3;
    root3.addAttribute(OCIO::METADATA_ID, "ID3");
    root3.addChildElement("test", "val3");

    /*
    root2 is:
    <ROOT name="root2">
    <test>val2</test>
    </ROOT>

    root3 is:
    <ROOT id="ID3">
    <test>val3</test>
    </ROOT>
    */

    root2.combine(root3);

    /*
    Now root2 is:
    <ROOT name="root2" id="ID3">
    <test>val2</test>
    <test>val3</test>
    </ROOT>
    */

    OCIO_REQUIRE_EQUAL(root2.getNumAttributes(), 2);
    OCIO_REQUIRE_EQUAL(root2.getNumChildrenElements(), 2);
    OCIO_CHECK_EQUAL(std::string(OCIO::METADATA_NAME), root2.getAttributeName(0));
    OCIO_CHECK_EQUAL(std::string("root2"), root2.getAttributeValue(0));

    OCIO_CHECK_EQUAL(std::string(OCIO::METADATA_ID), root2.getAttributeName(1));
    OCIO_CHECK_EQUAL(std::string("ID3"), root2.getAttributeValue(1));

    OCIO_CHECK_EQUAL(std::string("test"), root2.getChildrenElements()[0].getElementName());
    OCIO_CHECK_EQUAL(std::string("val2"), root2.getChildrenElements()[0].getElementValue());

    OCIO_CHECK_EQUAL(std::string("test"), root2.getChildrenElements()[1].getElementName());
    OCIO_CHECK_EQUAL(std::string("val3"), root2.getChildrenElements()[1].getElementValue());

    OCIO::FormatMetadataImpl metainfo(OCIO::METADATA_INFO, "");
    OCIO_CHECK_THROW_WHAT(metainfo.combine(root3), OCIO::Exception,
                          "Only FormatMetadata with the same name");

}


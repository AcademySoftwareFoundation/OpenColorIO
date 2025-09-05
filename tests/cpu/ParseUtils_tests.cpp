// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ParseUtils.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(ParseUtils, xml_text)
{
    const std::string in("abc \" def ' ghi < jkl > mnop & efg");
    const std::string ref("abc &quot; def &apos; ghi &lt; jkl &gt; mnop &amp; efg");

    std::string out = OCIO::ConvertSpecialCharToXmlToken(in);
    OCIO_CHECK_EQUAL(out, ref);

    std::string back = OCIO::ConvertXmlTokenToSpecialChar(ref);
    OCIO_CHECK_EQUAL(back, in);
}

OCIO_ADD_TEST(ParseUtils, bool_string)
{
    std::string resStr = OCIO::BoolToString(true);
    OCIO_CHECK_EQUAL("true", resStr);

    resStr = OCIO::BoolToString(false);
    OCIO_CHECK_EQUAL("false", resStr);

    bool resBool = OCIO::BoolFromString("yes");
    OCIO_CHECK_EQUAL(true, resBool);

    resBool = OCIO::BoolFromString("Yes");
    OCIO_CHECK_EQUAL(true, resBool);

    resBool = OCIO::BoolFromString("YES");
    OCIO_CHECK_EQUAL(true, resBool);

    resBool = OCIO::BoolFromString("YeS");
    OCIO_CHECK_EQUAL(true, resBool);

    resBool = OCIO::BoolFromString("yEs");
    OCIO_CHECK_EQUAL(true, resBool);

    resBool = OCIO::BoolFromString("true");
    OCIO_CHECK_EQUAL(true, resBool);

    resBool = OCIO::BoolFromString("TRUE");
    OCIO_CHECK_EQUAL(true, resBool);

    resBool = OCIO::BoolFromString("True");
    OCIO_CHECK_EQUAL(true, resBool);

    resBool = OCIO::BoolFromString("tRUe");
    OCIO_CHECK_EQUAL(true, resBool);

    resBool = OCIO::BoolFromString("tRUE");
    OCIO_CHECK_EQUAL(true, resBool);

    resBool = OCIO::BoolFromString("yes ");
    OCIO_CHECK_EQUAL(false, resBool);

    resBool = OCIO::BoolFromString(" true ");
    OCIO_CHECK_EQUAL(false, resBool);

    resBool = OCIO::BoolFromString("false");
    OCIO_CHECK_EQUAL(false, resBool);

    resBool = OCIO::BoolFromString("");
    OCIO_CHECK_EQUAL(false, resBool);

    resBool = OCIO::BoolFromString("no");
    OCIO_CHECK_EQUAL(false, resBool);

    resBool = OCIO::BoolFromString("valid");
    OCIO_CHECK_EQUAL(false, resBool);

    resBool = OCIO::BoolFromString("success");
    OCIO_CHECK_EQUAL(false, resBool);

    resBool = OCIO::BoolFromString("anything");
    OCIO_CHECK_EQUAL(false, resBool);
}

OCIO_ADD_TEST(ParseUtils, transform_direction)
{
    std::string resStr;
    resStr = OCIO::TransformDirectionToString(OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL("forward", resStr);
    resStr = OCIO::TransformDirectionToString(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL("inverse", resStr);

    OCIO::TransformDirection resDir = OCIO::TRANSFORM_DIR_INVERSE;
    OCIO_CHECK_NO_THROW(resDir = OCIO::TransformDirectionFromString("forward"));
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, resDir);
    OCIO_CHECK_NO_THROW(resDir = OCIO::TransformDirectionFromString("inverse"));
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_INVERSE, resDir);
    OCIO_CHECK_NO_THROW(resDir = OCIO::TransformDirectionFromString("Forward"));
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, resDir);
    OCIO_CHECK_NO_THROW(resDir = OCIO::TransformDirectionFromString("Inverse"));
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_INVERSE, resDir);
    OCIO_CHECK_NO_THROW(resDir = OCIO::TransformDirectionFromString("FORWARD"));
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, resDir);
    OCIO_CHECK_NO_THROW(resDir = OCIO::TransformDirectionFromString("INVERSE"));
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_INVERSE, resDir);
    OCIO_CHECK_THROW_WHAT(resDir = OCIO::TransformDirectionFromString("unknown"), OCIO::Exception,
                          "Unrecognized transform direction: 'unknown'");
    OCIO_CHECK_THROW_WHAT(resDir = OCIO::TransformDirectionFromString(""), OCIO::Exception,
                          "Unrecognized transform direction: ''");
    OCIO_CHECK_THROW_WHAT(resDir = OCIO::TransformDirectionFromString("anything"), OCIO::Exception,
                          "Unrecognized transform direction: 'anything'");
    OCIO_CHECK_THROW_WHAT(resDir = OCIO::TransformDirectionFromString(nullptr), OCIO::Exception,
                          "Unrecognized transform direction: ''");

    resDir = OCIO::CombineTransformDirections(OCIO::TRANSFORM_DIR_INVERSE,
                                              OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, resDir);
    resDir = OCIO::CombineTransformDirections(OCIO::TRANSFORM_DIR_FORWARD,
                                              OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, resDir);
    resDir = OCIO::CombineTransformDirections(OCIO::TRANSFORM_DIR_INVERSE,
                                              OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_INVERSE, resDir);
    resDir = OCIO::CombineTransformDirections(OCIO::TRANSFORM_DIR_FORWARD,
                                              OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_INVERSE, resDir);

    resDir = OCIO::GetInverseTransformDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_FORWARD, resDir);
    resDir = OCIO::GetInverseTransformDirection(OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(OCIO::TRANSFORM_DIR_INVERSE, resDir);
}

OCIO_ADD_TEST(ParseUtils, bitdepth)
{
    std::string resStr;
    resStr = OCIO::BitDepthToString(OCIO::BIT_DEPTH_UINT8);
    OCIO_CHECK_EQUAL("8ui", resStr);
    resStr = OCIO::BitDepthToString(OCIO::BIT_DEPTH_UINT10);
    OCIO_CHECK_EQUAL("10ui", resStr);
    resStr = OCIO::BitDepthToString(OCIO::BIT_DEPTH_UINT12);
    OCIO_CHECK_EQUAL("12ui", resStr);
    resStr = OCIO::BitDepthToString(OCIO::BIT_DEPTH_UINT14);
    OCIO_CHECK_EQUAL("14ui", resStr);
    resStr = OCIO::BitDepthToString(OCIO::BIT_DEPTH_UINT16);
    OCIO_CHECK_EQUAL("16ui", resStr);
    resStr = OCIO::BitDepthToString(OCIO::BIT_DEPTH_UINT32);
    OCIO_CHECK_EQUAL("32ui", resStr);
    resStr = OCIO::BitDepthToString(OCIO::BIT_DEPTH_F16);
    OCIO_CHECK_EQUAL("16f", resStr);
    resStr = OCIO::BitDepthToString(OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_EQUAL("32f", resStr);
    resStr = OCIO::BitDepthToString(OCIO::BIT_DEPTH_UNKNOWN);
    OCIO_CHECK_EQUAL("unknown", resStr);

    OCIO::BitDepth resBD;
    resBD = OCIO::BitDepthFromString("8ui");
    OCIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UINT8, resBD);
    resBD = OCIO::BitDepthFromString("8Ui");
    OCIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UINT8, resBD);
    resBD = OCIO::BitDepthFromString("8UI");
    OCIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UINT8, resBD);
    resBD = OCIO::BitDepthFromString("8uI");
    OCIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UINT8, resBD);
    resBD = OCIO::BitDepthFromString("10ui");
    OCIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UINT10, resBD);
    resBD = OCIO::BitDepthFromString("12ui");
    OCIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UINT12, resBD);
    resBD = OCIO::BitDepthFromString("14ui");
    OCIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UINT14, resBD);
    resBD = OCIO::BitDepthFromString("16ui");
    OCIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UINT16, resBD);
    resBD = OCIO::BitDepthFromString("32ui");
    OCIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UINT32, resBD);
    resBD = OCIO::BitDepthFromString("16f");
    OCIO_CHECK_EQUAL(OCIO::BIT_DEPTH_F16, resBD);
    resBD = OCIO::BitDepthFromString("32f");
    OCIO_CHECK_EQUAL(OCIO::BIT_DEPTH_F32, resBD);
    resBD = OCIO::BitDepthFromString("7ui");
    OCIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UNKNOWN, resBD);
    resBD = OCIO::BitDepthFromString("unknown");
    OCIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UNKNOWN, resBD);
    resBD = OCIO::BitDepthFromString("");
    OCIO_CHECK_EQUAL(OCIO::BIT_DEPTH_UNKNOWN, resBD);

    bool resBool;
    resBool = OCIO::BitDepthIsFloat(OCIO::BIT_DEPTH_F16);
    OCIO_CHECK_EQUAL(true, resBool);
    resBool = OCIO::BitDepthIsFloat(OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_EQUAL(true, resBool);
    resBool = OCIO::BitDepthIsFloat(OCIO::BIT_DEPTH_UINT8);
    OCIO_CHECK_EQUAL(false, resBool);
    resBool = OCIO::BitDepthIsFloat(OCIO::BIT_DEPTH_UINT10);
    OCIO_CHECK_EQUAL(false, resBool);
    resBool = OCIO::BitDepthIsFloat(OCIO::BIT_DEPTH_UINT12);
    OCIO_CHECK_EQUAL(false, resBool);
    resBool = OCIO::BitDepthIsFloat(OCIO::BIT_DEPTH_UINT14);
    OCIO_CHECK_EQUAL(false, resBool);
    resBool = OCIO::BitDepthIsFloat(OCIO::BIT_DEPTH_UINT16);
    OCIO_CHECK_EQUAL(false, resBool);
    resBool = OCIO::BitDepthIsFloat(OCIO::BIT_DEPTH_UINT32);
    OCIO_CHECK_EQUAL(false, resBool);
    resBool = OCIO::BitDepthIsFloat(OCIO::BIT_DEPTH_UNKNOWN);
    OCIO_CHECK_EQUAL(false, resBool);

    int resInt;
    resInt = OCIO::BitDepthToInt(OCIO::BIT_DEPTH_UINT8);
    OCIO_CHECK_EQUAL(8, resInt);
    resInt = OCIO::BitDepthToInt(OCIO::BIT_DEPTH_UINT10);
    OCIO_CHECK_EQUAL(10, resInt);
    resInt = OCIO::BitDepthToInt(OCIO::BIT_DEPTH_UINT12);
    OCIO_CHECK_EQUAL(12, resInt);
    resInt = OCIO::BitDepthToInt(OCIO::BIT_DEPTH_UINT14);
    OCIO_CHECK_EQUAL(14, resInt);
    resInt = OCIO::BitDepthToInt(OCIO::BIT_DEPTH_UINT16);
    OCIO_CHECK_EQUAL(16, resInt);
    resInt = OCIO::BitDepthToInt(OCIO::BIT_DEPTH_UINT32);
    OCIO_CHECK_EQUAL(32, resInt);
    resInt = OCIO::BitDepthToInt(OCIO::BIT_DEPTH_F16);
    OCIO_CHECK_EQUAL(0, resInt);
    resInt = OCIO::BitDepthToInt(OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_EQUAL(0, resInt);
    resInt = OCIO::BitDepthToInt(OCIO::BIT_DEPTH_UNKNOWN);
    OCIO_CHECK_EQUAL(0, resInt);

}

OCIO_ADD_TEST(ParseUtils, string_to_int)
{
    int ival = 0;
    bool success = false;

    success = OCIO::StringToInt(&ival, "", false);
    OCIO_CHECK_EQUAL(success, false);

    success = OCIO::StringToInt(&ival, "9", false);
    OCIO_CHECK_EQUAL(success, true);
    OCIO_CHECK_EQUAL(ival, 9);

    success = OCIO::StringToInt(&ival, " 10 ", false);
    OCIO_CHECK_EQUAL(success, true);
    OCIO_CHECK_EQUAL(ival, 10);

    success = OCIO::StringToInt(&ival, " 101", true);
    OCIO_CHECK_EQUAL(success, true);
    OCIO_CHECK_EQUAL(ival, 101);

    success = OCIO::StringToInt(&ival, " 11x ", false);
    OCIO_CHECK_EQUAL(success, true);
    OCIO_CHECK_EQUAL(ival, 11);

    success = OCIO::StringToInt(&ival, " 12x ", true);
    OCIO_CHECK_EQUAL(success, false);

    success = OCIO::StringToInt(&ival, "13", true);
    OCIO_CHECK_EQUAL(success, true);
    OCIO_CHECK_EQUAL(ival, 13);

    success = OCIO::StringToInt(&ival, "-14", true);
    OCIO_CHECK_EQUAL(success, true);
    OCIO_CHECK_EQUAL(ival, -14);

    success = OCIO::StringToInt(&ival, "x-15", false);
    OCIO_CHECK_EQUAL(success, false);

    success = OCIO::StringToInt(&ival, "x-16", false);
    OCIO_CHECK_EQUAL(success, false);
}

OCIO_ADD_TEST(ParseUtils, string_to_float)
{
    float fval = 0;
    bool success = false;

    success = OCIO::StringToFloat(&fval, "");
    OCIO_CHECK_EQUAL(success, false);

    success = OCIO::StringToFloat(&fval, "1.0");
    OCIO_CHECK_EQUAL(success, true);
    OCIO_CHECK_EQUAL(fval, 1.0f);

    success = OCIO::StringToFloat(&fval, "1");
    OCIO_CHECK_EQUAL(success, true);
    OCIO_CHECK_EQUAL(fval, 1.0f);

    success = OCIO::StringToFloat(&fval, "a1");
    OCIO_CHECK_EQUAL(success, false);

    success = OCIO::StringToFloat(&fval,
        "1 do we really want this to succeed?");
    OCIO_CHECK_EQUAL(success, true);
    OCIO_CHECK_EQUAL(fval, 1.0f);

    success = OCIO::StringToFloat(&fval, "1Success");
    OCIO_CHECK_EQUAL(success, true);
    OCIO_CHECK_EQUAL(fval, 1.0f);

    success = OCIO::StringToFloat(&fval,
        "1.0000000000000000000000000000000000000000000001");
    OCIO_CHECK_EQUAL(success, true);
    OCIO_CHECK_EQUAL(fval, 1.0f);
}

OCIO_ADD_TEST(ParseUtils, float_double)
{
    std::string resStr;
    resStr = OCIO::FloatToString(0.0f);
    OCIO_CHECK_EQUAL("0", resStr);

    resStr = OCIO::FloatToString(0.1111001f);
    OCIO_CHECK_EQUAL("0.1111001", resStr);

    resStr = OCIO::FloatToString(0.11000001f);
    OCIO_CHECK_EQUAL("0.11", resStr);

    resStr = OCIO::DoubleToString(0.11000001);
    OCIO_CHECK_EQUAL("0.11000001", resStr);

    resStr = OCIO::DoubleToString(0.1100000000000001);
    OCIO_CHECK_EQUAL("0.1100000000000001", resStr);

    resStr = OCIO::DoubleToString(0.11000000000000001);
    OCIO_CHECK_EQUAL("0.11", resStr);
}

OCIO_ADD_TEST(ParseUtils, string_vec_to_int_vec)
{
    std::vector<int> intArray;
    StringUtils::StringVec lineParts;
    bool success = OCIO::StringVecToIntVec(intArray, lineParts);
    OCIO_CHECK_EQUAL(true, success);
    OCIO_CHECK_EQUAL(0, intArray.size());

    lineParts.push_back("42");
    lineParts.push_back("");

    success = OCIO::StringVecToIntVec(intArray, lineParts);
    OCIO_CHECK_EQUAL(false, success);
    OCIO_CHECK_EQUAL(2, intArray.size());

    intArray.clear();
    lineParts.clear();

    lineParts.push_back("42");
    lineParts.push_back("0");

    success = OCIO::StringVecToIntVec(intArray, lineParts);
    OCIO_CHECK_EQUAL(true, success);
    OCIO_CHECK_EQUAL(2, intArray.size());
    OCIO_CHECK_EQUAL(42, intArray[0]);
    OCIO_CHECK_EQUAL(0, intArray[1]);

    intArray.clear();
    lineParts.clear();

    lineParts.push_back("42");
    lineParts.push_back("021");

    success = OCIO::StringVecToIntVec(intArray, lineParts);
    OCIO_CHECK_EQUAL(true, success);
    OCIO_CHECK_EQUAL(2, intArray.size());
    OCIO_CHECK_EQUAL(42, intArray[0]);
    OCIO_CHECK_EQUAL(21, intArray[1]);

    intArray.clear();
    lineParts.clear();

    lineParts.push_back("42");
    lineParts.push_back("0x21");

    success = OCIO::StringVecToIntVec(intArray, lineParts);
    OCIO_CHECK_EQUAL(false, success);
    OCIO_CHECK_EQUAL(2, intArray.size());

    intArray.clear();
    lineParts.clear();

    lineParts.push_back("42u");
    lineParts.push_back("21");

    success = OCIO::StringVecToIntVec(intArray, lineParts);
    OCIO_CHECK_EQUAL(false, success);
    OCIO_CHECK_EQUAL(2, intArray.size());
}

OCIO_ADD_TEST(ParseUtils, split_string_env_style)
{
    // For look parsing, the split needs to always return a result, even if empty.
    StringUtils::StringVec outputvec;
    outputvec = OCIO::SplitStringEnvStyle("");
    OCIO_CHECK_EQUAL(1, outputvec.size());
    outputvec.clear();

    outputvec = OCIO::SplitStringEnvStyle("This:is:a:test");
    OCIO_CHECK_EQUAL(4, outputvec.size());
    OCIO_CHECK_EQUAL("This", outputvec[0]);
    OCIO_CHECK_EQUAL("is", outputvec[1]);
    OCIO_CHECK_EQUAL("a", outputvec[2]);
    OCIO_CHECK_EQUAL("test", outputvec[3]);
    outputvec.clear();

    outputvec = OCIO::SplitStringEnvStyle("   \"This\"  : is   :   a:   test  ");
    OCIO_CHECK_EQUAL(4, outputvec.size());
    OCIO_CHECK_EQUAL("This", outputvec[0]);
    OCIO_CHECK_EQUAL("is", outputvec[1]);
    OCIO_CHECK_EQUAL("a", outputvec[2]);
    OCIO_CHECK_EQUAL("test", outputvec[3]);
    outputvec.clear();

    outputvec = OCIO::SplitStringEnvStyle("   This  , is   ,   a,   test  ");
    OCIO_CHECK_EQUAL(4, outputvec.size());
    OCIO_CHECK_EQUAL("This", outputvec[0]);
    OCIO_CHECK_EQUAL("is", outputvec[1]);
    OCIO_CHECK_EQUAL("a", outputvec[2]);
    OCIO_CHECK_EQUAL("test", outputvec[3]);
    outputvec.clear();

    outputvec = OCIO::SplitStringEnvStyle("This:is   ,   a:test  ");
    OCIO_CHECK_EQUAL(2, outputvec.size());
    OCIO_CHECK_EQUAL("This:is", outputvec[0]);
    OCIO_CHECK_EQUAL("a:test", outputvec[1]);
    outputvec.clear();

    outputvec = OCIO::SplitStringEnvStyle(",,");
    OCIO_CHECK_EQUAL(3, outputvec.size());
    OCIO_CHECK_EQUAL("", outputvec[0]);
    OCIO_CHECK_EQUAL("", outputvec[1]);
    OCIO_CHECK_EQUAL("", outputvec[2]);

    outputvec = OCIO::SplitStringEnvStyle("   \"This  : is   \":   a:   test  ");
    OCIO_CHECK_EQUAL(3, outputvec.size());
    OCIO_CHECK_EQUAL("This  : is   ", outputvec[0]);
    OCIO_CHECK_EQUAL("a", outputvec[1]);
    OCIO_CHECK_EQUAL("test", outputvec[2]);

    OCIO_CHECK_THROW_WHAT( OCIO::SplitStringEnvStyle("   This  : is   \":   a:   test  "),
                           OCIO::Exception, 
                           "The string 'This  : is   \":   a:   test' is not correctly formatted. "
                           "It is missing a closing quote.");

    OCIO_CHECK_THROW_WHAT( OCIO::SplitStringEnvStyle("   This  : is   :   a:   test  \""), 
                           OCIO::Exception, 
                           "The string 'This  : is   :   a:   test  \"' is not correctly formatted. "
                           "It is missing a closing quote.");

    outputvec = OCIO::SplitStringEnvStyle("   This  : is   \":   a:   test  \"");
    OCIO_CHECK_EQUAL(2, outputvec.size());
    OCIO_CHECK_EQUAL("This", outputvec[0]);
    OCIO_CHECK_EQUAL("is   \":   a:   test  \"" , outputvec[1]);

    outputvec = OCIO::SplitStringEnvStyle("   \"This  : is   \",   a,   test  ");
    OCIO_CHECK_EQUAL(3, outputvec.size());
    OCIO_CHECK_EQUAL("This  : is   ", outputvec[0]);
    OCIO_CHECK_EQUAL("a", outputvec[1]);
    OCIO_CHECK_EQUAL("test", outputvec[2]);

    // If the string contains a comma, it is chosen as the separator character rather than
    // the colon (even if it is within quotes and therefore not used as such).
    outputvec = OCIO::SplitStringEnvStyle("   \"This  , is   \":   a:   test  ");
    OCIO_CHECK_EQUAL(1, outputvec.size());
    OCIO_CHECK_EQUAL("\"This  , is   \":   a:   test", outputvec[0]);

    outputvec = OCIO::SplitStringEnvStyle("   \"This  , is   \":   a,   test  ");
    OCIO_CHECK_EQUAL(2, outputvec.size());
    OCIO_CHECK_EQUAL("\"This  , is   \":   a", outputvec[0]);
    OCIO_CHECK_EQUAL("test", outputvec[1]);
}

OCIO_ADD_TEST(ParseUtils, join_string_env_style)
{
    StringUtils::StringVec outputvec {"This", "is", "a", "test"};

    OCIO_CHECK_EQUAL( "This, is, a, test", OCIO::JoinStringEnvStyle(outputvec) );
    outputvec.clear();

    OCIO_CHECK_EQUAL( "", OCIO::JoinStringEnvStyle(outputvec) );
    outputvec.clear();

    outputvec = { "This:is", "a:test" };
    OCIO_CHECK_EQUAL( "\"This:is\", \"a:test\"", OCIO::JoinStringEnvStyle(outputvec) );
    outputvec.clear();

    outputvec = { "", "", "" };
    OCIO_CHECK_EQUAL( ", , ", OCIO::JoinStringEnvStyle(outputvec) );
    outputvec.clear();

    outputvec = { "This  : is", "a: test" };
    OCIO_CHECK_EQUAL( "\"This  : is\", \"a: test\"", OCIO::JoinStringEnvStyle(outputvec) );
    outputvec.clear();

    outputvec = { "This", "is   \":   a:   test" };
    OCIO_CHECK_EQUAL( "This, \"is   \":   a:   test\"", OCIO::JoinStringEnvStyle(outputvec) );

    outputvec = { "\"This, is, a, string\"", "this, one, too" };
    OCIO_CHECK_EQUAL( "\"This, is, a, string\", \"this, one, too\"" , 
                      OCIO::JoinStringEnvStyle(outputvec) );

    outputvec = { "This", "is: ", "\"a very good,\"", " fine, helpful, and useful ", "test" };
    OCIO_CHECK_EQUAL( "This, \"is: \", \"a very good,\", \" fine, helpful, and useful \", test", 
                      OCIO::JoinStringEnvStyle(outputvec) );
}

OCIO_ADD_TEST(ParseUtils, intersect_string_vecs_case_ignore)
{
    StringUtils::StringVec source1, source2;
    source1.push_back("111");
    source1.push_back("This");
    source1.push_back("is");
    source1.push_back("222");
    source1.push_back("a");
    source1.push_back("test");

    source2.push_back("333");
    source2.push_back("TesT");
    source2.push_back("this");
    source2.push_back("444");
    source2.push_back("a");
    source2.push_back("IS");

    StringUtils::StringVec resInter = OCIO::IntersectStringVecsCaseIgnore(source1, source2);
    OCIO_CHECK_EQUAL(4, resInter.size());
    OCIO_CHECK_EQUAL("This", resInter[0]);
    OCIO_CHECK_EQUAL("is", resInter[1]);
    OCIO_CHECK_EQUAL("a", resInter[2]);
    OCIO_CHECK_EQUAL("test", resInter[3]);
}


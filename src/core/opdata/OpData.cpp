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



#include <OpenColorIO/OpenColorIO.h>

#include "OpData.h"
#include "OpDataMatrix.h"

#include "../Platform.h"
#include "../BitDepthUtils.h"
#include <sstream>

OCIO_NAMESPACE_ENTER
{
// Private namespace to the OpData sub-directory
namespace OpData
{
OpData::OpData(BitDepth inBitDepth,
                BitDepth outBitDepth)
    :   m_inBitDepth(inBitDepth)
    ,   m_outBitDepth(outBitDepth)
{
}

OpData::OpData(BitDepth inBitDepth,
                BitDepth outBitDepth,
                const std::string& id,
                const std::string& name,
                const Descriptions& descriptions)
    :   m_id(id)
    ,   m_name(name)
    ,   m_inBitDepth(inBitDepth)
    ,   m_outBitDepth(outBitDepth)
    ,   m_descriptions(descriptions)
{
}

OpData::OpData(const OpData& rhs)
{
    *this = rhs;
}

OpData& OpData::operator=(const OpData& rhs)
{
    if (this != &rhs)
    {
        m_inBitDepth = rhs.m_inBitDepth;
        m_outBitDepth = rhs.m_outBitDepth;
        m_id = rhs.m_id;
        m_name = rhs.m_name;
        m_descriptions = rhs.m_descriptions;

        /* TODO: BypassProperty will be added later
        if (this != &rhs)
        {
        setBypass(rhs.getBypass());
        }*/
    }

    return *this;
}

OpData::~OpData()
{
}

void OpData::setId(const std::string& id)
{
    m_id = id;
}

void OpData::setName(const std::string& name)
{
    m_name = name;
}

void OpData::setInputBitDepth(BitDepth in)
{
    m_inBitDepth = in;
}

void OpData::setOutputBitDepth(BitDepth out)
{
    m_outBitDepth = out;
}

void OpData::validate() const
{
    if (getInputBitDepth() == BIT_DEPTH_UNKNOWN)
    {
        throw Exception("OpData missing 'InBitDepth' attribute.");
    }

    if (getOutputBitDepth() == BIT_DEPTH_UNKNOWN)
    {
        throw Exception("OpData missing 'OutBitDepth' attribute.");
    }
}


bool OpData::isNoOp() const
{
    return (getInputBitDepth() == getOutputBitDepth()) && isIdentity() && !isClamping();
}

const std::string& OpData::getMeaningfullIdentifier() const
{
    if (!getName().empty())
    {
        return getName();
    }

    if (!getId().empty())
    {
        return getId();
    }

    return getOpTypeName();
}

bool OpData::operator==(const OpData& other) const
{
    if (this == &other) return true;

    return (m_inBitDepth == other.m_inBitDepth && 
            m_outBitDepth == other.m_outBitDepth &&
            m_id == other.m_id &&
            m_name == other.m_name &&
            m_descriptions == other.m_descriptions);
}

} // exit OpData namespace
}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

OCIO_NAMESPACE_USING
namespace OCIO = OCIO_NAMESPACE;

#include "../UnitTest.h"

namespace
{
    const char uid[] = "uid";
    const char name[] = "name";
}

OIIO_ADD_TEST(OpData, op_accessor_test)
{
    OCIO::OpData::Matrix
        l1(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT10);

    OCIO::OpData::Descriptions desc;
    desc += "Descriptions 1";
    OCIO::OpData::Matrix 
        l2(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT10, uid, name, desc);

    OIIO_CHECK_EQUAL(l1.getInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(l1.getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    OIIO_CHECK_ASSERT(l1.getId().empty());
    OIIO_CHECK_ASSERT(l1.getName().empty());

    OIIO_CHECK_ASSERT(l1.isIdentity());
    OIIO_CHECK_ASSERT(!l1.isNoOp());
    OIIO_CHECK_NO_THROW(l1.validate());

    OIIO_CHECK_EQUAL(l2.getInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(l2.getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    OIIO_CHECK_EQUAL(l2.getId(), uid);
    OIIO_CHECK_EQUAL(l2.getName(), name);
    OIIO_CHECK_EQUAL(l2.getMeaningfullIdentifier(), name);
    OIIO_CHECK_EQUAL(l2.getDescriptions().getList().size(), 1);
    OIIO_CHECK_EQUAL(l2.getDescriptions().getList()[0], "Descriptions 1");

    l2.setId("uid2");
    l2.setInputBitDepth(OCIO::BIT_DEPTH_F16);
    l2.setOutputBitDepth(OCIO::BIT_DEPTH_F32);

    OIIO_CHECK_EQUAL(l2.getInputBitDepth(), OCIO::BIT_DEPTH_F16);
    OIIO_CHECK_EQUAL(l2.getOutputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(l2.getId(), "uid2");
    OIIO_CHECK_EQUAL(l2.getName(), name);
    OIIO_CHECK_EQUAL(l2.getMeaningfullIdentifier(), name);
}

/* TODO: port when DynamicProperty is available
BOOST_AUTO_TEST_CASE(op_default_dynamic_property_behavior)
{
    MyOp op(SYNCOLOR::BIT_DEPTH_8i, SYNCOLOR::BIT_DEPTH_10i);

    BOOST_CHECK(!op.isDynamic());
    BOOST_CHECK(!op.hasDynamicProperty(SYNCOLOR::DynamicProperty::Exposure));
    BOOST_CHECK(!op.hasDynamicProperty(SYNCOLOR::DynamicProperty::Contrast));
    BOOST_CHECK(!op.hasDynamicProperty(SYNCOLOR::DynamicProperty::Gamma));
    BOOST_CHECK(!op.hasDynamicProperty(SYNCOLOR::DynamicProperty::Bypass));

    SYNCOLOR::DynamicProperty e(SYNCOLOR::DynamicProperty::Exposure);
    SYNCOLOR::DynamicProperty c(SYNCOLOR::DynamicProperty::Contrast);
    SYNCOLOR::DynamicProperty g(SYNCOLOR::DynamicProperty::Gamma);
    SYNCOLOR::DynamicProperty l(SYNCOLOR::DynamicProperty::Bypass);

    e.value.dVal = .33f;
    c.value.dVal = .33f;
    g.value.dVal = .33f;
    l.value.bVal = true;

    BOOST_CHECK_EXCEPTION(
        op.setDynamicProperty(e),
        Color::Exception,
        checkEx<SYNCOLOR::ERROR_TRANSFORM_ALL_PROPERTY_NOT_AVAILABLE>);
    BOOST_CHECK_EXCEPTION(
        op.setDynamicProperty(c),
        Color::Exception,
        checkEx<SYNCOLOR::ERROR_TRANSFORM_ALL_PROPERTY_NOT_AVAILABLE>);
    BOOST_CHECK_EXCEPTION(
        op.setDynamicProperty(g),
        Color::Exception,
        checkEx<SYNCOLOR::ERROR_TRANSFORM_ALL_PROPERTY_NOT_AVAILABLE>);
    BOOST_CHECK_EXCEPTION(
        op.setDynamicProperty(l),
        Color::Exception,
        checkEx<SYNCOLOR::ERROR_TRANSFORM_ALL_PROPERTY_NOT_AVAILABLE>);

    BOOST_CHECK_EXCEPTION(
        op.getDynamicProperty(e),
        Color::Exception,
        checkEx<SYNCOLOR::ERROR_TRANSFORM_ALL_PROPERTY_NOT_AVAILABLE>);
    BOOST_CHECK_EXCEPTION(
        op.getDynamicProperty(c),
        Color::Exception,
        checkEx<SYNCOLOR::ERROR_TRANSFORM_ALL_PROPERTY_NOT_AVAILABLE>);
    BOOST_CHECK_EXCEPTION(
        op.getDynamicProperty(g),
        Color::Exception,
        checkEx<SYNCOLOR::ERROR_TRANSFORM_ALL_PROPERTY_NOT_AVAILABLE>);
    BOOST_CHECK_EXCEPTION(
        op.getDynamicProperty(l),
        Color::Exception,
        checkEx<SYNCOLOR::ERROR_TRANSFORM_ALL_PROPERTY_NOT_AVAILABLE>);

    op.getBypass()->setDynamic(true);

    BOOST_CHECK(op.hasDynamicProperty(SYNCOLOR::DynamicProperty::Bypass));
    op.getDynamicProperty(l);
}
*/

OIIO_ADD_TEST(OpData, op_equality_test)
{
    OCIO::OpData::Matrix l1(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT10);

    OCIO::OpData::Descriptions desc;
    desc += "Descriptions 1";
    OCIO::OpData::Matrix 
        l2(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT10, uid, name, desc);

    OIIO_CHECK_ASSERT(!(l1 == l2));

    OCIO::OpData::Matrix 
        l3(OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_UINT10, uid, name, desc);

    OIIO_CHECK_ASSERT(!(l3 == l2));

    OCIO::OpData::Matrix l4(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT10);

    OIIO_CHECK_ASSERT(l4 == l1);
}

/* TODO: port when BypassProperty is available
OIIO_ADD_TEST(OpData, op_bypass_equality_test)
{
    MyOp o1(SYNCOLOR::BIT_DEPTH_8i, SYNCOLOR::BIT_DEPTH_10i);
    MyOp o2(SYNCOLOR::BIT_DEPTH_8i, SYNCOLOR::BIT_DEPTH_10i);

    o1.getBypass()->setDynamic(true);
    BOOST_CHECK(!(o1 == o2));
    BOOST_CHECK(!o1.hasMatchingBypass(o2));

    o2.getBypass()->setDynamic(true);
    BOOST_CHECK(o1 == o2);
    BOOST_CHECK(o1.hasMatchingBypass(o2));

    o1.getBypass()->setValue(true);
    BOOST_CHECK(!(o1 == o2));
    BOOST_CHECK(!o1.hasMatchingBypass(o2));

    o2.getBypass()->setValue(true);
    BOOST_CHECK(o1 == o2);
    BOOST_CHECK(o1.hasMatchingBypass(o2));
}*/

/* TODO: port when DynamicProperty is available
OIIO_ADD_TEST(OpData, op_clone_test)
{
    MyOp o1(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT10);
    BOOST_CHECK(!o1.isDynamic());

    MyOp o2(o1);
    BOOST_CHECK(!o2.isDynamic());

    o2.getBypass()->setDynamic(true);
    BOOST_CHECK(o2.isDynamic());
    BOOST_CHECK(!o1.isDynamic());

    BOOST_CHECK_EQUAL(o1.getBypass()->getValue(), o2.getBypass()->getValue());

    o2.getBypass()->setValue(true);
    BOOST_CHECK(!o1.getBypass()->getValue());

    MyOp o3(o2);
    BOOST_CHECK(o3.isDynamic());
    BOOST_CHECK(o2.isDynamic());
    BOOST_CHECK(!o1.isDynamic());

    o3.getBypass()->setValue(true);
    BOOST_CHECK_EQUAL(o3.getBypass()->getValue(), o2.getBypass()->getValue());

    o3.getBypass()->setValue(false);
    BOOST_CHECK_EQUAL(o3.getBypass()->getValue(), o2.getBypass()->getValue());
}*/


#endif // OCIO_UNIT_TEST

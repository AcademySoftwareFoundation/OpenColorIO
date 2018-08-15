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

#include "OpDataVec.h"
#include <sstream>

OCIO_NAMESPACE_ENTER
{
// Private namespace to the OpData sub-directory
namespace OpData
{
OpDataVec::OpDataVec()
{
}

OpDataVec::OpDataVec(const OpDataVec & v)
{
    *this = v;
}

OpDataVec::~OpDataVec()
{
    freeContent();
}

OpDataVec & OpDataVec::operator=(const OpDataVec & ops)
{
    if (this != &ops)
    {
        freeContent();
        *this += ops;
    }
    return *this;
}

void OpDataVec::append(OpData * op)
{
    if(!op)
    {
        throw Exception("Invalid Op");
    }

    m_content.push_back(op);

    const size_t sz = m_content.size();
    if (sz > 1)
    {
        m_content[sz - 1]->setInputBitDepth(m_content[sz - 2]->getOutputBitDepth());
    }
}

void OpDataVec::prepend(OpData * op)
{
    if(!op)
    {
        throw Exception("Invalid Op");
    }

    m_content.insert(m_content.begin(), op);

    if(m_content.size()>1)
    {
        m_content[1]->setInputBitDepth(m_content[0]->getOutputBitDepth());
    }
}

void OpDataVec::insert(OpData * op, const unsigned idx)
{
    if(!op)
    {
        throw Exception("Invalid Op");
    }

    size_t sz = m_content.size();
    if(idx > sz)
    {
        throw Exception("Index is out of range");
    }

    m_content.insert(m_content.begin() + idx, op);
    sz++;

    // Set input bit-depth of the new one to match the output
    //  of the previous op.
    if (idx > 0)
    {
        m_content[ idx ]->setInputBitDepth(
                                m_content[ idx - 1 ]->getOutputBitDepth());
    }

    // Set input bit-depth of following op to match the output of the new one.
    if (idx < sz - 1)
    {
        m_content[ idx + 1 ]->setInputBitDepth(
                                m_content[ idx ]->getOutputBitDepth());
    }
}

void OpDataVec::replace(OpData * op, const unsigned idx)
{
    if(!op)
    {
        throw Exception("Invalid Op");
    }

    const size_t sz = m_content.size();
    if(idx >= sz)
    {
        throw Exception("Index is out of range");
    }

    // Free the op that was at idx.
    delete m_content[idx];

    // Insert new op at idx.
    m_content[idx] = op;

    // Set input bit-depth of the new one to match the output
    //  of the previous op.
    if (idx > 0)
    {
        m_content[ idx ]->setInputBitDepth(m_content[ idx - 1 ]->getOutputBitDepth());
    }

    // Set input bit-depth of following op to match the output of the new one.
    if (idx < sz - 1)
    {
        m_content[ idx + 1 ]->setInputBitDepth(m_content[ idx ]->getOutputBitDepth());
    }
}

void OpDataVec::erase(const unsigned idx)
{
    delete remove(idx);
}

OpData * OpDataVec::remove(const unsigned idx)
{
    const size_t sz = m_content.size();
    if(idx >= sz)
    {
        throw Exception("Index is out of range");
    }

    OpData * removedOp = m_content[idx];

    // Shift any ops after idx left.
    // Change the size to sz - 1.
    m_content.erase( m_content.begin() + idx );

    // Make the bit-depths align at the new interface.
    if ( idx > 0 && idx < sz - 1 )
    {
        m_content[ idx ]->setInputBitDepth(
                                m_content[ idx - 1 ]->getOutputBitDepth());
    }

    return removedOp;
}

OpDataVec & OpDataVec::operator+=(const OpDataVec& rhs)
{
    const size_t rhsSize = rhs.m_content.size();
    if(rhsSize==0) return *this;

    const size_t currSize = m_content.size();
    const size_t newSize = currSize + rhsSize;

    m_content.resize(newSize);

    OpData * pOp = rhs.m_content[0]->clone(OpData::DO_SHALLOW_COPY);
    if(currSize>0)
    {
        pOp->setInputBitDepth(m_content[currSize-1]->getOutputBitDepth());
    }
    m_content[currSize] = pOp;

    for(size_t i=1; i<rhsSize; ++i)
    {
        m_content[currSize+i] = rhs.m_content[i]->clone(OpData::DO_SHALLOW_COPY);
    }

    return *this;
}

bool OpDataVec::empty() const
{
    return m_content.empty();
}

unsigned OpDataVec::size() const
{
    return (unsigned)m_content.size();
}

const OpData * OpDataVec::get(unsigned idx) const
{
    if(idx>=m_content.size())
    {
        throw Exception("Index is out of range");
    }
    return m_content[idx];
}

OpData * OpDataVec::get(unsigned idx)
{
    if(idx>=m_content.size())
    {
        throw Exception("Index is out of range");
    }
    return m_content[idx];
}

void OpDataVec::freeContent()
{
    const size_t max = m_content.size();
    for(size_t i=0; i<max; ++i)
    {
        delete m_content[i];
    }
    m_content.clear();
}

void OpDataVec::validate() const
{
    BitDepth bitdepth = BIT_DEPTH_UNKNOWN;

    const size_t max = m_content.size();
    for(size_t i=0; i<max; ++i)
    {
        const OpData * pOp = m_content[i];

        pOp->validate();

        if(i>0 && bitdepth!=pOp->getInputBitDepth())
        {
            std::ostringstream os;
            os << "Bitdepth missmatch between ops";
            os << "'. Op " << i - 1;
            os << " (" << m_content[i - 1]->getOpTypeName();
            os << ") output bitdepth is ";
            os << bitdepth << ". Op " << i;
            os << " (" << pOp->getOpTypeName();
            os << ") intput bitdepth is ";
            os << pOp->getInputBitDepth();
            throw Exception(os.str().c_str());
        }

        bitdepth = pOp->getOutputBitDepth();
    }
}

bool OpDataVec::operator==(const OpDataVec & other) const
{
    if (this == &other) return true;

    const size_t max = m_content.size();
    if (max != other.m_content.size()) return false;

    for (unsigned i=0;i<max;i++)
    {
        if (m_content[i] == other.m_content[i]) continue;
        if (!(*m_content[i] == *(other.m_content[i]))) return false;
    }

    return true;
}

void OpDataVec::inverse(OpDataVec & invOps) const
{
    const int max = static_cast<int>(m_content.size());
    for(int idx= max -1 ; idx>=0 ; --idx)
    {
        const OpData * op = m_content[idx];
        // Append new ops required to compute the inverse 
        // of this op to the end of the invOps list
        op->inverse(invOps);
    }
}

void OpDataVec::clone(OpDataVec & ops, OpData::CloneType type) const
{
    const unsigned max = static_cast<unsigned>(m_content.size());
    for(unsigned idx=0; idx<max; ++idx)
    {
        const OpData * op = m_content[idx];
        ops.append(op->clone(type));
    }
}

} // exit OpData namespace
}
OCIO_NAMESPACE_EXIT


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include "../UnitTest.h"
#include "../MathUtils.h"

#include "OpDataRange.h"
#include "OpDataCDL.h"


OIIO_ADD_TEST(OpDataVec, Accessors)
{
    const OCIO::OpData::Range o1, o2;

    OCIO::OpData::OpDataVec t;
    OIIO_CHECK_EQUAL(t.size(), 0);
    OIIO_CHECK_ASSERT(t.empty());

    OCIO::OpData::OpData * myOp1 
        = o1.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY);
    t.append(myOp1);
    OIIO_CHECK_EQUAL(t.size(), 1);
    OIIO_CHECK_ASSERT(!t.empty());
    OIIO_CHECK_EQUAL(myOp1, t.get(0));
    OIIO_CHECK_ASSERT(*dynamic_cast<const OCIO::OpData::Range *>(t.get(0))==o1);
    OIIO_CHECK_NO_THROW(t.validate());

    OCIO::OpData::OpData * myOp2 
        = o2.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY);
    t.append(myOp2);
    OIIO_CHECK_EQUAL(t.size(), 2);
    OIIO_CHECK_ASSERT(!t.empty());
    OIIO_CHECK_EQUAL(myOp1, t.get(0));
    OIIO_CHECK_EQUAL(myOp2, t.get(1));
    OIIO_CHECK_ASSERT(*dynamic_cast<const OCIO::OpData::Range *>(t.get(0))==o1);
    OIIO_CHECK_ASSERT(*dynamic_cast<const OCIO::OpData::Range *>(t.get(1))==o2);
    OIIO_CHECK_NO_THROW(t.validate());

    OCIO::OpData::OpDataVec t2;

    t2 = t;
    OIIO_CHECK_EQUAL(t2.size(), 2);
    OIIO_CHECK_ASSERT(!t2.empty());
    OIIO_CHECK_ASSERT(*dynamic_cast<const OCIO::OpData::Range *>(t2.get(0))==o1);
    OIIO_CHECK_ASSERT(*dynamic_cast<const OCIO::OpData::Range *>(t2.get(1))==o2);
    OIIO_CHECK_NO_THROW(t2.validate());

    t2 += t;
    OIIO_CHECK_EQUAL(t2.size(), 4);
    OIIO_CHECK_ASSERT(!t2.empty());
    OIIO_CHECK_ASSERT(*dynamic_cast<const OCIO::OpData::Range *>(t2.get(0))==o1);
    OIIO_CHECK_ASSERT(*dynamic_cast<const OCIO::OpData::Range *>(t2.get(1))==o2);
    OIIO_CHECK_ASSERT(*dynamic_cast<const OCIO::OpData::Range *>(t2.get(2))==o1);
    OIIO_CHECK_ASSERT(*dynamic_cast<const OCIO::OpData::Range *>(t2.get(3))==o2);
    OIIO_CHECK_NO_THROW(t2.validate());
}

OIIO_ADD_TEST(OpDataVec, Replace)
{
    OCIO::OpData::Range o1, o2, o3;
    OCIO::OpData::CDL o4, o5;

    o1.setName("1"); 
    o1.setInputBitDepth(OCIO::BIT_DEPTH_UINT8);
    o1.setOutputBitDepth(OCIO::BIT_DEPTH_UINT10);

    o2.setName("2");
    o2.setInputBitDepth(OCIO::BIT_DEPTH_UINT12);
    o2.setOutputBitDepth(OCIO::BIT_DEPTH_UINT8);

    o3.setName("3");
    o3.setInputBitDepth(OCIO::BIT_DEPTH_UINT10);
    o3.setOutputBitDepth(OCIO::BIT_DEPTH_UINT8);

    o4.setName("4");
    o4.setInputBitDepth(OCIO::BIT_DEPTH_UINT10);
    o4.setOutputBitDepth(OCIO::BIT_DEPTH_UINT8);

    o5.setName("5");
    o5.setInputBitDepth(OCIO::BIT_DEPTH_F16);
    o5.setOutputBitDepth(OCIO::BIT_DEPTH_F32);

    OCIO::OpData::OpDataVec t;
    t.append(o1.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY));
    t.append(o2.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY));
    t.append(o3.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY));
    t.append(o4.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY));

    // [o1, o2, o3, o4]
    OIIO_CHECK_EQUAL(t.get(1)->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    OIIO_CHECK_EQUAL(t.get(2)->getInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(t.get(3)->getInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_NO_THROW(t.validate());

    // replace o2 with o5
    // [o1, o5, o3, o4]
    t.replace( o5.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY), 1);
    OIIO_CHECK_EQUAL(t.size(), 4);
    OIIO_CHECK_EQUAL(t.get(0)->getName(), "1");
    OIIO_CHECK_EQUAL(t.get(1)->getName(), "5");
    OIIO_CHECK_EQUAL(t.get(2)->getName(), "3");
    OIIO_CHECK_EQUAL(t.get(1)->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    OIIO_CHECK_EQUAL(t.get(2)->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_NO_THROW(t.validate());

    // erase o3
    // [o1, o5, o4]
    t.erase( 2);
    OIIO_CHECK_EQUAL(t.size(), 3);
    OIIO_CHECK_EQUAL(t.get(2)->getName(), "4");
    OIIO_CHECK_EQUAL(t.get(2)->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_NO_THROW(t.validate());

    // replace last
    // [o1, o5, o5]
    t.replace( o5.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY), 2);
    OIIO_CHECK_EQUAL(t.get(2)->getName(), "5");
    OIIO_CHECK_EQUAL(t.get(2)->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_NO_THROW(t.validate());

    // replace first, second
    // [o5, o2, o5]
    t.replace( o2.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY), 1);
    t.replace( o5.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY), 0);
    OIIO_CHECK_EQUAL(t.get(0)->getName(), "5");
    OIIO_CHECK_EQUAL(t.get(1)->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(t.get(2)->getInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_NO_THROW(t.validate());

    // erase last
    // [o5, o2]
    t.erase( 2);
    OIIO_CHECK_EQUAL(t.size(), 2);
    OIIO_CHECK_EQUAL(t.get(1)->getName(), "2");
    OIIO_CHECK_NO_THROW(t.validate());

    // erase first
    // [o2]
    t.erase( 0);
    OIIO_CHECK_EQUAL(t.size(), 1);
    OIIO_CHECK_EQUAL(t.get(0)->getName(), "2");
    OIIO_CHECK_NO_THROW(t.validate());

    // replace only
    // [o5]
    t.replace( o5.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY), 0);
    OIIO_CHECK_EQUAL(t.get(0)->getName(), "5");
    OIIO_CHECK_NO_THROW(t.validate());

    // erase only
    // []
    t.erase( 0);
    OIIO_CHECK_EQUAL(t.size(), 0);
    OIIO_CHECK_NO_THROW(t.validate());

    // add o2, o5 and o4
    // [o2, o5, o4]
    t.append(o2.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY));
    t.append(o5.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY));
    t.append(o4.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY));

    OIIO_CHECK_EQUAL(t.size(), 3);
    OIIO_CHECK_EQUAL(t.get(1)->getInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(t.get(2)->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_NO_THROW(t.validate());

    // remove (without freeing) o5
    // [o2, o4]
    std::auto_ptr<OCIO::OpData::OpData> removedO5(t.remove(1));

    OIIO_CHECK_EQUAL(t.size(), 2);
    OIIO_CHECK_EQUAL(t.get(1)->getInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_NO_THROW(t.validate());

    // remove (without freeing) o2
    // [o4]
    std::auto_ptr<OCIO::OpData::OpData> removedO2(t.remove(0));

    OIIO_CHECK_EQUAL(t.size(), 1);
    OIIO_CHECK_NO_THROW(t.validate());

    // remove (without freeing) o4
    // []
    std::auto_ptr<OCIO::OpData::OpData> removedO4(t.remove(0));

    OIIO_CHECK_EQUAL(t.size(), 0);
    OIIO_CHECK_NO_THROW(t.validate());
}

OIIO_ADD_TEST(OpDataVec, Aggregation)
{
  OCIO::OpData::Range r1;
  r1.setInputBitDepth(OCIO::BIT_DEPTH_UINT8);  r1.setOutputBitDepth(OCIO::BIT_DEPTH_UINT16);
  OCIO::OpData::Range r2;
  r2.setInputBitDepth(OCIO::BIT_DEPTH_UINT10); r2.setOutputBitDepth(OCIO::BIT_DEPTH_UINT8);

  OCIO::OpData::OpDataVec t1;
  OIIO_CHECK_NO_THROW(t1.append(r1.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY)));

  OCIO::OpData::OpDataVec t2;
  OIIO_CHECK_NO_THROW(t2.append(r2.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY)));

  t1 += t2;
  OIIO_CHECK_EQUAL(t1.size(), 2);
  OIIO_CHECK_EQUAL(t1.get(0)->getOutputBitDepth(), t1.get(1)->getInputBitDepth());
  OIIO_CHECK_EQUAL(t1.get(0)->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT16);
  OIIO_CHECK_NO_THROW(t1.validate());
}

OIIO_ADD_TEST(OpDataVec, Equality)
{
  OCIO::OpData::Range r1, r2;
  OCIO::OpData::OpDataVec t1, t2;

  t1.append(r1.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY));

  OIIO_CHECK_ASSERT(!(t1 == t2));

  t2.append(r1.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY));
  
  OIIO_CHECK_ASSERT(t1==t2);

  t1.append(r2.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY));

  OIIO_CHECK_ASSERT(!(t1 == t2));

  t2.append(r2.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY));

  OIIO_CHECK_ASSERT(t1==t2);

}

OIIO_ADD_TEST(OpDataVec, Prepend)
{
  OCIO::OpData::Range r1;
  r1.setInputBitDepth(OCIO::BIT_DEPTH_UINT8);  r1.setOutputBitDepth(OCIO::BIT_DEPTH_UINT10);
  OCIO::OpData::Range r2;
  r2.setInputBitDepth(OCIO::BIT_DEPTH_UINT10); r2.setOutputBitDepth(OCIO::BIT_DEPTH_UINT8);

  {
    OCIO::OpData::OpDataVec t1;
    t1.append(r1.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY));
    t1.prepend(r2.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY));

    OIIO_CHECK_EQUAL(t1.size(), 2U);

    OIIO_CHECK_ASSERT(*t1.get(0)==r2);
    OIIO_CHECK_ASSERT(*t1.get(1)==r1);
  }

  {
    OCIO::OpData::OpDataVec t1;
    t1.append(r1.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY));
    t1.get(0)->setInputBitDepth(OCIO::BIT_DEPTH_F32);
    t1.prepend(r2.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY));

    OIIO_CHECK_EQUAL(t1.size(), 2U);
    OIIO_CHECK_EQUAL(t1.get(1)->getInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
  }
}

OIIO_ADD_TEST(OpDataVec, OpList)
{
    OCIO::OpData::Range o1, o2, o3;
    OCIO::OpData::CDL o4, o5;

    o1.setName("1"); 
    o1.setInputBitDepth(OCIO::BIT_DEPTH_UINT8); o1.setOutputBitDepth(OCIO::BIT_DEPTH_UINT10);

    o2.setName("2");
    o2.setInputBitDepth(OCIO::BIT_DEPTH_UINT12); o2.setOutputBitDepth(OCIO::BIT_DEPTH_UINT8);

    o3.setName("3");
    o3.setInputBitDepth(OCIO::BIT_DEPTH_UINT10); o3.setOutputBitDepth(OCIO::BIT_DEPTH_UINT8);

    o4.setName("4");
    o4.setInputBitDepth(OCIO::BIT_DEPTH_UINT10); o4.setOutputBitDepth(OCIO::BIT_DEPTH_UINT8);

    o5.setName("5");
    o5.setInputBitDepth(OCIO::BIT_DEPTH_F16); o5.setOutputBitDepth(OCIO::BIT_DEPTH_F32);


    // Check insertion at start of existing list.
    {
        OCIO::OpData::OpDataVec t;
        t.append(o2.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY));
        t.insert(o1.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY), 0);

        OIIO_CHECK_EQUAL(t.size(), 2u);
        OIIO_CHECK_EQUAL(t.get(0)->getName(), "1");
        OIIO_CHECK_EQUAL(t.get(1)->getName(), "2");
        OIIO_CHECK_EQUAL(t.get(0)->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_EQUAL(t.get(1)->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_NO_THROW(t.validate());
    }

    // Check insertion at end of existing list.
    {
        OCIO::OpData::OpDataVec t;
        t.append(o2.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY));
        t.insert(o1.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY), 1);

        OIIO_CHECK_EQUAL(t.size(), 2u);
        OIIO_CHECK_EQUAL(t.get(0)->getName(), "2");
        OIIO_CHECK_EQUAL(t.get(1)->getName(), "1");
        OIIO_CHECK_EQUAL(t.get(0)->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);
        OIIO_CHECK_EQUAL(t.get(1)->getInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
        OIIO_CHECK_NO_THROW(t.validate());
    }

    // Check insertion at start of empty list.
    {
        OCIO::OpData::OpDataVec t;
        t.insert(o1.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY), 0);

        OIIO_CHECK_EQUAL(t.size(), 1u);
        OIIO_CHECK_EQUAL(t.get(0)->getName(), "1");
        OIIO_CHECK_EQUAL(t.get(0)->getInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
        OIIO_CHECK_EQUAL(t.get(0)->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_NO_THROW(t.validate());
    }

    // Check insertion in middle of existing list.
    {
        OCIO::OpData::OpDataVec t;
        t.append(o1.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY));
        t.append(o2.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY));
        t.insert(o5.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY), 1);

        OIIO_CHECK_EQUAL(t.size(), 3u);
        OIIO_CHECK_EQUAL(t.get(0)->getName(), "1");
        OIIO_CHECK_EQUAL(t.get(1)->getName(), "5");
        OIIO_CHECK_EQUAL(t.get(2)->getName(), "2");
        OIIO_CHECK_EQUAL(t.get(0)->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_EQUAL(t.get(1)->getInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
        OIIO_CHECK_EQUAL(t.get(1)->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_EQUAL(t.get(2)->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
        OIIO_CHECK_NO_THROW(t.validate());
    }
}


#endif
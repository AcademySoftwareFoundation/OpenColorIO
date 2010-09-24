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
#include "OpBuilders.h"
#include "JPLogOp.h"

OCIO_NAMESPACE_ENTER
{
    JPLogTransformRcPtr
    JPLogTransform::Create()
    {
        return JPLogTransformRcPtr(new JPLogTransform(), &deleter);
    }
    
    void
    JPLogTransform::deleter(JPLogTransform* t)
    {
        delete t;
    }
    
    class JPLogTransform::Impl
    {
        
    public:
        
        TransformDirection dir_;
        
        Impl() :
            dir_(TRANSFORM_DIR_FORWARD)
        { }
        
        ~Impl()
        { }
        
        Impl& operator= (const Impl & rhs)
        {
            dir_ = rhs.dir_;
            return *this;
        }
    };
    
    ///////////////////////////////////////////////////////////////////////////
    
    JPLogTransform::JPLogTransform() :
        m_impl(new JPLogTransform::Impl)
    { }
    
    TransformRcPtr
    JPLogTransform::createEditableCopy() const
    {
        JPLogTransformRcPtr transform = JPLogTransform::Create();
        *(transform->m_impl) = *m_impl;
        return transform;
    }
    
    JPLogTransform::~JPLogTransform()
    { }
    
    JPLogTransform&
    JPLogTransform::operator= (const JPLogTransform& rhs)
    {
        *m_impl = *rhs.m_impl;
        return *this;
    }
    
    TransformDirection
    JPLogTransform::getDirection() const
    {
        return m_impl->dir_;
    }
    
    void
    JPLogTransform::setDirection(TransformDirection dir)
    {
        m_impl->dir_ = dir;
    }
    
    std::ostream&
    operator<< (std::ostream& os, const JPLogTransform& t)
    {
        os << "<JPLogTransform ";
        os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
        os << ">\n";
        return os;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    void
    BuildJPLogOps(OpRcPtrVec & ops,
                  const Config&, /* config */
                  const JPLogTransform& transform,
                  TransformDirection dir)
    {
        TransformDirection combinedDir =
            CombineTransformDirections(dir, transform.getDirection());
        
        if(combinedDir == TRANSFORM_DIR_FORWARD)
        {
            CreateJPLogOp(ops, TRANSFORM_DIR_FORWARD);
        }
        else if(combinedDir == TRANSFORM_DIR_INVERSE)
        {
            CreateJPLogOp(ops, TRANSFORM_DIR_INVERSE);
        } else {
            throw Exception("foo bar");
        }
    }
    
}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <cmath>
#include "tinyxml/tinyxml.h"

// check if two float values match
inline bool
is_equal (float x, float y)
{
    return std::abs(x - y) <= 1e-05 * std::abs(x); // 1e-05
}

BOOST_AUTO_TEST_SUITE( JPLogTransform_Unit_Tests )

BOOST_AUTO_TEST_CASE ( test_JPLogTransformCreation )
{
    
    OCIO::JPLogTransformRcPtr test_ptr = OCIO::JPLogTransform::Create();
    
    // default direction
    BOOST_CHECK_EQUAL (TransformDirectionToString(test_ptr->getDirection()), "forward");
    
    // test each direction
    test_ptr->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    BOOST_CHECK_EQUAL (TransformDirectionToString(test_ptr->getDirection()), "inverse");
    test_ptr->setDirection(OCIO::TRANSFORM_DIR_UNKNOWN);
    BOOST_CHECK_EQUAL (TransformDirectionToString(test_ptr->getDirection()), "unknown");
    test_ptr->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    BOOST_CHECK_EQUAL (TransformDirectionToString(test_ptr->getDirection()), "forward");
    
    // test ostream << operator
    test_ptr->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    std::ostringstream strebuf;
    strebuf << (*test_ptr);
    BOOST_CHECK_EQUAL (strebuf.str(), "<JPLogTransform direction=forward, >\n");
    
}

BOOST_AUTO_TEST_CASE ( test_JPLogTransformDeSerialization )
{
    
    std::ostringstream strebuf;
    strebuf << "<ocioconfig version='1' strictparsing='true' luma_r='0.2126' luma_g='0.7152' luma_b='0.0722' role_scene_linear='lnh'>\n";
    strebuf << "  <colorspace name='lnh' family='ln' bitdepth='16f' isdata='false' gpuallocation='uniform' gpumin='0' gpumax='14'>\n";
    strebuf << "    <description>scene ref linear</description>\n";
    strebuf << "  </colorspace>\n";
    strebuf << "  <colorspace name='lg10' family='lg' bitdepth='10ui' isdata='false' gpuallocation='uniform' gpumin='0' gpumax='1'>\n";
    strebuf << "    <description>JPLog De-Serialization Test</description>\n";
    strebuf << "      <to_reference>\n";
    strebuf << "        <group>\n";
    strebuf << "          <JPLog />\n";
    strebuf << "        </group>\n";
    strebuf << "      </to_reference>\n";
    strebuf << "  </colorspace>\n";
    strebuf << "</ocioconfig>\n";
    std::istringstream jplogxml;
    jplogxml.str(strebuf.str());
    
    OCIO::ConstConfigRcPtr config;
    BOOST_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(jplogxml));
    BOOST_CHECK_EQUAL (config->getNumColorSpaces(), 2);
    
    // lin2log
    OCIO::ConstProcessorRcPtr process_lin2log = config->getProcessor("lnh", "lg10");
    BOOST_CHECK_EQUAL (process_lin2log->isNoOp(), false);
    float pixel_lin[4] = {0.18f, 0.18f, 0.18f, 1.f};
    process_lin2log->applyRGBA(pixel_lin);
    // DEBUG
    //std::cout << "p: " << pixel_lin[0] << "," << pixel_lin[1] << "," << pixel_lin[2] << "," << pixel_lin[3] << "\n";
    BOOST_CHECK (is_equal(pixel_lin[0], 0.44477f));
    BOOST_CHECK (is_equal(pixel_lin[1], 0.44477f));
    BOOST_CHECK (is_equal(pixel_lin[2], 0.44477f));
    BOOST_CHECK (is_equal(pixel_lin[3], 1.f));
    
    // log2lin
    OCIO::ConstProcessorRcPtr process_log2lin = config->getProcessor("lg10", "lnh");
    BOOST_CHECK_EQUAL (process_log2lin->isNoOp(), false);
    float pixel_log[4] = {0.85f, 0.85f, 0.85f, 1.f};
    process_log2lin->applyRGBA(pixel_log);
    // DEBUG
    //std::cout << "p: " << pixel_log[0] << "," << pixel_log[1] << "," << pixel_log[2] << "," << pixel_log[3] << "\n";
    BOOST_CHECK (is_equal(pixel_log[0], 4.33617f));
    BOOST_CHECK (is_equal(pixel_log[1], 4.33617f));
    BOOST_CHECK (is_equal(pixel_log[2], 4.33617f));
    BOOST_CHECK (is_equal(pixel_log[3], 1.f));
    
}

BOOST_AUTO_TEST_CASE ( test_JPLogTransformFwdOps )
{
    
    OCIO::OpRcPtrVec testops;
    OCIO::ConfigRcPtr config_ptr = OCIO::Config::Create(); /* empty config */
    OCIO::JPLogTransformRcPtr test_ptr = OCIO::JPLogTransform::Create();
    OCIO::BuildJPLogOps(testops, *config_ptr, *test_ptr, OCIO::TRANSFORM_DIR_FORWARD);
    float pixel[4] = {0.5f, 0.5f, 0.5f, 1.f};
    for(OCIO::OpRcPtrVec::size_type i = 0, size = testops.size(); i < size; ++i)
    {
        testops[i]->apply(pixel, 1);
    }
    // DEBUG
    //std::cout << "p: " << pixel[0] << "," << pixel[1] << "," << pixel[2] << "," << pixel[3] << "\n";
    BOOST_CHECK (is_equal(pixel[0], 0.277719f));
    BOOST_CHECK (is_equal(pixel[1], 0.277719f));
    BOOST_CHECK (is_equal(pixel[2], 0.277719f));
    BOOST_CHECK (is_equal(pixel[3], 1.f));
    
}

BOOST_AUTO_TEST_CASE ( test_JPLogTransformInvOps )
{
    
    OCIO::OpRcPtrVec testops;
    OCIO::ConfigRcPtr config_ptr = OCIO::Config::Create(); /* empty config */
    OCIO::JPLogTransformRcPtr test_ptr = OCIO::JPLogTransform::Create();
    OCIO::BuildJPLogOps(testops, *config_ptr, *test_ptr, OCIO::TRANSFORM_DIR_INVERSE);
    float pixel[4] = {0.18f, 0.18f, 0.18f, 1.f};
    for(OCIO::OpRcPtrVec::size_type i = 0, size = testops.size(); i < size; ++i)
    {
        testops[i]->apply(pixel, 1);
    }
    // DEBUG
    //std::cout << "p: " << pixel[0] << "," << pixel[1] << "," << pixel[2] << "," << pixel[3] << "\n";
    BOOST_CHECK (is_equal(pixel[0], 0.44477f));
    BOOST_CHECK (is_equal(pixel[1], 0.44477f));
    BOOST_CHECK (is_equal(pixel[2], 0.44477f));
    BOOST_CHECK (is_equal(pixel[3], 1.f));
    
}

BOOST_AUTO_TEST_SUITE_END()

#endif // OCIO_BUILD_TESTS

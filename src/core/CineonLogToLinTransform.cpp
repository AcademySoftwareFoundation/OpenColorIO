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

#include <cstring>

#include <OpenColorIO/OpenColorIO.h>
#include "OpBuilders.h"
#include "LogOps.h"
#include "MathUtils.h"

OCIO_NAMESPACE_ENTER
{
    CineonLogToLinTransformRcPtr CineonLogToLinTransform::Create()
    {
        return CineonLogToLinTransformRcPtr(new CineonLogToLinTransform(), &deleter);
    }
    
    void CineonLogToLinTransform::deleter(CineonLogToLinTransform* t)
    {
        delete t;
    }
    
    class CineonLogToLinTransform::Impl
    {
    public:
        
        TransformDirection dir_;
        float maxAimDensity_[3];
        float negGamma_[3];
        float negGrayReference_[3];
        float linearGrayReference_[3];
        
        Impl() :
            dir_(TRANSFORM_DIR_FORWARD)
        {
            for(int i=0; i<3; ++i)
            {
                maxAimDensity_[i] = 2.046f;
                negGamma_[i] = 0.60f;
                negGrayReference_[i] = 445.0f/1023.0f;
                linearGrayReference_[i] = 0.18f;
            }
        }
        
        ~Impl()
        { }
        
        Impl& operator= (const Impl & rhs)
        {
            dir_ = rhs.dir_;
            memcpy(maxAimDensity_, rhs.maxAimDensity_, sizeof(float)*3);
            memcpy(negGamma_, rhs.negGamma_, sizeof(float)*3);
            memcpy(negGrayReference_, rhs.negGrayReference_, sizeof(float)*3);
            memcpy(linearGrayReference_, rhs.linearGrayReference_, sizeof(float)*3);
            
            return *this;
        }
    };
    
    ///////////////////////////////////////////////////////////////////////////
    
    CineonLogToLinTransform::CineonLogToLinTransform() :
        m_impl(new CineonLogToLinTransform::Impl)
    { }
    
    TransformRcPtr CineonLogToLinTransform::createEditableCopy() const
    {
        CineonLogToLinTransformRcPtr transform = CineonLogToLinTransform::Create();
        *(transform->m_impl) = *m_impl;
        return transform;
    }
    
    CineonLogToLinTransform::~CineonLogToLinTransform()
    { }
    
    CineonLogToLinTransform& CineonLogToLinTransform::operator= (const CineonLogToLinTransform& rhs)
    {
        *m_impl = *rhs.m_impl;
        return *this;
    }
    
    TransformDirection CineonLogToLinTransform::getDirection() const
    {
        return m_impl->dir_;
    }
    
    void CineonLogToLinTransform::setDirection(TransformDirection dir)
    {
        m_impl->dir_ = dir;
    }
    
    
    void CineonLogToLinTransform::getMaxAimDensity(float * v3) const
    {
        memcpy(v3, m_impl->maxAimDensity_, sizeof(float)*3);
    }
    
    void CineonLogToLinTransform::setMaxAimDensity(const float * v3)
    {
        memcpy(m_impl->maxAimDensity_, v3, sizeof(float)*3);
    }
    
    
    
    void CineonLogToLinTransform::getNegGamma(float * v3) const
    {
        memcpy(v3, m_impl->negGamma_, sizeof(float)*3);
    }
    
    void CineonLogToLinTransform::setNegGamma(const float * v3)
    {
        memcpy(m_impl->negGamma_, v3, sizeof(float)*3);
    }
    
    
    void CineonLogToLinTransform::getNegGrayReference(float * v3) const
    {
        memcpy(v3, m_impl->negGrayReference_, sizeof(float)*3);
    }
    
    void CineonLogToLinTransform::setNegGrayReference(const float * v3)
    {
        memcpy(m_impl->negGrayReference_, v3, sizeof(float)*3);
    }
    
    
    
    void CineonLogToLinTransform::getLinearGrayReference(float * v3) const
    {
        memcpy(v3, m_impl->linearGrayReference_, sizeof(float)*3);
    }
    
    void CineonLogToLinTransform::setLinearGrayReference(const float * v3)
    {
        memcpy(m_impl->linearGrayReference_, v3, sizeof(float)*3);
    }
    
    
    std::ostream& operator<< (std::ostream& os, const CineonLogToLinTransform& t)
    {
        os << "<CineonLogToLinTransform ";
        os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
        os << ">\n";
        return os;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    void BuildCineonLogToLinOps(OpRcPtrVec & ops,
                                const Config&, /* config */
                                const CineonLogToLinTransform& transform,
                                TransformDirection dir)
    {
        TransformDirection combinedDir =
            CombineTransformDirections(dir, transform.getDirection());
        
        float linearGrayRef[3];
        transform.getLinearGrayReference(linearGrayRef);
        
        float maxAimDensity[3];
        transform.getMaxAimDensity(maxAimDensity);
        
        float negGamma[3];
        transform.getNegGamma(negGamma);
        
        float negGrayRef[3];
        transform.getNegGrayReference(negGrayRef);
        
        
        
        // Compute constants for log convert
        
        if(VecContainsZero(maxAimDensity, 3))
        {
            std::ostringstream os;
            os << "CineonLogToLinTransform error, ";
            os << "maxAimDensity cannot have a 0.0 value.";
            throw Exception(os.str().c_str());
        }
        
        float k[3] = { negGamma[0] / maxAimDensity[0],
                       negGamma[1] / maxAimDensity[1],
                       negGamma[2] / maxAimDensity[2] };
        
        if(VecContainsZero(linearGrayRef, 3))
        {
            std::ostringstream os;
            os << "CineonLogToLinTransform error, ";
            os << "linearGrayRef cannot have a 0.0 value.";
            throw Exception(os.str().c_str());
        }
        
        float m[3] = { 1.0f / linearGrayRef[0],
                       1.0f / linearGrayRef[1],
                       1.0f / linearGrayRef[2] };
        
        float base[3] = { 10.0f, 10.0f, 10.0f };
        float b[3] = { 0.0f, 0.0f, 0.0f };
        
        // We get the inverse transform direction,
        // as CreateLogOp is a lin to log transform,
        // and this function is log to lin.
        
        combinedDir = GetInverseTransformDirection(combinedDir);
        
        CreateLogOp(ops,
                    k, m, b, base, negGrayRef,
                    combinedDir);
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

BOOST_AUTO_TEST_SUITE( CineonLogToLinTransform_Unit_Tests )

BOOST_AUTO_TEST_CASE ( test_CineonLogToLinTransform_Forward )
{
    OCIO::CineonLogToLinTransformRcPtr transform = OCIO::CineonLogToLinTransform::Create();
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    OCIO::ConstProcessorRcPtr processor = config->getProcessor(transform);
    
    float data[3] = { 0.0f,
                      0.43499511241446726f,
                      1.0f };
    float result[3] = { 0.0059147878456811799f,
                        0.18f,
                        15.203345734511421f };
    
    processor->applyRGB(data);
    
    for(int i=0; i<3; ++i)
    {
        BOOST_REQUIRE_CLOSE( data[i], result[i], 1.0e-3 );
    }
}

BOOST_AUTO_TEST_CASE ( test_CineonLogToLinTransform_Inverse )
{
    OCIO::CineonLogToLinTransformRcPtr transform = OCIO::CineonLogToLinTransform::Create();
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    OCIO::ConstProcessorRcPtr processor = config->getProcessor(transform, OCIO::TRANSFORM_DIR_INVERSE);
    
    float data[3] = { 0.0063979253982160934,
                      0.18f,
                      15.203345734511421f };
    
    float result[3] = { 0.01f,
                        0.43499511241446726f,
                        1.0f };
    
    processor->applyRGB(data);
    
    for(int i=0; i<3; ++i)
    {
        BOOST_REQUIRE_CLOSE( data[i], result[i], 1.0e-3 );
    }
}

BOOST_AUTO_TEST_SUITE_END()

#endif // OCIO_BUILD_TESTS

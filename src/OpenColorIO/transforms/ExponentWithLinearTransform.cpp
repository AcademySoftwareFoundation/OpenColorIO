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

#include <cstring>

#include <OpenColorIO/OpenColorIO.h>

#include "OpBuilders.h"
#include "ops/Gamma/GammaOpData.h"
#include "ops/Gamma/GammaOps.h"


OCIO_NAMESPACE_ENTER
{

ExponentWithLinearTransformRcPtr ExponentWithLinearTransform::Create()
{
    return ExponentWithLinearTransformRcPtr(new ExponentWithLinearTransform(), &deleter);
}

void ExponentWithLinearTransform::deleter(ExponentWithLinearTransform* t)
{
    delete t;
}


class ExponentWithLinearTransform::Impl : public GammaOpData
{
public:
    TransformDirection m_dir;
    
    Impl()
        :   GammaOpData()
        ,   m_dir(TRANSFORM_DIR_FORWARD)
    {
        setRedParams  ( {1.} );
        setGreenParams( {1.} );
        setBlueParams ( {1.} );
        setAlphaParams( {1.} );

        setStyle(GammaOpData::MONCURVE_FWD);
    }
    
    ~Impl()
    { }
    
    Impl& operator= (const Impl & rhs)
    {
        if (this != &rhs)
        {
            GammaOpData::operator=(rhs);
            m_dir = rhs.m_dir;
        }
        return *this;
    }


private:        
    Impl(const Impl & rhs);
};

///////////////////////////////////////////////////////////////////////////



ExponentWithLinearTransform::ExponentWithLinearTransform()
    : m_impl(new ExponentWithLinearTransform::Impl)
{
}

TransformRcPtr ExponentWithLinearTransform::createEditableCopy() const
{
    ExponentWithLinearTransformRcPtr transform = ExponentWithLinearTransform::Create();
    *(transform->m_impl) = *m_impl;
    return transform;
}

ExponentWithLinearTransform::~ExponentWithLinearTransform()
{
    delete m_impl;
    m_impl = nullptr;
}

ExponentWithLinearTransform& ExponentWithLinearTransform::operator= (const ExponentWithLinearTransform & rhs)
{
    if (this != &rhs)
    {
        *m_impl = *rhs.m_impl;
    }
    return *this;
}

TransformDirection ExponentWithLinearTransform::getDirection() const
{
    return getImpl()->m_dir;
}

void ExponentWithLinearTransform::setDirection(TransformDirection dir)
{
    getImpl()->m_dir = dir;
}

void ExponentWithLinearTransform::validate() const
{
    try
    {
        Transform::validate();
        getImpl()->validate();
    }
    catch(Exception & ex)
    {
        std::string errMsg("ExponentWithLinearTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }
}

void ExponentWithLinearTransform::setGamma(const double * vec4)
{
    if(vec4)
    {
        getImpl()->getRedParams()  [0] = vec4[0];
        getImpl()->getGreenParams()[0] = vec4[1];
        getImpl()->getBlueParams() [0] = vec4[2];
        getImpl()->getAlphaParams()[0] = vec4[3];
    }
}

void ExponentWithLinearTransform::getGamma(double * vec4) const
{
    if(vec4)
    {
        vec4[0] = getImpl()->getRedParams()  [0];
        vec4[1] = getImpl()->getGreenParams()[0];
        vec4[2] = getImpl()->getBlueParams() [0];
        vec4[3] = getImpl()->getAlphaParams()[0];
    }
}

void ExponentWithLinearTransform::setOffset(const double * vec4)
{
    if(vec4)
    {
        const GammaOpData::Params red = { getImpl()->getRedParams()[0]  , vec4[0] };
        const GammaOpData::Params grn = { getImpl()->getGreenParams()[0], vec4[1] };
        const GammaOpData::Params blu = { getImpl()->getBlueParams()[0] , vec4[2] };
        const GammaOpData::Params alp = { getImpl()->getAlphaParams()[0], vec4[3] };

        getImpl()->setRedParams(red);
        getImpl()->setGreenParams(grn);
        getImpl()->setBlueParams(blu);
        getImpl()->setAlphaParams(alp);
    }
}

void ExponentWithLinearTransform::getOffset(double * vec4) const
{
    vec4[0] = getImpl()->getRedParams().size()==2   ? getImpl()->getRedParams()[1]   : 0.;
    vec4[1] = getImpl()->getGreenParams().size()==2 ? getImpl()->getGreenParams()[1] : 0.;
    vec4[2] = getImpl()->getBlueParams().size()==2  ? getImpl()->getBlueParams()[1]  : 0.;
    vec4[3] = getImpl()->getAlphaParams().size()==2 ? getImpl()->getAlphaParams()[1] : 0.;
}

std::ostream& operator<< (std::ostream& os, const ExponentWithLinearTransform & t)
{
    os << "<ExponentWithLinearTransform ";
    os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
    
    double gamma[4];
    t.getGamma(gamma);

    os << "gamma=" << gamma[0];
    for (int i = 1; i < 4; ++i)
    {
      os << " " << gamma[i];
    }
    
    double offset[4];
    t.getOffset(offset);

    os << ", offset=" << offset[0];
    for (int i = 1; i < 4; ++i)
    {
      os << " " << offset[i];
    }   

    os << ">";
    return os;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////

void BuildExponentWithLinearOps(OpRcPtrVec & ops,
                                const Config & /*config*/,
                                const ExponentWithLinearTransform & transform,
                                TransformDirection dir)
{
    TransformDirection combinedDir 
        = CombineTransformDirections(dir, transform.getDirection());
    
    double gamma4[4] = { 1., 1., 1., 1. };
    transform.getGamma(gamma4);

    double offset4[4] = { 0., 0., 0., 0. };
    transform.getOffset(offset4);

    CreateGammaOp(ops, "", OpData::Descriptions(),
                  combinedDir==TRANSFORM_DIR_FORWARD ? GammaOpData::MONCURVE_FWD
                                                     : GammaOpData::MONCURVE_REV,
                  gamma4, offset4);
}

}
OCIO_NAMESPACE_EXIT


////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"


OIIO_ADD_TEST(ExponentWithLinearTransform, basic)
{
    OCIO::ExponentWithLinearTransformRcPtr exp = OCIO::ExponentWithLinearTransform::Create();
    OIIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    exp->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(exp->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    std::vector<double> val4(4, 1.), identity_val4(4, 1.);
    OIIO_CHECK_NO_THROW(exp->getGamma(&val4[0]));
    OIIO_CHECK_ASSERT(val4 == identity_val4);

    val4[1] = 2.;
    OIIO_CHECK_NO_THROW(exp->setGamma(&val4[0]));
    OIIO_CHECK_NO_THROW(exp->getGamma(&val4[0]));
    OIIO_CHECK_ASSERT(val4 != identity_val4);

    val4[1] = 1.;
    identity_val4 = { 0., 0., 0., 0. };
    OIIO_CHECK_NO_THROW(exp->getOffset(&val4[0]));
    OIIO_CHECK_ASSERT(val4 == identity_val4);

    val4[1] = 2.;
    OIIO_CHECK_NO_THROW(exp->setOffset(&val4[0]));
    OIIO_CHECK_NO_THROW(exp->getGamma(&val4[0]));
    OIIO_CHECK_ASSERT(val4 != identity_val4);
}

#endif

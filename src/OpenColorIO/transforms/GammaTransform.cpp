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

GammaTransformRcPtr GammaTransform::Create()
{
    return GammaTransformRcPtr(new GammaTransform(), &deleter);
}

void GammaTransform::deleter(GammaTransform* t)
{
    delete t;
}


class GammaTransform::Impl : public GammaOpData
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



GammaTransform::GammaTransform()
    : m_impl(new GammaTransform::Impl)
{
}

TransformRcPtr GammaTransform::createEditableCopy() const
{
    GammaTransformRcPtr transform = GammaTransform::Create();
    *(transform->m_impl) = *m_impl;
    return transform;
}

GammaTransform::~GammaTransform()
{
    delete m_impl;
    m_impl = nullptr;
}

GammaTransform& GammaTransform::operator= (const GammaTransform & rhs)
{
    if (this != &rhs)
    {
        *m_impl = *rhs.m_impl;
    }
    return *this;
}

TransformDirection GammaTransform::getDirection() const
{
    return getImpl()->m_dir;
}

void GammaTransform::setDirection(TransformDirection dir)
{
    getImpl()->m_dir = dir;
}

//
// At the config level, OCIO provides a direction attribute for transforms 
// and we add a public two-element style attribute to control the type of function applied. 
// 
// However, at the Gamma Op & OpData level, it is more convenient to have 
// the two direction+style attributes combined into a single four-element enum 
// that unambiguously identifies which rendering math to apply. The translation 
// to that GammaOpData::Style enum is done in this module and 
// so there is no separate direction enum in the op/Gamma modules. 
// This is also aligned with the four styles supported in CLF/CTF files.
//

GammaStyle GammaTransform::getStyle() const
{
    if(getImpl()->getStyle()==GammaOpData::MONCURVE_FWD)
    {
        return GAMMA_MONCURVE;
    }
    else
    {
        return GAMMA_BASIC;
    }
}

void GammaTransform::setStyle(GammaStyle style)
{
    if(style==GAMMA_MONCURVE)
    {
        getImpl()->setStyle(GammaOpData::MONCURVE_FWD);
    }
    else
    {
        getImpl()->setStyle(GammaOpData::BASIC_FWD);
    }
}

void GammaTransform::validate() const
{
    Transform::validate();
    getImpl()->validate();
}

void GammaTransform::setGammaValues(const double * vec4)
{
    if(vec4)
    {
        getImpl()->getRedParams()  [0] = vec4[0];
        getImpl()->getGreenParams()[0] = vec4[1];
        getImpl()->getBlueParams() [0] = vec4[2];
        getImpl()->getAlphaParams()[0] = vec4[3];
    }
}

void GammaTransform::getGammaValues(double * vec4) const
{
    if(vec4)
    {
        vec4[0] = getImpl()->getRedParams()  [0];
        vec4[1] = getImpl()->getGreenParams()[0];
        vec4[2] = getImpl()->getBlueParams() [0];
        vec4[3] = getImpl()->getAlphaParams()[0];
    }
}

void GammaTransform::setOffsetValues(const double * vec4)
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

void GammaTransform::getOffsetValues(double * vec4) const
{
    vec4[0] = getImpl()->getRedParams().size()==2   ? getImpl()->getRedParams()[1]   : 0.;
    vec4[1] = getImpl()->getGreenParams().size()==2 ? getImpl()->getGreenParams()[1] : 0.;
    vec4[2] = getImpl()->getBlueParams().size()==2  ? getImpl()->getBlueParams()[1]  : 0.;
    vec4[3] = getImpl()->getAlphaParams().size()==2 ? getImpl()->getAlphaParams()[1] : 0.;
}

std::ostream& operator<< (std::ostream& os, const GammaTransform & t)
{
    os << "<GammaTransform ";
    os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
    os << "style=" << GammaStyleToString(t.getStyle()) << ", ";
    
    double gamma[4];
    t.getGammaValues(gamma);

    os << "gamma=" << gamma[0];
    for (int i = 1; i < 4; ++i)
    {
      os << " " << gamma[i];
    }
    
    if(t.getStyle()==GAMMA_MONCURVE)
    {
        double offset[4];
        t.getOffsetValues(offset);

        os << ", offset=" << offset[0];
        for (int i = 1; i < 4; ++i)
        {
          os << " " << offset[i];
        }   
    }

    os << ">";
    return os;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////

void BuildGammaOps(OpRcPtrVec & ops,
                   const Config & /*config*/,
                   const GammaTransform & transform,
                   TransformDirection dir)
{
    TransformDirection combinedDir 
        = CombineTransformDirections(dir, transform.getDirection());
    
    double gamma4[4] = { 1., 1., 1., 1. };
    transform.getGammaValues(gamma4);

    if(transform.getStyle()==GAMMA_MONCURVE)
    {
        double offset4[4] = { 0., 0., 0., 0. };
        transform.getOffsetValues(offset4);

        CreateGammaOp(ops, "", OpData::Descriptions(),
                      combinedDir==TRANSFORM_DIR_FORWARD ? GammaOpData::MONCURVE_FWD
                                                         : GammaOpData::MONCURVE_REV,
                      gamma4, offset4);
    }
    else
    {

        CreateGammaOp(ops, "", OpData::Descriptions(),
                      combinedDir==TRANSFORM_DIR_FORWARD ? GammaOpData::BASIC_FWD
                                                         : GammaOpData::BASIC_REV,
                      gamma4, nullptr);
    }
}

}
OCIO_NAMESPACE_EXIT


////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"


OCIO_NAMESPACE_USING


OIIO_ADD_TEST(GammaTransform, basic)
{
    OCIO::GammaTransformRcPtr gamma = OCIO::GammaTransform::Create();
    OIIO_CHECK_EQUAL(gamma->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(gamma->getStyle(), OCIO::GAMMA_BASIC);

    gamma->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(gamma->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    std::vector<double> val4(4, 1.), identity_val4(4, 1.);
    OIIO_CHECK_NO_THROW(gamma->getGammaValues(&val4[0]));
    OIIO_CHECK_ASSERT(val4 == identity_val4);

    val4[1] = 2.;
    OIIO_CHECK_NO_THROW(gamma->setGammaValues(&val4[0]));
    OIIO_CHECK_NO_THROW(gamma->getGammaValues(&val4[0]));
    OIIO_CHECK_ASSERT(val4 != identity_val4);

    OIIO_CHECK_NO_THROW(gamma->setStyle(OCIO::GAMMA_MONCURVE));
    OIIO_CHECK_EQUAL(gamma->getStyle(), OCIO::GAMMA_MONCURVE);

    val4[1] = 1.;
    identity_val4 = { 0., 0., 0., 0. };
    OIIO_CHECK_NO_THROW(gamma->getOffsetValues(&val4[0]));
    OIIO_CHECK_ASSERT(val4 == identity_val4);

    val4[1] = 2.;
    OIIO_CHECK_NO_THROW(gamma->setOffsetValues(&val4[0]));
    OIIO_CHECK_NO_THROW(gamma->getGammaValues(&val4[0]));
    OIIO_CHECK_ASSERT(val4 != identity_val4);
}

#endif

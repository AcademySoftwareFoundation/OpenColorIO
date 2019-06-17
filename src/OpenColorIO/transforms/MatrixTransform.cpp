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

#include "MathUtils.h"
#include "OpBuilders.h"
#include "ops/Matrix/MatrixOps.h"


OCIO_NAMESPACE_ENTER
{
    MatrixTransformRcPtr MatrixTransform::Create()
    {
        return MatrixTransformRcPtr(new MatrixTransform(), &deleter);
    }
    
    void MatrixTransform::deleter(MatrixTransform* t)
    {
        delete t;
    }
    
    class MatrixTransform::Impl : public MatrixOpData
    {
    public:
        TransformDirection dir_;
        
        Impl()
            : MatrixOpData()
            , dir_(TRANSFORM_DIR_FORWARD)
        {
        }

        Impl(const Impl &) = delete;

        ~Impl()
        { }
        
        Impl& operator= (const Impl & rhs)
        {
            if (this != &rhs)
            {
                MatrixOpData::operator=(rhs);
                dir_ = rhs.dir_;
            }
            return *this;
        }
    };
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    MatrixTransform::MatrixTransform()
        : m_impl(new MatrixTransform::Impl)
    {
    }
    
    TransformRcPtr MatrixTransform::createEditableCopy() const
    {
        MatrixTransformRcPtr transform = MatrixTransform::Create();
        *(transform->m_impl) = *m_impl;
        return transform;
    }
    
    MatrixTransform::~MatrixTransform()
    {
        delete m_impl;
        m_impl = NULL;
    }
    
    MatrixTransform& MatrixTransform::operator= (const MatrixTransform & rhs)
    {
        if (this != &rhs)
        {
            *m_impl = *rhs.m_impl;
        }
        return *this;
    }
    
    TransformDirection MatrixTransform::getDirection() const
    {
        return getImpl()->dir_;
    }
    
    void MatrixTransform::setDirection(TransformDirection dir)
    {
        getImpl()->dir_ = dir;
    }
    
    void MatrixTransform::validate() const
    {
        try
        {
            Transform::validate();
            getImpl()->validate();
        }
        catch(Exception & ex)
        {
            std::string errMsg("MatrixTransform validation failed: ");
            errMsg += ex.what();
            throw Exception(errMsg.c_str());
        }
    }

    bool MatrixTransform::equals(const MatrixTransform & other) const
    {
        if (this == &other) return true;

        if (getImpl()->dir_ != other.getImpl()->dir_)
        {
            return false;
        }

        return *getImpl() == *(other.getImpl());
    }
    
    void MatrixTransform::getValue(float * m44, float * offset4) const
    {
        getMatrix(m44);
        getOffset(offset4);
    }
    
    void MatrixTransform::setValue(const float * m44, const float * offset4)
    {
        setMatrix(m44);
        setOffset(offset4);
    }
    
    void MatrixTransform::setMatrix(const float * m44)
    {
        if (m44) getImpl()->setRGBA(m44);
    }
    
    void MatrixTransform::setMatrix(const double * m44)
    {
        if (m44) getImpl()->setRGBA(m44);
    }

    template<typename T>
    void GetMatrix(const ArrayDouble::Values & vals, T * m44)
    {
        static_assert(std::is_floating_point<T>::value, 
                      "Only single and double precision floats are supported");

        if (m44)
        {
            m44[0]  = (T)vals[0];
            m44[1]  = (T)vals[1];
            m44[2]  = (T)vals[2];
            m44[3]  = (T)vals[3];
            m44[4]  = (T)vals[4];
            m44[5]  = (T)vals[5];
            m44[6]  = (T)vals[6];
            m44[7]  = (T)vals[7];
            m44[8]  = (T)vals[8];
            m44[9]  = (T)vals[9];
            m44[10] = (T)vals[10];
            m44[11] = (T)vals[11];
            m44[12] = (T)vals[12];
            m44[13] = (T)vals[13];
            m44[14] = (T)vals[14];
            m44[15] = (T)vals[15];
        }
    }
    
    void MatrixTransform::getMatrix(float * m44) const
    {
        const ArrayDouble::Values & vals = getImpl()->getArray().getValues();
        GetMatrix(vals, m44);
    }
    
    void MatrixTransform::getMatrix(double * m44) const
    {
        const ArrayDouble::Values & vals = getImpl()->getArray().getValues();
        GetMatrix(vals, m44);
    }
    
    void MatrixTransform::setOffset(const float * offset4)
    {
        if (offset4) getImpl()->setRGBAOffsets(offset4);
    }
    
    void MatrixTransform::setOffset(const double * offset4)
    {
        if (offset4) getImpl()->setRGBAOffsets(offset4);
    }

    template<typename T>
    void GetOffset(const double * vals, T * offset4)
    {
        static_assert(std::is_floating_point<T>::value, 
                      "Only single and double precision floats are supported");

        if(offset4)
        {
            offset4[0] = (T)vals[0];
            offset4[1] = (T)vals[1];
            offset4[2] = (T)vals[2];
            offset4[3] = (T)vals[3];
        }
    }
    
    void MatrixTransform::getOffset(float * offset4) const
    {
        const double * vals = getImpl()->getOffsets().getValues();
        GetOffset(vals, offset4);
    }
    
    void MatrixTransform::getOffset(double * offset4) const
    {
        const double * vals = getImpl()->getOffsets().getValues();
        GetOffset(vals, offset4);
    }
    
    /*
    Fit is canonically formulated as:
    out = newmin + ((value-oldmin)/(oldmax-oldmin)*(newmax-newmin))
    I.e., subtract the old offset, descale into the [0,1] range,
          scale into the new range, and add the new offset
    
    We algebraiclly manipulate the terms into y = mx + b form as:
    m = (newmax-newmin)/(oldmax-oldmin)
    b = (newmin*oldmax - newmax*oldmin) / (oldmax-oldmin)
    */
    
    void MatrixTransform::Fit(float * m44, float * offset4,
                              const float * oldmin4, const float * oldmax4,
                              const float * newmin4, const float * newmax4)
    {
        if(!oldmin4 || !oldmax4) return;
        if(!newmin4 || !newmax4) return;
        
        if(m44) memset(m44, 0, 16*sizeof(float));
        if(offset4) memset(offset4, 0, 4*sizeof(float));
        
        for(int i=0; i<4; ++i)
        {
            float denom = oldmax4[i] - oldmin4[i];
            if(IsScalarEqualToZero(denom))
            {
                std::ostringstream os;
                os << "Cannot create Fit operator. ";
                os << "Max value equals min value '";
                os << oldmax4[i] << "' in channel index ";
                os << i << ".";
                throw Exception(os.str().c_str());
            }
            
            if(m44) m44[5*i] = (newmax4[i]-newmin4[i]) / denom;
            if(offset4) offset4[i] = (newmin4[i]*oldmax4[i] - newmax4[i]*oldmin4[i]) / denom;
        }
    }
    
    void MatrixTransform::Identity(float * m44, float * offset4)
    {
        if (m44)
        {
            memset(m44, 0, 16 * sizeof(float));
            m44[0] = 1.0f;
            m44[5] = 1.0f;
            m44[10] = 1.0f;
            m44[15] = 1.0f;
        }

        if (offset4)
        {
            offset4[0] = 0.0f;
            offset4[1] = 0.0f;
            offset4[2] = 0.0f;
            offset4[3] = 0.0f;
        }
    }

    void MatrixTransform::Sat(float * m44, float * offset4,
                              float sat, const float * lumaCoef3)
    {
        if(!lumaCoef3) return;
        
        if(m44)
        {
            m44[0] = (1 - sat) * lumaCoef3[0] + sat;
            m44[1] = (1 - sat) * lumaCoef3[1];
            m44[2] = (1 - sat) * lumaCoef3[2];
            m44[3] = 0.0f;
            
            m44[4] = (1 - sat) * lumaCoef3[0];
            m44[5] = (1 - sat) * lumaCoef3[1] + sat;
            m44[6] = (1 - sat) * lumaCoef3[2];
            m44[7] = 0.0f;
            
            m44[8] = (1 - sat) * lumaCoef3[0];
            m44[9] = (1 - sat) * lumaCoef3[1];
            m44[10] = (1 - sat) * lumaCoef3[2] + sat;
            m44[11] = 0.0f;
            
            m44[12] = 0.0f;
            m44[13] = 0.0f;
            m44[14] = 0.0f;
            m44[15] = 1.0f;
        }
        
        if(offset4)
        {
            offset4[0] = 0.0f;
            offset4[1] = 0.0f;
            offset4[2] = 0.0f;
            offset4[3] = 0.0f;
        }
    }
    
    void MatrixTransform::Scale(float * m44, float * offset4,
                                const float * scale4)
    {
        if(!scale4) return;
        
        if(m44)
        {
            memset(m44, 0, 16*sizeof(float));
            m44[0] = scale4[0];
            m44[5] = scale4[1];
            m44[10] = scale4[2];
            m44[15] = scale4[3];
        }
        
        if(offset4)
        {
            offset4[0] = 0.0f;
            offset4[1] = 0.0f;
            offset4[2] = 0.0f;
            offset4[3] = 0.0f;
        }
    }
    
    void MatrixTransform::View(float * m44, float * offset4,
                               int * channelHot4,
                               const float * lumaCoef3)
    {
        if(!channelHot4 || !lumaCoef3) return;
        
        if(offset4)
        {
            offset4[0] = 0.0f;
            offset4[1] = 0.0f;
            offset4[2] = 0.0f;
            offset4[3] = 0.0f;
        }
        
        if(m44)
        {
            memset(m44, 0, 16*sizeof(float));
            
            // All channels are hot, return identity
            if(channelHot4[0] && channelHot4[1] &&
               channelHot4[2] && channelHot4[3])
            {
                Identity(m44, 0x0);
            }
            // If not all the channels are hot, but alpha is,
            // just show it.
            else if(channelHot4[3])
            {
                for(int i=0; i<4; ++i)
                {
                     m44[4*i+3] = 1.0f;
                }
            }
            // Blend rgb as specified, place it in all 3 output
            // channels (to make a grayscale final image)
            else
            {
                float values[3] = { 0.0f, 0.0f, 0.0f };
                
                for(int i = 0; i < 3; ++i)
                {
                    values[i] += lumaCoef3[i] * (channelHot4[i] ? 1.0f : 0.0f);
                }
                
                float sum = values[0] + values[1] + values[2];
                if(!IsScalarEqualToZero(sum))
                {
                    values[0] /= sum;
                    values[1] /= sum;
                    values[2] /= sum;
                }
                
                // Copy rgb into rgb rows
                for(int row=0; row<3; ++row)
                {
                    for(int i=0; i<3; i++)
                    {
                        m44[4*row+i] = values[i];
                    }
                }
                
                // Preserve alpha
                m44[15] = 1.0f;
            }
        }
    }
    
    std::ostream& operator<< (std::ostream& os, const MatrixTransform& t)
    {
        float matrix[16], offset[4];

        t.getMatrix(matrix);
        t.getOffset(offset);

        os << "<MatrixTransform ";
        os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
        os << "matrix=" << matrix[0];
        for (int i = 1; i < 16; ++i)
        {
            os << " " << matrix[i];
        }
        os << ", offset=" << offset[0];
        for (int i = 1; i < 4; ++i)
        {
            os << " " << offset[i];
        }
        os << ">";
        return os;
    }
        
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    void BuildMatrixOps(OpRcPtrVec & ops,
                        const Config& /*config*/,
                        const MatrixTransform & transform,
                        TransformDirection dir)
    {
        TransformDirection combinedDir = CombineTransformDirections(dir,
            transform.getDirection());
        
        float matrix[16];
        float offset[4];
        transform.getValue(matrix, offset);
        
        CreateMatrixOffsetOp(ops,
                             matrix, offset,
                             combinedDir);
    }
    
}
OCIO_NAMESPACE_EXIT

////////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

OIIO_ADD_TEST(MatrixTransform, basic)
{
    OCIO::MatrixTransformRcPtr matrix = OCIO::MatrixTransform::Create();
    OIIO_CHECK_EQUAL(matrix->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    
    float m44[16];
    float offset4[4];
    matrix->getValue(m44, offset4);

    OIIO_CHECK_EQUAL(m44[0], 1.0f);
    OIIO_CHECK_EQUAL(m44[1], 0.0f);
    OIIO_CHECK_EQUAL(m44[2], 0.0f);
    OIIO_CHECK_EQUAL(m44[3], 0.0f);
    
    OIIO_CHECK_EQUAL(m44[4], 0.0f);
    OIIO_CHECK_EQUAL(m44[5], 1.0f);
    OIIO_CHECK_EQUAL(m44[6], 0.0f);
    OIIO_CHECK_EQUAL(m44[7], 0.0f);
    
    OIIO_CHECK_EQUAL(m44[8], 0.0f);
    OIIO_CHECK_EQUAL(m44[9], 0.0f);
    OIIO_CHECK_EQUAL(m44[10], 1.0f);
    OIIO_CHECK_EQUAL(m44[11], 0.0f);
    
    OIIO_CHECK_EQUAL(m44[12], 0.0f);
    OIIO_CHECK_EQUAL(m44[13], 0.0f);
    OIIO_CHECK_EQUAL(m44[14], 0.0f);
    OIIO_CHECK_EQUAL(m44[15], 1.0f);

    OIIO_CHECK_EQUAL(offset4[0], 0.0f);
    OIIO_CHECK_EQUAL(offset4[1], 0.0f);
    OIIO_CHECK_EQUAL(offset4[2], 0.0f);
    OIIO_CHECK_EQUAL(offset4[3], 0.0f);

    matrix->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(matrix->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    m44[0]  = 1.0f;
    m44[1]  = 1.01f;
    m44[2]  = 1.02f;
    m44[3]  = 1.03f;
            
    m44[4]  = 1.04f;
    m44[5]  = 1.05f;
    m44[6]  = 1.06f;
    m44[7]  = 1.07f;
            
    m44[8]  = 1.08f;
    m44[9]  = 1.09f;
    m44[10] = 1.10f;
    m44[11] = 1.11f;

    m44[12] = 1.12f;
    m44[13] = 1.13f;
    m44[14] = 1.14f;
    m44[15] = 1.15f;

    offset4[0] = 1.0f;
    offset4[1] = 1.1f;
    offset4[2] = 1.2f;
    offset4[3] = 1.3f;

    matrix->setValue(m44, offset4);

    float m44r[16];
    float offset4r[4];

    matrix->getValue(m44r, offset4r);

    for (int i = 0; i < 16; ++i)
    {
        OIIO_CHECK_EQUAL(m44r[i], m44[i]);
    }

    OIIO_CHECK_EQUAL(offset4r[0], 1.0f);
    OIIO_CHECK_EQUAL(offset4r[1], 1.1f);
    OIIO_CHECK_EQUAL(offset4r[2], 1.2f);
    OIIO_CHECK_EQUAL(offset4r[3], 1.3f);
}

OIIO_ADD_TEST(MatrixTransform, equals)
{
    OCIO::MatrixTransformRcPtr matrix1 = OCIO::MatrixTransform::Create();
    OCIO::MatrixTransformRcPtr matrix2 = OCIO::MatrixTransform::Create();

    OIIO_CHECK_ASSERT(matrix1->equals(*matrix2));

    matrix1->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_ASSERT(!matrix1->equals(*matrix2));
    matrix1->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    float m44[16];
    float offset4[4];
    matrix1->getValue(m44, offset4);
    m44[0] = 1.0f + 1e-6f;
    matrix1->setValue(m44, offset4);
    OIIO_CHECK_ASSERT(!matrix1->equals(*matrix2));
    m44[0] = 1.0f;
    matrix1->setValue(m44, offset4);

    offset4[0] = 1e-6f;
    matrix1->setValue(m44, offset4);
    OIIO_CHECK_ASSERT(!matrix1->equals(*matrix2));
}

#endif
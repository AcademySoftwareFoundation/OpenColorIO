// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>

#include <OpenColorIO/OpenColorIO.h>

#include "MathUtils.h"
#include "ops/Matrix/MatrixOpData.h"

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
        TransformDirection m_dir = TRANSFORM_DIR_FORWARD;

        Impl()
            : MatrixOpData()
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
                m_dir = rhs.m_dir;
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
        return getImpl()->m_dir;
    }
    
    void MatrixTransform::setDirection(TransformDirection dir)
    {
        getImpl()->m_dir = dir;
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

    BitDepth MatrixTransform::getFileInputBitDepth() const
    {
        return getImpl()->getFileInputBitDepth();
    }
    BitDepth MatrixTransform::getFileOutputBitDepth() const
    {
        return getImpl()->getFileOutputBitDepth();
    }
    void MatrixTransform::setFileInputBitDepth(BitDepth bitDepth)
    {
        getImpl()->setFileInputBitDepth(bitDepth);
    }
    void MatrixTransform::setFileOutputBitDepth(BitDepth bitDepth)
    {
        getImpl()->setFileOutputBitDepth(bitDepth);
    }

    FormatMetadata & MatrixTransform::getFormatMetadata()
    {
        return m_impl->getFormatMetadata();
    }

    const FormatMetadata & MatrixTransform::getFormatMetadata() const
    {
        return m_impl->getFormatMetadata();
    }

    bool MatrixTransform::equals(const MatrixTransform & other) const
    {
        if (this == &other) return true;

        if (getImpl()->m_dir != other.getImpl()->m_dir)
        {
            return false;
        }

        return *getImpl() == *(other.getImpl());
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

    void MatrixTransform::getMatrix(double * m44) const
    {
        const ArrayDouble::Values & vals = getImpl()->getArray().getValues();
        GetMatrix(vals, m44);
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
    void MatrixTransform::Fit(double * m44, double * offset4,
                              const double * oldmin4, const double * oldmax4,
                              const double * newmin4, const double * newmax4)
    {
        if(!oldmin4 || !oldmax4) return;
        if(!newmin4 || !newmax4) return;
        
        if(m44) memset(m44, 0, 16*sizeof(double));
        if(offset4) memset(offset4, 0, 4*sizeof(double));
        
        for(int i=0; i<4; ++i)
        {
            double denom = oldmax4[i] - oldmin4[i];
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

    void MatrixTransform::Identity(double * m44, double * offset4)
    {
        if (m44)
        {
            memset(m44, 0, 16 * sizeof(double));
            m44[0] = 1.0;
            m44[5] = 1.0;
            m44[10] = 1.0;
            m44[15] = 1.0;
        }

        if (offset4)
        {
            offset4[0] = 0.0;
            offset4[1] = 0.0;
            offset4[2] = 0.0;
            offset4[3] = 0.0;
        }
    }

    void MatrixTransform::Sat(double * m44, double * offset4,
                              double sat, const double * lumaCoef3)
    {
        if(!lumaCoef3) return;
        
        if(m44)
        {
            m44[0] = (1 - sat) * lumaCoef3[0] + sat;
            m44[1] = (1 - sat) * lumaCoef3[1];
            m44[2] = (1 - sat) * lumaCoef3[2];
            m44[3] = 0.0;
            
            m44[4] = (1 - sat) * lumaCoef3[0];
            m44[5] = (1 - sat) * lumaCoef3[1] + sat;
            m44[6] = (1 - sat) * lumaCoef3[2];
            m44[7] = 0.0;
            
            m44[8] = (1 - sat) * lumaCoef3[0];
            m44[9] = (1 - sat) * lumaCoef3[1];
            m44[10] = (1 - sat) * lumaCoef3[2] + sat;
            m44[11] = 0.0;
            
            m44[12] = 0.0;
            m44[13] = 0.0;
            m44[14] = 0.0;
            m44[15] = 1.0;
        }
        
        if(offset4)
        {
            offset4[0] = 0.0;
            offset4[1] = 0.0;
            offset4[2] = 0.0;
            offset4[3] = 0.0;
        }
    }

    void MatrixTransform::Scale(double * m44, double * offset4,
                                const double * scale4)
    {
        if(!scale4) return;
        
        if(m44)
        {
            memset(m44, 0, 16*sizeof(double));
            m44[0] = scale4[0];
            m44[5] = scale4[1];
            m44[10] = scale4[2];
            m44[15] = scale4[3];
        }
        
        if(offset4)
        {
            offset4[0] = 0.0;
            offset4[1] = 0.0;
            offset4[2] = 0.0;
            offset4[3] = 0.0;
        }
    }
    void MatrixTransform::View(double * m44, double * offset4,
                               int * channelHot4,
                               const double * lumaCoef3)
    {
        if(!channelHot4 || !lumaCoef3) return;
        
        if(offset4)
        {
            offset4[0] = 0.0;
            offset4[1] = 0.0;
            offset4[2] = 0.0;
            offset4[3] = 0.0;
        }
        
        if(m44)
        {
            memset(m44, 0, 16*sizeof(double));
            
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
                     m44[4*i+3] = 1.0;
                }
            }
            // Blend rgb as specified, place it in all 3 output
            // channels (to make a grayscale final image)
            else
            {
                double values[3] = { 0.0, 0.0, 0.0 };
                
                for(int i = 0; i < 3; ++i)
                {
                    values[i] += lumaCoef3[i] * (channelHot4[i] ? 1.0 : 0.0);
                }
                
                double sum = values[0] + values[1] + values[2];
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
                m44[15] = 1.0;
            }
        }
    }
    
    namespace
    {
        const int DOUBLE_DECIMALS = 16;
    }

    std::ostream& operator<< (std::ostream& os, const MatrixTransform& t)
    {
        double matrix[16], offset[4];

        t.getMatrix(matrix);
        t.getOffset(offset);

        os.precision(DOUBLE_DECIMALS);

        os << "<MatrixTransform ";
        os << "direction=" << TransformDirectionToString(t.getDirection());
        os << ", fileindepth=" << BitDepthToString(t.getFileInputBitDepth());
        os << ", fileoutdepth=" << BitDepthToString(t.getFileOutputBitDepth());
        os << ", matrix=" << matrix[0];
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
        
}
OCIO_NAMESPACE_EXIT

////////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

OCIO_ADD_TEST(MatrixTransform, basic)
{
    OCIO::MatrixTransformRcPtr matrix = OCIO::MatrixTransform::Create();
    OCIO_CHECK_EQUAL(matrix->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    
    double m44[16];
    double offset4[4];
    matrix->getMatrix(m44);
    matrix->getOffset(offset4);

    OCIO_CHECK_EQUAL(m44[0], 1.0);
    OCIO_CHECK_EQUAL(m44[1], 0.0);
    OCIO_CHECK_EQUAL(m44[2], 0.0);
    OCIO_CHECK_EQUAL(m44[3], 0.0);
    
    OCIO_CHECK_EQUAL(m44[4], 0.0);
    OCIO_CHECK_EQUAL(m44[5], 1.0);
    OCIO_CHECK_EQUAL(m44[6], 0.0);
    OCIO_CHECK_EQUAL(m44[7], 0.0);
    
    OCIO_CHECK_EQUAL(m44[8], 0.0);
    OCIO_CHECK_EQUAL(m44[9], 0.0);
    OCIO_CHECK_EQUAL(m44[10], 1.0);
    OCIO_CHECK_EQUAL(m44[11], 0.0);
    
    OCIO_CHECK_EQUAL(m44[12], 0.0);
    OCIO_CHECK_EQUAL(m44[13], 0.0);
    OCIO_CHECK_EQUAL(m44[14], 0.0);
    OCIO_CHECK_EQUAL(m44[15], 1.0);

    OCIO_CHECK_EQUAL(offset4[0], 0.0);
    OCIO_CHECK_EQUAL(offset4[1], 0.0);
    OCIO_CHECK_EQUAL(offset4[2], 0.0);
    OCIO_CHECK_EQUAL(offset4[3], 0.0);

    matrix->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(matrix->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    m44[0]  = 1.0;
    m44[1]  = 1.01;
    m44[2]  = 1.02;
    m44[3]  = 1.03;
            
    m44[4]  = 1.04;
    m44[5]  = 1.05;
    m44[6]  = 1.06;
    m44[7]  = 1.07;
            
    m44[8]  = 1.08;
    m44[9]  = 1.09;
    m44[10] = 1.10;
    m44[11] = 1.11;

    m44[12] = 1.12;
    m44[13] = 1.13;
    m44[14] = 1.14;
    m44[15] = 1.15;

    offset4[0] = 1.0;
    offset4[1] = 1.1;
    offset4[2] = 1.2;
    offset4[3] = 1.3;

    matrix->setMatrix(m44);
    matrix->setOffset(offset4);

    double m44r[16];
    double offset4r[4];

    matrix->getMatrix(m44r);
    matrix->getOffset(offset4r);

    for (int i = 0; i < 16; ++i)
    {
        OCIO_CHECK_EQUAL(m44r[i], m44[i]);
    }

    OCIO_CHECK_EQUAL(offset4r[0], 1.0);
    OCIO_CHECK_EQUAL(offset4r[1], 1.1);
    OCIO_CHECK_EQUAL(offset4r[2], 1.2);
    OCIO_CHECK_EQUAL(offset4r[3], 1.3);

    OCIO_CHECK_EQUAL(matrix->getFileInputBitDepth(), OCIO::BIT_DEPTH_UNKNOWN);
    OCIO_CHECK_EQUAL(matrix->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UNKNOWN);

    matrix->setFileInputBitDepth(OCIO::BIT_DEPTH_UINT8);
    matrix->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);

    OCIO_CHECK_EQUAL(matrix->getFileInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OCIO_CHECK_EQUAL(matrix->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

    // File bit-depth does not affect values.
    matrix->getMatrix(m44r);
    matrix->getOffset(offset4r);

    for (int i = 0; i < 16; ++i)
    {
        OCIO_CHECK_EQUAL(m44r[i], m44[i]);
    }

    OCIO_CHECK_EQUAL(offset4r[0], 1.0);
    OCIO_CHECK_EQUAL(offset4r[1], 1.1);
    OCIO_CHECK_EQUAL(offset4r[2], 1.2);
    OCIO_CHECK_EQUAL(offset4r[3], 1.3);
}

OCIO_ADD_TEST(MatrixTransform, equals)
{
    OCIO::MatrixTransformRcPtr matrix1 = OCIO::MatrixTransform::Create();
    OCIO::MatrixTransformRcPtr matrix2 = OCIO::MatrixTransform::Create();

    OCIO_CHECK_ASSERT(matrix1->equals(*matrix2));

    matrix1->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_ASSERT(!matrix1->equals(*matrix2));
    matrix1->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    double m44[16];
    double offset4[4];
    matrix1->getMatrix(m44);
    matrix1->getOffset(offset4);
    m44[0] = 1.0 + 1e-6;
    matrix1->setMatrix(m44);
    OCIO_CHECK_ASSERT(!matrix1->equals(*matrix2));
    m44[0] = 1.0;
    matrix1->setMatrix(m44);
    OCIO_CHECK_ASSERT(matrix1->equals(*matrix2));

    offset4[0] = 1e-6;
    matrix1->setOffset(offset4);
    OCIO_CHECK_ASSERT(!matrix1->equals(*matrix2));
}

#endif
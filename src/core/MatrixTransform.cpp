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
#include "MatrixOps.h"
#include "MathUtils.h"


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
    
    class MatrixTransform::Impl
    {
    public:
        TransformDirection dir_;
        float matrix_[16];
        float offset_[4];
        
        Impl() :
            dir_(TRANSFORM_DIR_FORWARD)
        {
            Identity(matrix_, offset_);
        }
        
        ~Impl()
        { }
        
        Impl& operator= (const Impl & rhs)
        {
            dir_ = rhs.dir_;
            memcpy(matrix_, rhs.matrix_, 16*sizeof(float));
            memcpy(offset_, rhs.offset_, 4*sizeof(float));
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
        *m_impl = *rhs.m_impl;
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
    
    bool MatrixTransform::equals(const MatrixTransform & other) const
    {
        const float abserror = 1e-9f;
        
        for(int i=0; i<16; ++i)
        {
            if(!equalWithAbsError(getImpl()->matrix_[i],
                other.getImpl()->matrix_[i], abserror))
            {
                return false;
            }
        }
        
        for(int i=0; i<4; ++i)
        {
            if(!equalWithAbsError(getImpl()->offset_[i],
                other.getImpl()->offset_[i], abserror))
            {
                return false;
            }
        }
        
        return true;
    }
    
    void MatrixTransform::getValue(float * m44, float * offset4) const
    {
        if(m44) memcpy(m44, getImpl()->matrix_, 16*sizeof(float));
        if(offset4) memcpy(offset4, getImpl()->offset_, 4*sizeof(float));
    }
    
    void MatrixTransform::setValue(const float * m44, const float * offset4)
    {
        if(m44) memcpy(getImpl()->matrix_, m44, 16*sizeof(float));
        if(offset4) memcpy(getImpl()->offset_, offset4, 4*sizeof(float));
    }
    
    void MatrixTransform::setMatrix(const float * m44)
    {
        if(m44) memcpy(getImpl()->matrix_, m44, 16*sizeof(float));
    }
    
    void MatrixTransform::getMatrix(float * m44) const
    {
        if(m44) memcpy(m44, getImpl()->matrix_, 16*sizeof(float));
    }
    
    void MatrixTransform::setOffset(const float * offset4)
    {
        if(offset4) memcpy(getImpl()->offset_, offset4, 4*sizeof(float));
    }
    
    void MatrixTransform::getOffset(float * offset4) const
    {
        if(offset4) memcpy(offset4, getImpl()->offset_, 4*sizeof(float));
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
        if(m44)
        {
            memset(m44, 0, 16*sizeof(float));
            m44[0] = 1.0f;
            m44[5] = 1.0f;
            m44[10] = 1.0f;
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
        TransformDirection dir_;
        float matrix_[16];
        float offset_[4];
        
    
    
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

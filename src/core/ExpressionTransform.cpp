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

#include "ExpressionOps.h"
#include "OpBuilders.h"

OCIO_NAMESPACE_ENTER
{
    ExpressionTransformRcPtr ExpressionTransform::Create()
    {
        return ExpressionTransformRcPtr(new ExpressionTransform(), &deleter);
    }

    void ExpressionTransform::deleter(ExpressionTransform* t)
    {
        delete t;
    }

    class ExpressionTransform::Impl
    {
    public:
        TransformDirection dir_;
        bool is3d_;
        std::string expressionR_;
        std::string expressionG_;
        std::string expressionB_;

        Impl() :
            dir_(TRANSFORM_DIR_FORWARD),
            is3d_(false)
        { }

        ~Impl()
        { }

        Impl& operator= (const Impl & rhs)
        {
            dir_ = rhs.dir_;
            expressionR_ = rhs.expressionR_;
            expressionG_ = rhs.expressionG_;
            expressionB_ = rhs.expressionB_;
            is3d_        = rhs.is3d_;
            return *this;
        }
    };

    ///////////////////////////////////////////////////////////////////////////

    ExpressionTransform::ExpressionTransform()
        : m_impl(new ExpressionTransform::Impl)
    {
    }

    TransformRcPtr ExpressionTransform::createEditableCopy() const
    {
        ExpressionTransformRcPtr transform = ExpressionTransform::Create();
        *(transform->m_impl) = *m_impl;
        return transform;
    }

    ExpressionTransform::~ExpressionTransform()
    {
        delete m_impl;
        m_impl = NULL;
    }

    ExpressionTransform& ExpressionTransform::operator= (const ExpressionTransform & rhs)
    {
        *m_impl = *rhs.m_impl;
        return *this;
    }

    TransformDirection ExpressionTransform::getDirection() const
    {
        return getImpl()->dir_;
    }

    void ExpressionTransform::setDirection(TransformDirection dir)
    {
        getImpl()->dir_ = dir;
    }

    void ExpressionTransform::setExpressionR(const char * expressionR)
    {
        getImpl()->expressionR_ = expressionR;
    }

    void ExpressionTransform::setExpressionG(const char * expressionG)
    {
        getImpl()->expressionG_ = expressionG;
    }

    void ExpressionTransform::setExpressionB(const char * expressionB)
    {
        getImpl()->expressionB_ = expressionB;
    }

    void ExpressionTransform::setIs3d(const bool is3dExpression)
    {
        getImpl()->is3d_ = is3dExpression;
    }

    bool ExpressionTransform::is3d() const
    {
        return getImpl()->is3d_;
    }

    const char * ExpressionTransform::getExpressionR() const
    {
        return getImpl()->expressionR_.c_str();
    }

    const char * ExpressionTransform::getExpressionG() const
    {
        return getImpl()->expressionG_.c_str();
    }

    const char * ExpressionTransform::getExpressionB() const
    {
        return getImpl()->expressionB_.c_str();
    }


    std::ostream& operator<< (std::ostream& os, const ExpressionTransform& t)
    {
        os << "<ExpressionTransform ";
        os << "direction=" << TransformDirectionToString(t.getDirection()) << ", ";
        os << ">\n";
        return os;
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////////

    void BuildExpressionOps(OpRcPtrVec & ops,
                          const Config& /*config*/,
                          const ExpressionTransform & transform,
                          TransformDirection dir)
    {
        TransformDirection combinedDir = CombineTransformDirections(dir,
            transform.getDirection());

        if (transform.is3d())
        {

            std::string expressionR = transform.getExpressionR();
            std::string expressionG = transform.getExpressionG();
            std::string expressionB = transform.getExpressionB();
            CreateExpressionOp(ops, expressionR, expressionG, expressionB, combinedDir);
        }
        else
        {
            std::string expression = transform.getExpressionR();
            CreateExpressionOp(ops, expression, combinedDir);
        }
    }

}
OCIO_NAMESPACE_EXIT

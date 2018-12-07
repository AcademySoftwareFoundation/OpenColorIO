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


#ifndef INCLUDED_OCIO_LOGOPS_H
#define INCLUDED_OCIO_LOGOPS_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"

#include <vector>

OCIO_NAMESPACE_ENTER
{
    class LogOpData;
    typedef OCIO_SHARED_PTR<LogOpData> LogOpDataRcPtr;
    typedef OCIO_SHARED_PTR<const LogOpData> ConstLogOpDataRcPtr;

    class LogOpData : public OpData
    {
    public:
        LogOpData(float base);
        LogOpData(const float * k,
                  const float * m,
                  const float * b,
                  const float * base,
                  const float * kb);
        virtual ~LogOpData() {}

        LogOpData & operator = (const LogOpData & rhs);

        virtual Type getType() const override { return LogType; }

        virtual bool isNoOp() const override { return false; }
        virtual bool isIdentity() const override { return false; }

        virtual bool hasChannelCrosstalk() const override { return false; }

        float m_k[3];
        float m_m[3];
        float m_b[3];
        float m_base[3];
        float m_kb[3];

        virtual void finalize() override;
    };

    // output = k * log(mx+b, base) + kb
    // This does not affect alpha
    // In the forward direction this is lin->log
    // All input vectors are size 3 (including base)
    
    void CreateLogOp(OpRcPtrVec & ops,
                     const float * k,
                     const float * m,
                     const float * b,
                     const float * base,
                     const float * kb,
                     TransformDirection direction);

    void CreateLogOp(OpRcPtrVec & ops, float base, TransformDirection direction);
    
}
OCIO_NAMESPACE_EXIT

#endif

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
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/CDL/CDLOps.h"
#include "ops/Lut1D/Lut1DOp.h"
#include "ops/Lut3D/Lut3DOp.h"
#include "ops/Range/RangeOps.h"
#include "pystring/pystring.h"


OCIO_NAMESPACE_ENTER
{
    OpData::OpData(BitDepth inBitDepth, BitDepth outBitDepth)
        :   m_inBitDepth(inBitDepth)
        ,   m_outBitDepth(outBitDepth)
    { }

    OpData::OpData(BitDepth inBitDepth, BitDepth outBitDepth,
                   const std::string & id, 
                   const Descriptions & desc)
        :   m_id(id)
        ,   m_descriptions(desc)
        ,   m_inBitDepth(inBitDepth)
        ,   m_outBitDepth(outBitDepth)
    { }

    OpData::OpData(const OpData & rhs)
    {
        *this = rhs;
    }

    OpData& OpData::operator=(const OpData & rhs)
    {
        if (this != &rhs)
        {
            m_id           = rhs.m_id;
            m_descriptions = rhs.m_descriptions;
            m_inBitDepth   = rhs.m_inBitDepth;
            m_outBitDepth  = rhs.m_outBitDepth;
        }

        return *this;
    }

    OpData::~OpData()
    { }

    void OpData::validate() const
    {
        if (getInputBitDepth() == BIT_DEPTH_UNKNOWN)
        {
            throw Exception("OpData missing 'Input Bit Depth' value.");
        }

        if (getOutputBitDepth() == BIT_DEPTH_UNKNOWN)
        {
            throw Exception("OpData missing 'Output Bit Depth' value.");
        }
    }

    bool OpData::operator==(const OpData& other) const
    {
        if (this == &other) return true;

        return (m_id == other.m_id &&
                m_descriptions == other.m_descriptions &&
                m_inBitDepth == other.m_inBitDepth && 
                m_outBitDepth == other.m_outBitDepth);
    }


    Op::Op()
    { }

    Op::~Op()
    { }
    
    bool Op::canCombineWith(ConstOpRcPtr & /*op*/) const
    {
        return false;
    }
    
    void Op::combineWith(OpRcPtrVec & /*ops*/,
                         ConstOpRcPtr & /*secondOp*/) const
    {
        std::ostringstream os;
        os << "Op: " << getInfo() << " cannot be combined. ";
        os << "A type-specific combining function is not defined.";
        throw Exception(os.str().c_str());
    }
    
    std::ostream& operator<< (std::ostream & os, const Op & op)
    {
        os << op.getInfo();
        return os;
    }
    
    namespace
    {
        const int FLOAT_DECIMALS = 7;
    }
    
    std::string AllocationData::getCacheID() const
    {
        std::ostringstream os;
        os.precision(FLOAT_DECIMALS);
        os << AllocationToString(allocation) << " ";
        
        for(unsigned int i=0; i<vars.size(); ++i)
        {
            os << vars[i] << " ";
        }
        
        return os.str();
    }
    
    std::ostream& operator<< (std::ostream & os, const AllocationData & allocation)
    {
        os << allocation.getCacheID();
        return os;
    }
    
    std::string SerializeOpVec(const OpRcPtrVec & ops, int indent)
    {
        std::ostringstream os;
        
        for(OpRcPtrVec::size_type i = 0, size = ops.size(); i < size; ++i)
        {
            os << pystring::mul(" ", indent);
            os << "Op " << i << ": " << *ops[i] << " ";
            os << ops[i]->getCacheID() << " supports_gpu:" << ops[i]->supportedByLegacyShader();
            os << "\n";
        }
        
        return os.str();
    }
    
    bool IsOpVecNoOp(const OpRcPtrVec & ops)
    {
        for(OpRcPtrVec::size_type i = 0, size = ops.size(); i < size; ++i)
        {
            if(!ops[i]->isNoOp()) return false;
        }
        
        return true;
    }
    
    void FinalizeOpVec(OpRcPtrVec & ops, bool optimize)
    {
        // TODO: Add envvar to force disable optimizations
        
        if(optimize)
        {
            OptimizeOpVec(ops);
        }
        
        for(OpRcPtrVec::size_type i = 0, size = ops.size(); i < size; ++i)
        {
            ops[i]->finalize();
        }
    }

    void CreateOpVecFromOpData(OpRcPtrVec & ops,
                               const OpDataRcPtr & opData,
                               TransformDirection dir)
    {
        switch (opData->getType())
        {
        case OpData::CDLType:
        {
            auto cdlSrc = std::dynamic_pointer_cast<CDLOpData>(opData);
            auto cdl = std::make_shared<CDLOpData>(*cdlSrc);
            CreateCDLOp(ops, cdl, dir);
            break;
        }

        case OpData::OpData::Lut1DType:
        {
            auto lutSrc = std::dynamic_pointer_cast<Lut1DOpData>(opData);
            auto lut = std::make_shared<Lut1DOpData>(*lutSrc);
            CreateLut1DOp(ops, lut, dir);
            break;
        }

        case OpData::OpData::Lut3DType:
        {
            auto lutSrc = std::dynamic_pointer_cast<Lut3DOpData>(opData);
            auto lut = std::make_shared<Lut3DOpData>(*lutSrc);
            CreateLut3DOp(ops, lut, dir);
            break;
        }

        case OpData::MatrixType:
        {
            auto matrixSrc = std::dynamic_pointer_cast<MatrixOpData>(opData);
            auto matrix = std::make_shared<MatrixOpData>(*matrixSrc);
            CreateMatrixOp(ops, matrix, dir);
            break;
        }

        case OpData::RangeType:
        {
            auto rangeSrc = std::dynamic_pointer_cast<RangeOpData>(opData);
            auto range = std::make_shared<RangeOpData>(*rangeSrc);
            CreateRangeOp(ops, range, dir);
            break;
        }

        // TODO: Add the other ops when ready
        default:
            throw Exception("OpData is not supported");
        }
    }

    void CreateOpVecFromOpDataVec(OpRcPtrVec & ops,
                                  const OpDataVec & opDataVec,
                                  TransformDirection dir)
    {
        if (dir == TRANSFORM_DIR_FORWARD)
        {
            for (auto & opData : opDataVec)
            {
                CreateOpVecFromOpData(ops, opData, dir);
            }
        }
        else
        {
            for (int idx = (int)opDataVec.size() - 1; idx >= 0; --idx)
            {
                CreateOpVecFromOpData(ops, opDataVec[idx], dir);
            }
        }
    }

}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"
#include "ops/Matrix/MatrixOps.h"
#include "ops/Log/LogOps.h"
#include "ops/NoOp/NoOps.h"

OCIO_NAMESPACE_USING

void Apply(const OpRcPtrVec ops, float * source, long numPixels)
{
    OpRcPtrVec::size_type numOps = ops.size();
    for (OpRcPtrVec::size_type i = 0; i < numOps; ++i)
    {
        ops[i]->apply(source, numPixels);
    }
}

OIIO_ADD_TEST(FinalizeOpVec, optimize_combine)
{
    const float m1[16] = { 1.1f, 0.2f, 0.3f, 0.4f,
        0.5f, 1.6f, 0.7f, 0.8f,
        0.2f, 0.1f, 1.1f, 0.2f,
        0.3f, 0.4f, 0.5f, 1.6f };

    const float v1[4] = { -0.5f, -0.25f, 0.25f, 0.0f };

    const float m2[16] = { 1.1f, -0.1f, -0.1f, 0.0f,
        0.1f, 0.9f, -0.2f, 0.0f,
        0.05f, 0.0f, 1.1f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f };
    const float v2[4] = { -0.2f, -0.1f, -0.1f, -0.2f };

    const float source[] = { 0.1f, 0.2f, 0.3f, 0.4f,
        -0.1f, -0.2f, 50.0f, 123.4f,
        1.0f, 1.0f, 1.0f, 1.0f };
    const float error = 1e-4f;

    const float k[3] = { 0.18f, 0.18f, 0.18f };
    const float m[3] = { 2.0f, 2.0f, 2.0f };
    const float b[3] = { 0.1f, 0.1f, 0.1f };
    const float base[3] = { 10.0f, 10.0f, 10.0f };
    const float kb[3] = { 1.0f, 1.0f, 1.0f };

    // Combining ops
    {
        OpRcPtrVec ops;
        OIIO_CHECK_NO_THROW(CreateMatrixOffsetOp(ops, m1, v1, TRANSFORM_DIR_FORWARD));
        OIIO_CHECK_NO_THROW(CreateMatrixOffsetOp(ops, m2, v2, TRANSFORM_DIR_FORWARD));
        OIIO_CHECK_EQUAL(ops.size(), 2);
     
        // No optimize: keep both matrix ops
        OIIO_CHECK_NO_THROW(FinalizeOpVec(ops, false));
        OIIO_CHECK_EQUAL(ops.size(), 2);

        // apply ops
        float tmp[12];
        memcpy(tmp, source, 12 * sizeof(float));
        Apply(ops, tmp, 3);

        // Optimize: Combine 2 matrix ops
        OIIO_CHECK_NO_THROW(FinalizeOpVec(ops));
        OIIO_CHECK_EQUAL(ops.size(), 1);

        // apply op
        float tmp2[12];
        memcpy(tmp2, source, 12 * sizeof(float));
        Apply(ops, tmp2, 3);

        // compare results
        for (unsigned int i = 0; i<12; ++i)
        {
            OIIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
        }
    }

    // remove NoOp at the beginning
    {
        OpRcPtrVec ops;
        // NoOp
        OIIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OIIO_CHECK_NO_THROW(CreateMatrixOffsetOp(ops, m1, v1, TRANSFORM_DIR_FORWARD));
        OIIO_CHECK_NO_THROW(CreateLogOp(ops, k, m, b, base, kb, TRANSFORM_DIR_FORWARD));

        OIIO_CHECK_EQUAL(ops.size(), 3);

        // No optimize: keep both all ops
        OIIO_CHECK_NO_THROW(FinalizeOpVec(ops, false));
        OIIO_CHECK_EQUAL(ops.size(), 3);

        // apply ops
        float tmp[12];
        memcpy(tmp, source, 12 * sizeof(float));
        Apply(ops, tmp, 3);

        // Optimize: remove the no op
        OIIO_CHECK_NO_THROW(FinalizeOpVec(ops));
        OIIO_CHECK_EQUAL(ops.size(), 2);
        OIIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
        OIIO_CHECK_EQUAL(ops[1]->getInfo(), "<LogOp>");

        // apply ops
        float tmp2[12];
        memcpy(tmp2, source, 12 * sizeof(float));
        Apply(ops, tmp2, 3);

        // compare results
        for (unsigned int i = 0; i<12; ++i)
        {
            OIIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
        }
    }

    // remove NoOp in the middle
    {
        OpRcPtrVec ops;
        OIIO_CHECK_NO_THROW(CreateMatrixOffsetOp(ops, m1, v1, TRANSFORM_DIR_FORWARD));
        // NoOp
        OIIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OIIO_CHECK_NO_THROW(CreateLogOp(ops, k, m, b, base, kb, TRANSFORM_DIR_FORWARD));

        OIIO_CHECK_EQUAL(ops.size(), 3);

        // No optimize: keep both all ops
        OIIO_CHECK_NO_THROW(FinalizeOpVec(ops, false));
        OIIO_CHECK_EQUAL(ops.size(), 3);

        // apply ops
        float tmp[12];
        memcpy(tmp, source, 12 * sizeof(float));
        Apply(ops, tmp, 3);

        // Optimize: remove the no op
        OIIO_CHECK_NO_THROW(FinalizeOpVec(ops));
        OIIO_CHECK_EQUAL(ops.size(), 2);
        OIIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
        OIIO_CHECK_EQUAL(ops[1]->getInfo(), "<LogOp>");

        // apply ops
        float tmp2[12];
        memcpy(tmp2, source, 12 * sizeof(float));
        Apply(ops, tmp2, 3);

        // compare results
        for (unsigned int i = 0; i<12; ++i)
        {
            OIIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
        }
    }

    // remove NoOp in the end
    {
        OpRcPtrVec ops;
        OIIO_CHECK_NO_THROW(CreateMatrixOffsetOp(ops, m1, v1, TRANSFORM_DIR_FORWARD));
        OIIO_CHECK_NO_THROW(CreateLogOp(ops, k, m, b, base, kb, TRANSFORM_DIR_FORWARD));
        // NoOp
        OIIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));

        OIIO_CHECK_EQUAL(ops.size(), 3);

        // No optimize: keep both all ops
        OIIO_CHECK_NO_THROW(FinalizeOpVec(ops, false));
        OIIO_CHECK_EQUAL(ops.size(), 3);

        // apply ops
        float tmp[12];
        memcpy(tmp, source, 12 * sizeof(float));
        Apply(ops, tmp, 3);

        // Optimize: remove the no op
        OIIO_CHECK_NO_THROW(FinalizeOpVec(ops));
        OIIO_CHECK_EQUAL(ops.size(), 2);
        OIIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
        OIIO_CHECK_EQUAL(ops[1]->getInfo(), "<LogOp>");

        // apply ops
        float tmp2[12];
        memcpy(tmp2, source, 12 * sizeof(float));
        Apply(ops, tmp2, 3);

        // compare results
        for (unsigned int i = 0; i<12; ++i)
        {
            OIIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
        }
    }

    // remove several NoOp
    {
        OpRcPtrVec ops;
        OIIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OIIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OIIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OIIO_CHECK_NO_THROW(CreateMatrixOffsetOp(ops, m1, v1, TRANSFORM_DIR_FORWARD));
        OIIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OIIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OIIO_CHECK_NO_THROW(CreateLogOp(ops, k, m, b, base, kb, TRANSFORM_DIR_FORWARD));
        OIIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OIIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));

        OIIO_CHECK_EQUAL(ops.size(), 9);

        // No optimize: keep both all ops
        OIIO_CHECK_NO_THROW(FinalizeOpVec(ops, false));
        OIIO_CHECK_EQUAL(ops.size(), 9);

        // apply ops
        float tmp[12];
        memcpy(tmp, source, 12 * sizeof(float));
        Apply(ops, tmp, 3);

        // Optimize: remove the no op
        OIIO_CHECK_NO_THROW(FinalizeOpVec(ops));
        OIIO_CHECK_EQUAL(ops.size(), 2);
        OIIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
        OIIO_CHECK_EQUAL(ops[1]->getInfo(), "<LogOp>");

        // apply ops
        float tmp2[12];
        memcpy(tmp2, source, 12 * sizeof(float));
        Apply(ops, tmp2, 3);

        // compare results
        for (unsigned int i = 0; i<12; ++i)
        {
            OIIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
        }
    }
}

OIIO_ADD_TEST(Descriptions, basic)
{
    OpData::Descriptions desc1;
    OIIO_CHECK_ASSERT( desc1 == desc1 );

    OpData::Descriptions desc2("My dummy comment");
    OIIO_CHECK_ASSERT( desc1 != desc2 );
    OIIO_CHECK_ASSERT( desc2 != desc1 );
    OIIO_REQUIRE_EQUAL( desc2.size(), 1);
    OIIO_CHECK_EQUAL(desc2[0], std::string("My dummy comment"));

    OIIO_CHECK_NO_THROW( desc1 = desc2 );
    OIIO_CHECK_ASSERT( desc1 == desc2 );
    OIIO_CHECK_ASSERT( desc2 == desc1 );

    OIIO_CHECK_NO_THROW( desc2 += "My second dummy comment" );
    OIIO_REQUIRE_EQUAL( desc2.size(), 2);
    OIIO_CHECK_ASSERT( desc1 != desc2 );
    OIIO_CHECK_ASSERT( desc2 != desc1 );
}

OIIO_ADD_TEST(CreateOpVecFromOpDataVec, basic)
{
    OpDataVec opDataVec;
    auto mat = MatrixOpData::CreateDiagonalMatrix(BIT_DEPTH_F32,
                                                  BIT_DEPTH_F32,
                                                  2.0);
    opDataVec.push_back(mat);

    auto range = std::make_shared<RangeOpData>(BIT_DEPTH_F32,
                                               BIT_DEPTH_F32,
                                               0.0f, 1.0f, 0.5f, 1.5f);

    opDataVec.push_back(range);

    OIIO_REQUIRE_EQUAL(opDataVec.size(), 2);

    {
        OpRcPtrVec ops;
        OIIO_CHECK_NO_THROW(CreateOpVecFromOpDataVec(ops, opDataVec, TRANSFORM_DIR_FORWARD));
        OIIO_REQUIRE_EQUAL(ops.size(), 2);

        OIIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
        OIIO_CHECK_EQUAL(ops[1]->getInfo(), "<RangeOp>");
    }

    {
        OpRcPtrVec ops;
        OIIO_CHECK_NO_THROW(CreateOpVecFromOpDataVec(ops, opDataVec, TRANSFORM_DIR_INVERSE));
        OIIO_REQUIRE_EQUAL(ops.size(), 2);

        OIIO_CHECK_EQUAL(ops[0]->getInfo(), "<RangeOp>");
        OIIO_CHECK_EQUAL(ops[1]->getInfo(), "<MatrixOffsetOp>");
    }
}

#endif

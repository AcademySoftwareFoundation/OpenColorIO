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
#include "ops/Exponent/ExponentOps.h"
#include "ops/exposurecontrast/ExposureContrastOps.h"
#include "ops/FixedFunction/FixedFunctionOps.h"
#include "ops/Gamma/GammaOps.h"
#include "ops/Log/LogOps.h"
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

    bool OpData::operator==(const OpData & other) const
    {
        if (this == &other) return true;

        return (getType() == other.getType()
                && m_id == other.m_id
                && m_descriptions == other.m_descriptions
                && m_inBitDepth == other.m_inBitDepth
                && m_outBitDepth == other.m_outBitDepth);
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

    bool Op::hasDynamicProperty(DynamicPropertyType type) const
    {
        return false;
    }

    DynamicPropertyRcPtr Op::getDynamicProperty(DynamicPropertyType type) const
    {
        throw Exception("Op does not implement dynamic property.");
    }

    void Op::replaceDynamicProperty(DynamicPropertyType type, DynamicPropertyImplRcPtr prop)
    {
        throw Exception("Op does not implement dynamic property.");
    }


    OpRcPtrVec::OpRcPtrVec(const OpRcPtrVec & v)
    {
        *this = v; 
    }

    OpRcPtrVec & OpRcPtrVec::operator=(const OpRcPtrVec & v)
    {
        if(this!=&v)
        {
            m_ops = v.m_ops;
        }

        return *this;
    }

    OpRcPtrVec & OpRcPtrVec::operator+=(const OpRcPtrVec & v)
    {
        m_ops.insert(end(), v.begin(), v.end());
        adjustBitDepths();
        return *this;
    }

// gcc 4.8 partially supports C++11.
// Note: gcc 4.8 C++11 does not support C++11 changes for 
//       std::vector::erase() & std::vector::insert() methods.
#if __GNUC__ <= 4
    // Convert a const iterator to a non const iterator.
    #define ITER(pos) (m_ops.begin()+(pos-m_ops.begin())) 
#else
    #define ITER(pos) pos 
#endif

    OpRcPtrVec::iterator OpRcPtrVec::erase(OpRcPtrVec::const_iterator position) 
    { 
        OpRcPtrVec::iterator iter = m_ops.erase(ITER(position)); 
        adjustBitDepths();
        return iter;
    }
    
    OpRcPtrVec::iterator OpRcPtrVec::erase(OpRcPtrVec::const_iterator first, 
                                           OpRcPtrVec::const_iterator last)
    { 
        OpRcPtrVec::iterator iter = m_ops.erase(ITER(first), ITER(last)); 
        adjustBitDepths();
        return iter;
    }

    void OpRcPtrVec::insert(OpRcPtrVec::const_iterator position, 
                            OpRcPtrVec::const_iterator first, 
                            OpRcPtrVec::const_iterator last)
    {
        m_ops.insert(ITER(position), first, last);
        adjustBitDepths();
    }

    void OpRcPtrVec::push_back(const OpRcPtrVec::value_type & val) 
    {
        m_ops.push_back(val);
        adjustBitDepths();
    }

    OpRcPtrVec::const_reference OpRcPtrVec::back() const
    {
        return m_ops.back();
    }

    void OpRcPtrVec::validate() const
    {
        const size_type numOps = m_ops.size();
        BitDepth prevOutBD = BIT_DEPTH_UNKNOWN;

        for(size_type idx=0; idx<numOps; ++idx)
        {
            if(!m_ops[idx]->isNoOpType())
            {
                if(m_ops[idx]->getInputBitDepth() == BIT_DEPTH_UNKNOWN)
                {
                    throw Exception("Unsupported 'unknown' bit-depth.");
                }
                if(prevOutBD != BIT_DEPTH_UNKNOWN
                    && m_ops[idx]->getInputBitDepth() != prevOutBD)
                {
                    throw Exception("Bit-depth mismatch between adjacent ops.");
                }
                prevOutBD = m_ops[idx]->getOutputBitDepth();
            }
        }
    }

    // As ops are added or removed from the OpVec, we need to adjust the inputBitDepth
    // of an op to match the outputBitDepth of the one before it. This allows the op 
    // to make any necessary adjustments to the scaling of its parameter values.
    void OpRcPtrVec::adjustBitDepths()
    {
        const size_type numOps = m_ops.size();
        BitDepth prevOutBD = BIT_DEPTH_UNKNOWN;

        for(size_type idx=0; idx<numOps; ++idx)
        {
            if(!m_ops[idx]->isNoOpType())
            {
                if(prevOutBD != BIT_DEPTH_UNKNOWN 
                    && m_ops[idx]->getInputBitDepth() != prevOutBD)
                {
                    m_ops[idx]->setInputBitDepth(prevOutBD);
                }
                prevOutBD = m_ops[idx]->getOutputBitDepth();
            }
        }
    }

    OpRcPtrVec OpRcPtrVec::clone() const
    {
        OpRcPtrVec v;

        for(const auto & op : m_ops)
        {
            v.push_back(op->clone());
        }

        return v;
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
        for(const auto & op : ops)
        {
            if(!op->isNoOp()) return false;
        }
        
        return true;
    }

    void FinalizeOpVec(OpRcPtrVec & ops, bool optimize)
    {
        // TODO: Add envvar to force disable optimizations

        ops.validate();
        
        if(optimize)
        {
            OptimizeOpVec(ops);
        }
        
        for(auto & op : ops)
        {
            op->finalize();
        }
    }
    
    void CreateOpVecFromOpData(OpRcPtrVec & ops,
                               const OpDataRcPtr & opData,
                               TransformDirection dir)
    {
        if (dir == TRANSFORM_DIR_UNKNOWN)
        {
            throw Exception("Cannot create Op with unspecified transform direction.");
        }

        static_assert(OpData::NoOpType == 11, "Need to handle new type here");
        
        switch (opData->getType())
        {
        case OpData::CDLType:
        {
            auto cdlSrc = std::dynamic_pointer_cast<CDLOpData>(opData);
            auto cdl = std::make_shared<CDLOpData>(*cdlSrc);
            CreateCDLOp(ops, cdl, dir);
            break;
        }

        case OpData::ExponentType:
        {
            auto expSrc = std::dynamic_pointer_cast<ExponentOpData>(opData);
            auto exp = std::make_shared<ExponentOpData>(*expSrc);
            CreateExponentOp(ops, exp, dir);
            break;
        }

        case OpData::ExposureContrastType:
        {
            auto ecSrc = std::dynamic_pointer_cast<ExposureContrastOpData>(opData);
            auto ec = ecSrc->clone();
            CreateExposureContrastOp(ops, ec, dir);
            break;
        }
        
        case OpData::FixedFunctionType:
        {
            auto ffSrc = std::dynamic_pointer_cast<FixedFunctionOpData>(opData);
            auto ff = std::make_shared<FixedFunctionOpData>(*ffSrc);
            CreateFixedFunctionOp(ops, ff, dir);
            break;
        }

        case OpData::GammaType:
        {
            auto gammaSrc = std::dynamic_pointer_cast<GammaOpData>(opData);
            auto gamma = std::make_shared<GammaOpData>(*gammaSrc);
            CreateGammaOp(ops, gamma, dir);
            break;
        }

        case OpData::LogType:
        {
            auto logSrc = std::dynamic_pointer_cast<LogOpData>(opData);
            auto log = std::make_shared<LogOpData>(*logSrc);
            CreateLogOp(ops, log, dir);
            break;
        }

        case OpData::Lut1DType:
        {
            auto lutSrc = std::dynamic_pointer_cast<Lut1DOpData>(opData);
            auto lut = std::make_shared<Lut1DOpData>(*lutSrc);
            CreateLut1DOp(ops, lut, dir);
            break;
        }

        case OpData::Lut3DType:
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
        
        case OpData::ReferenceType:
        {
            throw Exception("ReferenceOpData should have been replaced by referenced ops");
            break;
        }

        default:
            throw Exception("OpData is not supported");
        }
    }

    void CreateOpVecFromOpDataVec(OpRcPtrVec & ops,
                                  const OpDataVec & opDataVec,
                                  TransformDirection dir)
    {
        if(dir == TRANSFORM_DIR_FORWARD)
        {
            for(const auto & opData : opDataVec)
            {
                CreateOpVecFromOpData(ops, opData, dir);
            }
        }
        else
        {
            for(int idx = (int)opDataVec.size() - 1; idx >= 0; --idx)
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

void Apply(const OpRcPtrVec & ops, float * source, long numPixels)
{
    for(const auto & op : ops)
    {
        op->apply(source, numPixels);
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
                           0.1f,  0.9f, -0.2f, 0.0f,
                           0.05f, 0.0f,  1.1f, 0.0f,
                           0.0f,  0.0f,  0.0f, 1.0f };
    const float v2[4] = { -0.2f, -0.1f, -0.1f, -0.2f };

    const float source[] = { 0.1f,  0.2f,  0.3f,   0.4f,
                            -0.1f, -0.2f, 50.0f, 123.4f,
                             1.0f,  1.0f,  1.0f,   1.0f };
    const float error = 1e-4f;

    const double base = 10.0;
    const double logSlope[3] = { 0.18, 0.18, 0.18 };
    const double linSlope[3] = { 2.0, 2.0, 2.0 };
    const double linOffset[3] = { 0.1, 0.1, 0.1 };
    const double logOffset[3] = { 1.0, 1.0, 1.0 };

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
        OIIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset,
                                        linSlope, linOffset,
                                        OCIO::TRANSFORM_DIR_FORWARD));

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
        OIIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset,
                                        linSlope, linOffset,
                                        OCIO::TRANSFORM_DIR_FORWARD));

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
        OIIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset,
                                        linSlope, linOffset,
                                        OCIO::TRANSFORM_DIR_FORWARD));
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
        OIIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset,
                                        linSlope, linOffset,
                                        OCIO::TRANSFORM_DIR_FORWARD));
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

OIIO_ADD_TEST(Op, non_dynamic_ops)
{
    float scale[4] = { 2.0f, 2.0f, 2.0f, 1.0f };

    OCIO::OpRcPtrVec ops;
    OCIO::CreateScaleOp(ops, scale, OCIO::TRANSFORM_DIR_FORWARD);

    OIIO_REQUIRE_EQUAL(ops.size(), 1);
    OIIO_REQUIRE_ASSERT(ops[0]);
    
    // Test that non-dynamic ops such as matrix respond properly to dynamic
    // property requests.
    OIIO_CHECK_ASSERT(!ops[0]->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE));
    OIIO_CHECK_ASSERT(!ops[0]->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST));
    OIIO_CHECK_ASSERT(!ops[0]->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA));

    OIIO_CHECK_THROW_WHAT(ops[0]->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA),
                          OCIO::Exception, "does not implement dynamic property");
}

OIIO_ADD_TEST(OpRcPtrVec, bit_depth)
{
    OCIO::OpRcPtrVec ops;
    auto mat = OCIO::MatrixOpData::CreateDiagonalMatrix(OCIO::BIT_DEPTH_F32,
                                                        OCIO::BIT_DEPTH_UINT8,
                                                        1024.);
    OCIO::CreateMatrixOp(ops, mat, OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_REQUIRE_EQUAL(ops.size(), 1);
    OIIO_CHECK_EQUAL(ops[0]->getInputBitDepth(),  OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(ops[0]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);

    auto range = std::make_shared<OCIO::RangeOpData>(OCIO::BIT_DEPTH_F32,
                                                     OCIO::BIT_DEPTH_F32,
                                                     0.0f, 1.0f, 0.5f, 1.5f);

    OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD);

    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    OIIO_CHECK_EQUAL(ops[0]->getInputBitDepth(),  OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(ops[0]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops[1]->getInputBitDepth(),  OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops[1]->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    // Test push_back.

    mat = OCIO::MatrixOpData::CreateDiagonalMatrix(OCIO::BIT_DEPTH_UINT8,
                                                   OCIO::BIT_DEPTH_UINT8,
                                                   1.1);
    OCIO::CreateMatrixOp(ops, mat, OCIO::TRANSFORM_DIR_FORWARD);

    OIIO_REQUIRE_EQUAL(ops.size(), 3);
    OIIO_CHECK_EQUAL(ops[0]->getInputBitDepth(),  OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(ops[0]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops[1]->getInputBitDepth(),  OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops[1]->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(ops[2]->getInputBitDepth(),  OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(ops[2]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);

    // Test erase.

    ops.erase(ops.begin()+1);

    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    OIIO_CHECK_EQUAL(ops[0]->getInputBitDepth(),  OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(ops[0]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops[1]->getInputBitDepth(),  OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops[1]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);

    // Test erase.

    mat = OCIO::MatrixOpData::CreateDiagonalMatrix(OCIO::BIT_DEPTH_UINT8,
                                                   OCIO::BIT_DEPTH_UINT12,
                                                   1.2);
    OCIO::CreateMatrixOp(ops, mat, OCIO::TRANSFORM_DIR_FORWARD);

    mat = OCIO::MatrixOpData::CreateDiagonalMatrix(OCIO::BIT_DEPTH_UINT8,
                                                   OCIO::BIT_DEPTH_UINT16,
                                                   0.9);
    OCIO::CreateMatrixOp(ops, mat, OCIO::TRANSFORM_DIR_FORWARD);

    mat = OCIO::MatrixOpData::CreateDiagonalMatrix(OCIO::BIT_DEPTH_F32,
                                                   OCIO::BIT_DEPTH_UINT12,
                                                   0.9);
    OCIO::CreateMatrixOp(ops, mat, OCIO::TRANSFORM_DIR_FORWARD);

    OIIO_CHECK_EQUAL(ops[0]->getInputBitDepth(),  OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(ops[0]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops[1]->getInputBitDepth(),  OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops[1]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops[2]->getInputBitDepth(),  OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops[2]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);
    OIIO_CHECK_EQUAL(ops[3]->getInputBitDepth(),  OCIO::BIT_DEPTH_UINT12);
    OIIO_CHECK_EQUAL(ops[3]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT16);   
    OIIO_CHECK_EQUAL(ops[4]->getInputBitDepth(),  OCIO::BIT_DEPTH_UINT16);
    OIIO_CHECK_EQUAL(ops[4]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);

    OIIO_REQUIRE_EQUAL(ops.size(), 5);
    ops.erase(ops.begin()+1, ops.begin()+4);

    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    OIIO_CHECK_EQUAL(ops[0]->getInputBitDepth(),  OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(ops[0]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops[1]->getInputBitDepth(),  OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops[1]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);

    // Test insert.

    OCIO::OpRcPtrVec ops1 = ops.clone();

    OIIO_REQUIRE_EQUAL(ops1.size(), 2);
    OIIO_CHECK_EQUAL(ops1[0]->getInputBitDepth(),  OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(ops1[0]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops1[1]->getInputBitDepth(),  OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops1[1]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);

    ops1.insert(ops1.begin()+1, ops.begin(), ops.begin()+1);
    OIIO_REQUIRE_EQUAL(ops1.size(), 3);
    OIIO_CHECK_EQUAL(ops1[0]->getInputBitDepth(),  OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(ops1[0]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops1[1]->getInputBitDepth(),  OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops1[1]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops1[2]->getInputBitDepth(),  OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops1[2]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);

    // Test operator +=.

    OCIO::OpRcPtrVec ops2 = ops.clone();

    OIIO_REQUIRE_EQUAL(ops2.size(), 2);
    ops2[0]->setInputBitDepth(OCIO::BIT_DEPTH_F32);

    OIIO_CHECK_EQUAL(ops2[0]->getInputBitDepth(),  OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(ops2[0]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops2[1]->getInputBitDepth(),  OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops2[1]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);

    ops2 += ops2.clone();

    OIIO_REQUIRE_EQUAL(ops2.size(), 4);
    OIIO_CHECK_EQUAL(ops2[0]->getInputBitDepth(),  OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(ops2[0]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops2[1]->getInputBitDepth(),  OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops2[1]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);
    OIIO_CHECK_EQUAL(ops2[2]->getInputBitDepth(),  OCIO::BIT_DEPTH_UINT12);
    OIIO_CHECK_EQUAL(ops2[2]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops2[3]->getInputBitDepth(),  OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops2[3]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);
}

OIIO_ADD_TEST(OpRcPtrVec, bit_depth_with_filenoop)
{
    OCIO::OpRcPtrVec ops;
    auto mat = OCIO::MatrixOpData::CreateDiagonalMatrix(OCIO::BIT_DEPTH_F32,
                                                        OCIO::BIT_DEPTH_UINT8,
                                                        255.);
    OCIO::CreateMatrixOp(ops, mat, OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_REQUIRE_EQUAL(ops.size(), 1);
    OIIO_CHECK_EQUAL(ops[0]->getInputBitDepth(),  OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(ops[0]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);

    OIIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);

    auto range = std::make_shared<OCIO::RangeOpData>(OCIO::BIT_DEPTH_F32,
                                                     OCIO::BIT_DEPTH_F32,
                                                     0.0f, 1.0f, 0.5f, 1.5f);
    OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD);

    OIIO_REQUIRE_EQUAL(ops.size(), 3);
    OIIO_CHECK_EQUAL(ops[0]->getInputBitDepth(),  OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(ops[0]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    // FileNoOp should not change bit-depths.
    OIIO_CHECK_EQUAL(ops[2]->getInputBitDepth(),  OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops[2]->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    OIIO_CHECK_NO_THROW(ops.validate());

    OIIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
    OIIO_REQUIRE_EQUAL(ops.size(), 4);

    OIIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
    OIIO_REQUIRE_EQUAL(ops.size(), 5);

    auto mat2 = OCIO::MatrixOpData::CreateDiagonalMatrix(OCIO::BIT_DEPTH_F16,
                                                        OCIO::BIT_DEPTH_UINT12,
                                                        4095.);
    OCIO::CreateMatrixOp(ops, mat2, OCIO::TRANSFORM_DIR_FORWARD);

    OIIO_REQUIRE_EQUAL(ops.size(), 6);
    OIIO_CHECK_EQUAL(ops[0]->getInputBitDepth(),  OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(ops[0]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    // FileNoOp should not change bit-depths.
    OIIO_CHECK_EQUAL(ops[2]->getInputBitDepth(),  OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops[2]->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);
    // FileNoOp should not change bit-depths.
    // FileNoOp should not change bit-depths.
    OIIO_CHECK_EQUAL(ops[5]->getInputBitDepth(),  OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(ops[5]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);

    OIIO_CHECK_NO_THROW(ops.validate());

    // Remove FileNoOp and adjust the bit-depths.

    OIIO_CHECK_NO_THROW(OptimizeOpVec(ops));

    OIIO_REQUIRE_EQUAL(ops.size(), 3);
    OIIO_CHECK_EQUAL(ops[0]->getInputBitDepth(),  OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(ops[0]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops[1]->getInputBitDepth(),  OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(ops[1]->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(ops[2]->getInputBitDepth(),  OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(ops[2]->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);

    OIIO_CHECK_NO_THROW(ops.validate());

}

OIIO_ADD_TEST(OpData, equality)
{
    auto mat1 = OCIO::MatrixOpData::CreateDiagonalMatrix(OCIO::BIT_DEPTH_F32,
                                                         OCIO::BIT_DEPTH_F32,
                                                         1.1);
    auto mat2 = mat1->clone();

    // Use the MatrixOpData::operator==().
    OIIO_CHECK_ASSERT(*mat2==*mat1);

    auto range = std::make_shared<RangeOpData>(BIT_DEPTH_F32,
                                               BIT_DEPTH_F32,
                                               0.0f, 1.0f, 0.5f, 1.5f);

    // Use the MatrixOpData::operator==().
    OIIO_CHECK_ASSERT( !(*mat2==*range) );

    // Use the RangeOpData::operator==().
    OIIO_CHECK_ASSERT( !(*range==*mat1) );

    // Use the OpData::operator==().
    auto op1 = DynamicPtrCast<OpData>(range);
    OIIO_CHECK_ASSERT( !(*op1==*mat1) );    

    // Use the OpData::operator==().
    auto op2 = DynamicPtrCast<OpData>(mat2);
    OIIO_CHECK_ASSERT( !(*op2==*op1) );

    // Change something.

    // Use the MatrixOpData::operator==().
    OIIO_CHECK_ASSERT( *mat2==*mat1 );

    // Use the OpData::operator==().
    OIIO_CHECK_ASSERT( *op2==*mat1 );

    mat2->setOffsetValue(1, mat2->getOffsetValue(1) + 1.0);

    // Use the MatrixOpData::operator==().
    OIIO_CHECK_ASSERT( !(*mat2==*mat1) );

    // Use the OpData::operator==().
    OIIO_CHECK_ASSERT( !(*op2==*mat1) );
}

#endif

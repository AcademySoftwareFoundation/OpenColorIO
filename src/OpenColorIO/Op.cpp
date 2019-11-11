// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

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
    bool OpCPU::hasDynamicProperty(DynamicPropertyType type) const
    {
        return false;
    }

    DynamicPropertyRcPtr OpCPU::getDynamicProperty(DynamicPropertyType type) const
    {
        throw Exception("Op does not implement dynamic property.");
    }


    OpData::OpData()
        :   m_metadata()
    { }

    OpData::OpData(const OpData & rhs)
        : m_metadata()
    {
        *this = rhs;
    }

    OpData & OpData::operator=(const OpData & rhs)
    {
        if (this != &rhs)
        {
            m_metadata = rhs.m_metadata;
        }

        return *this;
    }

    OpData::~OpData()
    { }

    void OpData::validate() const
    {
    }

    OpDataRcPtr OpData::getIdentityReplacement() const
    {
        return std::make_shared<MatrixOpData>();
    }

    bool OpData::operator==(const OpData & other) const
    {
        if (this == &other) return true;

        // Ignore metadata.
        return getType() == other.getType();
    }

    const std::string & OpData::getID() const
    {
        return m_metadata.getAttributeValue(METADATA_ID);
    }

    void OpData::setID(const std::string & id)
    {
        return m_metadata.addAttribute(METADATA_ID, id.c_str());
    }

    const std::string & OpData::getName() const
    {
        return m_metadata.getAttributeValue(METADATA_NAME);
    }

    void OpData::setName(const std::string & name)
    {
        return m_metadata.addAttribute(METADATA_NAME, name.c_str());
    }

    const char * GetTypeName(OpData::Type type)
    {
        static_assert(OpData::NoOpType == 11, "Need to handle new type here");
        switch (type)
        {
        case OpData::CDLType:
            return "CDL";
        case OpData::ExponentType:
            return "Exponent";
        case OpData::ExposureContrastType:
            return "ExposureContrast";
        case OpData::FixedFunctionType:
            return "FixedFunction";
        case OpData::GammaType:
            return "Gamma";
        case OpData::LogType:
            return "Log";
        case OpData::Lut1DType:
            return "LUT1D";
        case OpData::Lut3DType:
            return "LUT3D";
        case OpData::MatrixType:
            return "Matrix";
        case OpData::RangeType:
            return "Range";
        case OpData::ReferenceType:
        case OpData::NoOpType:
            break;
        }
        throw Exception("Unexpected op type.");
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

    bool Op::isDynamic() const
    {
        return false;
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

    OpRcPtr Op::getIdentityReplacement() const
    {
        auto opData = m_data->getIdentityReplacement();
        OpRcPtrVec ops;
        if (opData->getType() == OpData::MatrixType)
        {
            // No-op that will be optimized.
            auto mat = std::dynamic_pointer_cast<MatrixOpData>(opData);
            CreateMatrixOp(ops, mat, TRANSFORM_DIR_FORWARD);
        }
        else if (opData->getType() == OpData::RangeType)
        {
            // Clamping op.
            auto range = std::dynamic_pointer_cast<RangeOpData>(opData);
            CreateRangeOp(ops, range, TRANSFORM_DIR_FORWARD);
        }
        else
        {
            std::ostringstream oss;
            oss << "Unexpected type in getIdentityReplacement. Expecting Matrix or Range, got :"
                << std::string(GetTypeName(opData->getType())) << ".";
            throw Exception(oss.str().c_str());
        }
        return ops[0];
    }

    OpRcPtrVec::OpRcPtrVec()
        : m_metadata()
    {
    }

    OpRcPtrVec::OpRcPtrVec(const OpRcPtrVec & v)
        : OpRcPtrVec()
    {
        *this = v; 
    }

    OpRcPtrVec & OpRcPtrVec::operator=(const OpRcPtrVec & v)
    {
        if(this!=&v)
        {
            m_ops = v.m_ops;
            m_metadata = v.m_metadata;
        }

        return *this;
    }

    OpRcPtrVec & OpRcPtrVec::operator+=(const OpRcPtrVec & v)
    {
        if (this != &v)
        {
            m_ops.insert(end(), v.begin(), v.end());
            m_metadata.combine(v.m_metadata);
            return *this;
        }
        else
        {
            OpRcPtrVec other = v;
            return operator+=(other);
        }
    }

    OpRcPtrVec::iterator OpRcPtrVec::erase(OpRcPtrVec::const_iterator position) 
    { 
        return m_ops.erase(position); 
    }
    
    OpRcPtrVec::iterator OpRcPtrVec::erase(OpRcPtrVec::const_iterator first, 
                                           OpRcPtrVec::const_iterator last)
    { 
        return m_ops.erase(first, last); 
    }

    void OpRcPtrVec::insert(OpRcPtrVec::const_iterator position, 
                            OpRcPtrVec::const_iterator first, 
                            OpRcPtrVec::const_iterator last)
    {
        m_ops.insert(position, first, last);
    }

    void OpRcPtrVec::push_back(const OpRcPtrVec::value_type & val) 
    {
        m_ops.push_back(val);
    }

    OpRcPtrVec::const_reference OpRcPtrVec::back() const
    {
        return m_ops.back();
    }

    OpRcPtrVec::const_reference OpRcPtrVec::front() const
    {
        return m_ops.front();
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
    
    void FinalizeOpVec(OpRcPtrVec & ops, OptimizationFlags oFlags)
    {
        for(auto & op : ops)
        {
            op->finalize(oFlags);
        }
    }

    namespace
    {

    void UnifyDynamicProperty(OpRcPtr op,
                              DynamicPropertyImplRcPtr & prop,
                              DynamicPropertyType type)
    {
        if (op->hasDynamicProperty(type))
        {
            if (!prop)
            {
                // Initialize property.
                DynamicPropertyRcPtr dp = op->getDynamicProperty(type);
                prop = OCIO_DYNAMIC_POINTER_CAST<DynamicPropertyImpl>(dp);
            }
            else
            {
                // Share the property.
                op->replaceDynamicProperty(type, prop);
            }
        }
    }

    }

    // This ensures that when a dynamic property on a processor is
    // modified, all ops that respond to that property (and which
    // are enabled) are synchronized.
    void UnifyDynamicProperties(OpRcPtrVec & ops)
    {
        DynamicPropertyImplRcPtr dpExposure;
        DynamicPropertyImplRcPtr dpContrast;
        DynamicPropertyImplRcPtr dpGamma;
        for (auto op : ops)
        {
            UnifyDynamicProperty(op, dpExposure, DYNAMIC_PROPERTY_EXPOSURE);
            UnifyDynamicProperty(op, dpContrast, DYNAMIC_PROPERTY_CONTRAST);
            UnifyDynamicProperty(op, dpGamma, DYNAMIC_PROPERTY_GAMMA);
        }
    }

    void CreateOpVecFromOpData(OpRcPtrVec & ops,
                               const ConstOpDataRcPtr & opData,
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
            auto cdlSrc = std::dynamic_pointer_cast<const CDLOpData>(opData);
            auto cdl = std::make_shared<CDLOpData>(*cdlSrc);
            CreateCDLOp(ops, cdl, dir);
            break;
        }

        case OpData::ExponentType:
        {
            auto expSrc = std::dynamic_pointer_cast<const ExponentOpData>(opData);
            auto exp = std::make_shared<ExponentOpData>(*expSrc);
            CreateExponentOp(ops, exp, dir);
            break;
        }

        case OpData::ExposureContrastType:
        {
            auto ecSrc = std::dynamic_pointer_cast<const ExposureContrastOpData>(opData);
            auto ec = ecSrc->clone();
            CreateExposureContrastOp(ops, ec, dir);
            break;
        }
        
        case OpData::FixedFunctionType:
        {
            auto ffSrc = std::dynamic_pointer_cast<const FixedFunctionOpData>(opData);
            auto ff = std::make_shared<FixedFunctionOpData>(*ffSrc);
            CreateFixedFunctionOp(ops, ff, dir);
            break;
        }

        case OpData::GammaType:
        {
            auto gammaSrc = std::dynamic_pointer_cast<const GammaOpData>(opData);
            auto gamma = std::make_shared<GammaOpData>(*gammaSrc);
            CreateGammaOp(ops, gamma, dir);
            break;
        }

        case OpData::LogType:
        {
            auto logSrc = std::dynamic_pointer_cast<const LogOpData>(opData);
            auto log = std::make_shared<LogOpData>(*logSrc);
            CreateLogOp(ops, log, dir);
            break;
        }

        case OpData::Lut1DType:
        {
            auto lutSrc = std::dynamic_pointer_cast<const Lut1DOpData>(opData);
            auto lut = std::make_shared<Lut1DOpData>(*lutSrc);
            CreateLut1DOp(ops, lut, dir);
            break;
        }

        case OpData::Lut3DType:
        {
            auto lutSrc = std::dynamic_pointer_cast<const Lut3DOpData>(opData);
            auto lut = std::make_shared<Lut3DOpData>(*lutSrc);
            CreateLut3DOp(ops, lut, dir);
            break;
        }

        case OpData::MatrixType:
        {
            auto matrixSrc = std::dynamic_pointer_cast<const MatrixOpData>(opData);
            auto matrix = std::make_shared<MatrixOpData>(*matrixSrc);
            CreateMatrixOp(ops, matrix, dir);
            break;
        }

        case OpData::RangeType:
        {
            auto rangeSrc = std::dynamic_pointer_cast<const RangeOpData>(opData);
            auto range = std::make_shared<RangeOpData>(*rangeSrc);
            CreateRangeOp(ops, range, dir);
            break;
        }
        
        case OpData::ReferenceType:
        {
            throw Exception("ReferenceOpData should have been replaced by referenced ops");
        }

        default:
        {
            throw Exception("OpData is not supported");
        }

        }
    }

    void CreateOpVecFromOpDataVec(OpRcPtrVec & ops,
                                  const ConstOpDataVec & opDataVec,
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

#include "ops/NoOp/NoOps.h"
#include "UnitTest.h"
#include "UnitTestUtils.h"

void Apply(const OCIO::OpRcPtrVec & ops, float * source, long numPixels)
{
    for(const auto & op : ops)
    {
        op->apply(source, numPixels);
    }
}

OCIO_ADD_TEST(FinalizeOpVec, optimize_combine)
{
    const double m1[16] = { 1.1, 0.2, 0.3, 0.4,
                            0.5, 1.6, 0.7, 0.8,
                            0.2, 0.1, 1.1, 0.2,
                            0.3, 0.4, 0.5, 1.6 };

    const double v1[4] = { -0.5, -0.25, 0.25, 0.0 };

    const double m2[16] = { 1.1, -0.1, -0.1, 0.0,
                            0.1,  0.9, -0.2, 0.0,
                            0.05, 0.0,  1.1, 0.0,
                            0.0,  0.0,  0.0, 1.0 };
    const double v2[4] = { -0.2, -0.1, -0.1, -0.2 };

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
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(CreateMatrixOffsetOp(ops, m1, v1, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(CreateMatrixOffsetOp(ops, m2, v2, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_EQUAL(ops.size(), 2);
     
        // No optimize: keep both matrix ops
        OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, OCIO::OPTIMIZATION_NONE));
        OCIO_CHECK_EQUAL(ops.size(), 2);

        // apply ops
        float tmp[12];
        memcpy(tmp, source, 12 * sizeof(float));
        Apply(ops, tmp, 3);

        // Optimize: Combine 2 matrix ops
        OCIO_CHECK_NO_THROW(OCIO::OptimizeFinalizeOpVec(ops));
        OCIO_CHECK_EQUAL(ops.size(), 1);

        // apply op
        float tmp2[12];
        memcpy(tmp2, source, 12 * sizeof(float));
        Apply(ops, tmp2, 3);

        // compare results
        for (unsigned int i = 0; i<12; ++i)
        {
            OCIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
        }
    }

    // remove NoOp at the beginning
    {
        OCIO::OpRcPtrVec ops;
        // NoOp
        OCIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OCIO_CHECK_NO_THROW(CreateMatrixOffsetOp(ops, m1, v1, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset,
                                        linSlope, linOffset,
                                        OCIO::TRANSFORM_DIR_FORWARD));

        OCIO_CHECK_EQUAL(ops.size(), 3);

        // No optimize: keep both all ops
        OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, OCIO::OPTIMIZATION_NONE));
        OCIO_CHECK_EQUAL(ops.size(), 3);

        // apply ops
        float tmp[12];
        memcpy(tmp, source, 12 * sizeof(float));
        Apply(ops, tmp, 3);

        // Optimize: remove the no op
        OCIO_CHECK_NO_THROW(OCIO::OptimizeFinalizeOpVec(ops));
        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
        OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<LogOp>");

        // apply ops
        float tmp2[12];
        memcpy(tmp2, source, 12 * sizeof(float));
        Apply(ops, tmp2, 3);

        // compare results
        for (unsigned int i = 0; i<12; ++i)
        {
            OCIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
        }
    }

    // remove NoOp in the middle
    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(CreateMatrixOffsetOp(ops, m1, v1, OCIO::TRANSFORM_DIR_FORWARD));
        // NoOp
        OCIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OCIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset,
                                        linSlope, linOffset,
                                        OCIO::TRANSFORM_DIR_FORWARD));

        OCIO_CHECK_EQUAL(ops.size(), 3);

        // No optimize: keep both all ops
        OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, OCIO::OPTIMIZATION_NONE));
        OCIO_CHECK_EQUAL(ops.size(), 3);

        // apply ops
        float tmp[12];
        memcpy(tmp, source, 12 * sizeof(float));
        Apply(ops, tmp, 3);

        // Optimize: remove the no op
        OCIO_CHECK_NO_THROW(OCIO::OptimizeFinalizeOpVec(ops));
        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
        OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<LogOp>");

        // apply ops
        float tmp2[12];
        memcpy(tmp2, source, 12 * sizeof(float));
        Apply(ops, tmp2, 3);

        // compare results
        for (unsigned int i = 0; i<12; ++i)
        {
            OCIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
        }
    }

    // remove NoOp in the end
    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(CreateMatrixOffsetOp(ops, m1, v1, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset,
                                        linSlope, linOffset,
                                        OCIO::TRANSFORM_DIR_FORWARD));
        // NoOp
        OCIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));

        OCIO_CHECK_EQUAL(ops.size(), 3);

        // No optimize: keep both all ops
        OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, OCIO::OPTIMIZATION_NONE));
        OCIO_CHECK_EQUAL(ops.size(), 3);

        // apply ops
        float tmp[12];
        memcpy(tmp, source, 12 * sizeof(float));
        Apply(ops, tmp, 3);

        // Optimize: remove the no op
        OCIO_CHECK_NO_THROW(OCIO::OptimizeFinalizeOpVec(ops));
        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
        OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<LogOp>");

        // apply ops
        float tmp2[12];
        memcpy(tmp2, source, 12 * sizeof(float));
        Apply(ops, tmp2, 3);

        // compare results
        for (unsigned int i = 0; i<12; ++i)
        {
            OCIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
        }
    }

    // remove several NoOp
    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OCIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OCIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OCIO_CHECK_NO_THROW(CreateMatrixOffsetOp(ops, m1, v1, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OCIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OCIO_CHECK_NO_THROW(CreateLogOp(ops, base, logSlope, logOffset,
                                        linSlope, linOffset,
                                        OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
        OCIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));

        OCIO_CHECK_EQUAL(ops.size(), 9);

        // No optimize: keep both all ops
        OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, OCIO::OPTIMIZATION_NONE));
        OCIO_CHECK_EQUAL(ops.size(), 9);

        // apply ops
        float tmp[12];
        memcpy(tmp, source, 12 * sizeof(float));
        Apply(ops, tmp, 3);

        // Optimize: remove the no op
        OCIO_CHECK_NO_THROW(OCIO::OptimizeFinalizeOpVec(ops));
        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
        OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<LogOp>");

        // apply ops
        float tmp2[12];
        memcpy(tmp2, source, 12 * sizeof(float));
        Apply(ops, tmp2, 3);

        // compare results
        for (unsigned int i = 0; i < 12; ++i)
        {
            OCIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
        }
    }
}

OCIO_ADD_TEST(CreateOpVecFromOpDataVec, basic)
{
    OCIO::ConstOpDataVec opDataVec;
    auto mat = OCIO::MatrixOpData::CreateDiagonalMatrix(2.0);
    opDataVec.push_back(mat);

    auto range = std::make_shared<OCIO::RangeOpData>(0.0, 1.0, 0.5, 1.5);

    opDataVec.push_back(range);

    OCIO_REQUIRE_EQUAL(opDataVec.size(), 2);
	
    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::CreateOpVecFromOpDataVec(ops, opDataVec, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_REQUIRE_EQUAL(ops.size(), 2);

        OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
        OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<RangeOp>");
    }

    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::CreateOpVecFromOpDataVec(ops, opDataVec, OCIO::TRANSFORM_DIR_INVERSE));
        OCIO_REQUIRE_EQUAL(ops.size(), 2);

        OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<RangeOp>");
        OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<MatrixOffsetOp>");
    }
}

OCIO_ADD_TEST(Op, non_dynamic_ops)
{
    double scale[4] = { 2.0, 2.0, 2.0, 1.0 };

    OCIO::OpRcPtrVec ops;
    OCIO::CreateScaleOp(ops, scale, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);
    
    // Test that non-dynamic ops such as matrix respond properly to dynamic
    // property requests.
    OCIO_CHECK_ASSERT(!ops[0]->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE));
    OCIO_CHECK_ASSERT(!ops[0]->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST));
    OCIO_CHECK_ASSERT(!ops[0]->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA));

    OCIO_CHECK_THROW_WHAT(ops[0]->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA),
                          OCIO::Exception, "does not implement dynamic property");
}

OCIO_ADD_TEST(OpRcPtrVec, erase_insert)
{
    OCIO::OpRcPtrVec ops;
    auto mat = OCIO::MatrixOpData::CreateDiagonalMatrix(1.1);
    mat->setID("First");
    OCIO::CreateMatrixOp(ops, mat, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    
    auto range = std::make_shared<OCIO::RangeOpData>(0.0, 1.0, 0.5, 1.5);

    OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    // Test push_back.
    mat = OCIO::MatrixOpData::CreateDiagonalMatrix(1.3);
    OCIO::CreateMatrixOp(ops, mat, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_REQUIRE_EQUAL(ops.size(), 3);

    // Test erase.
    ops.erase(ops.begin()+1);

    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
    OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<MatrixOffsetOp>");

    // Test erase.
    OCIO::CreateLogOp(ops, 1.2, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateLogOp(ops, 1.1, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_REQUIRE_EQUAL(ops.size(), 5);

    ops.erase(ops.begin()+1, ops.begin()+4);

    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
    OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<RangeOp>");

    // Test insert.
    OCIO::OpRcPtrVec ops1 = ops;

    OCIO_REQUIRE_EQUAL(ops1.size(), 2);

    ops1.insert(ops1.begin()+1, ops.begin(), ops.begin()+1);
    OCIO_REQUIRE_EQUAL(ops1.size(), 3);
    OCIO_CHECK_EQUAL(ops1[0]->getInfo(), "<MatrixOffsetOp>");
    OCIO_CHECK_EQUAL(ops1[1]->getInfo(), "<MatrixOffsetOp>");
    OCIO_CHECK_EQUAL(ops1[2]->getInfo(), "<RangeOp>");

    // Test operator +=.
    OCIO::OpRcPtrVec ops2 = ops;
    OCIO_REQUIRE_EQUAL(ops2.size(), 2);

    ops2 += ops2;

    OCIO_REQUIRE_EQUAL(ops2.size(), 4);
    OCIO_CHECK_EQUAL(ops2[0]->getInfo(), "<MatrixOffsetOp>");
    OCIO_CHECK_EQUAL(ops2[1]->getInfo(), "<RangeOp>");
    OCIO_CHECK_EQUAL(ops2[2]->getInfo(), "<MatrixOffsetOp>");
    OCIO_CHECK_EQUAL(ops2[3]->getInfo(), "<RangeOp>");
}

OCIO_ADD_TEST(OpData, equality)
{
    auto mat1 = OCIO::MatrixOpData::CreateDiagonalMatrix(1.1);
    auto mat2 = mat1->clone();

    // Use the MatrixOpData::operator==().
    OCIO_CHECK_ASSERT(*mat2==*mat1);

    auto range = std::make_shared<OCIO::RangeOpData>(0.0, 1.0, 0.5, 1.5);

    // Use the MatrixOpData::operator==().
    OCIO_CHECK_ASSERT( !(*mat2==*range) );

    // Use the RangeOpData::operator==().
    OCIO_CHECK_ASSERT( !(*range==*mat1) );

    // Use the OpData::operator==().
    auto op1 = OCIO::DynamicPtrCast<OCIO::OpData>(range);
    OCIO_CHECK_ASSERT( !(*op1==*mat1) );    

    // Use the OpData::operator==().
    auto op2 = OCIO::DynamicPtrCast<OCIO::OpData>(mat2);
    OCIO_CHECK_ASSERT( !(*op2==*op1) );

    // Change something.

    // Use the MatrixOpData::operator==().
    OCIO_CHECK_ASSERT( *mat2==*mat1 );

    // Use the OpData::operator==().
    OCIO_CHECK_ASSERT( *op2==*mat1 );

    mat2->setOffsetValue(1, mat2->getOffsetValue(1) + 1.0);

    // Use the MatrixOpData::operator==().
    OCIO_CHECK_ASSERT( !(*mat2==*mat1) );

    // Use the OpData::operator==().
    OCIO_CHECK_ASSERT( !(*op2==*mat1) );
}

#endif

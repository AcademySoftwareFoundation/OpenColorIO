// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/cdl/CDLOp.h"
#include "ops/exponent/ExponentOp.h"
#include "ops/exposurecontrast/ExposureContrastOp.h"
#include "ops/fixedfunction/FixedFunctionOp.h"
#include "ops/gamma/GammaOp.h"
#include "ops/log/LogOp.h"
#include "ops/lut1d/Lut1DOp.h"
#include "ops/lut3d/Lut3DOp.h"
#include "ops/range/RangeOp.h"
#include "pystring/pystring.h"

namespace OCIO_NAMESPACE
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

void Op::combineWith(OpRcPtrVec & /*ops*/, ConstOpRcPtr & /*secondOp*/) const
{
    std::ostringstream os;
    os << "Op: " << getInfo() << " cannot be combined. ";
    os << "A type-specific combining function is not defined.";
    throw Exception(os.str().c_str());
}

void Op::validate() const
{
    m_data->validate();
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

bool OpRcPtrVec::isNoOp() const noexcept
{
    for (const auto & op : m_ops)
    {
        if(!op->isNoOp()) return false;
    }

    return true;
}

bool OpRcPtrVec::hasChannelCrosstalk() const noexcept
{
    for (const auto & op : m_ops)
    {
        if(op->hasChannelCrosstalk()) return true;
    }

    return false;
}

bool OpRcPtrVec::hasDynamicProperty(DynamicPropertyType type) const noexcept
{
    for (const auto & op : m_ops)
    {
        if (op->hasDynamicProperty(type))
        {
            return true;
        }
    }

    return false;
}

DynamicPropertyRcPtr OpRcPtrVec::getDynamicProperty(DynamicPropertyType type) const
{
    for (const auto & op : m_ops)
    {
        if (op->hasDynamicProperty(type))
        {
            return op->getDynamicProperty(type);
        }
    }

    throw Exception("Cannot find dynamic property.");
}

OpRcPtrVec OpRcPtrVec::clone() const 
{
    OpRcPtrVec cloned;

    for (const auto & op : m_ops)
    {
        cloned.push_back(op->clone());
    }

    return cloned;
}

OpRcPtrVec OpRcPtrVec::invert() const
{
    OpRcPtrVec inverted;

    OpRcPtrVec::const_reverse_iterator iter = m_ops.rbegin();
    OpRcPtrVec::const_reverse_iterator end  = m_ops.rend();
    for (; iter!=end; ++iter)
    {
        ConstOpRcPtr op = *iter;
        if (op->isNoOpType())
        {
            // Keep track of the information.
            inverted.push_back(op->clone());
        }
        else
        {
            ConstOpDataRcPtr data = op->data();
            CreateOpVecFromOpData(inverted, data, TRANSFORM_DIR_INVERSE);
        }
    }

    return inverted;
}

void OpRcPtrVec::validate() const
{
    for (auto & op : m_ops)
    {
        op->validate();
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
void OpRcPtrVec::unifyDynamicProperties()
{
    DynamicPropertyImplRcPtr dpExposure;
    DynamicPropertyImplRcPtr dpContrast;
    DynamicPropertyImplRcPtr dpGamma;

    for (auto op : m_ops)
    {
        UnifyDynamicProperty(op, dpExposure, DYNAMIC_PROPERTY_EXPOSURE);
        UnifyDynamicProperty(op, dpContrast, DYNAMIC_PROPERTY_CONTRAST);
        UnifyDynamicProperty(op, dpGamma, DYNAMIC_PROPERTY_GAMMA);
    }
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

void CreateOpVecFromOpData(OpRcPtrVec & ops,
                           const ConstOpDataRcPtr & opData,
                           TransformDirection dir)
{
    if (dir == TRANSFORM_DIR_UNKNOWN)
    {
        throw Exception("Cannot create Op with unspecified transform direction.");
    }

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

    case OpData::NoOpType:
    {
        throw Exception("OpData is not supported");
    }

    }
}

} // namespace OCIO_NAMESPACE


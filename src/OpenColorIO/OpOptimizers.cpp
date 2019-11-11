// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <iterator>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "Logging.h"
#include "Op.h"
#include "ops/Lut1D/Lut1DOp.h"
#include "ops/Lut1D/Lut1DOpData.h"
#include "ops/Range/RangeOpData.h"

OCIO_NAMESPACE_ENTER
{
    namespace
    {

    bool HasFlag(OptimizationFlags flags, OptimizationFlags queryFlag)
    {
        return (flags & queryFlag) == queryFlag;
    }

    bool IsPairInverseEnabled(OpData::Type type, OptimizationFlags flags)
    {
        switch (type)
        {
        case OpData::CDLType:
            return HasFlag(flags, OPTIMIZATION_PAIR_IDENTITY_CDL);
        case OpData::ExposureContrastType:
            return HasFlag(flags, OPTIMIZATION_PAIR_IDENTITY_EXPOSURE_CONTRAST);
        case OpData::FixedFunctionType:
            return HasFlag(flags, OPTIMIZATION_PAIR_IDENTITY_FIXED_FUNCTION);
        case OpData::GammaType:
            return HasFlag(flags, OPTIMIZATION_PAIR_IDENTITY_GAMMA);
        case OpData::Lut1DType:
            return HasFlag(flags, OPTIMIZATION_PAIR_IDENTITY_LUT1D);
        case OpData::Lut3DType:
            return HasFlag(flags, OPTIMIZATION_PAIR_IDENTITY_LUT3D);
        case OpData::LogType:
            return HasFlag(flags, OPTIMIZATION_PAIR_IDENTITY_LOG);

        case OpData::ExponentType:
        case OpData::MatrixType:
        case OpData::RangeType:
            return false; // Use composition to optimize.

        default:
            // Other type are not controled by a flag.
            return true;
        }
    }

    bool IsCombineEnabled(OpData::Type type, OptimizationFlags flags)
    {
        // Some types are controled by a flag.
        return (type == OpData::ExponentType && HasFlag(flags, OPTIMIZATION_COMP_EXPONENT)) ||
               (type == OpData::GammaType    && HasFlag(flags, OPTIMIZATION_COMP_GAMMA))    ||
               (type == OpData::Lut1DType    && HasFlag(flags, OPTIMIZATION_COMP_LUT1D))    ||
               (type == OpData::Lut3DType    && HasFlag(flags, OPTIMIZATION_COMP_LUT3D))    ||
               (type == OpData::MatrixType   && HasFlag(flags, OPTIMIZATION_COMP_MATRIX))   ||
               (type == OpData::RangeType    && HasFlag(flags, OPTIMIZATION_COMP_RANGE));
    }

    const int MAX_OPTIMIZATION_PASSES = 8;

    void RemoveNoOpTypes(OpRcPtrVec & opVec)
    {
        OpRcPtrVec::const_iterator iter = opVec.begin();
        while (iter != opVec.end())
        {
            ConstOpRcPtr o = (*iter);
            if (o->data()->getType() == OpData::NoOpType)
            {
                iter = opVec.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }

    // Ops are preserved, dynamic properties are made non-dynamic.
    void RemoveDynamicProperties(OpRcPtrVec & opVec)
    {
        const size_t nbOps = opVec.size();
        for (size_t i = 0; i < nbOps; ++i)
        {
            auto & op = opVec[i];
            if (op->isDynamic())
            {
                // Optimization flag is tested before.
                auto replacedBy = op->clone();
                replacedBy->removeDynamicProperties();
                opVec[i] = replacedBy;
            }
        }
    }

    int RemoveNoOps(OpRcPtrVec & opVec)
    {
        int count = 0;
        OpRcPtrVec::const_iterator iter = opVec.begin();
        while (iter != opVec.end())
        {
            if ((*iter)->isNoOp())
            {
                iter = opVec.erase(iter);
                ++count;
            }
            else
            {
                ++iter;
            }
        }
        return count;
    }

    int ReplaceIdentityOps(OpRcPtrVec & opVec, OptimizationFlags oFlags)
    {
        int count = 0;

        // Remove any identity ops (other than gamma).
        const bool optIdentity = HasFlag(oFlags, OPTIMIZATION_IDENTITY);
        // Remove identity gamma ops (handled separately to give control over negative
        // alpha clamping).
        const bool optIdGamma = HasFlag(oFlags, OPTIMIZATION_IDENTITY_GAMMA);
        if (optIdentity || optIdGamma)
        {
            const size_t nbOps = opVec.size();
            for (size_t i = 0; i < nbOps; ++i)
            {
                ConstOpRcPtr op = opVec[i];
                const auto type = op->data()->getType();
                if (type != OpData::RangeType && // Do not replace a range identity.
                    ((type == OpData::GammaType && optIdGamma) ||
                     (type != OpData::GammaType && optIdentity)) &&
                    op->isIdentity())
                {
                    // Optimization flag is tested before.
                    auto replacedBy = op->getIdentityReplacement();
                    opVec[i] = replacedBy;
                    ++count;
                }
            }
        }
        return count;
    }

    int RemoveInverseOps(OpRcPtrVec & opVec, OptimizationFlags oFlags)
    {
        int count      = 0;
        int firstindex = 0; // this must be a signed int

        while (firstindex < static_cast<int>(opVec.size() - 1))
        {
            ConstOpRcPtr op1 = opVec[firstindex];
            ConstOpRcPtr op2 = opVec[firstindex + 1];
            const auto type1 = op1->data()->getType();
            const auto type2 = op2->data()->getType();
            // The common case of inverse ops is to have a deep nesting:
            // ..., A, B, B', A', ...
            //
            // Consider the above, when firstindex reaches B:
            //
            //         |
            // ..., A, B, B', A', ...
            //
            // We will remove B and B'.
            // Firstindex remains pointing at the original location:
            //
            //         |
            // ..., A, A', ...
            //
            // We then decrement firstindex by 1,
            // to backstep and reconsider the A, A' case:
            //
            //      |            <-- firstindex decremented
            // ..., A, A', ...
            //

            if (type1 == type2 &&
                IsPairInverseEnabled(type1, oFlags) &&
                op1->isInverse(op2))
            {
                // When a pair of inverse ops is removed, we want the optimized ops to give the
                // same result as the original.  For certain ops such as Lut1D or Log this may
                // mean inserting a Range to emulate the clamping done by the original ops.
                auto replacedBy = op1->getIdentityReplacement();
                if (replacedBy->isNoOp())
                {
                    opVec.erase(opVec.begin() + firstindex, opVec.begin() + firstindex + 2);
                    firstindex = std::max(0, firstindex - 1);
                }
                else
                {
                    // Forward + inverse does clamp.
                    opVec[firstindex] = replacedBy;
                    opVec.erase(opVec.begin() + firstindex + 1);
                    ++firstindex;
                }
                ++count;
            }
            else
            {
                ++firstindex;
            }
        }

        return count;
    }

    int CombineOps(OpRcPtrVec & opVec, OptimizationFlags oFlags)
    {
        int count      = 0;
        int firstindex = 0; // this must be a signed int

        OpRcPtrVec tmpops;

        while (firstindex < static_cast<int>(opVec.size() - 1))
        {
            ConstOpRcPtr op1 = opVec[firstindex];
            ConstOpRcPtr op2 = opVec[firstindex + 1];
            const auto type1 = op1->data()->getType();

            if (IsCombineEnabled(type1, oFlags) && op1->canCombineWith(op2))
            {
                tmpops.clear();
                op1->combineWith(tmpops, op2);

                // tmpops may have any number of ops in it. (0, 1, 2, ...)
                // (size 0 would occur potentially iff the combination results in a no-op).
                //
                // No matter the number, we need to swap them in for the original ops.

                // Erase the initial two ops we've combined.
                opVec.erase(opVec.begin() + firstindex, opVec.begin() + firstindex + 2);

                // Insert the new ops (which may be empty) at this location.
                opVec.insert(opVec.begin() + firstindex, tmpops.begin(), tmpops.end());

                // Decrement firstindex by 1,
                // to backstep and reconsider the A, A' case.
                // See RemoveInverseOps for the full discussion of
                // why this is appropriate.
                firstindex = std::max(0, firstindex - 1);

                // We've done something so increment the count!
                ++count;
            }
            else
            {
                ++firstindex;
            }
        }

        return count;
    }

    int RemoveLeadingClampIdentity(OpRcPtrVec & opVec)
    {
        int count = 0;
        OpRcPtrVec::const_iterator iter = opVec.begin();
        while (iter != opVec.end())
        {
            ConstOpRcPtr o = (*iter);
            auto oData = o->data();
            if (oData->getType() == OpData::RangeType && oData->isIdentity())
            {
                iter++;
                ++count;
            }
            else
            {
                break;
            }
        }
        if (count != 0)
        {
            OpRcPtrVec::const_iterator iter = opVec.begin() + count;
            opVec.erase(opVec.begin(), iter);
        }
        return count;
    }

    int RemoveTrailingClampIdentity(OpRcPtrVec & opVec)
    {
        int count = 0;
        int current = static_cast<int>(opVec.size()) - 1;
        while (current >= 0)
        {
            ConstOpRcPtr o = opVec[current];
            auto oData = o->data();
            if (oData->getType() == OpData::RangeType && oData->isIdentity())
            {
                ++count;
                --current;
            }
            else
            {
                break;
            }
        }

        if (count != 0)
        {
            OpRcPtrVec::const_iterator iter = opVec.begin() + (current + 1);
            opVec.erase(iter, opVec.end());
        }
        return count;
    }

    // (Note: the term "separable" in mathematics refers to a multi-dimensional
    // function where the dimensions are independent of each other.)
    //
    // The goal here is to speed up calculations by replacing the contiguous separable
    // (channel independent) list of ops from the first op onwards with a single
    // LUT1D whose domain is sampled for the target bit depth.  A typical use-case
    // would be a list of ops that starts with a gamma that is processing integer 10i
    // pixels.  Rather than convert to float and apply the power function on each
    // pixel, it's better to build a 1024 entry LUT and just do a look-up.
    //
    unsigned FindSeparablePrefix(const OpRcPtrVec & ops)
    {
        unsigned prefixLen = 0;

        // Loop over the ops until we get to one that cannot be combined.
        //
        // Note: For some ops such as Matrix and CDL, the separability depends upon
        //       the parameters.
        for (const auto & op : ops)
        {
            // In OCIO, the hasChannelCrosstalk method returns false for separable ops.
            if (op->hasChannelCrosstalk() || op->isDynamic())
            {
                break;
            }

            // Op is separable, keep going.
            prefixLen++;
        }

        // If the only op is a 1D LUT, there is actually nothing to optimize
        // so set the length to 0.  (This also avoids an infinite loop.)
        // (If it is an inverse 1D LUT, proceed since we want to replace it with a 1D LUT.)
        if (prefixLen == 1)
        {
            ConstOpRcPtr constOp0 = ops[0];
            auto opData = constOp0->data();
            if (opData->getType() == OpData::Lut1DType)
            {
                auto lutData = OCIO_DYNAMIC_POINTER_CAST<const Lut1DOpData>(opData);
                if (lutData->getDirection() == TRANSFORM_DIR_FORWARD)
                {
                    return 0;
                }
            }
        }

        // Some ops are so fast that it may not make sense to replace just one of those.
        // E.g., if it's just a single matrix, it may not be faster to replace it with a LUT.
        // So make sure there are some more expensive ops to combine.
        unsigned expensiveOps = 0U;
        for (unsigned i = 0; i < prefixLen; ++i)
        {
            auto op = ops[i];

            if (op->hasChannelCrosstalk())
            {
                // Non-separable ops (should never get here).
                throw Exception("Non-separable op.");
            }

            ConstOpRcPtr constOp = op;
            switch (constOp->data()->getType())
            {
                // Potentially separable, but inexpensive ops.
                // TODO: Perhaps a LUT is faster once the conversion to float is considered?
                case OpData::MatrixType:
                case OpData::RangeType:
                {
                    break;
                }

                // Potentially separable, and more expensive.
                default:
                {
                    expensiveOps++;
                    break;
                }
            }
        }

        if (expensiveOps == 0)
        {
            return 0;
        }

        // TODO: The main source of potential lossiness is where there is a 1D LUT
        // that has extended range values followed by something that clamps.  In
        // that case, the clamp would get baked into the LUT entries and therefore
        // result in a different interpolated value.  Could look for that case and
        // turn off the optimization.

        return prefixLen;
    }

    // Use functional composition to replace a string of separable ops at the head of
    // the op list with a single 1D LUT that is built to do a look-up for the input bit-depth.
    void OptimizeSeparablePrefix(OpRcPtrVec & ops, BitDepth in)
    {
        if (ops.empty())
        {
            return;
        }

        // TODO: Investigate whether even the F32 case could be sped up via interpolating 
        //       in a half-domain Lut1D (e.g. replacing a string of exponent, log, etc.).
        if (in == BIT_DEPTH_F32 || in == BIT_DEPTH_UINT32)
        {
            return;
        }

        const unsigned prefixLen = FindSeparablePrefix(ops);
        if (prefixLen == 0)
        {
            return; // Nothing to do.
        }

        OpRcPtrVec prefixOps;
        for (unsigned i = 0; i < prefixLen; ++i)
        {
            prefixOps.push_back(ops[i]->clone());
        }

        // Make a domain for the LUT.  (Will be half-domain for target == 16f.)
        Lut1DOpDataRcPtr newDomain = Lut1DOpData::MakeLookupDomain(in);

        // Send the domain through the prefix ops.
        // Note: This sets the outBitDepth of newDomain to match prefixOps.
        Lut1DOpData::ComposeVec(newDomain, prefixOps);

        // Remove the prefix ops.
        ops.erase(ops.begin(), ops.begin() + prefixLen);

        // Insert the new LUT to replace the prefix ops.
        OpRcPtrVec lutOps;
        CreateLut1DOp(lutOps, newDomain, TRANSFORM_DIR_FORWARD);

        ops.insert(ops.begin(), lutOps.begin(), lutOps.end());
    }
    } // namespace

    void OptimizeOpVec(OpRcPtrVec & ops,
                       const BitDepth & inBitDepth,
                       const BitDepth & outBitDepth,
                       OptimizationFlags oFlags)
    {
        if (ops.empty())
            return;

        if (IsDebugLoggingEnabled())
        {
            LogDebug("Optimizing Op Vec...");
            LogDebug(SerializeOpVec(ops, 4));
        }

        // NoOpType can be removed.
        RemoveNoOpTypes(ops);

        if (oFlags == OPTIMIZATION_NONE)
        {
            return;
        }

        // Keep dynamic ops using their default values. Remove the ability to modify
        // them dynamically.
        const auto removeDynamic = HasFlag(oFlags, OPTIMIZATION_NO_DYNAMIC_PROPERTIES);
        if (removeDynamic)
        {
            RemoveDynamicProperties(ops);
        }

        // As the input and output bit-depths represent the color processing
        // request and they may be altered by the following optimizations,
        // preserve their values.

        const auto originalSize = ops.size();
        int total_noops         = 0;
        int total_identityops   = 0;
        int total_inverseops    = 0;
        int total_combines      = 0;
        int passes              = 0;

        const bool optimizeIdentity = HasFlag(oFlags, OPTIMIZATION_IDENTITY);

        while (passes <= MAX_OPTIMIZATION_PASSES)
        {
            int noops       = optimizeIdentity ? RemoveNoOps(ops) : 0;
            int identityops = ReplaceIdentityOps(ops, oFlags);
            int inverseops  = RemoveInverseOps(ops, oFlags);
            int combines    = CombineOps(ops, oFlags);

            if (noops + identityops + inverseops + combines == 0)
            {
                // No optimization progress was made, so stop trying.
                break;
            }

            total_noops += noops;
            total_identityops += identityops;
            total_inverseops += inverseops;
            total_combines += combines;

            ++passes;
        }

        if (!ops.empty())
        {
            if (!IsFloatBitDepth(inBitDepth))
            {
                RemoveLeadingClampIdentity(ops);
            }
            if (!IsFloatBitDepth(outBitDepth))
            {
                RemoveTrailingClampIdentity(ops);
            }

            if(HasFlag(oFlags, OPTIMIZATION_COMP_SEPARABLE_PREFIX))
            {
                OptimizeSeparablePrefix(ops, inBitDepth);
            }
        }

        OpRcPtrVec::size_type finalSize = ops.size();

        if (passes == MAX_OPTIMIZATION_PASSES)
        {
            std::ostringstream os;
            os << "The max number of passes, " << passes << ", ";
            os << "was reached during optimization. This is likely a sign ";
            os << "that either the complexity of the color transform is ";
            os << "very high, or that some internal optimizers are in conflict ";
            os << "(undo-ing / redo-ing the other's results).";
            LogDebug(os.str());
        }

        if (IsDebugLoggingEnabled())
        {
            std::ostringstream os;
            os << "Optimized ";
            os << originalSize << "->" << finalSize << ", ";
            os << passes << " passes, ";
            os << total_noops << " noops removed, ";
            os << total_identityops << " identity ops replaced, ";
            os << total_inverseops << " inverse ops removed\n";
            os << total_combines << " ops combines\n";
            os << SerializeOpVec(ops, 4);
            LogDebug(os.str());
        }
    }

}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

#include "ops/CDL/CDLOps.h"
#include "ops/Exponent/ExponentOps.h"
#include "ops/exposurecontrast/ExposureContrastOps.h"
#include "ops/FixedFunction/FixedFunctionOps.h"
#include "ops/Gamma/GammaOps.h"
#include "ops/Log/LogOps.h"
#include "ops/Matrix/MatrixOps.h"
#include "ops/Range/RangeOps.h"
#include "transforms/FileTransform.h"
#include "UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;

namespace
{
OCIO::OptimizationFlags AllBut(OCIO::OptimizationFlags notFlag)
{
    return static_cast<OCIO::OptimizationFlags>(OCIO::OPTIMIZATION_ALL & ~notFlag);
}

void Clone(OCIO::OpRcPtrVec & cloned, const OCIO::OpRcPtrVec toClone)
{
    cloned.clear();
    for (auto op : toClone)
    {
        cloned.push_back(op->clone());
    }
}

void CompareRender(OCIO::OpRcPtrVec & ops1, OCIO::OpRcPtrVec & ops2,
                   unsigned line, float errorThreshold,
                   bool forceAlphaInRange = false)
{
    std::vector<float> img1 = {
        0.778f,  0.824f,   0.885f,  0.153f,
        0.044f,  0.014f,   0.088f,  0.999f,
        0.488f,  0.381f,   0.f,     0.f,
        1.000f,  1.52e-4f, 0.0229f, 1.f,
        0.f,    -0.1f,    -2.f,    -0.1f,
        2.f,     1.9f,     0.f,     2.f };

    if (forceAlphaInRange)
    {
        img1[19] = 0.f;
    }

    std::vector<float> img2 = img1;

    const long nbPixels = (long)img1.size() / 4;

    OCIO_CHECK_NO_THROW_FROM(FinalizeOpVec(ops1, OCIO::OPTIMIZATION_LUT_INV_FAST), line);
    OCIO_CHECK_NO_THROW_FROM(FinalizeOpVec(ops2, OCIO::OPTIMIZATION_LUT_INV_FAST), line);

    for (const auto & op : ops1)
    {
        op->apply(&img1[0], &img1[0], nbPixels);
    }

    for (const auto & op : ops2)
    {
        op->apply(&img2[0], &img2[0], nbPixels);
    }

    for (size_t idx = 0; idx < img1.size(); ++idx)
    {
        OCIO_CHECK_CLOSE_FROM(img1[idx], img2[idx], errorThreshold, line);
    }
}

} // namespace

OCIO_ADD_TEST(OpOptimizers, remove_leading_clamp_identity)
{
    OCIO::OpRcPtrVec ops;

    auto range = std::make_shared<OCIO::RangeOpData>(0., 1., 0., 1.);
    auto range2 = std::make_shared<OCIO::RangeOpData>(0., 1., 0., 2.);
    auto matrix = std::make_shared<OCIO::MatrixOpData>();

    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 4);
    OCIO::RemoveLeadingClampIdentity(ops);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO::ConstOpRcPtr o0 = ops[0];
    OCIO_CHECK_EQUAL(o0->data()->getType(), OCIO::OpData::MatrixType);
    ops.clear();

    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 3);
    OCIO::RemoveLeadingClampIdentity(ops);
    OCIO_CHECK_EQUAL(ops.size(), 0);
    ops.clear();

    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 4);
    OCIO::RemoveLeadingClampIdentity(ops);
    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    o0 = ops[0];
    OCIO::ConstOpRcPtr o1 = ops[1];
    OCIO::ConstOpRcPtr o2 = ops[2];
    OCIO_CHECK_EQUAL(o0->data()->getType(), OCIO::OpData::MatrixType);
    OCIO_CHECK_EQUAL(o1->data()->getType(), OCIO::OpData::RangeType);
    OCIO_CHECK_EQUAL(o2->data()->getType(), OCIO::OpData::RangeType);

    ops.clear();

    // First range is not an identity, nothing to remove.
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range2, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 4);
    OCIO::RemoveLeadingClampIdentity(ops);
    OCIO_REQUIRE_EQUAL(ops.size(), 4);
    o0 = ops[0];
    o1 = ops[1];
    o2 = ops[2];
    OCIO::ConstOpRcPtr o3 = ops[3];
    OCIO_CHECK_EQUAL(o0->data()->getType(), OCIO::OpData::RangeType);
    OCIO_CHECK_EQUAL(o1->data()->getType(), OCIO::OpData::MatrixType);
    OCIO_CHECK_EQUAL(o2->data()->getType(), OCIO::OpData::RangeType);
    OCIO_CHECK_EQUAL(o3->data()->getType(), OCIO::OpData::RangeType);
    ops.clear();

    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range2, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 4);
    OCIO::RemoveLeadingClampIdentity(ops);
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    o0 = ops[0];
    o1 = ops[1];
    OCIO_CHECK_EQUAL(o0->data()->getType(), OCIO::OpData::RangeType);
    OCIO_CHECK_EQUAL(o1->data()->getType(), OCIO::OpData::MatrixType);
    ops.clear();
}

OCIO_ADD_TEST(OpOptimizers, remove_trailing_clamp_identity)
{
    OCIO::OpRcPtrVec ops;

    auto range = std::make_shared<OCIO::RangeOpData>(0., 1., 0., 1.);
    auto range2 = std::make_shared<OCIO::RangeOpData>(0., 1., 0., 2.);
    auto matrix = std::make_shared<OCIO::MatrixOpData>();

    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 4);
    OCIO::RemoveTrailingClampIdentity(ops);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO::ConstOpRcPtr o0 = ops[0];
    OCIO_CHECK_EQUAL(o0->data()->getType(), OCIO::OpData::MatrixType);
    ops.clear();

    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 3);
    OCIO::RemoveTrailingClampIdentity(ops);
    OCIO_CHECK_EQUAL(ops.size(), 0);
    ops.clear();

    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 4);
    OCIO::RemoveTrailingClampIdentity(ops);
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    o0 = ops[0];
    OCIO::ConstOpRcPtr o1 = ops[1];
    OCIO_CHECK_EQUAL(o0->data()->getType(), OCIO::OpData::RangeType);
    OCIO_CHECK_EQUAL(o1->data()->getType(), OCIO::OpData::MatrixType);
    ops.clear();

    // Last range is not an identity, nothing to remove.
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range2, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range2, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 4);
    OCIO::RemoveTrailingClampIdentity(ops);
    OCIO_REQUIRE_EQUAL(ops.size(), 4);
    o0 = ops[0];
    o1 = ops[1];
    OCIO::ConstOpRcPtr o2 = ops[2];
    OCIO::ConstOpRcPtr o3 = ops[3];
    OCIO_CHECK_EQUAL(o0->data()->getType(), OCIO::OpData::RangeType);
    OCIO_CHECK_EQUAL(o1->data()->getType(), OCIO::OpData::MatrixType);
    OCIO_CHECK_EQUAL(o2->data()->getType(), OCIO::OpData::RangeType);
    OCIO_CHECK_EQUAL(o3->data()->getType(), OCIO::OpData::RangeType);
    ops.clear();

    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range2, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 4);
    OCIO::RemoveTrailingClampIdentity(ops);
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    o0 = ops[0];
    o1 = ops[1];
    OCIO_CHECK_EQUAL(o0->data()->getType(), OCIO::OpData::RangeType);
    OCIO_CHECK_EQUAL(o1->data()->getType(), OCIO::OpData::MatrixType);
    ops.clear();
}

OCIO_ADD_TEST(OpOptimizers, remove_inverse_ops)
{
    OCIO::OpRcPtrVec ops;

    auto func = std::make_shared<OCIO::FixedFunctionOpData>();

    const double logSlope[3]  = {0.18, 0.18, 0.18};
    const double linSlope[3]  = {2.0, 2.0, 2.0};
    const double linOffset[3] = {0.1, 0.1, 0.1};
    const double base         = 10.0;
    const double logOffset[3] = {1.0, 1.0, 1.0};

    OCIO::CreateFixedFunctionOp(ops, func, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                      OCIO::TRANSFORM_DIR_INVERSE);
    OCIO::CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                      OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateFixedFunctionOp(ops, func, OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(ops.size(), 4);

    // Inverse + forward log are optimized as no-op then forward and inverse exponent are
    // optimized as no-op within the same call.
    OCIO::RemoveInverseOps(ops, OCIO::OPTIMIZATION_ALL);
    OCIO_CHECK_EQUAL(ops.size(), 0);
    ops.clear();

    OCIO::CreateFixedFunctionOp(ops, func, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                      OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                      OCIO::TRANSFORM_DIR_INVERSE);
    OCIO::CreateFixedFunctionOp(ops, func, OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(ops.size(), 4);

    // Forward + inverse log are optimized as a clamping range that stays between 
    // forward and inverse exponents.
    OCIO::RemoveInverseOps(ops, OCIO::OPTIMIZATION_ALL);
    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<FixedFunctionOp>");
    OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<RangeOp>");
    OCIO_CHECK_EQUAL(ops[2]->getInfo(), "<FixedFunctionOp>");
    ops.clear();

    OCIO::CreateFixedFunctionOp(ops, func, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateFixedFunctionOp(ops, func, OCIO::TRANSFORM_DIR_INVERSE);
    OCIO::CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                      OCIO::TRANSFORM_DIR_INVERSE);
    OCIO::CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset,
                      OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateFixedFunctionOp(ops, func, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(ops.size(), 5);

    OCIO::RemoveInverseOps(ops, OCIO::OPTIMIZATION_ALL);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<FixedFunctionOp>");
}

OCIO_ADD_TEST(OpOptimizers, combine_ops)
{
    double m1[4] = {2.0, 2.0, 2.0, 1.0};
    double m2[4] = {0.5, 0.5, 0.5, 1.0};
    double m3[4] = {0.6, 0.6, 0.6, 1.0};
    double m4[4] = {0.7, 0.7, 0.7, 1.0};

    const double exp[4] = {1.2, 1.3, 1.4, 1.5};

    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_EQUAL(ops.size(), 1);
        OCIO::CombineOps(ops, OCIO::OPTIMIZATION_ALL);
        OCIO_CHECK_EQUAL(ops.size(), 1);
    }

    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateScaleOp(ops, m3, OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO::CombineOps(ops, AllBut(OCIO::OPTIMIZATION_COMP_MATRIX));
        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO::CombineOps(ops, OCIO::OPTIMIZATION_ALL);
        OCIO_CHECK_EQUAL(ops.size(), 1);
    }

    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateScaleOp(ops, m3, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateScaleOp(ops, m4, OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_EQUAL(ops.size(), 3);
        OCIO::CombineOps(ops, AllBut(OCIO::OPTIMIZATION_COMP_MATRIX));
        OCIO_CHECK_EQUAL(ops.size(), 3);
        OCIO::CombineOps(ops, OCIO::OPTIMIZATION_ALL);
        OCIO_CHECK_EQUAL(ops.size(), 1);
    }

    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateScaleOp(ops, m2, OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO::CombineOps(ops, AllBut(OCIO::OPTIMIZATION_COMP_MATRIX));
        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO::CombineOps(ops, OCIO::OPTIMIZATION_ALL);
        OCIO_CHECK_EQUAL(ops.size(), 0);
    }

    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_INVERSE);

        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO::CombineOps(ops, AllBut(OCIO::OPTIMIZATION_COMP_MATRIX));
        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO::CombineOps(ops, OCIO::OPTIMIZATION_ALL);
        OCIO_CHECK_EQUAL(ops.size(), 0);
    }

    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_EQUAL(ops.size(), 5);
        OCIO::CombineOps(ops, AllBut(OCIO::OPTIMIZATION_COMP_MATRIX));
        OCIO_CHECK_EQUAL(ops.size(), 5);
        OCIO::CombineOps(ops, OCIO::OPTIMIZATION_ALL);
        OCIO_CHECK_EQUAL(ops.size(), 1);
    }

    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateExponentOp(ops, exp, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateScaleOp(ops, m1, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateScaleOp(ops, m2, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateExponentOp(ops, exp, OCIO::TRANSFORM_DIR_INVERSE);

        OCIO_CHECK_EQUAL(ops.size(), 4);
        OCIO::CombineOps(ops, AllBut(OCIO::OPTIMIZATION_COMP_MATRIX));
        OCIO_CHECK_EQUAL(ops.size(), 4);
        OCIO::CombineOps(ops, OCIO::OPTIMIZATION_ALL);
        OCIO_CHECK_EQUAL(ops.size(), 0);
    }
}

OCIO_ADD_TEST(OpOptimizers, non_optimizable)
{
    OCIO::OpRcPtrVec ops;
    // Create non identity Matrix.
    const double m44[16] = { 2., 0., 0., 0.,
                             0., 1., 0., 0.,
                             0., 0., 1., 0.,
                             0., 0., 0., 1. };
    const double offset4[4] = { 0., 0., 0., 0. };
    OCIO::CreateMatrixOffsetOp(ops, m44, offset4, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_CHECK_EQUAL(ops.size(), 1);

    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                            OCIO::OPTIMIZATION_DEFAULT));

    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    OCIO::ConstOpRcPtr op = ops[0];
    OCIO_REQUIRE_ASSERT(op);
    auto mat = OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixOpData>(op->data()); 
    OCIO_REQUIRE_ASSERT(mat);

    OCIO_CHECK_EQUAL(mat->getArray().getValues()[0], 2.);
    OCIO_CHECK_ASSERT(mat->isDiagonal());
}

OCIO_ADD_TEST(OpOptimizers, optimizable)
{
    OCIO::OpRcPtrVec ops;
    // Create identity Matrix.
    double m44[16] = { 1., 0., 0., 0.,
                       0., 1., 0., 0.,
                       0., 0., 1., 0.,
                       0., 0., 0., 1. };
    const double offset4[4] = { 0., 0., 0., 0. };
    OCIO::CreateMatrixOffsetOp(ops, m44, offset4, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_CHECK_EQUAL(ops.size(), 1);

    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                            OCIO::OPTIMIZATION_DEFAULT));

    // Identity matrix is a no-op and is removed. CPU processor will re-add an identity matrix
    // if there are no ops left.
    OCIO_CHECK_EQUAL(ops.size(), 0);
    ops.clear();

    // Add identity matrix.
    OCIO::CreateMatrixOffsetOp(ops, m44, offset4, OCIO::TRANSFORM_DIR_FORWARD);

    // No more an 'identity matrix'.
    m44[0] = 2.;
    m44[1] = 2.;
    OCIO::CreateMatrixOffsetOp(ops, m44, offset4, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_CHECK_EQUAL(ops.size(), 2);

    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                            OCIO::OPTIMIZATION_DEFAULT));

    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    OCIO::ConstOpRcPtr op = ops[0];
    OCIO_REQUIRE_ASSERT(op);
    auto mat = OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixOpData>(op->data());
    OCIO_REQUIRE_ASSERT(mat);
    OCIO_CHECK_ASSERT(!mat->isIdentity());
    OCIO_CHECK_ASSERT(!mat->isDiagonal());
}

OCIO_ADD_TEST(OpOptimizers, optimization)
{
    // This is a transform consisting of a Lut1d, Matrix, Matrix, Lut1d.
    // The matrices and luts are inverses of one another, so when they are
    // composed they become identities which are then replaced.
    // So this one test actually tests quite a lot of the optimize and compose
    // functionality.

    const std::string fileName("opt_test1.ctf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, fileName, context,
                                           OCIO::TRANSFORM_DIR_FORWARD));

    // First one is the file no op.
    OCIO_CHECK_EQUAL(ops.size(), 5);

    OCIO_CHECK_NO_THROW(OCIO::RemoveNoOpTypes(ops));

    OCIO_CHECK_EQUAL(ops.size(), 4);

    OCIO::OpRcPtrVec optOps;
    Clone(optOps, ops);
    OCIO_CHECK_EQUAL(optOps.size(), 4);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOps, OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                            OCIO::OPTIMIZATION_DEFAULT));

    OCIO_REQUIRE_EQUAL(optOps.size(), 1);
    OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");

    // Compare renders.
    CompareRender(ops, optOps, __LINE__, 1e-6f);
}

OCIO_ADD_TEST(OpOptimizers, optimization2)
{
    // This transform has the following ops:
    // 1 Lut1D, half domain, effectively an identity
    // 2 Matrix, bit depth conversion identity
    // 3 Range, bit depth conversion identity
    // 4 Matrix, almost identity
    // 5 Range, clamp identity
    // 6 Lut1D, half domain, raw halfs, identity
    // 7 Lut1D, raw halfs, identity
    // 8 Matrix, not identity
    // 9 Matrix, not identity
    // 10 Lut1D, almost identity
    // 11 Lut1D, almost identity that composes to an identity with the previous one
    // 12 Lut3D, not identity
    // 13 Lut3D, not identity but composes to an identity with the previous one

    const std::string fileName("opt_test2.ctf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, fileName, context,
                                           OCIO::TRANSFORM_DIR_FORWARD));

    // First one is the file no op.
    OCIO_CHECK_EQUAL(ops.size(), 14);

    OCIO_CHECK_NO_THROW(OCIO::RemoveNoOpTypes(ops));

    OCIO_CHECK_EQUAL(ops.size(), 13);

    OCIO::OpRcPtrVec optOps;
    Clone(optOps, ops);
    OCIO_CHECK_EQUAL(optOps.size(), 13);

    // No need to remove OPTIMIZATION_COMP_SEPARABLE_PREFIX because optimization is for F32.
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOps, OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                            OCIO::OPTIMIZATION_GOOD));
    OCIO_REQUIRE_EQUAL(optOps.size(), 4);

    // Op 1 is exactly an identity except for the first value which is 0.000001. Since the
    // outDepth=16i, this gets normalized by 1/65536, which puts it well under the noise
    // threshold so it's optimized it out as an identity.

    // Ops 2 & 3 are identities and removed.
    // This is op 4.
    OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<MatrixOffsetOp>");
    // Op 5 is a clamp identity and removed.
    // Ops 6 & 7 are identities and are replaced by ranges, one is removed.
    OCIO_CHECK_EQUAL(optOps[1]->getInfo(), "<RangeOp>");
    // This is op 8 composed with op 9.
    OCIO_CHECK_EQUAL(optOps[2]->getInfo(), "<MatrixOffsetOp>");
    // Ops 10 & 11 composed become an identity, is replaced with a range,
    // which is then removed as a clamp identity
    // This is op 12 composed with op 13.  It is an identity.
    // NB: We don't try to detect Lut3DOp identities.
    OCIO_CHECK_EQUAL(optOps[3]->getInfo(), "<Lut3DOp>");

    // Compare renders.
    CompareRender(ops, optOps, __LINE__, 1e-6f);
}

OCIO_ADD_TEST(OpOptimizers, lut1d_identities)
{
    // This transform has the following ops:
    // 1 Lut1D, identity
    // 2 Lut1D, identity
    // 3 Lut1D, not quite identity
    // 4 Lut1D, half-domain identity (note 16i outDepth)
    // 5 Lut1D, half-domain not an identity (values clamped due to rawHalfs encoding)
    // 6 Lut1D, identity

    const std::string fileName("lut1d_identity_test.ctf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, fileName, context,
                                           OCIO::TRANSFORM_DIR_FORWARD));

    // First one is the file no op.
    OCIO_REQUIRE_EQUAL(ops.size(), 7);

    OCIO_CHECK_ASSERT(ops[1]->isIdentity());
    OCIO_CHECK_ASSERT(ops[2]->isIdentity());
    OCIO_CHECK_ASSERT(false == ops[3]->isIdentity());
    OCIO_CHECK_ASSERT(ops[4]->isIdentity());
    OCIO_CHECK_ASSERT(false == ops[5]->isIdentity());
    OCIO_CHECK_ASSERT(ops[6]->isIdentity());

    OCIO::OpRcPtrVec optOps;
    Clone(optOps, ops);
    OCIO_CHECK_EQUAL(optOps.size(), 7);

    // No need to remove OPTIMIZATION_COMP_SEPARABLE_PREFIX because optimization is for F32.
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOps, OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                            OCIO::OPTIMIZATION_DEFAULT));

    OCIO_REQUIRE_EQUAL(optOps.size(), 2);

    // The first two LUTs should get detected as identities and replaced with ranges which then
    // get removed as clamp identities. LUT 3 is not identity and is not removed.
    // The next LUT should also get detected as an identity, and replaced with a matrix (rather
    // than a range since it is half-domain) which is then optimized out. The next LUT is almost
    // an identity except the large values are clamped due to the rawHalfs encoding, so it is not
    // removed. The final LUT is a normal domain to be replaced with a range. The two Luts get
    // combined into a single Lut with a standard domain.

    OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<Lut1DOp>");
    OCIO_CHECK_EQUAL(optOps[1]->getInfo(), "<RangeOp>");

    OCIO::ConstOpRcPtr lutOp = optOps[0];
    auto lutData = OCIO::DynamicPtrCast<const OCIO::Lut1DOpData>(lutOp->data());
    OCIO_REQUIRE_ASSERT(lutData);
    OCIO_CHECK_EQUAL(lutData->getArray().getLength(), 65536);
    OCIO_CHECK_ASSERT(!lutData->isInputHalfDomain());

    // Now check that the optimized transform renders the same as the original.
    // TODO: Shall investigate why this test requires a bigger error.
    CompareRender(ops, optOps, __LINE__, 3e-4f);
}

OCIO_ADD_TEST(OpOptimizers, lut1d_identity_replacement)
{
    // Test that an identity Lut1D becomes a range but a half-domain becomes a matrix.
    {
        auto lutData = std::make_shared<OCIO::Lut1DOpData>(3);
        OCIO_CHECK_ASSERT(lutData->isIdentity());

        OCIO::OpRcPtrVec ops;
        OCIO::CreateLut1DOp(ops, lutData, OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_EQUAL(ops.size(), 1);

        OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops));
        OCIO_REQUIRE_EQUAL(ops.size(), 1);

        OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<RangeOp>");
    }
    {
        // By default, this constructor creates an 'identity lut'.
        OCIO::Lut1DOpDataRcPtr lutData
            = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_INPUT_OUTPUT_HALF_CODE,
                                                  65536);
        lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_F32);
        OCIO_CHECK_ASSERT(lutData->isIdentity());
        OCIO_CHECK_ASSERT(lutData->isInputHalfDomain());

        OCIO::OpRcPtrVec ops;
        OCIO::CreateLut1DOp(ops, lutData, OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_EQUAL(ops.size(), 1);

        OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops));

        // Half domain LUT 1d is a no-op.
        // CPU processor will add an identity matrix.
        OCIO_CHECK_EQUAL(ops.size(), 0);
    }
}

OCIO_ADD_TEST(OpOptimizers, lut1d_half_domain_keep_prior_range)
{
    // A half-domain LUT should not allow removal of a prior range op.

    OCIO::OpRcPtrVec ops;
    OCIO::CreateRangeOp(ops, 0., 1., 0., 1., OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::Lut1DOpDataRcPtr lutData
        = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_INPUT_OUTPUT_HALF_CODE,
                                              65536);
    lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_F32);

    // Add no-op LUT.
    OCIO::CreateLut1DOp(ops, lutData, OCIO::TRANSFORM_DIR_INVERSE);

    // Add another LUT.
    lutData = lutData->clone();
    auto & lutArray = lutData->getArray();
    for (auto & val : lutArray.getValues())
    {
        val = -val;
    }
    OCIO::CreateLut1DOp(ops, lutData, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_CHECK_EQUAL(ops.size(), 3);

    OCIO::OpRcPtrVec optOps;
    Clone(optOps, ops);
    OCIO_CHECK_EQUAL(optOps.size(), 3);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOps));
    OCIO_CHECK_EQUAL(optOps.size(), 2);

    OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
    OCIO_CHECK_EQUAL(optOps[1]->getInfo(), "<Lut1DOp>");

    // Now check that the optimized transform renders the same as the original.
    CompareRender(ops, optOps, __LINE__, 1e-6f);
}

OCIO_ADD_TEST(OpOptimizers, range_composition)
{
    const double emptyValue = OCIO::RangeOpData::EmptyValue();
    {
        // Two identity clamp negs ranges are collapsed into one.
        OCIO::OpRcPtrVec ops;
        OCIO::CreateRangeOp(ops, 0., emptyValue, 0., emptyValue, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0., emptyValue, 0., emptyValue, OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO::OpRcPtrVec optOps;
        Clone(optOps, ops);
        OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOps));
        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
        CompareRender(ops, optOps, __LINE__, 1e-6f);
    }
    {
        // Non identity ranges are combined.
        OCIO::OpRcPtrVec ops;
        OCIO::CreateRangeOp(ops, 0.1, emptyValue, 0., emptyValue, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0.1, emptyValue, 0., emptyValue, OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO::OpRcPtrVec optOps;
        Clone(optOps, ops);
        OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOps));
        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
        CompareRender(ops, optOps, __LINE__, 1e-6f);
    }
    {
        // Non identity ranges are combined.
        OCIO::OpRcPtrVec ops;
        OCIO::CreateRangeOp(ops, 0.1, emptyValue, 0., emptyValue, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0.1, emptyValue, 0., emptyValue, OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_REQUIRE_EQUAL(ops.size(), 2);

        OCIO::OpRcPtrVec optOps;
        Clone(optOps, ops);
        OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOps));
        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
        CompareRender(ops, optOps, __LINE__, 1e-6f);
    }
    {
        // Non identity ranges are combined.
        OCIO::OpRcPtrVec ops;
        OCIO::CreateRangeOp(ops, -0.1, emptyValue, 0., emptyValue, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, -0.1, emptyValue, 0., emptyValue, OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_REQUIRE_EQUAL(ops.size(), 2);

        OCIO::OpRcPtrVec optOps;
        Clone(optOps, ops);
        OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOps));
        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
        CompareRender(ops, optOps, __LINE__, 1e-6f);
    }
    {
        // A clamp negs range is dropped before a more restrictive one.
        OCIO::OpRcPtrVec ops;
        OCIO::CreateRangeOp(ops, 0., emptyValue, 0., emptyValue, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0.1, 2., 0., 2., OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO::OpRcPtrVec optOps;
        Clone(optOps, ops);
        OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOps));
        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
        CompareRender(ops, optOps, __LINE__, 1e-6f);
    }
    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateRangeOp(ops, 0., 0.5, 0., 0.5, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0.1, 1., 0.1, 1., OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO::OpRcPtrVec optOps;
        Clone(optOps, ops);
        OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOps));
        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
        CompareRender(ops, optOps, __LINE__, 1e-6f);
    }
    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateRangeOp(ops, 0., 0.6, 0., 0.5, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0.1, 1., 0.2, 1., OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0., 1., 0., 2., OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_EQUAL(ops.size(), 3);
        OCIO::OpRcPtrVec optOps;
        Clone(optOps, ops);
        OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOps));
        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
        CompareRender(ops, optOps, __LINE__, 1e-6f);
    }
    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateRangeOp(ops, 0., 0.5, 0., 0.5, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0.6, 1., 0.6, 1., OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO::OpRcPtrVec optOps;
        Clone(optOps, ops);
        // Two Ranges with non-overlapping pass regions are replaced with a clamp to a constant.
        OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOps));
        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
        CompareRender(ops, optOps, __LINE__, 1e-6f);
    }
    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateRangeOp(ops, 0.6, 1., 0.6, 1., OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0., 0.5, 0., 0.5, OCIO::TRANSFORM_DIR_FORWARD);

        OCIO_CHECK_EQUAL(ops.size(), 2);
        OCIO::OpRcPtrVec optOps;
        Clone(optOps, ops);
        // Ranges can not be combined out domain of the first does not intersect
        // with in domain of the second.
        OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOps));
        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
        CompareRender(ops, optOps, __LINE__, 1e-6f);
    }
    {
        OCIO::OpRcPtrVec ops;
        OCIO::CreateRangeOp(ops, 0., 0.6, 0., 0.5, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0.1, 1., 0.2, 1., OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0.1, 1., 0.2, 1., OCIO::TRANSFORM_DIR_INVERSE);
        OCIO::CreateRangeOp(ops, 0., 1., 0., 2., OCIO::TRANSFORM_DIR_FORWARD);
        OCIO::CreateRangeOp(ops, 0.1, 0.8, 0.2, 1., OCIO::TRANSFORM_DIR_INVERSE);
        OCIO::CreateRangeOp(ops, 0.2, 0.6, 0.1, 0.7, OCIO::TRANSFORM_DIR_INVERSE);

        OCIO_CHECK_EQUAL(ops.size(), 6);
        OCIO::OpRcPtrVec optOps;
        Clone(optOps, ops);
        OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOps));
        OCIO_REQUIRE_EQUAL(optOps.size(), 1);
        OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
        CompareRender(ops, optOps, __LINE__, 1e-6f);
    }
}

OCIO_ADD_TEST(OpOptimizers, invlut_pair_identities)
{
    // The file contains an InverseLUT1D and LUT1D, both with the same array, followed by
    // an InverseLUT3D and LUT3D, also both with the same array.  The pairs should each get
    // replaced by a range and then the ranges should be combined.
    const std::string fileName("lut_inv_pairs.ctf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, fileName, context,
                                           OCIO::TRANSFORM_DIR_FORWARD));

    // First one is the file no op.
    OCIO_CHECK_EQUAL(ops.size(), 5);

    // Remove no ops.
    OCIO_CHECK_NO_THROW(OCIO::RemoveNoOpTypes(ops));

    OCIO_CHECK_EQUAL(ops.size(), 4);

    OCIO::OpRcPtrVec optOps;
    Clone(optOps, ops);
    OCIO_CHECK_EQUAL(optOps.size(), 4);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOps, OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                            OCIO::OPTIMIZATION_DEFAULT));

    OCIO_REQUIRE_EQUAL(optOps.size(), 1);
    OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
    // (A range that clamps on either side is not a no-op.)
    OCIO_CHECK_ASSERT(!optOps[0]->isNoOp());

    // Now check that the optimized transform renders the same as the original.
    CompareRender(ops, optOps, __LINE__, 1e-6f);
}

OCIO_ADD_TEST(OpOptimizers, mntr_identities)
{
    // Forward and inverse monitor transforms should become an identity.
    const std::string fileName("mntr_srgb_identity.ctf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, fileName, context,
                                           OCIO::TRANSFORM_DIR_FORWARD));

    // First one is the file no op.
    OCIO_CHECK_EQUAL(ops.size(), 5);

    // Remove no ops.
    OCIO_CHECK_NO_THROW(OCIO::RemoveNoOpTypes(ops));

    OCIO_CHECK_EQUAL(ops.size(), 4);

    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops));

    // Identity transform.
    OCIO_CHECK_EQUAL(ops.size(), 0);
}

OCIO_ADD_TEST(OpOptimizers, gamma_comp)
{
    // This transform has a pair of gammas separated by an identity matrix
    // that should compose into a single (non-identity) gamma that then should
    // be identified as a pair identity with another gamma and be replaced with
    // a clamp-negs range.

    const std::string fileName("gamma_comp_test.ctf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, fileName, context,
                                           OCIO::TRANSFORM_DIR_FORWARD));

    // First one is the file no op.
    OCIO_CHECK_EQUAL(ops.size(), 5);

    // Remove no ops.
    OCIO_CHECK_NO_THROW(OCIO::RemoveNoOpTypes(ops));

    OCIO_CHECK_EQUAL(ops.size(), 4);

    OCIO::OpRcPtrVec optOps;
    Clone(optOps, ops);
    OCIO::OpRcPtrVec optOps_noComp;
    Clone(optOps_noComp, ops);
    OCIO::OpRcPtrVec optOps_noIdentity;
    Clone(optOps_noIdentity, ops);

    OCIO_CHECK_EQUAL(optOps_noComp.size(), 4);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOps_noComp, OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                            AllBut(OCIO::OPTIMIZATION_COMP_GAMMA)));
    // Identity matrix is removed but gamma are not combined.
    OCIO_CHECK_EQUAL(optOps_noComp.size(), 3);
    OCIO_CHECK_EQUAL(optOps_noComp[0]->getInfo(), "<GammaOp>");
    OCIO_CHECK_EQUAL(optOps_noComp[1]->getInfo(), "<GammaOp>");
    OCIO_CHECK_EQUAL(optOps_noComp[2]->getInfo(), "<GammaOp>");

    CompareRender(ops, optOps_noComp, __LINE__, 1e-6f);

    OCIO_CHECK_EQUAL(optOps_noIdentity.size(), 4);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOps_noIdentity, OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                            AllBut(OCIO::OPTIMIZATION_IDENTITY_GAMMA)));
    // Identity matrix is removed and gammas are combined into an identity gamma
    // but it is not removed.
    OCIO_CHECK_EQUAL(optOps_noIdentity.size(), 1);
    OCIO_CHECK_EQUAL(optOps_noIdentity[0]->getInfo(), "<GammaOp>");

    CompareRender(ops, optOps_noIdentity, __LINE__, 5e-5f);

    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOps));
    // Identity matrix is removed and gammas combined and optimized as a range.
    OCIO_REQUIRE_EQUAL(optOps.size(), 1);
    OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");

    // Now check that the optimized transform renders the same as the original.
    // TODO: Gamma is clamping alpha, and Range does not.
    CompareRender(ops, optOps, __LINE__, 1e-4f, true);
}

OCIO_ADD_TEST(OpOptimizers, gamma_comp_identity)
{
    OCIO::OpRcPtrVec ops;

    OCIO::GammaOpData::Params params1 = { 0.45 };
    OCIO::GammaOpData::Params paramsA = { 1. };

    auto gamma1 = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_FWD,
                                                      params1, params1, params1, paramsA);

    OCIO::GammaOpData::Params params2 = { 1. / 0.45 };

    auto gamma2 = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_FWD,
                                                      params2, params2, params2, paramsA);

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gamma1, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gamma2, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 2);

    OCIO::OpRcPtrVec optOps;
    Clone(optOps, ops);

    // BASIC gamma are composed resulting into identity, that get optimized as a range.
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOps, OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                             OCIO::OPTIMIZATION_DEFAULT));

    OCIO_REQUIRE_EQUAL(optOps.size(), 1);
    OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");

    ops.clear();
    optOps.clear();

    gamma1 = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::MONCURVE_FWD,
                                                 params1, params1, params1, paramsA);
    gamma2 = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::MONCURVE_FWD,
                                                 params2, params2, params2, paramsA);

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gamma1, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gamma2, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 2);

    Clone(optOps, ops);

    // MONCURVE composition is not supported yet.
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOps, OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                            OCIO::OPTIMIZATION_DEFAULT));

    OCIO_REQUIRE_EQUAL(optOps.size(), 2);
    OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<GammaOp>");
    OCIO_CHECK_EQUAL(optOps[1]->getInfo(), "<GammaOp>");
}

OCIO_ADD_TEST(OpOptimizers, log_identities)
{
    // Log fwd and rev transforms should become a range.
    // This transform has two pair of LogOps separated by an identity matrix
    // that should optimize into a range.

    const std::string fileName("log_identities.ctf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, fileName, context,
                                           OCIO::TRANSFORM_DIR_FORWARD));


    // First one is the file no op.
    OCIO_CHECK_EQUAL(ops.size(), 6);

    // Remove no ops.
    OCIO_CHECK_NO_THROW(OCIO::RemoveNoOpTypes(ops));

    OCIO_CHECK_EQUAL(ops.size(), 5);

    OCIO::OpRcPtrVec optOps;
    Clone(optOps, ops);
    OCIO::OpRcPtrVec optOpsOff;
    Clone(optOpsOff, ops);

    OCIO_CHECK_EQUAL(optOpsOff.size(), 5);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOpsOff, OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                            AllBut(OCIO::OPTIMIZATION_PAIR_IDENTITY_LOG)));
    // Only the identity matrix is optimized.
    OCIO_CHECK_EQUAL(optOpsOff.size(), 4);

    OCIO_CHECK_EQUAL(optOps.size(), 5);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOps));
    // Identity matrix is optimized and forward/inverse log are combined.
    OCIO_REQUIRE_EQUAL(optOps.size(), 1);

    // (A range that clamps on either side is not a no-op.)
    OCIO_CHECK_ASSERT(!optOps[0]->isNoOp());
    OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");

    // Now check that the optimized transform renders the same as the original.
    CompareRender(ops, optOps, __LINE__, 1e-4f);
}

OCIO_ADD_TEST(OpOptimizers, range_lut)
{
    // Non-identity range before a Lut1D should not be removed.

    const std::string fileName("range_lut.ctf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, fileName, context,
                                           OCIO::TRANSFORM_DIR_FORWARD));

    // First one is the file no op.
    OCIO_CHECK_EQUAL(ops.size(), 3);

    // Remove no ops.
    OCIO_CHECK_NO_THROW(OCIO::RemoveNoOpTypes(ops));

    OCIO_CHECK_EQUAL(ops.size(), 2);

    OCIO::OpRcPtrVec optOps;
    Clone(optOps, ops);
    OCIO_CHECK_EQUAL(optOps.size(), 2);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOps));
    OCIO_REQUIRE_EQUAL(optOps.size(), 2);
    OCIO_CHECK_EQUAL(optOps[0]->getInfo(), "<RangeOp>");
    OCIO_CHECK_EQUAL(optOps[1]->getInfo(), "<Lut1DOp>");

    // Now check that the optimized transform renders the same as the original.
    CompareRender(ops, optOps, __LINE__, 1e-6f);
}

OCIO_ADD_TEST(OpOptimizers, dynamic_ops)
{
    // Non-identity matrix.
    OCIO::MatrixOpDataRcPtr matrix = std::make_shared<OCIO::MatrixOpData>();
    matrix->setArrayValue(0, 2.);

    // Identity exposure contrast.
    OCIO::ExposureContrastOpDataRcPtr exposure = std::make_shared<OCIO::ExposureContrastOpData>();

    OCIO::ExposureContrastOpDataRcPtr exposureDyn = exposure->clone();
    exposureDyn->getExposureProperty()->makeDynamic();

    // Test with non dynamic exposure contrast.
    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, exposure,
                                                           OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_REQUIRE_EQUAL(ops.size(), 2);
        OCIO_CHECK_ASSERT(!ops[0]->isIdentity());
        OCIO_CHECK_ASSERT(ops[1]->isIdentity());

        OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops));
        // It gets optimized.
        OCIO_REQUIRE_EQUAL(ops.size(), 1);
        OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
    }

    // Test with dynamic exposure contrast.
    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(OCIO::CreateExposureContrastOp(ops, exposureDyn,
                                                           OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_REQUIRE_EQUAL(ops.size(), 3);
        OCIO_CHECK_ASSERT(!ops[0]->isIdentity());

        // Exposure contrast is dynamic.
        OCIO_CHECK_ASSERT(ops[1]->isDynamic());
        OCIO_CHECK_ASSERT(!ops[1]->isIdentity());

        OCIO_CHECK_ASSERT(!ops[2]->isIdentity());

        // It does not get optimized with default flags (OPTIMIZATION_NO_DYNAMIC_PROPERTIES off).
        OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops));
        OCIO_REQUIRE_EQUAL(ops.size(), 3);
        OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
        OCIO_CHECK_EQUAL(ops[1]->getInfo(), "<ExposureContrastOp>");
        OCIO_CHECK_EQUAL(ops[2]->getInfo(), "<MatrixOffsetOp>");

        // It does get optimized if flag is set.
        // OPTIMIZATION_ALL includes OPTIMIZATION_NO_DYNAMIC_PROPERTIES.
        // Exposure contrast will get optimized and the 2 matrix will be composed.
        OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                                OCIO::OPTIMIZATION_ALL));
        OCIO_REQUIRE_EQUAL(ops.size(), 1);
        OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
    }
}

OCIO_ADD_TEST(OpOptimizers, gamma_prefix)
{
    OCIO::OpRcPtrVec originalOps;

    OCIO::GammaOpData::Params params1 = {2.6};
    OCIO::GammaOpData::Params paramsA = {1.};

    OCIO::GammaOpDataRcPtr gamma1
        = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_REV, 
                                              params1, params1, params1, paramsA);

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(originalOps, gamma1, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(originalOps.size(), 1);

    OCIO::OpRcPtrVec optimizedOps;
    Clone(optimizedOps, originalOps);

    // Optimize it.
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optimizedOps,
                                            OCIO::BIT_DEPTH_UINT16,
                                            OCIO::BIT_DEPTH_F32,
                                            OCIO::OPTIMIZATION_DEFAULT));

    // Validate the result.

    OCIO_REQUIRE_EQUAL(optimizedOps.size(), 1);

    OCIO::ConstOpRcPtr o1              = optimizedOps[0];
    OCIO::ConstLut1DOpDataRcPtr oData1 = OCIO::DynamicPtrCast<const OCIO::Lut1DOpData>(o1->data());
    OCIO_REQUIRE_ASSERT(oData1);
    OCIO_CHECK_EQUAL(oData1->getType(), OCIO::OpData::Lut1DType);
    OCIO_CHECK_EQUAL(oData1->getArray().getLength(), 65536);
    originalOps.clear();

    // However, if the input bit depth is F32, it should not be optimized.

    OCIO::GammaOpDataRcPtr gamma2
        = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_REV, 
                                              params1, params1, params1, paramsA);

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(originalOps, gamma2, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(originalOps.size(), 1);

    // Optimize it.
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(originalOps,
                                            OCIO::BIT_DEPTH_F32,
                                            OCIO::BIT_DEPTH_F32,
                                            OCIO::OPTIMIZATION_DEFAULT));

    OCIO_REQUIRE_EQUAL(originalOps.size(), 1);
    OCIO::ConstOpRcPtr o2 = originalOps[0];
    OCIO_CHECK_EQUAL(o2->data()->getType(), OCIO::OpData::GammaType);
}

OCIO_ADD_TEST(OpOptimizers, multi_op_prefix)
{
    // Test prefix optimization of a complex transform.

    OCIO::OpRcPtrVec originalOps;

    OCIO::MatrixOpDataRcPtr matrix = std::make_shared<OCIO::MatrixOpData>();
    matrix->setArrayValue(0, 2.);

    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(originalOps, matrix, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(originalOps.size(), 1);

    OCIO::RangeOpDataRcPtr range
        = std::make_shared<OCIO::RangeOpData>(0., 1., -1000./65535., 66000./65535);

    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(originalOps, range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(originalOps.size(), 2);

    OCIO::OpRcPtrVec optimizedOps;
    Clone(optimizedOps, originalOps);

    // Nothing to optimize.
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optimizedOps,
                                            OCIO::BIT_DEPTH_UINT8,
                                            OCIO::BIT_DEPTH_F32,
                                            OCIO::OPTIMIZATION_DEFAULT));

    // Validate ops are unchanged.

    OCIO_REQUIRE_EQUAL(optimizedOps.size(), 2U);

    OCIO_CHECK_NO_THROW(FinalizeOpVec(originalOps, OCIO::OPTIMIZATION_LUT_INV_FAST));
    OCIO_CHECK_NO_THROW(FinalizeOpVec(optimizedOps, OCIO::OPTIMIZATION_LUT_INV_FAST));

    OCIO_CHECK_EQUAL(std::string(originalOps[0]->getCacheID()),
                     std::string(optimizedOps[0]->getCacheID()));

    OCIO_CHECK_EQUAL(std::string(originalOps[1]->getCacheID()),
                     std::string(optimizedOps[1]->getCacheID()));

    // Add more ops to originalOps.
    const OCIO::CDLOpData::ChannelParams slope(1.35, 1.1, 0.071);
    const OCIO::CDLOpData::ChannelParams offset(0.05, -0.23, 0.11);
    const OCIO::CDLOpData::ChannelParams power(1.27, 0.81, 0.2);
    const double saturation = 1.;

    OCIO::CDLOpDataRcPtr cdl
        = std::make_shared<OCIO::CDLOpData>(OCIO::CDLOpData::CDL_V1_2_FWD, 
                                            slope, offset, power, saturation);

    OCIO_CHECK_NO_THROW(OCIO::CreateCDLOp(originalOps, cdl, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(originalOps.size(), 3);

    Clone(optimizedOps, originalOps);

    // Optimize it.
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optimizedOps,
                                            OCIO::BIT_DEPTH_UINT8,
                                            OCIO::BIT_DEPTH_F32,
                                            OCIO::OPTIMIZATION_DEFAULT));

    // Validate the result.

    OCIO_REQUIRE_EQUAL(optimizedOps.size(), 1U);

    OCIO::ConstOpRcPtr o              = optimizedOps[0];
    OCIO::ConstLut1DOpDataRcPtr oData = OCIO::DynamicPtrCast<const OCIO::Lut1DOpData>(o->data());
    OCIO_CHECK_EQUAL(oData->getType(), OCIO::OpData::Lut1DType);
    OCIO_CHECK_EQUAL(oData->getArray().getLength(), 256);

    // Although finalized for UINT8, the transform may still be evaluated at 32f to verify that
    // it is a good approximation to the original.
    CompareRender(originalOps, optimizedOps, __LINE__, 5e-5f);
}

OCIO_ADD_TEST(OpOptimizers, dyn_properties_prefix)
{
    // Test prefix optimization of a complex transform.

    OCIO::OpRcPtrVec originalOps;

    OCIO::MatrixOpDataRcPtr matrix = std::make_shared<OCIO::MatrixOpData>();
    matrix->setArrayValue(0, 2.);

    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(originalOps, matrix, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(originalOps.size(), 1);

    OCIO::ExposureContrastOpDataRcPtr exposure = std::make_shared<OCIO::ExposureContrastOpData>();

    exposure->setExposure(1.2);
    exposure->setPivot(0.5);

    OCIO_CHECK_NO_THROW(
        OCIO::CreateExposureContrastOp(originalOps, exposure, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(originalOps.size(), 2);

    exposure = exposure->clone();
    exposure->getExposureProperty()->makeDynamic();

    OCIO_CHECK_NO_THROW(
        OCIO::CreateExposureContrastOp(originalOps, exposure, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(originalOps.size(), 3);

    OCIO::OpRcPtrVec optimizedOps;
    Clone(optimizedOps, originalOps);

    // Optimize it.
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optimizedOps,
                                            OCIO::BIT_DEPTH_UINT8,
                                            OCIO::BIT_DEPTH_F32,
                                            OCIO::OPTIMIZATION_DEFAULT));

    // Validate the result.

    OCIO_REQUIRE_EQUAL(optimizedOps.size(), 2U);

    OCIO::ConstOpRcPtr o              = optimizedOps[0];
    OCIO::ConstLut1DOpDataRcPtr oData = OCIO::DynamicPtrCast<const OCIO::Lut1DOpData>(o->data());
    OCIO_CHECK_ASSERT(oData);
    OCIO_CHECK_EQUAL(oData->getType(), OCIO::OpData::Lut1DType);
    OCIO_CHECK_EQUAL(oData->getArray().getLength(), 256);
    OCIO_CHECK_CLOSE(oData->getArray()[255 * 3 + 0], 4.5947948f, 1.e-6f);
    OCIO_CHECK_CLOSE(oData->getArray()[255 * 3 + 1], 2.2973969f, 1.e-6f);
    OCIO_CHECK_CLOSE(oData->getArray()[255 * 3 + 2], 2.2973969f, 1.e-6f);

    o = optimizedOps[1];
    OCIO::ConstExposureContrastOpDataRcPtr exp =
        OCIO::DynamicPtrCast<const OCIO::ExposureContrastOpData>(o->data());

    OCIO_CHECK_ASSERT(exp);
    OCIO_CHECK_EQUAL(exp->getType(), OCIO::OpData::ExposureContrastType);
    OCIO_CHECK_ASSERT(exp->isDynamic());
}

OCIO_ADD_TEST(OpOptimizers, opt_prefix_test1)
{
    const std::string fileName("opt_prefix_test1.ctf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, fileName, context,
                                           OCIO::TRANSFORM_DIR_FORWARD));

    // First one is the file no op.
    OCIO_CHECK_EQUAL(ops.size(), 12);

    OCIO_CHECK_NO_THROW(OCIO::RemoveNoOpTypes(ops));

    OCIO_CHECK_EQUAL(ops.size(), 11);

    OCIO::OpRcPtrVec optOps;
    Clone(optOps, ops);
    OCIO_CHECK_EQUAL(optOps.size(), 11);
    // Ignore dynamic properties.
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(optOps, OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_F32,
                                            OCIO::OPTIMIZATION_ALL));

    OCIO_REQUIRE_EQUAL(optOps.size(), 3);

    OCIO::ConstOpRcPtr o0 = optOps[0];
    OCIO::ConstOpRcPtr o1 = optOps[1];
    OCIO::ConstOpRcPtr o2 = optOps[2];

    OCIO_CHECK_EQUAL(o0->data()->getType(), OCIO::OpData::Lut1DType);
    OCIO_CHECK_EQUAL(o1->data()->getType(), OCIO::OpData::MatrixType);
    OCIO_CHECK_EQUAL(o2->data()->getType(), OCIO::OpData::GammaType);

    auto lut0 = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Lut1DOpData>(o0->data());
    OCIO_CHECK_ASSERT(!lut0->isIdentity());
    OCIO_CHECK_EQUAL(lut0->getArray().getLength(), 65536u);
}

#endif // OCIO_UNIT_TEST

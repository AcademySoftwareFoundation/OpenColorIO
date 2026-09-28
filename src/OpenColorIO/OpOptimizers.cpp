// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "Logging.h"
#include "Op.h"
#include "ops/lut1d/Lut1DOp.h"
#include "ops/lut3d/Lut3DOp.h"
#include "ops/range/RangeOp.h"

namespace OCIO_NAMESPACE
{

namespace
{

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

    case OpData::GradingPrimaryType:
    case OpData::GradingRGBCurveType:
    case OpData::GradingHueCurveType:
    case OpData::GradingToneType:
        return HasFlag(flags, OPTIMIZATION_PAIR_IDENTITY_GRADING);

    case OpData::ExponentType:
    case OpData::MatrixType:
    case OpData::RangeType:
        return false; // Use composition to optimize.

    case OpData::ReferenceType:
    case OpData::NoOpType:
    default:
        // Other types are not controlled by a flag.
        return true;
    }
}

bool IsCombineEnabled(OpData::Type type, OptimizationFlags flags)
{
    // Some types are controlled by a flag.
    return  (type == OpData::ExponentType && HasFlag(flags, OPTIMIZATION_COMP_EXPONENT)) ||
            (type == OpData::GammaType    && HasFlag(flags, OPTIMIZATION_COMP_GAMMA))    ||
            (type == OpData::Lut1DType    && HasFlag(flags, OPTIMIZATION_COMP_LUT1D))    ||
            (type == OpData::Lut3DType    && HasFlag(flags, OPTIMIZATION_COMP_LUT3D))    ||
            (type == OpData::MatrixType   && HasFlag(flags, OPTIMIZATION_COMP_MATRIX))   ||
            (type == OpData::RangeType    && HasFlag(flags, OPTIMIZATION_COMP_RANGE));
}

constexpr int MAX_OPTIMIZATION_PASSES = 80;

size_t RemoveNoOpTypes(OpRcPtrVec & opVec, [[maybe_unused]] OptimizationFlags flags)
{
    // TODO: with c++20 could use std::erase_if ?
    auto newEnd = std::remove_if(opVec.begin(), opVec.end(), [](const auto & o) {
        return o->isNoOpType();
    });
    
    size_t count = std::distance(newEnd, opVec.end());
    opVec.erase(newEnd, opVec.end());
    return count;
}

// Ops are preserved, dynamic properties are made non-dynamic.
size_t RemoveDynamicProperties(OpRcPtrVec & opVec, OptimizationFlags oFlags)
{
    size_t count = 0;
    const auto removeDynamic = HasFlag(oFlags, OPTIMIZATION_NO_DYNAMIC_PROPERTIES);
    if (!removeDynamic)
    {
        return count;
    }

    std::for_each(opVec.begin(), opVec.end(), [&count](auto & op) {
        if (op->isDynamic())
        {
            // Optimization flag is tested before.
            auto replacedBy = op->clone();
            replacedBy->removeDynamicProperties();
            op = std::move(replacedBy);
            ++count;
        }
    });
    return count;
}

size_t RemoveNoOps(OpRcPtrVec & opVec, OptimizationFlags oFlags)
{
    const bool optimizeIdentity = HasFlag(oFlags, OPTIMIZATION_IDENTITY);
    if (!optimizeIdentity)
    {
        return 0;
    }

    // TODO: with c++20 could use std::erase_if ?
    auto newEnd = std::remove_if(opVec.begin(), opVec.end(), [](const auto& op)
    {
        return op->isNoOp();
    });

    size_t count = std::distance(newEnd, opVec.end());
    opVec.erase(newEnd, opVec.end());
    return count;
}

void FinalizeOps(OpRcPtrVec const & opVec)
{
    for (const auto &op : opVec)
    {
        // Prepare LUT 1D for inversion and ensure Matrix & Range are forward.
        op->finalize();
    }
}

// Some rather complex ops can get replaced based on their data by simpler ops.
// For instance CDL that does not use power will get replaced.
size_t ReplaceOps(OpRcPtrVec & opVec, [[maybe_unused]] OptimizationFlags oFlags)
{
    size_t count = 0;
    const bool replaceOps = HasFlag(oFlags, OPTIMIZATION_SIMPLIFY_OPS);
    if (!replaceOps)
    {
        return count;
    }


    OpRcPtrVec tmpops;
    OpRcPtrVec newOpVec;
    bool rebuilding = false;

    for (size_t i = 0; i < opVec.size(); ++i)
    {
        tmpops.clear();
        opVec[i]->getSimplerReplacement(tmpops);

        if (!tmpops.empty())
        {
            if (!rebuilding)
            {
                rebuilding = true;
                // Defer allocation until the first replacement to prevent
                // large, unnecessary heap allocations on no-op optimizer passes.
                newOpVec.reserve(opVec.size());
                
                // Catch up any unmodified elements prior to this replacement
                for (size_t j = 0; j < i; ++j)
                {
                    newOpVec.emplace_back(std::move(opVec[j]));
                }
            }

            FinalizeOps(tmpops);
            for (auto & newOp : tmpops)
            {
                newOpVec.emplace_back(std::move(newOp));
            }
            ++count;
        }
        else
        {
            if (rebuilding)
            {
                newOpVec.emplace_back(std::move(opVec[i]));
            }
        }
        
    }

    if (rebuilding)
    {
        opVec = std::move(newOpVec);
    }

    return count;
}

size_t ReplaceIdentityOps(OpRcPtrVec & opVec, OptimizationFlags oFlags)
{
    size_t count = 0;

    // Remove any identity ops (other than gamma).
    const bool optIdentity = HasFlag(oFlags, OPTIMIZATION_IDENTITY);
    // Remove identity gamma ops (handled separately to give control over negative
    // alpha clamping).
    const bool optIdGamma = HasFlag(oFlags, OPTIMIZATION_IDENTITY_GAMMA);
    if (!optIdentity && !optIdGamma)
    {
        return count;
    }

    for (auto & op : opVec)
    {
        const auto type = std::as_const(*op).data()->getType();
        if (type != OpData::RangeType && // Do not replace a range identity.
            ((type == OpData::GammaType && optIdGamma) ||
                (type != OpData::GammaType && optIdentity)) &&
            op->isIdentity())
        {
            // Optimization flag is tested before.
            auto replacedBy = op->getIdentityReplacement();
            replacedBy->finalize();
            op = std::move(replacedBy);
            ++count;
        }
    }
    return count;
}

size_t RemoveInverseOps(OpRcPtrVec & opVec, OptimizationFlags oFlags)
{
    size_t count      = 0;
    size_t writeIdx   = 0;
    const size_t numOps = opVec.size();

    for (size_t readIdx = 0; readIdx < numOps; ++readIdx)
    {
        auto & op = opVec[readIdx];

        if (writeIdx > 0)
        {
            ConstOpRcPtr lastOp = opVec[writeIdx - 1];
            ConstOpRcPtr constOp = op;
            const auto type1 = lastOp->data()->getType();
            const auto type2 = constOp->data()->getType();

            // The common case of inverse ops is to have a deep nesting:
            // ..., A, B, B', A', ...
            //
            // By treating the processed portion of the vector as a stack (`writeIdx`), 
            // popping an element automatically exposes `A` to be reconsidered against `A'` 
            // on the next loop iteration.

            if (type1 == type2 &&
                IsPairInverseEnabled(type1, oFlags) &&
                lastOp->isInverse(constOp))
            {
                // When a pair of inverse ops is removed, we want the optimized ops to give the
                // same result as the original.  For certain ops such as Lut1D or Log this may
                // mean inserting a Range to emulate the clamping done by the original ops.

                OpRcPtr replacedBy;
                if (type1 == OpData::Lut1DType)
                {
                    // Lut1D gets special handling so that both halfs of the pair are available.
                    // Only the inverse LUT has the values needed to generate the replacement.

                    ConstLut1DOpDataRcPtr lut1 = OCIO_DYNAMIC_POINTER_CAST<const Lut1DOpData>(lastOp->data());
                    ConstLut1DOpDataRcPtr lut2 = OCIO_DYNAMIC_POINTER_CAST<const Lut1DOpData>(constOp->data());

                    OpDataRcPtr opData = lut1->getPairIdentityReplacement(lut2);

                    OpRcPtrVec ops;
                    if (opData->getType() == OpData::MatrixType)
                    {
                        // No-op that will be optimized.
                        auto mat = OCIO_DYNAMIC_POINTER_CAST<MatrixOpData>(opData);
                        CreateMatrixOp(ops, mat, TRANSFORM_DIR_FORWARD);
                    }
                    else if (opData->getType() == OpData::RangeType)
                    {
                        // Clamping op.
                        auto range = OCIO_DYNAMIC_POINTER_CAST<RangeOpData>(opData);
                        CreateRangeOp(ops, range, TRANSFORM_DIR_FORWARD);
                    }
                    replacedBy = std::move(ops[0]);
                }
                else
                {
                    replacedBy = lastOp->getIdentityReplacement();
                }

                replacedBy->finalize();
                if (replacedBy->isNoOp())
                {
                    // Pop the last element off the stack to naturally backstep
                    --writeIdx;
                }
                else
                {
                    // Forward + inverse does clamp.
                    opVec[writeIdx - 1] = std::move(replacedBy);
                }
                ++count;
                continue;
            }
        }

        // Push the current op to the stack if it wasn't cancelled out
        if (writeIdx != readIdx)
        {
            opVec[writeIdx] = std::move(op);
        }
        ++writeIdx;
    }

    if (count > 0)
    {
        // Drop any unused elements at the tail in a single O(N) pass
        opVec.erase(opVec.begin() + writeIdx, opVec.end());
    }

    return count;
}

size_t CombineOps(OpRcPtrVec & opVec, OptimizationFlags oFlags)
{
    auto it = std::adjacent_find(opVec.begin(), opVec.end(),
        [oFlags](const auto & ptr1, const auto & ptr2) {
            ConstOpRcPtr op2 = ptr2;
            return IsCombineEnabled(std::as_const(*ptr1).data()->getType(), oFlags) && ptr1->canCombineWith(op2);
        });

    if (it != opVec.end())
    {
        OpRcPtrVec tmpops;
        ConstOpRcPtr op2 = *(it + 1);
        
        (*it)->combineWith(tmpops, op2);
        FinalizeOps(tmpops);

        // The tmpops may have any number of ops in it: (0, 1, 2, ...).
        // (Size 0 would occur only if the combination results in a no-op,
        //  for example, a pair of matrices that compose into a no-op are
        //  returned as empty rather than as an identity matrix.)
        //
        // No matter the number, we need to swap them in for the original ops.

        const size_t numNewOps = tmpops.size();
        if (numNewOps == 0)
        {
            opVec.erase(it, it + 2);
        }
        else if (numNewOps == 1)
        {
            *it = std::move(tmpops[0]);
            opVec.erase(it + 1);
        }
        else if (numNewOps == 2)
        {
            *it = std::move(tmpops[0]);
            *(it + 1) = std::move(tmpops[1]);
        }
        else
        {
            *it = std::move(tmpops[0]);
            *(it + 1) = std::move(tmpops[1]);
            opVec.insert(it + 2, 
                         std::make_move_iterator(tmpops.begin() + 2), 
                         std::make_move_iterator(tmpops.end()));        }


        // Return 1 since combining ops is less desirable than other optimization options.
        // For example, it is preferable to remove a pair of ops using RemoveInverseOps
        // rather than combining them. Consider this example:
        // Lut1D A --> Matrix B --> Matrix C --> Lut1D Ainv
        // If Matrix B & C are not pair inverses but do combine into an identity, then
        // CombineOps would compose Lut1D A & Ainv, into a new Lut1D rather than
        // allowing another round of optimization which would remove them as inverses.
        return 1;
    }

    return 0;
}

// Replace any Lut1D or Lut3D that specify inverse evaluation with a faster forward approximation.
// There are two inversion modes: EXACT and FAST. The EXACT method is slower, and only available
// on the CPU, but it calculates an exact inverse. The exact inverse is based on the use of LINEAR
// forward interpolation for Lut1D and TETRAHEDRAL forward interpolation for Lut3D. The FAST method
// bakes the inverse into another forward LUT (using the exact method). For Lut1D, a half-domain
// LUT is used and so this is quite accurate even for scene-linear values, but for Lut3D the baked
// version is more of an approximation. The default optimization level uses the FAST method since
// it is the only one available on both CPU and GPU.
size_t ReplaceInverseLuts(OpRcPtrVec & opVec, OptimizationFlags oFlags)
{
    size_t count = 0;
    const bool fastLut = HasFlag(oFlags, OPTIMIZATION_LUT_INV_FAST);
    if (!fastLut)
    {
        return count;
    }

    OpRcPtrVec tmpops;

    for (auto & op : opVec)
    {
        auto opData = std::as_const(*op).data();
        const auto type = opData->getType();
        if (type == OpData::Lut1DType)
        {
            auto lutData = OCIO_DYNAMIC_POINTER_CAST<const Lut1DOpData>(opData);
            if (lutData->getDirection() == TRANSFORM_DIR_INVERSE)
            {
                auto invLutData = MakeFastLut1DFromInverse(lutData);
                tmpops.clear();
                CreateLut1DOp(tmpops, invLutData, TRANSFORM_DIR_FORWARD);
                FinalizeOps(tmpops);
                op = std::move(tmpops[0]);
                ++count;
            }
        }
        else if (type == OpData::Lut3DType)
        {
            auto lutData = OCIO_DYNAMIC_POINTER_CAST<const Lut3DOpData>(opData);
            if (lutData->getDirection() == TRANSFORM_DIR_INVERSE)
            {
                auto invLutData = MakeFastLut3DFromInverse(lutData);
                tmpops.clear();
                CreateLut3DOp(tmpops, invLutData, TRANSFORM_DIR_FORWARD);
                FinalizeOps(tmpops);
                op = std::move(tmpops[0]);
                ++count;
            }
        }
    }
    return count;
}

size_t RemoveLeadingClampIdentity(OpRcPtrVec & opVec)
{
    auto it = std::find_if_not(opVec.begin(), opVec.end(), [](const auto & op) {
        auto oData = std::as_const(*op).data();
        return oData->getType() == OpData::RangeType && oData->isIdentity();
    });

    size_t count = std::distance(opVec.begin(), it);
    if (count > 0)
    {
        opVec.erase(opVec.begin(), it);
    }
    return count;
}

size_t RemoveTrailingClampIdentity(OpRcPtrVec & opVec)
{
    // Note the use of reverse iterators
    auto rit = std::find_if_not(opVec.rbegin(), opVec.rend(), [](const auto & op) {
        auto oData = std::as_const(*op).data();
        return oData->getType() == OpData::RangeType && oData->isIdentity();
    });

    size_t count = std::distance(opVec.rbegin(), rit);
    if (count > 0)
    {
        opVec.erase(rit.base(), opVec.end());
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
size_t FindSeparablePrefix(const OpRcPtrVec & ops)
{
    // Loop over the ops until we get to one that cannot be combined.
    //
    // Note: For some ops such as Matrix and CDL, the separability depends upon
    //       the parameters.
    auto it = std::find_if(ops.begin(), ops.end(), [](const auto & op) {
        // In OCIO, the hasChannelCrosstalk method returns false for separable ops.
        return op->hasChannelCrosstalk() || op->isDynamic();
    });

    size_t prefixLen = std::distance(ops.begin(), it);

    // If the only op is a 1D LUT, there is actually nothing to optimize
    // so set the length to 0.  (This also avoids an infinite loop.)
    // (If it is an inverse 1D LUT, proceed since we want to replace it with a 1D LUT.)
    if (prefixLen == 1)
    {
        auto opData = std::as_const(*ops[0]).data();
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
    bool hasExpensiveOps = std::any_of(ops.begin(), ops.begin() + prefixLen, [](const auto & op) {
        if (op->hasChannelCrosstalk())
        {
            // Non-separable ops (should never get here).
            throw Exception("Non-separable op.");
        }

        const auto type = std::as_const(*op).data()->getType();
        
        // Potentially separable, but inexpensive ops are not counted.
        // TODO: Perhaps a LUT is faster once the conversion to float is considered?

        return type != OpData::MatrixType && type != OpData::RangeType;
    });

    if (!hasExpensiveOps)
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

    const auto prefixLen = FindSeparablePrefix(ops);
    if (prefixLen == 0)
    {
        return; // Nothing to do.
    }

    OpRcPtrVec prefixOps;
    prefixOps.reserve(prefixLen);
    for (size_t i = 0; i < prefixLen; ++i)
    {
        prefixOps.emplace_back(ops[i]->clone());
    }

    // Make a domain for the LUT.  (Will be half-domain for target == 16f.)
    Lut1DOpDataRcPtr newDomain = Lut1DOpData::MakeLookupDomain(in);

    // Send the domain through the prefix ops.
    // Note: This sets the outBitDepth of newDomain to match prefixOps.
    Lut1DOpData::ComposeVec(newDomain, prefixOps);

    // Insert the new LUT to replace the prefix ops.
    OpRcPtrVec lutOps;
    lutOps.reserve(1);
    CreateLut1DOp(lutOps, newDomain, TRANSFORM_DIR_FORWARD);
    FinalizeOps(lutOps);

    const auto numNewOps = lutOps.size();
    const auto elementsToOverwrite = std::min(numNewOps, prefixLen);

    for (size_t i = 0; i < elementsToOverwrite; ++i)
    {
        ops[i] = std::move(lutOps[i]);
    }

    if (numNewOps < prefixLen)
    {
        ops.erase(ops.begin() + numNewOps, ops.begin() + prefixLen);
    }
    else if (numNewOps > prefixLen)
    {
        ops.insert(ops.begin() + prefixLen, 
                   std::make_move_iterator(lutOps.begin() + prefixLen), 
                   std::make_move_iterator(lutOps.end()));
    }
}

size_t PerformOptimisation(size_t (*Operation)(OpRcPtrVec &, OptimizationFlags), OpRcPtrVec & opVec, OptimizationFlags oFlags, bool debugLoggingEnabled, std::string_view operationName)
{
    const size_t ops_removed = Operation(opVec, oFlags);
    if (debugLoggingEnabled)
    {
        std::ostringstream os;
        os << operationName << " - " << ops_removed << " optimisations found\n";
        if (ops_removed != 0)
        {
            os << SerializeOpVec(opVec, 4);
        }
        LogDebug(os.str());
    }
    return ops_removed;
}

} // namespace

void OpRcPtrVec::finalize()
{
    if (m_ops.empty())
    {
        return;
    }

    validate();

    // Prepare LUT 1D for inversion and ensure Matrix & Range are forward.
    FinalizeOps(*this);
}

void OpRcPtrVec::optimize(OptimizationFlags oFlags)
{
    if (m_ops.empty())
    {
        return;
    }

    const bool debugLoggingEnabled = IsDebugLoggingEnabled();
    if (debugLoggingEnabled)
    {
        const std::string message =  "\n**\nOptimizing Op Vec...\n" + SerializeOpVec(*this, 4);
        LogDebug(message);
    }

    const auto originalSize = size();
    size_t total_noops         = 0;
    size_t total_replacedops   = 0;
    size_t total_identityops   = 0;
    size_t total_inverseops    = 0;
    size_t total_combines      = 0;
    size_t total_inverses      = 0;
    int    passes              = 1;

    // NoOpType can be removed (facilitates conversion to a CPU/GPUProcessor).
    const auto total_nooptype = PerformOptimisation(RemoveNoOpTypes, *this, oFlags, debugLoggingEnabled, "RemoveNoOpTypes");

    if (oFlags == OPTIMIZATION_NONE)
    {
        if (debugLoggingEnabled)
        {
            const auto finalSize = size();

            std::ostringstream os;
            os << "**\nOptimized " << originalSize << "->" << finalSize << ", " << passes << " pass, "
               << total_nooptype << " no-op types removed\n"
               << SerializeOpVec(*this, 4);
            LogDebug(os.str());
        }

        return;
    }

    // Keep dynamic ops using their default values. Remove the ability to modify
    // them dynamically.
    const auto total_dynamicOps = PerformOptimisation(RemoveDynamicProperties, *this, oFlags, debugLoggingEnabled, "RemoveDynamicProperties");

    while (passes <= MAX_OPTIMIZATION_PASSES)
    {
        if (debugLoggingEnabled)
        {
            const auto message = "Starting pass " + std::to_string(passes);
            LogDebug(message);
        }
        // Remove all ops for which isNoOp is true, including identity matrices.
        const auto noops = PerformOptimisation(RemoveNoOps, *this, oFlags, debugLoggingEnabled, "RemoveNoOps");
        total_noops += noops;

        // Replace all complex ops with simpler ops (e.g., a CDL which only scales with a matrix).
        // Note this might increase the number of ops.
        const auto replacedOps = PerformOptimisation(ReplaceOps, *this, oFlags, debugLoggingEnabled, "ReplaceOps");
        total_replacedops += replacedOps;

        // Replace all complex identities with simpler ops (e.g., an identity Lut1D with a range).
        const auto identityops = PerformOptimisation(ReplaceIdentityOps, *this, oFlags, debugLoggingEnabled, "ReplaceIdentityOps");
        total_identityops += identityops;

        // Remove all adjacent pairs of ops that are inverses of each other.
        const auto inverseops = PerformOptimisation(RemoveInverseOps, *this, oFlags, debugLoggingEnabled, "RemoveInverseOps");
        total_inverseops += inverseops;

        // Combine a pair of ops, for example multiply two adjacent Matrix ops.
        // (Combines at most one pair on each iteration.)
        const auto combines = PerformOptimisation(CombineOps, *this, oFlags, debugLoggingEnabled, "CombineOps");
        total_combines += combines;

        if (noops + replacedOps + identityops + inverseops + combines == 0)
        {
            // No optimization progress was made, so stop trying.  If requested, replace any
            // inverse LUTs with faster forward LUTs and do another pass to see if more
            // optimization is possible.
            const auto inverses = PerformOptimisation(ReplaceInverseLuts, *this, oFlags, debugLoggingEnabled, "ReplaceInverseLuts");
            total_inverses += inverses;

            if (inverses == 0)
            {
                break;
            }
        }

        if (debugLoggingEnabled)
        {
            std::ostringstream os;

            os << "Pass " << passes << " summary: "
                          << noops << " no-op removed, "
                          << replacedOps << " ops replaced, "
                          << identityops << " identity ops replaced, "
                          << inverseops << " inverse op pairs removed, "
                          << combines << " ops combined.";
            LogDebug(os.str());
        }
        ++passes;
    }

    if (debugLoggingEnabled && (passes == MAX_OPTIMIZATION_PASSES))
    {
        std::ostringstream os;
        os << "The max number of passes, " << passes << ", "
              "was reached during optimization. This is likely a sign "
              "that either the complexity of the color transform is "
              "very high, or that some internal optimizers are in conflict "
              "(undo-ing / redo-ing the other's results).";
        LogDebug(os.str());
    }

    if (debugLoggingEnabled)
    {
        const auto finalSize = size();

        std::ostringstream os;
        os << "**\nOptimized "
           << originalSize << "->" << finalSize << ", "
           << passes << " passes, "
           << total_nooptype << " no-op types removed, "
           << total_dynamicOps << " dynamic-ops made static, "
           << total_noops << " no-ops removed, "
           << total_replacedops << " ops replaced, "
           << total_identityops << " identity ops replaced, "
           << total_inverseops << " inverse op pairs removed, "
           << total_combines << " ops combined, "
           << total_inverses << " ops inverted\n"
           << SerializeOpVec(*this, 4);
        LogDebug(os.str());
    }
}

void OpRcPtrVec::optimizeForBitdepth(const BitDepth & inBitDepth,
                                     const BitDepth & outBitDepth,
                                     OptimizationFlags oFlags)
{
    if (!empty())
    {
        if (!IsFloatBitDepth(inBitDepth))
        {
            RemoveLeadingClampIdentity(*this);
        }
        if (!IsFloatBitDepth(outBitDepth))
        {
            RemoveTrailingClampIdentity(*this);
        }
        if (HasFlag(oFlags, OPTIMIZATION_COMP_SEPARABLE_PREFIX))
        {
            OptimizeSeparablePrefix(*this, inBitDepth);
        }
    }
}

} // namespace OCIO_NAMESPACE

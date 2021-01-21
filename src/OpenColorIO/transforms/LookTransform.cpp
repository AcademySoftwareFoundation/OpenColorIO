// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include <algorithm>
#include <iterator>

#include "ContextVariableUtils.h"
#include "LookParse.h"
#include "ops/noop/NoOps.h"
#include "OpBuilders.h"
#include "ParseUtils.h"


namespace OCIO_NAMESPACE
{
LookTransformRcPtr LookTransform::Create()
{
    return LookTransformRcPtr(new LookTransform(), &deleter);
}

void LookTransform::deleter(LookTransform* t)
{
    delete t;
}

class LookTransform::Impl
{
public:
    TransformDirection m_dir{ TRANSFORM_DIR_FORWARD };
    bool m_skipColorSpaceConversion{ false };
    std::string m_src;
    std::string m_dst;
    std::string m_looks;

    Impl() = default;
    Impl(const Impl &) = delete;
    ~Impl() = default;

    Impl& operator= (const Impl & rhs)
    {
        if (this != &rhs)
        {
            m_dir                      = rhs.m_dir;
            m_src                      = rhs.m_src;
            m_dst                      = rhs.m_dst;
            m_looks                    = rhs.m_looks;
            m_skipColorSpaceConversion = rhs.m_skipColorSpaceConversion;
        }
        return *this;
    }
};

///////////////////////////////////////////////////////////////////////////

LookTransform::LookTransform()
    : m_impl(new LookTransform::Impl)
{
}

TransformRcPtr LookTransform::createEditableCopy() const
{
    LookTransformRcPtr transform = LookTransform::Create();
    *(transform->m_impl) = *m_impl;
    return transform;
}

LookTransform::~LookTransform()
{
    delete m_impl;
    m_impl = nullptr;
}

TransformDirection LookTransform::getDirection() const noexcept
{
    return getImpl()->m_dir;
}

void LookTransform::setDirection(TransformDirection dir) noexcept
{
    getImpl()->m_dir = dir;
}

void LookTransform::validate() const
{
    try
    {
        Transform::validate();
    }
    catch (Exception & ex)
    {
        std::string errMsg("LookTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }

    if (getImpl()->m_src.empty())
    {
        throw Exception("LookTransform: empty source color space name.");
    }

    if (getImpl()->m_dst.empty())
    {
        throw Exception("LookTransform: empty destination color space name.");
    }
}

const char * LookTransform::getSrc() const
{
    return getImpl()->m_src.c_str();
}

void LookTransform::setSrc(const char * src)
{
    getImpl()->m_src = src ? src : "";
}

const char * LookTransform::getDst() const
{
    return getImpl()->m_dst.c_str();
}

void LookTransform::setDst(const char * dst)
{
    getImpl()->m_dst = dst ? dst : "";
}

void LookTransform::setLooks(const char * looks)
{
    getImpl()->m_looks = looks ? looks : "";
}

const char * LookTransform::getLooks() const
{
    return getImpl()->m_looks.c_str();
}

void LookTransform::setSkipColorSpaceConversion(bool skip)
{
    getImpl()->m_skipColorSpaceConversion = skip;
}

bool LookTransform::getSkipColorSpaceConversion() const
{
    return getImpl()->m_skipColorSpaceConversion;
}

const char * LooksResultColorSpace(const Config & config,
                                   const ConstContextRcPtr & context,
                                   const LookParseResult & looks)
{
    if (!looks.empty())
    {
        // Apply looks in forward direction to update the source space.
        // Note that we cannot simply take the process space of the last look, since one of the
        // look fall-backs may be used, so the look tokens need to be run.
        ConstColorSpaceRcPtr currentColorSpace;
        OpRcPtrVec tmp;
        BuildLookOps(tmp, currentColorSpace, false, config, context, looks);
        if (currentColorSpace)
        {
            return currentColorSpace->getName();
        }
    }
    return "";
}

const char * LookTransform::GetLooksResultColorSpace(const ConstConfigRcPtr & config,
                                                     const ConstContextRcPtr & context,
                                                     const char * looksStr)
{
    if (!looksStr || !*looksStr) return "";

    LookParseResult looks;
    looks.parse(looksStr);

    return LooksResultColorSpace(*config, context, looks);
}

std::ostream& operator<< (std::ostream& os, const LookTransform& t)
{
    os << "<LookTransform";
    os <<  " direction=" << TransformDirectionToString(t.getDirection());
    os << ", src="       << t.getSrc();
    os << ", dst="       << t.getDst();
    os << ", looks="     << t.getLooks();
    if (t.getSkipColorSpaceConversion()) os << ", skipCSConversion";
    os << ">";
    return os;
}

////////////////////////////////////////////////////////////////////////////

namespace
{

void RunLookTokens(OpRcPtrVec & ops,
                   ConstColorSpaceRcPtr & currentColorSpace,
                   bool skipColorSpaceConversion,
                   const Config& config,
                   const ConstContextRcPtr & context,
                   const LookParseResult::Tokens & lookTokens)
{
    if(lookTokens.empty()) return;

    for(unsigned int i=0; i<lookTokens.size(); ++i)
    {
        const std::string & lookName = lookTokens[i].name;

        if(lookName.empty()) continue;

        ConstLookRcPtr look = config.getLook(lookName.c_str());
        if(!look)
        {
            std::ostringstream os;
            os << "RunLookTokens error. ";
            os << "The specified look, '" << lookName;
            os << "', cannot be found. ";
            if(config.getNumLooks() == 0)
            {
                os << " (No looks defined in config).";
            }
            else
            {
                os << " (looks: ";
                for(int ii=0; ii<config.getNumLooks(); ++ii)
                {
                    if(ii != 0) os << ", ";
                    os << config.getLookNameByIndex(ii);
                }
                os << ").";
            }

            throw Exception(os.str().c_str());
        }

        // Put the new ops into a temp array, to see if it's a no-op
        // If it is a no-op, dont bother doing the colorspace conversion.
        OpRcPtrVec tmpOps;

        switch (lookTokens[i].dir)
        {
        case TRANSFORM_DIR_FORWARD:
        {
            CreateLookNoOp(tmpOps, lookName);
            if (look->getTransform())
            {
                BuildOps(tmpOps, config, context, look->getTransform(), TRANSFORM_DIR_FORWARD);
            }
            else if (look->getInverseTransform())
            {
                BuildOps(tmpOps, config, context, look->getInverseTransform(), TRANSFORM_DIR_INVERSE);
            }
            break;
        }
        case TRANSFORM_DIR_INVERSE:
        {
            CreateLookNoOp(tmpOps, std::string("-") + lookName);
            if (look->getInverseTransform())
            {
                BuildOps(tmpOps, config, context, look->getInverseTransform(), TRANSFORM_DIR_FORWARD);
            }
            else if (look->getTransform())
            {
                BuildOps(tmpOps, config, context, look->getTransform(), TRANSFORM_DIR_INVERSE);
            }
            break;
        }
        }

        if (!tmpOps.isNoOp())
        {
            ConstColorSpaceRcPtr processColorSpace = config.getColorSpace(look->getProcessSpace());
            if(!processColorSpace)
            {
                std::ostringstream os;
                os << "RunLookTokens error. ";
                os << "The specified look, '" << lookTokens[i].name;
                os << "', requires processing in the ColorSpace, '";
                os << look->getProcessSpace() << "' which is not defined.";
                throw Exception(os.str().c_str());
            }
            if (!currentColorSpace)
            {
                currentColorSpace = processColorSpace;
            }
            // If current color space is already the process space skip the conversion.
            if (!skipColorSpaceConversion &&
                currentColorSpace.get() != processColorSpace.get())
            {
                // Default behavior is to bypass data color space.
                BuildColorSpaceOps(ops, config, context,
                                   currentColorSpace, processColorSpace, true);
                currentColorSpace = processColorSpace;
            }

            ops += tmpOps;
        }
    }
}

} // anon namespace

////////////////////////////////////////////////////////////////////////////

void BuildLookOps(OpRcPtrVec & ops,
                  const Config & config,
                  const ConstContextRcPtr & context,
                  const LookTransform & lookTransform,
                  TransformDirection dir)
{
    ConstColorSpaceRcPtr src = config.getColorSpace(lookTransform.getSrc());
    if(!src)
    {
        std::ostringstream os;
        os << "BuildLookOps error.";
        os << "The specified lookTransform specifies a src colorspace, '";
        os <<  lookTransform.getSrc() << "', which is not defined.";
        throw Exception(os.str().c_str());
    }

    ConstColorSpaceRcPtr dst = config.getColorSpace(lookTransform.getDst());
    if(!dst)
    {
        std::ostringstream os;
        os << "BuildLookOps error.";
        os << "The specified lookTransform specifies a dst colorspace, '";
        os <<  lookTransform.getDst() << "', which is not defined.";
        throw Exception(os.str().c_str());
    }

    LookParseResult looks;
    looks.parse(lookTransform.getLooks());

    // The code must handle the inverse src/dst colorspace transformation explicitly.
    const auto combinedDir = CombineTransformDirections(dir, lookTransform.getDirection());
    if(combinedDir == TRANSFORM_DIR_INVERSE)
    {
        std::swap(src, dst);
        looks.reverse();
    }

    const bool skipColorSpaceConversion = lookTransform.getSkipColorSpaceConversion();
    ConstColorSpaceRcPtr currentColorSpace = src;
    BuildLookOps(ops,
                 currentColorSpace,
                 skipColorSpaceConversion,
                 config,
                 context,
                 looks);

    // If current color space is already the dst space skip the conversion.
    if (!skipColorSpaceConversion && currentColorSpace.get() != dst.get())
    {
        // Default behavior is to bypass data color space.
        BuildColorSpaceOps(ops, config, context, currentColorSpace, dst, true);
    }
}

void BuildLookOps(OpRcPtrVec & ops,
                  ConstColorSpaceRcPtr & currentColorSpace, // in-out
                  bool skipColorSpaceConversion,
                  const Config & config,
                  const ConstContextRcPtr & context,
                  const LookParseResult & looks)
{
    const LookParseResult::Options & options = looks.getOptions();

    if(options.empty())
    {
        // Do nothing
    }
    else if(options.size() == 1)
    {
        // As an optimization, if we only have a single look option,
        // just push back onto the final location
        RunLookTokens(ops,
                      currentColorSpace,
                      skipColorSpaceConversion,
                      config,
                      context,
                      options[0]);
    }
    else
    {
        // If we have multiple look options, try each one in order,
        // and if we can create the ops without a missing file exception,
        // push back it's results and return.

        bool success = false;
        std::ostringstream os;

        OpRcPtrVec tmpOps;
        ConstColorSpaceRcPtr cs;

        for(unsigned int i=0; i<options.size(); ++i)
        {
            cs = currentColorSpace;
            tmpOps.clear();

            try
            {
                RunLookTokens(tmpOps,
                              cs,
                              skipColorSpaceConversion,
                              config,
                              context,
                              options[i]);
                success = true;
                break;
            }
            catch(ExceptionMissingFile & e)
            {
                if(i != 0) os << "  ...  ";

                os << "(";
                LookParseResult::serialize(os, options[i]);
                os << ") " << e.what();
            }
        }

        if(success)
        {
            currentColorSpace = cs;
            ops += tmpOps;
        }
        else
        {
            throw ExceptionMissingFile(os.str().c_str());
        }
    }
}

bool CollectContextVariables(const Config & config, 
                             const Context & context,
                             const LookTransform & look,
                             ContextRcPtr & usedContextVars)
{
    bool foundContextVars = false;

    ConstColorSpaceRcPtr src = config.getColorSpace(look.getSrc());
    if (CollectContextVariables(config, context, src, usedContextVars))
    {
        foundContextVars = true;
    }

    ConstColorSpaceRcPtr dst = config.getColorSpace(look.getDst());
    if (CollectContextVariables(config, context, dst, usedContextVars))
    {
        foundContextVars = true;
    }

    const char * looks = look.getLooks();
    if (looks && *looks)
    {
        LookParseResult lookList;
        lookList.parse(looks);

        // Note: For now, simply concatenating vars used by all of the options rather than trying to
        // figure out which option would be used.

        const LookParseResult::Options & options = lookList.getOptions();
        for (const auto & tokens : options)
        {
            for (const auto & token : tokens)
            {
                ConstLookRcPtr look = config.getLook(token.name.c_str());
                if (look && CollectContextVariables(config, context, token.dir, *look, usedContextVars))
                {
                    foundContextVars = true;
                }
            }
        }
    }

    return foundContextVars;
}

} // namespace OCIO_NAMESPACE

// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <iostream>

#include <OpenColorIO/OpenColorIO.h>

#include "transforms/FileTransform.h"
#include "BakingUtils.h"
#include "MathUtils.h"


namespace OCIO_NAMESPACE
{

BakerRcPtr Baker::Create()
{
    return BakerRcPtr(new Baker(), &deleter);
}

void Baker::deleter(Baker* c)
{
    delete c;
}

class Baker::Impl
{
public:

    ConfigRcPtr m_config;
    std::string m_formatName;
	FormatMetadataImpl m_formatMetadata{ METADATA_ROOT, "" };
    std::string m_inputSpace;
    std::string m_shaperSpace;
    std::string m_looks;
    std::string m_targetSpace;
    std::string m_display;
    std::string m_view;
    int m_shapersize;
    int m_cubesize;

    Impl() :
        m_shapersize(-1),
        m_cubesize(-1)
    {
    }

    Impl(const Impl &) = delete;

    ~Impl()
    {
    }

    Impl& operator= (const Impl & rhs)
    {
        if (this != &rhs)
        {
            m_config = rhs.m_config;
            m_formatName = rhs.m_formatName;
            m_formatMetadata = rhs.m_formatMetadata;
            m_inputSpace = rhs.m_inputSpace;
            m_shaperSpace = rhs.m_shaperSpace;
            m_looks = rhs.m_looks;
            m_targetSpace = rhs.m_targetSpace;
            m_display = rhs.m_display;
            m_view = rhs.m_view;
            m_shapersize = rhs.m_shapersize;
            m_cubesize = rhs.m_cubesize;
        }
        return *this;
    }
};

Baker::Baker()
: m_impl(new Baker::Impl)
{
}

Baker::~Baker()
{
    delete m_impl;
    m_impl = NULL;
}

BakerRcPtr Baker::createEditableCopy() const
{
    BakerRcPtr oven = Baker::Create();
    *oven->m_impl = *m_impl;
    return oven;
}

void Baker::setConfig(const ConstConfigRcPtr & config)
{
    getImpl()->m_config = config->createEditableCopy();
}

ConstConfigRcPtr Baker::getConfig() const
{
    return getImpl()->m_config;
}

int Baker::getNumFormats()
{
    return FormatRegistry::GetInstance().getNumFormats(FORMAT_CAPABILITY_BAKE);
}

const char * Baker::getFormatNameByIndex(int index)
{
    return FormatRegistry::GetInstance().getFormatNameByIndex(FORMAT_CAPABILITY_BAKE, index);
}

const char * Baker::getFormatExtensionByIndex(int index)
{
    return FormatRegistry::GetInstance().getFormatExtensionByIndex(FORMAT_CAPABILITY_BAKE, index);
}

void Baker::setFormat(const char * formatName)
{
    FileFormat * fmt = FormatRegistry::GetInstance().getFileFormatByName(formatName);
    if (fmt)
    {
        FormatInfoVec formatInfoVec;
        fmt->getFormatInfo(formatInfoVec);
        for (unsigned int i = 0; i < formatInfoVec.size(); ++i)
        {
            if (formatInfoVec[i].capabilities & FORMAT_CAPABILITY_BAKE)
            {
                getImpl()->m_formatName = formatName ? formatName : "";
                return;
            }
        }
    }

    std::ostringstream os;
    os << "File format " << formatName;
    os << " does not support baking.";
    throw Exception(os.str().c_str());
}

const char * Baker::getFormat() const
{
    return getImpl()->m_formatName.c_str();
}

const FormatMetadata & Baker::getFormatMetadata() const
{
    return getImpl()->m_formatMetadata;
}

FormatMetadata & Baker::getFormatMetadata()
{
    return getImpl()->m_formatMetadata;
}

void Baker::setInputSpace(const char * inputSpace)
{
    getImpl()->m_inputSpace = inputSpace ? inputSpace : "";
}

const char * Baker::getInputSpace() const
{
    return getImpl()->m_inputSpace.c_str();
}

void Baker::setShaperSpace(const char * shaperSpace)
{
    getImpl()->m_shaperSpace = shaperSpace ? shaperSpace : "";
}

const char * Baker::getShaperSpace() const
{
    return getImpl()->m_shaperSpace.c_str();
}

void Baker::setLooks(const char * looks)
{
    getImpl()->m_looks = looks ? looks : "";
}

const char * Baker::getLooks() const
{
    return getImpl()->m_looks.c_str();
}

void Baker::setTargetSpace(const char * targetSpace)
{
    getImpl()->m_targetSpace = targetSpace ? targetSpace : "";
}

const char * Baker::getTargetSpace() const
{
    return getImpl()->m_targetSpace.c_str();
}

const char * Baker::getDisplay() const
{
    return getImpl()->m_display.c_str();
}

const char * Baker::getView() const
{
    return getImpl()->m_view.c_str();
}

void Baker::setDisplayView(const char * display, const char * view)
{
    if (!display || !view)
    {
        throw Exception("Both display and view must be set.");
    }

    getImpl()->m_display = display;
    getImpl()->m_view = view;
}

void Baker::setShaperSize(int shapersize)
{
    getImpl()->m_shapersize = shapersize;
}

int Baker::getShaperSize() const
{
    return getImpl()->m_shapersize;
}

void Baker::setCubeSize(int cubesize)
{
    getImpl()->m_cubesize = cubesize;
}

int Baker::getCubeSize() const
{
    return getImpl()->m_cubesize;
}

void Baker::bake(std::ostream & os) const
{
    FileFormat* fmt = FormatRegistry::GetInstance().getFileFormatByName(getImpl()->m_formatName);

    if(!fmt)
    {
        std::ostringstream err;
        err << "The format named '" << getImpl()->m_formatName;
        err << "' could not be found. ";
        throw Exception(err.str().c_str());
    }

    FormatInfoVec fmtInfoVec;
    fmt->getFormatInfo(fmtInfoVec);
    FormatInfo fmtInfo = fmtInfoVec[0];

    const std::string & inputSpace = getImpl()->m_inputSpace;
    const std::string & targetSpace = getImpl()->m_targetSpace;
    const std::string & shaperSpace = getImpl()->m_shaperSpace;
    const std::string & display = getImpl()->m_display;
    const std::string & view = getImpl()->m_view;

    const bool displayViewMode = !display.empty() && !view.empty();
    const bool colorSpaceMode = !targetSpace.empty();

    // Settings validation.
    if(!getConfig())
    {
        throw Exception("No OCIO config has been set.");
    }

    if(inputSpace.empty())
    {
        throw Exception("No input space has been set.");
    }

    if(!displayViewMode && !colorSpaceMode)
    {
        throw Exception("No display / view or target colorspace has been set.");
    }

    if(displayViewMode && colorSpaceMode)
    {
        throw Exception("Cannot use both display / view and target colorspace.");
    }

    if(!getConfig()->getColorSpace(inputSpace.c_str()))
    {
        std::ostringstream os;
        os << "Could not find input colorspace '" << inputSpace << "'.";
        throw Exception(os.str().c_str());
    }

    if(colorSpaceMode && !getConfig()->getColorSpace(targetSpace.c_str()))
    {
        std::ostringstream os;
        os << "Could not find target colorspace '" << targetSpace << "'.";
        throw Exception(os.str().c_str());
    }

    if(displayViewMode)
    {
        bool foundDisplay = false;
        bool foundView = false;

        // Make sure we also search through inactive views.
        auto hasViewByType = [this](ViewType type, const char * display, const char * view)
        {
            for (int i = 0; i < getConfig()->getNumViews(type, display); ++i)
            {
                if (std::string(getConfig()->getView(type, display, i)) == std::string(view))
                {
                    return true;
                }
            }

            return false;
        };

        for (int i = 0; i < getConfig()->getNumDisplaysAll(); ++i)
        {
            const char * currDisplay = getConfig()->getDisplayAll(i);
            if (std::string(currDisplay) == display)
            {
                foundDisplay = true;
                foundView |= hasViewByType(VIEW_DISPLAY_DEFINED, currDisplay, view.c_str());
                foundView |= hasViewByType(VIEW_SHARED, currDisplay, view.c_str());
            }
            if (foundDisplay && foundView)
            {
                break;
            }
        }

        if (!foundDisplay)
        {
            std::ostringstream os;
            os << "Could not find display '" << display << "'.";
            throw Exception(os.str().c_str());
        }
        else if (!foundView)
        {
            std::ostringstream os;
            os << "Could not find view '" << view << "'.";
            throw Exception(os.str().c_str());
        }
    }

    const bool bake_1D = fmtInfo.bake_capabilities == FORMAT_BAKE_CAPABILITY_1DLUT;
    if(bake_1D && GetInputToTargetProcessor(*this)->hasChannelCrosstalk())
    {
        std::ostringstream os;
        os << "The format '" << getImpl()->m_formatName << "' does not support";
        os << " transformations with channel crosstalk.";
        throw Exception(os.str().c_str());
    }

    if(getCubeSize() != -1 && getCubeSize() < 2)
    {
        throw Exception("Cube size must be at least 2 if set.");
    }

    const bool supportShaper =
        fmtInfo.bake_capabilities & FORMAT_BAKE_CAPABILITY_1D_3D_LUT ||
        fmtInfo.bake_capabilities & FORMAT_BAKE_CAPABILITY_1DLUT;
    if(!shaperSpace.empty() && !supportShaper)
    {
        std::ostringstream os;
        os << "The format '" << getImpl()->m_formatName << "' does not support shaper space.";
        throw Exception(os.str().c_str());
    }

    if(!shaperSpace.empty() && getShaperSize() != -1 && getShaperSize() < 2)
    {
        std::ostringstream os;
        os << "A shaper space '" << getShaperSpace() << "' has";
        os << " been specified, so the shaper size must be 2 or larger.";
        throw Exception(os.str().c_str());
    }

    if(!shaperSpace.empty())
    {
        ConstCPUProcessorRcPtr inputToShaper = GetInputToShaperProcessor(*this);
        ConstCPUProcessorRcPtr shaperToInput = GetShaperToInputProcessor(*this);

        if(inputToShaper->hasChannelCrosstalk() || shaperToInput->hasChannelCrosstalk())
        {
            std::ostringstream os;
            os << "The specified shaper space, '" << getShaperSpace();
            os << "' has channel crosstalk, which is not appropriate for";
            os << " shapers. Please select an alternate shaper space or";
            os << " omit this option.";
            throw Exception(os.str().c_str());
        }
    }

    try
    {
        fmt->bake(*this, getImpl()->m_formatName, os);
    }
    catch(std::exception & e)
    {
        std::ostringstream err;
        err << "Error baking " << getImpl()->m_formatName << ":";
        err << e.what();
        throw Exception(err.str().c_str());
    }

    // 
    // TODO:
    // 
    // - check limits of shaper and target, throw exception if we can't
    //   write that much data in x format
    // - add some checks to make sure we are monotonic
    // - do a compare between OCIO transform and output LUT transform
    //   throw error if we going beyond tolerance
    //
}

} // namespace OCIO_NAMESPACE

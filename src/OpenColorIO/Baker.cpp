// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <iostream>

#include <OpenColorIO/OpenColorIO.h>

#include "transforms/FileTransform.h"
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
                getImpl()->m_formatName = formatName;
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
    getImpl()->m_inputSpace = inputSpace;
}

const char * Baker::getInputSpace() const
{
    return getImpl()->m_inputSpace.c_str();
}

void Baker::setShaperSpace(const char * shaperSpace)
{
    getImpl()->m_shaperSpace = shaperSpace;
}

const char * Baker::getShaperSpace() const
{
    return getImpl()->m_shaperSpace.c_str();
}

void Baker::setLooks(const char * looks)
{
    getImpl()->m_looks = looks;
}

const char * Baker::getLooks() const
{
    return getImpl()->m_looks.c_str();
}

void Baker::setTargetSpace(const char * targetSpace)
{
    getImpl()->m_targetSpace = targetSpace;
}

const char * Baker::getTargetSpace() const
{
    return getImpl()->m_targetSpace.c_str();
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

    if(!getConfig())
    {
        throw Exception("No OCIO config has been set");
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
    // - throw exception when we don't have inputSpace and targetSpace
    //   at least set
    // - check limits of shaper and target, throw exception if we can't
    //   write that much data in x format
    // - check that the shaper is 1D transform only, throw exception
    // - check the file format supports shapers, 1D and 3D
    // - add some checks to make sure we are monotonic
    // - deal with the case of writing out non cube formats (1D only)
    // - do a compare between OCIO transform and output LUT transform
    //   throw error if we going beyond tolerance
    //
}

} // namespace OCIO_NAMESPACE

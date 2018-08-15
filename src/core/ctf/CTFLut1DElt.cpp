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

#include "CTFLut1DElt.h"
#include "CTFReaderUtils.h"
#include <sstream>

#include "../Platform.h"
#include "../Logging.h"
#include "../MathUtils.h"

#include "../opdata/OpDataRange.h"

OCIO_NAMESPACE_ENTER
{
// Private namespace to the OpData sub-directory
namespace CTF
{
// Private namespace for the xml reader utils
namespace Reader
{

Lut1DElt::Lut1DElt()
    : OpElt()
    , ArrayMgt()
    , IndexMapMgt()
    , m_lut(new OpData::Lut1D(2))
    , m_indexMapping(0)
{
}

Lut1DElt::~Lut1DElt()
{
    // Do not delete  m_lut
    m_lut = 0x0;
}

void Lut1DElt::start(const char **atts)
{
    OpElt::start(atts);

    // As the 'interpolation' element is optional,
    // set the value to default behavior
    m_lut->setInterpolation(INTERP_DEFAULT);

    unsigned i = 0;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_INTERPOLATION, atts[i]))
        {
            try
            {
                m_lut->setInterpolation(
                    OpData::Lut1D::getInterpolation(atts[i + 1]));
            }
            catch (const std::exception& e)
            {
                Throw(e.what());
            }
        }

        if (0 == Platform::Strcasecmp(ATTR_HALF_DOMAIN, atts[i]))
        {
            if (0 != Platform::Strcasecmp("true", atts[i + 1]))
            {
                std::ostringstream oss;
                oss << "Illegal 'halfDomain' attribute '" << atts[i + 1];
                oss << "' while parsing Lut1D. ";
                Throw(oss.str());
            }

            m_lut->setInputHalfDomain(true);
        }

        if (0 == Platform::Strcasecmp(ATTR_RAW_HALFS, atts[i]))
        {
            if (0 != Platform::Strcasecmp("true", atts[i + 1]))
            {
                std::ostringstream oss;
                oss << "Illegal 'rawHalfs' attribute '" << atts[i + 1];
                oss << "' while parsing Lut1D. ";
                Throw(oss.str());
            }

            m_lut->setOutputRawHalfs(true);
        }

        i += 2;
    }
}

void Lut1DElt::end()
{
    OpElt::end();
    m_lut->validate();
}

OpData::OpData* Lut1DElt::getOp() const
{
    return m_lut;
}

OpData::ArrayBase* Lut1DElt::updateDimension(const Dimensions& dims)
{
    if (dims.size() != 2)
    {
        return 0x0;
    }

    const size_t max = (dims.empty() ? 0 : (dims.size() - 1));
    const unsigned numColorComponents = dims[max];

    if (dims[1] != 3 && dims[1] != 1)
    {
        return 0x0;
    }

    OpData::Array* pArray = &m_lut->getArray();
    pArray->resize(dims[0], numColorComponents);
    return pArray;
}

void Lut1DElt::finalize(unsigned position)
{
    OpData::Array* pArray = &m_lut->getArray();

    // Convert half bits to float values if needed
    if (m_lut->isOutputRawHalfs())
    {
        const size_t maxValues = pArray->getNumValues();
        for (size_t i = 0; i<maxValues; ++i)
        {
            pArray->getValues()[i]
                = ConvertHalfBitsToFloat((unsigned short)pArray->getValues()[i]);
        }
    }

    if (pArray->getNumValues() != position)
    {
        const unsigned numColorComponents = pArray->getNumColorComponents();
        const unsigned maxColorComponents = 3;

        const unsigned dimensions = pArray->getLength();

        if (numColorComponents != 1 || position != dimensions)
        {
            std::ostringstream arg;
            arg << "Expected ";
            arg << dimensions;
            arg << "x" << numColorComponents;
            arg << " Array values, found ";
            arg << position << ". ";
            Throw(arg.str());
        }

        // Convert a 1D LUT to a 3by1D LUT
        // (duplicate values from the Red to the Green and Blue)
        const unsigned numLuts = maxColorComponents;

        // TODO: Should improve Lut1DOp so that the copy is unnecessary.
        for (signed i = (dimensions - 1); i >= 0; --i)
        {
            for (unsigned j = 0; j<numLuts; ++j)
            {
                pArray->getValues()[(i*numLuts) + j] = pArray->getValues()[i];
            }
        }
    }

    pArray->validate();
    setCompleted(true);
}

OpData::IndexMapping* Lut1DElt::updateDimensionIM(const DimensionsIM& dims)
{
    if (dims.size() != 1)
    {
        return 0x0;
    }

    const unsigned numComponents = dims[0];

    if (dims[0] == 0)
    {
        return 0x0;
    }

    m_indexMapping.resize(numComponents);
    return &m_indexMapping;
}

void Lut1DElt::finalizeIM(unsigned position)
{
    if (m_indexMapping.getDimension() != position)
    {
        std::ostringstream arg;
        arg << "Expected ";
        arg << m_indexMapping.getDimension();
        arg << " IndexMap values, found ";
        arg << position << ". ";
        Throw(arg.str());
    }

    // TODO: For both Array and IndexMap, validate is called multiple times.
    m_indexMapping.validate();
    setCompletedIM(true);
}

void Lut1DElt_1_4::start(const char **atts)
{
    OpElt::start(atts);

    // As the 'interpolation' element is optional,
    // set the value to default behavior
    m_lut->setInterpolation(INTERP_DEFAULT);

    unsigned i = 0;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_INTERPOLATION, atts[i]))
        {
            try
            {
                m_lut->setInterpolation(
                    OpData::Lut1D::getInterpolation(atts[i + 1]));
            }
            catch (const std::exception& e)
            {
                Throw(e.what());
            }
        }

        if (0 == Platform::Strcasecmp(ATTR_HALF_DOMAIN, atts[i]))
        {
            if (0 != Platform::Strcasecmp("true", atts[i + 1]))
            {
                std::ostringstream oss;
                oss << "Illegal 'halfDomain' attribute '" << atts[i + 1];
                oss << "' while parsing Lut1D. ";
                Throw(oss.str());
            }

            m_lut->setInputHalfDomain(true);
        }

        if (0 == Platform::Strcasecmp(ATTR_RAW_HALFS, atts[i]))
        {
            if (0 != Platform::Strcasecmp("true", atts[i + 1]))
            {
                std::ostringstream oss;
                oss << "Illegal 'rawHalfs' attribute '" << atts[i + 1];
                oss << "' while parsing Lut1D. ";
                Throw(oss.str());
            }

            m_lut->setOutputRawHalfs(true);
        }

        // This was added in v1.4.
        if (0 == Platform::Strcasecmp(ATTR_HUE_ADJUST, atts[i]))
        {
            if (0 != Platform::Strcasecmp("dw3", atts[i + 1]))
            {
                std::ostringstream oss;
                oss << "Illegal 'hueAdjust' attribute '" << atts[i + 1];
                oss << "' while parsing Lut1D. ";
                Throw(oss.str());
            }

            m_lut->setHueAdjust(OpData::Lut1D::HUE_DW3);
        }

        i += 2;
    }

}

void Lut1DElt_1_7::end()
{
    OpElt::end();
    m_lut->validate();

    // The LUT renderers do not currently support an indexMap, however for
    // compliance with the CLF spec we want to support the case of a single 2-entry
    // indexMap by converting it into a RangeOp which we insert before the LUT.
    if (isCompletedIM())
    {
        // This will throw if the indexMap is not length 2.
        std::auto_ptr<OpData::Range> pRng(
            new OpData::Range(m_indexMapping,
                                m_lut->getInputBitDepth(),
                                m_lut->getArray().getLength()));

        // This code assumes that the current LUT is at the end of the opList.
        // In other words, that this LUT's end() method will be called before
        // any other Op's start().
        const unsigned len = (unsigned)m_transform->getOps().size();
        // TODO: According to Pat, there is a problem here in case of exception
        // as insert() requests ownership of the pointer.
        // (See also similar situation elsewhere in this module.)
        const unsigned pos = len - 1;
        m_transform->getOps().insert(pRng.release(), pos);
    }
}

} // exit Reader namespace
} // exit CTF namespace
}
OCIO_NAMESPACE_EXIT


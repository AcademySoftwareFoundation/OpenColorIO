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

#include "CTFLut3DElt.h"
#include "CTFReaderUtils.h"
#include <sstream>
#include "../Platform.h"
#include "../Logging.h"
#include "../opdata/OpDataRange.h"

OCIO_NAMESPACE_ENTER
{
// Private namespace to the OpData sub-directory
namespace CTF
{
// Private namespace for the xml reader utils
namespace Reader
{

Lut3DElt::Lut3DElt()
    : OpElt()
    , ArrayMgt()
    , IndexMapMgt()
    , m_lut(new OpData::Lut3D(2))
    , m_indexMapping(0)
{
}

Lut3DElt::~Lut3DElt()
{
    // Do not delete  m_lut
    m_lut = 0x0;
}

void Lut3DElt::start(const char **atts)
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
                    OpData::Lut3D::getInterpolation(atts[i + 1]));
            }
            catch (const std::exception& e)
            {
                Throw(e.what());
            }
        }

        i += 2;
    }
}

void Lut3DElt::end()
{
    OpElt::end();
    m_lut->validate();
}

OpData::OpData* Lut3DElt::getOp() const
{
    return m_lut;
}

OpData::ArrayBase* Lut3DElt::updateDimension(const Dimensions& dims)
{
    if (dims.size() != 4)
    {
        return 0x0;
    }

    const size_t max = (dims.empty() ? 0 : (dims.size() - 1));
    const unsigned numColorComponents = dims[max];

    if (dims[3] != 3 || dims[1] != dims[0] || dims[2] != dims[0])
    {
        return 0x0;
    }

    OpData::Array* pArray = &m_lut->getArray();
    pArray->resize(dims[0], numColorComponents);

    return pArray;
}

void Lut3DElt::finalize(unsigned position)
{
    OpData::Array* pArray = &m_lut->getArray();
    if (pArray->getNumValues() != position)
    {
        std::ostringstream arg;
        arg << "Expected ";
        arg << pArray->getLength() << "x" << pArray->getLength();
        arg << "x" << pArray->getLength();
        arg << "x" << pArray->getNumColorComponents();
        arg << " Array values, found ";
        arg << position;

        throw Exception(arg.str().c_str());
    }

    pArray->validate();
    setCompleted(true);
}

OpData::IndexMapping* Lut3DElt::updateDimensionIM(const DimensionsIM& dims)
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

void Lut3DElt::finalizeIM(unsigned position)
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

// TODO: Factor duplicate code from Lut1D version.
void Lut3DElt_1_7::end()
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

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

#include "CTFMatrixElt.h"
#include "../opdata/OpDataMatrix.h"
#include "../Platform.h"
#include <sstream>
#include "CTFReaderVersion.h"

OCIO_NAMESPACE_ENTER
{

// Private namespace to the OpData sub-directory
namespace CTF
{
// Private namespace for the xml reader utils
namespace Reader
{

MatrixElt::MatrixElt()
    : OpElt()
    , ArrayMgt()
    , m_matrix(new OpData::Matrix)
{
}

MatrixElt::~MatrixElt()
{
    // Do not delete  m_matrix
    m_matrix = 0x0;
}

void MatrixElt::start(const char **atts)
{
    OpElt::start(atts);
}

void MatrixElt::end()
{
    OpElt::end();

    // Validate the end result
    m_matrix->validate();
}

OpData::OpData* MatrixElt::getOp() const
{
    return m_matrix;
}

OpData::ArrayBase* MatrixElt::updateDimension(const Dimensions& dims)
{
    if (dims.size() != 3)
    {
        return 0x0;
    }

    const size_t max = (dims.empty() ? 0 : (dims.size() - 1));
    const unsigned numColorComponents = dims[max];

    if (dims[0] != dims[1] || dims[2] != 3)
    {
        return 0x0;
    }

    OpData::ArrayDouble* pArray = &m_matrix->getArray();
    pArray->resize(dims[0], numColorComponents);

    return pArray;
}

void MatrixElt::finalize(unsigned position)
{
    OpData::ArrayDouble& array = m_matrix->getArray();
    if (array.getNumValues() != position)
    {
        std::ostringstream arg;
        arg << "Expected ";
        arg << array.getLength();
        arg << "x" << array.getLength();
        arg << " Array values, found ";
        arg << position;

        throw Exception(arg.str().c_str());
    }

    // Extract offsets

    const OpData::ArrayDouble::Values& values = array.getValues();

    if (array.getLength() == 4)
    {
        m_matrix->setOffsetValue(0, values[3]);
        m_matrix->setOffsetValue(1, values[7]);
        m_matrix->setOffsetValue(2, values[11]);

        m_matrix->setArrayValue(3, 0.0f);
        m_matrix->setArrayValue(7, 0.0f);
        m_matrix->setArrayValue(11, 0.0f);
        m_matrix->setArrayValue(15, 1.0f);
    }

    // Array parsing is done

    setCompleted(true);

    convert_1_2_to_Latest();

    array.validate();
}

void MatrixElt::convert_1_2_to_Latest()
{
    if (CTF_PROCESS_LIST_VERSION_1_2 < CTF_PROCESS_LIST_VERSION)
    {
        OpData::ArrayDouble& array = m_matrix->getArray();

        if (array.getLength() == 3)
        {
            const float offsets[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            m_matrix->setRGBAOffsets(offsets);
        }
        else if (array.getLength() == 4)
        {
            m_matrix->setOffsetValue(3, 0.0f);

            array = m_matrix->getArray();
            OpData::ArrayDouble::Values oldV = array.getValues();

            array.resize(3, 3);

            OpData::ArrayDouble::Values& v = array.getValues();
            v[0] = oldV[0];
            v[1] = oldV[1];
            v[2] = oldV[2];

            v[3] = oldV[4];
            v[4] = oldV[5];
            v[5] = oldV[6];

            v[6] = oldV[8];
            v[7] = oldV[9];
            v[8] = oldV[10];
        }
        else if (array.getLength() != 4)
        {
            std::ostringstream arg;
            arg << "MatrixElt: Expecting array dimension to be 3 or 4. Got: ";
            arg << array.getLength();
            arg << ".";

            throw Exception(arg.str().c_str());
        }
    }
}

//////////////////////////////////////////////////////////

OpData::ArrayBase* MatrixElt_1_3::updateDimension(const Dimensions& dims)
{
    if (dims.size() != 3)
    {
        return 0x0;
    }

    const size_t max = (dims.empty() ? 0 : (dims.size() - 1));
    const unsigned numColorComponents = dims[max];

    if (!((dims[0] == 3 && dims[1] == 3 && dims[2] == 3) ||
        (dims[0] == 3 && dims[1] == 4 && dims[2] == 3) ||
        (dims[0] == 4 && dims[1] == 4 && dims[2] == 4) ||
        (dims[0] == 4 && dims[1] == 5 && dims[2] == 4)))
    {
        return 0x0;
    }

    OpData::ArrayDouble* pArray = &getMatrix()->getArray();
    pArray->resize(dims[1], numColorComponents);

    return pArray;
}

void MatrixElt_1_3::finalize(unsigned position)
{
    OpData::ArrayDouble& array = getMatrix()->getArray();

    const OpData::ArrayDouble::Values& values = array.getValues();

    // Note: Version 1.3 of Matrix Op supports 4 xml formats:
    //       1) 4x5x4, matrix with alpha and offsets
    //       2) 4x4x4, matrix with alpha and no offsets
    //       3) 3x4x3, matrix only with offsets and no alpha
    //       4) 3x3x3, matrix with no alpha and no offsets

    if (array.getLength() == 3 && array.getNumColorComponents() == 3)
    {
        if (position != 9)
        {
            std::ostringstream oss;
            oss << "Expected 3x3x3 Array values, found ";
            oss << position << ".";
            Throw(oss.str());
        }
    }
    else if (array.getLength() == 4)
    {
        if (array.getNumColorComponents() == 3)
        {
            if (position != 12)
            {
                std::ostringstream oss;
                oss << "Expected 3x4x3 Array values, found ";
                oss << position << ".";
                Throw(oss.str());
            }

            OpData::Matrix* pMatrix = getMatrix();

            pMatrix->setOffsetValue(0, values[3]);
            pMatrix->setOffsetValue(1, values[7]);
            pMatrix->setOffsetValue(2, values[11]);
            pMatrix->setOffsetValue(3, 0.0);

            OpData::ArrayDouble::Values oldV = array.getValues();

            array.setLength(3);

            OpData::ArrayDouble::Values& v = array.getValues();
            v[0] = oldV[0];
            v[1] = oldV[1];
            v[2] = oldV[2];

            v[3] = oldV[4];
            v[4] = oldV[5];
            v[5] = oldV[6];

            v[6] = oldV[8];
            v[7] = oldV[9];
            v[8] = oldV[10];
        }
        else
        {
            if (position != 16)
            {
                std::ostringstream oss;
                oss << "Expected 4x4x4 Array values, found ";
                oss << position << ".";
                Throw(oss.str());
            }

            OpData::Matrix* pMatrix = getMatrix();

            const float offsets[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            pMatrix->setRGBAOffsets(offsets);
        }
    }
    else
    {
        if (position != 20)
        {
            std::ostringstream oss;
            oss << "Expected 4x5x4 Array values, found ";
            oss << position << ".";
            Throw(oss.str());
        }

        OpData::Matrix* pMatrix = getMatrix();

        pMatrix->setOffsetValue(0, values[4]);
        pMatrix->setOffsetValue(1, values[9]);
        pMatrix->setOffsetValue(2, values[14]);
        pMatrix->setOffsetValue(3, values[19]);

        OpData::ArrayDouble::Values oldV = array.getValues();

        array.resize(4, 4);

        OpData::ArrayDouble::Values& v = array.getValues();
        v[0] = oldV[0];
        v[1] = oldV[1];
        v[2] = oldV[2];
        v[3] = oldV[3];

        v[4] = oldV[5];
        v[5] = oldV[6];
        v[6] = oldV[7];
        v[7] = oldV[8];

        v[8] = oldV[10];
        v[9] = oldV[11];
        v[10] = oldV[12];
        v[11] = oldV[13];

        v[12] = oldV[15];
        v[13] = oldV[16];
        v[14] = oldV[17];
        v[15] = oldV[18];
    }

    // Array parsing is done

    setCompleted(true);

    array.validate();
}



} // exit Reader namespace
} // exit CTF namespace
}
OCIO_NAMESPACE_EXIT

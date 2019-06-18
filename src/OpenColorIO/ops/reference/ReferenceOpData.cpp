/*
Copyright (c) 2019 Autodesk Inc., et al.
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

#include <OpenColorIO/OpenColorIO.h>

#include "ops/reference/ReferenceOpData.h"
#include "Platform.h"
#include "pystring/pystring.h"
#include "transforms/FileTransform.h"

OCIO_NAMESPACE_ENTER
{

ReferenceOpData::ReferenceOpData()
    : OpData(BIT_DEPTH_F32, BIT_DEPTH_F32)
{
}

ReferenceOpData::~ReferenceOpData()
{
}

void ReferenceOpData::validate() const
{
}

bool ReferenceOpData::isNoOp() const
{
    return false;
}

bool ReferenceOpData::isIdentity() const
{
    return false;
}

bool ReferenceOpData::hasChannelCrosstalk() const
{
    return true;
}

bool ReferenceOpData::operator==(const OpData& other) const
{
    if (this == &other) return true;

    if (!OpData::operator==(other)) return false;

    const ReferenceOpData* rop = static_cast<const ReferenceOpData*>(&other);

    if (m_referenceStyle != rop->m_referenceStyle) return false;
    if (m_direction != rop->m_direction) return false;
    if (m_referenceStyle == REF_PATH)
    {
        if (m_path != rop->m_path) return false;
    }
    else
    {
        if (m_alias != rop->m_alias) return false;
    }

    return true;
}

void ReferenceOpData::finalize()
{
    AutoMutex lock(m_mutex);

    std::ostringstream cacheIDStream;
    cacheIDStream << getID() << " ";

    cacheIDStream << m_referenceStyle << " ";
    cacheIDStream << TransformDirectionToString(m_direction) << " ";
    if (m_referenceStyle == REF_PATH)
    {
        cacheIDStream << m_path << " ";
    }
    else
    {
        cacheIDStream << m_alias << " ";
    }

    m_cacheID = cacheIDStream.str();
}

}
OCIO_NAMESPACE_EXIT

/////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include "ops/CDL/CDLOpData.h"
#include "ops/Lut1D/Lut1DOpData.h"
#include "ops/Matrix/MatrixOpData.h"
#include "ops/NoOp/NoOps.h"
#include "ops/Range/RangeOpData.h"
#include "unittest.h"
#include "UnitTestUtils.h"

OIIO_ADD_TEST(Reference, accessors)
{
    OCIO::ReferenceOpData r;

    OIIO_CHECK_EQUAL(r.getReferenceStyle(), OCIO::REF_PATH);
    OIIO_CHECK_EQUAL(r.getPath(), "");

    const std::string alias{ "Alias" };
    r.setAlias(alias);
    OIIO_CHECK_EQUAL(r.getReferenceStyle(), OCIO::REF_ALIAS);
    OIIO_CHECK_EQUAL(r.getAlias(), alias);

    const std::string file{ "TestPath.txt" };
    r.setPath(file);
    OIIO_CHECK_EQUAL(r.getReferenceStyle(), OCIO::REF_PATH);
    OIIO_CHECK_EQUAL(r.getPath(), file);
}

// Use OCIO public interface to load a file transform.
// We can't validate what the processor contains.
OCIO::ConstProcessorRcPtr GetTransformFileProcessor(const std::string & fileName)
{
    // (Path will be normalized, so this works even on Windows.)
    const std::string filePath(std::string(OCIO::getTestFilesDir()) + "/"
                               + fileName);
    OCIO::FileTransformRcPtr pFileTransform
        = OCIO::FileTransform::Create();
    pFileTransform->setInterpolation(OCIO::INTERP_LINEAR);
    pFileTransform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    pFileTransform->setSrc(filePath.c_str());

    OCIO::ConfigRcPtr pConfig = OCIO::Config::Create();

    // Use search paths to resolve references.
    pConfig->addSearchPath(OCIO::getTestFilesDir());

    return pConfig->getProcessor(pFileTransform);
}

OIIO_ADD_TEST(Reference, load_path_resolve_failing)
{
    std::string fileName("reference_path_missing_file.ctf");
    OIIO_CHECK_THROW_WHAT(GetTransformFileProcessor(fileName), OCIO::Exception,
                          "could not be located");
}

OIIO_ADD_TEST(Reference, load_multiple_resolve)
{
    std::string fileName("references_some_inverted.ctf");
    OCIO::ConstProcessorRcPtr processor;
    OIIO_CHECK_NO_THROW(processor = GetTransformFileProcessor(fileName));
    OIIO_REQUIRE_ASSERT(processor);
}

OIIO_ADD_TEST(Reference, load_same_twice_resolve)
{
    std::string fileName("references_same_twice.ctf");
    OCIO::ConstProcessorRcPtr processor;
    OIIO_CHECK_NO_THROW(processor = GetTransformFileProcessor(fileName));
    OIIO_REQUIRE_ASSERT(processor);
}

OIIO_ADD_TEST(Reference, load_nested_resolve)
{
    std::string fileName("reference_nested.ctf");
    OCIO::ConstProcessorRcPtr processor;
    OIIO_CHECK_NO_THROW(processor = GetTransformFileProcessor(fileName));
    OIIO_REQUIRE_ASSERT(processor);
}

OIIO_ADD_TEST(Reference, load_cycle_itself_resolve_failing)
{
    std::string fileName("reference_cycle_itself.ctf");
    OIIO_CHECK_THROW_WHAT(GetTransformFileProcessor(fileName), OCIO::Exception,
                          "is creating a recursion");
}

OIIO_ADD_TEST(Reference, load_cycle_2_resolve_failing)
{
    std::string fileName("reference_cycle_2levels.ctf");
    OIIO_CHECK_THROW_WHAT(GetTransformFileProcessor(fileName), OCIO::Exception,
                          "is creating a recursion");
}

OIIO_ADD_TEST(Reference, load_cycle_3_resolve_failing)
{
    std::string fileName("reference_cycle_3levels.ctf");
    OIIO_CHECK_THROW_WHAT(GetTransformFileProcessor(fileName), OCIO::Exception,
                          "is creating a recursion");
}

OIIO_ADD_TEST(Reference, load_cycle_rel_resolve_failing)
{
    std::string fileName("reference_cycle_path_not_equal.ctf");
    OIIO_CHECK_THROW_WHAT(GetTransformFileProcessor(fileName), OCIO::Exception,
                          "is creating a recursion");
}

namespace
{
bool GetFilePath(std::string & path, OCIO::ConstOpRcPtr & op)
{
    OCIO::ConstOpDataRcPtr data = op->data();
    auto fileData = OCIO::DynamicPtrCast<const OCIO::FileNoOpData>(data);
    if (fileData)
    {
        path = fileData->getPath();
        return true;
    }
    return false;
}
}

//
// The following tests load files using the non-public API in order to validate
// the referenced files are loaded correctly.
//

OIIO_ADD_TEST(Reference, load_one_reference)
{
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    context->addSearchPath(OCIO::getTestFilesDir());
    std::string fileName("reference_one_matrix.ctf");
    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(BuildOpsTest(ops, fileName, context, OCIO::TRANSFORM_DIR_FORWARD));

    // Ops contains [FileNoOp, FileNoOp, Matrix].
    OIIO_REQUIRE_EQUAL(ops.size(), 3);

    OIIO_CHECK_NO_THROW(ops.validate());

    OCIO::ConstOpRcPtr op = ops[2];
    auto matrixData = OCIO::DynamicPtrCast<const OCIO::MatrixOpData>(op->data());
    OIIO_REQUIRE_ASSERT(matrixData);
    OIIO_CHECK_EQUAL(op->getInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OIIO_CHECK_EQUAL(op->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);
}

OIIO_ADD_TEST(Reference, load_multiple_resolve_internal)
{
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    context->addSearchPath(OCIO::getTestFilesDir());
    std::string fileName("references_some_inverted.ctf");
    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(BuildOpsTest(ops, fileName, context, OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_REQUIRE_EQUAL(ops.size(), 9);
    OIIO_REQUIRE_ASSERT(ops[0]);
    std::string path;
    OCIO::ConstOpRcPtr op = ops[0];
    OIIO_REQUIRE_ASSERT(GetFilePath(path, op));
    OIIO_CHECK_NE(path.find(fileName), std::string::npos);

    OIIO_REQUIRE_ASSERT(ops[1]);
    op = ops[1];
    OIIO_REQUIRE_ASSERT(GetFilePath(path, op));
    OIIO_CHECK_NE(path.find("matrix_example.clf"), std::string::npos);

    OIIO_REQUIRE_ASSERT(ops[2]);
    op = ops[2];
    auto matrixData = OCIO::DynamicPtrCast<const OCIO::MatrixOpData>(op->data());
    OIIO_REQUIRE_ASSERT(matrixData);
    OIIO_CHECK_EQUAL(op->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    // (OutDepth was 32f in referenced file.)
    OIIO_CHECK_EQUAL(op->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);

    OIIO_REQUIRE_ASSERT(ops[3]);
    op = ops[3];
    OIIO_REQUIRE_ASSERT(GetFilePath(path, op));
    OIIO_CHECK_NE(path.find("xyz_to_rgb.clf"), std::string::npos);

    OIIO_REQUIRE_ASSERT(ops[4]);
    op = ops[4];
    auto lutData = OCIO::DynamicPtrCast<const OCIO::Lut1DOpData>(op->data());
    OIIO_REQUIRE_ASSERT(lutData);
    OIIO_CHECK_EQUAL(lutData->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
    // (InDepth was 32f in original.)
    OIIO_CHECK_EQUAL(op->getInputBitDepth(), OCIO::BIT_DEPTH_UINT12);
    OIIO_CHECK_EQUAL(op->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    OIIO_REQUIRE_ASSERT(ops[5]);
    op = ops[5];
    auto matrixData2 = OCIO::DynamicPtrCast<const OCIO::MatrixOpData>(op->data());
    OIIO_REQUIRE_ASSERT(matrixData2);
    OIIO_CHECK_EQUAL(op->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    // (OutDepth was 32f in referenced file.)
    OIIO_CHECK_EQUAL(op->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);

    OIIO_REQUIRE_ASSERT(ops[6]);
    op = ops[6];
    auto rangeData = OCIO::DynamicPtrCast<const OCIO::RangeOpData>(op->data());
    OIIO_REQUIRE_ASSERT(rangeData);

    OIIO_REQUIRE_ASSERT(ops[7]);
    op = ops[7];
    OIIO_REQUIRE_ASSERT(GetFilePath(path, op));
    OIIO_CHECK_NE(path.find("cdl_clamp_fwd.clf"), std::string::npos);

    OIIO_REQUIRE_ASSERT(ops[8]);
    op = ops[8];
    auto cdlData = OCIO::DynamicPtrCast<const OCIO::CDLOpData>(op->data());
    OIIO_REQUIRE_ASSERT(cdlData);
    // (OutDepth was 16f in original.)
    OIIO_CHECK_EQUAL(op->getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(op->getOutputBitDepth(), OCIO::BIT_DEPTH_F32);
}

OIIO_ADD_TEST(Reference, load_nested_resolve_internal)
{
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    context->addSearchPath(OCIO::getTestFilesDir());
    std::string fileName("reference_nested.ctf");
    OCIO::OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(BuildOpsTest(ops, fileName, context, OCIO::TRANSFORM_DIR_FORWARD));
    OIIO_REQUIRE_EQUAL(ops.size(), 4);

    std::string path;
    OCIO::ConstOpRcPtr op = ops[0];
    OIIO_REQUIRE_ASSERT(op);
    OIIO_REQUIRE_ASSERT(GetFilePath(path, op));
    OIIO_CHECK_NE(path.find("reference_nested.ctf"), std::string::npos);

    op = ops[1];
    OIIO_REQUIRE_ASSERT(op);
    OIIO_REQUIRE_ASSERT(GetFilePath(path, op));
    OIIO_CHECK_NE(path.find("reference_nested_2.ctf"), std::string::npos);

    op = ops[2];
    OIIO_REQUIRE_ASSERT(op);
    OIIO_REQUIRE_ASSERT(GetFilePath(path, op));
    OIIO_CHECK_NE(path.find("matrix_example.clf"), std::string::npos);

    op = ops[3];
    OIIO_REQUIRE_ASSERT(op);
    auto matrixData = OCIO::DynamicPtrCast<const OCIO::MatrixOpData>(op->data());
    OIIO_REQUIRE_ASSERT(matrixData);
}

// TODO: Once searchPath for Windows is fixed, add test where files are in different directories
// and several path are being searched.

#endif
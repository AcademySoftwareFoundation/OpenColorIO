// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/reference/ReferenceOpData.cpp"

#include "ops/cdl/CDLOpData.h"
#include "ops/lut1d/Lut1DOpData.h"
#include "ops/matrix/MatrixOpData.h"
#include "ops/noop/NoOps.h"
#include "ops/range/RangeOpData.h"
#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(Reference, accessors)
{
    OCIO::ReferenceOpData r;

    OCIO_CHECK_EQUAL(r.getReferenceStyle(), OCIO::REF_PATH);
    OCIO_CHECK_EQUAL(r.getPath(), "");

    const std::string alias{ "Alias" };
    r.setAlias(alias);
    OCIO_CHECK_EQUAL(r.getReferenceStyle(), OCIO::REF_ALIAS);
    OCIO_CHECK_EQUAL(r.getAlias(), alias);

    const std::string file{ "TestPath.txt" };
    r.setPath(file);
    OCIO_CHECK_EQUAL(r.getReferenceStyle(), OCIO::REF_PATH);
    OCIO_CHECK_EQUAL(r.getPath(), file);
}

namespace
{
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
}

OCIO_ADD_TEST(Reference, load_path_resolve_failing)
{
    std::string fileName("reference_path_missing_file.ctf");
    OCIO_CHECK_THROW_WHAT(GetTransformFileProcessor(fileName), OCIO::Exception,
                          "could not be located");
}

OCIO_ADD_TEST(Reference, load_multiple_resolve)
{
    std::string fileName("references_some_inverted.ctf");
    OCIO::ConstProcessorRcPtr processor;
    OCIO_CHECK_NO_THROW(processor = GetTransformFileProcessor(fileName));
    OCIO_REQUIRE_ASSERT(processor);
}

OCIO_ADD_TEST(Reference, load_same_twice_resolve)
{
    std::string fileName("references_same_twice.ctf");
    OCIO::ConstProcessorRcPtr processor;
    OCIO_CHECK_NO_THROW(processor = GetTransformFileProcessor(fileName));
    OCIO_REQUIRE_ASSERT(processor);
}

OCIO_ADD_TEST(Reference, load_nested_resolve)
{
    std::string fileName("reference_nested.ctf");
    OCIO::ConstProcessorRcPtr processor;
    OCIO_CHECK_NO_THROW(processor = GetTransformFileProcessor(fileName));
    OCIO_REQUIRE_ASSERT(processor);
}

OCIO_ADD_TEST(Reference, load_cycle_itself_resolve_failing)
{
    std::string fileName("reference_cycle_itself.ctf");
    OCIO_CHECK_THROW_WHAT(GetTransformFileProcessor(fileName), OCIO::Exception,
                          "is creating a recursion");
}

OCIO_ADD_TEST(Reference, load_cycle_2_resolve_failing)
{
    std::string fileName("reference_cycle_2levels.ctf");
    OCIO_CHECK_THROW_WHAT(GetTransformFileProcessor(fileName), OCIO::Exception,
                          "is creating a recursion");
}

OCIO_ADD_TEST(Reference, load_cycle_3_resolve_failing)
{
    std::string fileName("reference_cycle_3levels.ctf");
    OCIO_CHECK_THROW_WHAT(GetTransformFileProcessor(fileName), OCIO::Exception,
                          "is creating a recursion");
}

OCIO_ADD_TEST(Reference, load_cycle_rel_resolve_failing)
{
    std::string fileName("reference_cycle_path_not_equal.ctf");
    OCIO_CHECK_THROW_WHAT(GetTransformFileProcessor(fileName), OCIO::Exception,
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

OCIO_ADD_TEST(Reference, load_one_reference)
{
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    context->addSearchPath(OCIO::getTestFilesDir());
    std::string fileName("reference_one_matrix.ctf");
    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, fileName, context, OCIO::TRANSFORM_DIR_FORWARD));

    // Ops contains [FileNoOp, FileNoOp, Matrix].
    OCIO_REQUIRE_EQUAL(ops.size(), 3);

    OCIO::ConstOpRcPtr op = ops[2];
    auto matrixData = OCIO::DynamicPtrCast<const OCIO::MatrixOpData>(op->data());
    OCIO_REQUIRE_ASSERT(matrixData);
}

OCIO_ADD_TEST(Reference, load_multiple_resolve_internal)
{
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    context->addSearchPath(OCIO::getTestFilesDir());
    std::string fileName("references_some_inverted.ctf");
    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, fileName, context, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 9);
    OCIO_REQUIRE_ASSERT(ops[0]);
    std::string path;
    OCIO::ConstOpRcPtr op = ops[0];
    OCIO_REQUIRE_ASSERT(GetFilePath(path, op));
    OCIO_CHECK_NE(path.find(fileName), std::string::npos);

    OCIO_REQUIRE_ASSERT(ops[1]);
    op = ops[1];
    OCIO_REQUIRE_ASSERT(GetFilePath(path, op));
    OCIO_CHECK_NE(path.find("matrix_example_1_3_offsets.ctf"), std::string::npos);

    OCIO_REQUIRE_ASSERT(ops[2]);
    op = ops[2];
    auto matrixData = OCIO::DynamicPtrCast<const OCIO::MatrixOpData>(op->data());
    OCIO_REQUIRE_ASSERT(matrixData);

    OCIO_REQUIRE_ASSERT(ops[3]);
    op = ops[3];
    OCIO_REQUIRE_ASSERT(GetFilePath(path, op));
    OCIO_CHECK_NE(path.find("xyz_to_rgb.clf"), std::string::npos);

    OCIO_REQUIRE_ASSERT(ops[4]);
    op = ops[4];
    auto lutData = OCIO::DynamicPtrCast<const OCIO::Lut1DOpData>(op->data());
    OCIO_REQUIRE_ASSERT(lutData);
    OCIO_CHECK_EQUAL(lutData->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_REQUIRE_ASSERT(ops[5]);
    op = ops[5];
    auto matrixData2 = OCIO::DynamicPtrCast<const OCIO::MatrixOpData>(op->data());
    OCIO_REQUIRE_ASSERT(matrixData2);

    OCIO_REQUIRE_ASSERT(ops[6]);
    op = ops[6];
    auto rangeData = OCIO::DynamicPtrCast<const OCIO::RangeOpData>(op->data());
    OCIO_REQUIRE_ASSERT(rangeData);

    OCIO_REQUIRE_ASSERT(ops[7]);
    op = ops[7];
    OCIO_REQUIRE_ASSERT(GetFilePath(path, op));
    OCIO_CHECK_NE(path.find("cdl_clamp_fwd.clf"), std::string::npos);

    OCIO_REQUIRE_ASSERT(ops[8]);
    op = ops[8];
    auto cdlData = OCIO::DynamicPtrCast<const OCIO::CDLOpData>(op->data());
    OCIO_REQUIRE_ASSERT(cdlData);
}

OCIO_ADD_TEST(Reference, load_nested_resolve_internal)
{
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    context->addSearchPath(OCIO::getTestFilesDir());
    std::string fileName("reference_nested.ctf");
    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, fileName, context, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 4);

    std::string path;
    OCIO::ConstOpRcPtr op = ops[0];
    OCIO_REQUIRE_ASSERT(op);
    OCIO_REQUIRE_ASSERT(GetFilePath(path, op));
    OCIO_CHECK_NE(path.find("reference_nested.ctf"), std::string::npos);

    op = ops[1];
    OCIO_REQUIRE_ASSERT(op);
    OCIO_REQUIRE_ASSERT(GetFilePath(path, op));
    OCIO_CHECK_NE(path.find("reference_nested_2.ctf"), std::string::npos);

    op = ops[2];
    OCIO_REQUIRE_ASSERT(op);
    OCIO_REQUIRE_ASSERT(GetFilePath(path, op));
    OCIO_CHECK_NE(path.find("matrix_example.clf"), std::string::npos);

    op = ops[3];
    OCIO_REQUIRE_ASSERT(op);
    auto matrixData = OCIO::DynamicPtrCast<const OCIO::MatrixOpData>(op->data());
    OCIO_REQUIRE_ASSERT(matrixData);
}

// TODO: Once searchPath for Windows is fixed, add test where files are in different directories
// and several path are being searched.


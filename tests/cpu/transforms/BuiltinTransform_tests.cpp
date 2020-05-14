// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <map>
#include <utility>
#include <vector>

#include "transforms/BuiltinTransform.cpp"

#include "ops/matrix/MatrixOp.h"
#include "Platform.h"
#include "transforms/builtins/ACES.h"
#include "transforms/builtins/ColorMatrixHelpers.h"
#include "transforms/builtins/OpHelpers.h"
#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(BuiltinTransform, creation)
{
    // Tests around the creation of a built-in transform instance.

    OCIO::BuiltinTransformRcPtr blt = OCIO::BuiltinTransform::Create();

    OCIO_CHECK_EQUAL(blt->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(std::string(blt->getStyle()), "IDENTITY");
    OCIO_CHECK_NO_THROW(blt->validate());

    OCIO_CHECK_NO_THROW(blt->setStyle("ACES-AP0_to_CIE-XYZ-D65_BFD"));
    OCIO_CHECK_EQUAL(std::string(blt->getStyle()), "ACES-AP0_to_CIE-XYZ-D65_BFD");
    OCIO_CHECK_NO_THROW(blt->validate());

    OCIO_CHECK_EQUAL(std::string("Convert ACES AP0 primaries to CIE XYZ with a D65 white point with Bradford adaptation"),
                     blt->getDescription());

    OCIO_CHECK_NO_THROW(blt->setDirection(OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_EQUAL(blt->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_NO_THROW(blt->validate());

    // The style is case insensitive.
    OCIO_CHECK_NO_THROW(blt->setStyle("ACES-AP0_to_cie-xyz-D65_BFD"));
    OCIO_CHECK_NO_THROW(blt->validate());

    // Try an unknown style.
    OCIO_CHECK_THROW_WHAT(blt->setStyle("ACES-AP0_to_CIE-XYZ-D65_BFD_UNKNOWN"),
                          OCIO::Exception,
                          "BuiltinTransform: invalid built-in transform style 'ACES-AP0_to_CIE-XYZ-D65_BFD_UNKNOWN'.");
}

OCIO_ADD_TEST(BuiltinTransform, access)
{
    // Only test some default built-in transforms.

    OCIO_CHECK_EQUAL(std::string("IDENTITY"),
                     OCIO::BuiltinTransformRegistry::Get()->getBuiltinStyle(0));

    OCIO_CHECK_EQUAL(std::string("ACES-AP0_to_CIE-XYZ-D65_BFD"),
                     OCIO::BuiltinTransformRegistry::Get()->getBuiltinStyle(1));

    OCIO_CHECK_EQUAL(std::string("Convert ACES AP0 primaries to CIE XYZ with a D65 white point with Bradford adaptation"),
                     OCIO::BuiltinTransformRegistry::Get()->getBuiltinDescription(1));
}

OCIO_ADD_TEST(BuiltinTransform, forward_inverse)
{
    // A forward and inverse built-in transform must be optimized out.

    // Note: As the optimization is performed using the Ops (i.e. resulting from the built-in
    // transforms), it depends on the op list optimizations and not on the transform list. 

    OCIO::BuiltinTransformRcPtr fwdBuiltin = OCIO::BuiltinTransform::Create();
    OCIO_CHECK_NO_THROW(fwdBuiltin->setStyle("ACEScct_to_ACES2065-1"));
    OCIO_CHECK_NO_THROW(fwdBuiltin->setDirection(OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(fwdBuiltin->validate());

    OCIO::BuiltinTransformRcPtr invBuiltin = OCIO::BuiltinTransform::Create();
    OCIO_CHECK_NO_THROW(invBuiltin->setStyle("ACEScct_to_ACES2065-1"));
    OCIO_CHECK_NO_THROW(invBuiltin->setDirection(OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_NO_THROW(invBuiltin->validate());

    OCIO::GroupTransformRcPtr grp = OCIO::GroupTransform::Create();
    OCIO_CHECK_NO_THROW(grp->appendTransform(fwdBuiltin));
    OCIO_CHECK_NO_THROW(grp->appendTransform(invBuiltin));
    // Content is [BuiltinTransform, BuiltinTransform].
    OCIO_CHECK_EQUAL(grp->getNumTransforms(), 2);

    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateRaw();
    OCIO::ConstProcessorRcPtr proc;
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(grp));

    // Without any optimizations.
    {
        OCIO_CHECK_NO_THROW(proc = proc->getOptimizedProcessor(OCIO::BIT_DEPTH_F32,
                                                               OCIO::BIT_DEPTH_F32,
                                                               OCIO::OPTIMIZATION_NONE));

        OCIO_CHECK_NO_THROW(grp = proc->createGroupTransform());
        // Content is [Lut1DTransform, MatrixTransform, MatrixTransform, Lut1DTransform].
        OCIO_CHECK_EQUAL(4, grp->getNumTransforms());
    }

    // With default optimizations.
    {
        OCIO_CHECK_NO_THROW(proc = proc->getOptimizedProcessor(OCIO::BIT_DEPTH_F32,
                                                               OCIO::BIT_DEPTH_F32,
                                                               OCIO::OPTIMIZATION_DEFAULT));

        OCIO_CHECK_NO_THROW(grp = proc->createGroupTransform());
        // All transforms have been optimized out.
        OCIO_CHECK_EQUAL(0, grp->getNumTransforms());
    }
}


namespace
{

template<typename T>
void ValidateValues(const char * prefixMsg, T in, T out, T errorThreshold, int lineNo)
{
    // Using rel error with a large minExpected value of 1 will transition
    // from absolute error for expected values < 1 and
    // relative error for values > 1.
    if (!OCIO::EqualWithSafeRelError(in, out, errorThreshold, T(1.)))
    {
        std::ostringstream errorMsg;
        errorMsg.precision(std::numeric_limits<T>::max_digits10);
        if (prefixMsg && *prefixMsg)
        {
            errorMsg << prefixMsg << ": ";
        }
        errorMsg << "value = " << in << " but expected = " << out;
        OCIO_CHECK_ASSERT_MESSAGE_FROM(0, errorMsg.str(), lineNo);
    }
}

template<typename T>
void ValidateValues(unsigned idx, T in, T out, T errorThreshold, int lineNo)
{
    std::ostringstream oss;
    oss << "Index = " << idx << " with threshold = " << errorThreshold;

    ValidateValues<T>(oss.str().c_str(), in, out, errorThreshold, lineNo);
}

template<typename T>
void ValidateValues(T in, T out, int lineNo)
{
    ValidateValues<T>(nullptr, in, out, T(1e-7), lineNo);
}

} // anon.


OCIO_ADD_TEST(Builtins, color_matrix_helpers)
{
    // Test all the color matrix helper methods.

    {
        OCIO::MatrixOpData::MatrixArrayPtr matrix = OCIO::rgb2xyz_from_xy(OCIO::ACES_AP1::primaries);

        ValidateValues( 0U, matrix->getDoubleValue( 0),  0.66245418, 1e-7, __LINE__);
        ValidateValues( 1U, matrix->getDoubleValue( 1),  0.13400421, 1e-7, __LINE__);
        ValidateValues( 2U, matrix->getDoubleValue( 2),  0.15618769, 1e-7, __LINE__);

        ValidateValues( 4U, matrix->getDoubleValue( 4),  0.27222872, 1e-7, __LINE__);
        ValidateValues( 5U, matrix->getDoubleValue( 5),  0.67408177, 1e-7, __LINE__);
        ValidateValues( 6U, matrix->getDoubleValue( 6),  0.05368952, 1e-7, __LINE__);

        ValidateValues( 8U, matrix->getDoubleValue( 8), -0.00557465, 1e-7, __LINE__);
        ValidateValues( 9U, matrix->getDoubleValue( 9),  0.00406073, 1e-7, __LINE__);
        ValidateValues(10U, matrix->getDoubleValue(10),  1.0103391 , 1e-6, __LINE__);


        OCIO_CHECK_EQUAL(matrix->getDoubleValue( 3), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue( 7), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(11), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(12), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(13), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(14), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(15), 1.);
    }

    {
        // D65 to D60.
        static const OCIO::MatrixOpData::Offsets src_XYZ(0.9504559270516716, 1., 1.0890577507598784, 0.);
        static const OCIO::MatrixOpData::Offsets dst_XYZ(0.9526460745698463, 1., 1.0088251843515859, 0.);

        OCIO::MatrixOpData::MatrixArrayPtr matrix = OCIO::build_vonkries_adapt(
            src_XYZ, dst_XYZ, OCIO::ADAPTATION_BRADFORD);

        ValidateValues( 0U, matrix->getDoubleValue( 0),  1.01303491, 1e-7, __LINE__);
        ValidateValues( 1U, matrix->getDoubleValue( 1),  0.00610526, 1e-7, __LINE__);
        ValidateValues( 2U, matrix->getDoubleValue( 2), -0.01497094, 1e-7, __LINE__);

        ValidateValues( 4U, matrix->getDoubleValue( 4),  0.00769823, 1e-7, __LINE__);
        ValidateValues( 5U, matrix->getDoubleValue( 5),  0.99816335, 1e-7, __LINE__);
        ValidateValues( 6U, matrix->getDoubleValue( 6), -0.00503204, 1e-7, __LINE__);

        ValidateValues( 8U, matrix->getDoubleValue( 8), -0.00284132, 1e-7, __LINE__);
        ValidateValues( 9U, matrix->getDoubleValue( 9),  0.00468516, 1e-7, __LINE__);
        ValidateValues(10U, matrix->getDoubleValue(10),  0.92450614, 1e-7, __LINE__);


        OCIO_CHECK_EQUAL(matrix->getDoubleValue( 3), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue( 7), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(11), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(12), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(13), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(14), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(15), 1.);
    }

    {
        // Note: Source and dest white points are equal.
        OCIO::MatrixOpData::MatrixArrayPtr matrix
            = build_conversion_matrix(OCIO::P3_D65::primaries, OCIO::REC709::primaries,
                                      OCIO::ADAPTATION_BRADFORD);

        ValidateValues( 0U, matrix->getDoubleValue( 0),  1.22494018, 1e-7, __LINE__);
        ValidateValues( 1U, matrix->getDoubleValue( 1), -0.22494018, 1e-7, __LINE__);
        ValidateValues( 2U, matrix->getDoubleValue( 2),  0.        , 1e-7, __LINE__);

        ValidateValues( 4U, matrix->getDoubleValue( 4), -0.04205695, 1e-7, __LINE__);
        ValidateValues( 5U, matrix->getDoubleValue( 5),  1.04205695, 1e-7, __LINE__);
        ValidateValues( 6U, matrix->getDoubleValue( 6),  0.        , 1e-7, __LINE__);

        ValidateValues( 8U, matrix->getDoubleValue( 8), -0.01963755, 1e-7, __LINE__);
        ValidateValues( 9U, matrix->getDoubleValue( 9), -0.07863605, 1e-7, __LINE__);
        ValidateValues(10U, matrix->getDoubleValue(10),  1.09827360, 1e-7, __LINE__);


        OCIO_CHECK_EQUAL(matrix->getDoubleValue( 3), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue( 7), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(11), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(12), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(13), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(14), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(15), 1.);
    }

    {
        // Note: Source and dest white points differ.
        OCIO::MatrixOpData::MatrixArrayPtr matrix
            = build_conversion_matrix(OCIO::ACES_AP1::primaries, OCIO::REC709::primaries,
                                      OCIO::ADAPTATION_BRADFORD);

        ValidateValues( 0U, matrix->getDoubleValue( 0),  1.70505099, 1e-7, __LINE__);
        ValidateValues( 1U, matrix->getDoubleValue( 1), -0.62179212, 1e-7, __LINE__);
        ValidateValues( 2U, matrix->getDoubleValue( 2), -0.08325887, 1e-7, __LINE__);

        ValidateValues( 4U, matrix->getDoubleValue( 4), -0.13025642, 1e-7, __LINE__);
        ValidateValues( 5U, matrix->getDoubleValue( 5),  1.14080474, 1e-7, __LINE__);
        ValidateValues( 6U, matrix->getDoubleValue( 6), -0.01054832, 1e-7, __LINE__);

        ValidateValues( 8U, matrix->getDoubleValue( 8), -0.02400336, 1e-7, __LINE__);
        ValidateValues( 9U, matrix->getDoubleValue( 9), -0.12896898, 1e-7, __LINE__);
        ValidateValues(10U, matrix->getDoubleValue(10),  1.15297233, 1e-7, __LINE__);


        OCIO_CHECK_EQUAL(matrix->getDoubleValue( 3), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue( 7), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(11), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(12), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(13), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(14), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(15), 1.);
    }

    {
        // Note: Source and dest white points differ.
        OCIO::MatrixOpData::MatrixArrayPtr matrix
            = build_conversion_matrix(OCIO::ACES_AP0::primaries, OCIO::CIE_XYZ_D65::primaries,
                                      OCIO::ADAPTATION_BRADFORD);

        ValidateValues( 0U, matrix->getDoubleValue( 0),  0.987189224216, 1e-7, __LINE__);
        ValidateValues( 1U, matrix->getDoubleValue( 1), -0.004683484721, 1e-7, __LINE__);
        ValidateValues( 2U, matrix->getDoubleValue( 2),  0.017494260505, 1e-7, __LINE__);

        ValidateValues( 4U, matrix->getDoubleValue( 4),  0.337368890788, 1e-7, __LINE__);
        ValidateValues( 5U, matrix->getDoubleValue( 5),  0.729521566690 , 1e-7, __LINE__);
        ValidateValues( 6U, matrix->getDoubleValue( 6), -0.066890457478, 1e-7, __LINE__);

        ValidateValues( 8U, matrix->getDoubleValue( 8),  0.001077950962, 1e-7, __LINE__);
        ValidateValues( 9U, matrix->getDoubleValue( 9), -0.003407263205, 1e-7, __LINE__);
        ValidateValues(10U, matrix->getDoubleValue(10),  1.002329312243, 1e-7, __LINE__);


        OCIO_CHECK_EQUAL(matrix->getDoubleValue( 3), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue( 7), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(11), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(12), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(13), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(14), 0.);
        OCIO_CHECK_EQUAL(matrix->getDoubleValue(15), 1.);
    }
}

OCIO_ADD_TEST(Builtins, interpolate)
{
    // Test the non-uniform 1D linear interpolation helper function.

    static constexpr unsigned lutSize = 4;
    static constexpr double lutValues[lutSize * 2]
    {
        0.,    1.0,
        0.50,  2.0,
        0.75,  2.5,
        1.,    3.
    };

    ValidateValues(OCIO::Interpolate1D(lutSize, &lutValues[0], -1.  ), 1.  , __LINE__);
    ValidateValues(OCIO::Interpolate1D(lutSize, &lutValues[0],  0.  ), 1.  , __LINE__);
    ValidateValues(OCIO::Interpolate1D(lutSize, &lutValues[0],  0.1 ), 1.2 , __LINE__);
    ValidateValues(OCIO::Interpolate1D(lutSize, &lutValues[0],  0.5 ), 2.  , __LINE__);
    ValidateValues(OCIO::Interpolate1D(lutSize, &lutValues[0],  0.99), 2.98, __LINE__);
    ValidateValues(OCIO::Interpolate1D(lutSize, &lutValues[0],  2.  ), 3.  , __LINE__);
}

namespace
{

using Values = std::vector<float>;
using AllValues = std::map<std::string, std::pair<Values, Values>>;

void ValidateBuiltinTransform(const char * style, const Values & in, const Values & out, int lineNo)
{
    OCIO::BuiltinTransformRcPtr builtin = OCIO::BuiltinTransform::Create();
    OCIO_CHECK_NO_THROW_FROM(builtin->setStyle(style), lineNo);
    OCIO_CHECK_NO_THROW_FROM(builtin->setDirection(OCIO::TRANSFORM_DIR_FORWARD), lineNo);
    OCIO_CHECK_NO_THROW_FROM(builtin->validate(), lineNo);

    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateRaw();

    OCIO::ConstProcessorRcPtr proc;
    OCIO_CHECK_NO_THROW_FROM(proc = config->getProcessor(builtin), lineNo);

    OCIO::ConstCPUProcessorRcPtr cpu;
    OCIO_CHECK_NO_THROW_FROM(cpu = proc->getDefaultCPUProcessor(), lineNo);

    OCIO::PackedImageDesc inDesc((void *)&in[0], long(in.size() / 3), 1, 3);

    Values vals(in.size(), -1.0f);
    OCIO::PackedImageDesc outDesc((void *)&vals[0], long(vals.size() / 3), 1, 3);

    OCIO_CHECK_NO_THROW_FROM(cpu->apply(inDesc, outDesc), lineNo);

    static constexpr float errorThreshold = 1e-5f; 

    for (size_t idx = 0; idx < out.size(); ++idx)
    {
        std::ostringstream oss;
        oss << style << ": for index = " << idx << " with threshold = " << errorThreshold;
        ValidateValues(oss.str().c_str(), vals[idx], out[idx], errorThreshold, lineNo);
    }
}

AllValues UnitTestValues
{
    // Contains the name, the input values and the expected output values.

    { "IDENTITY",
        { { 0.5f, 0.4f, 0.3f }, { 0.5f,            0.4f,            0.3f } } },
    { "ACES-AP0_to_CIE-XYZ-D65_BFD",
        { { 0.5f, 0.4f, 0.3f }, { 0.496969496371f, 0.440425934827f, 0.299874863872f } } },
    { "ACES-AP1_to_CIE-XYZ-D65_BFD",
        { { 0.5f, 0.4f, 0.3f }, { 0.450739364025f, 0.420968434905f, 0.299137367021f } } },
    { "ACES-AP1_to_LINEAR-REC709_BFD",
        { { 0.5f, 0.4f, 0.3f }, { 0.578830986466f, 0.388029190156f, 0.282302431033f } } },
    { "ACEScct-LOG_to_LIN",
        { { 0.5f, 0.4f, 0.3f }, { 0.514056913328f, 0.152618314084f, 0.045310838527f } } },
    { "ACEScct_to_ACES2065-1",
        { { 0.5f, 0.4f, 0.3f }, { 0.386397222658f, 0.158557251811f, 0.043152537925f } } },
    { "ACEScc_to_ACES2065-1",
        { { 0.5f, 0.4f, 0.3f }, { 0.386397222658f, 0.158557251811f, 0.043152537925f } } },
    { "ACEScg_to_ACES2065-1",
        { { 0.5f, 0.4f, 0.3f }, { 0.453158317919f, 0.394926024520f, 0.299297344519f } } },
    { "ACESproxy10i_to_ACES2065-1",
        { { 0.5f, 0.4f, 0.3f }, { 0.433437174444f, 0.151629880817f, 0.031769555400f } } },
    { "ADX10_to_ACES2065-1",
        { { 0.5f, 0.4f, 0.3f }, { 0.210518101020f, 0.148655364394f, 0.085189053481f } } },
    { "ADX16_to_ACES2065-1",
        { { 0.125f, 0.1f, 0.075f }, { 0.211320835792f, 0.149169650771f, 0.085452970479f } } },
    { "ARRI_ALEXA-LOGC-EI800-AWG_to_ACES2065-1",
        { { 0.5f, 0.4f, 0.3f }, { 0.401621427766f, 0.236455447604f,  0.064830001192f } } },
    { "CANON_CLOG2-CGAMUT_to_ACES2065-1",
        { { 0.5f, 0.4f, 0.3f }, { 0.408435767126f, 0.197486903378f,  0.034204558318f } } },
    { "CANON_CLOG3-CGAMUT_to_ACES2065-1",
        { { 0.5f, 0.4f, 0.3f }, { 0.496034919950f, 0.301015360499f,  0.083691829261f } } },
    { "PANASONIC_VLOG-VGAMUT_to_ACES2065-1",
        { { 0.5f, 0.4f, 0.3f }, { 0.306918773245f, 0.148128050597f,  0.046334439047f } } },
    { "RED_REDLOGFILM-RWG_to_ACES2065-1",
        { { 0.5f, 0.4f, 0.3f }, { 0.216116808829f, 0.121529105934f,  0.008171766322f } } },
    { "RED_LOG3G10-RWG_to_ACES2065-1",
        { { 0.5f, 0.4f, 0.3f }, { 0.887988237100f, 0.416932247547f, -0.025442210717f } } },
    { "SONY_SLOG3-SGAMUT3_to_ACES2065-1",
        { { 0.5f, 0.4f, 0.3f }, { 0.342259707137f, 0.172043362337f,  0.057188031769f } } },
    { "SONY_SLOG3-SGAMUT3.CINE_to_ACES2065-1",
        { { 0.5f, 0.4f, 0.3f }, { 0.314942672433f, 0.170408017753f,  0.046854940520f } } }
};

} // anon.

OCIO_ADD_TEST(Builtins, validate)
{
    OCIO::ConstBuiltinTransformRegistryRcPtr reg = OCIO::BuiltinTransformRegistry::Get();

    for (size_t index = 0; index < reg->getNumBuiltins(); ++index)
    {
        const char * name = reg->getBuiltinStyle(index);
        const auto values = UnitTestValues[name];

        if (values.first.empty() || values.second.empty())
        {
            std::ostringstream errorMsg;
            errorMsg << "For the built-in transform '" << name << "' the values are missing.";
            OCIO_CHECK_ASSERT_MESSAGE(0, errorMsg.str());
        }
        else if (values.first.size() != values.second.size())
        {
            std::ostringstream errorMsg;
            errorMsg << "For the built-in transform '" << name 
                     << "' the input and output values do not match.";
            OCIO_CHECK_ASSERT_MESSAGE(0, errorMsg.str());
        }
        else if ((values.first.size() % 3) != 0)
        {
            std::ostringstream errorMsg;
            errorMsg << "For the built-in transform '" << name 
                     << "' only RGB values are supported.";
            OCIO_CHECK_ASSERT_MESSAGE(0, errorMsg.str());
        }
        else
        {
            ValidateBuiltinTransform(name, values.first, values.second, __LINE__);
        }
    }
}

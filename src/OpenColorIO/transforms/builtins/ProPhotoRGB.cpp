// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cmath>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/gamma/GammaOp.h"
#include "ops/matrix/MatrixOp.h"
#include "transforms/builtins/ACES.h"
#include "transforms/builtins/BuiltinTransformRegistry.h"
#include "transforms/builtins/ColorMatrixHelpers.h"
#include "transforms/builtins/OpHelpers.h"
#include "transforms/builtins/ProPhotoRGB.h"


namespace OCIO_NAMESPACE
{

	// ProPhoto RGB / ROMM RGB (Reference Output Medium Metric RGB)
	// Specified in ANSI/I3A IT10.7666:2003
	//
	// Primaries and white point.
	namespace ROMM_RGB
	{

		static const Chromaticities red_xy(0.7347, 0.2653);
		static const Chromaticities grn_xy(0.1596, 0.8404);
		static const Chromaticities blu_xy(0.0366, 0.0001);
		static const Chromaticities wht_xy(0.3457, 0.3585);  // D50

		const Primaries primaries(red_xy, grn_xy, blu_xy, wht_xy);

	} // namespace ROMM_RGB


	// ROMM RGB uses a piecewise gamma function with gamma 1.8.
	//
	// Encoded to Linear (decoding):
	//   if (encoded < 1.0 / 512.0):    // breakpoint = 16 * 1.0 / 512.0
	//       linear = encoded / 16.0
	//   else:
	//       linear = encoded ^ 1.8
	//
	// Linear to Encoded (encoding):
	//   if (linear < 1.0 / 512.0):
	//       encoded = linear * 16.0
	//   else:
	//       encoded = linear ^ (1/1.8)
	//
	namespace ROMM_RGB_GAMMA_18
	{

		static constexpr double gamma = 1.8;
		static constexpr double breakLinear = 1.0 / 512.0;	// Linear breakpoint.
		static constexpr double slope = 16.0;				// Slope of linear segment.
		static constexpr double breakEnc = 1.0 / 32.0;		// Encoded breakpoint

		void GenerateLinearToEncodedOps(OpRcPtrVec& ops)
		{
			// Linear to encoded gamma 1.8 curve using LUT for accuracy.
			auto GenerateLutValues = [](double in) -> float
				{
					const double absIn = std::abs(in);
					double out = 0.0;

					if (absIn < breakLinear)
					{
						out = absIn * slope;
					}
					else
					{
						out = std::pow(absIn, 1.0 / gamma);
					}

					return float(std::copysign(out, in));
				};

			CreateHalfLut(ops, GenerateLutValues);
		}

		void GenerateEncodedToLinearOps(OpRcPtrVec& ops)
		{
			// Encoded gamma 1.8 to linear curve using LUT for accuracy.
			auto GenerateLutValues = [](double in) -> float
				{
					const double absIn = std::abs(in);
					double out = 0.0;

					if (absIn < breakEnc)
					{
						out = absIn / slope;
					}
					else
					{
						out = std::pow(absIn, gamma);
					}

					return float(std::copysign(out, in));
				};

			CreateHalfLut(ops, GenerateLutValues);
		}

	} // namespace ROMM_RGB_GAMMA_18


	// ProPhoto RGB with sRGB gamma curve.
	// This is a common variant used by Adobe and other applications.
	// Uses the sRGB transfer function (gamma 2.4, offset 0.055) instead of
	// the standard ROMM RGB gamma 1.8 curve.
	namespace ROMM_RGB_SRGB_GAMMA
	{

		void GenerateLinearToEncodedOps(OpRcPtrVec& ops)
		{
			// sRGB gamma encoding: gamma=2.4, offset=0.055
			// This uses the MONCURVE model which is efficient for sRGB-style curves.
			const GammaOpData::Params rgbParams = { 2.4, 0.055 };
			const GammaOpData::Params alphaParams = { 1.0, 0.0 };
			auto gammaData = std::make_shared<GammaOpData>(GammaOpData::MONCURVE_FWD,
				rgbParams, rgbParams, rgbParams, alphaParams);
			CreateGammaOp(ops, gammaData, TRANSFORM_DIR_FORWARD);
		}

		void GenerateEncodedToLinearOps(OpRcPtrVec& ops)
		{
			// sRGB gamma decoding: gamma=2.4, offset=0.055
			const GammaOpData::Params rgbParams = { 2.4, 0.055 };
			const GammaOpData::Params alphaParams = { 1.0, 0.0 };
			auto gammaData = std::make_shared<GammaOpData>(GammaOpData::MONCURVE_REV,
				rgbParams, rgbParams, rgbParams, alphaParams);
			CreateGammaOp(ops, gammaData, TRANSFORM_DIR_FORWARD);
		}

	} // namespace ROMM_RGB_SRGB_GAMMA


	namespace PROPHOTO
	{

		void RegisterAll(BuiltinTransformRegistryImpl& registry) noexcept
		{
			// Linear ProPhoto RGB to ACES2065-1.
			{
				auto LINEAR_RIMM_to_ACES2065_1_BFD_Functor = [](OpRcPtrVec& ops)
					{
						// Convert from ROMM RGB (D50) to ACES AP0 (D60).
						// Uses Bradford chromatic adaptation.
						MatrixOpData::MatrixArrayPtr matrix
							= build_conversion_matrix(ROMM_RGB::primaries,
								ACES_AP0::primaries,
								ADAPTATION_BRADFORD);
						CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);
					};

				registry.addBuiltin("LINEAR-RIMM_to_ACES2065-1_BFD",
					"Convert ProPhoto RGB (linear) to ACES2065-1",
					LINEAR_RIMM_to_ACES2065_1_BFD_Functor);
			}

			// Encoded ProPhoto RGB (gamma 1.8) to ACES2065-1.
			{
				auto ROMM_to_CIE_XYZ_D65_BFD_Functor = [](OpRcPtrVec& ops)
					{
						// 1. Decode gamma 1.8 to linear.
						ROMM_RGB_GAMMA_18::GenerateEncodedToLinearOps(ops);

						// 2. Convert color space from ROMM RGB (D50) to ACES AP0 (D60).
						MatrixOpData::MatrixArrayPtr matrix
							= build_conversion_matrix(ROMM_RGB::primaries,
								ACES_AP0::primaries,
								ADAPTATION_BRADFORD);
						CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);
					};

				registry.addBuiltin("ROMM_to_CIE-XYZ-D65_BFD",
					"Convert ProPhoto RGB (gamma 1.8 encoded) to ACES2065-1",
					ROMM_to_CIE_XYZ_D65_BFD_Functor);
			}

			// ProPhoto RGB with sRGB gamma to ACES2065-1.
			{
				auto ROMM_RGB_SRGB_to_ACES2065_1_Functor = [](OpRcPtrVec& ops)
					{
						// 1. Decode sRGB gamma to linear.
						ROMM_RGB_SRGB_GAMMA::GenerateEncodedToLinearOps(ops);

						// 2. Convert color space from ROMM RGB (D50) to ACES AP0 (D60).
						MatrixOpData::MatrixArrayPtr matrix
							= build_conversion_matrix(ROMM_RGB::primaries,
								ACES_AP0::primaries,
								ADAPTATION_BRADFORD);
						CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);
					};

				registry.addBuiltin("PROPHOTO-RGB-SRGB-GAMMA_to_ACES2065-1",
					"Convert ProPhoto RGB (sRGB gamma encoded) to ACES2065-1",
					ROMM_RGB_SRGB_to_ACES2065_1_Functor);
			}

		}

	} // namespace PROPHOTO

} // namespace OCIO_NAMESPACE

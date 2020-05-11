# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

include(ExternalProject)

# Define used for tests in tests/cpu/Context_tests.cpp
add_definitions("-DOCIO_SOURCE_DIR=${CMAKE_SOURCE_DIR}")

function(add_ocio_test NAME SOURCES PRIVATE_INCLUDES)
	set(TEST_BINARY "test_${NAME}_exec")
	set(TEST_NAME "test_${NAME}")
	add_executable(${TEST_BINARY} ${SOURCES})
	target_compile_definitions(${TEST_BINARY}
		PRIVATE
			OpenColorIO_SKIP_IMPORTS

	)
	target_link_libraries(${TEST_BINARY}
		PUBLIC
			public_api
		PRIVATE
			expat::expat
			ilmbase::ilmbase
			pystring::pystring
			sampleicc::sampleicc
			unittest_data
			utils::strings
			yamlcpp::yamlcpp
			testutils
	)
	if(PRIVATE_INCLUDES)
		target_include_directories(${TEST_BINARY}
			PRIVATE
				"${CMAKE_SOURCE_DIR}/src/OpenColorIO"
				"${CMAKE_SOURCE_DIR}/tests/cpu"
		)
	endif(PRIVATE_INCLUDES)
	if(OCIO_USE_SSE)
		target_compile_definitions(${TEST_BINARY}
			PRIVATE
				USE_SSE
		)
	endif(OCIO_USE_SSE)
	if(WIN32)
		# A windows application linking to eXpat static libraries must
		# have the global macro XML_STATIC defined
		target_compile_definitions(${TEST_BINARY}
			PRIVATE
				XML_STATIC
		)
	endif(WIN32)
	set_target_properties(${TEST_BINARY} PROPERTIES
		COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

	add_test(${TEST_NAME} ${TEST_BINARY})
endfunction(add_ocio_test)

# Eventually we will factor out each test into it's own executable
# but for now, we will maintain the status quo and copy all from the
# OpenColorIO target
set(SOURCES
	Caching.cpp
	Display.cpp
	fileformats/cdl/CDLParser.cpp
	fileformats/cdl/CDLReaderHelper.cpp
	fileformats/ctf/CTFReaderHelper.cpp
	fileformats/ctf/CTFReaderUtils.cpp
	fileformats/xmlutils/XMLReaderHelper.cpp
	fileformats/xmlutils/XMLWriterUtils.cpp
	GPUProcessor.cpp
	GpuShaderDesc.cpp
	HashUtils.cpp
	ImageDesc.cpp
	ImagePacking.cpp
	Look.cpp
	md5/md5.cpp
	OCIOYaml.cpp
	ops/cdl/CDLOpCPU.cpp
	ops/cdl/CDLOpGPU.cpp
	ops/exposurecontrast/ExposureContrastOpGPU.cpp
	ops/fixedfunction/FixedFunctionOpGPU.cpp
	ops/gamma/GammaOpGPU.cpp
	ops/log/LogOpGPU.cpp
	ops/lut3d/Lut3DOpGPU.cpp
	ops/matrix/MatrixOpGPU.cpp
	ops/OpTools.cpp
	ops/range/RangeOpGPU.cpp
	ScanlineHelper.cpp
	Transform.cpp
	transforms/LookTransform.cpp
)

set(TESTS
	Baker_tests.cpp
	BitDepthUtils_tests.cpp
	ColorSpace_tests.cpp
	ColorSpaceSet_tests.cpp
	Config_tests.cpp
	Context_tests.cpp
	CPUProcessor_tests.cpp
	DynamicProperty_tests.cpp
	Exception_tests.cpp
	fileformats/ctf/CTFTransform_tests.cpp
	fileformats/ctf/IndexMapping_tests.cpp
	fileformats/FileFormat3DL_tests.cpp
	fileformats/FileFormatCC_tests.cpp
	fileformats/FileFormatCCC_tests.cpp
	fileformats/FileFormatCDL_tests.cpp
	fileformats/FileFormatCSP_tests.cpp
	fileformats/FileFormatCTF_tests.cpp
	fileformats/FileFormatDiscreet1DL_tests.cpp
	fileformats/FileFormatHDL_tests.cpp
	fileformats/FileFormatICC_tests.cpp
	fileformats/FileFormatIridasCube_tests.cpp
	fileformats/FileFormatIridasItx_tests.cpp
	fileformats/FileFormatIridasLook_tests.cpp
	fileformats/FileFormatPandora_tests.cpp
	fileformats/FileFormatResolveCube_tests.cpp
	fileformats/FileFormatSpi1D_tests.cpp
	fileformats/FileFormatSpi3D_tests.cpp
	fileformats/FileFormatSpiMtx_tests.cpp
	fileformats/FileFormatTruelight_tests.cpp
	fileformats/FileFormatVF_tests.cpp
	fileformats/FormatMetadata_tests.cpp
	fileformats/xmlutils/XMLReaderUtils_tests.cpp
	FileRules_tests.cpp
	GpuShader_tests.cpp
	GpuShaderUtils_tests.cpp
	Logging_tests.cpp
	LookParse_tests.cpp
	MathUtils_tests.cpp
	Op_tests.cpp
	OpOptimizers_tests.cpp
	ops/allocation/AllocationOp_tests.cpp
	ops/cdl/CDLOpData_tests.cpp
	ops/cdl/CDLOp_tests.cpp
	ops/exponent/ExponentOp_tests.cpp
	ops/exposurecontrast/ExposureContrastOpCPU_tests.cpp
	ops/exposurecontrast/ExposureContrastOpData_tests.cpp
	ops/exposurecontrast/ExposureContrastOp_tests.cpp
	ops/fixedfunction/FixedFunctionOpCPU_tests.cpp
	ops/fixedfunction/FixedFunctionOpData_tests.cpp
	ops/fixedfunction/FixedFunctionOp_tests.cpp
	ops/gamma/GammaOp_tests.cpp
	ops/gamma/GammaOpCPU_tests.cpp
	ops/gamma/GammaOpData_tests.cpp
	ops/gamma/GammaOpUtils_tests.cpp
	ops/log/LogOpCPU_tests.cpp
	ops/log/LogOpData_tests.cpp
	ops/log/LogOp_tests.cpp
	ops/log/LogUtils_tests.cpp
	ops/lut1d/Lut1DOp_tests.cpp
	ops/lut1d/Lut1DOpCPU_tests.cpp
	ops/lut1d/Lut1DOpData_tests.cpp
	ops/lut1d/Lut1DOpGPU_tests.cpp
	ops/lut3d/Lut3DOp_tests.cpp
	ops/lut3d/Lut3DOpCPU_tests.cpp
	ops/lut3d/Lut3DOpData_tests.cpp
	ops/matrix/MatrixOpCPU_tests.cpp
	ops/matrix/MatrixOpData_tests.cpp
	ops/matrix/MatrixOp_tests.cpp
	ops/noop/NoOps_tests.cpp
	ops/range/RangeOpCPU_tests.cpp
	ops/range/RangeOpData_tests.cpp
	ops/range/RangeOp_tests.cpp
	ops/reference/ReferenceOpData_tests.cpp
	ParseUtils_tests.cpp
	PathUtils_tests.cpp
	Platform_tests.cpp
	Processor_tests.cpp
	SSE_tests.cpp
	transforms/FileTransform_tests.cpp
	transforms/FixedFunctionTransform_tests.cpp
	transforms/RangeTransform_tests.cpp
	transforms/AllocationTransform_tests.cpp
	transforms/CDLTransform_tests.cpp
	transforms/ColorSpaceTransform_tests.cpp
	transforms/DisplayTransform_tests.cpp
	transforms/ExponentTransform_tests.cpp
	transforms/ExponentWithLinearTransform_tests.cpp
	transforms/ExposureContrastTransform_tests.cpp
	transforms/FixedFunctionTransform_tests.cpp
	transforms/GroupTransform_tests.cpp
	transforms/LogAffineTransform_tests.cpp
	transforms/LogCameraTransform_tests.cpp
	transforms/LogTransform_tests.cpp
	transforms/Lut1DTransform_tests.cpp
	transforms/Lut3DTransform_tests.cpp
	transforms/MatrixTransform_tests.cpp
	UnitTestLogUtils.cpp
	UnitTestMain.cpp
	UnitTestOptimFlags.cpp
	UnitTestUtils.cpp
	ViewTransform_tests.cpp
)

function(prepend var prefix)
	set(new "")
	foreach(f ${ARGN})
		list(APPEND new "${prefix}${f}")
	endforeach(f)
	set(${var} "${new}" PARENT_SCOPE)
endfunction(prepend)

prepend(SOURCES "${CMAKE_SOURCE_DIR}/src/OpenColorIO/" ${SOURCES})

list(APPEND SOURCES ${TESTS})

add_ocio_test(cpu "${SOURCES}" TRUE)

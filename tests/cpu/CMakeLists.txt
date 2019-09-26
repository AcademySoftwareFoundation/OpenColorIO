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
			OCIO_UNIT_TEST
			OpenColorIO_SKIP_IMPORTS

	)
	target_link_libraries(${TEST_BINARY}
		PUBLIC
			public_api
		PRIVATE
			yamlcpp::yamlcpp
			pystring::pystring
			sampleicc::sampleicc
			unittest_data
			expat::expat
			ilmbase::ilmbase
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
	Baker.cpp
	BitDepthUtils.cpp
	Caching.cpp
	ColorSpace.cpp
	ColorSpaceSet.cpp
	Config.cpp
	CPUProcessor.cpp
	Display.cpp
	DynamicProperty.cpp
	Exception.cpp
	fileformats/cdl/CDLParser.cpp
	fileformats/cdl/CDLReaderHelper.cpp
	fileformats/ctf/CTFReaderHelper.cpp
	fileformats/ctf/CTFReaderUtils.cpp
	fileformats/FileFormatCCC.cpp
	fileformats/FileFormatCC.cpp
	fileformats/FileFormatCDL.cpp
	fileformats/FileFormatCSP.cpp
	fileformats/FileFormatDiscreet1DL.cpp
	fileformats/FileFormatHDL.cpp
	fileformats/FileFormatICC.cpp
	fileformats/FileFormatIridasCube.cpp
	fileformats/FileFormatIridasItx.cpp
	fileformats/FileFormatIridasLook.cpp
	fileformats/FileFormatPandora.cpp
	fileformats/FileFormatResolveCube.cpp
	fileformats/FileFormatSpi1D.cpp
	fileformats/FileFormatSpi3D.cpp
	fileformats/FileFormatSpiMtx.cpp
	fileformats/FileFormatTruelight.cpp
	fileformats/FileFormatVF.cpp
	fileformats/FormatMetadata.cpp
	fileformats/xmlutils/XMLReaderHelper.cpp
	fileformats/xmlutils/XMLReaderUtils.cpp
	fileformats/xmlutils/XMLWriterUtils.cpp
	GPUProcessor.cpp
	GpuShader.cpp
	GpuShaderDesc.cpp
	GpuShaderUtils.cpp
	HashUtils.cpp
	ImageDesc.cpp
	ImagePacking.cpp
	Logging.cpp
	Look.cpp
	LookParse.cpp
	MathUtils.cpp
	md5/md5.cpp
	OCIOYaml.cpp
	Op.cpp
	OpOptimizers.cpp
	ops/Allocation/AllocationOp.cpp
	ops/CDL/CDLOpCPU.cpp
	ops/CDL/CDLOpData.cpp
	ops/CDL/CDLOps.cpp
	ops/Exponent/ExponentOps.cpp
	ops/exposurecontrast/ExposureContrastOpCPU.cpp
	ops/exposurecontrast/ExposureContrastOpData.cpp
	ops/exposurecontrast/ExposureContrastOpGPU.cpp
	ops/exposurecontrast/ExposureContrastOps.cpp
	ops/FixedFunction/FixedFunctionOpCPU.cpp
	ops/FixedFunction/FixedFunctionOpData.cpp
	ops/FixedFunction/FixedFunctionOpGPU.cpp
	ops/FixedFunction/FixedFunctionOps.cpp
	ops/Gamma/GammaOpCPU.cpp
	ops/Gamma/GammaOpData.cpp
	ops/Gamma/GammaOpUtils.cpp
	ops/Gamma/GammaOps.cpp
	ops/IndexMapping.cpp
	ops/Log/LogOpCPU.cpp
	ops/Log/LogOpData.cpp
	ops/Log/LogOpGPU.cpp
	ops/Log/LogOps.cpp
	ops/Log/LogUtils.cpp
	ops/Lut1D/Lut1DOp.cpp
	ops/Lut1D/Lut1DOpCPU.cpp
	ops/Lut1D/Lut1DOpData.cpp
	ops/Lut1D/Lut1DOpGPU.cpp
	ops/Lut3D/Lut3DOp.cpp
	ops/Lut3D/Lut3DOpCPU.cpp
	ops/Lut3D/Lut3DOpData.cpp
	ops/Lut3D/Lut3DOpGPU.cpp
	ops/Matrix/MatrixOpCPU.cpp
	ops/Matrix/MatrixOpData.cpp
	ops/Matrix/MatrixOps.cpp
	ops/NoOp/NoOps.cpp
	ops/Range/RangeOpCPU.cpp
	ops/Range/RangeOpData.cpp
	ops/Range/RangeOpGPU.cpp
	ops/Range/RangeOps.cpp
	ops/reference/ReferenceOpData.cpp
	OpTools.cpp
	ParseUtils.cpp
	PathUtils.cpp
	Platform.cpp
	ScanlineHelper.cpp
	SSE.cpp
	Transform.cpp
	transforms/AllocationTransform.cpp
	transforms/CDLTransform.cpp
	transforms/ColorSpaceTransform.cpp
	transforms/DisplayTransform.cpp
	transforms/ExponentTransform.cpp
	transforms/ExponentWithLinearTransform.cpp
	transforms/ExposureContrastTransform.cpp
	transforms/FixedFunctionTransform.cpp
	transforms/GroupTransform.cpp
	transforms/LogAffineTransform.cpp
	transforms/LogTransform.cpp
	transforms/LookTransform.cpp
	transforms/LUT1DTransform.cpp
	transforms/LUT3DTransform.cpp
	transforms/MatrixTransform.cpp
	transforms/RangeTransform.cpp
)

set(TESTS
	Context_tests.cpp
	fileformats/ctf/CTFTransform_tests.cpp
	fileformats/FileFormat3DL_tests.cpp
	fileformats/FileFormatCTF_tests.cpp
	Processor_tests.cpp
	transforms/FileTransform_tests.cpp
	UnitTest.cpp
	UnitTestUtils.cpp
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

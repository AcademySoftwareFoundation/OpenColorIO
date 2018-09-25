include(ExternalProject)

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
			tinyxml
			yamlcpp
			pystring
			sampleicc
			oiio_unittest
			unittest_data
	)
	if(PRIVATE_INCLUDES)
		target_include_directories(${TEST_BINARY}
			PRIVATE
				"${CMAKE_SOURCE_DIR}/src/OpenColorIO"
		)
	endif(PRIVATE_INCLUDES)
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
	Config.cpp
	Context.cpp
	Display.cpp
	Exception.cpp
	fileformats/FileFormat3DL.cpp
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
	ops/Exponent/ExponentOps.cpp
	ops/Log/LogOps.cpp
	ops/Lut1D/Lut1DOp.cpp
	ops/Lut3D/Lut3DOp.cpp
	ops/Matrix/MatrixOps.cpp
	ops/NoOp/NoOps.cpp
	ops/Truelight/TruelightOp.cpp
	OpTools.cpp
	ParseUtils.cpp
	PathUtils.cpp
	Platform.cpp
	Processor.cpp
	ScanlineHelper.cpp
	Transform.cpp
	transforms/AllocationTransform.cpp
	transforms/CDLTransform.cpp
	transforms/ColorSpaceTransform.cpp
	transforms/DisplayTransform.cpp
	transforms/ExponentTransform.cpp
	transforms/FileTransform.cpp
	transforms/GroupTransform.cpp
	transforms/LogTransform.cpp
	transforms/LookTransform.cpp
	transforms/MatrixTransform.cpp
	transforms/TruelightTransform.cpp
	UnitTest.cpp
)

function(prepend var prefix)
	set(new "")
	foreach(f ${ARGN})
		list(APPEND new "${prefix}${f}")
	endforeach(f)
	set(${var} "${new}" PARENT_SCOPE)
endfunction(prepend)

prepend(SOURCES "${CMAKE_SOURCE_DIR}/src/OpenColorIO/" ${SOURCES})

add_ocio_test(cpu "${SOURCES}" TRUE)

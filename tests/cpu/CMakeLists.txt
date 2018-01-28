include(ExternalProject)

function(add_ocio_test NAME SOURCES PRIVATE_INCLUDES)
	set(TEST_BINARY "test_${NAME}_exec")
	set(TEST_NAME "test_${NAME}")
	add_executable(${TEST_BINARY} ${SOURCES})
	target_compile_definitions(${TEST_BINARY}
		PRIVATE
			-DOCIO_UNIT_TEST
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
	AllocationOp.cpp
	AllocationTransform.cpp
	Baker.cpp
	BitDepthUtils.cpp
	Caching.cpp
	CDLTransform.cpp
	ColorSpace.cpp
	ColorSpaceTransform.cpp
	Config.cpp
	Context.cpp
	Display.cpp
	DisplayTransform.cpp
	Exception.cpp
	ExponentOps.cpp
	ExponentTransform.cpp
	FileFormat3DL.cpp
	FileFormatCCC.cpp
	FileFormatCC.cpp
	FileFormatCDL.cpp
	FileFormatCSP.cpp
	FileFormatDiscreet1DL.cpp
	FileFormatHDL.cpp
	FileFormatICC.cpp
	FileFormatIridasCube.cpp
	FileFormatIridasItx.cpp
	FileFormatIridasLook.cpp
	FileFormatPandora.cpp
	FileFormatResolveCube.cpp
	FileFormatSpi1D.cpp
	FileFormatSpi3D.cpp
	FileFormatSpiMtx.cpp
	FileFormatTruelight.cpp
	FileFormatVF.cpp
	FileTransform.cpp
	GpuShader.cpp
	GpuShaderDesc.cpp
	GpuShaderUtils.cpp
	GroupTransform.cpp
	HashUtils.cpp
	ImageDesc.cpp
	ImagePacking.cpp
	Logging.cpp
	LogOps.cpp
	LogTransform.cpp
	Look.cpp
	LookParse.cpp
	LookTransform.cpp
	Lut1DOp.cpp
	Lut3DOp.cpp
	MathUtils.cpp
	MatrixOps.cpp
	MatrixTransform.cpp
	md5/md5.cpp
	NoOps.cpp
	OCIOYaml.cpp
	Op.cpp
	OpOptimizers.cpp
	OpTools.cpp
	ParseUtils.cpp
	PathUtils.cpp
	Platform.cpp
	Processor.cpp
	ScanlineHelper.cpp
	Transform.cpp
	TruelightOp.cpp
	TruelightTransform.cpp
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

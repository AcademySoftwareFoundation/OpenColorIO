set(SOURCES
	Baker.cpp
	BitDepthUtils.cpp
	Caching.cpp
	ColorSpace.cpp
	Config.cpp
	Context.cpp
	Display.cpp
	Exception.cpp
	fileformats/cdl/CDLParser.cpp
	fileformats/cdl/CDLReaderHelper.cpp
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
	ops/CDL/CDLOpCPU.cpp
	ops/CDL/CDLOpData.cpp
	ops/CDL/CDLOps.cpp
	ops/Exponent/ExponentOps.cpp
	ops/Log/LogOps.cpp
	ops/Lut1D/Lut1DOp.cpp
	ops/Lut3D/Lut3DOp.cpp
	ops/Lut3D/Lut3DOpCPU.cpp
	ops/Lut3D/Lut3DOpData.cpp
	ops/Matrix/MatrixOps.cpp
	ops/NoOp/NoOps.cpp
	ops/Range/RangeOpCPU.cpp
	ops/Range/RangeOpData.cpp
	ops/Range/RangeOps.cpp
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
	transforms/RangeTransform.cpp
	transforms/TruelightTransform.cpp
	UnitTest.cpp
)

add_library(OpenColorIO ${SOURCES})
target_include_directories(OpenColorIO
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(OpenColorIO 
	PUBLIC 
		public_api
	PRIVATE
		yamlcpp
		pystring
		sampleicc
		libexpat
		ilmbase
)
if(NOT BUILD_SHARED_LIBS)
	target_compile_definitions(OpenColorIO
		PRIVATE
		OpenColorIO_SKIP_IMPORTS
	)
else()
	target_compile_definitions(OpenColorIO
		PRIVATE
		OpenColorIO_EXPORTS
	)
endif()

if(OCIO_USE_SSE)
	target_compile_definitions(OpenColorIO
		PRIVATE
			USE_SSE
	)
endif()
if(WIN32)
	# A windows application linking to eXpat static libraries must
    # have the global macro XML_STATIC defined
	target_compile_definitions(OpenColorIO
		PRIVATE
			XML_STATIC
	)
endif()
if(MSVC)
	# Because our Exceptions derive from std::runtime_error we can safely disable this warning
	set_target_properties(OpenColorIO PROPERTIES COMPILE_FLAGS "/wd4275")
endif()

set_target_properties(OpenColorIO PROPERTIES
    VERSION ${OpenColorIO_VERSION}
    SOVERSION ${SOVERSION})

install(TARGETS OpenColorIO
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION lib
)

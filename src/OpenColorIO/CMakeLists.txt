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

add_library(OpenColorIO ${SOURCES})
set_property(TARGET OpenColorIO PROPERTY CXX_STANDARD 11)
target_include_directories(OpenColorIO
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(OpenColorIO 
	PUBLIC 
		public_api
	PRIVATE
		tinyxml
		yamlcpp
		pystring
		sampleicc
)

target_compile_definitions(OpenColorIO
	PRIVATE
		OpenColorIO_EXPORTS
)
if(OCIO_USE_SSE)
	target_compile_definitions(OpenColorIO
		PRIVATE
			USE_SSE
	)
endif()
if(MSVC)
	# Because our Exceptions derive from std::runtime_error we can safely disable this warning
	set_target_properties(OpenColorIO PROPERTIES COMPILE_FLAGS "/wd4275")
endif()

install(TARGETS OpenColorIO
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION lib
	ARCHIVE DESTINATION lib
)

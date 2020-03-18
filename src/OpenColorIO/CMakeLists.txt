# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
	Baker.cpp
	BitDepthUtils.cpp
	Caching.cpp
	ColorSpace.cpp
	ColorSpaceSet.cpp
	Config.cpp
	Context.cpp
	CPUProcessor.cpp
	Display.cpp
	DynamicProperty.cpp
	Exception.cpp
	fileformats/cdl/CDLParser.cpp
	fileformats/cdl/CDLReaderHelper.cpp
	fileformats/ctf/CTFReaderHelper.cpp
	fileformats/ctf/CTFReaderUtils.cpp
	fileformats/ctf/CTFTransform.cpp
	fileformats/ctf/IndexMapping.cpp
	fileformats/FileFormat3DL.cpp
	fileformats/FileFormatCCC.cpp
	fileformats/FileFormatCC.cpp
	fileformats/FileFormatCDL.cpp
	fileformats/FileFormatCSP.cpp
	fileformats/FileFormatCTF.cpp
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
	FileRules.cpp
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
	ops/allocation/AllocationOp.cpp
	ops/cdl/CDLOpCPU.cpp
	ops/cdl/CDLOpData.cpp
	ops/cdl/CDLOpGPU.cpp
	ops/cdl/CDLOp.cpp
	ops/exponent/ExponentOp.cpp
	ops/exposurecontrast/ExposureContrastOpCPU.cpp
	ops/exposurecontrast/ExposureContrastOpData.cpp
	ops/exposurecontrast/ExposureContrastOpGPU.cpp
	ops/exposurecontrast/ExposureContrastOp.cpp
	ops/fixedfunction/FixedFunctionOpCPU.cpp
	ops/fixedfunction/FixedFunctionOpData.cpp
	ops/fixedfunction/FixedFunctionOpGPU.cpp
	ops/fixedfunction/FixedFunctionOp.cpp
	ops/gamma/GammaOpCPU.cpp
	ops/gamma/GammaOpData.cpp
	ops/gamma/GammaOpGPU.cpp
	ops/gamma/GammaOpUtils.cpp
	ops/gamma/GammaOp.cpp
	ops/log/LogOpCPU.cpp
	ops/log/LogOpData.cpp
	ops/log/LogOpGPU.cpp
	ops/log/LogOp.cpp
	ops/log/LogUtils.cpp
	ops/lut1d/Lut1DOp.cpp
	ops/lut1d/Lut1DOpCPU.cpp
	ops/lut1d/Lut1DOpData.cpp
	ops/lut1d/Lut1DOpGPU.cpp
	ops/lut3d/Lut3DOp.cpp
	ops/lut3d/Lut3DOpCPU.cpp
	ops/lut3d/Lut3DOpData.cpp
	ops/lut3d/Lut3DOpGPU.cpp
	ops/matrix/MatrixOpCPU.cpp
	ops/matrix/MatrixOpData.cpp
	ops/matrix/MatrixOpGPU.cpp
	ops/matrix/MatrixOp.cpp
	ops/noop/NoOps.cpp
	ops/OpTools.cpp
	ops/range/RangeOpCPU.cpp
	ops/range/RangeOpData.cpp
	ops/range/RangeOpGPU.cpp
	ops/range/RangeOp.cpp
	ops/reference/ReferenceOpData.cpp
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
	transforms/ExponentWithLinearTransform.cpp
	transforms/ExposureContrastTransform.cpp
	transforms/FileTransform.cpp
	transforms/FixedFunctionTransform.cpp
	transforms/GroupTransform.cpp
	transforms/LogAffineTransform.cpp
	transforms/LogCameraTransform.cpp
	transforms/LogTransform.cpp
	transforms/LookTransform.cpp
	transforms/Lut1DTransform.cpp
	transforms/Lut3DTransform.cpp
	transforms/MatrixTransform.cpp
	transforms/RangeTransform.cpp
	ViewTransform.cpp
)

if(WIN32 AND BUILD_SHARED_LIBS)

    # Impose a versioned name on Windows to avoid binary name clashes
    # between different versions of the same library.

    # Note: Do not append the PATCH to allow Apps to use the
    #       latest binary compatible library.
    set(LIBNAME_SUFFIX
        _${OpenColorIO_VERSION_MAJOR}_${OpenColorIO_VERSION_MINOR}
    )

    # Create the version.rc file for the Windows DLL.
    configure_file(res/version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/version.rc @ONLY)
    set(SOURCES ${SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
endif()

add_library(OpenColorIO ${SOURCES})

target_include_directories(OpenColorIO
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(OpenColorIO
	PUBLIC
		public_api
	PRIVATE
		expat::expat
		ilmbase::ilmbase
		pystring::pystring
		sampleicc::sampleicc
		utils::strings
		yamlcpp::yamlcpp
)

if(NOT BUILD_SHARED_LIBS)
	target_compile_definitions(OpenColorIO
		PRIVATE
			OpenColorIO_SKIP_IMPORTS
	)
else()
	# Only exports selected symbols.
	target_compile_definitions(OpenColorIO
		PRIVATE
			OpenColorIO_EXPORTS
	)
endif()

if(BUILD_SHARED_LIBS OR (OCIO_BUILD_PYTHON AND UNIX))
	# The Python bindings is a 'module' which is a dynamic library on Linux
	# (i.e. '-fPIC' needed), but a static library on Windows.

	# If supported for the target machine, emit position-independent code
	# suitable for dynamic linking.
	set_property(TARGET OpenColorIO PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

if(OCIO_USE_SSE)
	target_compile_definitions(OpenColorIO
		PRIVATE
			USE_SSE
	)
endif()

if(MSVC AND BUILD_TYPE_DEBUG AND BUILD_SHARED_LIBS)
    set_target_properties(OpenColorIO PROPERTIES
        PDB_NAME ${PROJECT_NAME}_${LIBNAME_SUFFIX}
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

set_target_properties(OpenColorIO PROPERTIES
    OUTPUT_NAME   OpenColorIO${OCIO_LIBNAME_SUFFIX}${LIBNAME_SUFFIX}
    COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}"
    VERSION       ${OpenColorIO_VERSION}
    SOVERSION     ${SOVERSION})

if(MSVC AND BUILD_SHARED_LIBS)
	# Install the pdb file if any.
	install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION bin OPTIONAL)
endif()

install(TARGETS OpenColorIO
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION lib
)

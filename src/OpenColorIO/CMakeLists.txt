# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
	apphelpers/CategoryHelpers.cpp
	apphelpers/ColorSpaceHelpers.cpp
	apphelpers/DisplayViewHelpers.cpp
	apphelpers/LegacyViewingPipeline.cpp
	apphelpers/MixingHelpers.cpp
	Baker.cpp
	BitDepthUtils.cpp
	Caching.cpp
	ColorSpace.cpp
	ColorSpaceSet.cpp
	Config.cpp
	Context.cpp
	ContextVariableUtils.cpp
	CPUProcessor.cpp
	Display.cpp
	DynamicProperty.cpp
	Exception.cpp
	fileformats/cdl/CDLParser.cpp
	fileformats/cdl/CDLReaderHelper.cpp
	fileformats/cdl/CDLWriter.cpp
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
	fileformats/FileFormatUtils.cpp
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
	NamedTransform.cpp
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
	ops/gradingprimary/GradingPrimary.cpp
	ops/gradingprimary/GradingPrimaryOpCPU.cpp
	ops/gradingprimary/GradingPrimaryOpData.cpp
	ops/gradingprimary/GradingPrimaryOpGPU.cpp
	ops/gradingprimary/GradingPrimaryOp.cpp
	ops/gradingrgbcurve/GradingBSplineCurve.cpp
	ops/gradingrgbcurve/GradingRGBCurveOpCPU.cpp
	ops/gradingrgbcurve/GradingRGBCurveOpData.cpp
	ops/gradingrgbcurve/GradingRGBCurveOpGPU.cpp
	ops/gradingrgbcurve/GradingRGBCurveOp.cpp
	ops/gradingrgbcurve/GradingRGBCurve.cpp
	ops/gradingtone/GradingTone.cpp
	ops/gradingtone/GradingToneOpCPU.cpp
	ops/gradingtone/GradingToneOpData.cpp
	ops/gradingtone/GradingToneOpGPU.cpp
	ops/gradingtone/GradingToneOp.cpp
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
	transforms/builtins/ACES.cpp
	transforms/builtins/BuiltinTransformRegistry.cpp
	transforms/builtins/ColorMatrixHelpers.cpp
	transforms/builtins/OpHelpers.cpp
	transforms/BuiltinTransform.cpp
	transforms/CDLTransform.cpp
	transforms/ColorSpaceTransform.cpp
	transforms/DisplayViewTransform.cpp
	transforms/ExponentTransform.cpp
	transforms/ExponentWithLinearTransform.cpp
	transforms/ExposureContrastTransform.cpp
	transforms/FileTransform.cpp
	transforms/FixedFunctionTransform.cpp
	transforms/GradingPrimaryTransform.cpp
	transforms/GradingRGBCurveTransform.cpp
	transforms/GradingToneTransform.cpp
	transforms/GroupTransform.cpp
	transforms/LogAffineTransform.cpp
	transforms/LogCameraTransform.cpp
	transforms/LogTransform.cpp
	transforms/LookTransform.cpp
	transforms/Lut1DTransform.cpp
	transforms/Lut3DTransform.cpp
	transforms/MatrixTransform.cpp
	transforms/RangeTransform.cpp
	ViewingRules.cpp
	ViewTransform.cpp
	SystemMonitor.cpp
)

if(OCIO_ADD_EXTRA_BUILTINS)
	set(SOURCES_BUILTINS
		transforms/builtins/ArriCameras.cpp
		transforms/builtins/CanonCameras.cpp
		transforms/builtins/Displays.cpp
		transforms/builtins/PanasonicCameras.cpp
		transforms/builtins/RedCameras.cpp
		transforms/builtins/SonyCameras.cpp
	)

	list(INSERT SOURCES 0 ${SOURCES_BUILTINS})
else()
	message(WARNING "Disabling supplemental built-in transforms removes all built-in camera transforms, limiting OCIO configuration compatibility.")
endif()

if(NOT WIN32)

    # Install the pkg-config file.

    include(GNUInstallDirs)

    set(prefix ${CMAKE_INSTALL_PREFIX})
    set(exec_prefix "\${prefix}")
    set(libdir "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
    set(includedir "\${exec_prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
    configure_file(res/OpenColorIO.pc.in ${CMAKE_CURRENT_BINARY_DIR}/OpenColorIO.pc @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/OpenColorIO.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()

# TODO: Install the OpenColorIO-config.cmake.

add_library(OpenColorIO ${SOURCES})

if(BUILD_SHARED_LIBS AND WIN32)
	# Impose a versioned name on Windows to avoid binary name clashes.
    set(OCIO_LIBNAME_SUFFIX
    	${OCIO_LIBNAME_SUFFIX}_${OpenColorIO_VERSION_MAJOR}_${OpenColorIO_VERSION_MINOR}
    )

    # Create the version.rc file for the Windows DLL.
    configure_file(res/version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/version.rc @ONLY)
    set(SOURCES ${SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
endif()

target_include_directories(OpenColorIO
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(OpenColorIO
	PUBLIC
		public_api
	PRIVATE
		expat::expat
		IlmBase::Half
		pystring::pystring
		sampleicc::sampleicc
		utils::strings
		yaml-cpp
)

if(APPLE)
	target_link_libraries(OpenColorIO
		PRIVATE
			"-framework Carbon"
			"-framework IOKit"
	)
endif()

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

if(OCIO_ADD_EXTRA_BUILTINS)
	target_compile_definitions(OpenColorIO
		PRIVATE
			ADD_EXTRA_BUILTINS
	)
endif()

if(MSVC AND BUILD_TYPE_DEBUG AND BUILD_SHARED_LIBS)
    set_target_properties(OpenColorIO PROPERTIES
        PDB_NAME ${PROJECT_NAME}${OCIO_LIBNAME_SUFFIX}
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
    OUTPUT_NAME   ${PROJECT_NAME}${OCIO_LIBNAME_SUFFIX}
    COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}"
    VERSION       ${OpenColorIO_VERSION}
    SOVERSION     ${SOVERSION}
)

if(MSVC AND BUILD_SHARED_LIBS)
	# Install the pdb file if any.
	install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION bin OPTIONAL)
endif()

install(TARGETS OpenColorIO
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION lib
)

# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
    apphelpers/CategoryHelpers.cpp
    apphelpers/ColorSpaceHelpers.cpp
    apphelpers/DisplayViewHelpers.cpp
    apphelpers/LegacyViewingPipeline.cpp
    apphelpers/MixingHelpers.cpp
    Baker.cpp
    BakingUtils.cpp
    BitDepthUtils.cpp
    builtinconfigs/BuiltinConfigRegistry.cpp
    builtinconfigs/CGConfig.cpp
    builtinconfigs/StudioConfig.cpp
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
    GpuShaderClassWrapper.cpp
    GpuShaderUtils.cpp
    HashUtils.cpp
    ImageDesc.cpp
    ImagePacking.cpp
    Logging.cpp
    Look.cpp
    LookParse.cpp
    MathUtils.cpp
    NamedTransform.cpp
    OCIOYaml.cpp
    OCIOZArchive.cpp
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
    transforms/builtins/ArriCameras.cpp
    transforms/builtins/CanonCameras.cpp
    transforms/builtins/Displays.cpp
    transforms/builtins/PanasonicCameras.cpp
    transforms/builtins/RedCameras.cpp
    transforms/builtins/SonyCameras.cpp
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

if(NOT WIN32)

    # Install the pkg-config file.

    set(prefix ${CMAKE_INSTALL_PREFIX})
    set(exec_prefix "\${prefix}")
    set(libdir "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
    set(includedir "\${exec_prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
    configure_file(res/OpenColorIO.pc.in ${CMAKE_CURRENT_BINARY_DIR}/OpenColorIO.pc @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/OpenColorIO.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()

add_library(OpenColorIO ${SOURCES})

# Require at least a C++11 compatible compiler for consumer projects.
target_compile_features(OpenColorIO
    PUBLIC cxx_std_11
)

if(BUILD_SHARED_LIBS AND WIN32)
    # Only use the versioned name with the dynamic library.
    set(CMAKE_SHARED_LIBRARY_SUFFIX _${OpenColorIO_VERSION_MAJOR}_${OpenColorIO_VERSION_MINOR}.dll)

    # Create the version.rc file for the Windows DLL.
    configure_file(res/version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/version.rc @ONLY)
    set(SOURCES ${SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
endif()


set(HEADER_LOCATION "${CMAKE_SOURCE_DIR}/include/OpenColorIO")
file(GLOB_RECURSE INSTALL_HEADERS "${HEADER_LOCATION}/*.h")

# Some part of the build process expect to find the ABI header here,
# notably the documentation.
set(ABI_HEADER_LOCATION "${CMAKE_BINARY_DIR}/include/OpenColorIO")
list(APPEND INSTALL_HEADERS ${ABI_HEADER_LOCATION}/OpenColorABI.h)
configure_file("${HEADER_LOCATION}/OpenColorABI.h.in" "${ABI_HEADER_LOCATION}/OpenColorABI.h" @ONLY)

# Built-in configs
set(CONFIGS_HEADER_LOCATION "${CMAKE_BINARY_DIR}/generated_include")
file(GLOB CONFIGS_PATHS "builtinconfigs/configs/*.ocio")
# Loop through the .ocio config files.
foreach(C ${CONFIGS_PATHS})
    get_filename_component(CONFIG_FULLNAME "${C}" NAME)
    string(REGEX MATCH "^(.*)\\.ocio$" tempvar ${CONFIG_FULLNAME})
    set(CONFIG_NAME ${CMAKE_MATCH_1})
    list(APPEND CONFIGS_ALL ${CONFIG_NAME})

    file(READ builtinconfigs/configs/${CONFIG_FULLNAME} HEX_CONTENTS HEX)
    # Separate into individual bytes.
    string(REGEX MATCHALL "([A-Za-z0-9][A-Za-z0-9])" SEPARATED_HEX "${HEX_CONTENTS}")

    # Prepend the "0x" to each byte.
    list(JOIN SEPARATED_HEX ",0x" FORMATTED_HEX)

    # Prepend "0x" to the first byte.
    string(PREPEND FORMATTED_HEX "0x")

    # Append 0x00 to null-terminated the string.
    string(APPEND FORMATTED_HEX ",0x00")
    
    # Set the dynamically named variable "${CONFIG_NAME}" to the config in HEX.
    set(${CONFIG_NAME} ${FORMATTED_HEX})
endforeach()

file(GLOB TEMPLATE_HEADER_CONFIG_FILES "builtinconfigs/*.cpp.in")
foreach(C ${TEMPLATE_HEADER_CONFIG_FILES})
    get_filename_component(TEMPLATE_HEADER_FULLNAME "${C}" NAME)

    string(LENGTH ${TEMPLATE_HEADER_FULLNAME} TEMPLATE_HEADER_LENGTH)
    MATH(EXPR TEMPLATE_HEADER_LENGTH "${TEMPLATE_HEADER_LENGTH}-3")
    # Get TEMPLATE_HEADER_FULLNAME without .in extension.
    string(SUBSTRING ${TEMPLATE_HEADER_FULLNAME} 0 ${TEMPLATE_HEADER_LENGTH} TEMPLATE_HEADER_FULLNAME_H)

    configure_file(builtinconfigs/${TEMPLATE_HEADER_FULLNAME} ${CONFIGS_HEADER_LOCATION}/${TEMPLATE_HEADER_FULLNAME_H} @ONLY)
endforeach()


set(BUILD_INCLUDES
    ${HEADER_LOCATION}/..
    ${HEADER_LOCATION}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ABI_HEADER_LOCATION}
)

target_include_directories(OpenColorIO
    PUBLIC
        "$<BUILD_INTERFACE:${BUILD_INCLUDES}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    PRIVATE
        "${CONFIGS_HEADER_LOCATION}"
)

target_link_libraries(OpenColorIO
    PRIVATE
        expat::expat
        Imath::Imath
        pystring::pystring
        "$<BUILD_INTERFACE:sampleicc::sampleicc>"
        "$<BUILD_INTERFACE:utils::from_chars>"
        "$<BUILD_INTERFACE:utils::strings>"
        "$<BUILD_INTERFACE:xxHash>"
        yaml-cpp
        minizip-ng::minizip-ng
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
        PUBLIC
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
        PDB_NAME ${PROJECT_NAME}${OCIO_LIBNAME_SUFFIX}_${OpenColorIO_VERSION_MAJOR}_${OpenColorIO_VERSION_MINOR}
    )
endif()

if(WIN32)
    # A windows application linking to eXpat static libraries must
    # have the global macro XML_STATIC defined
    target_compile_definitions(OpenColorIO
        PRIVATE
            XML_STATIC
    )

    if (OCIO_USE_WINDOWS_UNICODE)
        # Add Unicode definitions to use Unicode functions
        target_compile_definitions(OpenColorIO
            PRIVATE
                UNICODE
                _UNICODE
        )
    endif()
endif()

set_target_properties(OpenColorIO PROPERTIES
    OUTPUT_NAME   ${PROJECT_NAME}${OCIO_LIBNAME_SUFFIX}
    COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}"
    VERSION       ${OpenColorIO_VERSION}
    SOVERSION     ${SOVERSION}
    PUBLIC_HEADER "${INSTALL_HEADERS}"
)

if(UNIX AND NOT APPLE)
    # Also hide all the symbols of dependent libraries to prevent clashes if
    # an app using this project is linked against other versions of our
    # dependencies.
    set_property (TARGET OpenColorIO
                  APPEND PROPERTY LINK_FLAGS "-Wl,--exclude-libs,ALL")
elseif(APPLE)
    # Hide the expat symbols.
    set_property (TARGET OpenColorIO
                  APPEND PROPERTY LINK_FLAGS "-Wl,-hidden-lexpat")
endif()

if(MSVC AND BUILD_SHARED_LIBS)
    # Install the pdb file if any.
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
endif()

install(TARGETS OpenColorIO
    EXPORT ${PROJECT_NAME}_EXPORTED_TARGETS
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OpenColorIO
)

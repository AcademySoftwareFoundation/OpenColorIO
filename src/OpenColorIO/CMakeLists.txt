# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
    apphelpers/CategoryHelpers.cpp
    apphelpers/ColorSpaceHelpers.cpp
    apphelpers/DisplayViewHelpers.cpp
    apphelpers/LegacyViewingPipeline.cpp
    apphelpers/mergeconfigs/MergeConfigsHelpers.cpp
    apphelpers/mergeconfigs/OCIOMYaml.cpp
    apphelpers/mergeconfigs/SectionMerger.cpp
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
    ConfigUtils.cpp
    Context.cpp
    ContextVariableUtils.cpp
    CPUInfo.cpp
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
    ops/fixedfunction/ACES2/Transform.cpp
    ops/fixedfunction/FixedFunctionOpCPU.cpp
    ops/fixedfunction/FixedFunctionOpData.cpp
    ops/fixedfunction/FixedFunctionOpGPU.cpp
    ops/fixedfunction/FixedFunctionOp.cpp
    ops/gamma/GammaOpCPU.cpp
    ops/gamma/GammaOpData.cpp
    ops/gamma/GammaOpGPU.cpp
    ops/gamma/GammaOpUtils.cpp
    ops/gamma/GammaOp.cpp
    ops/gradinghuecurve/GradingHueCurve.cpp
    ops/gradinghuecurve/GradingHueCurveOpCPU.cpp
    ops/gradinghuecurve/GradingHueCurveOpData.cpp
    ops/gradinghuecurve/GradingHueCurveOpGPU.cpp
    ops/gradinghuecurve/GradingHueCurveOp.cpp
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
    ops/lut1d/Lut1DOpCPU_SSE2.cpp
    ops/lut1d/Lut1DOpCPU_AVX.cpp
    ops/lut1d/Lut1DOpCPU_AVX2.cpp
    ops/lut1d/Lut1DOpCPU_AVX512.cpp
    ops/lut1d/Lut1DOpData.cpp
    ops/lut1d/Lut1DOpGPU.cpp
    ops/lut3d/Lut3DOp.cpp
    ops/lut3d/Lut3DOpCPU.cpp
    ops/lut3d/Lut3DOpCPU_SSE2.cpp
    ops/lut3d/Lut3DOpCPU_AVX.cpp
    ops/lut3d/Lut3DOpCPU_AVX2.cpp
    ops/lut3d/Lut3DOpCPU_AVX512.cpp
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
    transforms/builtins/AppleCameras.cpp
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
    transforms/GradingHueCurveTransform.cpp
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

# Install the pkg-config file.

set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix "\${prefix}")

# CMAKE_INSTALL_LIBDIR is not guaranteed to be relative.
# Not using cmake_path function since it is only available from CMake ≥ 3.20.
if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
    set(libdir "${CMAKE_INSTALL_LIBDIR}")
else()
    set(libdir "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
endif()

# CMAKE_INSTALL_INCLUDEDIR is not guaranteed to be relative.
# Not using cmake_path function since it is only available from CMake ≥ 3.20.
if(IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
    set(includedir "${CMAKE_INSTALL_INCLUDEDIR}")
else()
    set(includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
endif()

configure_file(res/OpenColorIO.pc.in ${CMAKE_CURRENT_BINARY_DIR}/OpenColorIO.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/OpenColorIO.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

if(OCIO_USE_SIMD AND (OCIO_ARCH_X86 OR OCIO_USE_SSE2NEON))
    # Note that these files are gated by preprocessors to remove them based on the OCIO_USE_* vars.
    set_property(SOURCE ops/lut1d/Lut1DOpCPU_SSE2.cpp APPEND PROPERTY COMPILE_OPTIONS ${OCIO_SSE2_ARGS})
    set_property(SOURCE ops/lut1d/Lut1DOpCPU_AVX.cpp APPEND PROPERTY COMPILE_OPTIONS ${OCIO_AVX_ARGS})
    set_property(SOURCE ops/lut1d/Lut1DOpCPU_AVX2.cpp APPEND PROPERTY COMPILE_OPTIONS ${OCIO_AVX2_ARGS})
    set_property(SOURCE ops/lut1d/Lut1DOpCPU_AVX512.cpp APPEND PROPERTY COMPILE_OPTIONS ${OCIO_AVX512_ARGS})
    set_property(SOURCE ops/lut3d/Lut3DOpCPU_SSE2.cpp APPEND PROPERTY COMPILE_OPTIONS ${OCIO_SSE2_ARGS})
    set_property(SOURCE ops/lut3d/Lut3DOpCPU_AVX.cpp APPEND PROPERTY COMPILE_OPTIONS ${OCIO_AVX_ARGS})
    set_property(SOURCE ops/lut3d/Lut3DOpCPU_AVX2.cpp APPEND PROPERTY COMPILE_OPTIONS ${OCIO_AVX2_ARGS})
    set_property(SOURCE ops/lut3d/Lut3DOpCPU_AVX512.cpp APPEND PROPERTY COMPILE_OPTIONS ${OCIO_AVX512_ARGS})
endif()

configure_file(CPUInfoConfig.h.in CPUInfoConfig.h)

add_library(OpenColorIO ${SOURCES})

# Group the source files to replicate the source tree structure in the IDEs.
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})

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


set(HEADER_LOCATION "${PROJECT_SOURCE_DIR}/include/OpenColorIO")
file(GLOB_RECURSE INSTALL_HEADERS "${HEADER_LOCATION}/*.h")

# Some part of the build process expect to find the ABI header here,
# notably the documentation.
set(ABI_HEADER_LOCATION "${PROJECT_BINARY_DIR}/include/OpenColorIO")
list(APPEND INSTALL_HEADERS ${ABI_HEADER_LOCATION}/OpenColorABI.h)
configure_file("${HEADER_LOCATION}/OpenColorABI.h.in" "${ABI_HEADER_LOCATION}/OpenColorABI.h" @ONLY)

# Built-in configs
set(CONFIGS_HEADER_LOCATION "${PROJECT_BINARY_DIR}/generated_include")
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
    ${CMAKE_CURRENT_BINARY_DIR}
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
        yaml-cpp::yaml-cpp
        MINIZIP::minizip-ng
)

if(OCIO_USE_SIMD AND OCIO_USE_SSE2NEON AND COMPILER_SUPPORTS_SSE_WITH_SSE2NEON)
    target_link_libraries(OpenColorIO PRIVATE $<BUILD_INTERFACE:sse2neon>)
endif()

if(APPLE)
    target_link_libraries(OpenColorIO
        PRIVATE
            "-framework ColorSync"
            "-framework CoreFoundation"
            "-framework CoreGraphics"
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

set(CUSTOM_LINK_FLAGS ${PLATFORM_LINK_OPTIONS})

if(UNIX AND NOT APPLE)
    # Also hide all the symbols of dependent libraries to prevent clashes if
    # an app using this project is linked against other versions of our
    # dependencies.
    set(CUSTOM_LINK_FLAGS "${CUSTOM_LINK_FLAGS};-Wl,--exclude-libs,ALL")
elseif(APPLE)
    if (expat_LIBRARY)
        get_filename_component(_expat_LIBDIR "${expat_LIBRARY}" DIRECTORY)
        # Add the path to CMake list of search paths for libraries.
        list(APPEND _OCIO_LINK_FLAGS_LIST_ "-Wl,-L${_expat_LIBDIR}")
        # Hide the expat symbols.
        list(APPEND _OCIO_LINK_FLAGS_LIST_ "-Wl,-hidden-lexpat")
    endif()


    # Check for minizip first since our Findminizip module sets minizip-ng_LIBRARY.
    if (minizip_LIBRARY OR minizip-ng_COMPAT)
        get_filename_component(_minizip-ng_LIBDIR "${minizip_LIBRARY}" DIRECTORY)
        set(_minizip-ng_NAME "minizip")
    elseif(minizip-ng_LIBRARY)
        get_filename_component(_minizip-ng_LIBDIR "${minizip-ng_LIBRARY}" DIRECTORY)
        set(_minizip-ng_NAME "minizip-ng")
    endif()

    if (_minizip-ng_LIBDIR)
        # Add the path to CMake list of search paths for libraries.
        list(APPEND _OCIO_LINK_FLAGS_LIST_ "-Wl,-L${_minizip-ng_LIBDIR}")
        # Hide the minizip-ng symbols.
        list(APPEND _OCIO_LINK_FLAGS_LIST_ "-Wl,-hidden-l${_minizip-ng_NAME}")
    endif()

    if (_OCIO_LINK_FLAGS_LIST_)
        list(JOIN _OCIO_LINK_FLAGS_LIST_ ";" _OCIO_LINK_FLAGS_LIST_)
        set(CUSTOM_LINK_FLAGS "${CUSTOM_LINK_FLAGS};${_OCIO_LINK_FLAGS_LIST_}")
    endif()
endif()

if (OCIO_USE_SOVERSION)
    set_target_properties(OpenColorIO PROPERTIES
        VERSION       ${OpenColorIO_VERSION}
        SOVERSION     ${SOVERSION}
    )
endif()

set_target_properties(OpenColorIO PROPERTIES
    OUTPUT_NAME   ${PROJECT_NAME}${OCIO_LIBNAME_SUFFIX}
    COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS}"
    LINK_OPTIONS "${CUSTOM_LINK_FLAGS}"
    PUBLIC_HEADER "${INSTALL_HEADERS}"
)

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

# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# Define used for tests in tests/cpu/Context_tests.cpp
add_definitions("-DOCIO_SOURCE_DIR=${PROJECT_SOURCE_DIR}")


macro(add_ocio_test_variant NAME BINARY)
    add_test(NAME ${NAME} COMMAND ${BINARY} ${ARGN})

    if(OCIO_ENABLE_SANITIZER)
        # - Ignore odr-violation warning coming supposedly from compiling OCIO
        # sources within the test target as well as linking to the library.
        # - Provide better stack traces for malloc related leaks.
        set_tests_properties(${NAME} PROPERTIES
            ENVIRONMENT
                "ASAN_OPTIONS=detect_odr_violation=0:fast_unwind_on_malloc=0"
        )
    endif()
endmacro()

function(add_ocio_test NAME SOURCES PRIVATE_INCLUDES)
    set(TEST_BINARY "test_${NAME}_exec")
    set(TEST_NAME "test_${NAME}")
    add_executable(${TEST_BINARY} ${SOURCES})
    target_compile_definitions(${TEST_BINARY}
        PRIVATE
            OpenColorIO_SKIP_IMPORTS
    )
    target_link_libraries(${TEST_BINARY}
        PRIVATE
            expat::expat
            Imath::Imath
            pystring::pystring
            sampleicc::sampleicc
            unittest_data
            utils::strings
            yaml-cpp::yaml-cpp
            testutils
            MINIZIP::minizip-ng
            xxHash
    )

    if(OCIO_USE_SIMD AND OCIO_USE_SSE2NEON AND COMPILER_SUPPORTS_SSE_WITH_SSE2NEON)
        target_link_libraries(${TEST_BINARY} PRIVATE sse2neon)
    endif()

    if(APPLE)
        # Frameworks needed to access the ICC monitor profile.
        target_link_libraries(${TEST_BINARY}
            PRIVATE
                "-framework ColorSync"
                "-framework CoreFoundation"
                "-framework CoreGraphics"
                "-framework IOKit"
        )
    endif(APPLE)

    if(PRIVATE_INCLUDES)
        target_include_directories(${TEST_BINARY}
            PRIVATE
                "$<TARGET_PROPERTY:OpenColorIO,INCLUDE_DIRECTORIES>"
                "${PROJECT_SOURCE_DIR}/tests/cpu"
                "${PROJECT_BINARY_DIR}/generated_include"
        )
    endif(PRIVATE_INCLUDES)

    if(WIN32)
        # A windows application linking to eXpat static libraries must
        # have the global macro XML_STATIC defined
        target_compile_definitions(${TEST_BINARY}
            PRIVATE
                XML_STATIC
        )

        if (OCIO_USE_WINDOWS_UNICODE)
            # Add Unicode definitions to use Unicode functions
            target_compile_definitions(${TEST_BINARY}
                PRIVATE
                    UNICODE
                    _UNICODE
            )
        endif()
    endif(WIN32)

    set_target_properties(${TEST_BINARY} PROPERTIES
        COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS}"
        LINK_OPTIONS "${PLATFORM_LINK_OPTIONS}"
    )

    if(OCIO_USE_SIMD AND (OCIO_ARCH_X86 OR OCIO_USE_SSE2NEON))
        add_ocio_test_variant(${TEST_NAME} ${TEST_BINARY})
        add_ocio_test_variant(${TEST_NAME}_no_accel ${TEST_BINARY} --no_accel)

        if(OCIO_USE_SSE2)
            add_ocio_test_variant(${TEST_NAME}_sse2 ${TEST_BINARY} --sse2)
        endif()

        if(OCIO_USE_AVX)
            add_ocio_test_variant(${TEST_NAME}_avx ${TEST_BINARY} --avx)
            if(OCIO_USE_F16C)
                add_ocio_test_variant(${TEST_NAME}_avx+f16c ${TEST_BINARY} --avx --f16c)
            endif()
        endif()

        if(OCIO_USE_AVX2)
            add_ocio_test_variant(${TEST_NAME}_avx2 ${TEST_BINARY} --avx2)
            if(${OCIO_USE_F16C})
                add_ocio_test_variant(${TEST_NAME}_avx2+f16c ${TEST_BINARY} --avx2 --f16c)
            endif()
        endif()

        if(OCIO_USE_AVX512)
            add_ocio_test_variant(${TEST_NAME}_avx512 ${TEST_BINARY} --avx512)
        endif()
    else()
        add_ocio_test_variant(${TEST_NAME} ${TEST_BINARY})
    endif()

endfunction(add_ocio_test)

# Eventually we will factor out each test into it's own executable
# but for now, we will maintain the status quo and copy all from the
# OpenColorIO target
set(SOURCES
    builtinconfigs/CGConfig.cpp
    builtinconfigs/StudioConfig.cpp
    ConfigUtils.cpp
    fileformats/cdl/CDLParser.cpp
    fileformats/cdl/CDLReaderHelper.cpp
    fileformats/cdl/CDLWriter.cpp
    fileformats/ctf/CTFReaderHelper.cpp
    fileformats/ctf/CTFReaderUtils.cpp
    fileformats/FileFormatUtils.cpp
    fileformats/xmlutils/XMLReaderHelper.cpp
    fileformats/xmlutils/XMLWriterUtils.cpp
    BakingUtils.cpp
    CPUInfo.cpp
    GPUProcessor.cpp
    GpuShaderDesc.cpp
    GpuShaderClassWrapper.cpp
    HashUtils.cpp
    ImageDesc.cpp
    ImagePacking.cpp
    Look.cpp
    OCIOYaml.cpp
    OCIOZArchive.cpp
    ops/cdl/CDLOpCPU.cpp
    ops/cdl/CDLOpGPU.cpp
    ops/exposurecontrast/ExposureContrastOpGPU.cpp
    ops/fixedfunction/ACES2/Transform.cpp
    ops/fixedfunction/FixedFunctionOpGPU.cpp
    ops/gamma/GammaOpGPU.cpp
    ops/gradingprimary/GradingPrimaryOpGPU.cpp
    ops/gradingrgbcurve/GradingRGBCurveOpGPU.cpp
    ops/gradingtone/GradingToneOpGPU.cpp
    ops/log/LogOpGPU.cpp
    ops/lut1d/Lut1DOpCPU_SSE2.cpp
    ops/lut1d/Lut1DOpCPU_AVX.cpp
    ops/lut1d/Lut1DOpCPU_AVX2.cpp
    ops/lut1d/Lut1DOpCPU_AVX512.cpp
    ops/lut3d/Lut3DOpGPU.cpp
    ops/lut3d/Lut3DOpCPU_SSE2.cpp
    ops/lut3d/Lut3DOpCPU_AVX.cpp
    ops/lut3d/Lut3DOpCPU_AVX2.cpp
    ops/lut3d/Lut3DOpCPU_AVX512.cpp
    ops/matrix/MatrixOpGPU.cpp
    ops/OpTools.cpp
    ops/range/RangeOpGPU.cpp
    ScanlineHelper.cpp
    Transform.cpp
    transforms/builtins/ACES.cpp
    transforms/builtins/AppleCameras.cpp
    transforms/builtins/ArriCameras.cpp
    transforms/builtins/CanonCameras.cpp
    transforms/builtins/ColorMatrixHelpers.cpp
    transforms/builtins/Displays.cpp
    transforms/builtins/OpHelpers.cpp
    transforms/builtins/PanasonicCameras.cpp
    transforms/builtins/RedCameras.cpp
    transforms/builtins/SonyCameras.cpp
    SystemMonitor.cpp
)

set(TESTS
    apphelpers/CategoryHelpers_tests.cpp
    apphelpers/ColorSpaceHelpers_tests.cpp
    apphelpers/DisplayViewHelpers_tests.cpp
    apphelpers/LegacyViewingPipeline_tests.cpp
    apphelpers/MixingHelpers_tests.cpp
    Baker_tests.cpp
    BitDepthUtils_tests.cpp
    builtinconfigs/BuiltinConfig_tests.cpp
    Caching_tests.cpp
    ColorSpace_tests.cpp
    ColorSpaceSet_tests.cpp
    Config_tests.cpp
    Context_tests.cpp
    ContextVariableUtils_tests.cpp
    CPUProcessor_tests.cpp
    Display_tests.cpp
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
    NamedTransform_tests.cpp
    OCIOZArchive_tests.cpp
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
    ops/gradingprimary/GradingPrimary_tests.cpp
    ops/gradingprimary/GradingPrimaryOpCPU_tests.cpp
    ops/gradingprimary/GradingPrimaryOpData_tests.cpp
    ops/gradingprimary/GradingPrimaryOp_tests.cpp
    ops/gradingrgbcurve/GradingBSplineCurve_tests.cpp
    ops/gradingrgbcurve/GradingRGBCurve_tests.cpp
    ops/gradingrgbcurve/GradingRGBCurveOpCPU_tests.cpp
    ops/gradingrgbcurve/GradingRGBCurveOpData_tests.cpp
    ops/gradingrgbcurve/GradingRGBCurveOp_tests.cpp
    ops/gradingtone/GradingTone_tests.cpp
    ops/gradingtone/GradingToneOpCPU_tests.cpp
    ops/gradingtone/GradingToneOpData_tests.cpp
    ops/gradingtone/GradingToneOp_tests.cpp
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
    SIMD_tests.cpp
    SSE_tests.cpp
    SSE2_tests.cpp
    AVX_tests.cpp
    AVX2_tests.cpp
    AVX512_tests.cpp
    transforms/AllocationTransform_tests.cpp
    transforms/builtins/BuiltinTransformRegistry_tests.cpp
    transforms/BuiltinTransform_tests.cpp
    transforms/CDLTransform_tests.cpp
    transforms/ColorSpaceTransform_tests.cpp
    transforms/DisplayViewTransform_tests.cpp
    transforms/ExponentTransform_tests.cpp
    transforms/ExponentWithLinearTransform_tests.cpp
    transforms/ExposureContrastTransform_tests.cpp
    transforms/FileTransform_tests.cpp
    transforms/FixedFunctionTransform_tests.cpp
    transforms/GradingPrimaryTransform_tests.cpp
    transforms/GradingRGBCurveTransform_tests.cpp
    transforms/GradingToneTransform_tests.cpp
    transforms/GroupTransform_tests.cpp
    transforms/LogAffineTransform_tests.cpp
    transforms/LogCameraTransform_tests.cpp
    transforms/LogTransform_tests.cpp
    transforms/LookTransform_tests.cpp
    transforms/Lut1DTransform_tests.cpp
    transforms/Lut3DTransform_tests.cpp
    transforms/MatrixTransform_tests.cpp
    transforms/RangeTransform_tests.cpp
    UnitTestLogUtils.cpp
    UnitTestMain.cpp
    UnitTestOptimFlags.cpp
    UnitTestUtils.cpp
    ViewingRules_tests.cpp
    ViewTransform_tests.cpp
)

function(prepend var prefix)
    set(new "")
    foreach(f ${ARGN})
        list(APPEND new "${prefix}${f}")
    endforeach(f)
    set(${var} "${new}" PARENT_SCOPE)
endfunction(prepend)

prepend(SOURCES "${PROJECT_SOURCE_DIR}/src/OpenColorIO/" ${SOURCES})

list(APPEND SOURCES ${TESTS})

if(OCIO_USE_SIMD AND (OCIO_ARCH_X86 OR OCIO_USE_SSE2NEON))
    # Note that these files are gated by preprocessors to remove them based on the OCIO_USE_* vars.
    set_property(SOURCE "${CMAKE_SOURCE_DIR}/src/OpenColorIO/ops/lut1d/Lut1DOpCPU_SSE2.cpp" APPEND PROPERTY COMPILE_OPTIONS ${OCIO_SSE2_ARGS})
    set_property(SOURCE "${CMAKE_SOURCE_DIR}/src/OpenColorIO/ops/lut1d/Lut1DOpCPU_AVX.cpp" APPEND PROPERTY COMPILE_OPTIONS ${OCIO_AVX_ARGS})
    set_property(SOURCE "${CMAKE_SOURCE_DIR}/src/OpenColorIO/ops/lut1d/Lut1DOpCPU_AVX2.cpp" APPEND PROPERTY COMPILE_OPTIONS ${OCIO_AVX2_ARGS})
    set_property(SOURCE "${CMAKE_SOURCE_DIR}/src/OpenColorIO/ops/lut1d/Lut1DOpCPU_AVX512.cpp" APPEND PROPERTY COMPILE_OPTIONS ${OCIO_AVX512_ARGS})
    set_property(SOURCE "${CMAKE_SOURCE_DIR}/src/OpenColorIO/ops/lut3d/Lut3DOpCPU_SSE2.cpp" APPEND PROPERTY COMPILE_OPTIONS ${OCIO_SSE2_ARGS})
    set_property(SOURCE "${CMAKE_SOURCE_DIR}/src/OpenColorIO/ops/lut3d/Lut3DOpCPU_AVX.cpp" APPEND PROPERTY COMPILE_OPTIONS ${OCIO_AVX_ARGS})
    set_property(SOURCE "${CMAKE_SOURCE_DIR}/src/OpenColorIO/ops/lut3d/Lut3DOpCPU_AVX2.cpp" APPEND PROPERTY COMPILE_OPTIONS ${OCIO_AVX2_ARGS})
    set_property(SOURCE "${CMAKE_SOURCE_DIR}/src/OpenColorIO/ops/lut3d/Lut3DOpCPU_AVX512.cpp" APPEND PROPERTY COMPILE_OPTIONS ${OCIO_AVX512_ARGS})
    set_property(SOURCE "SSE2_tests.cpp" APPEND PROPERTY COMPILE_OPTIONS ${OCIO_SSE2_ARGS})
    set_property(SOURCE "AVX_tests.cpp" APPEND PROPERTY COMPILE_OPTIONS ${OCIO_AVX_ARGS})
    set_property(SOURCE "AVX2_tests.cpp" APPEND PROPERTY COMPILE_OPTIONS ${OCIO_AVX2_ARGS})
    set_property(SOURCE "AVX512_tests.cpp" APPEND PROPERTY COMPILE_OPTIONS ${OCIO_AVX512_ARGS})
endif()

add_ocio_test(cpu "${SOURCES}" TRUE)

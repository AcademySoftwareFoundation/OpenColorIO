# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate the dxcompiler.dll + dxil.dll runtime pair needed to run D3D12 shader
# compilation at test time, and surface their file version.
#
# Inputs:
#   OCIO_DXCOMPILER_DLL  - Optional user-supplied path to dxcompiler.dll.
#                          When set, overrides the Windows SDK Redist/D3D
#                          search. Useful when the SDK-bundled DLL is too old.
#
# Outputs:
#   DXCOMPILER_DLL       - Path to dxcompiler.dll (cache variable).
#   DXIL_DLL             - Path to the adjacent dxil.dll (cache variable, may
#                          be left unset if not found next to dxcompiler.dll).

set(OCIO_DXCOMPILER_DLL "" CACHE FILEPATH
    "Optional explicit path to dxcompiler.dll (e.g. from a newer DirectX Shader Compiler release). \
Overrides the automatic Windows SDK Redist/D3D search."
)

if(OCIO_DXCOMPILER_DLL)
    if(NOT EXISTS "${OCIO_DXCOMPILER_DLL}")
        message(FATAL_ERROR "OCIO_DXCOMPILER_DLL=${OCIO_DXCOMPILER_DLL} does not exist.")
    endif()
    set(DXCOMPILER_DLL "${OCIO_DXCOMPILER_DLL}" CACHE FILEPATH
        "Path to dxcompiler.dll (user-supplied via OCIO_DXCOMPILER_DLL)" FORCE)
else()
    find_file(DXCOMPILER_DLL
        NAMES dxcompiler.dll
        PATHS
            # Note: x64 hardcoded; update if ARM64 Windows support is needed.
            "$ENV{WindowsSdkDir}Redist/D3D/x64"
            "C:/Program Files (x86)/Windows Kits/10/Redist/D3D/x64"
        NO_DEFAULT_PATH
        DOC "Path to dxcompiler.dll from Windows SDK"
    )
endif()

if(DXCOMPILER_DLL)
    get_filename_component(_dxc_dll_dir "${DXCOMPILER_DLL}" DIRECTORY)
    find_file(DXIL_DLL
        NAMES dxil.dll
        HINTS "${_dxc_dll_dir}"
        NO_DEFAULT_PATH
    )

    # Report the found dxcompiler.dll version so crash reports can identify
    # mismatched or outdated DXC builds without re-running diagnostics.
    string(REPLACE "'" "''" _dxc_dll_ps "${DXCOMPILER_DLL}")
    execute_process(
        COMMAND powershell -NoProfile -Command
            "(Get-Item -LiteralPath '${_dxc_dll_ps}').VersionInfo.FileVersion"
        OUTPUT_VARIABLE _dxc_version
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(_dxc_version)
        message(STATUS "Found dxcompiler.dll (version ${_dxc_version}): ${DXCOMPILER_DLL}")
    else()
        message(STATUS "Found dxcompiler.dll (version unknown): ${DXCOMPILER_DLL}")
    endif()
    message(STATUS
        "If test_dx crashes during shader compilation, the Windows SDK's dxcompiler.dll "
        "may be too old to produce signed DXIL on this system. Replace it with a newer "
        "build from https://github.com/microsoft/DirectXShaderCompiler/releases, or set "
        "-DOCIO_DXCOMPILER_DLL=<path> to point at a specific dxcompiler.dll."
    )
else()
    message(WARNING
        "OCIO_DIRECTX_ENABLED is ON but dxcompiler.dll was not found in the "
        "Windows SDK Redist/D3D path. test_dx will fail at runtime unless "
        "dxcompiler.dll and dxil.dll are on PATH. Install the Windows SDK "
        "redistributable components, set -DOCIO_DXCOMPILER_DLL=<path> to supply "
        "a specific dxcompiler.dll, or set -DOCIO_DIRECTX_ENABLED=OFF to "
        "disable the DirectX 12 backend."
    )
endif()

# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

$cmakeVersion = $Args[0]
$cmakeMajorMinor = [io.path]::GetFileNameWithoutExtension("$cmakeVersion")

Invoke-WebRequest "https://cmake.org/files/v${cmakeMajorMinor}/cmake-${cmakeVersion}-win64-x64.zip" -OutFile "C:\cmake-${cmakeVersion}-win64-x64.zip"
Expand-Archive "C:\cmake-${cmakeVersion}-win64-x64.zip" -DestinationPath C:\
Rename-Item -Path "C:\cmake-${cmakeVersion}-win64-x64" -NewName C:\_cmake
Write-Host "##vso[task.prependpath]C:\_cmake\bin"

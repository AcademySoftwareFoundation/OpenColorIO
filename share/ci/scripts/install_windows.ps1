param (
    [Parameter(Mandatory=$true)][string]$PythonVersion,
    [Parameter(Mandatory=$true)][string]$CMakeVersion
)

$cmakeMajorMinor = [io.path]::GetFileNameWithoutExtension("$CMakeVersion")

# Install Python
Invoke-WebRequest https://www.python.org/ftp/python/${PythonVersion}/python-${PythonVersion}.amd64.msi -OutFile C:\python-${PythonVersion}.amd64.msi
msiexec /i C:\python-${PythonVersion}.amd64.msi /quiet /l* C:\_python.log TARGETDIR=C:\_python
Write-Host "##vso[task.prependpath]C:\_python"
Write-Host "##vso[task.prependpath]C:\_python\Scripts"

# Install CMake
Invoke-WebRequest https://cmake.org/files/v${cmakeMajorMinor}/cmake-${CMakeVersion}-win64-x64.zip -OutFile C:\cmake-${CMakeVersion}-win64-x64.zip
Expand-Archive C:\cmake-${CMakeVersion}-win64-x64.zip -DestinationPath C:\
Rename-Item -Path C:\cmake-${CMakeVersion}-win64-x64 -NewName C:\_cmake
Write-Host "##vso[task.prependpath]C:\_cmake\bin"
